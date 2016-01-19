module_version(3.0)

# PORTHOLE Add websockets lib and zlib if on windows
if(OMEGA_OS_WIN)
    add_subdirectory(zlib)
endif(OMEGA_OS_WIN)
add_subdirectory(websockets)

# Set module name and source files here
add_library(porthole MODULE 
    PortholeService.h
    PortholeClient.h
    porthole.cpp
    PortholeService.cpp
    PortholeClient.cpp
    ServerThread.cpp
    ServerThread_Json.cpp
    ServerThread_Http.cpp
    ServerThread_Ws.cpp
    # A tiny JSON library
    vjson/block_allocator.cpp
    vjson/json.cpp)

# Set the module library dependencies here
target_link_libraries(porthole omega omegaToolkit websockets)

include_directories(websockets/include)
if (OMEGA_OS_WIN)
    include_directories(websockets/win32helpers)
    target_link_libraries(porthole zlib)
    include_directories(zlib)
endif()

declare_native_module(porthole)
