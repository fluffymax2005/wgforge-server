/*
 * cli_controller.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#include "cli_controller.h"

#include <stdexcept>

namespace WgForge {

CliController::CliController(std::shared_ptr<IPeerManager> peer_mgr,
                             std::shared_ptr<IInterfaceManager> iface_mgr,
                             std::shared_ptr<IServerManager> srv_mgr,
                             std::shared_ptr<IDataProcessor> data_prs,
                             std::shared_ptr<IDeployManager> deploy_mgr, std::ostream& os)
    : os_(os) {
  if (peer_mgr == nullptr)
	throw std::invalid_argument("Peer manager is nullptr");
  if (iface_mgr == nullptr)
	throw std::invalid_argument("Interface manager is nullptr");
  if (srv_mgr == nullptr)
	throw std::invalid_argument("Server manager is nullptr");
  if (data_prs == nullptr)
	throw std::invalid_argument("Data processor is nullptr");
  if (deploy_mgr == nullptr)
	throw std::invalid_argument("Deploy manager is nullptr");

  peer_mgr_ = std::move(peer_mgr);
  iface_mgr_ = std::move(iface_mgr);
  srv_mgr_ = std::move(srv_mgr);
  data_prs_ = std::move(data_prs);
  deploy_mgr_ = std::move(deploy_mgr);
}

void CliController::getPeer(const GetPeerRequest& req) {
  auto peer_opt = peer_mgr_->getByPublicKey(req);
  if (!peer_opt) {
	os_ << ENDLINE << "Peer not found" << ENDLINE;
	return;
  }

  auto peer = *peer_opt;
  os_ << ENDLINE << data_prs_->peersToString(std::vector{peer}, req.display, req.short_keys)
      << ENDLINE << ENDLINE;
}

void CliController::getAllPeers(const GetAllPeerRequest& req) {
  auto peers_opt = peer_mgr_->getAll(req);
  if (!peers_opt) {
	os_ << ENDLINE << "Peers not found" << ENDLINE << ENDLINE;
	return;
  }

  auto peers = *peers_opt;
  os_ << ENDLINE << data_prs_->peersToString(peers, req.display, req.short_keys) << ENDLINE
      << ENDLINE;
}

void CliController::createPeer(const CreatePeerRequest& req) {
  peer_mgr_->create(req);
}

void CliController::updatePeer(const UpdatePeerRequest& req) {
  peer_mgr_->updateByPublicKey(req);
  os_ << ENDLINE << "Peer updated successfully" << ENDLINE << ENDLINE;
}

void CliController::deletePeer(const DeletePeerRequest& req) {
  peer_mgr_->deleteByPublicKey(req);
  os_ << ENDLINE << "Peer is deleted" << ENDLINE << ENDLINE;
}

void CliController::getPeerConfig(const GetPeerConfigRequest& req) {
  auto config = peer_mgr_->getConfig(req);
  if (config.length() == 0) {
	os_ << ENDLINE << "Peer not found" << ENDLINE << ENDLINE;
	return;
  }

  os_ << ENDLINE << config << ENDLINE << ENDLINE;
}

void CliController::rotatePeerPublicKey(const RotatePeerPublicKeyRequest& req) {
  auto res = peer_mgr_->rotatePublicKey(req);
  os_ << ENDLINE << data_prs_->rotatedPeerKeyToString(res) << ENDLINE << ENDLINE;
}

void CliController::getInterfaceConfig(const GetInterfaceConfigRequest& req) {
  auto config = iface_mgr_->getConfig(req);
  os_ << ENDLINE << data_prs_->interfaceToString(config /*, req.display*/) << ENDLINE << ENDLINE;
}

void CliController::updateInterfaceConfig(const UpdateInterfaceRequest& req) {
  iface_mgr_->updateConfig(req);
  os_ << ENDLINE << "Interface config updated successfully" << ENDLINE << ENDLINE;
}

void CliController::getServerConfig(const GetServerConfigRequest& req) {
  auto config = srv_mgr_->getConfig(req.iface);
  os_ << ENDLINE << data_prs_->serverToString(config /*, req.display*/) << ENDLINE << ENDLINE;
}

void CliController::updateServerConfig(const UpdateServerRequest& req) {
  srv_mgr_->updateConfig(req);
  os_ << ENDLINE << "Server config updated successfully" << ENDLINE << ENDLINE;
}

void CliController::deployServer(const DeployServerRequest& req) {
  os_ << ENDLINE;
  deploy_mgr_->deploy(req, os_);
  os_ << ENDLINE;
}

void CliController::teardownServer(const TeardownServerRequest& req) {
  os_ << ENDLINE;
  deploy_mgr_->teardown(req, os_);
  os_ << ENDLINE;
}


}  // namespace WgForge
