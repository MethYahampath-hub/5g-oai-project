##########################################################

# LDPC offload library - AMD T2 Accelerator Card
##########################################################

add_boolean_option(ENABLE_LDPC_T2 OFF "Build support for LDPC Offload to T2 library" OFF)

if (ENABLE_LDPC_T2)

  pkg_check_modules(LIBDPDK_T2 REQUIRED libdpdk=20.11.9)

  find_library(PMD_T2 NAMES rte_baseband_accl_ldpc HINTS ${LIBDPDK_T2_LIBRARY_DIRS})
  if (NOT PMD_T2)
    message(FATAL_ERROR "could not find poll-mode driver for AccelerComm T2 LDPC Offload (rte_baseband_accl_ldpc.so)")
  endif()

  message(STATUS "T2 build: use ${PMD_T2}")
  add_library(ldpc_t2 MODULE nrLDPC_coding_t2.c)

  set_target_properties(ldpc_t2 PROPERTIES COMPILE_FLAGS "-DALLOW_EXPERIMENTAL_API")
  set_target_properties(ldpc_t2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  target_link_libraries(ldpc_t2 PRIVATE ldpc_gen_HEADERS ${LIBDPDK_T2_LDFLAGS} ${PMD_T2})

  add_dependencies(nr-softmodem ldpc_t2)
  add_dependencies(nr-uesoftmodem ldpc_t2)
  add_dependencies(nr_ulsim ldpc_t2)
  add_dependencies(nr_ulschsim ldpc_t2)
  add_dependencies(nr_dlsim ldpc_t2)
  add_dependencies(nr_dlschsim ldpc_t2)

endif()
