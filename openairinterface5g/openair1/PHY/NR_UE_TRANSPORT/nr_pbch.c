/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/LTE_TRANSPORT/pbch.c
* \brief Top-level routines for generating and decoding  the PBCH/BCH physical/transport channel V8.6 2009-03
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger.fr
* \note
* \warning
*/
#include "PHY/defs_nr_UE.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/sse_intrin.h"
#include "PHY/INIT/nr_phy_init.h"
#include "openair1/SCHED_NR_UE/defs.h"
#include <openair1/PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h>
#include <openair1/PHY/TOOLS/phy_scope_interface.h>
#include "openair1/PHY/NR_REFSIG/nr_refsig_common.h"
#include "instrumentation.h"
//#define DEBUG_PBCH
//#define DEBUG_PBCH_ENCODING

#define PBCH_A 24
#define PBCH_MAX_RE (PBCH_MAX_RE_PER_SYMBOL*4)
#define print_shorts(s,x) printf("%s : %d,%d,%d,%d,%d,%d,%d,%d\n",s,((int16_t*)x)[0],((int16_t*)x)[1],((int16_t*)x)[2],((int16_t*)x)[3],((int16_t*)x)[4],((int16_t*)x)[5],((int16_t*)x)[6],((int16_t*)x)[7])

