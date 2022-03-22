cmake_minimum_required(VERSION 3.11)

project(cdbnpp-cli VERSION 1 DESCRIPTION "Conditions Database Command-Line Interface")

IF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
ELSE()
  message("Debug build.")
ENDIF()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(modules
	iomaker.cpp
	tpc.cpp
	svt.cpp
	bemc.cpp
	eemc.cpp
	analysis.cpp
	modules.cpp
)

target_include_directories(modules PRIVATE ${CMAKE_SOURCE_DIR}/../../contrib)
target_include_directories(modules PRIVATE ${CMAKE_SOURCE_DIR}/../../lib/include)
target_link_libraries(modules ${CMAKE_SOURCE_DIR}/../../lib/build/libcdbnpp.so )
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
  target_link_libraries(modules stdc++fs )
endif()
find_package( Threads )
target_link_libraries (modules ${CMAKE_THREAD_LIBS_INIT})