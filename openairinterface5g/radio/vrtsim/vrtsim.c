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

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#include <common/utils/load_module_shlib.h>
#include <common/utils/telnetsrv/telnetsrv.h>
#include <common/config/config_userapi.h>
#include "common_lib.h"
#include "shm_td_iq_channel.h"
#include "SIMULATION/TOOLS/sim.h"
#include "actor.h"
#include "noise_device.h"
#include "simde/x86/avx512.h"

// Simulator role
typedef enum { ROLE_SERVER = 1, ROLE_CLIENT } role;

#define NUM_CHANMOD_THREADS 1

#define ROLE_CLIENT_STRING "client"
#define ROLE_SERVER_STRING "server"

#define VRTSIM_SECTION "vrtsim"
#define TIME_SCALE_HLP \
  "sample time scale. 1.0 means realtime. Values > 1 mean faster than realtime. Values < 1 mean slower than realtime\n"

// clang-format off
#define VRTSIM_PARAMS_DESC \
  { \
     {"channel_name", "shared memory channel name\n",  0, .strptr = &channel_name,               .defstrval = "vrtsim_channel", TYPE_STRING, 0}, \
     {"role",         "either client or server\n",     0, .strptr = &role,                       .defstrval = ROLE_CLIENT_STRING,  TYPE_STRING, 0}, \
     {"timescale",    TIME_SCALE_HLP,                  0, .dblptr = &vrtsim_state->timescale, .defdblval = 1.0,                 TYPE_DOUBLE, 0}, \
     {"chanmod",      "Enable channel modelling",      0, .iptr = &vrtsim_state->chanmod,     .defintval = 0,                   TYPE_INT,    0}, \
  };
// clang-format on

typedef struct histogram_s {
  uint64_t diff[30];
  int num_samples;
  int min_samples;
  double range;
} histogram_t;

// Information about the peer
typedef struct peer_info_s {
  int num_rx_antennas;
} peer_info_t;

typedef struct tx_timing_s {
  uint64_t tx_samples_late;
  uint64_t tx_early;
  uint64_t tx_samples_total;
  double average_tx_budget;
  histogram_t tx_histogram;
} tx_timing_t;

typedef struct {
  int role;
  char channel_name[40];
  ShmTDIQChannel *channel;
  uint64_t last_received_sample;
  pthread_t timing_thread;
  bool run_timing_thread;
  double timescale;
  double sample_rate;
  uint64_t rx_samples_late;
  uint64_t rx_early;
  uint64_t rx_samples_total;
  tx_timing_t *tx_timing;
  peer_info_t peer_info;
  int chanmod;
  double rx_freq;
  double tx_bw;
  int tx_num_channels;
  int rx_num_channels;
  channel_desc_t *channel_desc;
  Actor_t *channel_modelling_actors;
} vrtsim_state_t;

static void histogram_add(histogram_t *histogram, double diff)
{
  histogram->num_samples++;
  if (histogram->num_samples >= histogram->min_samples) {
    int bin = min(sizeofArray(histogram->diff) - 1, max(0, (int)(diff / histogram->range * sizeofArray(histogram->diff))));
    histogram->diff[bin]++;
  }
}

static void histogram_print(histogram_t *histogram)
{
  LOG_I(HW, "VRTSIM: TX budget histogram: %d samples\n", histogram->num_samples);
  float bin_size = histogram->range / sizeofArray(histogram->diff);
  float bin_start = 0;
  for (int i = 0; i < sizeofArray(histogram->diff); i++) {
    LOG_I(HW, "Bin %d\t[%.1f - %.1fuS]:\t\t%lu\n", i, bin_start, bin_start + bin_size, histogram->diff[i]);
    bin_start += bin_size;
  }
}

static void histogram_merge(histogram_t *dest, histogram_t *src)
{
  for (int i = 0; i < sizeofArray(dest->diff); i++) {
    dest->diff[i] += src->diff[i];
  }
  dest->num_samples += src->num_samples;
}

