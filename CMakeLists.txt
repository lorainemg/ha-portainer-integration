cmake_minimum_required(VERSION 3.16)
project(ha-portainer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(ha-portainer
    src/main.cpp
    src/ha_client.cpp
)

target_link_libraries(ha-portainer PRIVATE
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)
