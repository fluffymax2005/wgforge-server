/*
 * deploy_manager.cpp
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

#include "deploy_manager.h"

#include <filesystem>
#include <memory>

#include "docker_api.h"
#include "spinner.hpp"
#include "ssh_tunnel.h"

namespace WgForge {

namespace fs = std::filesystem;

DeployManager::DeployManager(std::shared_ptr<IDockerApi> docker_api) {
  if (docker_api == nullptr)
	throw std::invalid_argument("Docker API is nullptr");

  docker_api_ = std::move(docker_api);
}

void DeployManager::deploy(const DeployServerRequest& req, std::ostream& os) {
  std::unique_ptr<SshTunnel> tunnel;
  auto active_api = docker_api_;

  auto tunnel_socket = (fs::temp_directory_path() / "wgforge_ssh_docker.sock").string();

  // If SSH host pointed explicitly then forward socket through SSH
  if (!req.ssh_host.empty()) {
	tunnel = std::make_unique<SshTunnel>(req.ssh_host, req.ssh_port, req.ssh_key, req.ssh_password);
	tunnel->start(tunnel_socket, os);

	// Forward DockerApi into forwarded socket via SSH
	active_api = std::make_shared<DockerApi>(tunnel_socket);
  }

  Spinner::run(os, "Checking Docker Daemon connection", [&] { active_api->ping(); });

  Spinner::run(os, std::format("Pulling Docker image \"{}\"", req.image),
               [&] { active_api->pullImage(req.image); });

  // Remove old container before creating new one. Suppress any exceptions
  try {
	Spinner::run(
	    os, std::format("Removing old running container \"{}\" if presented", req.container_name),
	    [&] { active_api->stopAndRemoveContainer(req.container_name); });
  } catch (...) {
  }

  Spinner::run(os, std::format("Creating Docker container \"{}\"", req.container_name),
               [&] { active_api->createContainer(req); });

  Spinner::run(os, std::format("Starting Docker container \"{}\"", req.container_name),
               [&] { active_api->startContainer(req.container_name); });

  std::println(os, "\n==> {}Deployment completed succesfully{} <==", WGFORGE_CORE_GREEN_COLOR,
               WGFORGE_CORE_CLEAR_FORMATTING);

  if (tunnel)
	tunnel->stop();
}

void DeployManager::teardown(const TeardownServerRequest& req, std::ostream& os) {
  std::unique_ptr<SshTunnel> tunnel;
  auto active_api = docker_api_;
  auto tunnel_socket = (fs::temp_directory_path() / "wgforge_ssh_docker.sock").string();

  // If SSH host pointed explicitly then forward socket through SSH
  if (!req.ssh_host.empty()) {
	tunnel = std::make_unique<SshTunnel>(req.ssh_host, req.ssh_port, req.ssh_key, req.ssh_password);
	tunnel->start(tunnel_socket, os);

	// Forward DockerApi into forwarded socket via SSH
	active_api = std::make_shared<DockerApi>(tunnel_socket);
  }

  Spinner::run(os, std::format("Stopping and removing container \"{}\"", req.container_name),
               [&] { active_api->stopAndRemoveContainer(req.container_name); });

  std::println(os, "\n==> {}Tearing down completed succesfully{} <==", WGFORGE_CORE_GREEN_COLOR,
               WGFORGE_CORE_CLEAR_FORMATTING);

  if (tunnel)
	tunnel->stop();
}


}  // namespace WgForge