static void load_channel_model(vrtsim_state_t *vrtsim_state)
{
  load_channellist(vrtsim_state->tx_num_channels,
                   vrtsim_state->peer_info.num_rx_antennas,
                   vrtsim_state->sample_rate,
                   vrtsim_state->rx_freq,
                   vrtsim_state->tx_bw);
  char *model_name = vrtsim_state->role == ROLE_CLIENT ? "client_tx_channel_model" : "server_tx_channel_model";
  vrtsim_state->channel_desc = find_channel_desc_fromname(model_name);
  random_channel(vrtsim_state->channel_desc, 0);
  AssertFatal(vrtsim_state->channel_desc != NULL, "Could not find channel model %s\n", model_name);
}

static void vrtsim_readconfig(vrtsim_state_t *vrtsim_state)
{
  char *channel_name = NULL;
  char *role = NULL;
  paramdef_t vrtsim_params[] = VRTSIM_PARAMS_DESC;
  int ret = config_get(config_get_if(), vrtsim_params, sizeofArray(vrtsim_params), VRTSIM_SECTION);
  AssertFatal(ret >= 0, "configuration couldn't be performed\n");
  strncpy(vrtsim_state->channel_name, channel_name, sizeof(vrtsim_state->channel_name) - 1);
  if (strncmp(role, ROLE_CLIENT_STRING, strlen(ROLE_CLIENT_STRING)) == 0) {
    vrtsim_state->role = ROLE_CLIENT;
  } else if (strncmp(role, ROLE_SERVER_STRING, strlen(ROLE_SERVER_STRING)) == 0) {
    vrtsim_state->role = ROLE_SERVER;
  } else {
    AssertFatal(false, "Invalid role configuration\n");
  }
}

static void *vrtsim_timing_job(void *arg)
{
  vrtsim_state_t *vrtsim_state = arg;
  struct timespec timestamp;
  if (clock_gettime(CLOCK_REALTIME, &timestamp)) {
    LOG_E(UTIL, "clock_gettime failed\n");
    exit(1);
  }
  double leftover_samples = 0;
  while (vrtsim_state->run_timing_thread) {
    struct timespec current_time;
    if (clock_gettime(CLOCK_REALTIME, &current_time)) {
      LOG_E(UTIL, "clock_gettime failed\n");
      exit(1);
    }
    uint64_t diff = (current_time.tv_sec - timestamp.tv_sec) * 1000000000 + (current_time.tv_nsec - timestamp.tv_nsec);
    timestamp = current_time;
    double samples_to_produce = vrtsim_state->sample_rate * vrtsim_state->timescale * diff / 1e9;

    // Attempt to correct compounding rounding error
    leftover_samples += samples_to_produce - (uint64_t)samples_to_produce;
    if (leftover_samples > 1.0f) {
      samples_to_produce += 1;
      leftover_samples -= 1;
    }
    shm_td_iq_channel_produce_samples(vrtsim_state->channel, samples_to_produce);
    usleep(1);
  }
  return 0;
}

/**
 * @brief Exchanges peer information between the server and the client.
 *
 * This function sets up a Unix domain socket server, waits for a client to connect,
 * and exchanges peer information (number of RX antennas) with the client.
 *
 * @param peer_info The peer information to send to the client.
 * @return The peer information received from the client.
 *
 * @note This function will block until a client connects and the information is exchanged.
 */
static peer_info_t server_exchange_peer_info(peer_info_t peer_info)
{
  int server_fd, client_fd;

  // Create a Unix domain socket
  server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  AssertFatal(server_fd >= 0, "socket() failed: errno: %d, %s\n", errno, strerror(errno));

  // Bind the socket to a file path
  struct sockaddr_un addr = {.sun_family = AF_UNIX};
  strncpy(addr.sun_path, "/tmp/vrtsim_socket", sizeof(addr.sun_path) - 1);
  unlink(addr.sun_path); // Ensure the path does not already exist
  int ret = bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
  AssertFatal(ret == 0, "bind() failed: errno: %d, %s\n", errno, strerror(errno));

  // Listen for incoming connections
  ret = listen(server_fd, 1);
  AssertFatal(ret == 0, "listen() failed: errno: %d, %s\n", errno, strerror(errno));

  // Accept a connection from a client
  socklen_t addr_len = sizeof(addr);
  client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
  AssertFatal(client_fd >= 0, "accept() failed: errno: %d, %s\n", errno, strerror(errno));

  // Send the antenna info to the server
  ssize_t bytes_sent = send(client_fd, &peer_info, sizeof(peer_info), 0);
  AssertFatal(bytes_sent == sizeof(peer_info), "send() failed: errno: %d, %s\n", errno, strerror(errno));

  ssize_t bytes_received = recv(client_fd, &peer_info, sizeof(peer_info), 0);
  AssertFatal(bytes_received == sizeof(peer_info), "recv() failed: errno: %d, %s\n", errno, strerror(errno));

  LOG_I(HW, "Received client info: RX %d\n", peer_info.num_rx_antennas);

  // Close the client and server sockets
  close(client_fd);
  close(server_fd);
  return peer_info;
}

