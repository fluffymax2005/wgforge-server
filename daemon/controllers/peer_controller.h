/*
 * peer_controller.h
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

#ifndef __WGFORGE_PEER_CONTROLLER_H__
#define __WGFORGE_PEER_CONTROLLER_H__

#include <memory.h>

#include "defines.h"
#include "ilogger.h"
#include "ipeerservice.h"

namespace WgForge {

class PeerController {
public:
  PeerController(std::shared_ptr<IPeerService> peer_service, std::shared_ptr<ILogger> logger);

  void getPeer(Request req, Response res);
  void getAllPeers(Request req, Response res);
  void createPeer(Request req, Response res);
  void updatePeer(Request req, Response res);
  void deletePeer(Request req, Response res);
  void rotatePeerKey(Request req, Response res);
  void getPeersConfig(Request req, Response res);


private:
  std::shared_ptr<IPeerService> peer_service_;
  std::shared_ptr<ILogger> logger_;

  void handleException(std::exception_ptr ep, Response& res);
};

}  // namespace WgForge

#endif  // __WGFORGE_PEER_CONTROLLER_H__
