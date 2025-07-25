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

/*! \file PHY/LTE_UE_TRANSPORT/pmch_ue.c
* \brief This includes routines for decoding the UE FeMBMS/PMCH physical/multicast/transport channel 3GPP TS 36.211 version 14.2.0 Release 14 Sections 6.5/6.10.2
* \author J. Morgade
* \date 2019
* \version 0.1
* \company Vicomtech
* \email: javier.morgade@ieee.org
* \note
* \warning
*/


#include "PHY/defs_UE.h"
#include "PHY/phy_extern.h"
#include "PHY/sse_intrin.h"
#include "PHY/LTE_UE_TRANSPORT/transport_proto_ue.h"

// Mask for identifying subframe for MBMS
#define MBSFN_TDD_SF3 0x80// for TDD
#define MBSFN_TDD_SF4 0x40
#define MBSFN_TDD_SF7 0x20
#define MBSFN_TDD_SF8 0x10
#define MBSFN_TDD_SF9 0x08

#define MBSFN_FDD_SF1 0x80// for FDD
#define MBSFN_FDD_SF2 0x40
#define MBSFN_FDD_SF3 0x20
#define MBSFN_FDD_SF6 0x10
#define MBSFN_FDD_SF7 0x08
#define MBSFN_FDD_SF8 0x04



void dump_mch(PHY_VARS_UE *ue,uint8_t eNB_id,uint16_t coded_bits_per_codeword,int subframe) {
  char fname[32],vname[32];
#define NSYMB_PMCH 12
  sprintf(fname,"mch_rxF_ext0.m");
  sprintf(vname,"pmch_rxF_ext0");
  LOG_M(fname,vname,ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id]->rxdataF_ext[0],12*(ue->frame_parms.N_RB_DL)*12,1,1);
  sprintf(fname,"mch_ch_ext00.m");
  sprintf(vname,"pmch_ch_ext00");
  LOG_M(fname,vname,ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id]->dl_ch_estimates_ext[0],12*(ue->frame_parms.N_RB_DL)*NSYMB_PMCH,1,1);
  /*
    LOG_M("dlsch%d_ch_ext01.m","dl01_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[1],12*N_RB_DL*NSYMB_PMCH,1,1);
    LOG_M("dlsch%d_ch_ext10.m","dl10_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[2],12*N_RB_DL*NSYMB_PMCH,1,1);
    LOG_M("dlsch%d_ch_ext11.m","dl11_ch0_ext",pdsch_vars[eNB_id]->dl_ch_estimates_ext[3],12*N_RB_DL*NSYMB_PMCH,1,1);
    LOG_M("dlsch%d_rho.m","dl_rho",pdsch_vars[eNB_id]->rho[0],12*N_RB_DL*NSYMB_PMCH,1,1);
  */
  sprintf(fname,"mch_rxF_comp0.m");
  sprintf(vname,"pmch_rxF_comp0");
  LOG_M(fname,vname,ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id]->rxdataF_comp0[0],12*(ue->frame_parms.N_RB_DL)*NSYMB_PMCH,1,1);
  sprintf(fname,"mch_rxF_llr.m");
  sprintf(vname,"pmch_llr");
  LOG_M(fname,vname, ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id]->llr[0],coded_bits_per_codeword,1,0);
  sprintf(fname,"mch_mag1.m");
  sprintf(vname,"pmch_mag1");
  LOG_M(fname,vname,ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id]->dl_ch_mag0[0],12*(ue->frame_parms.N_RB_DL)*NSYMB_PMCH,1,1);
  sprintf(fname,"mch_mag2.m");
  sprintf(vname,"pmch_mag2");
  LOG_M(fname,vname,ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id]->dl_ch_magb0[0],12*(ue->frame_parms.N_RB_DL)*NSYMB_PMCH,1,1);
  LOG_M("mch00_ch0.m","pmch00_ch0",
        &(ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id][0][0]),
        ue->frame_parms.ofdm_symbol_size*12,1,1);
  LOG_M("rxsig_mch.m","rxs_mch",
        &ue->common_vars.rxdata[0][subframe*ue->frame_parms.samples_per_tti],
        ue->frame_parms.samples_per_tti,1,1);
  /*
  if (PHY_vars_eNB_g)
    LOG_M("txsig_mch.m","txs_mch",
                 &PHY_vars_eNB_g[0][0]->common_vars.txdata[0][0][subframe*ue->frame_parms.samples_per_tti],
                 ue->frame_parms.samples_per_tti,1,1);*/
}


void fill_UE_dlsch_MCH(PHY_VARS_UE *ue,int mcs,int ndi,int rvidx,int eNB_id) {
  LTE_UE_DLSCH_t *dlsch = ue->dlsch_MCH[eNB_id];
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  dlsch->Mdlharq = 1;
  //  dlsch->rnti   = M_RNTI;
  dlsch->harq_processes[0]->mcs   = mcs;
  dlsch->harq_processes[0]->rvidx = rvidx;
  //  dlsch->harq_processes[0]->Ndi   = ndi;
  dlsch->harq_processes[0]->Nl    = 1;
  dlsch->harq_processes[0]->TBS = TBStable[get_I_TBS(dlsch->harq_processes[0]->mcs)][frame_parms->N_RB_DL-1];
  dlsch->current_harq_pid = 0;
  dlsch->harq_processes[0]->nb_rb = frame_parms->N_RB_DL;

  switch(frame_parms->N_RB_DL) {
    case 6:
      dlsch->harq_processes[0]->rb_alloc_even[0] = 0x3f;
      dlsch->harq_processes[0]->rb_alloc_odd[0] = 0x3f;
      break;

    case 25:
      dlsch->harq_processes[0]->rb_alloc_even[0] = 0x1ffffff;
      dlsch->harq_processes[0]->rb_alloc_odd[0] = 0x1ffffff;
      break;

    case 50:
      dlsch->harq_processes[0]->rb_alloc_even[0] = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_odd[0]  = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_even[1] = 0x3ffff;
      dlsch->harq_processes[0]->rb_alloc_odd[1]  = 0x3ffff;
      break;

    case 100:
      dlsch->harq_processes[0]->rb_alloc_even[0] = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_odd[0]  = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_even[1] = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_odd[1]  = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_even[2] = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_odd[2]  = 0xffffffff;
      dlsch->harq_processes[0]->rb_alloc_even[3] = 0xf;
      dlsch->harq_processes[0]->rb_alloc_odd[3]  = 0xf;
      break;
  }
}