/**
 * @brief Exchanges peer information between the client and the server.
 *
 * This function connects to a Unix domain socket server, exchanges peer information
 * (number of RX antennas) with the server, and returns the peer information received
 * from the server.
 *
 * @param peer_info The peer information to send to the server.
 * @return The peer information received from the server.
 *
 * @note This function will block until the information is exchanged.
 */
static peer_info_t client_exchange_peer_info(peer_info_t peer_info)
{
  int client_fd;

  // Create a Unix domain socket
  client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  AssertFatal(client_fd >= 0, "socket() failed: errno: %d, %s\n", errno, strerror(errno));

  // Connect to the server
  struct sockaddr_un addr = {.sun_family = AF_UNIX};
  strncpy(addr.sun_path, "/tmp/vrtsim_socket", sizeof(addr.sun_path) - 1);
  int ret = -1;
  int tries = 0;
  while (ret != 0 && tries < 10) {
    ret = connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
    tries++;
    sleep(1);
  }
  AssertFatal(ret == 0, "connect() failed: errno: %d, %s\n", errno, strerror(errno));

  peer_info_t peer_info_received;
  ssize_t bytes_received = recv(client_fd, &peer_info_received, sizeof(peer_info_received), 0);
  AssertFatal(bytes_received == sizeof(peer_info_received), "recv() failed: errno: %d, %s\n", errno, strerror(errno));

  // Send the antenna info to the server
  ssize_t bytes_sent = send(client_fd, &peer_info, sizeof(peer_info), 0);
  AssertFatal(bytes_sent == sizeof(peer_info), "send() failed: errno: %d, %s\n", errno, strerror(errno));

  LOG_I(HW, "Received peer info: RX %d\n", peer_info_received.num_rx_antennas);

  // Close the client socket
  close(client_fd);
  return peer_info_received;
}

static int vrtsim_connect(openair0_device *device)
{
  vrtsim_state_t *vrtsim_state = (vrtsim_state_t *)device->priv;
  // Exchange peer info
  peer_info_t peer_info = {.num_rx_antennas = device->openair0_cfg[0].rx_num_channels};
  if (vrtsim_state->role == ROLE_SERVER) {
    vrtsim_state->peer_info = server_exchange_peer_info(peer_info);
  } else {
    vrtsim_state->peer_info = client_exchange_peer_info(peer_info);
  }

  // Handle channel modelling after number of RX antennas are known
  int num_tx_stats = 1;
  if (vrtsim_state->chanmod) {
    vrtsim_state->channel_modelling_actors = calloc_or_fail(vrtsim_state->peer_info.num_rx_antennas, sizeof(Actor_t));
    for (int i = 0; i < vrtsim_state->peer_info.num_rx_antennas; i++) {
      init_actor(&vrtsim_state->channel_modelling_actors[i], "chanmod", -1);
    }
    load_channel_model(vrtsim_state);
    num_tx_stats = vrtsim_state->peer_info.num_rx_antennas;
  }
  vrtsim_state->tx_timing = calloc_or_fail(num_tx_stats, sizeof(tx_timing_t));
  for (int i = 0; i < num_tx_stats; i++) {
    vrtsim_state->tx_timing[i].tx_histogram.min_samples = 100;
    // Set the histogram range to 3000uS. Anything above that is not interesting
    vrtsim_state->tx_timing[i].tx_histogram.range = 3000.0;
  }

  // Setup a shared memory channel
  if (vrtsim_state->role == ROLE_SERVER) {
    vrtsim_state->channel = shm_td_iq_channel_create(vrtsim_state->channel_name,
                                                     vrtsim_state->peer_info.num_rx_antennas,
                                                     device->openair0_cfg[0].rx_num_channels);
    vrtsim_state->run_timing_thread = true;
    while (!shm_td_iq_channel_is_connected(vrtsim_state->channel)) {
      LOG_I(HW, "Waiting for client\n");
      sleep(1);
    }
    int ret = pthread_create(&vrtsim_state->timing_thread, NULL, vrtsim_timing_job, vrtsim_state);
    AssertFatal(ret == 0, "pthread_create() failed: errno: %d, %s\n", errno, strerror(errno));
  } else {
    vrtsim_state->channel = shm_td_iq_channel_connect(vrtsim_state->channel_name, 10);
  }

  return 0;
}

