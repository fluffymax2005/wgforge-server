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
#include "iconfiguration.h"

namespace WgForge {

/**
 * Setup WgInterface instance based on DB context and internal configuration.
 * @param iface interface instance
 * @param context database instance
 * @param config internal configuration
 * @throw std::runtime_error when failed to configure interface based on provided database data
 */
void setup_wginterface(WgInterface& iface, odb::database& context, const InterfaceConfig& config);

/**
 * Extract Wireguard interface configuration via associated manage provided by [Interface] section.
 * @param config manager to interact with
 * @param name interface name
 * @throw std::runtime_error if error occured while reading data via manager
 * @throw std::invalid_argument if one of provided values is invalid
 * @note config required to be already opened to target file.
 */
InterfaceConfig get_iface_config(const ConfigManager& config, const std::string& name);

/**
 * Extract HTTP server configuration via associated manager provided by [Server] section.
 * @param config manager to interact with
 * @throw std::runtime_error if error occured while reading data via manager
 * @throw std::invalid_argument if one of provided values is invalid
 * @note config required to be already opened to target file.
 */
ServerConfig get_server_config(const ConfigManager& config);

/**
 * Extract configuration via associated manager provided by [Wireguard] section.
 * @param config manager to interact with
 * @throw std::runtime_error if error occured while reading data via manager
 * @throw std::invalid_argument if one of provided values is invalid
 * @note config required to be already opened to target file.
 */
WireguardConfig get_wireguard_config(const ConfigManager& config);

/**
 * Validate loaded Wireguard interface configuration to stored one in database.
 * @param config manager to load config with
 * @param context database context to read from
 * @param name interface name
 * @retval true if database file was moved and new one should be created
 * @retval false if new database should not be created
 * @throw std::runtime_error failed to read data from config or database
 * @throw std::invalid_argument invalid data provided from either config either database 
 * @note config required to be already opened to target file as well as context associated with proper database.
 */
bool validateDatabaseSchemaUseCase(ConfigManager& config, odb::database& context);

/**
 * Migrate database. Move old one by new path /var/lib/wgforge/backup
 * @path name interface name
 * @throw std::runtime_error caught system IO errors
 */
std::string migrateDatabase(const std::string& name);



// Create config both for interface and http server. Implies lack of
// /usr/wgorge/<wg_interface_name>.conf
econf_file* create_config(std::string_view path, ConfigManager::ConfigReturnCode& rc) noexcept;

static std::optional<std::string> __get_public_ip();

}  // namespace WgForge

#endif  // WGFORGE_EXECUTION_INITIALIZER_H
