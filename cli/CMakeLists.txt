#
# CMakeLists.txt
# wgforge-cli — CLI frontend for wgforge-server (C)
# Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
# <santech_montage@mail.ru>
#
# This file is part of wgforge.
#
# wgforge-cli is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# wgforge-cli is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with wgforge-cli. If not, see <https://www.gnu.org/licenses/>.
#

set(WGFORGE_CLI_NAME wgforge-cli)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(${WGFORGE_CLI_NAME} VERSION 1.0.0 LANGUAGES CXX)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

find_package(CLI11 CONFIG REQUIRED)
find_package(wgcpp REQUIRED)
find_package(Boost 1.83 REQUIRED COMPONENTS url json CONFIG)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSSH REQUIRED IMPORTED_TARGET libssh)

add_executable(${WGFORGE_CLI_NAME}
    entrypoint/main.cpp
    entrypoint/cli_app.cpp

    controllers/cli_controller.cpp

    api/peer_api.cpp
    api/interface_api.cpp
    api/docker_api.cpp

    managers/interface_manager.cpp
    managers/peer_manager.cpp
    managers/server_manager.cpp
    managers/deploy_manager.cpp

    parsers/interface_serializer.cpp
    parsers/peer_serializer.cpp
    parsers/data_processor.cpp

    senders/boost_beast_http_sender.cpp

    services/ssh_tunnel.cpp
)

# Add Boost.DI (boost-ext library)
if (NOT TARGET Boost.DI)
    add_subdirectory(${LIBS_DIR}/Boost.DI ${CMAKE_CURRENT_BINARY_DIR}/Boost.DI)
endif()

target_link_libraries(${WGFORGE_CLI_NAME}
    PRIVATE
        wgforge-core
        CLI11::CLI11
        wgforge-contracts
        wgcpp
        Boost::url
        Boost::json
        Boost.DI
        PkgConfig::LIBSSH
)

target_include_directories(${WGFORGE_CLI_NAME}
    PRIVATE
        dto/
        entities/
        interfaces/
        entrypoint/
        controllers/
        managers/
        parsers/
        senders/
        services/
        api/

)

target_compile_definitions(${WGFORGE_CLI_NAME}
    PRIVATE
        WGFORGE_CLI_VERSION="${PROJECT_VERSION}"
)

install(TARGETS ${WGFORGE_CLI_NAME}
    DESTINATION bin
    COMPONENT wgforge-cli
)