static int vrtsim_write_internal(vrtsim_state_t *vrtsim_state,
                                 openair0_timestamp timestamp,
                                 c16_t *samples,
                                 int nsamps,
                                 int aarx,
                                 int flags,
                                 int stats_index)
{
  tx_timing_t *tx_timing = &vrtsim_state->tx_timing[stats_index];

  uint64_t sample = shm_td_iq_channel_get_current_sample(vrtsim_state->channel);
  int64_t diff = timestamp - sample;
  double budget = diff / (vrtsim_state->sample_rate / 1e6);
  tx_timing->average_tx_budget = .05 * budget + .95 * tx_timing->average_tx_budget;
  histogram_add(&tx_timing->tx_histogram, budget);

  int ret = shm_td_iq_channel_tx(vrtsim_state->channel, timestamp, nsamps, aarx, (sample_t *)samples);

  if (ret == CHANNEL_ERROR_TOO_LATE) {
    tx_timing->tx_samples_late += nsamps;
  } else if (ret == CHANNEL_ERROR_TOO_EARLY) {
    tx_timing->tx_early += 1;
  }
  tx_timing->tx_samples_total += nsamps;

  return nsamps;
}

typedef struct {
  vrtsim_state_t *vrtsim_state;
  openair0_timestamp timestamp;
  c16_t *samples[4];
  int nsamps;
  int nbAnt;
  int flags;
  int aarx;
} channel_modelling_args_t;

static void perform_channel_modelling(void *arg)
{
  channel_modelling_args_t *channel_modelling_args = arg;
  int nsamps = channel_modelling_args->nsamps;
  int aarx = channel_modelling_args->aarx;
  int nb_tx_ant = channel_modelling_args->nbAnt;
  c16_t **input_samples = (c16_t **)channel_modelling_args->samples;

  int aligned_nsamps = ceil_mod(nsamps, (512 / 8) / sizeof(cf_t));
  cf_t samples[aligned_nsamps] __attribute__((aligned(64)));
  // Apply noise from global settings
  get_noise_vector((float *)samples, nsamps * 2);

  channel_desc_t *channel_desc = channel_modelling_args->vrtsim_state->channel_desc;
  const float pathloss_linear = powf(10, channel_desc->path_loss_dB / 20.0);

  // Convert channel impulse response to float + apply pathloss
  cf_t channel_impulse_response[nb_tx_ant][channel_desc->channel_length];
  for (int aatx = 0; aatx < nb_tx_ant; aatx++) {
    const struct complexd *channelModel = channel_desc->ch[aarx + (aatx * channel_desc->nb_rx)];
    for (int i = 0; i < channel_desc->channel_length; i++) {
      channel_impulse_response[aatx][i].r = channelModel[i].r * pathloss_linear;
      channel_impulse_response[aatx][i].i = channelModel[i].i * pathloss_linear;
    }
  }

  for (int aatx = 0; aatx < nb_tx_ant; aatx++) {
    for (int i = 0; i < nsamps; i++) {
      cf_t *impulse_response = channel_impulse_response[aatx];
      for (int l = 0; l < channel_desc->channel_length; l++) {
        int idx = i - l;
        // TODO: Use AVX512 for this
        // TODO: What are the previously sent samples (for impulse response)
        if (idx >= 0) {
          c16_t tx_input = input_samples[aatx][idx];
          samples[i].r += tx_input.r * impulse_response[l].r - tx_input.i * impulse_response[l].i;
          samples[i].i += tx_input.i * impulse_response[l].r + tx_input.r * impulse_response[l].i;
        }
      }
    }
  }

  // Convert to c16_t
  c16_t samples_out[aligned_nsamps] __attribute__((aligned(64)));
#if defined(__AVX512F__)
  for (int i = 0; i < aligned_nsamps / 8; i++) {
    simde__m512 *in = (simde__m512 *)&samples[i * 8];
    simde__m256i *out = (simde__m256i *)&samples_out[i * 8];
    *out = simde_mm512_cvtsepi32_epi16(simde_mm512_cvtps_epi32(*in));
  }
#elif defined(__AVX2__)
  for (int i = 0; i < aligned_nsamps / 4; i++) {
    simde__m256 *in = (simde__m256 *)&samples[i * 4];
    simde__m128i *out = (simde__m128i *)&samples_out[i * 4];
    *out = simde_mm256_cvtsepi32_epi16(simde_mm256_cvtps_epi32(*in));
  }
#else
  for (int i = 0; i < nsamps; i++) {
    samples_out[i].r = lroundf(samples[i].r);
    samples_out[i].i = lroundf(samples[i].i);
  }
#endif

  vrtsim_write_internal(channel_modelling_args->vrtsim_state,
                        channel_modelling_args->timestamp,
                        samples_out,
                        channel_modelling_args->nsamps,
                        aarx,
                        channel_modelling_args->flags,
                        aarx);
}

