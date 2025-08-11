cmake_minimum_required(VERSION 3.5)
project(highway-pursuit-gym-server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(shared_headers INTERFACE)
target_include_directories(shared_headers
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/highway-pursuit-server/shared
)

add_subdirectory(minhook)
add_subdirectory(highway-pursuit-launcher)
add_subdirectory(highway-pursuit-server)