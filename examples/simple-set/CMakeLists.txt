cmake_minimum_required(VERSION 3.20)

project(simple-set LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(simple-set simple-set.cpp)

target_include_directories(simple-set PRIVATE ${CMAKE_SOURCE_DIR}/../../contrib)
target_include_directories(simple-set PRIVATE ${CMAKE_SOURCE_DIR}/../../lib/include)
target_link_libraries( simple-set ${CMAKE_SOURCE_DIR}/../../lib/build/libcdbnpp.so )
if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
	target_link_libraries( simple-set stdc++fs )
endif()