static int vrtsim_write_with_chanmod(vrtsim_state_t *vrtsim_state,
                                     openair0_timestamp timestamp,
                                     void **samplesVoid,
                                     int nsamps,
                                     int nbAnt,
                                     int flags)
{
  for (int aarx = 0; aarx < vrtsim_state->peer_info.num_rx_antennas; aarx++) {
    notifiedFIFO_elt_t *task = newNotifiedFIFO_elt(sizeof(channel_modelling_args_t), 0, NULL, perform_channel_modelling);
    channel_modelling_args_t *args = (channel_modelling_args_t *)NotifiedFifoData(task);
    args->vrtsim_state = vrtsim_state;
    args->timestamp = timestamp;
    args->nsamps = nsamps;
    args->nbAnt = nbAnt;
    args->flags = flags;
    args->aarx = aarx;
    for (int i = 0; i < nbAnt; i++) {
      args->samples[i] = samplesVoid[i];
    }
    pushNotifiedFIFO(&vrtsim_state->channel_modelling_actors[aarx].fifo, task);
  }
  return nsamps;
}

static int vrtsim_write(openair0_device *device, openair0_timestamp timestamp, void **samplesVoid, int nsamps, int nbAnt, int flags)
{
  timestamp -= device->openair0_cfg->command_line_sample_advance;
  vrtsim_state_t *vrtsim_state = (vrtsim_state_t *)device->priv;
  return vrtsim_state->chanmod ? vrtsim_write_with_chanmod(vrtsim_state, timestamp, samplesVoid, nsamps, nbAnt, flags)
                               : vrtsim_write_internal(vrtsim_state, timestamp, (c16_t *)samplesVoid[0], nsamps, 0, flags, 0);
}

static int vrtsim_read(openair0_device *device, openair0_timestamp *ptimestamp, void **samplesVoid, int nsamps, int nbAnt)
{
  vrtsim_state_t *vrtsim_state = (vrtsim_state_t *)device->priv;
  shm_td_iq_channel_wait(vrtsim_state->channel, vrtsim_state->last_received_sample + nsamps);
  int ret = shm_td_iq_channel_rx(vrtsim_state->channel, vrtsim_state->last_received_sample, nsamps, 0, samplesVoid[0]);
  if (ret == CHANNEL_ERROR_TOO_LATE) {
    vrtsim_state->rx_samples_late += nsamps;
  } else if (ret == CHANNEL_ERROR_TOO_EARLY) {
    vrtsim_state->rx_early += 1;
  }
  vrtsim_state->rx_samples_total += nsamps;
  *ptimestamp = vrtsim_state->last_received_sample;
  vrtsim_state->last_received_sample += nsamps;
  return nsamps;
}

