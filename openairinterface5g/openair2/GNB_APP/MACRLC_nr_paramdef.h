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

/*! \file MACRLC_nr_paramdef.h
 * \brief definition of configuration parameters for all gNodeB modules
 * \author Francois TABURET, WEI-TAI CHEN
 * \date 2018
 * \version 0.1
 * \company NOKIA BellLabs France, NTUST
 * \email: francois.taburet@nokia-bell-labs.com, kroempa@gmail.com
 * \note
 * \warning
 */

#ifndef __GNB_APP_MACRLC_NR_PARAMDEF__H__
#define __GNB_APP_MACRLC_NR_PARAMDEF__H__

/*----------------------------------------------------------------------------------------------------------------------------------------------------*/


/* MACRLC configuration parameters names   */
#define CONFIG_STRING_MACRLC_CC                            "num_cc"
#define CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE        "tr_n_preference"
#define CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS               "local_n_address"
#define CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS_F1U           "local_n_address_f1u"
#define CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS              "remote_n_address"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTC                 "local_n_portc"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTC                "remote_n_portc"
#define CONFIG_STRING_MACRLC_LOCAL_N_PORTD                 "local_n_portd"
#define CONFIG_STRING_MACRLC_REMOTE_N_PORTD                "remote_n_portd"
#define CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE        "tr_s_preference"
#define CONFIG_STRING_MACRLC_TRANSPORT_S_SHM_PREFIX "tr_s_shm_prefix"
#define CONFIG_STRING_MACRLC_TRANSPORT_S_POLL_CORE "tr_s_poll_core"
#define CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS               "local_s_address"
#define CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS              "remote_s_address"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTC                 "local_s_portc"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTC                "remote_s_portc"
#define CONFIG_STRING_MACRLC_LOCAL_S_PORTD                 "local_s_portd"
#define CONFIG_STRING_MACRLC_REMOTE_S_PORTD                "remote_s_portd"
#define CONFIG_STRING_MACRLC_ULSCH_MAX_FRAME_INACTIVITY    "ulsch_max_frame_inactivity"
#define CONFIG_STRING_MACRLC_PUSCHTARGETSNRX10             "pusch_TargetSNRx10"
#define CONFIG_STRING_MACRLC_PUCCHTARGETSNRX10             "pucch_TargetSNRx10"
#define CONFIG_STRING_MACRLC_UL_PRBBLACK_SNR_THRESHOLD     "ul_prbblack_SNR_threshold"
#define CONFIG_STRING_MACRLC_PUCCHFAILURETHRES             "pucch_FailureThres"
#define CONFIG_STRING_MACRLC_PUSCHFAILURETHRES             "pusch_FailureThres"
#define CONFIG_STRING_MACRLC_DL_BLER_TARGET_UPPER          "dl_bler_target_upper"
#define CONFIG_STRING_MACRLC_DL_BLER_TARGET_LOWER          "dl_bler_target_lower"
#define CONFIG_STRING_MACRLC_DL_MIN_MCS                    "dl_min_mcs"
#define CONFIG_STRING_MACRLC_DL_MAX_MCS                    "dl_max_mcs"
#define CONFIG_STRING_MACRLC_UL_BLER_TARGET_UPPER          "ul_bler_target_upper"
#define CONFIG_STRING_MACRLC_UL_BLER_TARGET_LOWER          "ul_bler_target_lower"
#define CONFIG_STRING_MACRLC_UL_MIN_MCS                    "ul_min_mcs"
#define CONFIG_STRING_MACRLC_UL_MAX_MCS                    "ul_max_mcs"
#define CONFIG_STRING_MACRLC_DL_HARQ_ROUND_MAX             "dl_harq_round_max"
#define CONFIG_STRING_MACRLC_UL_HARQ_ROUND_MAX             "ul_harq_round_max"
#define CONFIG_STRING_MACRLC_MIN_GRANT_PRB                 "min_grant_prb"
#define CONFIG_STRING_MACRLC_IDENTITY_PM                   "identity_precoding_matrix"
#define CONFIG_STRING_MACRLC_ANALOG_BEAMFORMING            "set_analog_beamforming"
#define CONFIG_STRING_MACRLC_BEAM_DURATION                 "beam_duration"
#define CONFIG_STRING_MACRLC_BEAMS_PERIOD                  "beams_per_period"
#define CONFIG_STRING_MACRLC_PUSCH_RSSI_THRESHOLD          "pusch_RSSI_Threshold"
#define CONFIG_STRING_MACRLC_PUCCH_RSSI_THRESHOLD          "pucch_RSSI_Threshold"

