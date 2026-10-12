#
# CMakeLists.txt
# wgforge-server — Backend to manage Wireguard
# Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
# <santech_montage@mail.ru>
#
# This file is part of wgforge.
#
# wgforge-server is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# wgforge-server is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME wgforge-server)

project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

find_package(Boost 1.70 REQUIRED COMPONENTS ALL)
find_package(wgcpp REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

add_executable(${PROJECT_NAME}
    entrypoint/main.cpp
    entrypoint/initializer.cpp

    http_server/src/connection.cpp
    http_server/src/connection_manager.cpp
    http_server/src/request_handler.cpp
    http_server/src/wgforgeserver.cpp

    classes/spdlogger.cpp
    classes/configuration_root.cpp
    classes/configuration_section.cpp

    controllers/peer_controller.cpp
    controllers/wireguard_interface_controller.cpp

    services/peer_service.cpp
    services/wireguard_interface_service.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    entrypoint/
    exceptions/
    http_server/include/
    interfaces/
    repositories/
    classes/
    controllers/
    dto/
    services/
    /usr/include/libnl3/
)

# Add Boost.DI (boost-ext library)
add_subdirectory(libs/Boost.DI)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Boost::boost
        Boost::json
        Boost::url
        Boost.DI
        wgcpp
        econf
        nl-3
        nl-route-3
        wgforge-core
        curl
        fmt::fmt
        spdlog
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)