void mch_extract_rbs_khz_1dot25(int **rxdataF,
                                int **dl_ch_estimates,
                                int **rxdataF_ext,
                                int **dl_ch_estimates_ext,
                                /*unsigned char symbol,*/
                                unsigned char subframe,
                                LTE_DL_FRAME_PARMS *frame_parms) {
  int i, j, offset, aarx;

  if( (subframe&0x1) == 0) {
    offset=0;
  } else {
    offset=3;
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    for (i=0,j=0; i<frame_parms->N_RB_DL*72; i++) {
      if( ((i-offset)%6) != 0 ) {
        //rxdataF_ext[aarx][j+0] = rxdataF[aarx][i+4344 +0];
        rxdataF_ext[aarx][j+0] = rxdataF[aarx][i+frame_parms->first_carrier_offset_khz_1dot25 +0];
        rxdataF_ext[aarx][(frame_parms->N_RB_DL*60)+j+0] = rxdataF[aarx][i+1+0]; //DC
        dl_ch_estimates_ext[aarx][j+0] = dl_ch_estimates[aarx][i+0];
        dl_ch_estimates_ext[aarx][(frame_parms->N_RB_DL*60)+j+0] = dl_ch_estimates[aarx][i+(frame_parms->N_RB_DL*72)+0];
        j++;
      }
    }
  }
}


void mch_extract_rbs(int **rxdataF,
                     int **dl_ch_estimates,
                     int **rxdataF_ext,
                     int **dl_ch_estimates_ext,
                     unsigned char symbol,
                     unsigned char subframe,
                     LTE_DL_FRAME_PARMS *frame_parms) {
  int pilots=0,i,j,offset,aarx;

  if ((symbol==2)||
      (symbol==10)) {
    pilots = 1;
    offset = 1;
  } else if (symbol==6) {
    pilots = 1;
    offset = 0;
  }

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    if (pilots==1) {
      for (i=offset,j=0; i<frame_parms->N_RB_DL*6; i+=2,j++) {
        /*  printf("MCH with pilots: i %d, j %d => %d,%d\n",i,j,
               *(int16_t*)&rxdataF[aarx][i+frame_parms->first_carrier_offset + (symbol*frame_parms->ofdm_symbol_size)],
               *(int16_t*)(1+&rxdataF[aarx][i+frame_parms->first_carrier_offset + (symbol*frame_parms->ofdm_symbol_size)]));
               */
        rxdataF_ext[aarx][j+symbol*(frame_parms->N_RB_DL*12)]                                  = rxdataF[aarx][i+frame_parms->first_carrier_offset + (symbol*frame_parms->ofdm_symbol_size)];
        rxdataF_ext[aarx][(frame_parms->N_RB_DL*3)+j+symbol*(frame_parms->N_RB_DL*12)]         = rxdataF[aarx][i+1+ (symbol*frame_parms->ofdm_symbol_size)];
        dl_ch_estimates_ext[aarx][j+symbol*(frame_parms->N_RB_DL*12)]                          = dl_ch_estimates[aarx][i+(symbol*frame_parms->ofdm_symbol_size)];
        dl_ch_estimates_ext[aarx][(frame_parms->N_RB_DL*3)+j+symbol*(frame_parms->N_RB_DL*12)] = dl_ch_estimates[aarx][i+(frame_parms->N_RB_DL*6)+(symbol*frame_parms->ofdm_symbol_size)];
      }
    } else {
      memcpy((void *)&rxdataF_ext[aarx][symbol*(frame_parms->N_RB_DL*12)],
             (void *)&rxdataF[aarx][frame_parms->first_carrier_offset + (symbol*frame_parms->ofdm_symbol_size)],
             frame_parms->N_RB_DL*24);
      memcpy((void *)&rxdataF_ext[aarx][(frame_parms->N_RB_DL*6) + symbol*(frame_parms->N_RB_DL*12)],
             (void *)&rxdataF[aarx][1 + (symbol*frame_parms->ofdm_symbol_size)],
             frame_parms->N_RB_DL*24);
      memcpy((void *)&dl_ch_estimates_ext[aarx][symbol*(frame_parms->N_RB_DL*12)],
             (void *)&dl_ch_estimates[aarx][(symbol*frame_parms->ofdm_symbol_size)],
             frame_parms->N_RB_DL*48);
    }
  }
}

void mch_channel_level(int **dl_ch_estimates_ext,
                       LTE_DL_FRAME_PARMS *frame_parms,
                       int *avg,
                       uint8_t symbol,
                       unsigned short nb_rb) {
  int i, aarx, nre;
  simde__m128i *dl_ch128, avg128;

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    //clear average level
    avg128 = simde_mm_setzero_si128();
    // 5 is always a symbol with no pilots for both normal and extended prefix
    dl_ch128 = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol * frame_parms->N_RB_DL * 12];

    if ((symbol == 2) || (symbol == 6) || (symbol == 10))
      nre = (frame_parms->N_RB_DL*6);
    else
      nre = (frame_parms->N_RB_DL*12);

    for (i = 0; i < (nre >> 2); i++) {
      avg128 =
          simde_mm_add_epi32(avg128, simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[0], dl_ch128[0]), log2_approx(nre >> 2) - 1));
    }

    avg[aarx] = (((((int*)&avg128)[0] +
                 ((int*)&avg128)[1] +
                 ((int*)&avg128)[2] +
                 ((int*)&avg128)[3])/(nre>>factor2(nre)))*(1<<(log2_approx(nre>>2)-1-factor2(nre))));

    //            printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
  }

}

