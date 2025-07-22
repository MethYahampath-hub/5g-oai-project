# Define a macro to add tests for different physical simulators
macro(add_physim_test test_gen test_exec test_description test_label)
  # catch all the arguments past the last expected arqument and store them in the options_list
  set(options_list ${ARGN})
  set(test_name physim.${test_gen}.${test_exec}.${test_description})
  add_test(
    NAME ${test_name}
    COMMAND ${test_exec} ${options_list}
    COMMAND_EXPAND_LISTS
  )
  set_tests_properties(${test_name} PROPERTIES
    LABELS "${test_exec}: ${test_label};${test_exec}"
    ENVIRONMENT "LD_LIBRARY_PATH=."
  )
  add_dependencies(tests ${test_exec})
endmacro()

####################################################################################
######                           dlsim unit test                              ######
####################################################################################

add_physim_test(4g dlsim basic.test1 "10 MHz, R2.FDD (MCS 5), EVA5, -1dB" -m=5 -g=F -s=-1 -w=1.0 -f=.2 -n=1500 -B=50 -c=2 -z=2 -Tperf=60)
add_physim_test(4g dlsim basic.test2 "1.4 MHz, R4.FDD (MCS 4), EVA5, 0dB (70%)" -m=4 -g=F -s=0 -w=1.0 -f=.2 -n=1500 -B=6 -c=4 -z=2 -Tperf=60)
add_physim_test(4g dlsim basic.test3 "10 MHz, R3.FDD (MCS 15), EVA5, 6.7dB (70%)" -m=15 -g=F -s=6.7 -w=1.0 -f=.2 -n=1500 -B=50 -c=2 -z=2 -Tperf=60)
add_physim_test(4g dlsim basic.test4 "5 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (70%)" -m=15 -g=F -s=6.7 -w=1.0 -f=.2 -n=1500 -B=25 -c=2 -z=2 -Tperf=60)
add_physim_test(4g dlsim basic.test5 "10 MHz, R3-1.FDD (MCS 15), EVA5, 6.7dB (30%)" -m=15 -g=G -s=1.4 -w=1.0 -f=.2 -n=1500 -B=50 -c=2 -z=2 -Tperf=25)
add_physim_test(4g dlsim basic.test6 "5 MHz, R3-1.FDD (MCS 15), ETU70, 1.4 dB (30%)" -m=15 -g=G -s=1.4 -w=1.0 -f=.2 -n=1500 -B=25 -c=2 -z=2 -Tperf=25)
add_physim_test(4g dlsim basic.test7 "5 MHz, R6.FDD (MCS 25), EVA5, 17.4 dB (70%)" -m=25 -g=F -s=17.4 -w=1.0 -f=.2 -n=1500 -B=25 -c=3 -z=2 -Tperf=60)
add_physim_test(4g dlsim basic.test8 "5 MHz, R6-1.FDD (MCS 24,18 PRB), EVA5, 17.5dB (70%)" -m=25 -g=F -s=17.5 -w=1.0 -f=.2 -n=1500 -B=25 -c=3 -z=2 -r=1022 -Tperf=60)
add_physim_test(4g dlsim basic.test9 "10 MHz, R7.FDD (MCS 25), EVA5, 17.7dB (70%)" -m=26 -g=F -s=17.7 -w=1.0 -f=.2 -n=1500 -B=50 -c=2 -z=2 -Tperf=60)
add_physim_test(4g dlsim 100rb.tm2.test1 "20 MHz, R.9 FDD (MCS 26), EVA5, 17.6dB (70%)" -m=26 -g=F -s=17.6 -w=1.0 -f=.2 -n=1500 -B=100 -c=2 -z=2 -Tperf=60)
add_physim_test(4g dlsim 100rb.tm2.test2 "20 MHz, R.9-2 FDD (MCS 26, 17PRB), EVA5, 17.3dB (70%)" -m=26 -g=F -s=17.3 -w=1.0 -f=.2 -n=1500 -B=100 -c=2 -z=2 -r=1600 -Tperf=60)
add_physim_test(4g dlsim 100rb.tm2.test3 "20 MHz, R.9-1 FDD (MCS 26, 83 PRB), EVA5, 16.6dB (70%)" -m=26 -g=F -s=16.6 -w=1.0 -f=.2 -n=1500 -B=100 -c=2 -z=2 -r=1899 -Tperf=60)
add_physim_test(4g dlsim 100rb.tm2.test4 "10 MHz, R.11 FDD (MCS 14), EVA5, 6.8 dB (70%)" -m=14 -g=F -s=6.8 -w=1.0 -f=.2 -n=1500 -B=50 -c=2 -x=2 -y=2 -z=2 -Tperf=60)
add_physim_test(4g dlsim 100rb.tm2.test5 "5 MHz, R.11-2 FDD (MCS 13), EVA5, 5.9 dB (70%)" -m=13 -g=F -s=5.9 -w=1.0 -f=.2 -n=1500 -B=25 -c=3 -x=2 -y=2 -z=2 -Tperf=60)

