cmake_minimum_required(VERSION 3.22)
project(engine)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include (${CMAKE_CURRENT_SOURCE_DIR}/../tools/cmake/utils.cmake)

#### system
message("SYSTEM_NAME: ${CMAKE_SYSTEM_NAME}")

option(USE_GLFW "Use glfw window" OFF)
option(VULKAN_AVAILABLE OFF)
option(OPENGL_AVAILABLE OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_definitions(-DENGINE_PLATFORM_WINDOWS)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR=1)
    add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
	add_definitions(-DNOMINMAX)
	add_definitions(-DGLFW_AVAILABLE)
	add_definitions(-DVULKAN_AVAILABLE)
	set(USE_GLFW ON)
	set(VULKAN_AVAILABLE ON)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	add_definitions(-DENGINE_PLATFORM_LINUX)
	add_definitions(-DVK_USE_PLATFORM_XCB_KHR=1)
	add_definitions(-DGLFW_EXPOSE_NATIVE_X11)
#	add_definitions(-DVK_USE_PLATFORM_XLIB_KHR=1)
#	add_definitions(-DVK_USE_PLATFORM_WAYLAND_KHR=1)

	add_definitions(-DGLFW_AVAILABLE)
	add_definitions(-DVULKAN_AVAILABLE)
	add_definitions(-DOPENGL_AVAILABLE)

	set(USE_GLFW ON)
	set(VULKAN_AVAILABLE ON)
	set(OPENGL_AVAILABLE ON)
else()
	set(USE_GLFW OFF)
	set(VULKAN_AVAILABLE OFF)
	set(OPENGL_AVAILABLE OFF)
endif()

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

#### flags
if (CMAKE_BUILD_TYPE MATCHES Debug)
	message("debug_mode")
	add_definitions(-D_DEBUG)

    add_definitions(-DCHECK_MEMORY_LEAKS)
else()
	add_definitions(-DNDEBUG)
	message("release_mode")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

#### sources
set(ENGINE_SRC_DIRS)
set(ENGINE_HEADERS)
set(ENGINE_SOURCES)

set(
	ENGINE_SRC_DIRS ${ENGINE_SRC_DIRS}
	
	allocators
	common
	core
	device
	file
	graphics
	graphics/render
	graphics/render/rhi/common
	graphics/ui
	graphics/scene
	graphics/scene/animation
	graphics/scene/animation/action
	graphics/scene/animation/mesh
	graphics/scene/animation/texture
	graphics/scene/sprite
	graphics/texture
	graphics/mesh
	graphics/common
	graphics/debug
	input
	log
	math
	
	threads
	threads/thread_pool

	utils
	utils/parsers
	utils/profile

	../3rd_party/imgui # use as static_lib?

	tests
)

if (OPENGL_AVAILABLE)
	set(
		ENGINE_SRC_DIRS ${ENGINE_SRC_DIRS}
		graphics/render/rhi/opengl
	)
endif()

if (VULKAN_AVAILABLE)
	set(
		ENGINE_SRC_DIRS ${ENGINE_SRC_DIRS}
		graphics/render/rhi/vulkan
	)

	# for spirv-reflect
	set(
		ENGINE_HEADERS ${ENGINE_HEADERS}
		${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/SPIRV-Reflect/spirv_reflect.h
	)
	set(
		ENGINE_SOURCES ${ENGINE_SOURCES}
		${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/SPIRV-Reflect/spirv_reflect.c
	)
endif()

if (USE_GLFW)
	set(
		ENGINE_SRC_DIRS ${ENGINE_SRC_DIRS}
		device/glfw
	)
else()
	message(FATAL_ERROR "no set correct platform directory!")
endif()

#### для каждого заданного пути
foreach (DIR ${ENGINE_SRC_DIRS})
	message("DIR:" ${DIR})

	# собираем список файлов (не рекурсивно, не GLOB_RECURSE !)
	# можно еще обобщить, задавая список нужных расширений

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

	list(APPEND ENGINE_HEADERS ${HEADERS})
	list(APPEND ENGINE_SOURCES ${SOURCES})

endforeach(DIR)

list(REMOVE_DUPLICATES ENGINE_HEADERS)
list(REMOVE_DUPLICATES ENGINE_SOURCES)
#######

#### fix paths for windows
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	foreach (source IN LISTS ENGINE_SOURCES)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "Engine/" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()

	foreach (source IN LISTS ENGINE_HEADERS)
		get_filename_component(source_path "${source}" PATH)
		string(REPLACE "/" "\\" source_path_msvc "${source_path}")
		string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "Engine/" source_path_msvc "${source_path}")
		source_group("${source_path_msvc}" FILES "${source}")
	endforeach()
endif()
#######

add_library(
	${PROJECT_NAME}
	STATIC
	${ENGINE_SOURCES}
	${ENGINE_HEADERS}
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/)

if (VULKAN_AVAILABLE) #### vulkan lib
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(
			USED_FRAMEWORKS
			general $ENV{VULKAN_SDK}/Lib/vulkan-1.lib
		)
		target_include_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/Include)
	elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(
			USED_FRAMEWORKS
			general $ENV{VULKAN_SDK}/lib/libvulkan.so
		)
		target_include_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/include)
	endif()

	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/vma/include)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/SPIRV-Reflect)
endif()

#### 3rd_party libraries

# meshoptimizer
message("meshoptimizer library begin")
add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/meshoptimizer ${CMAKE_BINARY_DIR}/3rd_party/meshoptimizer)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/meshoptimizer/src)

set(
	USED_FRAMEWORKS ${USED_FRAMEWORKS}
	meshoptimizer
	)
message("meshoptimizer library end")

# ktx
message("ktx library begin")
add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/KTX-Software ${CMAKE_BINARY_DIR}/3rd_party/KTX-Software)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/KTX-Software/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/KTX-Software/lib)
set(
	USED_FRAMEWORKS ${USED_FRAMEWORKS}
	ktx
	)
message("ktx library end")

## freetype library
#message("freetype library begin")
#add_3rdparty_project (${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/freetype ${CMAKE_BINARY_DIR}/3rd_party/freetype)
#target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/freetype/include)
# set(
# 	USED_FRAMEWORKS ${USED_FRAMEWORKS}
# 	freetype
# 	)
#message("freetype library end")

# glaze json library
message("glaze library begin")
add_3rdparty_project(${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glaze ${CMAKE_BINARY_DIR}/3rd_party/glaze)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glaze/include)

set(
	USED_FRAMEWORKS ${USED_FRAMEWORKS}
	glaze_glaze
	)
message("glaze library end")

# sail
message("sail library begin")
set(BUILD_SHARED_LIBS OFF)
set(SAIL_THIRD_PARTY_CODECS_PATH OFF)

add_3rdparty_project(${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/sail ${CMAKE_BINARY_DIR}/3rd_party/sail)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/sail/src)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/sail/src/bindings)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_BINARY_DIR}/3rd_party/sail/include)

set(
	USED_FRAMEWORKS ${USED_FRAMEWORKS}
	sail-c++
	)
message("sail library end")

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/imgui)

if (USE_GLFW)
	# glfw library
	message("glfw library begin")
	add_3rdparty_project(${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glfw ${CMAKE_BINARY_DIR}/3rd_party/glfw)
	target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party/glfw/include)
	
	set(
		USED_FRAMEWORKS ${USED_FRAMEWORKS}
		glfw
	)
	message("glfw library end")
endif()

# link libraries
target_link_libraries (
	${PROJECT_NAME}
	${USED_FRAMEWORKS}
)

message(STATUS "!!!!!engine generate success!!!!!")