void mch_channel_level_khz_1dot25(int **dl_ch_estimates_ext,
                                  LTE_DL_FRAME_PARMS *frame_parms,
                                  int *avg,
                                  /*uint8_t symbol,*/
                                  unsigned short nb_rb) {
  int i, aarx, nre;
  simde__m128i *dl_ch128, avg128;

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    //clear average level
    avg128 = simde_mm_setzero_si128();
    // 5 is always a symbol with no pilots for both normal and extended prefix
    dl_ch128 = (simde__m128i *)&dl_ch_estimates_ext[aarx][0 /*symbol*frame_parms->N_RB_DL*12*/];
    /*if ((symbol == 2) || (symbol == 6) || (symbol == 10))
      nre = (frame_parms->N_RB_DL*6);
    else
      nre = (frame_parms->N_RB_DL*12);*/
    nre = frame_parms->N_RB_DL*12*10;
    //nre = frame_parms->N_RB_DL*12;

    for (i = 0; i < (nre >> 2); i++) {
      // avg128 = simde_mm_add_epi32(avg128,simde_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
      avg128 =
          simde_mm_add_epi32(avg128, simde_mm_srai_epi32(simde_mm_madd_epi16(dl_ch128[0], dl_ch128[0]), log2_approx(nre >> 2) - 1));
    }

   // avg[aarx] = (((int*)&avg128)[0] +
   //              ((int*)&avg128)[1] +
   //              ((int*)&avg128)[2] +
   //              ((int*)&avg128)[3])/nre;
   avg[aarx] = (((((int*)&avg128)[0] +
                 ((int*)&avg128)[1] +
                 ((int*)&avg128)[2] +
                 ((int*)&avg128)[3])/(nre>>factor2(nre)))*(1<<(log2_approx(nre>>2)-1-factor2(nre))));
                //printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
  }

}



void mch_channel_compensation(int **rxdataF_ext,
                              int **dl_ch_estimates_ext,
                              int **dl_ch_mag,
                              int **dl_ch_magb,
                              int **rxdataF_comp,
                              LTE_DL_FRAME_PARMS *frame_parms,
                              unsigned char symbol,
                              unsigned char mod_order,
                              unsigned char output_shift) {
  int aarx, nre, i;
  simde__m128i *dl_ch128, *dl_ch_mag128, *dl_ch_mag128b, *rxdataF128, *rxdataF_comp128;
  simde__m128i QAM_amp128 = {0}, QAM_amp128b = {0};

  if ((symbol == 2) || (symbol == 6) || (symbol == 10))
    nre = frame_parms->N_RB_DL*6;
  else
    nre = frame_parms->N_RB_DL*12;

  if (mod_order == 4) {
    QAM_amp128 = simde_mm_set1_epi16(QAM16_n1); // 2/sqrt(10)
    QAM_amp128b = simde_mm_setzero_si128();
  } else if (mod_order == 6) {
    QAM_amp128 = simde_mm_set1_epi16(QAM64_n1); //
    QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);
  }

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    dl_ch128 = (simde__m128i *)&dl_ch_estimates_ext[aarx][symbol * frame_parms->N_RB_DL * 12];
    dl_ch_mag128 = (simde__m128i *)&dl_ch_mag[aarx][symbol * frame_parms->N_RB_DL * 12];
    dl_ch_mag128b = (simde__m128i *)&dl_ch_magb[aarx][symbol * frame_parms->N_RB_DL * 12];
    rxdataF128 = (simde__m128i *)&rxdataF_ext[aarx][symbol * frame_parms->N_RB_DL * 12];
    rxdataF_comp128 = (simde__m128i *)&rxdataF_comp[aarx][symbol * frame_parms->N_RB_DL * 12];

    for (i=0; i<(nre>>2); i+=2) {
      if (mod_order>2) {
        simde__m128i mmtmpD0, mmtmpD1;
        // get channel amplitude if not QPSK
        mmtmpD0 = oai_mm_smadd(dl_ch128[0], dl_ch128[0], output_shift);
        mmtmpD1 = oai_mm_smadd(dl_ch128[1], dl_ch128[1], output_shift);
        mmtmpD0 = simde_mm_packs_epi32(mmtmpD0, mmtmpD1);
        // store channel magnitude here in a new field of dlsch
        dl_ch_mag128[0] = simde_mm_unpacklo_epi16(mmtmpD0, mmtmpD0);
        dl_ch_mag128b[0] = dl_ch_mag128[0];
        dl_ch_mag128[0] = simde_mm_mulhi_epi16(dl_ch_mag128[0], QAM_amp128);
        dl_ch_mag128[0] = simde_mm_slli_epi16(dl_ch_mag128[0], 1);
        dl_ch_mag128[1] = simde_mm_unpackhi_epi16(mmtmpD0, mmtmpD0);
        dl_ch_mag128b[1] = dl_ch_mag128[1];
        dl_ch_mag128[1] = simde_mm_mulhi_epi16(dl_ch_mag128[1], QAM_amp128);
        dl_ch_mag128[1] = simde_mm_slli_epi16(dl_ch_mag128[1], 1);
        dl_ch_mag128b[0] = simde_mm_mulhi_epi16(dl_ch_mag128b[0], QAM_amp128b);
        dl_ch_mag128b[0] = simde_mm_slli_epi16(dl_ch_mag128b[0], 1);
        dl_ch_mag128b[1] = simde_mm_mulhi_epi16(dl_ch_mag128b[1], QAM_amp128b);
        dl_ch_mag128b[1] = simde_mm_slli_epi16(dl_ch_mag128b[1], 1);
      }

      // multiply by conjugated channel
      rxdataF_comp128[0] = oai_mm_cpx_mult_conj(dl_ch128[0], rxdataF128[0], output_shift);
      rxdataF_comp128[1] = oai_mm_cpx_mult_conj(dl_ch128[1], rxdataF128[1], output_shift);

      dl_ch128+=2;
      dl_ch_mag128+=2;
      dl_ch_mag128b+=2;
      rxdataF128+=2;
      rxdataF_comp128 += 2;
    }
  }

}



