cmake_minimum_required(VERSION 3.20)

project(libcdbnpp VERSION 1 DESCRIPTION "Conditions DataBase API library for NPP")

IF(CMAKE_BUILD_TYPE MATCHES Release)
  message("Release build.")
ELSE()
  message("Debug build.")
ENDIF()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cdbnpp SHARED
	src/http_curl_holder.cpp
	src/http_response.cpp
	src/http_client.cpp
	src/payload.cpp
	src/payload_adapter_memory.cpp
	src/payload_adapter_file.cpp
	src/payload_adapter_db.cpp
	src/payload_adapter_http.cpp
	src/service.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	if (CMAKE_BUILD_TYPE MATCHES Release)
		target_compile_options(cdbnpp PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -O2 -g -Wno-dollar-in-identifier-extension)
	else()
		target_compile_options(cdbnpp PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g -Wno-unused-parameter -Wno-dollar-in-identifier-extension)
	endif()
  target_link_libraries( cdbnpp stdc++ )
  target_link_libraries( cdbnpp stdc++fs )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	if (CMAKE_BUILD_TYPE MATCHES Release)
		target_compile_options(cdbnpp PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -O2 -g -Wno-maybe-uninitialized)
	else()
		target_compile_options(cdbnpp PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -fsanitize=address -g -Wno-unused-parameter -Wno-maybe-uninitialized)
		target_link_libraries( cdbnpp asan)
	endif()
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
		target_link_libraries( cdbnpp stdc++fs )
	endif()
endif()

target_link_libraries( cdbnpp soci_core )

find_package( CURL REQUIRED )
target_include_directories(cdbnpp PRIVATE "${CURL_INCLUDE_DIR}")
target_link_libraries( cdbnpp ${CURL_LIBRARIES} )
target_include_directories(cdbnpp PRIVATE ${CMAKE_SOURCE_DIR}/../contrib)

set_target_properties(cdbnpp PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(cdbnpp PROPERTIES PUBLIC_HEADER include/cdbnpp.h)
target_include_directories(cdbnpp PRIVATE include)
target_include_directories(cdbnpp PRIVATE src)
