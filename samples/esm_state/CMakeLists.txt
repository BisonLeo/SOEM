add_executable(esm_state esmstate.c)
target_link_libraries(esm_state soem)
install(TARGETS esm_state DESTINATION bin)
