project(highway-pursuit-launcher)

add_executable(highway-pursuit-launcher
    HighwayPursuitLauncher.cpp
    Injection.cpp
)

set_target_properties(highway-pursuit-launcher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/output/bin/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/output/bin/Release"
)

target_include_directories(highway-pursuit-launcher
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(highway-pursuit-launcher
    PRIVATE
    shared_headers
    user32
)