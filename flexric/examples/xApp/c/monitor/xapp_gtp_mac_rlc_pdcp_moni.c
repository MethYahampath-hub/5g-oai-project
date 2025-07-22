/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

 /*
 ==== TO DO ====
  - Pass validation
  - Clean up file and remove unnecessary code
 */

#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/alg_ds/alg/defer.h"
#include "../../../../src/util/time_now_us.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>


static
uint64_t cnt_mac;

typedef struct {
    int rlc_pkt_loss;
    int rlc_pkt_total;
    int pdcp_pkt_loss;
    int pdcp_pkt_total;
  } packet_stats;

  packet_stats tx_stats = {
    .rlc_pkt_loss = 0,
    .rlc_pkt_total = 0,
    .pdcp_pkt_loss = 0,
    .pdcp_pkt_total = 0,
  };

  packet_stats rx_stats = {
    .rlc_pkt_loss = 0,
    .rlc_pkt_total = 0,
    .pdcp_pkt_loss = 0,
    .pdcp_pkt_total = 0,
  };

int *pdcp_txpdu_sn_arr = NULL;
size_t pdcp_txpdu_arr_size = 0;
size_t pdcp_txpdu_arr_capacity = 0;
int pdcp_txpdu_sn_last = 0;
int pdcp_txpdu_sn_first = 0;
int pdcp_txpdu_pkt_count;

int *pdcp_rxpdu_sn_arr = NULL;
size_t pdcp_rxpdu_arr_size = 0;
size_t pdcp_rxpdu_arr_capacity = 0;
int pdcp_rxpdu_sn_last = 0;
int pdcp_rxpdu_sn_first = 0;
int pdcp_rxpdu_pkt_count;

// static
// void sm_cb_mac(sm_ag_if_rd_t const* rd)
// {
//   assert(rd != NULL);
//   assert(rd->type ==INDICATION_MSG_AGENT_IF_ANS_V0);
//   assert(rd->ind.type == MAC_STATS_V0);
 
//   int64_t now = time_now_us();
//   // if(cnt_mac % 1024 == 0)
//   //   printf("MAC ind_msg latency = %ld μs\n", now - rd->ind.mac.msg.tstamp);
//   cnt_mac++;
// }

static
uint64_t cnt_rlc;

static
void sm_cb_rlc(sm_ag_if_rd_t const* rd)
{
  assert(rd != NULL);
  assert(rd->type ==INDICATION_MSG_AGENT_IF_ANS_V0);

  assert(rd->ind.type == RLC_STATS_V0);

  int64_t now = time_now_us();


  const rlc_ind_msg_t* msg = &rd->ind.rlc.msg;

  for (size_t i = 0; i < msg->len; ++i) {
    const rlc_radio_bearer_stats_t* rb = &msg->rb[i];

    if(cnt_rlc % 1024 == 0) {
      // printf("RLC ind_msg latency = %ld μs\n", now - rd->ind.rlc.msg.tstamp);
      // printf("[RLC PDU] TX PDUs = %u, RX PDUs = %u, TX PDU Packets Dropped = %u, RX PDU Packets Dropped = %u\n",
      //        rb->txpdu_pkts,      // aggregated number of transmitted RLC PDUs
      //        rb->rxpdu_pkts,      // aggregated number of received RLC PDUs 
      //        rb->txpdu_dd_pkts,   // aggregated number of dropped or discarded tx packets by RLC
      //        rb->rxpdu_dd_pkts);  // aggregated number of rx packets dropped or discarded by RLC
      // printf("[RLC SDU] TX SDUs = %u, RX SDUs = %u, RX SDU Packets Dropped = %u\n",
      //        rb->txsdu_pkts,      // number of SDUs delivered
      //        rb->rxsdu_pkts,      // number of SDUs received
      //        rb->rxsdu_dd_pkts);  // number of dropped or discarded SDUs
    }
    tx_stats.rlc_pkt_loss = rb->txpdu_dd_pkts; // + txsdu_dd_pkts
    tx_stats.rlc_pkt_total = rb->txpdu_pkts; // + txsdu_pkts
    rx_stats.rlc_pkt_loss = rb->rxpdu_dd_pkts + rb->rxsdu_dd_pkts;
    rx_stats.rlc_pkt_total = rb->rxpdu_pkts + rb->rxsdu_pkts;
  }
  cnt_rlc++;
}

static
uint64_t cnt_pdcp;

