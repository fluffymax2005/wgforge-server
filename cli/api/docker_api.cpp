/*
 * docker_api.cpp
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

#include "docker_api.h"

#include <boost/json.hpp>
#include <format>
#include <iostream>

namespace WgForge {

namespace json = boost::json;
using stream_protocol = boost::asio::local::stream_protocol;

namespace {
std::string extractDockerError(std::string_view body) {
  try {
	auto obj = boost::json::parse(body).as_object();
	if (obj.contains("message"))
	  return std::string(obj["message"].as_string());
  } catch (...) {
  }

  throw std::runtime_error(std::format("Unknown Docker error: {}", body));
}
}  // namespace

DockerApi::DockerApi(std::string socket_path) : socket_path_(std::move(socket_path)) {
}

template<typename RequestBody>
http::response<http::string_body> DockerApi::sendRequest(http::request<RequestBody>& req) {
  boost::asio::io_context ioc;
  stream_protocol::socket socket(ioc);
  socket.connect(stream_protocol::endpoint(socket_path_));

  req.set(http::field::host, "localhost");
  req.set(http::field::user_agent, "wgforge-cli");

  auto handle_op = [](const boost::system::error_code& ec) -> void {
	if (ec)
	  throw std::runtime_error(
	      std::format("Failed to proceed request to Docker socket: {}. Beast error code: {}",
		              ec.message(), ec.value()));
  };

  boost::system::error_code ec;
  http::write(socket, req, ec);
  handle_op(ec);


  beast::flat_buffer buffer;
  http::response<http::string_body> res;
  http::read(socket, buffer, res, ec);
  handle_op(ec);

  socket.shutdown(stream_protocol::socket::shutdown_both);
  return res;
}

void DockerApi::ping() {
  http::request<http::empty_body> req{http::verb::get, "/_ping", 11};
  auto res = sendRequest(req);
  if (res.result() == http::status::internal_server_error) {
	throw std::runtime_error(std::format("Failed to test Docker server is accessible. Reason: {}",
	                                     extractDockerError(res.body())));
  }
}

void DockerApi::pullImage(const std::string& image) {
  const std::string target = std::format("/images/create?fromImage={}", image);
  http::request<http::empty_body> req{http::verb::post, target, 11};

  auto res = sendRequest(req);
  if (res.result() == http::status::not_found)
	throw std::runtime_error("Target image is not found");
  if (res.result() == http::status::internal_server_error)
	throw std::runtime_error(
	    std::format("Failed to pull image: {}. Reason: {}", image, extractDockerError(res.body())));
}

void DockerApi::createContainer(const DeployServerRequest& req) {
  json::object body{
      {"Image",                                           req.image},
      {  "Env", json::array{std::format("INTERFACE={}", req.iface)}}
  };

  json::object host_config{
      {"NetworkMode",                                                                        "host"},
      {     "CapAdd",                                                      json::array{"NET_ADMIN"}},
      {      "Binds", json::array{std::format("{}:/etc/wgforge", req.config_path),
 std::format("{}:/var/lib/wgforge", req.data_path)}                           }
  };

  json::object restart_policy{
      {"Name", "unless-stopped"}
  };
  host_config["RestartPolicy"] = std::move(restart_policy);

  body["HostConfig"] = std::move(host_config);

  const std::string target = std::format("/containers/create?name={}", req.container_name);
  http::request<http::string_body> http_req{http::verb::post, target, 11};
  http_req.set(http::field::content_type, "application/json");
  http_req.body() = json::serialize(body);
  http_req.prepare_payload();

  auto res = sendRequest(http_req);
  if (res.result() == http::status::bad_request)
	throw std::invalid_argument(std::format("Failed to create container \"{}\". Bad parameter: {}",
	                                        req.container_name, extractDockerError(res.body())));
  if (res.result() == http::status::not_found)
	throw std::runtime_error(std::format("Failed to create container \"{}\". No such image: {}",
	                                     req.container_name, req.image));
  if (res.result() == http::status::conflict)
	throw std::invalid_argument(std::format("Failed to create container \"{}\". Confict: {}",
	                                        req.container_name, extractDockerError(res.body())));
  if (res.result() == http::status::internal_server_error)
	throw std::invalid_argument(std::format("Failed to create container \"{}\". Reason: {}",
	                                        req.container_name, extractDockerError(res.body())));
}

void DockerApi::startContainer(const std::string& container_name) {
  const std::string target = std::format("/containers/{}/start", container_name);
  http::request<http::empty_body> req{http::verb::post, target, 11};

  auto res = sendRequest(req);
  if (res.result() == http::status::not_modified)
	throw std::runtime_error(std::format("Container \"{}\" already started", container_name));
  if (res.result() == http::status::bad_request ||
      res.result() == http::status::internal_server_error)
	throw std::runtime_error(std::format("Failed to start container \"{}\". Reason: {}",
	                                     container_name, extractDockerError(res.body())));
}

void DockerApi::stopAndRemoveContainer(const std::string& container_name) {
  const std::string target_stop = std::format("/containers/{}/stop", container_name);
  http::request<http::empty_body> stop_req{http::verb::post, target_stop, 11};

  auto res = sendRequest(stop_req);
  if (res.result() == http::status::not_modified)
	throw std::runtime_error(std::format("Container \"{}\" already stopped", container_name));
  if (res.result() == http::status::internal_server_error)
	throw std::runtime_error(std::format("Failed to stop container \"{}\". Reason: {}",
	                                     container_name, extractDockerError(res.body())));
  if (res.result() == http::status::not_found)
	throw std::runtime_error(std::format("No such container found: {}", container_name));

  const std::string target_rm = std::format("/containers/{}?v=true", container_name);
  http::request<http::empty_body> rm_req{http::verb::delete_, target_rm, 11};

  res = sendRequest(rm_req);
  if (res.result() == http::status::bad_request ||
      res.result() == http::status::internal_server_error || res.result() == http::status::conflict)
	throw std::runtime_error(

	    std::format("Failed to remove container \"{}\". Reason: {}", container_name,
		            extractDockerError(res.body())));
  if (res.result() == http::status::not_found)
	throw std::runtime_error(std::format("No such container found: {}", container_name));
}

}  // namespace WgForge
