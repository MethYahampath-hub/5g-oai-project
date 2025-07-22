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

/*! \file fapi/oai-integration/fapi_vnf_p7.c
* \brief OAI FAPI VNF P7 message handler procedures
* \author Ruben S. Silva
* \date 2023
* \version 0.1
* \company OpenAirInterface Software Alliance
* \email: contact@openairinterface.org, rsilva@allbesmart.pt
* \note
* \warning
 */

#include "fapi_vnf_p7.h"
#include "nr_nfapi_p7.h"
#include "nr_fapi_p7.h"
#include "nr_fapi_p7_utils.h"

#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h" // for handle_nr_srs_measurements()

extern pthread_cond_t nfapi_sync_cond;
extern pthread_mutex_t nfapi_sync_mutex;
extern int nfapi_sync_var;

int aerial_phy_nr_crc_indication(nfapi_nr_crc_indication_t *ind)
{
  nfapi_nr_crc_indication_t *crc_ind = CALLOC(1, sizeof(*crc_ind));
  copy_crc_indication(ind, crc_ind);
  if (!put_queue(&gnb_crc_ind_queue, crc_ind)) {
    LOG_E(NR_MAC, "Put_queue failed for crc_ind\n");
    free_crc_indication(crc_ind);
    free(crc_ind);
  }
  return 1;
}

int aerial_phy_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind)
{
  nfapi_nr_rx_data_indication_t *rx_ind = CALLOC(1, sizeof(*rx_ind));
  copy_rx_data_indication(ind, rx_ind);
  if (!put_queue(&gnb_rx_ind_queue, rx_ind)) {
    LOG_E(NR_MAC, "Put_queue failed for rx_ind\n");
    free_rx_data_indication(rx_ind);
    free(rx_ind);
  }
  return 1;
}

int aerial_phy_nr_rach_indication(nfapi_nr_rach_indication_t *ind)
{
  nfapi_nr_rach_indication_t *rach_ind = CALLOC(1, sizeof(*rach_ind));
  copy_rach_indication(ind, rach_ind);
  if (!put_queue(&gnb_rach_ind_queue, rach_ind)) {
    LOG_E(NR_MAC, "Put_queue failed for rach_ind\n");
    free_rach_indication(rach_ind);
    free(rach_ind);
  } else {
    LOG_I(NR_MAC, "RACH.indication put_queue successfull\n");
  }
  return 1;
}

int aerial_phy_nr_uci_indication(nfapi_nr_uci_indication_t *ind)
{
  nfapi_nr_uci_indication_t *uci_ind = CALLOC(1, sizeof(*uci_ind));
  AssertFatal(uci_ind, "Memory not allocated for uci_ind in phy_nr_uci_indication.");
  copy_uci_indication(ind, uci_ind);
  if (!put_queue(&gnb_uci_ind_queue, uci_ind)) {
    LOG_E(NR_MAC, "Put_queue failed for uci_ind\n");
    free_uci_indication(uci_ind);
    free(uci_ind);
    uci_ind = NULL;
  }
  return 1;
}

void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frame, slot_t slot, NR_Sched_Rsp_t* sched_info);
int oai_fapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req);
int oai_fapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req);
int oai_fapi_tx_data_req(nfapi_nr_tx_data_request_t* tx_data_req);
int oai_fapi_ul_dci_req(nfapi_nr_ul_dci_request_t* ul_dci_req);
int oai_fapi_send_end_request(int cell, uint32_t frame, uint32_t slot);
extern int trigger_scheduler(nfapi_nr_slot_indication_scf_t *slot_ind);

int aerial_phy_nr_slot_indication(nfapi_nr_slot_indication_scf_t *ind)
{
  LOG_D(MAC, "VNF SFN/Slot %d.%d \n", ind->sfn, ind->slot);
  trigger_scheduler(ind);

  return 1;
}

int aerial_phy_nr_srs_indication(nfapi_nr_srs_indication_t *ind)
{
  // or queue to decouple, but I think it should be fast (in all likelihood,
  // the VNF has nothing to do)
  for (int i = 0; i < ind->number_of_pdus; ++i)
    handle_nr_srs_measurements(0, ind->sfn, ind->slot, &ind->pdu_list[i]);
  return 1;
}

void *aerial_vnf_allocate(size_t size)
{
  // return (void*)memory_pool::allocate(size);
  return (void *)malloc(size);
}

