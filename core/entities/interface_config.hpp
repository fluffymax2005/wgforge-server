/*
 * interface_config.hpp
 * wgforge-core — Core library for wgforge-server (C) and wgforge-cli (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-core is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-core. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_INTERFACE_CONFIG_HPP
#define WGFORGE_INTERFACE_CONFIG_HPP

#include <net/if.h>
#include <netinet/in.h>

#include <boost/json.hpp>
#include <cstring>
#include <optional>
#include <wgallowedip.hpp>

#include "wgforge.h"

namespace WgForge {

namespace json = boost::json;

/**
 * @brief Wireguard iface configuration
 */
struct InterfaceConfig {
  std::optional<std::string>
      iface;  ///< Interface name to read config for from /etc/wgforge/{}.conf
  std::optional<std::string> endpoint;  ///< Override interface config reading and send HTTP request

  char addr[INET_ADDRSTRLEN];  ///< IPv4 address
  WgPrivateKey private_key;    ///< Private key
  char name[IFNAMSIZ];         ///< Name
  in_port_t listenPort;        ///< Listen port
  Protocol proto_type;         ///< Protocol type listening on. Reserved for future
  bool checkSubnet;            ///< Whether daemon check subnet occupation on startup
};

inline void tag_invoke(json::value_from_tag, json::value& jv, const InterfaceConfig& config) {
  jv = json::object{
      {    "address",                                           config.addr},
      { "privateKey",                      config.private_key.toB64String()},
      {  "ifaceName",                                           config.name},
      { "listenPort",                                     config.listenPort},
      {      "proto", config.proto_type == Protocol::IPv4 ? "IPv4" : "IPv6"},
      {"checkSubnet",                                    config.checkSubnet}
  };
}

inline InterfaceConfig tag_invoke(json::value_to_tag<InterfaceConfig>, const json::value& jv) {
  const auto& obj = jv.as_object();
  InterfaceConfig config;

  auto addr = json::value_to<std::string>(obj.at("address"));
  std::strcpy(config.addr, addr.c_str());

  in6_addr buf;
  if (inet_pton(AF_INET, config.addr, &buf) == 0)
	config.proto_type = Protocol::IPv4;
  else if (inet_pton(AF_INET6, config.addr, &buf) == 0)
	config.proto_type = Protocol::IPv6;
  else
	throw std::invalid_argument("Invalid address");


  auto private_key = json::value_to<std::string>(obj.at("privateKey"));
  config.private_key = WgPrivateKey(private_key);

  auto name = json::value_to<std::string>(obj.at("ifaceName"));
  std::strcpy(config.name, name.c_str());

  config.listenPort = json::value_to<in_port_t>(obj.at("listenPort"));

  if (auto val = obj.try_at("checkSubnet"))
	config.checkSubnet = json::value_to<bool>(*val);

  return config;
}


}  // namespace WgForge


#endif  // WGFORGE_INTERFACE_CONFIG_HPP