// the following function has been modified extensively
// original implementation can be found in FlexRIC repository
void sm_cb_pdcp(sm_ag_if_rd_t const* rd)
{
  assert(rd != NULL);
  assert(rd->type == INDICATION_MSG_AGENT_IF_ANS_V0);
  assert(rd->ind.type == PDCP_STATS_V0);

  int64_t now = time_now_us();

  

  const pdcp_ind_msg_t* msg = &rd->ind.pdcp.msg;

  for (size_t i = 0; i < msg->len; ++i) {
    const pdcp_radio_bearer_stats_t* rb = &msg->rb[i];

    if (cnt_pdcp % 1024 == 0) {
      printf("PDCP ind_msg latency = %ld μs\n", now - rd->ind.pdcp.msg.tstamp);
      printf("[PDCP SDU] UE RNTI %u RBID %u: TX SDUs = %u, RX SDUs = %u, RX Dup Discards = %u\n",
             rb->rnti,
             rb->rbid,
             rb->txsdu_pkts,
             rb->rxsdu_pkts,
             rb->rxpdu_dd_pkts);
    }
    printf("Last seqence number = %u\n", rb->txpdu_sn);
    // packet lost sn calculation, tx
    if (rb->txpdu_sn < pdcp_txpdu_sn_last) {
      bool new_tx_sn = true;
      for (int i = 0; i < pdcp_txpdu_arr_size; ++i) {
        if (rb->txpdu_sn == pdcp_txpdu_sn_arr[i]) { new_tx_sn = false; }
      }
      if (new_tx_sn == true) {
        if (pdcp_txpdu_arr_size >= pdcp_txpdu_arr_capacity) {
          pdcp_txpdu_arr_capacity = pdcp_txpdu_arr_capacity == 0 ? 1 : pdcp_txpdu_arr_capacity * 2;
          pdcp_txpdu_sn_arr = realloc(pdcp_txpdu_sn_arr, pdcp_txpdu_arr_capacity * sizeof(int));
        }
        pdcp_txpdu_sn_arr[pdcp_txpdu_arr_size++] = rb->txpdu_sn;
      }
    }
    pdcp_txpdu_sn_last = rb->txpdu_sn;
    if (pdcp_txpdu_sn_first == 0) { pdcp_txpdu_sn_first = rb->txpdu_sn; }
    pdcp_txpdu_pkt_count = rb->txpdu_sn - pdcp_txpdu_sn_first;

    // packet lost sn calculation, rx
    if (rb->rxpdu_sn < pdcp_rxpdu_sn_last) {
      bool new_rx_sn = true;
      for (int i = 0; i < pdcp_rxpdu_arr_size; ++i) {
        if (rb->rxpdu_sn == pdcp_rxpdu_sn_arr[i]) { new_rx_sn = false; }
      }
      if (new_rx_sn == true) {
        if (pdcp_rxpdu_arr_size >= pdcp_rxpdu_arr_capacity) {
          pdcp_rxpdu_arr_capacity = pdcp_rxpdu_arr_capacity == 0 ? 1 : pdcp_rxpdu_arr_capacity * 2;
          pdcp_rxpdu_sn_arr = realloc(pdcp_rxpdu_sn_arr, pdcp_rxpdu_arr_capacity * sizeof(int));
        }
        pdcp_rxpdu_sn_arr[pdcp_rxpdu_arr_size++] = rb->rxpdu_sn;
      }
    }
    pdcp_rxpdu_sn_last = rb->rxpdu_sn;
    if (pdcp_rxpdu_sn_first == 0) { pdcp_rxpdu_sn_first = rb->rxpdu_sn; }
    pdcp_rxpdu_pkt_count = rb->rxpdu_sn - pdcp_rxpdu_sn_first;
    // rx_stats.pdcp_pkt_loss = rb->rxpdu_dd_pkts;
    // rx_stats.pdcp_pkt_total = rb->rxpdu_pkts;
  }
  // tx_stats.pdcp_pkt_loss = txpdu_dd_pkts;
  // tx_stats.pdcp_pkt_total = txpdu_pkts;
  cnt_pdcp++;
}
  

// static
// uint64_t cnt_gtp;

// static
// void sm_cb_gtp(sm_ag_if_rd_t const* rd)
// {
//   assert(rd != NULL);
//   assert(rd->type ==INDICATION_MSG_AGENT_IF_ANS_V0);

//   assert(rd->ind.type == GTP_STATS_V0);

//   int64_t now = time_now_us();
//   // if(cnt_gtp % 1024 == 0) {
//   //   printf("GTP ind_msg latency = %ld μs\n", now - rd->ind.gtp.msg.tstamp);
//   // }
//   cnt_gtp++;
// }