void aerial_vnf_deallocate(void *ptr)
{
  // memory_pool::deallocate((uint8_t*)ptr);
  free(ptr);
}

int aerial_phy_vendor_ext(struct nfapi_vnf_p7_config *config, void *msg)
{
  if (((nfapi_nr_p7_message_header_t *)msg)->message_id == P7_VENDOR_EXT_IND) {
    // vendor_ext_p7_ind* ind = (vendor_ext_p7_ind*)msg;
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] vendor_ext (error_code:%d)\n", ind->error_code);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] unknown %02x\n", ((nfapi_nr_p7_message_header_t *)msg)->message_id);
  }

  return 0;
}

int aerial_phy_unpack_p7_vendor_extension(void *header,
                                          uint8_t **ppReadPackedMessage,
                                          uint8_t *end,
                                          nfapi_p7_codec_config_t *config)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if (((nfapi_nr_p7_message_header_t *)header)->message_id == P7_VENDOR_EXT_IND) {
    vendor_ext_p7_ind *req = (vendor_ext_p7_ind *)(header);

    if (!pull16(ppReadPackedMessage, &req->error_code, end))
      return 0;
  }

  return 1;
}

int aerial_phy_pack_p7_vendor_extension(void *header, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if (((nfapi_nr_p7_message_header_t *)header)->message_id == P7_VENDOR_EXT_REQ) {
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
    vendor_ext_p7_req *req = (vendor_ext_p7_req *)(header);

    if (!(push16(req->dummy1, ppWritePackedMsg, end) && push16(req->dummy2, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

int aerial_phy_unpack_vendor_extension_tlv(nfapi_tl_t *tl,
                                           uint8_t **ppReadPackedMessage,
                                           uint8_t *end,
                                           void **ve,
                                           nfapi_p7_codec_config_t *codec)
{
  (void)tl;
  (void)ppReadPackedMessage;
  (void)ve;
  return -1;
}

int aerial_phy_pack_vendor_extension_tlv(void *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *codec)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "phy_pack_vendor_extension_tlv\n");
  nfapi_tl_t *tlv = (nfapi_tl_t *)ve;

  switch (tlv->tag) {
    case VENDOR_EXT_TLV_1_TAG: {
      // NFAPI_TRACE(NFAPI_TRACE_INFO, "Packing VENDOR_EXT_TLV_1\n");
      vendor_ext_tlv_1 *ve = (vendor_ext_tlv_1 *)tlv;

      if (!push32(ve->dummy, ppWritePackedMsg, end))
        return 0;

      return 1;
    } break;

    default:
      return -1;
      break;
  }
}

void *aerial_phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t *msg_size)
{
  if (message_id == P7_VENDOR_EXT_IND) {
    *msg_size = sizeof(vendor_ext_p7_ind);
    return (nfapi_nr_p7_message_header_t *)malloc(sizeof(vendor_ext_p7_ind));
  }

  return 0;
}

void aerial_phy_deallocate_p7_vendor_ext(void *header)
{
  free(header);
}

uint8_t aerial_unpack_nr_slot_indication(uint8_t **ppReadPackedMsg,
                                         uint8_t *end,
                                         nfapi_nr_slot_indication_scf_t *msg,
                                         nfapi_p7_codec_config_t *config)
{
  return unpack_nr_slot_indication(ppReadPackedMsg, end, msg, config);
}

static uint8_t aerial_unpack_nr_rx_data_indication_body(nfapi_nr_rx_data_pdu_t *value,
                                                        uint8_t **ppReadPackedMsg,
                                                        uint8_t *end,
                                                        nfapi_p7_codec_config_t *config)
{
  if (!(pull32(ppReadPackedMsg, &value->handle, end) && pull16(ppReadPackedMsg, &value->rnti, end)
        && pull8(ppReadPackedMsg, &value->harq_id, end) &&
        // For Aerial, RX_DATA.indication PDULength is changed to 32 bit field
        pull32(ppReadPackedMsg, &value->pdu_length, end) && pull8(ppReadPackedMsg, &value->ul_cqi, end)
        && pull16(ppReadPackedMsg, &value->timing_advance, end) && pull16(ppReadPackedMsg, &value->rssi, end))) {
    return 0;
  }

  // Allocate space for the pdu to be unpacked later
  value->pdu = nfapi_p7_allocate(sizeof(*value->pdu) * value->pdu_length, config);

  return 1;
}
uint8_t aerial_unpack_nr_rx_data_indication(uint8_t **ppReadPackedMsg,
                                            uint8_t *end,
                                            uint8_t **pDataMsg,
                                            uint8_t *data_end,
                                            nfapi_nr_rx_data_indication_t *msg,
                                            nfapi_p7_codec_config_t *config)
{
  // For Aerial unpacking, the PDU data is packed in pDataMsg, and we read it after unpacking the PDU headers

  nfapi_nr_rx_data_indication_t *pNfapiMsg = (nfapi_nr_rx_data_indication_t *)msg;
  // Unpack SFN, slot, nPDU
  if (!(pull16(ppReadPackedMsg, &pNfapiMsg->sfn, end) && pull16(ppReadPackedMsg, &pNfapiMsg->slot, end)
        && pull16(ppReadPackedMsg, &pNfapiMsg->number_of_pdus, end))) {
    return 0;
  }
  // Allocate the PDU list for number of PDUs
  if (pNfapiMsg->number_of_pdus > 0) {
    pNfapiMsg->pdu_list = nfapi_p7_allocate(sizeof(*pNfapiMsg->pdu_list) * pNfapiMsg->number_of_pdus, config);
  }
  // For each PDU, unpack its header
  for (int i = 0; i < pNfapiMsg->number_of_pdus; i++) {
    if (!aerial_unpack_nr_rx_data_indication_body(&pNfapiMsg->pdu_list[i], ppReadPackedMsg, end, config))
      return 0;
  }
  // After unpacking the PDU headers, unpack the PDU data from the separate buffer
  for (int i = 0; i < pNfapiMsg->number_of_pdus; i++) {
    if(!pullarray8(pDataMsg,
               pNfapiMsg->pdu_list[i].pdu,
               pNfapiMsg->pdu_list[i].pdu_length,
               pNfapiMsg->pdu_list[i].pdu_length,
               data_end)){
      return 0;
    }
  }

  return 1;
}

uint8_t aerial_unpack_nr_crc_indication(uint8_t **ppReadPackedMsg,
                                        uint8_t *end,
                                        nfapi_nr_crc_indication_t *msg,
                                        nfapi_p7_codec_config_t *config)
{
  return unpack_nr_crc_indication(ppReadPackedMsg, end, msg, config);
}

uint8_t aerial_unpack_nr_uci_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config)
{
  return unpack_nr_uci_indication(ppReadPackedMsg, end, msg, config);
}

uint8_t aerial_unpack_nr_srs_indication(uint8_t **ppReadPackedMsg,
                                        uint8_t *end,
                                        uint8_t **pDataMsg,
                                        uint8_t *data_end,
                                        void *msg,
                                        nfapi_p7_codec_config_t *config)
{
  uint8_t retval = unpack_nr_srs_indication(ppReadPackedMsg, end, msg, config);
  nfapi_nr_srs_indication_t *srs_ind = (nfapi_nr_srs_indication_t *)msg;
  for (uint8_t pdu_idx = 0; pdu_idx < srs_ind->number_of_pdus; pdu_idx++) {
    nfapi_nr_srs_indication_pdu_t *pdu = &srs_ind->pdu_list[pdu_idx];
    if (!unpack_nr_srs_report_tlv_value(&pdu->report_tlv, pDataMsg, data_end)) {
      return 0;
    }
  }
  return retval;
}

uint8_t aerial_unpack_nr_rach_indication(uint8_t **ppReadPackedMsg,
                                         uint8_t *end,
                                         nfapi_nr_rach_indication_t *msg,
                                         nfapi_p7_codec_config_t *config)
{
  return unpack_nr_rach_indication(ppReadPackedMsg, end, msg, config);
}

static int32_t aerial_pack_tx_data_request(void *pMessageBuf,
                                           void *pPackedBuf,
                                           void *pDataBuf,
                                           uint32_t packedBufLen,
                                           uint32_t dataBufLen,
                                           nfapi_p7_codec_config_t *config,
                                           uint32_t *data_len)
{
  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack supplied pointers are null\n");
    return -1;
  }

  nfapi_nr_p7_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *data_end = pDataBuf + dataBufLen;
  uint8_t *pDataPackedMessage = pDataBuf;
  uint8_t *pPackedDataField = &pDataPackedMessage[0];
  uint8_t *pPackedDataFieldStart = &pDataPackedMessage[0];
  uint8_t **ppWriteData = &pPackedDataField;
  const uint32_t dataBufLen32 = (dataBufLen + 3) / 4;
  nfapi_nr_tx_data_request_t *pNfapiMsg = (nfapi_nr_tx_data_request_t *)pMessageHeader;
  // Actual payloads are packed in a separate buffer
  for (int i = 0; i < pNfapiMsg->Number_of_PDUs; i++) {
    nfapi_nr_pdu_t *value = (nfapi_nr_pdu_t *)&pNfapiMsg->pdu_list[i];
    // recalculate PDU_Length for Aerial (leave only the size occupied in the payload buffer afterward)
    // assuming there is only 1 TLV present
    value->PDU_length = value->TLVs[0].length;
    for (uint32_t k = 0; k < value->num_TLV; ++k) {
      // Ensure tag is 2
      value->TLVs[k].tag = 2;
      uint32_t num_values_to_push = ((value->TLVs[k].length + 3) / 4);
      if (value->TLVs[k].length > 0) {
        if (value->TLVs[k].length % 4 != 0) {
          if (!pusharray32(value->TLVs[k].value.direct, dataBufLen32, num_values_to_push - 1, ppWriteData, data_end)) {
            return 0;
          }
          int bytesToAdd = 4 - (4 - (value->TLVs[k].length % 4)) % 4;
          if (bytesToAdd != 4) {
            for (int j = 0; j < bytesToAdd; j++) {
              uint8_t toPush = (uint8_t)(value->TLVs[k].value.direct[num_values_to_push - 1] >> (j * 8));
              if (!push8(toPush, ppWriteData, data_end)) {
                return 0;
              }
            }
          }
        } else {
          // no padding needed
          if (!pusharray32(value->TLVs[k].value.direct, dataBufLen32, num_values_to_push, ppWriteData, data_end)) {
            return 0;
          }
        }
      } else {
        LOG_E(NR_MAC, "value->TLVs[i].length was 0! (%d.%d) \n", pNfapiMsg->SFN, pNfapiMsg->Slot);
      }
    }
  }

  // calculate data_len
  uintptr_t dataHead = (uintptr_t)pPackedDataFieldStart;
  uintptr_t dataEnd = (uintptr_t)pPackedDataField;
  data_len[0] = dataEnd - dataHead;


   if (!fapi_nr_p7_message_pack(pMessageBuf, pPackedBuf, packedBufLen, config)) {
     return -1;
   }

  return (int32_t)(pMessageHeader->message_length);
}


