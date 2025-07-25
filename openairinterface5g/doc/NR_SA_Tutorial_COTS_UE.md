<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI 5G NR SA tutorial with COTS UE</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

[[_TOC_]]

#  1. Scenario
In this tutorial we describe how to configure and run a 5G end-to-end setup with OAI CN5G, OAI gNB and COTS UE.

Minimum hardware requirements:
- Laptop/Desktop/Server for OAI CN5G and OAI gNB
    - Operating System: [Ubuntu 24.04 LTS](https://releases.ubuntu.com/24.04/ubuntu-24.04.2-desktop-amd64.iso)
    - CPU: 8 cores x86_64 @ 3.5 GHz
    - RAM: 32 GB
- Laptop for UE
    - Operating System: Microsoft Windows 10 x64
    - CPU: 4 cores x86_64
    - RAM: 8 GB
    - Windows driver for Quectel MUST be equal or higher than version **2.4.6**
- [USRP B210](https://www.ettus.com/all-products/ub210-kit/), [USRP N300](https://www.ettus.com/all-products/USRP-N300/) or [USRP X300](https://www.ettus.com/all-products/x300-kit/)
    - Please identify the network interface(s) on which the USRP is connected and update the gNB configuration file
- Quectel RM500Q
    - Module, M.2 to USB adapter, antennas and SIM card
    - Firmware version of Quectel MUST be equal or higher than **RM500QGLABR11A06M4G**


# 2. OAI CN5G

## 2.1 OAI CN5G pre-requisites

Please install and configure OAI CN5G as described here:
[OAI CN5G](NR_SA_Tutorial_OAI_CN5G.md)


## 2.2  SIM Card
Program UICC/SIM Card with [Open Cells Project](https://open-cells.com/) programming tool [uicc-v3.3](https://open-cells.com/d5138782a8739209ec5760865b1e53b0/uicc-v3.3.tgz).

```bash
sudo ./program_uicc --adm 12345678 --imsi 001010000000001 --isdn 00000001 --acc 0001 --key fec86ba6eb707ed08905757b1bb44b8f --opc C42449363BBAD02B66D16BC975D77CC1 -spn "OpenAirInterface" --authenticate
```


# 3. OAI gNB

## 3.1 OAI gNB pre-requisites

### Build UHD from source
```bash
# https://files.ettus.com/manual/page_build_guide.html
sudo apt install -y autoconf automake build-essential ccache cmake cpufrequtils doxygen ethtool g++ git inetutils-tools libboost-all-dev libncurses-dev libusb-1.0-0 libusb-1.0-0-dev libusb-dev python3-dev python3-mako python3-numpy python3-requests python3-scipy python3-setuptools python3-ruamel.yaml

git clone https://github.com/EttusResearch/uhd.git ~/uhd
cd ~/uhd
git checkout v4.8.0.0
cd host
mkdir build
cd build
cmake ../
make -j $(nproc)
make test # This step is optional
sudo make install
sudo ldconfig
sudo uhd_images_downloader
```

## 3.2 Build OAI gNB

```bash
# Get openairinterface5g source code
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g
git checkout develop

# Install OAI dependencies
cd ~/openairinterface5g/cmake_targets
./build_oai -I

# Build OAI gNB
cd ~/openairinterface5g/cmake_targets
./build_oai -w USRP --ninja --gNB -C
```

# 4. Run OAI CN5G and OAI gNB

## 4.1 Run OAI CN5G

```bash
cd ~/oai-cn5g
docker compose up -d
```

## 4.2 Run OAI gNB

**Note:** From tag `2024.w45`, OAI gNB runs by default in standalone (SA) mode.  
In earlier versions the default mode was non-standalone (NSA).  
If you are using an earlier version than `2024.w45`, you should add the `--sa` argument to the sample commands below to obtain a correct behavior.

### USRP B210
```bash
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf -E --continuous-tx
```
### USRP N300
```bash
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band77.fr1.273PRB.2x2.usrpn300.conf --usrp-tx-thread-config 1
```

### USRP X300
```bash
cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band77.fr1.273PRB.2x2.usrpn300.conf --usrp-tx-thread-config 1 -E --continuous-tx
```

# 5. Run UE
## 5.1 Testing with Quectel RM500Q

### 5.1.1 Setup Quectel
With [PuTTY](https://the.earth.li/~sgtatham/putty/latest/w64/putty.exe), send the following AT commands to the module using a serial interface (ex: COM2) at 115200 bps:
```bash
# MUST be sent at least once everytime there is a firmware upgrade!
AT+CGDCONT=1,"IP","oai"
AT+CGDCONT=2
AT+CGDCONT=3

# (Optional, debug only, AT commands) Activate PDP context, retrieve IP address and test with ping
AT+CGACT=1,1
AT+CGPADDR=1
AT+QPING=1,"openairinterface.org"
```

### 5.1.2 Ping test
- UE host
```bash
ping 192.168.70.135 -t -S 12.1.1.2
```
- CN5G host
```bash
docker exec -it oai-ext-dn ping 12.1.1.2
```

### 5.1.3 Downlink iPerf test
- UE host
    - Download iPerf for Microsoft Windows from [here](https://iperf.fr/download/windows/iperf-2.0.9-win64.zip).
    - Extract to Desktop and run with Command Prompt:
```bash
cd C:\Users\User\Desktop\iPerf\iperf-2.0.9-win64\iperf-2.0.9-win64
iperf -s -u -i 1 -B 12.1.1.2
```

- CN5G host
```bash
docker exec -it oai-ext-dn iperf -u -t 86400 -i 1 -fk -B 192.168.70.135 -b 100M -c 12.1.1.2
```

# 6. Advanced configurations (optional)

See also the [dedicated document on performance tuning](./tuning_and_security.md).

## 6.1 USRP N300 and X300 Ethernet Tuning

Please also refer to the official [USRP Host Performance Tuning Tips and Tricks](https://kb.ettus.com/USRP_Host_Performance_Tuning_Tips_and_Tricks) tuning guide.

The following steps are recommended. Please change the network interface(s) as required. Also, you should have 10Gbps interface(s): if you use two cables, you should have the XG interface. Refer to the [N300 Getting Started Guide](https://kb.ettus.com/USRP_N300/N310/N320/N321_Getting_Started_Guide) for more information.

* Use an MTU of 9000: how to change this depends on the network management tool. In the case of Network Manager, this can be done from the GUI.
* Increase the kernel socket buffer (also done by the USRP driver in OAI)
* Increase Ethernet Ring Buffers: `sudo ethtool -G <ifname> rx 4096 tx 4096`
* Disable hyper-threading in the BIOS (This step is optional)
* Optional: Disable KPTI Protections for Spectre/Meltdown for more performance. **This is a security risk.** Add `mitigations=off nosmt` in your grub config and update grub. (This step is optional)

Example code to run:
```
for ((i=0;i<$(nproc);i++)); do sudo cpufreq-set -c $i -r -g performance; done
sudo sysctl -w net.core.wmem_max=62500000
sudo sysctl -w net.core.rmem_max=62500000
sudo sysctl -w net.core.wmem_default=62500000
sudo sysctl -w net.core.rmem_default=62500000
sudo ethtool -G enp1s0f0 tx 4096 rx 4096
```

## 6.2 Real-time performance workarounds
- Enable Performance Mode `sudo cpupower idle-set -D 0`
- If you get real-time problems on heavy UL traffic, reduce the maximum UL MCS using an additional command-line switch: `--MACRLCs.[0].ul_max_mcs 14`.
- You can also reduce the number of LDPC decoder iterations, which will make the LDPC decoder take less time: `--L1s.[0].max_ldpc_iterations 4`.

## 6.3 Uplink issues related with noise on the DC carriers

With devices like the USRP N300 and especially the X300, there is noise in the DC carriers: this can cause uplink PRBs that overlap with the DC carrier to experience interference and increased noise.

There are two possible solution that can be enabled in OAI:

* `--tune-offset`: it consists in shifting away the operational bandwidth to avoid the center frequency
* `ul_prbblacklist`: can be used to define specific PRBs that should not be used for uplink scheduling

A spectrum clean from the noisy PRBs will eventually result in an enhanced UL throughput.

Using `--tune-offset`, `ul_prbblacklist` or none at all is depending on the combination of the USRP model, the operational bandwidth configuration and the bandwidth of the daughterboard.

There are two main points to keep in mind:

- USRPs come with various daughterboards, each with its own specific bandwidth (see datasheets).
- Using `--tune-offset` to shift the entire operational bandwidth to avoid the center frequency could exceed the device capabilities: if the shifted operational bandwidth falls entirely within the bandwidth of the daughterboard, than `--tune-offset` is sufficient, otherwise  `--tune-offset` is ineffective and `ul_prbblacklist` is needed.

The option `ul_prbblacklist` is more relevant when using high-bandwidth configurations (e.g., 100 MHz) with devices like the USRP N310 or X310: in this scenarios, `--tune-offset` could not be sufficient to get rid of the noisy PRBs in the center frequency entirely, because it is not possible to shift the DC carriers out of the bandwidth (tune offset shall be smaller than half the bandwidth of the board).

## 6.3.1 Tune offset

The value passed to the command line option `--tune-offset <Hz>` will be calling an UHD API. It represents the LO offset frequency in Hz.
The API (tune_request_t class) will send a frequency tuning request (`tx_tune_req`, `rx_tune_req`) to the USRP device, in order to configure the target baseband tx/rx frequency, therefore shifting the tx/rx signal spectrum.

The value passed to this option should be ideally equal to half the operational bandwidth, or possibly less, depending on the bandwidth configuration, and also it shall be lower or equal than half the bandwidth of the board.

A visual representation of the impact of tune-offset with a 120 MHz bandwidth daughterboard:

![Tune_Offset](./images/USRP_tune_offset.png)

## 6.3.2 UL PRBs Blacklist

To use this option, in the configuration file, e.g. 100 MHz bandwidth setup: `ul_prbblacklist = "135,136,137,138"`.

## 6.4 Lower latency on user plane
- To lower latency on the user plane, you can force the UE to be scheduled constantly in uplink: `--MACRLCs.[0].ulsch_max_frame_inactivity 0` .
