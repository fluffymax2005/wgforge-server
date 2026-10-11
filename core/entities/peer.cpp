/*
 * peer.cpp
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

#include "peer.hxx"

namespace WgForge {

Peer::operator json::object() const& {
  json::object o;

  o["id"] = id;
  o["publicKey"] = public_key;
  o["presharedKey"] = preshared_key.value_or(std::string());
  o["status"] = static_cast<int>(status);
  o["tunnelAddress"] = tunnel_address;
  o["persistentKeepalive"] = persistent_keepalive;
  o["description"] = description;
  o["createdAt"] = created_at;
  o["updatedAt"] = updated_at.value_or(std::string());
  o["deletedAt"] = deleted_at.value_or(std::string());

  return o;
}

Peer::operator json::object() && {
  json::object o;

  o["id"] = id;
  o["publicKey"] = std::move(public_key);
  o["presharedKey"] = std::move(preshared_key.value_or(std::string()));
  o["status"] = static_cast<int>(status);
  o["tunnelAddress"] = std::move(tunnel_address);
  o["persistentKeepalive"] = persistent_keepalive;
  o["description"] = std::move(description);
  o["createdAt"] = std::move(created_at);
  o["updatedAt"] = std::move(updated_at.value_or(std::string()));
  o["deletedAt"] = std::move(deleted_at.value_or(std::string()));

  return o;
}

}  // namespace WgForge
