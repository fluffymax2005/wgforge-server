/*
 * idataprocessor.h
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_IDATAPROCESSOR_H
#define WGFORGE_IDATAPROCESSOR_H

#include <server_config.h>

#include <allowedip.hxx>
#include <interface_config.hpp>
#include <peer.hxx>
#include <string>
#include <vector>

#include "display.h"
#include "rotate_peer_public_key_response.hpp"

namespace WgForge {

class IDataProcessor {
public:
  virtual ~IDataProcessor() = default;

  virtual std::string peersToString(const std::vector<Peer>& peers, Display display,
                                    bool short_keys) const = 0;
  virtual std::string rotatedPeerKeyToString(const RotatePeerPublicKeyResponse& res) const = 0;
  virtual std::string interfaceToString(
      const InterfaceConfig& iface /*, Display display*/) const = 0;
  virtual std::string serverToString(const ServerConfig& server /*, Display display*/) const = 0;
};

}  // namespace WgForge


#endif  // WGFORGE_IDATAPROCESSOR_H
