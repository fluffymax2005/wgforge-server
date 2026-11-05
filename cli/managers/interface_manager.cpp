/*
 * interface_manager.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
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

#include "interface_manager.h"

#include <cstring>

namespace WgForge {

InterfaceManager::InterfaceManager(std::shared_ptr<IInterfaceApi> api) {
  if (api == nullptr)
	throw std::invalid_argument("Interface api is nullptr");

  api_ = std::move(api);
}

InterfaceConfig InterfaceManager::getConfig(const GetInterfaceConfigRequest& req) const {
  return api_->get(req);
}

void InterfaceManager::updateConfig(const UpdateInterfaceRequest& req) {
  if (!req.requiresQuery())
	throw std::invalid_argument("No data to update interface provided");

  if (req.address && !WgAllowedIP::validate(*req.address))
	throw std::invalid_argument("Invalid address");
  if ((req.private_key) && !WgPrivateKey::validateB64StringKey(*req.private_key))
	throw std::invalid_argument("Invalid private key");
  api_->update(req);
}

}  // namespace WgForge
