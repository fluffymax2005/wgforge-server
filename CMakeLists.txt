cmake_minimum_required(VERSION 3.28)

set(PROJECT_NAME_CORE wgforge)

project(${PROJECT_NAME_CORE} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)


add_subdirectory(core)
add_subdirectory(contracts)
add_subdirectory(cli)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(daemon)
endif()

# Debian and Ubuntu
set(CPACK_PACKAGE_NAME "wgforge")
set(CPACK_PACKAGE_VENDOR "Ledovskiy Maksim")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "DEB;RPM;TGZ")

# Debian and Ubuntu
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ledovskiy Maksim <santech_montage@mail.ru>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libnl-3-200, libnl-route-3-200, libcurl4")

include(CPack)


