# Include the local .env file
include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env file included successfully.")

# Set the minumum required version
cmake_minimum_required(VERSION ${CMAKE_MIN_VERSION})
message(STATUS "CMake minimum required version set to ${CMAKE_MIN_VERSION}.")

# Create the project
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.0)
message(STATUS "Project name set to \"${PROJECT_NAME}\".")
message(STATUS "Project version set to ${PROJECT_VERSION}.")

# Add MSVC compile options, if required
if(MSVC)
	add_compile_options(/D_CRT_SECURE_NO_WARNINGS /D_USE_MATH_DEFINES /Zc:preprocessor)
endif()

# Configure the info header file
configure_file(${PROJECT_SOURCE_DIR}/info/ProjectInfo.hpp.in ${PROJECT_SOURCE_DIR}/info/ProjectInfo.hpp)
message(STATUS "Configured info header file successfully.")

# Find the source files
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
list(LENGTH SOURCES SOURCE_COUNT)

if(SOURCE_COUNT EQUAL 0)
	message(FATAL_ERROR "ERROR: No sources found!")
elseif(SOURCE_COUNT EQUAL 1)
	message(STATUS "1 source found.")
else()
	message(STATUS "${SOURCE_COUNT} sources found.")
endif()

# Create the project executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
message(STATUS "Project executable created successfully.")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	# Add the include directories for Windows
	target_include_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/Include ${PROJECT_SOURCE_DIR}/info ${PROJECT_SOURCE_DIR}/src)
	message(STATUS "Project include directories added.")

	if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
		# Add the link directories for 32-bit Windows
		target_link_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/Lib32)
		message(STATUS "Project link directories added.")
	else()
		# Add the link directories for 64-bit Windows
		target_link_directories(${PROJECT_NAME} PUBLIC $ENV{VULKAN_SDK}/Lib)
		message(STATUS "Project link directories added.")
	endif()

	# Add the link libraries for Windows
	target_link_libraries(${PROJECT_NAME} vulkan-1)
	message(STATUS "Project link libraries added.")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	# Add the include directories for Linux
	target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/info ${PROJECT_SOURCE_DIR}/src)
	message(STATUS "Project include directories added.")

	# Add the link libraries for Linux
	target_link_libraries(${PROJECT_NAME} vulkan X11 Xfixes xkbcommon)
	message(STATUS "Project link libraries added.")
endif()

# Find all shaders in the project
file(GLOB_RECURSE GLSL_SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.vert ${PROJECT_SOURCE_DIR}/src/*.frag ${PROJECT_SOURCE_DIR}/src/*.comp)
set(GLSL_VALIDATOR glslangValidator)
list(LENGTH GLSL_SOURCE_FILES GLSL_COUNT)

if(GLSL_COUNT EQUAL 0)
    message(STATUS "No shaders found.")
elseif(GLSL_COUNT EQUAL 1)
    message(STATUS "1 shader found.")
else()
    message(STATUS "${GLSL_COUNT} shaders found.")
endif()

# Compile the shaders
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(GLSL_NAME ${GLSL} NAME)
    set(SPIRV "${GLSL}.u32")
    add_custom_command(OUTPUT ${SPIRV} COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.1 -x ${GLSL} -o ${SPIRV} DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(SHADERS ALL DEPENDS ${SPIRV_BINARY_FILES})
message(STATUS "Shader compile step added successfully.")

# Add CPack components
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
message(STATUS "CPack components added.")