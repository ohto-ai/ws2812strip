cmake_minimum_required(VERSION 3.10)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
# set the project name with the current directory name
get_filename_component(ProjectId ${CMAKE_SOURCE_DIR} NAME_WE)
string(REPLACE " " "_" ProjectId ${ProjectId})

# set the project name and version
project(${ProjectId})

if(WIN32)
    set(WINDOWS TRUE)
elseif(APPLE)
    set(MACOS TRUE)
elseif(UNIX)
    set(LINUX TRUE)
endif()

if(LINUX AND EXISTS "/sys/firmware/devicetree/base/model")
    file(READ "/sys/firmware/devicetree/base/model" RASPBERRY_PI_MODEL)
    string(STRIP ${RASPBERRY_PI_MODEL} RASPBERRY_PI_MODEL)
    if(RASPBERRY_PI_MODEL MATCHES "Raspberry")
        set(RASPBERRY_PI TRUE)
    endif()
endif()

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard" FORCE)
set(SOURCE_FOLDER ${PROJECT_SOURCE_DIR}/src CACHE STRING "Source folder")
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus /MP")
endif()
if(WINDOWS)
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL" "C:/Program Files/OpenSSL-Win64" CACHE STRING "openssl root" FORCE)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
    add_compile_definitions(NOMINMAX)
endif(WINDOWS)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_definitions(_DEBUG)
    add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

if (RASPBERRY_PI)
    add_compile_definitions(RASPBERRY_PI)
endif()
add_compile_definitions(MSGPACK_NO_BOOST)

if(GENERATE_CODE_COVERAGE AND LINUX AND CMAKE_BUILD_TYPE MATCHES Debug)
    include(CodeCoverage)
    append_coverage_compiler_flags()
endif(GENERATE_CODE_COVERAGE AND LINUX AND CMAKE_BUILD_TYPE MATCHES Debug)

find_package(OpenSSL QUIET)
find_package(Threads REQUIRED)

# include top level include directory
include_directories(${PROJECT_SOURCE_DIR}/inc)
include_directories(${PROJECT_SOURCE_DIR}/3rd/inc)
include_directories(${PROJECT_BINARY_DIR}/inc)

add_subdirectory(3rd)
add_subdirectory(src)
add_subdirectory(test)
