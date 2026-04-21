cmake_minimum_required(VERSION 3.16)
project(ha-portainer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(yaml-cpp REQUIRED)

# Fetch Google Test from GitHub at configure time
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(ha-portainer
    src/main.cpp
    src/ha/ha_client.cpp
    src/ws/boost_ws_connection.cpp
    src/dashboard/dashboard_updater.cpp
    src/config/config_parser.cpp
    src/portainer/slug.cpp
    src/portainer/boost_http_client.cpp
)

target_link_libraries(ha-portainer PRIVATE
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
)

# Tests
enable_testing()

add_executable(ha-portainer-tests
    tests/ha_client_test.cpp
    tests/dashboard_updater_test.cpp
    tests/config_parser_test.cpp
    tests/slug_test.cpp
    src/ha/ha_client.cpp
    src/dashboard/dashboard_updater.cpp
    src/config/config_parser.cpp
    src/portainer/slug.cpp
)

target_link_libraries(ha-portainer-tests PRIVATE
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    gtest_main
    gmock
)

# Registers the test executable with ctest
add_test(NAME ha-portainer-tests COMMAND ha-portainer-tests)