####################################################################################
######                           ulsim unit test                              ######
####################################################################################

add_physim_test(4g ulsim test1 "5 MHz, FDD (MCS 5), AWGN, 6dB" -BnbRBs=25 -mcs=5 -yN_rx=1 -gchannel=N -xTransmission=1 -snr=6 -wsnrInterrupt=1.0 -e_snr_step=.1 -P -nb_frame=1500 -Tperf=70)
add_physim_test(4g ulsim test2 "5 MHz, FDD (MCS 16), AWGN , 12dB (70%)" -BnbRBs=25 -mcs=16 -yN_rx=1 -gchannel=N -xTransmission=1 -snr=12 -wsnrInterrupt=1.0 -e_snr_step=.1 -P -nb_frame=1500 -Tperf=70)
add_physim_test(4g ulsim test3 "10 MHz, R3.FDD (MCS 5), AWGN, 6dB (70%)" -BnbRBs=50 -mcs=5 -yN_rx=1 -gchannel=N -xTransmission=1 -snr=6 -wsnrInterrupt=1.0 -e_snr_step=.1 -P -nb_frame=1500 -Tperf=70)
add_physim_test(4g ulsim test4 "10 MHz, R3-1.FDD (MCS 16), AWGN, 12dB (70%)" -BnbRBs=50 -mcs=16 -yN_rx=1 -gchannel=N -xTransmission=1 -snr=12 -wsnrInterrupt=1.0 -e_snr_step=.1 -P -nb_frame=1500 -Tperf=70)
add_physim_test(4g ulsim test5 "20 MHz, FDD (MCS 5), AWGN, 6dB (70%)" -BnbRBs=100 -mcs=5 -yN_rx=1 -gchannel=N -xTransmission=1 -snr=6 -wsnrInterrupt=1.0 -e_snr_step=.1 -P -nb_frame=1500 -Tperf=70)
add_physim_test(4g ulsim test6 "20 MHz, FDD (MCS 16), AWGN, 12 dB (70%)" -BnbRBs=100 -mcs=16 -yN_rx=1 -gchannel=N -xTransmission=1 -snr=12 -wsnrInterrupt=1.0 -e_snr_step=.1 -P -nb_frame=1500 -Tperf=70)

####################################################################################
######                           ldpctest unit test                           ######
####################################################################################

add_physim_test(5g ldpctest test1 "block length = 3872, BG1" -l3872 -s10 -n300)
add_physim_test(5g ldpctest test2 "block length = 4224, BG1" -l4224 -s10 -n300)
add_physim_test(5g ldpctest test3 "block length = 4576, BG1" -l4576 -s10 -n300)
add_physim_test(5g ldpctest test4 "block length = 4928, BG1" -l4928 -s10 -n300)
add_physim_test(5g ldpctest test5 "block length = 5280, BG1" -l5280 -s10 -n300)
add_physim_test(5g ldpctest test6 "block length = 5632, BG1" -l5632 -s10 -n300)
add_physim_test(5g ldpctest test7 "block length = 6336, BG1" -l6336 -s10 -n300)
add_physim_test(5g ldpctest test8 "block length = 7040, BG1" -l7040 -s10 -n300)
add_physim_test(5g ldpctest test9 "block length = 7744, BG1" -l7744 -s10 -n300)
add_physim_test(5g ldpctest test10 "block length = 8448, BG1" -l8448 -s10 -n300)
add_physim_test(5g ldpctest test11 "block length = 1, BG2" -l1 -s10 -n300)
add_physim_test(5g ldpctest test12 "block length = 100, BG2" -l100 -s10 -n300)
add_physim_test(5g ldpctest test13 "block length = 193, BG2" -l193 -s10 -n300)
add_physim_test(5g ldpctest test14 "block length = 500, BG2" -l500 -s10 -n300)
add_physim_test(5g ldpctest test15 "block length = 561, BG2" -l561 -s10 -n300)
add_physim_test(5g ldpctest test16 "block length = 600, BG2" -l600 -s10 -n300)
add_physim_test(5g ldpctest test17 "block length = 641, BG2" -l641 -s10 -n300)
add_physim_test(5g ldpctest test18 "block length = 2000, BG1" -l2000 -s10 -n300)
add_physim_test(5g ldpctest test19 "block length = 3000, BG1" -l3000 -s10 -n300)
add_physim_test(5g ldpctest test20 "block length = 3840, BG1" -l3840 -s10 -n300)

