cmake_minimum_required(VERSION 3.22)

project(demo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	message("demo cmake flags (debug+release) configured")
else()
	set(CURL_USE_LIBPSL OFF CACHE BOOL "Disable libpsl" FORCE)
	set(LIBPSL_INCLUDE_DIR "LIBPSL_INCLUDE_DIR-NOTFOUND" CACHE PATH "Disable libpsl" FORCE)
	set(LIBPSL_LIBRARY "LIBPSL_LIBRARY-NOTFOUND" CACHE FILEPATH "Disable libpsl" FORCE)
endif()

if (MSVC)
#	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
#	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	string(FIND "${CMAKE_CXX_FLAGS}" "/utf-8" _utf8_pos)
	if (_utf8_pos EQUAL -1)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
	endif()
endif()

#add_compile_options(-fno-exceptions)

# defining the engine directory
set(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine")
if (NOT EXISTS "${ENGINE_DIR}/src/CMakeLists.txt")
	get_filename_component(ENGINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
	message(STATUS "engine directory (symlink not found): ${ENGINE_DIR}")
endif()


########## собираем engine
add_subdirectory(${ENGINE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/engine)

######### vtzero
set(PROTOZERO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/protozero/include)
set(VTZERO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/vtzero/include)


#### flags
add_compile_definitions(
	$<$<CONFIG:Debug>:_DEBUG>
	$<$<CONFIG:Debug>:CHECK_MEMORY_LEAKS>
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

if (WIN32)
	add_compile_definitions(ENGINE_PLATFORM_WINDOWS NOMINMAX)
elseif (UNIX)
	add_compile_definitions(ENGINE_PLATFORM_LINUX)
endif()


# sources
set(EXAMPLE_SRC_DIRS)
set(EXAMPLE_HEADERS)
set(EXAMPLE_SOURCES)

set(
	EXAMPLE_SOURCES ${EXAMPLE_SOURCES}
	main.cpp
)

set(
	EXAMPLE_SRC_DIRS ${EXAMPLE_SRC_DIRS}
	
	./
	map
	game_map
)

foreach (DIR ${EXAMPLE_SRC_DIRS})
	message("DIR:" ${DIR})

	file(GLOB
		HEADERS
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.h
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.hpp
	)

	file(GLOB
		SOURCES
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/*.cpp
	)

	list(APPEND EXAMPLE_HEADERS ${HEADERS})
	list(APPEND EXAMPLE_SOURCES ${SOURCES})

endforeach(DIR)

list(REMOVE_DUPLICATES EXAMPLE_HEADERS)
list(REMOVE_DUPLICATES EXAMPLE_SOURCES)

add_executable(
	${PROJECT_NAME}
	${EXAMPLE_SOURCES}
	#${EXAMPLE_HEADERS}
)

#target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions)
target_include_directories(${PROJECT_NAME} PUBLIC ${ENGINE_DIR}/3rd_party/imgui)

target_include_directories(${PROJECT_NAME} PUBLIC ${ENGINE_DIR}/3rd_party/sail/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${ENGINE_DIR}/3rd_party/sail/src/bindings)

if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE EASTL_EXCEPTIONS=0)
	target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
endif()

########## подключаем engine
target_link_libraries(${PROJECT_NAME} engine)
include_directories(./)

######## vtzero
include_directories(${PROJECT_NAME} ${PROTOZERO_INCLUDE_DIR} ${VTZERO_INCLUDE_DIR})

########## ресурсы
set(resources_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/resources")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(resources_dst_dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# target_compile_options(${PROJECT_NAME} PRIVATE -mavx2)
	set(resources_dst_dir "${PROJECT_BINARY_DIR}/resources")
endif()

find_package(Python3 COMPONENTS Interpreter QUIET)
if (Python3_Interpreter_FOUND)
	set(PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
else()
	find_program(PYTHON_EXECUTABLE NAMES python3 python py)
endif()
if (NOT PYTHON_EXECUTABLE)
	message(FATAL_ERROR "Python 3 interpreter not found. Install Python 3 and add it to PATH.")
endif()

# glslc поставляется вместе с Vulkan SDK.
find_program(GLSL_COMPILER NAMES glslc glslc.exe
	HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
if (NOT GLSL_COMPILER)
	message(FATAL_ERROR "glslc not found. Install the Vulkan SDK and/or add its Bin directory to PATH.")
endif()

# add_custom_command(TARGET ${PROJECT_NAME}
# 				  POST_BUILD
# 	              COMMAND ${CMAKE_COMMAND}
#     	          -Dsrc_dir="${resources_src_dir}/assets"
#     	          -Ddst_dir="${resources_dst_dir}/assets"
#     	          -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake/update_directory.cmake"
# 	              )

#add_custom_command(TARGET ${PROJECT_NAME}
#                   POST_BUILD
#                   COMMAND ${CMAKE_COMMAND} -E make_directory ${resources_dst_dir}/shaders
#                   COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/glslToSpirv/shaders_convert.py -i ${resources_src_dir}/shaders/ -o ${resources_dst_dir}/shaders/
#                   )


add_custom_target(resources
				  COMMAND ${CMAKE_COMMAND}
				  -Dsrc_dir="${resources_src_dir}/assets"
				  -Ddst_dir="${resources_dst_dir}/assets"
				  -P "${ENGINE_DIR}/tools/cmake/update_directory.cmake"
)

add_custom_target(render_pass
				  COMMAND ${CMAKE_COMMAND}
				  -Dsrc_dir="${resources_src_dir}/descriptors"
				  -Ddst_dir="${resources_dst_dir}/descriptors"
				  -P "${ENGINE_DIR}/tools/cmake/update_directory.cmake"
)

message(STATUS "GLSL_COMPILER: ${GLSL_COMPILER}")

add_custom_target(shaders
                  COMMAND ${CMAKE_COMMAND} -E make_directory "${resources_dst_dir}/shaders"
                  COMMAND "${PYTHON_EXECUTABLE}" "${ENGINE_DIR}/tools/glslToSpirv/shaders_convert.py"
                          -i "${resources_src_dir}/shaders/"
                          -o "${resources_dst_dir}/shaders/"
                          -p "${GLSL_COMPILER}"
)
add_dependencies(${PROJECT_NAME} resources shaders render_pass)

message(STATUS "!!!!!project generate complete!!!!!")
