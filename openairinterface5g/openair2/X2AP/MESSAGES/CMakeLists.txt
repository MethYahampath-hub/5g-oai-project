set(X2AP_VERSION 15 6 0)
make_version(X2AP_cc ${X2AP_VERSION})
#add_definitions(-DX2AP_VERSION=${X2AP_cc})
string(REPLACE ";" "." X2AP_RELEASE "${X2AP_VERSION}")

if(X2AP_RELEASE VERSION_EQUAL "15.6.0")
  include(ASN1/R15/x2ap-15.6.0.cmake)
else()
  message(FATAL_ERROR "unknown X2AP_RELEASE ${X2AP_RELEASE}")
endif()

run_asn1c(${CMAKE_CURRENT_SOURCE_DIR}/${X2AP_GRAMMAR}
          "X2AP_"
          OUTPUT ${x2ap_source} ${x2ap_headers}
          OPTIONS -pdu=all -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps
          COMMENT "X2AP source files"
)

add_library(asn1_x2ap ${x2ap_source})
target_include_directories(asn1_x2ap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_x2ap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)