static uint16_t nr_pbch_extract(uint32_t rxdataF_sz,
                                const c16_t rxdataF[][rxdataF_sz],
                                const int estimateSz,
                                struct complex16 dl_ch_estimates[][estimateSz],
                                struct complex16 rxdataF_ext[][PBCH_MAX_RE_PER_SYMBOL],
                                struct complex16 dl_ch_estimates_ext[][PBCH_MAX_RE_PER_SYMBOL],
                                uint32_t symbol,
                                uint32_t s_offset,
                                int ssb_start_subcarrier,
                                const NR_DL_FRAME_PARMS *frame_parms,
                                int nid)
{
  uint16_t rb;
  uint8_t i, j, aarx;
  int nushiftmod4 = nid % 4;
  AssertFatal(symbol>=1 && symbol<5,
              "symbol %d illegal for PBCH extraction\n",
              symbol);

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    unsigned int rx_offset = frame_parms->first_carrier_offset + ssb_start_subcarrier;
    rx_offset = (rx_offset)%(frame_parms->ofdm_symbol_size);
    const struct complex16 *rxF = &rxdataF[aarx][(symbol + s_offset) * frame_parms->ofdm_symbol_size];
    struct complex16 *rxF_ext = rxdataF_ext[aarx];
#ifdef DEBUG_PBCH
    printf("extract_rbs (nushift %d): rx_offset=%d, symbol %u\n",
           nushiftmod4,
           (rx_offset + ((symbol+s_offset) * (frame_parms->ofdm_symbol_size))),
           symbol);
    int16_t *p = (int16_t *)rxF;

    for (int i =0; i<8; i++) {
      printf("rxF.r [%d]= %d rxF.i [%d]= %d\n", i, rxF[i].r, i, rxF[i].i);
      printf("pbch extract rxF  %d %d addr %p\n", p[2*i], p[2*i+1], &p[2*i]);
    }

#endif

    for (rb=0; rb<20; rb++) {
      j=0;

      if (symbol==1 || symbol==3) {
        for (i=0; i<12; i++) {
          if ((i!=nushiftmod4) &&
              (i!=(nushiftmod4+4)) &&
              (i!=(nushiftmod4+8))) {
            rxF_ext[j]=rxF[rx_offset];
#ifdef DEBUG_PBCH
            printf("rxF ext[%d] = (%d,%d) rxF [%u]= (%d,%d)\n",
		   (9 * rb) + j,
                   rxF_ext[j].r,
                   rxF_ext[j].i,
                   rx_offset,
                   rxF[rx_offset].r,
                   rxF[rx_offset].i);
#endif
            j++;
          }

          rx_offset=(rx_offset+1)%(frame_parms->ofdm_symbol_size);
          //rx_offset = (rx_offset >= frame_parms->ofdm_symbol_size) ? (rx_offset - frame_parms->ofdm_symbol_size + 1) : (rx_offset+1);
        }

        rxF_ext+=9;
      } else { //symbol 2
        if ((rb < 4) || (rb >15)) {
          for (i=0; i<12; i++) {
            if ((i!=nushiftmod4) &&
                (i!=(nushiftmod4+4)) &&
                (i!=(nushiftmod4+8))) {
              rxF_ext[j]=rxF[rx_offset];
#ifdef DEBUG_PBCH
              printf("rxF ext[%d] = (%d,%d) rxF [%u]= (%d,%d)\n",
                     (rb < 4) ? (9 * rb) + j : (9 * (rb - 12)) + j,
		     rxF_ext[j].r,
                     rxF_ext[j].i,
                     rx_offset,
		     rxF[rx_offset].r,
                     rxF[rx_offset].i);
#endif
              j++;
            }

            rx_offset=(rx_offset+1)%(frame_parms->ofdm_symbol_size);
            //rx_offset = (rx_offset >= frame_parms->ofdm_symbol_size) ? (rx_offset - frame_parms->ofdm_symbol_size + 1) : (rx_offset+1);
          }

          rxF_ext+=9;
        } else { //rx_offset = (rx_offset >= frame_parms->ofdm_symbol_size) ? (rx_offset - frame_parms->ofdm_symbol_size + 12) : (rx_offset+12);
          rx_offset = (rx_offset+12)%(frame_parms->ofdm_symbol_size);
        }
      }
    }

    struct complex16 *dl_ch0 = &dl_ch_estimates[aarx][((symbol+s_offset)*(frame_parms->ofdm_symbol_size))];

    //printf("dl_ch0 addr %p\n",dl_ch0);
    struct complex16 *dl_ch0_ext = dl_ch_estimates_ext[aarx];

    for (rb=0; rb<20; rb++) {
      j=0;

      if (symbol==1 || symbol==3) {
        for (i=0; i<12; i++) {
          if ((i!=nushiftmod4) &&
              (i!=(nushiftmod4+4)) &&
              (i!=(nushiftmod4+8))) {
            dl_ch0_ext[j]=dl_ch0[i];
#ifdef DEBUG_PBCH
            if ((rb == 0) && (i < 2))
              printf("dl ch0 ext[%d] = (%d,%d)  dl_ch0 [%d]= (%d,%d)\n",
                     j,
                     dl_ch0_ext[j].r,
                     dl_ch0_ext[j].i,
                     i,
                     dl_ch0[j].r,
                     dl_ch0[j].i);
#endif
            j++;
          }
        }

        dl_ch0+=12;
        dl_ch0_ext+=9;
      } else {
        if ((rb < 4) || (rb >15)) {
          for (i=0; i<12; i++) {
            if ((i!=nushiftmod4) &&
                (i!=(nushiftmod4+4)) &&
                (i!=(nushiftmod4+8))) {
              dl_ch0_ext[j]=dl_ch0[i];
#ifdef DEBUG_PBCH
              printf("dl ch0 ext[%d] = (%d,%d)  dl_ch0 [%d]= (%d,%d)\n",
                     j,
                     dl_ch0_ext[j].r,
                     dl_ch0_ext[j].i,
                     i,
                     dl_ch0[j].r,
                     dl_ch0[j].i);
#endif
              j++;
            }
          }

          dl_ch0_ext+=9;
        }

        dl_ch0+=12;
      }
    }
  }

  return(0);
}

//__m128i avg128;

//compute average channel_level on each (TX,RX) antenna pair
int nr_pbch_channel_level(struct complex16 dl_ch_estimates_ext[][PBCH_MAX_RE_PER_SYMBOL],
                          const NR_DL_FRAME_PARMS *frame_parms,
                          int nb_re)
{
  int32_t avg2 = 0;

  for (int aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    simde__m128i *dl_ch128 = (simde__m128i *)dl_ch_estimates_ext[aarx];

    //compute average level
    int32_t avg1 = simde_mm_average(dl_ch128, nb_re, 0, nb_re);

    if (avg1>avg2)
      avg2 = avg1;

    //LOG_I(PHY,"Channel level : %d, %d\n",avg1, avg2);
  }

  return(avg2);
}