void mch_channel_compensation_khz_1dot25(int **rxdataF_ext,
    int **dl_ch_estimates_ext,
    int **dl_ch_mag,
    int **dl_ch_magb,
    int **rxdataF_comp,
    LTE_DL_FRAME_PARMS *frame_parms,
    /*unsigned char symbol,*/
    unsigned char mod_order,
    unsigned char output_shift) {
  int aarx, nre, i;
  simde__m128i *dl_ch128, *dl_ch_mag128, *dl_ch_mag128b, *rxdataF128, *rxdataF_comp128;
  simde__m128i QAM_amp128 = {0}, QAM_amp128b = {0};
  /*if ((symbol == 2) || (symbol == 6) || (symbol == 10))
    nre = frame_parms->N_RB_DL*6;
  else
    nre = frame_parms->N_RB_DL*12;*/
  nre = frame_parms->N_RB_DL * 12 * 10;

  if (mod_order == 4) {
    QAM_amp128 = simde_mm_set1_epi16(QAM16_n1); // 2/sqrt(10)
    QAM_amp128b = simde_mm_setzero_si128();
  } else if (mod_order == 6) {
    QAM_amp128 = simde_mm_set1_epi16(QAM64_n1); //
    QAM_amp128b = simde_mm_set1_epi16(QAM64_n2);
  }

  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    dl_ch128 = (simde__m128i *)&dl_ch_estimates_ext[aarx][0];
    dl_ch_mag128 = (simde__m128i *)&dl_ch_mag[aarx][0];
    dl_ch_mag128b = (simde__m128i *)&dl_ch_magb[aarx][0];
    rxdataF128 = (simde__m128i *)&rxdataF_ext[aarx][0];
    rxdataF_comp128 = (simde__m128i *)&rxdataF_comp[aarx][0];

    for (i=0; i<(nre>>2); i+=2) {
      if (mod_order>2) {
        simde__m128i mmtmpD0, mmtmpD1;
        // get channel amplitude if not QPSK
        mmtmpD0 = oai_mm_smadd(dl_ch128[0], dl_ch128[0], output_shift);
        mmtmpD1 = oai_mm_smadd(dl_ch128[1], dl_ch128[1], output_shift);
        mmtmpD0 = simde_mm_packs_epi32(mmtmpD0, mmtmpD1);
        // store channel magnitude here in a new field of dlsch
        dl_ch_mag128[0] = simde_mm_unpacklo_epi16(mmtmpD0, mmtmpD0);
        dl_ch_mag128b[0] = dl_ch_mag128[0];
        dl_ch_mag128[0] = simde_mm_mulhi_epi16(dl_ch_mag128[0], QAM_amp128);
        dl_ch_mag128[0] = simde_mm_slli_epi16(dl_ch_mag128[0], 1);
        dl_ch_mag128[1] = simde_mm_unpackhi_epi16(mmtmpD0, mmtmpD0);
        dl_ch_mag128b[1] = dl_ch_mag128[1];
        dl_ch_mag128[1] = simde_mm_mulhi_epi16(dl_ch_mag128[1], QAM_amp128);
        dl_ch_mag128[1] = simde_mm_slli_epi16(dl_ch_mag128[1], 1);
        dl_ch_mag128b[0] = simde_mm_mulhi_epi16(dl_ch_mag128b[0], QAM_amp128b);
        dl_ch_mag128b[0] = simde_mm_slli_epi16(dl_ch_mag128b[0], 1);
        dl_ch_mag128b[1] = simde_mm_mulhi_epi16(dl_ch_mag128b[1], QAM_amp128b);
        dl_ch_mag128b[1] = simde_mm_slli_epi16(dl_ch_mag128b[1], 1);
      }

      // multiply by conjugated channel
      rxdataF_comp128[0] = oai_mm_cpx_mult_conj(dl_ch128[0], rxdataF128[0], output_shift);
      rxdataF_comp128[1] = oai_mm_cpx_mult_conj(dl_ch128[1], rxdataF128[1], output_shift);

      dl_ch128+=2;
      dl_ch_mag128+=2;
      dl_ch_mag128b+=2;
      rxdataF128+=2;
      rxdataF_comp128 += 2;
    }
  }

}



void mch_detection_mrc(LTE_DL_FRAME_PARMS *frame_parms,
                       int **rxdataF_comp,
                       int **dl_ch_mag,
                       int **dl_ch_magb,
                       unsigned char symbol) {
  int i;
  simde__m128i *rxdataF_comp128_0, *rxdataF_comp128_1, *dl_ch_mag128_0, *dl_ch_mag128_1, *dl_ch_mag128_0b, *dl_ch_mag128_1b;

  if (frame_parms->nb_antennas_rx > 1) {
    rxdataF_comp128_0 = (simde__m128i *)&rxdataF_comp[0][symbol * frame_parms->N_RB_DL * 12];
    rxdataF_comp128_1 = (simde__m128i *)&rxdataF_comp[1][symbol * frame_parms->N_RB_DL * 12];
    dl_ch_mag128_0 = (simde__m128i *)&dl_ch_mag[0][symbol * frame_parms->N_RB_DL * 12];
    dl_ch_mag128_1 = (simde__m128i *)&dl_ch_mag[1][symbol * frame_parms->N_RB_DL * 12];
    dl_ch_mag128_0b = (simde__m128i *)&dl_ch_magb[0][symbol * frame_parms->N_RB_DL * 12];
    dl_ch_mag128_1b = (simde__m128i *)&dl_ch_magb[1][symbol * frame_parms->N_RB_DL * 12];

    // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
    for (i = 0; i < frame_parms->N_RB_DL * 3; i++) {
      rxdataF_comp128_0[i] =
          simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_0[i], 1), simde_mm_srai_epi16(rxdataF_comp128_1[i], 1));
      dl_ch_mag128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0[i], 1), simde_mm_srai_epi16(dl_ch_mag128_1[i], 1));
      dl_ch_mag128_0b[i] =
          simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0b[i], 1), simde_mm_srai_epi16(dl_ch_mag128_1b[i], 1));
    }
  }

}


