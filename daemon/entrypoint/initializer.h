/*
 * initializer.h
 * wgforge-server — Backend to manage Wireguard
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_EXECUTION_INITIALIZER_H
#define WGFORGE_EXECUTION_INITIALIZER_H

#include <libeconf.h>
#include <server_config.h>
#include <wireguard_config.h>

#include <config_manager.hpp>
#include <interface_config.hpp>
#include <odb/database.hxx>

#include "defines.h"

namespace WgForge {

void setup_wginterface(WgInterface& iface, odb::database& context, const InterfaceConfig& config);
InterfaceConfig get_iface_config(const ConfigManager& config, const char* name);
ServerConfig get_server_config(const ConfigManager& config);
WireguardConfig get_wireguard_config(const ConfigManager& config);

// Create config both for interface and http server. Implies lack of
// /usr/wgorge/<wg_interface_name>.conf
econf_file* create_config(std::string_view path, ConfigManager::ConfigReturnCode& rc) noexcept;

static std::optional<std::string> __get_public_ip();

}  // namespace WgForge

#endif  // WGFORGE_EXECUTION_INITIALIZER_H
