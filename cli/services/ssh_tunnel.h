/*
 * ssh_tunnel.h
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

#ifndef WGFORGE_CLI_SSH_TUNNEL_H
#define WGFORGE_CLI_SSH_TUNNEL_H

#include <libssh/libssh.h>

#include <atomic>
#include <ostream>
#include <string>
#include <thread>

namespace WgForge {

class SshTunnel {
public:
  SshTunnel(std::string host, int port, std::string key_path, std::string password);
  ~SshTunnel();

  void start(const std::string& local_socket_path, std::ostream& os);
  void stop();

private:
  std::string host_;
  int port_;
  std::string user_;
  std::string key_path_;
  std::string password_;
  std::string local_socket_path_;

  ssh_session session_ = nullptr;
  std::atomic<bool> running_ = false;
  std::thread worker_thread_;

  void authenticate(std::ostream& os);
  void parseHostToString();
};

}  // namespace WgForge


#endif  // WGFORGE_CLI_SSH_TUNNEL_H
