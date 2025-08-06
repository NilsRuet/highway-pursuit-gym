project(highway-pursuit-server)

add_library(highway-pursuit-server SHARED
    dllmain.cpp
    HighwayPursuitServer.cpp
    CommunicationManager.cpp
    HookManager.cpp
    HPLogger.cpp
    pch.cpp
    Injected/CheatService.cpp
    Injected/EpisodeService.cpp
    Injected/InputService.cpp
    Injected/RenderingService.cpp
    Injected/ScoreService.cpp
    Injected/UpdateService.cpp
    Injected/WindowService.cpp
)

set_target_properties(highway-pursuit-server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/output/bin/Debug"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/output/lib/Debug"
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/output/lib/Debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/output/bin/Release"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/output/lib/Release"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/output/lib/Release"
)

target_include_directories(highway-pursuit-server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Injected
    ${CMAKE_CURRENT_SOURCE_DIR}/Data
)

target_precompile_headers(highway-pursuit-server PRIVATE pch.h)

target_link_libraries(highway-pursuit-server
    PRIVATE
    minhook
    shared_headers
    user32
)