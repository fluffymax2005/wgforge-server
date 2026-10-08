/*
 * peer_config.cpp
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

#include "peer_config.h"

namespace WgForge {

PeerConfig::operator json::object() const& {
  json::object o;
  o["id"] = record.id;
  o["public_key"] = record.public_key;
  if (record.preshared_key)
	o["preshared_key"] = *record.preshared_key;
  o["CIDR"] = record.cidr;
  o["persistent_keepalive"] = record.persistent_keepalive;
  o["description"] = record.description;
  o["created_at"] = record.created_at;
  if (record.updated_at)
	o["updated_at"] = *record.updated_at;
  if (record.deleted_at)
	o["deleted_at"] = *record.deleted_at;

  json::array arr;
  arr.reserve(allowed_ips.size());
  for (const auto& ip : allowed_ips)
	arr.emplace_back(ip);
  o["allowed_ips"] = std::move(arr);

  return o;
}

PeerConfig::operator json::object() && {
  json::object o;
  o["id"] = record.id;
  o["public_key"] = std::move(record.public_key);
  if (record.preshared_key)
	o["preshared_key"] = std::move(*record.preshared_key);
  o["CIDR"] = std::move(record.cidr);
  o["persistent_keepalive"] = record.persistent_keepalive;
  o["description"] = std::move(record.description);
  o["created_at"] = std::move(record.created_at);
  if (record.updated_at)
	o["updated_at"] = std::move(*record.updated_at);
  if (record.deleted_at)
	o["deleted_at"] = std::move(*record.deleted_at);

  json::array arr;
  arr.reserve(allowed_ips.size());
  for (const auto& ip : allowed_ips)
	arr.emplace_back(ip);
  o["allowed_ips"] = std::move(arr);

  return o;
}

}  // namespace WgForge
