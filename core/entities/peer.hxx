/*
 * peer.h
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

#ifndef __WGFORGE_PEER_H__
#define __WGFORGE_PEER_H__

#include <boost/json.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace WgForge {

namespace json = boost::json;

/**
 * @brief Peer record database table represention.
 */
#pragma db object table("Peers")

struct Peer {
  enum class Status : uint8_t {
	Active = 0,  //< Peer is active, tunnel_address in use
	Suspended,   //< Peer is suspended, tunnel_address is reserved for it
	Revoked,     //< Peer is not used any more (for audit)
	Archived     //< Same as Status::Revoked but not shown in UI
  };

#pragma db id auto column("Id")
  int64_t id;

#pragma db column("Public_key") type("TEXT") unique options("CHECK(length(Public_key) = 44)")
  std::string public_key;

#pragma db column("Preshared_key") type("TEXT") unique options("CHECK(length(Preshared_key) = 44)")
  std::optional<std::string> preshared_key;

#pragma db column("Status") type("INTEGER")
  Status status;

#pragma db column("Tunnel_address") type("TEXT")
  std::string tunnel_address;

#pragma db column("Persistent_keepalive") options("CHECK(Persistent_keepalive BETWEEN 1 AND 65535)")
  uint16_t persistent_keepalive;

#pragma db column("Description") type("VARCHAR(50)")
  std::string description;

#pragma db column("Created_at") type("DATETIME") default("CURRENT_TIMESTAMP")
  std::string created_at;

#pragma db column("Updated_at") type("DATETIME")
  std::optional<std::string> updated_at;

#pragma db column("Deleted_at") type("DATETIME")
  std::optional<std::string> deleted_at;

  operator json::object() const&;
  operator json::object() &&;
};

}  // namespace WgForge

#endif  // __WGFORGE_PEER_H__
