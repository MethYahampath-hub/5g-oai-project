add_library(fgs_5gmm_lib OBJECT
    fgmm_lib.c
    fgs_service_request.c
    fgmm_service_accept.c
    fgmm_service_reject.c
    RegistrationRequest.c
    RegistrationAccept.c
    FGSIdentityResponse.c
    FGSAuthenticationResponse.c
    FGSNASSecurityModeComplete.c
    RegistrationComplete.c
    FGSUplinkNasTransport.c
    FGSDeregistrationRequestUEOriginating.c
    fgmm_authentication_failure.c
)

target_link_libraries(fgs_5gmm_lib PUBLIC fgs_5gmm_ies_lib)

target_include_directories(fgs_5gmm_lib PRIVATE ../IES/ ../../)