void mch_detection_mrc_khz_1dot25(LTE_DL_FRAME_PARMS *frame_parms,
                                  int **rxdataF_comp,
                                  int **dl_ch_mag,
                                  int **dl_ch_magb/*,
                       unsigned char symbol*/) {
  int i;
  simde__m128i *rxdataF_comp128_0, *rxdataF_comp128_1, *dl_ch_mag128_0, *dl_ch_mag128_1, *dl_ch_mag128_0b, *dl_ch_mag128_1b;

  if (frame_parms->nb_antennas_rx > 1) {
    rxdataF_comp128_0 = (simde__m128i *)&rxdataF_comp[0][0];
    rxdataF_comp128_1 = (simde__m128i *)&rxdataF_comp[1][0];
    dl_ch_mag128_0 = (simde__m128i *)&dl_ch_mag[0][0];
    dl_ch_mag128_1 = (simde__m128i *)&dl_ch_mag[1][0];
    dl_ch_mag128_0b = (simde__m128i *)&dl_ch_magb[0][0];
    dl_ch_mag128_1b = (simde__m128i *)&dl_ch_magb[1][0];

    // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
    for (i = 0; i < frame_parms->N_RB_DL * 30; i++) {
      rxdataF_comp128_0[i] =
          simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_0[i], 1), simde_mm_srai_epi16(rxdataF_comp128_1[i], 1));
      dl_ch_mag128_0[i] = simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0[i], 1), simde_mm_srai_epi16(dl_ch_mag128_1[i], 1));
      dl_ch_mag128_0b[i] =
          simde_mm_adds_epi16(simde_mm_srai_epi16(dl_ch_mag128_0b[i], 1), simde_mm_srai_epi16(dl_ch_mag128_1b[i], 1));
    }
  }

}






int mch_qpsk_llr(LTE_DL_FRAME_PARMS *frame_parms,
                 int **rxdataF_comp,
                 short *dlsch_llr,
                 unsigned char symbol,
                 short **llr32p) {
  uint32_t *rxF = (uint32_t *)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  uint32_t *llr32;
  int i,len;

  if (symbol==2) {
    llr32 = (uint32_t *)dlsch_llr;
  } else {
    llr32 = (uint32_t *)(*llr32p);
  }

  AssertFatal(llr32!=NULL,"dlsch_qpsk_llr: llr is null, symbol %d, llr32=%p\n",symbol, llr32);

  if ((symbol==2) || (symbol==6) || (symbol==10)) {
    len = frame_parms->N_RB_DL*6;
  } else {
    len = frame_parms->N_RB_DL*12;
  }

  for (i=0; i<len; i++) {
    *llr32 = *rxF;
    rxF++;
    llr32++;
  }

  *llr32p = (short *)llr32;
  return(0);
}


int mch_qpsk_llr_khz_1dot25(LTE_DL_FRAME_PARMS *frame_parms,
                            int **rxdataF_comp,
                            short *dlsch_llr,
                            /*unsigned char symbol,*/
                            short **llr32p) {
  uint32_t *rxF = (uint32_t *)&rxdataF_comp[0][0/*(symbol*frame_parms->N_RB_DL*12)*/];
  //uint32_t *rxF = (uint32_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  uint32_t *llr32;
  int i,len;
  //if (symbol==0) {
  llr32 = (uint32_t *)dlsch_llr;
  //} else {
  //llr32 = (uint32_t*)(*llr32p);
  //}
  //AssertFatal(llr32!=NULL,"dlsch_qpsk_llr: llr is null, symbol %d, llr32=%p\n",symbol, llr32);
  AssertFatal(llr32!=NULL,"dlsch_qpsk_llr: llr is null, llr32=%p\n",llr32);
  len = frame_parms->N_RB_DL*12*10;

  for (i=0; i<len; i++) {
    *llr32 = *rxF;
    rxF++;
    llr32++;
  }

  *llr32p = (short *)llr32;
  return(0);
}



//----------------------------------------------------------------------------------------------
// 16-QAM
//----------------------------------------------------------------------------------------------

