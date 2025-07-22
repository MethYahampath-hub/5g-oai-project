# Overview

This implements a shared memory realtime and near-realtime radio interface.
This is performed using `shm_td_iq_channel` which handles the shared memory
interface.

# Limitations

 - Only 1gNB to 1nrUE: gNB as a server, UE as a client.
 - Server only accepts connections during device bringup. This means that
   restarting the client will not make it reconnect, the server has to be
   restarted as well.

# Usage

On the UE and gNB: use `device.name vrtsim` command line argument.

Additionally on gNB use `vrtsim.role server` and optionally
`vrtsim.timescale <timescale>` to set the timescale. Timescale 1.0
is the default and means realtime.

Channel modelling can be enabled by adding `vrtsim.chanmod 1` to the
command line and should work the same as channel modelling in rfsimulator,
see rfsimulator [documentation](../rfsimulator/README.md), provided that your
CPU is fast enough.
