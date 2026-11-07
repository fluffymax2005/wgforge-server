/*
 * docker_api.h
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

#ifndef WGFORGE_CLI_DOCKER_API_H
#define WGFORGE_CLI_DOCKER_API_H

#include <boost/asio/local/stream_protocol.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "idockerapi.h"

namespace WgForge {

namespace beast = boost::beast;
namespace http = beast::http;

class DockerApi : public IDockerApi {
public:
  explicit DockerApi(std::string socket_path = "/var/run/docker.sock");

  virtual void ping() override;
  virtual void pullImage(const std::string& image) override;
  virtual void createContainer(const DeployServerRequest& req) override;
  virtual void startContainer(const std::string& container_name) override;
  virtual void stopAndRemoveContainer(const std::string& container_name) override;

private:
  std::string socket_path_;

  template<typename RequestBody>
  http::response<http::string_body> sendRequest(http::request<RequestBody>& req);
};

}  // namespace WgForge

#endif  // WGFORGE_CLI_DOCKER_API_H
