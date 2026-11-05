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

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CORE_NAME wgforge-core)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

# External libs
find_package(Boost 1.83 REQUIRED COMPONENTS url json CONFIG)
find_package(wgcpp REQUIRED)

# ODB
find_package(PkgConfig REQUIRED)
pkg_check_modules(ODB_CORE REQUIRED IMPORTED_TARGET libodb)
pkg_check_modules(ODB_SQLITE REQUIRED IMPORTED_TARGET libodb-sqlite)
find_program(ODB_EXECUTABLE odb REQUIRED)

set(ODB_GENERATED_SOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/entities/generated")

file(MAKE_DIRECTORY ${ODB_GENERATED_SOURCES_DIR})

set(ENTITIES_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/entities/allowedip.hxx
    ${CMAKE_CURRENT_SOURCE_DIR}/entities/peer.hxx
)

set(ODB_GENERATED_SOURCES "")

foreach(HEADER ${ENTITIES_HEADERS})
    get_filename_component(HEADER_NAME ${HEADER} NAME_WE)

    set(GEN_CXX "${ODB_GENERATED_SOURCES_DIR}/${HEADER_NAME}-odb.cxx")
    set(GEN_HXX "${ODB_GENERATED_SOURCES_DIR}/${HEADER_NAME}-odb.hxx")
    set(GEN_IXX "${ODB_GENERATED_SOURCES_DIR}/${HEADER_NAME}-odb.ixx")

    add_custom_command(
        OUTPUT ${GEN_CXX} ${GEN_HXX} ${GEN_IXX}
        COMMAND ${ODB_EXECUTABLE}
            -d sqlite
            --std c++23
            -x ${CMAKE_CXX_COMPILER}
            --generate-schema
            --generate-query
            --schema-format embedded
            --default-pointer std::shared_ptr
            --output-dir "${ODB_GENERATED_SOURCES_DIR}"
            "${HEADER}"
        DEPENDS ${HEADER}
        COMMENT "Generating ODB code for ${HEADER_NAME}.hxx"
        VERBATIM
    )

    list(APPEND ODB_GENERATED_SOURCES ${GEN_CXX})
endforeach()

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
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_include_directories(${CORE_NAME} PRIVATE /usr/include/libnl3/)
endif()

target_link_libraries(${CORE_NAME}
    PUBLIC
       PkgConfig::ODB_CORE
       PkgConfig::ODB_SQLITE
       econf
       wgcpp
)
