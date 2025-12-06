cmake_minimum_required(VERSION 3.20)
project(hft-exchange VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Performance flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-march=native -O3 -flto -fno-exceptions)
endif()

# Dependencies
find_package(Threads REQUIRED)

include(FetchContent)

# Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Google Benchmark
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(benchmark)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# uWebSockets (header-only simplified)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

enable_testing()

# Core library
add_library(hft_core STATIC
    src/common/logging.cpp
    src/common/config.cpp
    src/utils/time.cpp
    src/utils/mmap_file.cpp
    src/feed/feed_handler.cpp
    src/feed/parser.cpp
    src/engine/order_book.cpp
    src/engine/matching_engine.cpp
    src/persistence/wal.cpp
    src/persistence/snapshot.cpp
    src/publish/publisher.cpp
    src/api/http_server.cpp
    src/api/metrics.cpp
)

target_include_directories(hft_core PUBLIC src)
target_link_libraries(hft_core PUBLIC 
    Threads::Threads
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# Main executable
add_executable(hft_exchange src/main.cpp)
target_link_libraries(hft_exchange PRIVATE hft_core)

# Unit tests
add_executable(unit_tests
    tests/unit/order_book_test.cpp
    tests/unit/parser_test.cpp
    tests/unit/wal_test.cpp
)
target_link_libraries(unit_tests PRIVATE hft_core GTest::gtest_main)
add_test(NAME unit_tests COMMAND unit_tests)

# Fuzz tests
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_executable(fuzz_parser tests/fuzz/fuzz_parser.cpp)
    target_link_libraries(fuzz_parser PRIVATE hft_core)
    target_compile_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address)
    target_link_options(fuzz_parser PRIVATE -fsanitize=fuzzer,address)
endif()

# Property tests
add_executable(property_tests tests/property/reference_engine_test.cpp)
target_link_libraries(property_tests PRIVATE hft_core GTest::gtest_main)
add_test(NAME property_tests COMMAND property_tests)

# Benchmarks
add_executable(orderbook_bench bench/orderbook_bench.cpp)
target_link_libraries(orderbook_bench PRIVATE hft_core benchmark::benchmark_main)

add_executable(engine_bench bench/engine_bench.cpp)
target_link_libraries(engine_bench PRIVATE hft_core benchmark::benchmark_main)
