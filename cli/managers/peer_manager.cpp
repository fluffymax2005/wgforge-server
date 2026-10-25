/*
 * peer_manager.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
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

#include "peer_manager.h"

#include <format>
#include <stdexcept>

#include "wgforge.h"

namespace WgForge {

PeerManager::PeerManager(std::shared_ptr<IPeerApi> api) {
  if (api == nullptr)
	throw std::invalid_argument("Peer manager api is nullptr");
  api_ = std::move(api);
}

std::optional<std::vector<Peer>> PeerManager::getAll(const GetAllPeerRequest& iface) const {
  return api_->getAll(iface);
}

std::optional<Peer> PeerManager::getByPublicKey(const GetPeerRequest& req) const {
  if (!req.requiresQuery())
	throw std::invalid_argument("No interface or endpoint provided");
  return api_->get(req);
}

CreatePeerResponse PeerManager::create(const CreatePeerRequest& req) {
  if (!req.requiresQuery())
	throw std::invalid_argument("No interface or endpoint provided");
  if (req.allowed_ips.size() == 0)
	throw std::invalid_argument("No allowed ips provided");
  for (const auto& ip : req.allowed_ips)
	if (!WgAllowedIP::validate(ip))
	  throw std::invalid_argument(std::format("Invalid allowed ip: {}", ip));

  return api_->create(req);
}

void PeerManager::updateByPublicKey(const UpdatePeerRequest& req) {
  if (!req.requiresQuery())
	throw std::invalid_argument(
	    "No data for peer update provided either interface or endpoint provided");

  Peer peer{.public_key = req.public_key};
  if (req.preshared_key) {
	if (!WgPresharedKey::validateB64StringKey(*req.preshared_key))
	  throw std::invalid_argument("Invalid preshared key");
	peer.preshared_key = *req.preshared_key;
  }
  if (req.status)
	peer.status = *req.status;
  if (req.description)
	peer.description = *req.description;
  if (req.persistent_keepalive)
	peer.persistent_keepalive = *req.persistent_keepalive;
  if (req.add_allowedips->size()) {
	for (const auto& ip : *req.add_allowedips)
	  if (!WgAllowedIP::validate(ip))
		throw std::invalid_argument(std::format("Invalid allowed ip: {}", ip));
  }
  if (req.remove_allowedips->size()) {
	for (const auto& ip : *req.remove_allowedips)
	  if (!WgAllowedIP::validate(ip))
		throw std::invalid_argument(std::format("Invalid allowed ip: {}", ip));
  }


  api_->update(req);
}

void PeerManager::deleteByPublicKey(const DeletePeerRequest& req) {
  if (!req.requiresQuery())
	throw std::invalid_argument(
	    "No data for peer update provided either interface or endpoint provided");
  if (req.public_key.length() == 0)
	throw std::invalid_argument("Enter public key");
  if (!WgPublicKey::validateB64StringKey(req.public_key))
	throw std::invalid_argument("Invalid public key");
  api_->softDelete(req);
}

RotatePeerPublicKeyResponse PeerManager::rotatePublicKey(const RotatePeerPublicKeyRequest& req) {
  if (!req.requiresQuery())
	throw std::invalid_argument(
	    "No data for peer update provided either interface or endpoint provided");
  if (req.old_key.length() == 0)
	throw std::invalid_argument("Enter old public key");
  if (!WgPublicKey::validateB64StringKey(req.old_key))
	throw std::invalid_argument("Invalid public old public key");
  return api_->rotatePublicKey(req);
}

std::string PeerManager::getConfig(const GetPeerConfigRequest& req) const {
  if (!req.requiresQuery())
	throw std::invalid_argument(
	    "No data for request provided either interface or endpoint provided");
  if (req.public_key.size() == 0)
	throw std::invalid_argument("Enter public key");
  if (!WgPublicKey::validateB64StringKey(req.public_key))
	throw std::invalid_argument("Invalid public key");

  // TODO
  // Implement IP or domain name recognizing
  /*if (req.public_endpoint && !WgEndpoint::validate(*req.public_endpoint))
	throw std::invalid_argument("Invalid public endpoint");*/
  return api_->getConfig(req);
}

}  // namespace WgForge