int fapi_nr_pack_and_send_p7_message(vnf_p7_t *vnf_p7, nfapi_nr_p7_message_header_t *header)
{
  uint8_t FAPI_buffer[1024 * 64];
  // Check if TX_DATA request, if true, need to pack to data_buf
  if (header->message_id == NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST) {
    nfapi_nr_tx_data_request_t *pNfapiMsg = (nfapi_nr_tx_data_request_t *)header;
    uint64_t size = 0;
    for (int i = 0; i < pNfapiMsg->Number_of_PDUs; ++i) {
      size += pNfapiMsg->pdu_list[i].PDU_length;
    }
    AssertFatal(size <= 1024 * 1024 * 2, "Message data larger than available buffer, tried to pack %" PRId64, size);
    uint8_t FAPI_data_buffer[1024 * 1024 * 2]; // 2MB
    uint32_t data_len = 0;
    int32_t len_FAPI = aerial_pack_tx_data_request(header,
                                                   FAPI_buffer,
                                                   FAPI_data_buffer,
                                                   sizeof(FAPI_buffer),
                                                   sizeof(FAPI_data_buffer),
                                                   &vnf_p7->_public.codec_config,
                                                   &data_len);
    if (len_FAPI <= 0) {
      LOG_E(NFAPI_VNF, "Problem packing TX_DATA_request\n");
      return len_FAPI;
    } else {
      int retval = aerial_send_P7_msg_with_data(FAPI_buffer, len_FAPI, FAPI_data_buffer, data_len, header);
      return retval;
    }
  } else {
    // Create and send FAPI P7 message
    int len_FAPI = fapi_nr_p7_message_pack(header, FAPI_buffer, sizeof(FAPI_buffer), &vnf_p7->_public.codec_config);
    return aerial_send_P7_msg(FAPI_buffer, len_FAPI, header);
  }
}
