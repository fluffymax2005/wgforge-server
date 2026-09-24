/*
 * peer_config.h
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

#ifndef __WGFORGE_PEER_RECORD_H__
#define __WGFORGE_PEER_RECORD_H__

#include <boost/json.hpp>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace WgForge {

namespace json = boost::json;

/**
 * @brief Peer record database table represention.
 */
struct PeerRecord {
  int64_t id;
  std::string public_key;
  std::optional<std::string> preshared_key;
  std::string cidr;
  uint16_t persistent_keepalive;
  std::optional<std::string> description;
  std::string created_at;
  std::optional<std::string> updated_at;
  std::optional<std::string> deleted_at;
};

/**
 * @brief Full peer configuration including AllowedIPs
 */
struct PeerConfig {
  PeerRecord record;
  std::vector<std::string> allowed_ips;

  /**
   * @brief lvalue operator to convert config to json object.
   */
  operator json::object() const&;

  /**
   * @brief rvalue operator to convert config to json object. 
   */
  operator json::object() &&;
};

} // namespace WgForge

#endif // __WGFORGE_PEER_RECORD_H__
