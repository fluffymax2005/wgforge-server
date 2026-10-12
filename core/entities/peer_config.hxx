/*
 * peer_config.hxx
 * wgforge-core — Core library for wgforge-server (C)
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

#ifndef __WGFORGE_PEER_CONFIG_H__
#define __WGFORGE_PEER_CONFIG_H__

#include <boost/json.hpp>
#include <vector>

#include "peer.hxx"

namespace WgForge {

namespace json = boost::json;

/**
 * @brief Full peer configuration including AllowedIPs
 */
struct PeerConfig {
  Peer record;
  std::vector<std::string> allowed_ips;
};

inline void tag_invoke(json::value_from_tag, json::value& jv, const PeerConfig& config) {
  jv = json::object{
      {      "peer",      json::value_from(config.record)},
      {"allowedIps", json::value_from(config.allowed_ips)}
  };
}

}  // namespace WgForge

#endif  // __WGFORGE_PEER_CONFIG_H__
