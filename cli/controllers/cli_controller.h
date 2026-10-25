/*
 * cli_controller.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_CLI_CONTROLLER_H
#define WGFORGE_CLI_CONTROLLER_H

#include <iostream>
#include <ostream>

#include "create_peer_request.hpp"
#include "get_all_peer_request.hpp"
#include "get_interface_config_request.h"
#include "get_peer_config_request.hpp"
#include "get_server_config_request.h"
#include "idataprocessor.h"
#include "iinterfacemanager.h"
#include "ipeermanager.h"
#include "iservermanager.h"
#include "rotate_peer_public_key_request.hpp"

namespace WgForge {

class CliController {
public:
  CliController(std::shared_ptr<IPeerManager> peer_mgr,
                std::shared_ptr<IInterfaceManager> iface_mgr,
                std::shared_ptr<IServerManager> srv_mgr, std::shared_ptr<IDataProcessor> data_prs,
                std::ostream& os = std::cout);

  void getPeer(const GetPeerRequest& req);
  void getAllPeers(const GetAllPeerRequest& req);
  void createPeer(const CreatePeerRequest& req);
  void updatePeer(const UpdatePeerRequest& req);
  void deletePeer(const DeletePeerRequest& req);

  void getPeerConfig(const GetPeerConfigRequest& req);
  void rotatePeerPublicKey(const RotatePeerPublicKeyRequest& req);

  void getInterfaceConfig(const GetInterfaceConfigRequest& req);
  void updateInterfaceConfig(const UpdateInterfaceRequest& req);

  void getServerConfig(const GetServerConfigRequest& req);
  void updateServerConfig(const UpdateServerRequest& req);

private:
  std::shared_ptr<IPeerManager> peer_mgr_;
  std::shared_ptr<IInterfaceManager> iface_mgr_;
  std::shared_ptr<IServerManager> srv_mgr_;
  std::shared_ptr<IDataProcessor> data_prs_;
  std::ostream& os_;
};

}  // namespace WgForge

#endif  // WGFORGE_CLI_CONTROLLER_H
