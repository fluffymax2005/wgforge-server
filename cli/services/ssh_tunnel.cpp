/*
 * ssh_tunnel.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-server. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ssh_tunnel.h"

#include <libssh/libssh.h>

#include <boost/asio.hpp>
#include <filesystem>
#include <iostream>

namespace WgForge {

namespace fs = std::filesystem;
namespace asio = boost::asio;
using stream_protocol = asio::local::stream_protocol;

SshTunnel::SshTunnel(std::string host, int port, std::string key_path, std::string password)
    : host_(std::move(host))
    , port_(port)
    , key_path_(std::move(key_path))
    , password_(std::move(password)) {
  parseHostToString();
}

SshTunnel::~SshTunnel() {
  stop();
}

void SshTunnel::start(const std::string& local_socket_path, std::ostream& os) {
  session_ = ssh_new();
  if (session_ == nullptr)
	throw std::runtime_error(
	    "Failed to authenticate. Reason: not enough memory not create session");

  ssh_options_set(session_, SSH_OPTIONS_HOST, host_.c_str());
  ssh_options_set(session_, SSH_OPTIONS_PORT, &port_);
  ssh_options_set(session_, SSH_OPTIONS_USER, user_.c_str());

  std::println(os, "==> Connecting to SSH {}@{}:{}\n", user_, host_, port_);
  if (ssh_connect(session_) != SSH_OK)
	throw std::runtime_error(std::format("Failed to connect. Reason: {}", ssh_get_error(session_)));

  authenticate(os);

  // Delete old bridge socket
  fs::remove(local_socket_path);

  running_ = true;

  // Initiate background socket mounting
  worker_thread_ = std::thread([this, local_socket_path]() {
	try {
	  asio::io_context ioc;
	  stream_protocol::acceptor acceptor(ioc, stream_protocol::endpoint(local_socket_path));

	  while (running_) {
		stream_protocol::socket local_sock(ioc);
		acceptor.accept(local_sock);

		// Open SSH pipe to forward to remote /var/run/docker.sock
		ssh_channel channel = ssh_channel_new(session_);
		if (ssh_channel_open_forward_unix(channel, "/var/run/docker.sock", "localhost", 0) !=
		    SSH_OK) {
		  ssh_channel_free(channel);
		  continue;
		}

		// Two sided traffic forward between local_sock and ssh_channel
		char buf[4096];  // 4 KiB
		boost::system::error_code ec;

		while (running_ && local_sock.is_open()) {
		  size_t len = local_sock.read_some(asio::buffer(buf), ec);
		  if (ec)
			break;

		  ssh_channel_write(channel, buf, len);

		  int nBytes = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);
		  if (nBytes > 0)
			asio::write(local_sock, asio::buffer(buf, nBytes), ec);
		}

		ssh_channel_close(channel);
		ssh_channel_free(channel);
	  }
	} catch (...) {
	}
  });

  // Sleep current thread 200ms to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void SshTunnel::stop() {
  running_ = false;
  if (worker_thread_.joinable())
	worker_thread_.detach();

  if (session_) {
	ssh_disconnect(session_);
	ssh_free(session_);
	session_ = nullptr;
  }
}

void SshTunnel::authenticate(std::ostream& os) {
  // 1. Authorization attempt by explicit key
  if (!key_path_.empty()) {
	std::println(os, "==> Trying to authorize with explicit key...\n");
	ssh_key key = nullptr;
	if (ssh_pki_import_privkey_file(key_path_.c_str(), nullptr, nullptr, nullptr, &key) == SSH_OK) {
	  int rc = ssh_userauth_publickey(session_, nullptr, key);
	  ssh_key_free(key);
	  if (rc == SSH_AUTH_SUCCESS) {
		std::println(os, "==> Authorization by key succeeded\n");
		return;
	  }
	  std::println(os, "==> Warning: failed to authorize by key. Keeping on...\n");
	} else {
	  std::println(os, "==> Failed to import private key file \"{}\". Keeping on...", key_path_);
	}
  }

  // 2. Variant A: Automatic authorization (ssh-agent + id_rsa / id_ed25519)
  std::println(os, "==> Trying to authorize via SSH agent...");
  if (ssh_userauth_publickey_auto(session_, nullptr, nullptr) == SSH_AUTH_SUCCESS) {
	std::println(os, "==> Authorization with SSH agent succeeded\n");
	return;
  }
  std::println(os, "==> Warning: automatic authorization failed. Keeping on...\n");

  // 3. Variant B: Password authorization
  if (!password_.empty()) {
	std::println(os, "==> Attempting to authorize with password\n");
	if (ssh_userauth_password(session_, nullptr, password_.c_str()) == SSH_AUTH_SUCCESS) {
	  std::println(os, "==> Succeeded");
	}
  }

  throw std::runtime_error("Failed to authenticate");
}

void SshTunnel::parseHostToString() {
  auto at_pos = host_.find('@');
  if (at_pos != std::string::npos) {
	user_ = host_.substr(0, at_pos);
	host_ = host_.substr(at_pos + 1);
  } else {
	user_ = "root";  // default one
  }
}


}  // namespace WgForge
