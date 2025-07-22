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

/*! \file ngap_common.c
 * \brief ngap procedures for both gNB and AMF
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com (yoshio.inoue%40fujitsu.com%2cmasayuki.harada%40fujitsu.com)
 * \date 2020
 * \version 0.1
 */

#include <stdint.h>
#include "ngap_common.h"

int asn1_xer_print = 0;

void encode_ngap_cause(NGAP_Cause_t *out, const ngap_cause_t *in)
{
  switch (in->type) {
    case NGAP_CAUSE_RADIO_NETWORK:
      out->present = NGAP_Cause_PR_radioNetwork;
      out->choice.radioNetwork = in->value;
      break;

    case NGAP_CAUSE_TRANSPORT:
      out->present = NGAP_Cause_PR_transport;
      out->choice.transport = in->value;
      break;

    case NGAP_CAUSE_NAS:
      out->present = NGAP_Cause_PR_nas;
      out->choice.nas = in->value;
      break;

    case NGAP_CAUSE_PROTOCOL:
      out->present = NGAP_Cause_PR_protocol;
      out->choice.protocol = in->value;
      break;

    case NGAP_CAUSE_MISC:
      out->present = NGAP_Cause_PR_misc;
      out->choice.misc = in->value;
      break;

    case NGAP_CAUSE_NOTHING:
    default:
      AssertFatal(false, "Unknown failure cause %d\n", in->type);
      break;
  }
}
