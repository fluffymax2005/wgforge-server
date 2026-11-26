/*
 * peer.h
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

#ifndef WGFORGE_PEER_HXX
#define WGFORGE_PEER_HXX

#include <boost/json.hpp>
#include <cstdint>
#include <optional>
#include <string>

#include "allowedip.hxx"

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

#pragma db column("Expired_at") type("DATETIME")
  std::optional<std::string> expired_at;

#pragma db column("Created_at") type("DATETIME") default("CURRENT_TIMESTAMP")
  std::string created_at;

#pragma db column("Updated_at") type("DATETIME")
  std::optional<std::string> updated_at;

#pragma db column("Deleted_at") type("DATETIME")
  std::optional<std::string> deleted_at;

#pragma db transient
  std::optional<std::vector<std::string>> allowed_ips;

  static constexpr std::string statusToString(Status status) {
	switch (status) {
	case Status::Active   : return "Active";
	case Status::Suspended: return "Suspended";
	case Status::Revoked  : return "Revoked";
	case Status::Archived : return "Archived";
	default               : return "Unknown";
	}
  }
};

#ifndef ODB_COMPILER

inline void tag_invoke(json::value_from_tag, json::value& jv, const Peer& peer) {
  jv = json::object{
      {                 "id",                         peer.id},
      {          "publicKey",                 peer.public_key},
      {       "presharedKey", peer.preshared_key.value_or("")},
      {             "status",   static_cast<int>(peer.status)},
      {      "tunnelAddress",             peer.tunnel_address},
      {         "allowedIps",                peer.allowed_ips},
      {"persistentKeepalive",       peer.persistent_keepalive},
      {        "description",                peer.description},
      {          "expiredAt",                 peer.expired_at},
      {          "createdAt",                 peer.created_at},
      {          "updatedAt",    peer.updated_at.value_or("")},
      {          "deletedAt",    peer.deleted_at.value_or("")}
  };
}

inline Peer tag_invoke(json::value_to_tag<Peer>, const json::value& jv) {
  const auto& obj = jv.as_object();
  Peer peer;

  peer.id = json::value_to<int64_t>(obj.at("id"));
  peer.public_key = json::value_to<std::string>(obj.at("publicKey"));
  peer.status = static_cast<Peer::Status>(json::value_to<uint8_t>(obj.at("status")));
  peer.tunnel_address = json::value_to<std::string>(obj.at("tunnelAddress"));
  peer.persistent_keepalive = json::value_to<uint16_t>(obj.at("persistentKeepalive"));
  peer.description = json::value_to<std::string>(obj.at("description"));
  peer.created_at = json::value_to<std::string>(obj.at("createdAt"));

  auto parse_opt = []<typename T>(const json::object& o, const char* key) -> std::optional<T> {
	if (auto* val = o.if_contains(key); val && !val->is_null()) {
	  auto result = json::value_to<T>(*val);
	  if constexpr (std::is_same_v<T, std::string>) {
		if (result.empty())
		  return std::nullopt;
	  } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
		if (result.empty())
		  return std::nullopt;
	  }
	  return result;
	}
	return std::nullopt;
  };

  // Parse opt single values
  peer.preshared_key = parse_opt.operator()<std::string>(obj, "presharedKey");
  peer.expired_at = parse_opt.operator()<std::string>(obj, "expiredAt");
  peer.updated_at = parse_opt.operator()<std::string>(obj, "updatedAt");
  peer.deleted_at = parse_opt.operator()<std::string>(obj, "deletedAt");
  peer.allowed_ips = parse_opt.operator()<std::vector<std::string>>(obj, "allowedIps");

  return peer;
}

#endif

}  // namespace WgForge

#endif  // WGFORGE_PEER_HXX
