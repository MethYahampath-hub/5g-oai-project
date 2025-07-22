add_library(vrtsim MODULE vrtsim.c noise_device.c)
set_target_properties(vrtsim PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_link_libraries(vrtsim PRIVATE SIMU shm_td_iq_channel actor)
add_dependencies(vrtsim generate_T)
