#
# CMakeLists.txt
# wgforge-core — Core library for wgforge-server (C) and wgforge-cli (C)
# Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
# <santech_montage@mail.ru>
#
# This file is part of wgforge.
#
# wgforge-core is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# wgforge-core is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with wgforge-core. If not, see <https://www.gnu.org/licenses/>.
#

set(CORE_NAME wgforge-core)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

# External libs
find_package(Boost 1.92 REQUIRED COMPONENTS url json CONFIG)
find_package(wgcpp REQUIRED)

# ODB
find_package(PkgConfig REQUIRED)
pkg_check_modules(ODB_CORE REQUIRED IMPORTED_TARGET libodb)
pkg_check_modules(ODB_SQLITE REQUIRED IMPORTED_TARGET libodb-sqlite)

set(ODB_GENERATED_SOURCES_DIR entities/generated)
set(ODB_GENERATED_SOURCES
    ${ODB_GENERATED_SOURCES_DIR}/peer-odb.cxx
    ${ODB_GENERATED_SOURCES_DIR}/allowedip-odb.cxx
)


add_library(${CORE_NAME}
    STATIC
        src/config_error_handler.cpp
        src/config_manager.cpp
        src/subnetscanner.cpp
        src/inetutils.cpp

        ${ODB_GENERATED_SOURCES}

)

target_include_directories(${CORE_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
        ${CMAKE_CURRENT_SOURCE_DIR}/entities/
        ${ODB_GENERATED_SOURCES_DIR}
        /usr/include/libnl3/
)

target_link_libraries(${CORE_NAME}
    PUBLIC
       PkgConfig::ODB_CORE
       PkgConfig::ODB_SQLITE
       econf
       wgcpp
)