#define HLP_MACRLC_UL_PRBBLACK "SNR threshold to decide whether a PRB will be blacklisted or not"
#define HLP_MACRLC_DL_BLER_UP "Upper threshold of BLER to decrease DL MCS"
#define HLP_MACRLC_DL_BLER_LO "Lower threshold of BLER to increase DL MCS"
#define HLP_MACRLC_DL_MIN_MCS "Minimum DL MCS that should be used"
#define HLP_MACRLC_DL_MAX_MCS "Maximum DL MCS that should be used"
#define HLP_MACRLC_UL_BLER_UP "Upper threshold of BLER to decrease UL MCS"
#define HLP_MACRLC_UL_BLER_LO "Lower threshold of BLER to increase UL MCS"
#define HLP_MACRLC_UL_MIN_MCS "Minimum UL MCS that should be used"
#define HLP_MACRLC_UL_MAX_MCS "Maximum UL MCS that should be used"
#define HLP_MACRLC_DL_HARQ_MAX "Maximum number of DL HARQ rounds"
#define HLP_MACRLC_UL_HARQ_MAX "Maximum number of UL HARQ rounds"
#define HLP_MACRLC_MIN_GRANT_PRB "Minimal Periodic ULSCH Grant PRBs"
#define HLP_MACRLC_IDENTITY_PM "Flag to use only identity matrix in DL precoding"
#define HLP_MACRLC_AB "Flag to enable analog beamforming"
#define HLP_MACRLC_BEAM_DURATION "number of consecutive slots for a given set of beams"
#define HLP_MACRLC_BEAMS_PERIOD "set of beams that can be simultaneously allocated in a period"
#define HLP_MACRLC_PUSCH_RSSI_THRESHOLD "Limits PUSCH TPC commands based on RSSI to prevent ADC railing. Value range [-1280, 0], unit 0.1 dBm/dBFS"
#define HLP_MACRLC_PUCCH_RSSI_THRESHOLD "Limits PUCCH TPC commands based on RSSI to prevent ADC railing. Value range [-1280, 0], unit 0.1 dBm/dBFS"