void mch_16qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                   int **rxdataF_comp,
                   short *dlsch_llr,
                   int **dl_ch_mag,
                   unsigned char symbol,
                   int16_t **llr32p)
{
  simde__m128i *rxF = (simde__m128i *)&rxdataF_comp[0][(symbol * frame_parms->N_RB_DL * 12)];
  simde__m128i *ch_mag;
  simde__m128i llr128[2], xmm0;
  uint32_t *llr32;
  int i,len;
  unsigned char len_mod4 = 0;

  if (symbol==2) {
    llr32 = (uint32_t *)dlsch_llr;
  } else {
    llr32 = (uint32_t *)*llr32p;
  }

  ch_mag = (simde__m128i *)&dl_ch_mag[0][(symbol * frame_parms->N_RB_DL * 12)];

  if ((symbol==2) || (symbol==6) || (symbol==10)) {
    len = frame_parms->N_RB_DL*6;
  } else {
    len = frame_parms->N_RB_DL*12;
  }

  // update output pointer according to number of REs in this symbol (<<2 because 4 bits per RE)
  if (symbol==2)
    *llr32p = dlsch_llr + (len<<2);
  else
    *llr32p += (len<<2);

  len_mod4 = len&3;
  len>>=2;  // length in quad words (4 REs)
  len+=(len_mod4==0 ? 0 : 1);

  for (i = 0; i < len; i++) {
    xmm0 = simde_mm_abs_epi16(rxF[i]);
    xmm0 = simde_mm_subs_epi16(ch_mag[i], xmm0);
    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2
    llr128[0] = simde_mm_unpacklo_epi32(rxF[i], xmm0);
    llr128[1] = simde_mm_unpackhi_epi32(rxF[i], xmm0);
    llr32[0] = ((uint32_t *)&llr128[0])[0];
    llr32[1] = ((uint32_t *)&llr128[0])[1];
    llr32[2] = ((uint32_t *)&llr128[0])[2];
    llr32[3] = ((uint32_t *)&llr128[0])[3];
    llr32[4] = ((uint32_t *)&llr128[1])[0];
    llr32[5] = ((uint32_t *)&llr128[1])[1];
    llr32[6] = ((uint32_t *)&llr128[1])[2];
    llr32[7] = ((uint32_t *)&llr128[1])[3];
    llr32 += 8;
  }

}

void mch_16qam_llr_khz_1dot25(LTE_DL_FRAME_PARMS *frame_parms,
                              int **rxdataF_comp,
                              short *dlsch_llr,
                              int **dl_ch_mag,
                              /*unsigned char symbol,*/
                              int16_t **llr32p)
{
  simde__m128i *rxF = (simde__m128i *)&rxdataF_comp[0][0];
  simde__m128i *ch_mag;
  simde__m128i llr128[2], xmm0;
  uint32_t *llr32;
  int i,len;
  unsigned char len_mod4 = 0;
  //if (symbol==2) {
  llr32 = (uint32_t *)dlsch_llr;
  //} else {
  // llr32 = (uint32_t*)*llr32p;
  //}
  ch_mag = (simde__m128i *)&dl_ch_mag[0][0];
  len = frame_parms->N_RB_DL*12*10;
  // update output pointer according to number of REs in this symbol (<<2 because 4 bits per RE)
  //if (symbol==2)
  *llr32p = dlsch_llr + (len<<2);
  //else
  //*llr32p += (len<<2);
  len_mod4 = len&3;
  len>>=2;  // length in quad words (4 REs)
  len+=(len_mod4==0 ? 0 : 1);

  for (i = 0; i < len; i++) {
    xmm0 = simde_mm_abs_epi16(rxF[i]);
    xmm0 = simde_mm_subs_epi16(ch_mag[i], xmm0);
    // lambda_1=y_R, lambda_2=|y_R|-|h|^2, lamda_3=y_I, lambda_4=|y_I|-|h|^2
    llr128[0] = simde_mm_unpacklo_epi32(rxF[i], xmm0);
    llr128[1] = simde_mm_unpackhi_epi32(rxF[i], xmm0);
    llr32[0] = ((uint32_t *)&llr128[0])[0];
    llr32[1] = ((uint32_t *)&llr128[0])[1];
    llr32[2] = ((uint32_t *)&llr128[0])[2];
    llr32[3] = ((uint32_t *)&llr128[0])[3];
    llr32[4] = ((uint32_t *)&llr128[1])[0];
    llr32[5] = ((uint32_t *)&llr128[1])[1];
    llr32[6] = ((uint32_t *)&llr128[1])[2];
    llr32[7] = ((uint32_t *)&llr128[1])[3];
    llr32 += 8;
  }

}

//----------------------------------------------------------------------------------------------
// 64-QAM
//----------------------------------------------------------------------------------------------

void mch_64qam_llr(LTE_DL_FRAME_PARMS *frame_parms,
                   int **rxdataF_comp,
                   short *dlsch_llr,
                   int **dl_ch_mag,
                   int **dl_ch_magb,
                   unsigned char symbol,
                   short **llr_save)
{
  simde__m128i xmm1, xmm2, *ch_mag, *ch_magb;
  simde__m128i *rxF = (simde__m128i *)&rxdataF_comp[0][(symbol * frame_parms->N_RB_DL * 12)];
  int i,len,len2;
  //   int j=0;
  unsigned char len_mod4;
  short *llr;
  int16_t *llr2;

  if (symbol==2)
    llr = dlsch_llr;
  else
    llr = *llr_save;

  ch_mag = (simde__m128i *)&dl_ch_mag[0][(symbol * frame_parms->N_RB_DL * 12)];
  ch_magb = (simde__m128i *)&dl_ch_magb[0][(symbol * frame_parms->N_RB_DL * 12)];

  if ((symbol==2) || (symbol==6) || (symbol==10)) {
    len = frame_parms->N_RB_DL*6;
  } else {
    len = frame_parms->N_RB_DL*12;
  }

  llr2 = llr;
  llr += (len*6);
  len_mod4 =len&3;
  len2=len>>2;  // length in quad words (4 REs)
  len2+=(len_mod4?0:1);

  for (i = 0; i < len2; i++) {
    xmm1 = simde_mm_abs_epi16(rxF[i]);
    xmm1 = simde_mm_subs_epi16(ch_mag[i], xmm1);
    xmm2 = simde_mm_abs_epi16(xmm1);
    xmm2 = simde_mm_subs_epi16(ch_magb[i], xmm2);
    // loop over all LLRs in quad word (24 coded bits)
    /*
    for (j=0;j<8;j+=2) {
      llr2[0] = ((short *)&rxF[i])[j];
      llr2[1] = ((short *)&rxF[i])[j+1];
      llr2[2] = simde_mm_extract_epi16(xmm1,j);
      llr2[3] = simde_mm_extract_epi16(xmm1,j+1);//((short *)&xmm1)[j+1];
      llr2[4] = simde_mm_extract_epi16(xmm2,j);//((short *)&xmm2)[j];
      llr2[5] = simde_mm_extract_epi16(xmm2,j+1);//((short *)&xmm2)[j+1];

      llr2+=6;
    }
    */
    llr2[0] = ((short *)&rxF[i])[0];
    llr2[1] = ((short *)&rxF[i])[1];
    llr2[2] = simde_mm_extract_epi16(xmm1, 0);
    llr2[3] = simde_mm_extract_epi16(xmm1, 1); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 0); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 1); //((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[2];
    llr2[1] = ((short *)&rxF[i])[3];
    llr2[2] = simde_mm_extract_epi16(xmm1, 2);
    llr2[3] = simde_mm_extract_epi16(xmm1, 3); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 2); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 3); //((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[4];
    llr2[1] = ((short *)&rxF[i])[5];
    llr2[2] = simde_mm_extract_epi16(xmm1, 4);
    llr2[3] = simde_mm_extract_epi16(xmm1, 5); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 4); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 5); //((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[6];
    llr2[1] = ((short *)&rxF[i])[7];
    llr2[2] = simde_mm_extract_epi16(xmm1, 6);
    llr2[3] = simde_mm_extract_epi16(xmm1, 7); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 6); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 7); //((short *)&xmm2)[j+1];
    llr2+=6;
  }

  *llr_save = llr;
}

