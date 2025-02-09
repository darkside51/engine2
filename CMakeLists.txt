cmake_minimum_required(VERSION 3.22)
project(demo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	message("engine cmake flags (debug+release) configured")
endif()

if (MSVC)
#	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
#	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	set(CMAKE_CXX_FLAGS "/utf-8")
	set(CMAKE_C_FLAGS "/utf-8")
endif()

########## собираем engine
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/engine)

#### flags
if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-D_DEBUG)
    add_definitions(-DCHECK_MEMORY_LEAKS)
else()
endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_executable(${PROJECT_NAME} main.cpp)

########## подключаем engine
target_link_libraries(${PROJECT_NAME} engine)
include_directories(./src)

message(STATUS "!!!!!project generate complete!!!!!")