int main(int argc, char *argv[])
{
  // printf("🚨===== THIS IS THE NEW BUILD ====🚨\n"); // REMOVE AFTER DEBUG

  fr_args_t args = init_fr_args(argc, argv);

  //Init the xApp
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  defer({ free_e2_node_arr_xapp(&nodes); });

  assert(nodes.len > 0);

  printf("Connected E2 nodes = %d\n", nodes.len);

  // // MAC indication
  // const char* i_0 = "1_ms";
  // sm_ans_xapp_t* mac_handle = NULL;
  // RLC indication
  const char* i_1 = "1_ms";
  sm_ans_xapp_t* rlc_handle = NULL;
  // PDCP indication
  const char* i_2 = "10_ms";
  sm_ans_xapp_t* pdcp_handle = NULL;
  // // GTP indication
  // const char* i_3 = "1_ms";
  // sm_ans_xapp_t* gtp_handle = NULL;


  if(nodes.len > 0){
    // mac_handle = calloc( nodes.len, sizeof(sm_ans_xapp_t) ); 
    // assert(mac_handle  != NULL);
    rlc_handle = calloc( nodes.len, sizeof(sm_ans_xapp_t) ); 
    assert(rlc_handle  != NULL);
    pdcp_handle = calloc( nodes.len, sizeof(sm_ans_xapp_t) ); 
    assert(pdcp_handle  != NULL);
    // gtp_handle = calloc( nodes.len, sizeof(sm_ans_xapp_t) ); 
    // assert(gtp_handle  != NULL);
  }

  for (int i = 0; i < nodes.len; i++) {
    e2_node_connected_xapp_t* n = &nodes.n[i];
    for (size_t j = 0; j < n->len_rf; j++)
      printf("Registered node %d ran func id = %d \n ", i, n->rf[j].id);

    if(n->id.type == ngran_gNB || n->id.type == ngran_eNB){
      // MAC Control is not yet implemented in OAI RAN
      // mac_ctrl_req_data_t wr = {.hdr.dummy = 1, .msg.action = 42 };
      // sm_ans_xapp_t const a = control_sm_xapp_api(&nodes.n[i].id, 142, &wr);
      // assert(a.success == true);
      // printf("==== ENTERED FIRST BRANCH ====");
      // mac_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 142, (void*)i_0, sm_cb_mac);
      // assert(mac_handle[i].success == true);

      rlc_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 143, (void*)i_1, sm_cb_rlc);
      assert(rlc_handle[i].success == true);

      pdcp_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 144, (void*)i_2, sm_cb_pdcp);
      assert(pdcp_handle[i].success == true);

      /*      
      pdcp_meas.measurement_types = MEAS_TYPE_PDCP_DISCARDED_SDU_COUNT;
      pdcp_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 144, (void*)i_2, sm_cb_pdcp);
      assert(pdcp_handle[i].success == true);
      */
      // gtp_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 148, (void*)i_3, sm_cb_gtp);
      // assert(gtp_handle[i].success == true);

    } else if(n->id.type ==  ngran_gNB_CU || n->id.type ==  ngran_gNB_CUUP){
      // printf("==== ENTERED SECOND BRANCH ====");
      
      pdcp_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 144, (void*)i_2, sm_cb_pdcp);
      assert(pdcp_handle[i].success == true);

      // gtp_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 148, (void*)i_3, sm_cb_gtp);
      // assert(gtp_handle[i].success == true);

    } else if(n->id.type == ngran_gNB_DU){
      // printf("==== ENTERED THIRD BRANCH ====");
      // mac_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 142, (void*)i_0, sm_cb_mac);
      // assert(mac_handle[i].success == true);

      rlc_handle[i] = report_sm_xapp_api(&nodes.n[i].id, 143, (void*)i_1, sm_cb_rlc);
      assert(rlc_handle[i].success == true);
    }

  }

  sleep(10);


  for(int i = 0; i < nodes.len; ++i){
    // Remove the handle previously returned
    // if(mac_handle[i].u.handle != 0 )
    //   rm_report_sm_xapp_api(mac_handle[i].u.handle);
    if(rlc_handle[i].u.handle != 0) 
      rm_report_sm_xapp_api(rlc_handle[i].u.handle);
    if(pdcp_handle[i].u.handle != 0)
      rm_report_sm_xapp_api(pdcp_handle[i].u.handle);
    // if(gtp_handle[i].u.handle != 0)
    //   rm_report_sm_xapp_api(gtp_handle[i].u.handle);
  }

  if(nodes.len > 0){
    // free(mac_handle);
    free(rlc_handle);
    free(pdcp_handle);
    // free(gtp_handle);
  }
  tx_stats.pdcp_pkt_loss = pdcp_txpdu_arr_size;
  tx_stats.pdcp_pkt_total = pdcp_txpdu_pkt_count; // counts total packets as total # unique SNs

  int tx_pdcp_pkt_loss = tx_stats.pdcp_pkt_loss;
  int tx_pdcp_pkt_total = tx_stats.pdcp_pkt_total;

  int rx_pdcp_pkt_loss = rx_stats.pdcp_pkt_loss;
  int rx_pdcp_pkt_total = rx_stats.pdcp_pkt_total;

  int tx_rlc_pkt_loss = tx_stats.rlc_pkt_loss;
  int tx_rlc_pkt_total = tx_stats.rlc_pkt_total;

  int rx_rlc_pkt_loss = rx_stats.rlc_pkt_loss;
  int rx_rlc_pkt_total = rx_stats.rlc_pkt_total;

  // double tx_pkt_lost = tx_pdcp_pkt_loss + tx_rlc_pkt_loss;
  // double tx_pkt_total = tx_pdcp_pkt_total + tx_rlc_pkt_total;
  // double tx_packet_loss_fig = tx_pkt_lost / tx_pkt_total;

  // double rx_pkt_lost = rx_pdcp_pkt_loss + rx_rlc_pkt_loss;
  // double rx_pkt_total = rx_pdcp_pkt_total + rx_rlc_pkt_total;
  // double rx_packet_loss_fig = rx_pkt_lost / rx_pkt_total;

  double tx_pdcp_pkt_loss_fig = (double)tx_pdcp_pkt_loss / (double)tx_pdcp_pkt_total;
  if (tx_pdcp_pkt_total == 0) { tx_pdcp_pkt_loss_fig = 0; }
  double rx_pdcp_pkt_loss_fig = (double)rx_pdcp_pkt_loss / (double)rx_pdcp_pkt_total;
  if (rx_pdcp_pkt_total == 0) { rx_pdcp_pkt_loss_fig = 0; }

  double tx_rlc_pkt_loss_fig = (double)tx_rlc_pkt_loss / (double)tx_rlc_pkt_total;
  if (tx_rlc_pkt_total == 0) { tx_rlc_pkt_loss_fig = 0; }
  double rx_rlc_pkt_loss_fig = (double)rx_rlc_pkt_loss / (double)rx_rlc_pkt_total;
  if (rx_rlc_pkt_total == 0) { rx_rlc_pkt_loss_fig = 0; }

  printf("[PDCP] DOWNLINK PACKET LOSS = %f, UPLINK PACKET LOSS = %f\n", 
         tx_pdcp_pkt_loss_fig, rx_pdcp_pkt_loss_fig);
  
  printf("[RLC] DOWNLINK PACKET LOSS = %f, UPLINK PACKET LOSS = %f\n", 
         tx_rlc_pkt_loss_fig, rx_rlc_pkt_loss_fig);

  printf("DOWNLINK PDCP LOST: %d, DOWNLINK PDCP TOTAL: %d\n",
         tx_pdcp_pkt_loss, tx_pdcp_pkt_total);
  printf("DOWNLINK RLC LOST: %d, DOWNLINK RLC TOTAL: %d\n",
         tx_rlc_pkt_loss, tx_rlc_pkt_total);
  // printf("DOWNLINK TOTAL LOST: %f, DOWNLINK TOTAL TOTAL: %f\n", 
  //        tx_pkt_lost, tx_pkt_total);

  // printf("UPLINK PDCP LOST: %d, UPLINK PDCP TOTAL: %d\n",
  //        rx_pdcp_pkt_loss, rx_pdcp_pkt_total);
  // printf("UPLINK RLC LOST: %d, UPLINK RLC TOTAL: %d\n",
  //        rx_rlc_pkt_loss, rx_rlc_pkt_total);
  // printf("UPLINK TOTAL LOST: %f, UPLINK TOTAL TOTAL: %f\n", 
  //        rx_pkt_lost, rx_pkt_total);

  // printf("[DEBUG RAW COUNTS] PDCP_LOST=%.6f, RLC_LOST=%.6f\n",
  //      tx_pdcp_pkt_loss, tx_rlc_pkt_loss);
  // printf("COMPUTED TOTAL LOST = %.6f\n", tx_pkt_lost);

  //Stop the xApp
  while(try_stop_xapp_api() == false)
    usleep(1000);

  printf("Test xApp run SUCCESSFULLY\n");
}



