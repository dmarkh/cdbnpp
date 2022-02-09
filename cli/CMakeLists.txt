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

add_executable(cdbnpp-cli
	src/cdbnpp-cli.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_BUILD_TYPE MATCHES Release)
	  target_compile_options(cdbnpp-cli PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -O2 -g -Wno-dollar-in-identifier-extension)
	elseif()
	  target_compile_options(cdbnpp-cli PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -g -Wno-dollar-in-identifier-extension)
	endif()
	target_link_libraries( cdbnpp-cli stdc++ )
  target_link_libraries( cdbnpp-cli stdc++fs )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if (CMAKE_BUILD_TYPE MATCHES Release)
	  target_compile_options(cdbnpp-cli PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -O2 -g)
	else()
	  target_compile_options(cdbnpp-cli PUBLIC -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -fsanitize=address -g)
  	target_link_libraries(cdbnpp-cli asan)
	endif()
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
    target_link_libraries( cdbnpp-cli stdc++fs )
  endif()
endif()

target_link_libraries( cdbnpp-cli soci_core )

target_include_directories(cdbnpp-cli PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(cdbnpp-cli PRIVATE ${CMAKE_SOURCE_DIR}/../contrib)
target_include_directories(cdbnpp-cli PRIVATE ${CMAKE_SOURCE_DIR}/../lib/include)
target_link_libraries( cdbnpp-cli ${CMAKE_SOURCE_DIR}/../lib/build/libcdbnpp.so )