void nr_pbch_channel_compensation(struct complex16 rxdataF_ext[][PBCH_MAX_RE_PER_SYMBOL],
                                  struct complex16 dl_ch_estimates_ext[][PBCH_MAX_RE_PER_SYMBOL],
                                  int nb_re,
                                  struct complex16 rxdataF_comp[][PBCH_MAX_RE_PER_SYMBOL],
                                  const NR_DL_FRAME_PARMS *frame_parms,
                                  uint8_t output_shift)
{
  for (int aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    mult_cpx_conj_vector((c16_t *)dl_ch_estimates_ext[aarx],
                         (c16_t *)rxdataF_ext[aarx],
                         (c16_t *)rxdataF_comp[aarx],
                         nb_re,
                         output_shift);
  }
}

void nr_pbch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                           int **rxdataF_comp,
                           uint8_t symbol) {
  uint8_t symbol_mod;
  int i, nb_rb = 6;
  simde__m128i *rxdataF_comp128_0, *rxdataF_comp128_1;
  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;

  if (frame_parms->nb_antennas_rx > 1) {
    rxdataF_comp128_0 = (simde__m128i *)&rxdataF_comp[0][symbol_mod * 6 * 12];
    rxdataF_comp128_1 = (simde__m128i *)&rxdataF_comp[1][symbol_mod * 6 * 12];

    // MRC on each re of rb, both on MF output and magnitude (for 16QAM/64QAM llr computation)
    for (i = 0; i < nb_rb * 3; i++) {
      rxdataF_comp128_0[i] =
          simde_mm_adds_epi16(simde_mm_srai_epi16(rxdataF_comp128_0[i], 1), simde_mm_srai_epi16(rxdataF_comp128_1[i], 1));
    }
  }

}

void nr_pbch_unscrambling(int16_t *demod_pbch_e,
                          uint16_t Nid,
                          uint8_t nushift,
                          uint16_t M,
                          uint16_t length,
                          uint8_t bitwise,
                          uint32_t unscrambling_mask,
                          uint32_t pbch_a_prime,
                          uint32_t *pbch_a_interleaved)
{
  uint32_t *seq = gold_cache(Nid, (nushift * M + length + 31) / 32); // this is c_init
  // The Gold sequence is shifted by nushift* M, so we skip (nushift*M /32) double words
  int idxGold = (nushift * M + 31) / 32 - 1;

  // Scrambling is now done with offset (nushift*M)%32
  int offset = (nushift * M) & 0x1f;
  uint8_t k = 0;
  for (int i = 0; i < length; i++) {
    if (bitwise) {
      if (((k + offset) & 0x1f) == 0 && (!((unscrambling_mask >> i) & 1)))
        idxGold++;
      *pbch_a_interleaved ^= ((unscrambling_mask >> i) & 1)
                                 ? ((pbch_a_prime >> i) & 1) << i
                                 : (((pbch_a_prime >> i) & 1) ^ ((seq[idxGold] >> ((k + offset) & 0x1f)) & 1)) << i;
      k += (!((unscrambling_mask>>i)&1));
#ifdef DEBUG_PBCH_ENCODING
      printf("i %d k %d offset %d (unscrambling_mask>>i)&1) %d s: %08x\t  pbch_a_interleaved 0x%08x (!((unscrambling_mask>>i)&1)) %d\n", i, k, offset, (unscrambling_mask>>i)&1, s, *pbch_a_interleaved,
             (!((unscrambling_mask>>i)&1)));
#endif
    } else {
      if (((i + offset) & 0x1f) == 0)
        idxGold++;

      if (seq[idxGold] & (1UL << ((i + offset) % 32)))
        demod_pbch_e[i] = -demod_pbch_e[i];

#ifdef DEBUG_PBCH_ENCODING

      if (i<8)
        printf("s %d demod_pbch_e[i] %d\n", ((s>>((i+offset)&0x1f))&1), demod_pbch_e[i]);

#endif
    }
  }
}

