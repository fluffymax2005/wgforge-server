/*
 * peer_controller.cpp
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

#include "peer_controller.h"

#include <boost/url/decode.hpp>
#include <boost/url/parse.hpp>

#include "endpoint_notfound.h"
#include "endpoint_runtime_error.h"
#include "ipeerservice.h"

namespace WgForge {

PeerController::PeerController(std::shared_ptr<IPeerServiceAsync> peer_service,
                               std::shared_ptr<ILogger> logger) {
  if (peer_service == nullptr)
	throw std::invalid_argument("Peer service is nullptr");
  if (logger == nullptr)
	throw std::invalid_argument("Logger is nullptr");
  peer_service_ = std::move(peer_service);
  logger_ = std::move(logger);
}

void PeerController::getPeer(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	std::string public_key = boost::urls::decode(key_it->value).value();
	logger_->warn(std::format("Getting peer's record. Public key: {}", public_key));
	if (!WgKey::validateB64StringKey(public_key)) {
	  logger_->error(std::format("Invalid public key \"{}\"", public_key));
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	}

	auto peer = peer_service_->getPeerByPublicKey(public_key).get();
	if (!peer.has_value())
	  throw EndpointNotFound("Peer not found");

	logger_->info(std::format("Got peer's record. Public key: {}", public_key));


	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(json::value_from(peer.value()));
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::getAllPeers(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  logger_->warn("Fetching all peers' records");

  std::exception_ptr ep;
  try {
	json::object j;
	json::array arr;

	auto peers_opt = peer_service_->getAllAsync().get();
	if (!peers_opt.has_value()) {
	  logger_->error("No peers presented for provided configuration");
	  throw EndpointNotFound("Peers not presented");
	}

	j["peers"] = json::value_from(std::move(peers_opt.value()));

	logger_->info("Fetching all peers' records succeeded");

	res.result(http::status::ok);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::createPeer(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::post) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only POST";
	res.prepare_payload();
	return;
  }

  logger_->warn("Creating peer requested");

  std::exception_ptr ep;
  try {
	auto j = json::parse(req.body()).as_object();
	auto record = peer_service_->createPeerAsync(j).get();

	logger_->info(std::format("Created peer. Public key: {}", record.public_key));

	res.result(http::status::created);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(json::value_from(record));
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::deletePeer(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::delete_) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only DELETE";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	std::string public_key = boost::urls::decode(key_it->value).value();
	logger_->warn(std::format("Soft deleting peer requested. Public key: {}", public_key));
	if (!WgKey::validateB64StringKey(public_key)) {
	  logger_->error(std::format("Invalid peer key. Public key: {}", public_key));
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	}

	auto peer = peer_service_->getPeerByPublicKey(public_key).get();
	if (!peer.has_value()) {
	  logger_->error(std::format("Peer not presented. Public key: {}", public_key));
	  throw EndpointNotFound("Peer not found");
	}
	peer_service_->softDeleteAsync(peer.value().id);

	logger_->info(std::format("Soft deleted peer. Public key: {}", public_key));

	res.result(http::status::no_content);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::updatePeer(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only PATCH";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	std::string public_key = boost::urls::decode(key_it->value).value();
	logger_->warn(std::format("Peer update requested. Public key: {}", public_key));
	if (!WgKey::validateB64StringKey(public_key)) {
	  logger_->error(std::format("Invalid public key. Public key: {}", public_key));
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	}

	auto j = json::parse(req.body()).as_object();
	peer_service_->updateAsync(j).get();

	logger_->info(std::format("Peer updated. Public key: {}", public_key));

	res.result(http::status::ok);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::rotatePeerKey(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::patch) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only PATCH";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto old_key_it = params.find("oldPublicKey");
	if (old_key_it == params.end())
	  throw EndpointInvalidArgument("\"oldPublicKey\" not provided");

	std::string old_key = boost::urls::decode(old_key_it->value).value();
	logger_->warn(std::format("Rotating peer's key requested. Public key: {}", old_key));
	if (!WgPublicKey::validateB64StringKey(old_key)) {
	  logger_->error(std::format("Invalid old public key. Public key: {}", old_key));
	  throw EndpointInvalidArgument("Invalid \"oldPublicKey\"");
	}

	auto result =
	    peer_service_->rotatePublicKeyAsync(RotatePeerPublicKeyRequest("", old_key)).get();

	logger_->info(
	    std::format("Rotated old public key: {}. New public key: {}", old_key, result.public_key));

	res.body() = json::serialize(json::value_from(result));
	res.set(http::field::content_type, "application/json");
	res.result(http::status::ok);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::getPeersConfig(RequestConstRef req, ResponseRef res) {
  if (req.method() != http::verb::get) {
	res.result(http::status::method_not_allowed);
	res.body() = "Only GET";
	res.prepare_payload();
	return;
  }

  std::exception_ptr ep;
  try {
	const auto& target = req.target();
	auto r = boost::urls::parse_origin_form(target);
	if (!r.has_value()) {
	  throw EndpointInvalidArgument(
	      "Failed to parse request. Check whether "
	      "public key is used via query");
	}

	auto url = *r;
	boost::urls::params_encoded_view params = url.encoded_params();
	auto key_it = params.find("publicKey");
	if (key_it == params.end())
	  throw EndpointInvalidArgument("\"publicKey\" not provided");

	std::string public_key = boost::urls::decode(key_it->value).value();
	logger_->info(std::format("Fetching peer's config. Public key: {}", public_key));
	if (!WgPublicKey::validateB64StringKey(public_key)) {
	  logger_->error(std::format("Invalid public key. Public key: {}", public_key));
	  throw EndpointInvalidArgument("Invalid \"publicKey\"");
	}


	std::string public_endpoint;
	auto endp_it = params.find("publicEndpoint");
	if (endp_it != params.end()) {
	  public_endpoint = endp_it->value.data();
	}

	json::object j;
	j["config"] =
	    peer_service_->getPeerConfigAsync(PeerConfigRequest(public_key, public_endpoint)).get();
	logger_->info(std::format("Configuring peer's config succeeded. Public key: {}", public_key));

	res.body() = json::serialize(j);
	res.set(http::field::content_type, "application/json");
	res.result(http::status::ok);
  } catch (const std::exception& e) {
	ep = std::current_exception();
  }

  handleException(ep, req, res);

  res.prepare_payload();
}

void PeerController::handleException(std::exception_ptr ep, RequestConstRef req, ResponseRef res) {
  try {
	if (ep)
	  std::rethrow_exception(ep);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const boost::system::system_error& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::runtime_error& e) {
	auto path = std::string(req.target());
	auto method = std::string(req.method_string());
	logger_->error(std::format("{} [{}]: {}", path, method, e.what()));

	res.result(http::status::internal_server_error);
  } catch (const EndpointNotFound& e) {
	json::object o;
	o["error"] = e.what();
	res.result(http::status::not_found);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(o);
  } catch (const std::exception& e) {
	logger_->error(std::format("Unexpected error: {}", e.what()));
	res.result(http::status::internal_server_error);
	res.body() = "Internal server error";
  }
}

}  // namespace WgForge