####################################################################################
######                           polartest unit test                          ######
####################################################################################

add_physim_test(5g polartest test1 "PBCH polar test" -q -s-10 -f0)
add_physim_test(5g polartest test2 "DCI polar test" -q -s-10 -f0 -m1)
add_physim_test(5g polartest test3 "UCI polar test,6-bit CRC" -q -s-2 -f2 -m2 -k12)
add_physim_test(5g polartest test4 "UCI polar test,11-bit CRC" -q -s-2 -f2 -m2 -k20)

####################################################################################
######                           nr_pbchsim unit test                         ######
####################################################################################

add_physim_test(5g nr_pbchsim 106rb.test1 "PBCH-only, 106 PRB, NID 2" -s-11 -S-8 -n30 -R106 -N2)
add_physim_test(5g nr_pbchsim 106rb.test2 "PBCH and synchronization, 106PBR" -s-11 -S-8 -n30 -o8000 -I -R106)
add_physim_test(5g nr_pbchsim 106rb.test3 "PBCH and synchronization, 106PBR, SSB SC OFFSET 6" -s-11 -S-8 -n30 -R106 -c6)
add_physim_test(5g nr_pbchsim 217rb.test1 "PBCH-only, 217 PRB" -s-10 -S-8 -n30 -R217)
add_physim_test(5g nr_pbchsim 217rb.test2 "PBCH and synchronization, 217 RPB" -s-10 -S-8 -n30 -o8000 -I -R217)
add_physim_test(5g nr_pbchsim 273rb.test1 "PBCH-only, 273 PRB" -s-10 -S-8 -n30 -R273)
add_physim_test(5g nr_pbchsim 273rb.test2 "PBCH and synchronization, 273 PRB" -s-10 -S-8 -n30 -o8000 -I -R273)
add_physim_test(5g nr_pbchsim otherSCS.test1 "PBCH-only, 15Khz, 5Mhz" -s-10 -S-8 -n30 -m0 -R25)
add_physim_test(5g nr_pbchsim otherSCS.test2 "PBCH-only, 120Khz, 50Mhz" -s-10 -S-8 -n30 -m3 -R32)

####################################################################################
######                           nr_dlsim unit test                           ######
####################################################################################

