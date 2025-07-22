# Physical Simulators ("Physims") User Guide

This document provides an overview and usage guide for the **physical simulators**, also referred to as **unitary
simulators** or simply **physims**, used in the OpenAirInterface (OAI) project for LTE and 5G PHY layer validation.

[[_TOC_]]

# Introduction

**Unitary simulations** are standalone test programs designed to validate individual physical layer (L1) transport or
control channels. These simulations are based on **Monte Carlo techniques**, enabling statistical evaluation of
performance metrics such as block error rate (BLER), hybrid automatic repeat request (HARQ) throughput, and decoding
accuracy under various signal conditions.

Physims are essential for:

* Debugging and evaluating new PHY code in isolation
* Regression testing
* Ensuring correctness before merging new contributions into the repository

## Examples of Simulators

| Technology | Simulators                                | Description                      |
|------------|-------------------------------------------|----------------------------------|
| 4G LTE     | `dlsim`, `ulsim`                          | Downlink and uplink simulators   |
| 5G NR      | `nr_dlsim`, `nr_ulsim`                    | Downlink and uplink simulators   |
|            | `nr_dlschsim`, `nr_ulschsim`              | HARQ and TB throughput tests     |
|            | `nr_pucchsim`                             | Control channel simulation       |
|            | `nr_pbchsim`                              | Broadcast channel simulation     |
|            | `nr_prachsim`                             | PRACH simulation                 |
|            | `nr_psbchsim`                             | Sidelink simulation              |
| Coding     | `ldpctest`, `polartest`, `smallblocktest` | LDPC, Polar, and other FEC tests |

### Source Locations

* 4G PHY simulators: `openair1/SIMULATION/LTE_PHY/`
* 5G PHY simulators: `openair1/SIMULATION/NR_PHY/`
* Coding unit tests: `openair1/PHY/CODING/TESTBENCH/`

Example:

```bash
# 5G Downlink simulator
openair1/SIMULATION/NR_PHY/dlsim.c
```

# How to Run Simulators Using `ctest`

## Option 1: Using CMake

```bash
cd openairinterface5g
mkdir build && cd build
cmake .. -GNinja -DENABLE_PHYSIM_TESTS=ON
ninja tests
ctest
```

## Option 2: Using the `build_oai` script

This method simplifies the process by automatically building the simulators with ctest support.

```bash
cd openairinterface5g/cmake_targets
./build_oai --ninja --phy_simulators
cd ran_build/build
ctest
```

# `ctest` Usage Tips

## Useful Options

Use the following options to customize test execution:

| Option           | Description                                          |
|------------------|------------------------------------------------------|
| `-R <regex>`     | Run tests matching the regex pattern (by name)       |
| `-L <regex>`     | Run tests with labels matching the regex pattern     |
| `-E <regex>`     | Exclude tests matching the regex pattern (by name)   |
| `-LE <regex>`    | Exclude tests with labels matching the regex pattern |
| `--print-labels` | Display all available test labels                    |
| `-j <jobs>`      | Run tests in parallel using specified number of jobs |

For the complete list of `ctest` options, refer to the manual:

```bash
man ctest
```

## Example

```bash
# Run only NR ULSCH simulator tests, in parallel
ctest -L nr_ulsch -j 4
```

# Adding a New Physim Test

To define a new test or modify existing ones, update the following file:

```
openair1/PHY/CODING/tests/CMakeLists.txt
```

Use the `add_physim_test` macro with the following arguments:

```cmake
add_physim_test(<test_gen> <test_exec> <test_description> <test_label> <test_cl_options>)
```

### Arguments:

* `<test_gen>`: Test generation (e.g., `4g` or `5g`)
* `<test_exec>`: Name of the test executable (e.g., `nr_prachsim`)
* `<test_description>`: Description shown in `ctest` output, useful for categorization and indexing.
* `<test_label>`: Label used for filtering tests (via `-L`), shown in the ctest output summary as a descriptive tag.
* `<test_cl_options>`: Command line options passed to the test

### Example:

```cmake
# add_physim_test(<test_gen> <test_exec> <test_description> <test_label> <test_cl_options>)
add_physim_test(5g nr_prachsim test8 "15kHz SCS, 25 PRBs" -a -s -30 -n 300 -p 99 -R 25 -m 0)
```

These tests are run automatically as part of the following
pipelines: [RAN-PhySim-Cluster-4G](https://jenkins-oai.eurecom.fr/job/RAN-PhySim-Cluster-4G/) and [RAN-PhySim-Cluster-5G](https://jenkins-oai.eurecom.fr/job/RAN-PhySim-Cluster-5G/)

### How to rerun failed CI tests using `ctest`

Ctest automatically logs the failed tests in LastTestsFailed.log. This log is archived in
the CI artifacts and can be reused locally to rerun only those failed tests.

```bash
# 1. Navigate to the build directory, build physims
cd ~/openairinterface5g/build
cmake .. -GNinja -DENABLE_PHYSIM_TESTS=ON && ninja tests

# 2. Create ctest directory structure
ctest -N --quiet

# 3. Unzip the test logs artifact from the CI run
unzip /path/to/test_logs_123.zip

# 4. Copy the LastTestsFailed.log file to the expected location
cp /path/to/test_logs_123/test_logs/LastTestsFailed.log ./Testing/Temporary/

# 5. Rerun only the failed tests using ctest
ctest --rerun-failed
```

# Legacy Bash Autotest (Deprecated)

> **Note:** Autotest script, configuration, and documentation, are no longer maintained.

For legacy support or archival purposes, you can still find this implementation by checking out the historical tag:

```bash
git checkout 2025.w18
```
