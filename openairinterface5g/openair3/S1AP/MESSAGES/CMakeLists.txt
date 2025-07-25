set(S1AP_VERSION 15 6 0)
make_version(S1AP_cc ${S1AP_VERSION})
#add_definitions(-DS1AP_VERSION=${S1AP_cc})
string(REPLACE ";" "." S1AP_RELEASE "${S1AP_VERSION}")

if(S1AP_RELEASE VERSION_EQUAL "15.6.0")
  include(ASN1/R15/s1ap-15.6.0.cmake)
else()
  message(FATAL_ERROR "unknown S1AP_RELEASE ${S1AP_RELEASE}")
endif()

run_asn1c(${CMAKE_CURRENT_SOURCE_DIR}/${S1AP_GRAMMAR}
          "S1AP_"
          OUTPUT ${s1ap_source} ${s1ap_headers}
          OPTIONS -gen-APER -no-gen-BER -no-gen-JER -no-gen-OER -gen-UPER -fcompound-names -no-gen-example -fno-include-deps
          COMMENT "S1AP source files"
)

add_library(asn1_s1ap ${s1ap_source})
target_include_directories(asn1_s1ap PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(asn1_s1ap PRIVATE -DASN_DISABLE_OER_SUPPORT -w)