add_physim_test(5g nr_dlsim basic.test1 "106 PRB" -n300 -R106 -b106 -s5)
add_physim_test(5g nr_dlsim basic.test2 "217 PRB" -n300 -R217 -b217 -s5)
add_physim_test(5g nr_dlsim basic.test3 "273 PRB" -n300 -R273 -b273 -s5)
add_physim_test(5g nr_dlsim basic.test4 "HARQ test 25% TP 4 rounds" -n300 -s1 -S2 -t25)
add_physim_test(5g nr_dlsim basic.test5 "HARQ test 33% TP 3 rounds" -n300 -s1 -S2 -t33)
add_physim_test(5g nr_dlsim basic.test6 "HARQ test 50% TP 2 rounds" -n300 -s5 -S7 -t50)
add_physim_test(5g nr_dlsim basic.test7 "25 PRBs, 15 kHz SCS" -n300 -m0 -e0 -R25 -b25 -i 2 1 0)
add_physim_test(5g nr_dlsim basic.test8 "32 PRBs, 120 kHz SCS" -n300 -s5 -m3 -R32 -b32)
add_physim_test(5g nr_dlsim offset.test1 "106 PRB 25 PDSCH-Offset" -n300 -R106 -a25 -s5)
add_physim_test(5g nr_dlsim offset.test2 "106 PRB 51 PDSCH-Offset" -n300 -R106 -a51 -s5)
add_physim_test(5g nr_dlsim offset.test3 "217 PRB 100 PDSCH-PRBs" -n300 -R217 -b100 -s5)
add_physim_test(5g nr_dlsim offset.test4 "217 PRB 80 PDSCH-Offset" -n300 -R217 -a80 -s5)
add_physim_test(5g nr_dlsim offset.test5 "217 PRB 100 PDSCH-PRBs 110 PDSCH-Offset" -n300 -R217 -a110 -s5 -b100)
add_physim_test(5g nr_dlsim mcs.mimo.test1 "106 PRBs 50 PDSCH-PRBs MCS Index 27" -n300 -e27 -s30)
add_physim_test(5g nr_dlsim mcs.mimo.test2 "106 PRBs 50 PDSCH-PRBs MCS Index 16" -n300 -e16 -s11 -S13)
add_physim_test(5g nr_dlsim mcs.mimo.test3 "106 MCS-TABLE 256 QAM MCS Index 26" -n300 -q1 -e26 -s30)
add_physim_test(5g nr_dlsim mcs.mimo.test4 "MCS 0, low SNR performance" -n300 -e0 -t95 -S-1.0 -i 2 1 0)
add_physim_test(5g nr_dlsim mcs.mimo.test5 "4x4 MIMO, 1 Layer" -n30 -s20 -U 3 0 0 2 -gA -x1 -y4 -z4)
add_physim_test(5g nr_dlsim mcs.mimo.test6 "4x4 MIMO, 2 Layers" -n30 -s20 -U 3 0 0 2 -gA -x2 -y4 -z4)
add_physim_test(5g nr_dlsim mcs.mimo.test7 "4x4 MIMO, 4 Layers" -n30 -s20 -U 3 0 0 2 -x4 -y4 -z4)
add_physim_test(5g nr_dlsim dmrs.ptrs.test1 "3 PTRS, 8 Interpolated Symbols" -n300 -s5 -T 2 2 2)
add_physim_test(5g nr_dlsim dmrs.ptrs.test2 "6 PTRS, 5 Interpolated Symbols" -n300 -s5 -T 2 1 2)
add_physim_test(5g nr_dlsim dmrs.ptrs.test3 "11 PTRS, 0 Interpolated Symbols" -n300 -s5 -T 2 0 4)
add_physim_test(5g nr_dlsim dmrs.ptrs.test4 "Mapping type A, 2 DMRS Symbols" -n300 -s5 -S7 -U 2 0 1)
add_physim_test(5g nr_dlsim dmrs.ptrs.test5 "Mapping type A, 3 DMRS Symbols" -n300 -s5 -S7 -U 2 0 2)
add_physim_test(5g nr_dlsim dmrs.ptrs.test6 "Mapping type B, 4 DMRS Symbols" -n300 -s5 -S7 -U 2 1 3)

####################################################################################
######                           nr_dlschsim unit test                        ######
####################################################################################

add_physim_test(5g nr_dlschsim test1 "106 PRBs" -R 106 -m9 -s13 -n300)
add_physim_test(5g nr_dlschsim test2 "217 PRBs" -R 217 -m15 -s15 -n300)
add_physim_test(5g nr_dlschsim test3 "273 PRBs" -R 273 -m19 -s20 -n300)

####################################################################################
######                           smallblocktest unit test                     ######
####################################################################################

add_physim_test(5g smallblocktest test1 "3 bits" -l 3 -s -4 -d 1 -i 10000)
add_physim_test(5g smallblocktest test2 "6 bits" -l 6 -s -4 -d 1 -i 10000)
add_physim_test(5g smallblocktest test3 "7 bits" -l 7 -s -4 -d 1 -i 10000)
add_physim_test(5g smallblocktest test4 "11 bits" -l 11 -s -4 -d 1 -i 10000)

####################################################################################
######                           nr_ulschsim unit test                        ######
####################################################################################

add_physim_test(5g nr_ulschsim test1 "106 PRBs" -R 106 -m9 -s13 -n300)
add_physim_test(5g nr_ulschsim test2 "217 PRBs" -R 217 -m15 -s15 -n300)
add_physim_test(5g nr_ulschsim test3 "273 PRBs" -R 273 -m19 -s20 -n300)
add_physim_test(5g nr_ulschsim test4 "106 PRBs 4-layer MIMO" -R 106 -m9 -s13 -n300 -y4 -z4 -W4)

####################################################################################
######                           nr_pucchsim unit test                        ######
####################################################################################

