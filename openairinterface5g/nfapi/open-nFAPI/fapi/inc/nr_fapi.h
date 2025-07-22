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

#ifndef OPENAIRINTERFACE_NR_FAPI_H
#define OPENAIRINTERFACE_NR_FAPI_H

#include "stdint.h"
#include "string.h"
#include "nfapi_interface.h"
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include "assertions.h"
#include "debug.h"

#define INDENTED_PRINTF(format, ...) printf("%*s" format, depth * 2, "", ##__VA_ARGS__)

#define EQ_TLV(_tlv_a, _tlv_b)        \
  do {                                \
    EQ(_tlv_a.tl.tag, _tlv_b.tl.tag); \
    EQ(_tlv_a.value, _tlv_b.value);   \
  } while (0)

#define EQ(_a, _b)      \
  do {                  \
    if ((_a) != (_b)) { \
      return false;     \
    }                   \
  } while (0)

#define COPY_TL(_dst_tl, _src_tl)    \
  do {                               \
    _dst_tl.tag = _src_tl.tag;       \
    _dst_tl.length = _src_tl.length; \
  } while (0)

#define COPY_TLV(_dst, _src)   \
  do {                         \
    COPY_TL(_dst.tl, _src.tl); \
    _dst.value = _src.value;   \
  } while (0)

typedef struct {
  uint8_t num_msg;
  uint8_t opaque_handle;
  uint16_t message_id;
  uint32_t message_length;
} fapi_message_header_t;

void copy_vendor_extension_value(nfapi_vendor_extension_tlv_t *dst, const nfapi_vendor_extension_tlv_t *src);

bool isFAPIMessageIDValid(uint16_t id);

int check_nr_fapi_unpack_length(nfapi_nr_phy_msg_type_e msgId, uint32_t unpackedBufLen);

int fapi_nr_message_header_unpack(uint8_t **pMessageBuf,
                                  uint32_t messageBufLen,
                                  void *pUnpackedBuf,
                                  uint32_t unpackedBufLen,
                                  nfapi_p4_p5_codec_config_t *config);

#endif // OPENAIRINTERFACE_NR_FAPI_H