/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                            MacRLC  configuration parameters                                                                           */
/*   optname                                            helpstr   paramflags    XXXptr              defXXXval                  type           numelt     */
/*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
// clang-format off
#define MACRLCPARAMS_DESC { \
  {CONFIG_STRING_MACRLC_CC,                          NULL,                     0, .uptr=NULL,   .defintval=50011,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_TRANSPORT_N_PREFERENCE,      NULL,                     0, .strptr=NULL, .defstrval="local_L1",      TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS,             NULL,                     0, .strptr=NULL, .defstrval="127.0.0.1",     TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_REMOTE_N_ADDRESS,            NULL,                     0, .uptr=NULL,   .defstrval="127.0.0.2",     TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_LOCAL_N_PORTC,               NULL,                     0, .uptr=NULL,   .defintval=50010,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_REMOTE_N_PORTC,              NULL,                     0, .uptr=NULL,   .defintval=50010,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_LOCAL_N_PORTD,               NULL,                     0, .uptr=NULL,   .defintval=50011,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_REMOTE_N_PORTD,              NULL,                     0, .uptr=NULL,   .defintval=50011,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_TRANSPORT_S_PREFERENCE,      NULL,                     0, .strptr=NULL, .defstrval="local_RRC",     TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_LOCAL_S_ADDRESS,             NULL,                     0, .uptr=NULL,   .defstrval="127.0.0.1",     TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_REMOTE_S_ADDRESS,            NULL,                     0, .uptr=NULL,   .defstrval="127.0.0.2",     TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_LOCAL_S_PORTC,               NULL,                     0, .uptr=NULL,   .defintval=50020,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_REMOTE_S_PORTC,              NULL,                     0, .uptr=NULL,   .defintval=50020,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_LOCAL_S_PORTD,               NULL,                     0, .uptr=NULL,   .defintval=50021,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_REMOTE_S_PORTD,              NULL,                     0, .uptr=NULL,   .defintval=50021,           TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_ULSCH_MAX_FRAME_INACTIVITY,  NULL,                     0, .uptr=NULL,   .defintval=10,              TYPE_UINT,    0}, \
  {CONFIG_STRING_MACRLC_PUSCHTARGETSNRX10,           NULL,                     0, .iptr=NULL,   .defintval=200,             TYPE_INT,     0}, \
  {CONFIG_STRING_MACRLC_PUCCHTARGETSNRX10,           NULL,                     0, .iptr=NULL,   .defintval=150,             TYPE_INT,     0}, \
  {CONFIG_STRING_MACRLC_UL_PRBBLACK_SNR_THRESHOLD,   HLP_MACRLC_UL_PRBBLACK,   0, .iptr=NULL,   .defintval=10,              TYPE_INT,     0}, \
  {CONFIG_STRING_MACRLC_PUCCHFAILURETHRES,           NULL,                     0, .iptr=NULL,   .defintval=10,              TYPE_INT,     0}, \
  {CONFIG_STRING_MACRLC_PUSCHFAILURETHRES,           NULL,                     0, .iptr=NULL,   .defintval=10,              TYPE_INT,     0}, \
  {CONFIG_STRING_MACRLC_DL_BLER_TARGET_UPPER,        HLP_MACRLC_DL_BLER_UP,    0, .dblptr=NULL, .defdblval=0.15,            TYPE_DOUBLE,  0}, \
  {CONFIG_STRING_MACRLC_DL_BLER_TARGET_LOWER,        HLP_MACRLC_DL_BLER_LO,    0, .dblptr=NULL, .defdblval=0.05,            TYPE_DOUBLE,  0}, \
  {CONFIG_STRING_MACRLC_DL_MIN_MCS,                  HLP_MACRLC_DL_MIN_MCS,    0, .u8ptr=NULL,  .defintval=0,               TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_DL_MAX_MCS,                  HLP_MACRLC_DL_MAX_MCS,    0, .u8ptr=NULL,  .defintval=28,              TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_UL_BLER_TARGET_UPPER,        HLP_MACRLC_UL_BLER_UP,    0, .dblptr=NULL, .defdblval=0.15,            TYPE_DOUBLE,  0}, \
  {CONFIG_STRING_MACRLC_UL_BLER_TARGET_LOWER,        HLP_MACRLC_UL_BLER_LO,    0, .dblptr=NULL, .defdblval=0.05,            TYPE_DOUBLE,  0}, \
  {CONFIG_STRING_MACRLC_UL_MIN_MCS,                  HLP_MACRLC_UL_MIN_MCS,    0, .u8ptr=NULL,  .defintval=0,               TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_UL_MAX_MCS,                  HLP_MACRLC_UL_MAX_MCS,    0, .u8ptr=NULL,  .defintval=28,              TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_DL_HARQ_ROUND_MAX,           HLP_MACRLC_DL_HARQ_MAX,   0, .u8ptr=NULL,  .defintval=4,               TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_UL_HARQ_ROUND_MAX,           HLP_MACRLC_UL_HARQ_MAX,   0, .u8ptr=NULL,  .defintval=4,               TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_MIN_GRANT_PRB,               HLP_MACRLC_MIN_GRANT_PRB, 0, .u16ptr=NULL, .defintval=5,               TYPE_UINT16,  0}, \
  {CONFIG_STRING_MACRLC_IDENTITY_PM,                 HLP_MACRLC_IDENTITY_PM,   PARAMFLAG_BOOL, .u8ptr=NULL, .defintval=0,   TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_LOCAL_N_ADDRESS_F1U,         NULL,                     0, .strptr=NULL, .defstrval=NULL,            TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_TRANSPORT_S_SHM_PREFIX,      NULL,                     0, .strptr=NULL, .defstrval="nvipc",         TYPE_STRING,  0}, \
  {CONFIG_STRING_MACRLC_TRANSPORT_S_POLL_CORE,       NULL,                     0, .i8ptr=NULL,  .defintval=-1,              TYPE_INT8,    0}, \
  {CONFIG_STRING_MACRLC_ANALOG_BEAMFORMING,          HLP_MACRLC_AB,            PARAMFLAG_BOOL, .u8ptr=NULL, .defintval=0,   TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_BEAM_DURATION,               HLP_MACRLC_BEAM_DURATION, 0, .u8ptr=NULL,  .defintval=1,               TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_BEAMS_PERIOD,                HLP_MACRLC_BEAMS_PERIOD,  0, .u8ptr=NULL,  .defintval=1,               TYPE_UINT8,   0}, \
  {CONFIG_STRING_MACRLC_PUSCH_RSSI_THRESHOLD,        HLP_MACRLC_PUSCH_RSSI_THRESHOLD, \
                                                                               0, .iptr=NULL,   .defintval=0,               TYPE_INT,     0}, \
  {CONFIG_STRING_MACRLC_PUCCH_RSSI_THRESHOLD,        HLP_MACRLC_PUCCH_RSSI_THRESHOLD, \
                                                                               0, .iptr=NULL,   .defintval=0,               TYPE_INT,     0}, \
}
// clang-format off

#define MACRLC_CC_IDX                                          0
#define MACRLC_TRANSPORT_N_PREFERENCE_IDX                      1
#define MACRLC_LOCAL_N_ADDRESS_IDX                             2
#define MACRLC_REMOTE_N_ADDRESS_IDX                            3
#define MACRLC_LOCAL_N_PORTC_IDX                               4
#define MACRLC_REMOTE_N_PORTC_IDX                              5
#define MACRLC_LOCAL_N_PORTD_IDX                               6
#define MACRLC_REMOTE_N_PORTD_IDX                              7
#define MACRLC_TRANSPORT_S_PREFERENCE_IDX                      8
#define MACRLC_LOCAL_S_ADDRESS_IDX                             9
#define MACRLC_REMOTE_S_ADDRESS_IDX                            10
#define MACRLC_LOCAL_S_PORTC_IDX                               11
#define MACRLC_REMOTE_S_PORTC_IDX                              12
#define MACRLC_LOCAL_S_PORTD_IDX                               13
#define MACRLC_REMOTE_S_PORTD_IDX                              14
#define MACRLC_ULSCH_MAX_FRAME_INACTIVITY                      15
#define MACRLC_PUSCHTARGETSNRX10_IDX                           16
#define MACRLC_PUCCHTARGETSNRX10_IDX                           17
#define MACRLC_UL_PRBBLACK_SNR_THRESHOLD_IDX                   18
#define MACRLC_PUCCHFAILURETHRES_IDX                           19
#define MACRLC_PUSCHFAILURETHRES_IDX                           20
#define MACRLC_DL_BLER_TARGET_UPPER_IDX                        21
#define MACRLC_DL_BLER_TARGET_LOWER_IDX                        22
#define MACRLC_DL_MIN_MCS_IDX                                  23
#define MACRLC_DL_MAX_MCS_IDX                                  24
#define MACRLC_UL_BLER_TARGET_UPPER_IDX                        25
#define MACRLC_UL_BLER_TARGET_LOWER_IDX                        26
#define MACRLC_UL_MIN_MCS_IDX                                  27
#define MACRLC_UL_MAX_MCS_IDX                                  28
#define MACRLC_DL_HARQ_ROUND_MAX_IDX                           29
#define MACRLC_UL_HARQ_ROUND_MAX_IDX                           30
#define MACRLC_MIN_GRANT_PRB_IDX                               31
#define MACRLC_IDENTITY_PM_IDX                                 32
#define MACRLC_LOCAL_N_ADDRESS_F1U_IDX                         33
#define MACRLC_TRANSPORT_S_SHM_PREFIX                          34
#define MACRLC_TRANSPORT_S_POLL_CORE                           35
#define MACRLC_ANALOG_BEAMFORMING_IDX                          36
#define MACRLC_ANALOG_BEAM_DURATION_IDX                        37
#define MACRLC_ANALOG_BEAMS_PERIOD_IDX                         38
#define MACRLC_PUSCH_RSSI_THRES_IDX                            39
#define MACRLC_PUCCH_RSSI_THRES_IDX                            40

#define MACRLCPARAMS_CHECK { \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s2 = { config_check_intrange, {0, 31} } }, /* DL min MCS */ \
  { .s2 = { config_check_intrange, {0, 31} } }, /* DL max MCS */ \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s2 = { config_check_intrange, {0, 31} } }, /* UL min MCS */ \
  { .s2 = { config_check_intrange, {0, 31} } }, /* UL max MCS */ \
  { .s2 = { config_check_intrange, {1, 8} } }, /* DL max HARQ rounds */ \
  { .s2 = { config_check_intrange, {1, 8} } }, /* UL max HARQ rounds */ \
  { .s5 = { NULL } }, \
  { .s2 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s2 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s5 = { NULL } }, \
  { .s2 =  { config_check_intrange, {-1280, 0}} }, /* PUSCH RSSI threshold range */ \
  { .s2 =  { config_check_intrange, {-1280, 0}} }, /* PUCCH RSSI threshold range */ \
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#endif