void mch_64qam_llr_khz_1dot25(LTE_DL_FRAME_PARMS *frame_parms,
                              int **rxdataF_comp,
                              short *dlsch_llr,
                              int **dl_ch_mag,
                              int **dl_ch_magb,
                              /*unsigned char symbol,*/
                              short **llr_save)
{
  simde__m128i xmm1, xmm2, *ch_mag, *ch_magb;
  simde__m128i *rxF = (simde__m128i *)&rxdataF_comp[0][0];
  int i,len,len2;
  //   int j=0;
  unsigned char len_mod4;
  short *llr;
  int16_t *llr2;
  //if (symbol==2)
  llr = dlsch_llr;
  // else
  // llr = *llr_save;
  ch_mag = (simde__m128i *)&dl_ch_mag[0][0];
  ch_magb = (simde__m128i *)&dl_ch_magb[0][0];
  len = frame_parms->N_RB_DL*12*10;
  llr2 = llr;
  llr += (len*6);
  len_mod4 =len&3;
  len2=len>>2;  // length in quad words (4 REs)
  len2+=(len_mod4?0:1);

  for (i = 0; i < len2; i++) {
    xmm1 = simde_mm_abs_epi16(rxF[i]);
    xmm1 = simde_mm_subs_epi16(ch_mag[i], xmm1);
    xmm2 = simde_mm_abs_epi16(xmm1);
    xmm2 = simde_mm_subs_epi16(ch_magb[i], xmm2);
    // loop over all LLRs in quad word (24 coded bits)
    /*
    for (j=0;j<8;j+=2) {
      llr2[0] = ((short *)&rxF[i])[j];
      llr2[1] = ((short *)&rxF[i])[j+1];
      llr2[2] = simde_mm_extract_epi16(xmm1,j);
      llr2[3] = simde_mm_extract_epi16(xmm1,j+1);//((short *)&xmm1)[j+1];
      llr2[4] = simde_mm_extract_epi16(xmm2,j);//((short *)&xmm2)[j];
      llr2[5] = simde_mm_extract_epi16(xmm2,j+1);//((short *)&xmm2)[j+1];

      llr2+=6;
    }
    */
    llr2[0] = ((short *)&rxF[i])[0];
    llr2[1] = ((short *)&rxF[i])[1];
    llr2[2] = simde_mm_extract_epi16(xmm1, 0);
    llr2[3] = simde_mm_extract_epi16(xmm1, 1); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 0); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 1); //((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[2];
    llr2[1] = ((short *)&rxF[i])[3];
    llr2[2] = simde_mm_extract_epi16(xmm1, 2);
    llr2[3] = simde_mm_extract_epi16(xmm1, 3); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 2); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 3); //((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[4];
    llr2[1] = ((short *)&rxF[i])[5];
    llr2[2] = simde_mm_extract_epi16(xmm1, 4);
    llr2[3] = simde_mm_extract_epi16(xmm1, 5); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 4); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 5); //((short *)&xmm2)[j+1];
    llr2+=6;
    llr2[0] = ((short *)&rxF[i])[6];
    llr2[1] = ((short *)&rxF[i])[7];
    llr2[2] = simde_mm_extract_epi16(xmm1, 6);
    llr2[3] = simde_mm_extract_epi16(xmm1, 7); //((short *)&xmm1)[j+1];
    llr2[4] = simde_mm_extract_epi16(xmm2, 6); //((short *)&xmm2)[j];
    llr2[5] = simde_mm_extract_epi16(xmm2, 7); //((short *)&xmm2)[j+1];
    llr2+=6;
  }

  *llr_save = llr;
}

