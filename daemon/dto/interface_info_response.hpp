/*
 * interface_info_response.hpp
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

#ifndef __WGFORGE_INTERFACE_INFO_RESPONSE_HPP__
#define __WGFORGE_INTERFACE_INFO_RESPONSE_HPP__

#include <netinet/in.h>

#include <boost/json.hpp>
#include <string>

#include "create_peer_response.hpp"
#include "defines.h"

namespace WgForge {

struct InterfaceInfoResponse {
  std::string iface_name;
  bool is_up;
  in_port_t listen_port;
  std::string address;
  std::string public_key;
  std::string private_key;
};

inline void tag_invoke(json::value_from_tag, json::value& jv, const InterfaceInfoResponse& res) {
  jv = json::object{
      { "ifaceName",  res.iface_name},
      {      "isUp",       res.is_up},
      {"listenPort", res.listen_port},
      {   "address",     res.address}

      ,
      { "publicKey",  res.public_key},
      {"privateKey", res.private_key}
  };
}

}  // namespace WgForge

#endif  // __WGFORGE_INTERFACE_INFO_RESPONSE_H__
