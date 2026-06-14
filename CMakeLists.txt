cmake_minimum_required(VERSION 3.16)

project(e84_cgm_aps_framework C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(e84_aps_framework
    firmware/src/aps_demo_state.c
    firmware/src/aps_log.c
    firmware/src/activity_engine.c
    firmware/src/cob_engine.c
    firmware/src/aps_physiology.c
    firmware/src/controller_openaps.c
    firmware/src/iob_engine.c
    firmware/src/metrics.c
    firmware/src/predictor_v2.c
    firmware/src/predictor_v2_generated.c
    firmware/src/safety_supervisor.c
    sim/audit_trace.c
    sim/replay_loader.c
    sim/scenario_runner.c
    sim/simulation_runner.c
)

target_include_directories(e84_aps_framework PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/firmware/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sim
)

target_link_libraries(e84_aps_framework PUBLIC m)

add_executable(e84_aps_host_tests
    tests/test_runner.c
)

target_link_libraries(e84_aps_host_tests PRIVATE e84_aps_framework)

add_executable(e84_aps_sim
    sim/main.c
)

target_link_libraries(e84_aps_sim PRIVATE e84_aps_framework)
