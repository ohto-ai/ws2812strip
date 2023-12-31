cmake_minimum_required(VERSION 3.24)

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_DESCRIBE --tags --always)
git_get_tag(GIT_TAG --tags)

if(NOT GIT_REFSPEC)
    set(GIT_REFSPEC "unknown")
endif(NOT GIT_REFSPEC)

message(STATUS "GIT_REFSPEC\t${GIT_REFSPEC}")
message(STATUS "GIT_SHA1\t${GIT_SHA1}")
message(STATUS "GIT_DESCRIBE\t${GIT_DESCRIBE}")

# get version from git describe
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" PROJECT_VERSION_MAJOR "${GIT_DESCRIBE}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" PROJECT_VERSION_MINOR "${GIT_DESCRIBE}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" PROJECT_VERSION_PATCH "${GIT_DESCRIBE}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+\\-([0-9]+).*" "\\1" PROJECT_VERSION_TWEAK "${GIT_DESCRIBE}")

if(PROJECT_VERSION_MAJOR MATCHES GIT_DESCRIBE)
    set(PROJECT_VERSION_MAJOR 0)
endif(PROJECT_VERSION_MAJOR MATCHES GIT_DESCRIBE)
if(PROJECT_VERSION_MINOR MATCHES GIT_DESCRIBE)
    set(PROJECT_VERSION_MINOR 0)
endif(PROJECT_VERSION_MINOR MATCHES GIT_DESCRIBE)
if(PROJECT_VERSION_PATCH MATCHES GIT_DESCRIBE)
    set(PROJECT_VERSION_PATCH 0)
endif(PROJECT_VERSION_PATCH MATCHES GIT_DESCRIBE)
if(PROJECT_VERSION_TWEAK MATCHES GIT_DESCRIBE)
    set(PROJECT_VERSION_TWEAK 0)
endif(PROJECT_VERSION_TWEAK MATCHES GIT_DESCRIBE)

if(PROJECT_VERSION_TWEAK)
    set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}")
else()
    set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif(PROJECT_VERSION_TWEAK)

# set the project name and version
project(${ProjectId} LANGUAGES CXX VERSION ${PROJECT_VERSION})

# set project maintainer and contact
set(PROJECT_MAINTAINER "OhtoAi")
set(PROJECT_CONTACT "zhu.thatboy@outlook.com")
set(PROJECT_DESCRIPTION "ws2812 client")
set(PROJECT_DEBIAN_DEPENDENCIES "libssl1.1 (>= 1.1.0)")
set(PROJECT_HOMEPAGE "https://ohtoai.top/")

# trans project name
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_VAR)
string(REPLACE "-" "_" PROJECT_NAME_VAR ${PROJECT_NAME_VAR})

# configure .in file
configure_file(inc/version.h.in ${CMAKE_BINARY_DIR}/inc/version.h @ONLY)

# Add source files
set(SOURCES
    main.cpp
)

# Build the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE inc ../3rd/rpi_ws281x)

target_link_libraries(${PROJECT_NAME} PRIVATE
    scn::scn
    Threads::Threads
    $<$<BOOL:${OpenSSL_FOUND}>:OpenSSL::SSL>
    $<$<BOOL:${OpenSSL_FOUND}>:OpenSSL::Crypto>
    nlohmann_json
    argparse
    tabulate
    eventpp
    yaml-cpp
    ws2811
    spdlog $<$<BOOL:${MINGW}>:ws2_32>)

include(InstallRequiredSystemLibraries)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)

# set install target
if (LINUX OR WINDOWS)
    if(LINUX)
        set(CPACK_GENERATOR "DEB")
        set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PROJECT_MAINTAINER})
        set(CPACK_DEBIAN_PACKAGE_DEPENDS ${PROJECT_DEBIAN_DEPENDENCIES})
        set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE})
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    elseif(WINDOWS)
        set(CPACK_GENERATOR "NSIS")
        set(CPACK_NSIS ${PROJECT_NAME})
        set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/res/icon.ico)
        set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/res/icon.ico)
        set(CPACK_NSIS_MODIFY_PATH ON)
        set(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME})
        set(CPACK_NSIS_HELP_LINK ${PROJECT_HOMEPAGE})
        set(CPACK_NSIS_URL_INFO_ABOUT ${PROJECT_HOMEPAGE})
        set(CPACK_NSIS_CONTACT ${PROJECT_CONTACT})
    else()
        message(FATAL_ERROR "Unsupported platform")
    endif()

    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_CONTACT ${PROJECT_CONTACT})
    set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    include(CPack)
endif()
