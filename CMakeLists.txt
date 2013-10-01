# Set the module name here
set(MODULE_NAME porthole)

# PORTHOLE Add websockets lib and zlib if on windows
if(OMEGA_OS_WIN)
	add_subdirectory(zlib)
endif(OMEGA_OS_WIN)
add_subdirectory(websockets)

# Set module name and source files here
add_library(${MODULE_NAME} SHARED 
	PortholeService.h
	PortholeGUI.h
	porthole.cpp
	PortholeService.cpp
	PortholeGUI.cpp
	# A tiny JSON library
	vjson/block_allocator.cpp
	vjson/json.cpp)

# Set the module library dependencies here
target_link_libraries(${MODULE_NAME}
	omega)

# Link against websockets library
target_link_libraries(${MODULE_NAME} websockets)
include_directories(websockets/include)
if (OMEGA_OS_WIN)
	include_directories(websockets/win32helpers)
	target_link_libraries(${MODULE_NAME} zlib)
	include_directories(zlib)
endif()

#------------------------------------------------------------------------------
# DO NOT MODIFY ANYTHING BELOW AFTER THIS LINE
set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "")
if(WIN32)
	set_target_properties(${MODULE_NAME} PROPERTIES FOLDER modules SUFFIX ".pyd")
endif()