int avg_pmch[4];
int rx_pmch(PHY_VARS_UE *ue,
            unsigned char eNB_id,
            uint8_t subframe,
            unsigned char symbol) {
  LTE_UE_COMMON *common_vars  = &ue->common_vars;
  LTE_UE_PDSCH **pdsch_vars   = &ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id];
  LTE_DL_FRAME_PARMS *frame_parms    = &ue->frame_parms;
  LTE_UE_DLSCH_t   **dlsch        = &ue->dlsch_MCH[eNB_id];
  int avgs,aarx;
  mch_extract_rbs(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                  common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id],
                  pdsch_vars[eNB_id]->rxdataF_ext,
                  pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                  symbol,
                  subframe,
                  frame_parms);

  if (symbol == 2) {
    mch_channel_level(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                      frame_parms,
                      avg_pmch,
                      symbol,
                      frame_parms->N_RB_DL);
  }

  avgs = 0;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++)
    avgs = cmax(avgs,avg_pmch[aarx]);

  if (get_Qm(dlsch[0]->harq_processes[0]->mcs)==2)
    pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2) ;// + 2
  else
    pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2); // + 5;// + 2

  mch_channel_compensation(pdsch_vars[eNB_id]->rxdataF_ext,
                           pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                           pdsch_vars[eNB_id]->dl_ch_mag0,
                           pdsch_vars[eNB_id]->dl_ch_magb0,
                           pdsch_vars[eNB_id]->rxdataF_comp0,
                           frame_parms,
                           symbol,
                           get_Qm(dlsch[0]->harq_processes[0]->mcs),
                           pdsch_vars[eNB_id]->log2_maxh);

  if (frame_parms->nb_antennas_rx > 1)
    mch_detection_mrc(frame_parms,
                      pdsch_vars[eNB_id]->rxdataF_comp0,
                      pdsch_vars[eNB_id]->dl_ch_mag0,
                      pdsch_vars[eNB_id]->dl_ch_magb0,
                      symbol);

  switch (get_Qm(dlsch[0]->harq_processes[0]->mcs)) {
    case 2 :
      mch_qpsk_llr(frame_parms,
                   pdsch_vars[eNB_id]->rxdataF_comp0,
                   pdsch_vars[eNB_id]->llr[0],
                   symbol,
                   pdsch_vars[eNB_id]->llr128);
      break;

    case 4:
      mch_16qam_llr(frame_parms,
                    pdsch_vars[eNB_id]->rxdataF_comp0,
                    pdsch_vars[eNB_id]->llr[0],
                    pdsch_vars[eNB_id]->dl_ch_mag0,
                    symbol,
                    pdsch_vars[eNB_id]->llr128);
      break;

    case 6:
      mch_64qam_llr(frame_parms,
                    pdsch_vars[eNB_id]->rxdataF_comp0,
                    pdsch_vars[eNB_id]->llr[0],
                    pdsch_vars[eNB_id]->dl_ch_mag0,
                    pdsch_vars[eNB_id]->dl_ch_magb0,
                    symbol,
                    pdsch_vars[eNB_id]->llr128);
      break;
  }

  return(0);
}

int rx_pmch_khz_1dot25(PHY_VARS_UE *ue,
                       unsigned char eNB_id,
                       uint8_t subframe/*,
            unsigned char symbol*/
                       ,int mcs) { // currently work around TOFIX
  //unsigned int symbol;
  LTE_UE_COMMON *common_vars  = &ue->common_vars;
  LTE_UE_PDSCH **pdsch_vars   = &ue->pdsch_vars_MCH[ue->current_thread_id[subframe]][eNB_id];
  LTE_DL_FRAME_PARMS *frame_parms    = &ue->frame_parms;
  //LTE_UE_DLSCH_t   **dlsch        = &ue->dlsch_MCH[eNB_id];
  int avgs,aarx;
  mch_extract_rbs_khz_1dot25(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF,
                             common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].dl_ch_estimates[eNB_id],
                             pdsch_vars[eNB_id]->rxdataF_ext,
                             pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                             /*symbol,*/
                             subframe,
                             frame_parms);
  mch_channel_level_khz_1dot25(pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                               frame_parms,
                               avg_pmch,
                               /*symbol,*/
                               frame_parms->N_RB_DL);
  avgs = 0;

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    avgs = cmax(avgs,avg_pmch[aarx]);
  }

  if (get_Qm(mcs/*dlsch[0]->harq_processes[0]->mcs)==2*/)==2)
    pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2) ;// + 2
  else
    pdsch_vars[eNB_id]->log2_maxh = (log2_approx(avgs)/2); // + 5;// + 2*/

  mch_channel_compensation_khz_1dot25(pdsch_vars[eNB_id]->rxdataF_ext,
                                      pdsch_vars[eNB_id]->dl_ch_estimates_ext,
                                      pdsch_vars[eNB_id]->dl_ch_mag0,
                                      pdsch_vars[eNB_id]->dl_ch_magb0,
                                      pdsch_vars[eNB_id]->rxdataF_comp0,
                                      frame_parms,
                                      /*symbol,*/
                                      get_Qm(mcs/*dlsch[0]->harq_processes[0]->mcs*/),
                                      pdsch_vars[eNB_id]->log2_maxh);

  if (frame_parms->nb_antennas_rx > 1) {
    mch_detection_mrc_khz_1dot25(frame_parms,
                                 pdsch_vars[eNB_id]->rxdataF_comp0,
                                 pdsch_vars[eNB_id]->dl_ch_mag0,
                                 pdsch_vars[eNB_id]->dl_ch_magb0/*,
                      symbol*/);
  }

  switch (get_Qm(mcs/*dlsch[0]->harq_processes[0]->mcs*/)) {
    case 2 :
      mch_qpsk_llr_khz_1dot25(frame_parms,
                              pdsch_vars[eNB_id]->rxdataF_comp0,
                              pdsch_vars[eNB_id]->llr[0],
                              /*symbol,*/
                              pdsch_vars[eNB_id]->llr128);
      break;

    case 4:
      mch_16qam_llr_khz_1dot25(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               pdsch_vars[eNB_id]->llr[0],
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               /*symbol,*/
                               pdsch_vars[eNB_id]->llr128);
      break;

    case 6:
      mch_64qam_llr_khz_1dot25(frame_parms,
                               pdsch_vars[eNB_id]->rxdataF_comp0,
                               pdsch_vars[eNB_id]->llr[0],
                               pdsch_vars[eNB_id]->dl_ch_mag0,
                               pdsch_vars[eNB_id]->dl_ch_magb0,
                               /*symbol,*/
                               pdsch_vars[eNB_id]->llr128);
      break;
  }

  return(0);
}


