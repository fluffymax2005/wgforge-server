/*
 * allowedip.h
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

#ifndef __WGFORGE_ALLOWEDIP_H__
#define __WGFORGE_ALLOWEDIP_H__

#include <boost/json.hpp>
#include <optional>
#include <string>

namespace WgForge {

namespace json = boost::json;

/**
 * @brief AllowedIPs record database table represention.
 */
#pragma db object table("AllowedIPs")

struct AllowedIP {
#pragma db id auto column("Id")
  int64_t id;

#pragma db column("Peer_id") not_null options("REFERENCES Peers(Id) ON DELETE CASCADE")
  int64_t peer_id;

#pragma db column("CIDR") type("TEXT")
  std::string cidr;

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

#endif  // __WGFORGE_ALLOWEDIP_H__