void nr_pbch_quantize(int16_t *pbch_llr8, int16_t *pbch_llr, uint16_t len)
{
  for (int i=0; i<len; i++) {
    if (pbch_llr[i]>31)
      pbch_llr8[i]=32;
    else if (pbch_llr[i]<-31)
      pbch_llr8[i]=-32;
    else
      pbch_llr8[i]=pbch_llr[i];
  }
}
/*
unsigned char sign(int8_t x) {
  return (unsigned char)x >> 7;
}
*/

const uint8_t pbch_deinterleaving_pattern[32] = {28, 0, 31, 30, 7,  29, 25, 27, 5,  8,  24, 9,  10, 11, 12, 13,
                                                 1,  4, 3,  14, 15, 16, 17, 2,  26, 18, 19, 20, 21, 22, 6,  23};

int nr_rx_pbch(PHY_VARS_NR_UE *ue,
               const UE_nr_rxtx_proc_t *proc,
               bool is_synchronized,
               int estimateSz,
               struct complex16 dl_ch_estimates[][estimateSz],
               const NR_DL_FRAME_PARMS *frame_parms,
               uint8_t i_ssb,
               int ssb_start_subcarrier,
               int Nid_cell,
               fapiPbch_t *result,
               int *half_frame_bit,
               int *ssb_index,
               int *ret_symbol_offset,
               int rxdataFSize,
               const struct complex16 rxdataF[][rxdataFSize])
{
  TracyCZone(ctx, true);
  int max_h=0;
  int symbol;
  uint8_t Lmax=frame_parms->Lmax;
  int M = NR_POLAR_PBCH_E;
  int nushift = (Lmax == 4) ? i_ssb & 3 : i_ssb & 7;
  int16_t pbch_e_rx[960]= {0}; //Fixme: previous version erase only NR_POLAR_PBCH_E bytes
  int16_t pbch_unClipped[960]= {0};
  int pbch_e_rx_idx=0;
  int symbol_offset=1;

  if (is_synchronized)
    symbol_offset=nr_get_ssb_start_symbol(frame_parms, i_ssb)%(frame_parms->symbols_per_slot);
  else
    symbol_offset=0;

#ifdef DEBUG_PBCH
  //printf("address dataf %p",nr_ue_common_vars->rxdataF);
  write_output("rxdataF0_pbch.m","rxF0pbch",
               &rxdataF[0][(symbol_offset+1)*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size*3,1,1);
#endif
  // symbol refers to symbol within SSB. symbol_offset is the offset of the SSB wrt start of slot
  double log2_maxh = 0;

  for (symbol=1; symbol<4; symbol++) {
    const uint16_t nb_re=symbol == 2 ? 72 : 180;
    __attribute__ ((aligned(32))) struct complex16 rxdataF_ext[frame_parms->nb_antennas_rx][PBCH_MAX_RE_PER_SYMBOL];
    __attribute__ ((aligned(32))) struct complex16 dl_ch_estimates_ext[frame_parms->nb_antennas_rx][PBCH_MAX_RE_PER_SYMBOL];
    memset(dl_ch_estimates_ext,0, sizeof  dl_ch_estimates_ext);
    nr_pbch_extract(frame_parms->samples_per_slot_wCP,
                    rxdataF,
                    estimateSz,
                    dl_ch_estimates,
                    rxdataF_ext,
                    dl_ch_estimates_ext,
                    symbol,
                    symbol_offset,
                    ssb_start_subcarrier,
                    frame_parms,
                    Nid_cell);
#ifdef DEBUG_PBCH
    LOG_I(PHY,"[PHY] PBCH Symbol %d ofdm size %d\n",symbol, frame_parms->ofdm_symbol_size);
    LOG_I(PHY,"[PHY] PBCH starting channel_level\n");
#endif

    if (symbol == 1) {
      max_h = nr_pbch_channel_level(dl_ch_estimates_ext,
                                    frame_parms,
                                    nb_re);
      log2_maxh = 3+(log2_approx(max_h)/2);
    }

#ifdef DEBUG_PBCH
    LOG_I(PHY,"[PHY] PBCH log2_maxh = %f (%d)\n", log2_maxh, max_h);
#endif
    __attribute__ ((aligned(32))) struct complex16 rxdataF_comp[frame_parms->nb_antennas_rx][PBCH_MAX_RE_PER_SYMBOL];
    nr_pbch_channel_compensation(rxdataF_ext,
                                 dl_ch_estimates_ext,
                                 nb_re,
                                 rxdataF_comp,
                                 frame_parms,
                                 log2_maxh); // log2_maxh+I0_shift

    /*if (frame_parms->nb_antennas_rx > 1)
      pbch_detection_mrc(frame_parms,
                         rxdataF_comp,
                         symbol);*/

    int nb=symbol==2 ? 144 : 360;
    nr_pbch_quantize(pbch_e_rx+pbch_e_rx_idx,
		     (short *)rxdataF_comp[0],
		     nb);
    memcpy(pbch_unClipped+pbch_e_rx_idx, rxdataF_comp[0], nb*sizeof(int16_t));
    pbch_e_rx_idx+=nb;
  }

  // legacy code use int16, but it is complex16
  if (ue) {
    metadata meta = {.slot = proc->nr_slot_rx, .frame = proc->frame_rx};
    UEscopeCopyWithMetadata(ue, pbchRxdataF_comp, pbch_unClipped, sizeof(struct complex16), frame_parms->nb_antennas_rx, pbch_e_rx_idx / 2, 0, &meta);
    UEscopeCopyWithMetadata(ue, pbchLlr, pbch_e_rx, sizeof(int16_t), frame_parms->nb_antennas_rx, pbch_e_rx_idx, 0, &meta);
  }
#ifdef DEBUG_PBCH
  for (int cnt = 0; cnt < 864  ; cnt++)
    printf("pbch rx llr %d\n", *(pbch_e_rx + cnt));
#endif
  // un-scrambling
  uint32_t unscrambling_mask = (Lmax==64)?0x100006D:0x1000041;
  uint32_t pbch_a_interleaved=0;
  uint32_t pbch_a_prime=0;
  nr_pbch_unscrambling(pbch_e_rx, Nid_cell, nushift, M, NR_POLAR_PBCH_E,
		       0, 0,  pbch_a_prime, &pbch_a_interleaved);
  //polar decoding de-rate matching
  uint64_t tmp=0;
  const uint32_t decoderState = polar_decoder_int16(pbch_e_rx,
                                                    (uint64_t *)&tmp,
                                                    0,
                                                    NR_POLAR_PBCH_MESSAGE_TYPE,
                                                    NR_POLAR_PBCH_PAYLOAD_BITS,
                                                    NR_POLAR_PBCH_AGGREGATION_LEVEL);
  pbch_a_prime = tmp;

  nr_downlink_indication_t dl_indication;
  fapi_nr_rx_indication_t rx_ind = {0};
  uint16_t number_pdus = 1;

  if (decoderState) {
    if (ue) { // decoding failed in synced state
      nr_fill_dl_indication(&dl_indication, NULL, &rx_ind, proc, ue, NULL);
      nr_fill_rx_indication(&rx_ind, FAPI_NR_RX_PDU_TYPE_SSB, ue, NULL, NULL, number_pdus, proc, NULL, NULL);
      if (ue->if_inst && ue->if_inst->dl_indication)
        ue->if_inst->dl_indication(&dl_indication);
    }
    return(decoderState);
  }
  //  printf("polar decoder output 0x%08x\n",pbch_a_prime);
  // Decoder reversal
  pbch_a_prime = (uint32_t)reverse_bits(pbch_a_prime, NR_POLAR_PBCH_PAYLOAD_BITS);

  //payload un-scrambling
  M = (Lmax == 64)? (NR_POLAR_PBCH_PAYLOAD_BITS - 6) : (NR_POLAR_PBCH_PAYLOAD_BITS - 3);
  nushift = ((pbch_a_prime>>24)&1) ^ (((pbch_a_prime>>6)&1)<<1);
  pbch_a_interleaved=0;
  nr_pbch_unscrambling(pbch_e_rx, Nid_cell, nushift, M, NR_POLAR_PBCH_PAYLOAD_BITS,
		       1, unscrambling_mask, pbch_a_prime, &pbch_a_interleaved);
  //printf("nushift %d sfn 3rd %d 2nd %d", nushift,((pbch_a_prime>>6)&1), ((pbch_a_prime>>24)&1) );
  //payload deinterleaving
  //uint32_t in=0;
  uint32_t out=0;

  for (int i=0; i<32; i++) {
    out |= ((pbch_a_interleaved>>i)&1)<<(pbch_deinterleaving_pattern[i]);
#ifdef DEBUG_PBCH
    printf("i %d in 0x%08x out 0x%08x ilv %d (in>>i)&1) 0x%08x\n", i, pbch_a_interleaved, out, pbch_deinterleaving_pattern[i], (pbch_a_interleaved>>i)&1);
#endif
  }

  result->xtra_byte = (out>>24)&0xff;

  const uint64_t payload = reverse_bits(out, NR_POLAR_PBCH_PAYLOAD_BITS);

  for (int i=0; i<3; i++)
    result->decoded_output[i] = (uint8_t)((payload>>((3-i)<<3))&0xff);

  *half_frame_bit = (result->xtra_byte >> 4) & 0x01; // computing the half frame index from the extra byte
  *ssb_index = i_ssb; // ssb index corresponds to i_ssb for Lmax = 4,8

  if (Lmax == 64) {   // for Lmax = 64 ssb index 4th,5th and 6th bits are in extra byte
    for (int i=0; i<3; i++)
      *ssb_index += (((result->xtra_byte >> (7 - i)) & 0x01) << (3 + i));
  }

  *ret_symbol_offset = nr_get_ssb_start_symbol(frame_parms, *ssb_index);

  if (*half_frame_bit)
    *ret_symbol_offset += (frame_parms->slots_per_frame >> 1) * frame_parms->symbols_per_slot;

#ifdef DEBUG_PBCH
  printf("xtra_byte %x payload %x\n", result->xtra_byte, payload);

  for (int i=0; i<(NR_POLAR_PBCH_PAYLOAD_BITS>>3); i++) {
    //     printf("unscrambling pbch_a[%d] = %x \n", i,pbch_a[i]);
    printf("[PBCH] decoder payload[%d] = %x\n",i,result->decoded_output[i]);
  }

#endif

  if (ue) {
    nr_fill_dl_indication(&dl_indication, NULL, &rx_ind, proc, ue, NULL);
    nr_fill_rx_indication(&rx_ind, FAPI_NR_RX_PDU_TYPE_SSB, ue, NULL, NULL, number_pdus, proc, (void *)result, NULL);

    if (ue->if_inst && ue->if_inst->dl_indication)
      ue->if_inst->dl_indication(&dl_indication);
  }

  TracyCZoneEnd(ctx);
  return 0;
}

double nr_ue_pbch_freq_offset(const NR_DL_FRAME_PARMS *frame_parms,
                              int estimateSz,
                              const c16_t dl_ch_estimates[][estimateSz])
{
  const int i_ssb = frame_parms->ssb_index;
  const int symbol_offset = nr_get_ssb_start_symbol(frame_parms, i_ssb) % frame_parms->symbols_per_slot;
  const c16_t *dl_ch_est_symb1 = &dl_ch_estimates[0][(symbol_offset + 1) * frame_parms->ofdm_symbol_size];
  const c16_t *dl_ch_est_symb3 = &dl_ch_estimates[0][(symbol_offset + 3) * frame_parms->ofdm_symbol_size];
  const int nb_re = 240;
  const c32_t dot_prod_res = dot_product(dl_ch_est_symb1, dl_ch_est_symb3, nb_re, 8);
  const double res_phase = atan2(dot_prod_res.i, dot_prod_res.r);
  const int samples_per_symbol = frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples;
  const double t_ofdm = samples_per_symbol / (frame_parms->samples_per_subframe * 1000.0); // symbol duration in sec
  const double freq_offset = res_phase / (2 * M_PI * (3 - 1) * t_ofdm);

  return freq_offset;
}
