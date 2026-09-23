/*
 * defines.h
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

#ifndef __WGFORGE_DEFINES_H__
#define __WGFORGE_DEFINES_H__

#define BLUE_COLOR  "\033[34m"
#define RED_COLOR   "\033[31m"
#define RESET_COLOR "\033[0m"

#include <config_manager.h>
#include <libeconf.h>
#include <sqlite3.h>
#include <sqlite_db_manager.h>
#include <wgforge.h>

#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <libwgcpp/wginterface.hpp>

namespace WgForge {

namespace beast = boost::beast;
namespace http = boost::beast::http;
namespace json = boost::json;

using Request = const http::request<http::string_body>&;
using Response = http::response<http::string_body>&;

struct WireguardConfig {
  char public_endpoint[INET_ADDRSTRLEN + 6]; // :<port_number>
};

struct EndpointArgs {
  WgInterface& iface;
  ConfigManager& config;
  SQLiteDbManager& db;
  const ServerConfig& server_cfg;
  const InterfaceConfig& iface_cfg;
  const WireguardConfig& wireguard_cfg;

  EndpointArgs(WgInterface& iface, ConfigManager& cfg, SQLiteDbManager& db,
               const ServerConfig& server_config, const InterfaceConfig& iface_config,
               const WireguardConfig& wireguard_config)
      : iface(iface)
      , config(cfg)
      , db(db)
      , server_cfg(server_config)
      , iface_cfg(iface_config)
      , wireguard_cfg(wireguard_config) {}
};


}  // namespace WgForge

#endif  // __WGFORGE_DEFINES_H__
