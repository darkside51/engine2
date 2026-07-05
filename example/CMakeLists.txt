cmake_minimum_required(VERSION 3.22)
project(demo)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT (CMAKE_SYSTEM_NAME STREQUAL "Windows"))
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	message("demo cmake flags (debug+release) configured")
endif()

if (MSVC)
#	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
#	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
	set(CMAKE_CXX_FLAGS "/utf-8")
	set(CMAKE_C_FLAGS "/utf-8")
endif()

########## собираем engine
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/engine/src ${CMAKE_CURRENT_BINARY_DIR}/engine)

######### vtzero
set(PROTOZERO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/protozero/include)
set(VTZERO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/vtzero/include)

#set(BUILD_EXAMPLES OFF)
#set(BUILD_TESTING OFF)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/libosmium ${CMAKE_CURRENT_BINARY_DIR}/libosmium)

#### flags
if (CMAKE_BUILD_TYPE MATCHES Debug)
	add_definitions(-D_DEBUG)
    add_definitions(-DCHECK_MEMORY_LEAKS)
else()
	add_definitions(-DNDEBUG)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_executable(${PROJECT_NAME} main.cpp example.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/3rd_party/imgui)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/3rd_party/sail/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/engine/3rd_party/sail/src/bindings)

########## подключаем engine
target_link_libraries(${PROJECT_NAME} engine)
include_directories(engine/src)

######## vtzero
include_directories(${PROJECT_NAME} ${PROTOZERO_INCLUDE_DIR} ${VTZERO_INCLUDE_DIR})

########## ресурсы
set(resources_src_dir "${CMAKE_CURRENT_SOURCE_DIR}/resources")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(resources_dst_dir "${PROJECT_BINARY_DIR}/$<CONFIG>/resources")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(resources_dst_dir "${PROJECT_BINARY_DIR}/resources")
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
				  -P "${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/cmake/update_directory.cmake"
)

add_custom_target(render_pass
				  COMMAND ${CMAKE_COMMAND}
				  -Dsrc_dir="${resources_src_dir}/descriptors"
				  -Ddst_dir="${resources_dst_dir}/descriptors"
				  -P "${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/cmake/update_directory.cmake"
)

add_custom_target(shaders
                  COMMAND ${CMAKE_COMMAND} -E make_directory ${resources_dst_dir}/shaders
                  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/engine/tools/glslToSpirv/shaders_convert.py -i ${resources_src_dir}/shaders/ -o ${resources_dst_dir}/shaders/
)
add_dependencies(${PROJECT_NAME} resources shaders render_pass)

message(STATUS "!!!!!project generate complete!!!!!")
