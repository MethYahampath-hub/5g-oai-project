This document describes the SmallCellForum (SCF) (n)FAPI split in 5G, i.e.,
between the MAC/L2 and PHY/L1.

The interested reader is recommended to read a copy of the SCF 222.10
specification ("FAPI"). This includes information on what is P5, P7, and how
FAPI works. The currently used version is SCF 222.10.02, with some messages
upgraded to SCF 222.10.04 due to bugfixes in the spec. Further information
about nFAPI can be found in SCF 225.2.0.

# Quickstart

Compile OAI as normal. Start the CN and make sure that the VNF configuration
matches the PLMN/IP addresses. Then, run the VNF

    sudo NFAPI_TRACE_LEVEL=info ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb-vnf.sa.band78.106prb.nfapi.conf --nfapi VNF

Afterwards, start and connect the PNF

    sudo NFAPI_TRACE_LEVEL=info ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb-pnf.band78.rfsim.conf --nfapi PNF --rfsim

Finally, you can start the UE (observe the radio configuration info in the
VNF!)

    sudo ./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 -O ue.conf

You should not observe a difference between nFAPI split and monolithic.


# Status

All FAPI message can be transferred between VNF and PNF. This is because OAI
uses FAPI with its corresponding messages internally, whether a split is in use
or not.

The nFAPI split mode supports any radio configuration that is also supported by
the monolithic gNB, with the notable exceptions that only numerologies of 15kHz
and 30kHz (mu=0 and mu=1, respectively) are supported.

The VNF requests to be notified about every slot by the PNF. No delay
management is employed as of now; instead, the PNF sends a Slot.indication to
the VNF in every slot (in divergence from the nFAPI spec). 

Currently, downlink transmissions work the same in monolithic and nFAPI. In
uplink, we observe an increased number of retransmissions, which limits the MCS
and hence the achievable throughput (which is limited to 10-20Mbps). We are still
debugging the root cause of this.

After stopping the PNF, you also have to restart the VNF.

When using RFsim, the system might run slower than in monolithic. This is
because the PNF needs to slow down the execution time of a specific slot,
because it has to send a Slot.indication to the VNF for scheduling.

# Configuration

Both PNF and VNF are run through the `nr-softmodem` executable. The type of
mode is switched through the `--nfapi` switch, with options `MONOLITHIC`
(default if not provided), `VNF`, `PNF`.

If the type is `VNF`, you have to modify the `MACRLCs.tr_s_preference`
(transport south preference) to `nfapi`. Further, configure these options:

- `MACRLCs.remote_s_address` (remote south address): IP of the PNF
- `MACRLCs.local_s_address` (local south address): IP of the VNF
- `MACRLCs.local_s_portc` (local south port for control): VNF's P5 local port
- `MACRLCs.remote_s_portc` (remote south port for data): PNF's P5 remote port
- `MACRLCs.local_s_portd` (local south port for control): VNF's P5 local port
- `MACRLCs.remote_s_portd` (remote south port for data): PNF's P7 remote port

Note that any L1-specific section (`L1s`, `RUs`,
RFsimulator-specific/IF7.2-specific configuration or other radios, if
necessary) will be ignored and can be deleted.

If the type is `PNF`, you have to modify modify the `L1s.tr_n_preference`
(transport north preference) to `nfapi`. Further, configure these options:

- `L1s.remote_n_address` (remote north address): IP of the VNF
- `L1s.local_n_address` (local north address): IP of the PNF
- `L1s.local_n_portc` (local north port for control): PNF's P5 local port
- `L1s.remote_n_portc` (remote north port for control): VNF's P5 remote port
- `L1s.local_n_portd` (local north port for data): PNF's P7 local port
- `L1s.remote_n_portd` (remote north port for data): VNF's P7 remote port

Note that this file should contain additional, L1-specific sections (`L1s`,
`RUs` RFsimulator-specific/IF7.2-specific configuration or other radios, if
necessary).

To split an existing config file `monolithic.conf` for nFAPI operation, you
can proceed as follows:

- copy `monolithic.conf`, which will be your VNF file (`vnf.conf`)
- in `vnf.conf`
  * modify `MACRLCs` section to configure south-bound nFAPI transport
  * delete `L1s`, `RUs`, and radio-specific sections.
  * in `gNBs` section, increase the `ra_ResponseWindow` by one to extend the RA
    window: this is necessary because the PNF triggers the scheduler in the VNF
    in advance, which might make the RA window more likely to run out
- copy `monolithic.conf`, which will be your PNF file (`pnf.conf`)
- in `pnf.conf`
  * modify `L1s` section to configure north-bound nFAPI transport (make sure it
    matches the `MACRLCs` section for `vnf.conf`
  * delete all the `gNBs`, `MACRLCs`, `security` sections (they are not needed)
- if you have root-level options in `monolithic.conf`, such as
  `usrp-tx-thread-config` or `tune-offset`, make sure to to add them to
  `pnf.conf`, or provide them on the command line for the PNF.
- to run, proceed as described in the quick start above.

Note: all L1-specific options have to be passed to the PNF, and remaining
options to the VNF.

# nFAPI logging system

nFAPI has its own logging system, independent of OAI's. It can be activated by
setting the `NFAPI_TRACE_LEVEL` environment variable to an appropriate value;
see [the environment variables documentation](./environment-variables.md) for
more info.

To see the (any) periodical output at the PNF, define `NFAPI_TRACE_LEVEL=info`.
This output shows:

```
41056.739654 [I] 3556767424: pnf_p7_slot_ind: [P7:1] msgs ontime 489 thr DL 0.06 UL 0.01 msg late 0 (vtime)
```

The first numbers are timestamps. `pnf_p7_slot_ind` is the name of the
functions that prints the output. `[P7:1]` refers to the fact that these are
information on P7, of PHY ID 1. Finally, `msgs ontime 489` means that in the
last window (since the last print), 489 messages arrived at the PNF in total.
The combined throughput of `TX_data.requests` (DL traffic) was 0.06 Mbps; note
that this includes SIB1 and other periodical data channels. In UL, 0.01 Mbps
have been sent through `RX_data.indication`. `msg late 0` means that 0 packets
have been late. _This number is an aggregate over the total runtime_, unlike
the other messages. Finally, `(vtime)` is a reminder that the calculations are
done over virtual time, i.e., frames/slots as executed by the 5G Systems. For
instance, these numbers might be slightly higher or slower in RFsim than in
wall-clock time, depending if the system advances faster or slower than
wall-clock time.
