# Windows needs some extra files
if(OMEGA_OS_WIN)
    
    set(websockets_SOURCES
			 ./src/libwebsockets.c
			 ./src/handshake.c
			 ./src/parsers.c
			 ./src/base64-decode.c
			 ./src/client-handshake.c
			 ./src/extension.c
			 ./src/extension-deflate-stream.c
			 ./src/md5.c
			 ./src/sha-1.c
			 ./win32helpers/getopt.c
			 ./win32helpers/getopt_long.c
			 ./win32helpers/gettimeofday.c
			 ./win32helpers/websock-w32.c
    )
    include_directories(./include ./win32helpers ../zlib)
else()
    set(websockets_SOURCES
			 ./src/libwebsockets.c
			 ./src/handshake.c
			 ./src/parsers.c
			 ./src/base64-decode.c
			 ./src/client-handshake.c
			 ./src/extension.c
			 ./src/extension-deflate-stream.c
			 ./src/md5.c
			 ./src/sha-1.c
    )
    include_directories(./include)
endif()

add_library(websockets STATIC ${websockets_SOURCES})
SET_TARGET_PROPERTIES(websockets PROPERTIES FOLDER 3rdparty)

# Windows version needs zlib to compile
if(OMEGA_OS_WIN)
    target_link_libraries(websockets zlib)
endif(OMEGA_OS_WIN)

IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  SET_TARGET_PROPERTIES(websockets PROPERTIES COMPILE_FLAGS "-fPIC")
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
