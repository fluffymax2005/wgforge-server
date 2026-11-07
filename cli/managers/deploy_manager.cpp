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
  std::string tunnel_socket = "/tmp/wgforge_ssh_docker.sock";

  // If SSH host pointed explicitly then forward socket through SSH
  if (!req.ssh_host.empty()) {
	tunnel = std::make_unique<SshTunnel>(req.ssh_host, req.ssh_port, req.ssh_key, req.ssh_password);
	if (tunnel == nullptr)
	  throw std::runtime_error(
	      "Failed to establish SSH connection. Reason: not enough memory to create connection");
	tunnel->start(tunnel_socket, os);

	// Forward DockerApi into forwarded socket via SSH
	active_api = std::make_shared<DockerApi>(tunnel_socket);
	if (active_api == nullptr) {
	  tunnel->stop();
	  fs::remove(tunnel_socket);
	  throw std::runtime_error("Failed to deploy container. Reason: internal program error");
	}
  }

  std::println(os, "Checking Docker Daemon connection...");
  std::println(os);
  active_api->ping();

  std::println(os, "==> Pulling Docker image \"{}\"...", req.image);
  std::println(os);
  active_api->pullImage(req.image);

  // Remove old container before creating new one. Suppress any exceptions
  try {
	active_api->stopAndRemoveContainer(req.container_name);
  } catch (...) {
  }

  std::println(os, "==> Creating Docker container \"{}\"...", req.container_name);
  std::println(os);
  active_api->createContainer(req);

  std::println(os, "==> Starting Docker container \"{}\"...", req.container_name);
  std::println(os);
  active_api->startContainer(req.container_name);

  std::println(os, "==> Deployment completed succesfully <==");

  if (tunnel) {
	tunnel->stop();
	fs::remove(tunnel_socket);
  }
}

void DeployManager::teardown(const TeardownServerRequest& req, std::ostream& os) {
  std::unique_ptr<SshTunnel> tunnel;
  auto active_api = docker_api_;
  std::string tunnel_socket = "/tmp/wgforge_ssh_docker.sock";

  // If SSH host pointed explicitly then forward socket through SSH
  if (!req.ssh_host.empty()) {
	tunnel = std::make_unique<SshTunnel>(req.ssh_host, req.ssh_port, req.ssh_key, req.ssh_password);
	if (tunnel == nullptr)
	  throw std::runtime_error(
	      "Failed to establish SSH connection. Reason: not enough memory to create connection");
	tunnel->start(tunnel_socket, os);

	// Forward DockerApi into forwarded socket via SSH
	active_api = std::make_shared<DockerApi>(tunnel_socket);
	if (active_api == nullptr) {
	  tunnel->stop();
	  fs::remove(tunnel_socket);
	  throw std::runtime_error("Failed to deploy container. Reason: internal program error");
	}
  }

  active_api->stopAndRemoveContainer(req.container_name);

  if (tunnel) {
	tunnel->stop();
	fs::remove(tunnel_socket);
  }
}


}  // namespace WgForge
