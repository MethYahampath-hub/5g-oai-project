# LDPC coding implementation
The LDPC encoder and decoder are implemented in a shared library, dynamically loaded at run-time using the [oai shared library loader](file://../../../../common/utils/DOC/loader.md).
Two types of library are available with two different interfaces. There are libraries implementing the encoder and decoder of code segments and libraries implementing the encoder and decoder of slots.

## LDPC slot coding
The interface of the library is defined in [nrLDPC_coding_interface.h](file://../nrLDPC_coding/nrLDPC_coding_interface.h).
The code loading the LDPC library is in [nrLDPC_coding_interface_load.c](file://../nrLDPC_coding/nrLDPC_coding_interface_load.c), in function `load_nrLDPC_coding_interface`, which must be called at init time.

### Selecting the LDPC library at run time

By default the function `int load_nrLDPC_coding_interface(void)` looks for `libldpc.so`.\
This default behavior can be changed using the oai loader configuration options in the configuration file or from the command line as shown below:

#### Examples of ldpc shared lib selection when running nr softmodem's:

loading default `libldpc.so`:

```
./nr-softmodem -O libconfig:gnb.band78.tm1.106PRB.usrpx300.conf:dbgl5
.......................
[CONFIG] loader.ldpc.shlibversion set to default value ""
[LIBCONFIG] loader.ldpc: 2/2 parameters successfully set, (2 to default value)
shlib_path libldpc.so
[LOADER] library libldpc.so successfully loaded
........................
```

`libldpc.so` has its decoder implemented in [nrLDPC_coding_segment_decoder.c](file://../nrLDPC_coding/nrLDPC_coding_segment/nrLDPC_coding_segment_decoder.c).\
Its encoder is implemented in [nrLDPC_coding_segment_encoder.c](file://../nrLDPC_coding/nrLDPC_coding_segment/nrLDPC_coding_segment_encoder.c).

loading `libldpc_t2.so` instead of `libldpc.so`:

`make ldpc_t2`

This command creates the `libldpc_t2.so` shared library.

```
Building C object CMakeFiles/ldpc_t2.dir/openair1/PHY/CODING/nrLDPC_coding/nrLDPC_coding_t2/nrLDPC_coding_t2.c.o
Linking C shared module libldpc_t2.so
```

At runtime, to successfully use the T2 board, you need to install vendor specific drivers and tools.\
Please refer to the dedicated documentation at [LDPC_T2_OFFLOAD_SETUP.md](file://../../../../doc/LDPC_T2_OFFLOAD_SETUP.md).

`./nr-softmodem -O  libconfig:gnb.band78.sa.fr1.106PRB.usrpb210.conf:dbgl5 --rfsim --rfsimulator.serveraddr server  --log_config.gtpu_log_level info  --loader.ldpc.shlibversion _t2 --nrLDPC_coding_t2.dpdk_dev 01:00.0 --nrLDPC_coding_t2.dpdk_core_list 0-1`

``` 

.......................
[CONFIG] loader.ldpc.shlibversion set to default value ""
[LIBCONFIG] loader.ldpc: 2/2 parameters successfully set, (1 to default value)
[CONFIG] shlibversion set to  _t2 from command line
[CONFIG] loader.ldpc 1 options set from command line
shlib_path libldpc_t2.so
[LOADER] library libldpc_t2.so successfully loaded
........................
```

`libldpc_t2.so` has its decoder and its encoder implemented in [nrLDPC_coding_t2.c](file://../nrLDPC_coding/nrLDPC_coding_t2/nrLDPC_coding_t2.c).

loading `libldpc_xdma.so` instead of `libldpc.so`:

`make ldpc_xdma` or `ninja ldpc_xdma`

This command creates the `libldpc_xdma.so` shared library.

```
ninja ldpc_xdma
[2/2] Linking C shared module libldpc_xdma.so
```

At runtime, to successfully use the xdma, you need to install vendor specific drivers and tools.\
Please refer to the dedicated documentation at [LDPC_XDMA_OFFLOAD_SETUP.md](file://../../../../doc/LDPC_XDMA_OFFLOAD_SETUP.md).

`./nr-softmodem -O libconfig:gnb.band78.sa.fr1.106PRB.usrpb210.conf:dbgl5 --rfsim --rfsimulator.serveraddr server --log_config.gtpu_log_level info --loader.ldpc.shlibversion _xdma --nrLDPC_coding_xdma.num_threads_prepare 2`

``` 
.......................
[CONFIG] loader.ldpc.shlibversion set to default value ""
[LIBCONFIG] loader.ldpc: 2/2 parameters successfully set, (1 to default value)
[CONFIG] shlibversion set to  _xdma from command line
[CONFIG] loader.ldpc 1 options set from command line
shlib_path libldpc_xdma.so
[LOADER] library libldpc_xdma.so successfully loaded
........................
``` 

`libldpc_xdma.so` has its decoder implemented in [nrLDPC_coding_xdma.c](file://../nrLDPC_coding/nrLDPC_coding_xdma/nrLDPC_coding_xdma.c).\
Its encoder is implemented in [nrLDPC_coding_segment_encoder.c](file://../nrLDPC_coding/nrLDPC_coding_segment/nrLDPC_coding_segment_encoder.c).

*Note: `libldpc_xdma.so` relies on a segment coding library for encoding.*
*The segment coding library is `libldpc.so` by default but it can be chosen with option `--nrLDPC_coding_xdma.encoder_shlibversion` followed by the library version - like with `--loder.ldpc.shlibversion` in the segment coding case above -*

#### Examples of ldpc shared lib selection when running ldpctest:

Slot coding libraries cannot be used yet within ldpctest.

But they can be used within nr_ulsim, nr_dlsim, nr_ulschsim and nr_dlschsim.\
In these PHY simulators, using the slot coding libraries is enabled in the exact same way as in nr-softmodem.

### LDPC libraries
Libraries implementing the slotwise LDPC coding must be named `libldpc<_version>.so`. They must implement four functions: `nrLDPC_coding_init`, `nrLDPC_coding_shutdown`, `nrLDPC_coding_decoder` and `nrLDPC_coding_encoder`. The prototypes for these functions is defined in [nrLDPC_coding_interface.h](file://../nrLDPC_coding/nrLDPC_coding_interface.h).

`libldpc.so` is completed.

`libldpc_t2.so` is completed.

`libldpc_xdma.so` is completed.

## LDPC segment coding
The interface of the library is defined in [nrLDPC_defs.h](file://../nrLDPC_defs.h).
The code loading the LDPC library is in [nrLDPC_load.c](file://../nrLDPC_load.c), in function `load_nrLDPClib`, which must be called at init time.

LDPC segment coding libraries are not loaded directly by OAI but are statically linked in libraries implementing the LDPC slot coding interface using an adaptation layer between the slot coding interface and the segment coding interface.  
This way, these libraries can be loaded to implement the slot coding interface as well as the segment coding interface in `ldpctest`.

### Selecting the LDPC library at run time

By default the function `int load_nrLDPClib(void)` looks for `libldpc.so`, this default behavior can be changed using the oai loader configuration options in the configuration file or from the command line as shown below:

#### Examples of ldpc shared lib selection when running nr softmodem's:

loading `libldpc_orig.so` instead of `libldpc.so`:

```
./nr-softmodem -O libconfig:gnb.band78.tm1.106PRB.usrpx300.conf:dbgl5  --loader.ldpc.shlibversion _orig
.......................
[CONFIG] loader.ldpc.shlibversion set to default value ""
[LIBCONFIG] loader.ldpc: 2/2 parameters successfully set, (1 to default value)
[CONFIG] shlibversion set to  _orig from command line
[CONFIG] loader.ldpc 1 options set from command line
[LOADER] library libldpc_orig.so successfully loaded
........................
```

loading `libldpc_cl.so` instead of `libldpc.so`:

`make ldpc_cl`

This command creates the `libldpc_cl.so` shared library. To perform this build successfully, only the OpenCL header `(/usr/include/CL/opencl.h)` and library `(/usr/lib/x86_64-linux-gnu/libOpenCL.so)`are required, they implement OpenCL API support which is not hardware dependent.

```
Scanning dependencies of target nrLDPC_decoder_kernels_CL
Built target nrLDPC_decoder_kernels_CL
Scanning dependencies of target ldpc_cl
Building C object CMakeFiles/ldpc_cl.dir/usr/local/oai/oai-develop/openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c.o
In file included from /usr/include/CL/cl.h:32,
                 from /usr/include/CL/opencl.h:38,
                 from /usr/local/oai/oai-develop/openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_decoder_CL.c:49:
/usr/include/CL/cl_version.h:34:9: note: #pragma message: cl_version.h: CL_TARGET_OPENCL_VERSION is not defined. Defaulting to 220 (OpenCL 2.2)
 #pragma message("cl_version.h: CL_TARGET_OPENCL_VERSION is not defined. Defaulting to 220 (OpenCL 2.2)")
         ^~~~~~~

Building C object CMakeFiles/ldpc_cl.dir/usr/local/oai/oai-develop/openairinterface5g/openair1/PHY/CODING/nrLDPC_encoder/ldpc_encoder_optim8segmulti.c.o
Linking C shared module libldpc_cl.so
Built target ldpc_cl

```

At runtime, to successfully use hardware acceleration via OpenCL, you need to install vendor specific packages which deliver the required drivers and tools to make use of their GPU (Nvidia, Intel...) , fpga (Xilinx, Intel) or CPU (Intel, AMD, ARM...) through OpenCL. 

`./nr-softmodem -O  libconfig:gnb.band78.sa.fr1.106PRB.usrpb210.conf:dbgl5 --rfsim --rfsimulator.serveraddr server  --log_config.gtpu_log_level info  --loader.ldpc.shlibversion _cl`

``` 
------------------------------------------------
[LOADER] library libldpc_cl.so successfully loaded
[HW]   Platform 0, OpenCL profile FULL_PROFILE
[HW]   Platform 0, OpenCL version OpenCL 2.1 LINUX
[HW]   Device 0 is  available
[HW]   Device 0, type 2 = 0x00000002: cpu 
[HW]   Device 0, number of Compute Units: 8
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 8192
[HW]   Device 0, max Work Items size for dimension: 1 8192
[HW]   Device 0, max Work Items size for dimension: 2 8192
[New Thread 0x7fffcc258700 (LWP 3945123)]
[New Thread 0x7fffc3e57700 (LWP 3945124)]
[New Thread 0x7fffcbe57700 (LWP 3945125)]
[New Thread 0x7fffcba56700 (LWP 3945126)]
[New Thread 0x7fffcb254700 (LWP 3945128)]
[New Thread 0x7fffcb655700 (LWP 3945127)]
[New Thread 0x7fffcae53700 (LWP 3945129)]
[HW]   Platform 1, OpenCL profile FULL_PROFILE
[HW]   Platform 1, OpenCL version OpenCL 2.0 beignet 1.3
[New Thread 0x7fffc965a700 (LWP 3945130)]
[Thread 0x7fffc965a700 (LWP 3945130) exited]
[HW]   Device 0 is  available
[HW]   Device 0, type 4 = 0x00000004: gpu 
[HW]   Device 0, number of Compute Units: 20
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 512
[HW]   Device 0, max Work Items size for dimension: 1 512
[HW]   Device 0, max Work Items size for dimension: 2 512
-----------------------------------------------------------------
```

`./nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim -O libconfig:/usr/local/oai/conf/nrue_sim.conf:dbgl5 --loader.ldpc.shlibversion _cl --log_config.hw_log_level info`

```
............................................................
[CONFIG] segment_shlibversion set to  _cl from command line
[CONFIG] loader.ldpc 1 options set from command line
[LOADER] library libldpc_cl.so successfully loaded
[HW]   Platform 0, OpenCL profile FULL_PROFILE
[HW]   Platform 0, OpenCL version OpenCL 2.1 LINUX
[HW]   Device 0 is  available
[HW]   Device 0, type 2 = 0x00000002: cpu 
[HW]   Device 0, number of Compute Units: 8
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 8192
[HW]   Device 0, max Work Items size for dimension: 1 8192
[HW]   Device 0, max Work Items size for dimension: 2 8192
[New Thread 0x7fffecccc700 (LWP 3945413)]
[New Thread 0x7fffec8cb700 (LWP 3945415)]
[New Thread 0x7fffec4ca700 (LWP 3945414)]
[New Thread 0x7fffdf7fd700 (LWP 3945417)]
[New Thread 0x7fffdfbfe700 (LWP 3945418)]
[New Thread 0x7fffdffff700 (LWP 3945416)]
[New Thread 0x7fffd73fc700 (LWP 3945419)]
[HW]   Platform 1, OpenCL profile FULL_PROFILE
[HW]   Platform 1, OpenCL version OpenCL 2.0 beignet 1.3
[New Thread 0x7fffde105700 (LWP 3945420)]
[Thread 0x7fffde105700 (LWP 3945420) exited]
[HW]   Device 0 is  available
[HW]   Device 0, type 4 = 0x00000004: gpu 
[HW]   Device 0, number of Compute Units: 20
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 512
[HW]   Device 0, max Work Items size for dimension: 1 512
[HW]   Device 0, max Work Items size for dimension: 2 512 
------------------------------------------------------------
```

A mechanism to select ldpc implementation is also available in the `ldpctest` phy simulator via the `-v` option, which can be used to specify the version of the ldpc shared library to be used.

#### Examples of ldpc shared lib selection when running ldpctest:

Loading libldpc_cuda.so, the cuda implementation of the ldpc decoder:

```
$ ./ldpctest -v _cuda
ldpctest -v _cuda
Initializing random number generator, seed 0
block length 8448: 
n_trials 1: 
SNR0 -2.000000: 
[CONFIG] get parameters from cmdline , debug flags: 0x00400000
[CONFIG] log_config: 2/3 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
log init done
[CONFIG] loader: 2/2 parameters successfully set 
[CONFIG] loader.ldpc: 2/2 parameters successfully set 
[LOADER] library libldpc_cuda.so successfully loaded
...................................
```


Loading libldpc_cl.so, the opencl implementation of the ldpc decoder:

`make ldpc_cl`


```
$ ./ldpctest -v _cl
Initializing random number generator, seed 0
block length 8448: 
n_trials 1: 
SNR0 -2.000000: 
[CONFIG] get parameters from cmdline , debug flags: 0x00400000
[CONFIG] log_config: 2/3 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 53/53 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
[CONFIG] log_config: 16/16 parameters successfully set 
log init done
[CONFIG] loader: 2/2 parameters successfully set 
[CONFIG] loader.ldpc: 1/2 parameters successfully set 
[LOADER] library libldpc_cl.so successfully loaded
[HW]   Platform 0, OpenCL profile FULL_PROFILE
[HW]   Platform 0, OpenCL version OpenCL 2.1 LINUX
[HW]   Device 0 is  available
[HW]   Device 0, type 2 = 0x00000002: cpu 
[HW]   Device 0, number of Compute Units: 8
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 8192
[HW]   Device 0, max Work Items size for dimension: 1 8192
[HW]   Device 0, max Work Items size for dimension: 2 8192
[HW]   Platform 1, OpenCL profile FULL_PROFILE
[HW]   Platform 1, OpenCL version OpenCL 2.0 beignet 1.3
[HW]   Device 0 is  available
[HW]   Device 0, type 4 = 0x00000004: gpu 
[HW]   Device 0, number of Compute Units: 20
[HW]   Device 0, max Work Items dimension: 3
[HW]   Device 0, max Work Items size for dimension: 0 512
[HW]   Device 0, max Work Items size for dimension: 1 512
[HW]   Device 0, max Work Items size for dimension: 2 512
................................
```



### LDPC libraries
Libraries implementing the LDPC algorithms must be named `libldpc<_version>.so`, they must implement three functions: `nrLDPC_initcall` `nrLDPC_decod` and `nrLDPC_encod`. The prototypes for these functions is defined in [nrLDPC_defs.h](file://nrLDPC_defs.h).

`libldpc_cuda.so`has been tested with the `ldpctest` executable, usage from the softmodem's has to be tested.

`libldpc_cl.so`is under development.

[oai Wikis home](https://gitlab.eurecom.fr/oai/openairinterface5g/wikis/home)
