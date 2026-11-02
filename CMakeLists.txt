cmake_minimum_required(VERSION 3.20)
project(EdgeLink VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror=return-type
)

add_library(edgelink_core STATIC
    src/model/telemetry_record.cpp
    src/protocol/crc32.cpp
    src/protocol/protocol_parser.cpp
    src/sync/thread_pool.cpp
    src/network/socket.cpp
    src/network/epoll_event_loop.cpp
    src/network/connection.cpp
    src/network/tcp_server.cpp
    src/network/udp_server.cpp
    src/network/network_gateway.cpp
    src/service/metrics_registry.cpp
    src/service/telemetry_aggregator.cpp
    src/service/data_processor.cpp
    src/diagnostics/icmp_diagnostics.cpp
    src/simulator/device_sensor.cpp
    src/simulator/device_simulator.cpp
)

target_include_directories(edgelink_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(edgelink_core PUBLIC
    pthread
)

add_executable(edgelink_gateway
    src/main.cpp
)
target_link_libraries(edgelink_gateway PRIVATE
    edgelink_core
)

add_executable(edgelink_simulator
    src/simulator_main.cpp
)
target_link_libraries(edgelink_simulator PRIVATE
    edgelink_core
)

add_executable(edgelink_diagnostics
    src/diag_main.cpp
)
target_link_libraries(edgelink_diagnostics PRIVATE
    edgelink_core
)

option(BUILD_TESTING "Build unit and integration tests" ON)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(edgelink_tests
        tests/test_crc32.cpp
        tests/test_protocol_parser.cpp
        tests/test_thread_safe_queue.cpp
        tests/test_thread_pool.cpp
        tests/test_data_processor.cpp
        tests/test_tcp_udp_gateway.cpp
    )

    target_link_libraries(edgelink_tests PRIVATE
        edgelink_core
        gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(edgelink_tests)
endif()
