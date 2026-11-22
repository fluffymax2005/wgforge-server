/*
 * peer_service.cpp
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

#include "peer_service.h"

#include <allowedip-odb.hxx>
#include <iomanip>
#include <peer-odb.hxx>
#include <sstream>
#include <string>
#include <wgpresharedkey.hpp>
#include <wgpublickey.hpp>

#include "endpoint_invalid_argument.h"
#include "endpoint_notfound.h"
#include "inetutils.h"
#include "peer_config_request.hpp"
#include "wgforge.h"

namespace WgForge {

PeerService::PeerService(std::shared_ptr<odb::database> context, std::shared_ptr<WgInterface> iface,
                         std::shared_ptr<ConfigManager> config_mgr, std::shared_ptr<ILogger> logger,
                         std::shared_ptr<IConfiguration> configuration) {
  if (context == nullptr)
	throw std::invalid_argument("Database context is nullptr");
  if (iface == nullptr)
	throw std::invalid_argument("Interface is nullptr");
  if (config_mgr == nullptr)
	throw std::invalid_argument("Config manager is nullptr");
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");
  if (configuration == nullptr)
	throw std::invalid_argument("Configuration is nullptr");

  context_ = std::move(context);
  iface_ = std::move(iface);
  config_mgr_ = std::move(config_mgr);
  logger_ = std::move(logger);
  configuration_ = std::move(configuration);
}

std::future<std::optional<Peer>> PeerService::getPeerAsync(const PeerService::Key& key) const {
  return std::async(std::launch::async, [this, key] -> std::optional<Peer> {
	odb::transaction t(context_->begin());
	auto result = context_->query<Peer>(odb::query<Peer>::id == key);
	if (result.empty())
	  return std::nullopt;
	Peer p = *result.begin();

	auto ips = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == p.id);
	if (!ips.empty()) {
	  for (const auto& ip : ips) {
		p.allowed_ips->push_back(ip.cidr);
	  }
	}

	t.commit();
	return p;
  });
}

std::future<std::optional<Peer>> PeerService::getPeerByPublicKey(std::string_view key) const {
  return std::async(std::launch::async, [this, key] -> std::optional<Peer> {
	odb::transaction t(context_->begin());
	auto result = context_->query<Peer>(odb::query<Peer>::public_key == std::string(key));
	if (result.empty())
	  return std::nullopt;
	Peer p = *result.begin();

	auto ips = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == p.id, true);
	if (!p.allowed_ips.has_value())
	  p.allowed_ips.emplace();
	for (const auto& ip : ips) {
	  p.allowed_ips->push_back(ip.cidr);
	}

	t.commit();
	return p;
  });
}

std::future<std::optional<std::vector<Peer>>> PeerService::getAllAsync() const {
  return std::async(std::launch::async, [this] -> std::optional<std::vector<Peer>> {
	odb::transaction t(context_->begin());
	auto result = context_->query<Peer>();
	if (result.empty())
	  return std::nullopt;
	std::vector<Peer> v(result.begin(), result.end());

	for (auto& p : v) {
	  auto ips = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == p.id, true);
	  if (!p.allowed_ips.has_value())
		p.allowed_ips.emplace();
	  for (const auto& ip : ips) {
		p.allowed_ips->push_back(ip.cidr);
	  }
	}

	t.commit();
	return v;
  });
}

std::future<CreatePeerResponse> PeerService::createPeerAsync(const CreatePeerRequest& req) {
  auto copy = req;

  return std::async(
      std::launch::async, [this, copy = std::move(copy)]() mutable -> CreatePeerResponse {
	    CreatePeerResponse res;
	    odb::transaction t(context_->begin());

	    Peer peer;
	    WgPrivateKey private_key;
	    WgPublicKey public_key(private_key);

	    while (iface_->hasPeerWithPublicKey(public_key)) {
	      private_key = WgPrivateKey();
	      public_key = WgPublicKey(private_key);
	    }

	    peer.public_key = public_key.toB64String();
	    res.public_key = public_key.toB64String();

	    logger_->info(
	        std::format("Started creating peer with public key: {}", public_key.toB64String()));
	    try {
	      WgPeer wg_peer;

	      wg_peer.setPublicKey(std::move(public_key));
	      if (copy.generate_preshared_key) {
		    WgPresharedKey preshared_key;
		    peer.preshared_key = preshared_key.toB64String();
		    wg_peer.setPresharedKey(std::move(preshared_key));
	      }
	      wg_peer.setPersistentKeepAlive(copy.persistent_keepalive);
	      peer.persistent_keepalive = copy.persistent_keepalive;

	      const auto& gateway_address = configuration_->getSection("Interface")["Address"];
	      auto tunnel_addresses_query =
	          context_->query<Peer>((odb::query<Peer>::status == Peer::Status::Active ||
			                         odb::query<Peer>::status == Peer::Status::Suspended) &&
			                        odb::query<Peer>::deleted_at.is_null());
	      std::vector<std::string> tunnel_addresses;
	      tunnel_addresses.reserve(tunnel_addresses.size());
	      for (const auto& peer : tunnel_addresses_query)
		    tunnel_addresses.push_back(peer.tunnel_address);

	      auto tunnel_address =
	          InetUtils::findFreeClientTunnelCIDR(gateway_address, tunnel_addresses);
	      if (tunnel_address.empty())
		    throw EndpointInvalidArgument("Network capacity out of range");

	      wg_peer.addAllowedIP(WgAllowedIP(tunnel_address));
	      peer.tunnel_address = std::move(tunnel_address);

	      iface_->addPeer(std::move(wg_peer));
	    } catch (const std::exception& e) {
	      throw std::runtime_error(e.what());
	    }

	    // Fill peer
	    peer.id = 0;
	    peer.status = Peer::Status::Active;
	    peer.created_at = getCurrentUtcTimestamp();
	    peer.updated_at = std::nullopt;
	    peer.deleted_at = std::nullopt;

	    auto new_peer_id = context_->persist(peer);
	    peer.id = new_peer_id;

	    // Fill allowedip
	    for (const auto& allowedip : copy.allowed_ips) {
	      AllowedIP ip{};
	      ip.peer_id = new_peer_id;
	      ip.cidr = allowedip;
	      ip.created_at = getCurrentUtcTimestamp();
	      context_->persist(ip);
	    }

	    t.commit();

	    res.private_key = private_key.toB64String();
	    res.preshared_key = copy.generate_preshared_key ? *peer.preshared_key : "";
	    res.tunnel_address = peer.tunnel_address;

	    return res;
      });
}

std::future<void> PeerService::updateAsync(const UpdatePeerRequest& req) {
  auto copy = req;
  return std::async(std::launch::async, [this, copy = std::move(copy)] {
	if (!copy.remove_allowedips && !copy.add_allowedips && !copy.persistent_keepalive &&
	    !copy.description && !copy.preshared_key && !copy.status)
	  return;

	odb::transaction t(context_->begin());
	auto peer_query = context_->query<Peer>(odb::query<Peer>::public_key == copy.public_key);
	if (peer_query.empty())
	  throw EndpointNotFound("Peer not found");
	auto peer = *peer_query.begin();

	if (copy.status)
	  peer.status = copy.status.value();
	if (copy.preshared_key)
	  peer.preshared_key = std::move(copy.preshared_key.value());
	if (copy.description)
	  peer.description = std::move(copy.description.value());
	if (copy.persistent_keepalive)
	  peer.persistent_keepalive = std::move(copy.persistent_keepalive.value());
	peer.updated_at = getCurrentUtcTimestamp();
	context_->persist(peer);

	// Remove allowed ips
	for (const auto allowedip : copy.remove_allowedips.value_or({})) {
	  auto allowedip_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id &&
	                                                    odb::query<AllowedIP>::cidr == allowedip);
	  if (allowedip_query.empty())
		throw EndpointNotFound(std::format("AllowedIP not found: {}", allowedip));

	  auto ip = *allowedip_query.begin();
	  if (!ip.deleted_at) {
		ip.deleted_at = getCurrentUtcTimestamp();
		ip.updated_at = getCurrentUtcTimestamp();
		context_->update(ip);
	  }
	}

	// Add allowed ips
	for (const auto& allowedip : copy.add_allowedips.value_or({})) {
	  auto allowedip_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id);
	  if (allowedip_query.empty()) {
		AllowedIP ip{};
		ip.peer_id = peer.id;
		ip.cidr = allowedip;
		ip.created_at = getCurrentUtcTimestamp();
		context_->persist(ip);
	  } else {
		auto ip = *allowedip_query.begin();
		if (ip.deleted_at) {
		  ip.deleted_at = std::nullopt;
		  ip.updated_at = getCurrentUtcTimestamp();
		  context_->update(ip);
		}
	  }
	}

	t.commit();
  });
}

std::future<void> PeerService::softDeleteAsync(const Key& key) {
  return std::async(std::launch::async, [this, key] -> void {
	odb::transaction t(context_->begin());

	auto result = context_->query<Peer>(odb::query<Peer>::id == key);
	if (result.empty())
	  throw EndpointNotFound("Peer not found");

	auto peer = *result.begin();
	peer.status = Peer::Status::Suspended;
	peer.updated_at = getCurrentUtcTimestamp();
	peer.deleted_at = getCurrentUtcTimestamp();

	logger_->warn(std::format("Soft deleting peer with public key initiated: {}", peer.public_key));

	context_->update(peer);

	try {
	  iface_->removePeer(WgPublicKey(peer.public_key));
	} catch (const std::exception& e) {
	  logger_->error(
	      std::format("Failed to apply soft deleting peer with public key", peer.public_key));
	  throw std::runtime_error(e.what());
	}

	t.commit();
  });
}

std::future<void> PeerService::hardDeleteAsync(const Key& key) {
  return std::async(std::launch::async, [this, key] -> void {
	odb::transaction t(context_->begin());

	auto result = context_->query<Peer>(odb::query<Peer>::id == key);
	if (result.empty())
	  throw EndpointNotFound("Peer not found by provided key");

	logger_->warn(std::format("Hard deleting peer with public key initiated: {}",
	                          (*result.begin()).public_key));

	context_->erase(*result.begin());
	t.commit();
  });
}

std::future<std::optional<AllowedIP>> PeerService::getAllowedIP(const Key& key) const {
  return std::async(std::launch::async, [this, key] -> std::optional<AllowedIP> {
	odb::transaction t(context_->begin());

	auto result = context_->query<AllowedIP>(odb::query<AllowedIP>::id == key);
	if (result.empty())
	  return std::nullopt;
	auto allowedip = *result.begin();
	t.commit();
	return allowedip;
  });
}

std::future<std::optional<std::vector<AllowedIP>>> PeerService::getAllAllowedIPs(
    const Key& key) const {
  return std::async(std::launch::async, [this, key] -> std::optional<std::vector<AllowedIP>> {
	odb::transaction t(context_->begin());

	auto peer_query = context_->query<Peer>(odb::query<Peer>::id == key);
	if (peer_query.empty())
	  return std::nullopt;

	auto peer = *peer_query.begin();
	auto allowedips_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id);
	if (allowedips_query.empty())
	  return std::nullopt;

	std::vector v(allowedips_query.begin(), allowedips_query.end());
	t.commit();
	return v;
  });
}

std::future<void> PeerService::addAllowedIPAsync(const Key& key, std::string_view allowedip) {
  if (!WgAllowedIP::validate(std::string(allowedip)))
	throw EndpointInvalidArgument("Invalid AllowedIP");

  return std::async(std::launch::async, [this, allowedip, key] -> void {
	odb::transaction t(context_->begin());

	auto peer_query = context_->query<Peer>(odb::query<Peer>::id == key);
	if (peer_query.empty())
	  throw EndpointNotFound("Peer not found");

	logger_->info(std::format("Adding allowed ip \"{}\" for peer with public key initiated: {}",
	                          allowedip, (*peer_query.begin()).public_key));

	auto peer = *peer_query.begin();
	auto allowedip_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id);
	if (peer.status == Peer::Status::Active) {
	  if (allowedip_query.empty()) {
		AllowedIP ip{};
		ip.peer_id = peer.id;
		ip.cidr = allowedip;
		ip.created_at = getCurrentUtcTimestamp();
		context_->persist(ip);
	  } else {
		auto ip = *allowedip_query.begin();
		if (ip.deleted_at) {
		  ip.deleted_at = std::nullopt;
		  ip.updated_at = getCurrentUtcTimestamp();
		  context_->update(ip);
		}
	  }
	}

	t.commit();
  });
}

std::future<void> PeerService::addAllowedIPsAsync(const Key& key,
                                                  const std::vector<std::string>& allowedips) {
  if (allowedips.empty())
	throw EndpointInvalidArgument("No AllowedIPs provided");
  for (const auto& allowedip : allowedips) {
	if (!WgAllowedIP::validate(allowedip.c_str()))
	  throw EndpointInvalidArgument(std::format("Invalid AllowedIP: {}", allowedip));
  }

  return std::async(std::launch::async, [this, allowedips, key] -> void {
	odb::transaction t(context_->begin());

	auto peer_query = context_->query<Peer>(odb::query<Peer>::id == key);
	if (peer_query.empty())
	  throw EndpointNotFound("Peer not found");

	auto peer = *peer_query.begin();

	logger_->info(
	    std::format("Adding allowed ips for peer with public key initiated: {}", peer.public_key));

	for (const auto& allowedip : allowedips) {
	  auto allowedip_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id);
	  if (peer.status == Peer::Status::Active) {
		if (allowedip_query.empty()) {
		  AllowedIP ip{};
		  ip.peer_id = peer.id;
		  ip.cidr = allowedip;
		  ip.created_at = getCurrentUtcTimestamp();
		  context_->persist(ip);
		} else {
		  auto ip = *allowedip_query.begin();
		  if (ip.deleted_at) {
			ip.deleted_at = std::nullopt;
			ip.updated_at = getCurrentUtcTimestamp();
			context_->update(ip);
		  }
		}
	  }
	}

	t.commit();
  });
}

std::future<void> PeerService::softDeleteAllowedIPAsync(const Key& key,
                                                        std::string_view allowedip) {
  if (!WgAllowedIP::validate(std::string(allowedip)))
	throw EndpointInvalidArgument("Invalid AllowedIP");

  return std::async(std::launch::async, [this, allowedip, key] -> void {
	odb::transaction t(context_->begin());

	auto peer_query = context_->query<Peer>(odb::query<Peer>::id == key);
	if (peer_query.empty())
	  throw EndpointNotFound("Peer not found");

	auto peer = *peer_query.begin();

	logger_->info(std::format("Removing allowed ip \"{}\" for peer with public key initiated: {}",
	                          allowedip, peer.public_key));

	auto allowedip_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id);
	if (allowedip_query.empty())
	  throw EndpointNotFound("AllowedIP not found");
	if (peer.status == Peer::Status::Active) {
	  auto ip = *allowedip_query.begin();
	  if (!ip.deleted_at) {
		ip.deleted_at = getCurrentUtcTimestamp();
		ip.updated_at = getCurrentUtcTimestamp();
		context_->update(ip);
	  }
	}

	t.commit();
  });
}

std::future<void> PeerService::softDeleteAllowedIPsAsync(
    const Key& key, const std::vector<std::string>& allowedips) {
  if (allowedips.empty())
	throw EndpointInvalidArgument("No AllowedIPs provided");
  for (const auto& allowedip : allowedips) {
	if (!WgAllowedIP::validate(allowedip.data()))
	  throw EndpointInvalidArgument("Invalid AllowedIP");
  }

  return std::async(std::launch::async, [this, allowedips, key] -> void {
	odb::transaction t(context_->begin());

	auto peer_query = context_->query<Peer>(odb::query<Peer>::id == key);
	if (peer_query.empty())
	  throw EndpointNotFound("Peer not found");

	auto peer = *peer_query.begin();

	logger_->info(std::format("Removing allowed ips for peer with public key initiated: {}",
	                          peer.public_key));

	for (const auto allowedip : allowedips) {
	  auto allowedip_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id &&
	                                                    odb::query<AllowedIP>::cidr == allowedip);
	  if (allowedip_query.empty())
		throw EndpointNotFound("AllowedIP not found");
	  if (peer.status == Peer::Status::Active) {
		auto ip = *allowedip_query.begin();
		if (ip.deleted_at) {
		  ip.deleted_at = getCurrentUtcTimestamp();
		  ip.updated_at = getCurrentUtcTimestamp();
		  context_->update(ip);
		}
	  }
	}

	t.commit();
  });
}

std::future<RotatePeerPublicKeyResponse> PeerService::rotatePublicKeyAsync(
    const RotatePeerPublicKeyRequest& req) {
  auto copy = req;
  return std::async(
      std::launch::async, [this, copy = std::move(copy)] -> RotatePeerPublicKeyResponse {
	    odb::transaction t(context_->begin());

	    auto peer_query = context_->query<Peer>(odb::query<Peer>::public_key == copy.old_key);
	    if (peer_query.empty())
	      throw EndpointNotFound("Peer not found");

	    auto peer = *peer_query.begin();

	    WgPrivateKey private_key;
	    WgPublicKey public_key(std::move(private_key));
	    while (iface_->hasPeerWithPublicKey(public_key)) {
	      private_key.generate();
	      public_key = WgPublicKey(private_key);
	    }

	    peer.public_key = public_key.toB64String();
	    peer.updated_at = getCurrentUtcTimestamp();
	    context_->update(peer);

	    t.commit();

	    RotatePeerPublicKeyResponse res{.private_key = private_key.toB64String(),
		                                .public_key = public_key.toB64String()};

	    return res;
      });
}

std::future<std::string> PeerService::getPeerConfigAsync(const PeerConfigRequest& req) {
  auto copy = req;
  return std::async(std::launch::async, [this, copy = std::move(copy)] -> std::string {
	odb::transaction t(context_->begin());
	auto peer_query = context_->query<Peer>(odb::query<Peer>::public_key == copy.public_key);
	if (peer_query.empty())
	  throw EndpointNotFound("Peer not found");
	auto peer = *peer_query.begin();

	auto allowedips_query = context_->query<AllowedIP>(odb::query<AllowedIP>::peer_id == peer.id);
	std::vector allowedips(allowedips_query.begin(), allowedips_query.end());

	std::ostringstream oss;
	oss << "# Wireguard Client configuration\n"
	    << "# Provided by WgForge\n\n"
	    << "[Interface]\n"
	    << "# Client Private key - keep this SECRET\n"
	    << "PrivateKey=<PRIVATE_KEY>\n\n"
	    << "# Client VPN tunnel IP address\n"
	    << "Address=" << peer.tunnel_address << "\n\n"
	    << "# DNS servers to resolve server domain name (if used)\n"
	    << "DNS=1.1.1.1, 8.8.8.8, 1.0.0.1\n\n"
	    << "[Peer]\n"
	    << "# Server Public key\n"
	    << "PublicKey=" << iface_->getPublicKey()->toB64String() << "\n\n"
	    << "# Server Endpoint\n"
	    << "Endpoint="
	    << copy.public_endpoint.value_or(configuration_->getSection("Wireguard")["PublicEndpoint"])
	    << "\n\n"
	    << "# Keepalive time in seconds for NAT\n"
	    << "PersistentKeepalive=" << peer.persistent_keepalive << "\n\n";

	if (peer.preshared_key)
	  oss << "# Preshared key\n"
	      << "PresharedKey=" << peer.preshared_key->c_str() << "\n\n";

	oss << "# Allowed IPs used for routing\n"
	    << "AllowedIPs=";

	for (auto i = 0; i < allowedips.size(); i++) {
	  oss << allowedips[i].cidr;
	  if (i < allowedips.size() - 1)
		oss << ", ";
	}

	return oss.str();
  });
}

std::string PeerService::getCurrentUtcTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

}  // namespace WgForge