add_physim_test(5g nr_pucchsim test1 "Format 0 1-bit ACK miss 106 PRB" -R 106 -i 1 -P 0 -b 1 -s-2 -n3000)
add_physim_test(5g nr_pucchsim test2 "Format 0 2-bit ACK miss 106 PRB" -R 106 -i 1 -P 0 -b 2 -s-2 -n3000)
add_physim_test(5g nr_pucchsim test3 "Format 0 2-bit ACK miss, 1-bit SR 106 PRB" -R 106 -i 1 -P 0 -b 2 -s-2 -c -n3000)
add_physim_test(5g nr_pucchsim test4 "Format 2 3-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 3 -s0 -n3000)
add_physim_test(5g nr_pucchsim test5 "Format 2 4-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 4 -s0 -n3000)
add_physim_test(5g nr_pucchsim test6 "Format 2 5-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 5 -s1 -n3000)
add_physim_test(5g nr_pucchsim test7 "Format 2 6-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 6 -s2 -n3000)
add_physim_test(5g nr_pucchsim test8 "Format 2 7-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 7 -s3 -n3000)
add_physim_test(5g nr_pucchsim test9 "Format 2 8-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 8 -s4 -n3000)
add_physim_test(5g nr_pucchsim test10 "Format 2 9-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 9 -s5 -n3000)
add_physim_test(5g nr_pucchsim test11 "Format 2 10-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 10 -s6 -n3000)
add_physim_test(5g nr_pucchsim test12 "Format 2 11-bit 2/106 PRB" -R 106 -i 1 -P 2 -b 11 -s6 -n3000)
add_physim_test(5g nr_pucchsim test13 "Format 2 12-bit 4/106 PRB" -R 106 -i 1 -P 2 -q4 -b 12 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test14 "Format 2 19-bit 4/106 PRB" -R 106 -i 1 -P 2 -q4 -b 19 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test15 "Format 2 12-bit 8/106 PRB" -R 106 -i 1 -P 2 -q8 -b 12 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test16 "Format 2 19-bit 8/106 PRB" -R 106 -i 1 -P 2 -q8 -b 19 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test17 "Format 2 32-bit 8/106 PRB" -R 106 -i 1 -P 2 -q8 -b 32 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test18 "Format 2 32-bit 16/106 PRB" -R 106 -i 1 -P 2 -q16 -b 32 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test19 "Format 2 64-bit 16/106 PRB" -R 106 -i 1 -P 2 -q16 -b 64 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test20 "Format 0 1-bit Ack miss 273 PRB" -R 273 -z8 -i 1 -P 0 -b 1 -s-2 -n3000)
add_physim_test(5g nr_pucchsim test21 "Format 0 2-bit Ack miss 273 PRB" -R 273 -z8 -i 1 -P 0 -b 2 -s-2 -n3000)
add_physim_test(5g nr_pucchsim test22 "Format 0 2-bit Ack miss+SR 273 PRB" -R 273 -z8 -i 1 -P 0 -b 2 -s-2 -c -n3000)
add_physim_test(5g nr_pucchsim test23 "Format 2 4-bit 2/273 PRB" -R 273 -z8 -i 1 -P 2 -b 4 -s-8 -S0 -n3000)
add_physim_test(5g nr_pucchsim test24 "Format 2 7-bit 2/273 PRB" -R 273 -z8 -i 1 -P 2 -b 7 -s3 -n3000)
add_physim_test(5g nr_pucchsim test25 "Format 2 11-bit 2/273 PRB" -R 273 -z8 -i 1 -P 2 -b 11 -s6 -n3000)
add_physim_test(5g nr_pucchsim test26 "Format 2 12-bit 8/273 PRB" -R 273 -z8 -i 1 -P 2 -q8 -b 12 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test27 "Format 2 19-bit 8/273 PRB" -R 273 -z8 -i 1 -P 2 -q8 -b 19 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test28 "Format 2 64-bit 16/273 PRB" -R 273 -z8 -i 1 -P 2 -q16 -b 64 -s-3 -n3000)
add_physim_test(5g nr_pucchsim test29 "Format 2 64-bit 16/273 PRB Delay 2us" -R 273 -z8 -i 1 -P 2 -q16 -b 64 -s0 -S7 -d 2 -n3000)

####################################################################################
######                           nr_ulsim unit test                           ######
####################################################################################

add_physim_test(5g nr_ulsim misc.test1 "MCS 9 106 PRBs" -n300 -m9 -r106 -s5)
add_physim_test(5g nr_ulsim misc.test2 "MCS 16 50 PRBs" -n300 -m16 -s10)
add_physim_test(5g nr_ulsim misc.test3 "MCS 28 50 PRBs" -n300 -m28 -s20)
add_physim_test(5g nr_ulsim misc.test4 "MCS 27 50 PRBs 256QAM" -n300 -m27 -s25 -q1)
add_physim_test(5g nr_ulsim misc.test5 "MCS 9 217 PRBs" -n300 -m9 -R217 -r217 -s5)
add_physim_test(5g nr_ulsim misc.test6 "MCS 9 273 PRBs" -n300 -m9 -R273 -r273 -s5)
add_physim_test(5g nr_ulsim misc.test7 "PUSCH Type A, 2 DMRS Symbols" -n300 -s5 -U 0,1,1,1)
add_physim_test(5g nr_ulsim misc.test8 "PUSCH Type A, 3 DMRS, 4 PTRS, 5 Interpolated Symbols" -n300 -s5 -T 1,2 -U 0,2,1,1)
add_physim_test(5g nr_ulsim misc.test9 "PUSCH Type B, 3 DMRS, 2 PTRS, 7 Interpolated Symbols" -n300 -s5 -T 2,2 -U 1,2,1,1)
add_physim_test(5g nr_ulsim misc.test10 "PUSCH Type B, 3 DMRS, 2 PTRS, 3 Interpolated Symbols" -n300 -s5 -a4 -b8 -T 1,2 -U 1,3,1,1)
add_physim_test(5g nr_ulsim misc.test11 "25 PRBs, 15 kHz SCS" -n300 -u0 -m0 -R25 -r25 -i 1,0)
add_physim_test(5g nr_ulsim misc.test12 "32 PRBs, 120 kHz SCS" -n300 -s5 -r32 -R32 -u3)
add_physim_test(5g nr_ulsim misc.test13 "MCS 0, low SNR performance" -n300 -m0 -S -0.6 -i 1,0)
add_physim_test(5g nr_ulsim misc.test14 "MCS 28, 106 PRBs, Time shift 8" -n300 -m28 -R106 -r106 -t90 -s24 -S24 -d 8)
add_physim_test(5g nr_ulsim misc.test15 "SRS, SNR 40 dB" -n300 -s40 -E 1)
add_physim_test(5g nr_ulsim sc-fdma.test1 "SC-FDMA, 50 PRBs" -n300 -s5 -Z)
add_physim_test(5g nr_ulsim sc-fdma.test2 "SC-FDMA, 75 PRBs" -n300 -s5 -Z -r75)
add_physim_test(5g nr_ulsim sc-fdma.test3 "SC-FDMA, 216 PRBs" -n150 -s5 -Z -r216 -R217)
add_physim_test(5g nr_ulsim sc-fdma.test4 "SC-FDMA, 273 PRBs" -n150 -s5 -Z -r270 -R273)
add_physim_test(5g nr_ulsim sc-fdma.test5 "SC-FDMA, 3 DMRS" -n300 -s5 -Z -U 0,2,1,2)
add_physim_test(5g nr_ulsim mimo.test1 "MCS 19 50 PRBs 2 RX_Antenna" -n300 -m19 -s10 -S15 -z2)
add_physim_test(5g nr_ulsim mimo.test2 "MCS 9 106 PRBs MIMO 2 layers" -n300 -m9 -r106 -s8 -W2 -y2 -z2)
add_physim_test(5g nr_ulsim mimo.test3 "MCS 10 106 PRBs MIMO 2 layers" -n300 -m10 -r106 -s12 -W2 -y2 -z2)
add_physim_test(5g nr_ulsim mimo.test4 "MCS 19 106 PRBs MIMO 2 layers" -n300 -m19 -r106 -s22 -W2 -y2 -z2)
add_physim_test(5g nr_ulsim mimo.test5 "MCS 9 106 PRBs MIMO 4 layers" -n300 -m9 -r106 -s10 -W4 -y4 -z4)
add_physim_test(5g nr_ulsim 3gpp.test1 "3GPP G-FR1-A5-13, PUSCH Type A, 40 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R106 -r106 -U 0,1,1,2 -z2 -s12.4 -S12.4)
add_physim_test(5g nr_ulsim 3gpp.test2 "3GPP G-FR1-A5-13, PUSCH Type A, 40 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R106 -r106 -U 0,1,1,2 -z4 -s8.5 -S8.5)
add_physim_test(5g nr_ulsim 3gpp.test3 "3GPP G-FR1-A5-13, PUSCH Type A, 40 MHz BW, 30 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R106 -r106 -U 0,1,1,2 -z8 -s5.4 -S5.4)
add_physim_test(5g nr_ulsim 3gpp.test4 "3GPP G-FR1-A5-8, PUSCH Type B, 5 MHz BW, 15 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R25 -r25 -U 1,1,1,2 -z2 -s12.5 -S12.5)
add_physim_test(5g nr_ulsim 3gpp.test5 "3GPP G-FR1-A5-8, PUSCH Type B, 5 MHz BW, 15 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R25 -r25 -U 1,1,1,2 -z4 -s8.9 -S8.9)
add_physim_test(5g nr_ulsim 3gpp.test6 "3GPP G-FR1-A5-8, PUSCH Type B, 5 MHz BW, 15 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R25 -r25 -U 1,1,1,2 -z8 -s5.7 -S5.7)
add_physim_test(5g nr_ulsim 3gpp.test7 "3GPP G-FR1-A5-9, PUSCH Type B, 10 MHz BW, 15 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R52 -r52 -U 1,1,1,2 -z2 -s12.6 -S12.6)
add_physim_test(5g nr_ulsim 3gpp.test8 "3GPP G-FR1-A5-9, PUSCH Type B, 10 MHz BW, 15 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R52 -r52 -U 1,1,1,2 -z4 -s8.9 -S8.9)
add_physim_test(5g nr_ulsim 3gpp.test9 "3GPP G-FR1-A5-9, PUSCH Type B, 10 MHz BW, 15 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R52 -r52 -U 1,1,1,2 -z8 -s5.8 -S5.8)
add_physim_test(5g nr_ulsim 3gpp.test10 "3GPP G-FR1-A5-10, PUSCH Type B, 20 MHz BW, 15 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R106 -r106 -U 1,1,1,2 -z2 -s12.3 -S12.3)
add_physim_test(5g nr_ulsim 3gpp.test11 "3GPP G-FR1-A5-10, PUSCH Type B, 20 MHz BW, 15 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R106 -r106 -U 1,1,1,2 -z4 -s8.8 -S8.8)
add_physim_test(5g nr_ulsim 3gpp.test12 "3GPP G-FR1-A5-10, PUSCH Type B, 20 MHz BW, 15 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 0 -m20 -R106 -r106 -U 1,1,1,2 -z8 -s5.7 -S5.7)
add_physim_test(5g nr_ulsim 3gpp.test13 "3GPP G-FR1-A5-11, PUSCH Type B, 10 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R24 -r24 -U 1,1,1,2 -z2 -s12.5 -S12.5)
add_physim_test(5g nr_ulsim 3gpp.test14 "3GPP G-FR1-A5-11, PUSCH Type B, 10 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R24 -r24 -U 1,1,1,2 -z4 -s8.6 -S8.6)
add_physim_test(5g nr_ulsim 3gpp.test15 "3GPP G-FR1-A5-11, PUSCH Type B, 10 MHz BW, 30 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R24 -r24 -U 1,1,1,2 -z8 -s5.6 -S5.6)
add_physim_test(5g nr_ulsim 3gpp.test16 "3GPP G-FR1-A5-12, PUSCH Type B, 20 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R51 -r51 -U 1,1,1,2 -z2 -s12.5 -S12.5)
add_physim_test(5g nr_ulsim 3gpp.test17 "3GPP G-FR1-A5-12, PUSCH Type B, 20 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R51 -r51 -U 1,1,1,2 -z4 -s8.6 -S8.6)
add_physim_test(5g nr_ulsim 3gpp.test18 "3GPP G-FR1-A5-12, PUSCH Type B, 20 MHz BW, 30 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R51 -r51 -U 1,1,1,2 -z8 -s5.6 -S5.6)
add_physim_test(5g nr_ulsim 3gpp.test19 "3GPP G-FR1-A5-13, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R106 -r106 -U 1,1,1,2 -z2 -s12.5 -S12.5)
add_physim_test(5g nr_ulsim 3gpp.test20 "3GPP G-FR1-A5-13, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R106 -r106 -U 1,1,1,2 -z4 -s8.7 -S8.7)
add_physim_test(5g nr_ulsim 3gpp.test21 "3GPP G-FR1-A5-13, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R106 -r106 -U 1,1,1,2 -z8 -s5.5 -S5.5)
add_physim_test(5g nr_ulsim 3gpp.test22 "3GPP G-FR1-A5-14, PUSCH Type B, 100 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R273 -r273 -U 1,1,1,2 -z2 -s13.1 -S13.1)
add_physim_test(5g nr_ulsim 3gpp.test23 "3GPP G-FR1-A5-14, PUSCH Type B, 100 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test" -n300 -b14 -I7 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R273 -r273 -U 1,1,1,2 -z4 -s9.2 -S9.2)
add_physim_test(5g nr_ulsim 3gpp.test24 "3GPP G-FR1-A5-14, PUSCH Type B, 100 MHz BW, 30 kHz SCS, 8 RX Antennas Requirements Test" -n300 -b14 -I8 -i 0,1 -g A,l,10 -t70 -u 1 -m20 -R273 -r273 -U 1,1,1,2 -z8 -s5.9 -S5.9)
add_physim_test(5g nr_ulsim 3gpp.test25 "3GPP G-FR1-A3-27, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test, 2 layers" -n300 -b14 -I15 -i 0,1 -g B,l -t70 -u 1 -m2 -R106 -r106 -U 1,1,1,2 -W2 -y2 -z2 -s1.7 -S1.7)
add_physim_test(5g nr_ulsim 3gpp.test26 "3GPP G-FR1-A3-27, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test, 2 layers" -n300 -b14 -I15 -i 0,1 -g B,l -t70 -u 1 -m2 -R106 -r106 -U 1,1,1,2 -W2 -y2 -z4 -s-2.1 -S-2.1)
add_physim_test(5g nr_ulsim 3gpp.test27 "3GPP G-FR1-A4-27, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 2 RX Antennas Requirements Test, 2 layers" -n300 -b14 -I15 -i 0,1 -g C,l -t70 -u 1 -m16 -R106 -r106 -U 1,1,1,2 -W2 -y2 -z2 -s18.7 -S18.7)
add_physim_test(5g nr_ulsim 3gpp.test28 "3GPP G-FR1-A4-27, PUSCH Type B, 40 MHz BW, 30 kHz SCS, 4 RX Antennas Requirements Test, 2 layers" -n300 -b14 -I15 -i 0,1 -g C,l -t70 -u 1 -m16 -R106 -r106 -U 1,1,1,2 -W2 -y2 -z4 -s11.2 -S11.2)

####################################################################################
######                           nr_prachsim unit test                        ######
####################################################################################

add_physim_test(5g nr_prachsim test1 "30kHz SCS, 106 PRBs, Prach format A2" -a -s -30 -n 300 -p 63 -R 106)
add_physim_test(5g nr_prachsim test2 "30kHz SCS, 217 PRBs, Prach format A2" -a -s -30 -n 300 -p 63 -R 217)
add_physim_test(5g nr_prachsim test3 "30kHz SCS, 273 PRBs, Prach format A2" -a -s -30 -n 300 -p 63 -R 273)
add_physim_test(5g nr_prachsim test4 "30kHz SCS, 106 PRBs, Prach format 0" -a -s -30 -n 300 -p 63 -R 106 -c 4)
add_physim_test(5g nr_prachsim test5 "120kHz SCS, 32 PRBs, Prach format A2" -a -s -30 -n 300 -p 32 -R 32 -m 3 -c52)
add_physim_test(5g nr_prachsim test6 "120kHz SCS, 66 PRBs, Prach format A2" -a -s -30 -n 300 -p 32 -R 66 -m 3 -c52)
add_physim_test(5g nr_prachsim test7 "120kHz SCS, 66 PRBs, High Speed Enabled" -a -s -30 -n 300 -R 66 -m 3 -c52 -H)
add_physim_test(5g nr_prachsim test8 "15kHz SCS, 25 PRBs" -a -s -30 -n 300 -p 99 -R 25 -m 0)

####################################################################################
######                           nr_psbchsim unit test                        ######
####################################################################################

add_physim_test(5g nr_psbchsim test1 "SLSS Search" -I)
add_physim_test(5g nr_psbchsim test2 "PSBCH TxRx" -n 30)
