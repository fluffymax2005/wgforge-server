#
# CMakeLists.txt
# wgforge-contracts — contracts for wgforge-server (C) and wgforge-cli  (C)
# Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
# <santech_montage@mail.ru>
#
# This file is part of wgforge.
#
# wgforge-contracts is free software: you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation,
# either version 3 of the License, or (at your option) any later version.
#
# wgforge-contracts is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with wgforge-contracts. If not, see <https://www.gnu.org/licenses/>.
#

set(WGFORGE_CONTRACTS_NAME wgforge-contracts)
project(${WGFORGE_CONTRACTS_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${WGFORGE_CONTRACTS_NAME} INTERFACE)

target_include_directories(${WGFORGE_CONTRACTS_NAME}
    INTERFACE
        dto/
)