static void vrtsim_end(openair0_device *device)
{
  vrtsim_state_t *vrtsim_state = (vrtsim_state_t *)device->priv;
  if (vrtsim_state->role == ROLE_SERVER) {
    vrtsim_state->run_timing_thread = false;
    int ret = pthread_join(vrtsim_state->timing_thread, NULL);
    AssertFatal(ret == 0, "pthread_join() failed: errno: %d, %s\n", errno, strerror(errno));
  }

  tx_timing_t *tx_timing = vrtsim_state->tx_timing;
  if (vrtsim_state->chanmod) {
    for (int i = 0; i < vrtsim_state->peer_info.num_rx_antennas; i++) {
      shutdown_actor(&vrtsim_state->channel_modelling_actors[i]);
    }
    free(vrtsim_state->channel_modelling_actors);
    for (int i = 1; i < vrtsim_state->peer_info.num_rx_antennas; i++) {
      histogram_merge(&tx_timing->tx_histogram, &tx_timing[i].tx_histogram);
      tx_timing->tx_early += tx_timing[i].tx_early;
      tx_timing->tx_samples_late += tx_timing[i].tx_samples_late;
      tx_timing->average_tx_budget += tx_timing[i].average_tx_budget;
      tx_timing->tx_samples_total += tx_timing[i].tx_samples_total;
    }
    tx_timing->average_tx_budget /= vrtsim_state->peer_info.num_rx_antennas;
  }
  // produce 1 second of extra samples so threads can finish
  shm_td_iq_channel_produce_samples(vrtsim_state->channel, vrtsim_state->sample_rate);
  sleep(1);
  shm_td_iq_channel_destroy(vrtsim_state->channel);

  LOG_I(HW,
        "VRTSIM: Realtime issues: TX %.2f%%, RX %.2f%%\n",
        tx_timing->tx_samples_late / (float)tx_timing->tx_samples_total * 100,
        vrtsim_state->rx_samples_late / (float)vrtsim_state->rx_samples_total * 100);
  LOG_I(HW,
        "VRTSIM: Read/write too early (suspected radio implementaton error) TX: %lu, RX: %lu\n",
        tx_timing->tx_early,
        vrtsim_state->rx_early);
  LOG_I(HW, "VRTSIM: Average TX budget %.3lf uS\n", tx_timing->average_tx_budget);
  histogram_print(&tx_timing->tx_histogram);
  free(vrtsim_state->tx_timing);
}

static int vrtsim_stub(openair0_device *device)
{
  return 0;
}
static int vrtsim_stub2(openair0_device *device, openair0_config_t *openair0_cfg)
{
  return 0;
}

static int vrtsim_set_freq(openair0_device *device, openair0_config_t *openair0_cfg)
{
  vrtsim_state_t *s = device->priv;
  s->rx_freq = openair0_cfg->rx_freq[0];
  return 0;
}

__attribute__((__visibility__("default"))) int device_init(openair0_device *device, openair0_config_t *openair0_cfg)
{
  vrtsim_state_t *vrtsim_state = calloc_or_fail(1, sizeof(vrtsim_state_t));
  vrtsim_readconfig(vrtsim_state);
  LOG_I(HW,
        "Running as %s\n",
        vrtsim_state->role == ROLE_SERVER ? "server: waiting for client to connect" : "client: will connect to a vrtsim server");
  device->trx_start_func = vrtsim_connect;
  device->trx_reset_stats_func = vrtsim_stub;
  device->trx_end_func = vrtsim_end;
  device->trx_stop_func = vrtsim_stub;
  device->trx_set_freq_func = vrtsim_set_freq;
  device->trx_set_gains_func = vrtsim_stub2;
  device->trx_write_func = vrtsim_write;
  device->trx_read_func = vrtsim_read;

  device->type = RFSIMULATOR;
  device->openair0_cfg = &openair0_cfg[0];
  device->priv = vrtsim_state;
  device->trx_write_init = vrtsim_stub;
  vrtsim_state->last_received_sample = 0;
  vrtsim_state->sample_rate = openair0_cfg->sample_rate;
  vrtsim_state->rx_freq = openair0_cfg->rx_freq[0];
  vrtsim_state->tx_bw = openair0_cfg->tx_bw;
  vrtsim_state->tx_num_channels = openair0_cfg->tx_num_channels;
  vrtsim_state->rx_num_channels = openair0_cfg->rx_num_channels;

  if (vrtsim_state->chanmod) {
    init_channelmod();
    int noise_power_dBFS = get_noise_power_dBFS();
    int16_t noise_power = noise_power_dBFS == INVALID_DBFS_VALUE ? 0 : (int16_t)(32767.0 / powf(10.0, .05 * -noise_power_dBFS));
    init_noise_device(noise_power);
  }
  return 0;
}
