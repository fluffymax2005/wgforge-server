/*
 * cli_app.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)
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

#include "cli_app.h"

#include <string>
#include <unordered_map>

namespace WgForge {

CliApp::CliApp(std::shared_ptr<CliController> cli_controller, std::ostream& os)
    : os_(os), app_("\nwgforge-cli - command line interface to manage wgforge-server.\n") {
  if (cli_controller == nullptr)
	throw std::invalid_argument("Cli controller is nullptr");

  cli_controller_ = std::move(cli_controller);
  setupOptions();
}

int CliApp::run(int argc, char* argv[]) {
  try {
	CLI11_PARSE(app_, argc, argv);
	return 0;
  } catch (const CLI::ParseError& e) {
	return app_.exit(e);
  } catch (const std::exception& e) {
	os_ << ENDLINE << e.what() << ENDLINE << ENDLINE;
	return -1;
  }
}

void CliApp::setupOptions() {
  app_.require_subcommand(0, 1);

  app_.set_version_flag("-v,--version", WGFORGE_CLI_VERSION, "Display version");

  const std::unordered_map<std::string, Display> display_map{
      {"grid", Display::GRID},
      {"list", Display::LIST}
  };

  // CMD: peers

  auto* cmd_peers = app_.add_subcommand("peers", "Display all peers");
  auto peer_all_req = std::make_shared<GetAllPeerRequest>();

#ifdef WGFORGE_CLI_WINDOWS
  constexpr auto ENDPOINT_DESCRIPTION =
      "Endpoint to send request to (for example, mydns.com:1111).";
#else
  constexpr auto ENDPOINT_DESCRIPTION =
      "Endpoint to send request to (for example, mydns.com:1111). Overrides "
      "interface option anyway";
#endif

#ifndef WGFORGE_CLI_WINDOWS
  cmd_peers->add_option("-i,--interface", peer_all_req->iface, "Interface name (for example, wg0)");
#endif
  cmd_peers->add_option("-e,--endpoint", peer_all_req->endpoint, ENDPOINT_DESCRIPTION);
  cmd_peers->add_flag("-s,--short-keys", peer_all_req->short_keys,
                      "Short key representation (for example, A1G5....FGA=)");
  cmd_peers
      ->add_option("-c,--columns", peer_all_req->cols,
	               "Field to display separated by commas:\n"
	               "id, public_key, preshared_key, status, description, allowed_ips, "
	               "created_at, updated_at, deleted_at")
      ->delimiter(',');

  cmd_peers->add_option("-d,--display", peer_all_req->display, "Display type")
      ->transform(CLI::CheckedTransformer(display_map, CLI::ignore_case));

  cmd_peers->callback([this, peer_all_req]() { cli_controller_->getAllPeers(*peer_all_req); });

  // CMD: peer

  auto* cmd_peer = app_.add_subcommand("peer", "Manage concrete peer");
  cmd_peer->require_subcommand(0, 1);

  auto peer_req = std::make_shared<GetPeerRequest>();
  cmd_peer->add_option("-p,--public-key", peer_req->public_key, "Peer's public key");
  cmd_peer->add_option("-e,--endpoint", peer_req->endpoint, ENDPOINT_DESCRIPTION);
  cmd_peer->add_flag("-s,--short-keys", peer_req->short_keys,
                     "Short key representation (for example, A1G5....FGA=)");
#ifndef WGFORGE_CLI_WINDOWS
  cmd_peer->add_option("-i,--interface", peer_req->iface, "Interface name");
#endif
  cmd_peer->add_option("-c,--columns", peer_req->cols, "Columns to display")->delimiter(',');
  cmd_peer->add_option("-d,--display", peer_req->display, "Display type")
      ->transform(CLI::CheckedTransformer(display_map, CLI::ignore_case));

  cmd_peer->callback([this, cmd_peer, peer_req] {
	if (cmd_peer->get_subcommands().empty())
	  cli_controller_->getPeer(*peer_req);
  });

  // CMD: peer config

  auto* cmd_peer_config =
      cmd_peer->add_subcommand("config", "Get client Wireguard compatible configuration");
  auto config_req = std::make_shared<GetPeerConfigRequest>();

  cmd_peer_config->add_option("-p,--public-key", config_req->public_key, "Peer's public key");
  cmd_peer_config->add_option(
      "-e,--endpoint", config_req->endpoint,
      "Endpoint to send request to (for example, mydns.com:1111). Overrides "
      "interface option anyway");
#ifndef WGFORGE_CLI_WINDOWS
  cmd_peer_config->add_option("-i,--interface", config_req->iface, "Interface name");
#endif
  cmd_peer_config->add_option("--public-endpoint", config_req->public_endpoint,
                              ENDPOINT_DESCRIPTION);
  cmd_peer_config->callback([this, config_req] { cli_controller_->getPeerConfig(*config_req); });

  // CMD: peer rotate

  auto cmd_peer_rotate =
      cmd_peer->add_subcommand("rotate",
	                           "Rotate peer's public key to new one. Regenerates old one and "
	                           "display it as well as private key which were used for generation");
  auto rotate_req = std::make_shared<RotatePeerPublicKeyRequest>();

  cmd_peer_rotate->add_option("-p,--public-key", rotate_req->old_key,
                              "Peer's current (old) public key");
  cmd_peer_rotate->add_option("-e,--endpoint", rotate_req->endpoint, ENDPOINT_DESCRIPTION);
#ifndef WGFORGE_CLI_WINDOWS
  cmd_peer_rotate->add_option("-i,--interface", rotate_req->iface, "Interface name");
#endif

  cmd_peer_rotate->callback(
      [this, rotate_req] { cli_controller_->rotatePeerPublicKey(*rotate_req); });

  // CMD: peer delete

  auto cmd_peer_del = cmd_peer->add_subcommand("delete", "Delete peer");
  auto del_req = std::make_shared<DeletePeerRequest>();
  cmd_peer_del->add_option("-e,--endpoint", del_req->endpoint, ENDPOINT_DESCRIPTION);
  cmd_peer_del->add_option("-p,--public-key", del_req->public_key, "Peer's public key");
#ifndef WGFORGE_CLI_WINDOWS
  cmd_peer_del->add_option("-i,--interface", del_req->iface, "Interface name");
#endif

  cmd_peer_del->callback([this, del_req] { cli_controller_->deletePeer(*del_req); });

  // CMD: peer update

  auto cmd_peer_upd = cmd_peer->add_subcommand("update", "Update peer");
  auto upd_req = std::make_shared<UpdatePeerRequest>();
  cmd_peer_upd->add_option("-p,--public-key", upd_req->public_key, "Peer's public key");
  cmd_peer_upd->add_option("-e,--endpoint", upd_req->endpoint, ENDPOINT_DESCRIPTION);
#ifndef WGFORGE_CLI_WINDOWS
  cmd_peer_upd->add_option("-i,--interface", upd_req->iface, "Interface name");
#endif
  cmd_peer_upd->add_option("--preshared-key", upd_req->preshared_key,
                           "Peer's preshared key to change to");
  cmd_peer_upd->add_option("--status", upd_req->status, "Peer's status to change to");
  cmd_peer_upd->add_option("--description", upd_req->description, "Peer's new description");
  cmd_peer_upd->add_option("--persistent-keepalive", upd_req->persistent_keepalive,
                           "Peer's new persistent keepalive in seconds");
  cmd_peer_upd->add_option("--add-allowedip", upd_req->add_allowedips,
                           "New accessible subnets for peer");
  cmd_peer_upd->add_option("--remove-allowedip", upd_req->remove_allowedips,
                           "Subnets peer is not allowedip to access to anymore");

  cmd_peer_upd->callback([this, upd_req] { cli_controller_->updatePeer(*upd_req); });

  // CMD: interface

  auto cmd_iface = app_.add_subcommand("interface", "Get Wireguard interface configuration");
  cmd_iface->require_subcommand(0, 1);
  auto iface_get_req = std::make_shared<GetInterfaceConfigRequest>();
#ifndef WGFORGE_CLI_WINDOWS
  cmd_iface->add_option("interface", iface_get_req->iface, "Wireguard interface name");
#endif
  cmd_iface->add_option("-e,--endpoint", iface_get_req->endpoint, ENDPOINT_DESCRIPTION);

  cmd_iface->callback([this, cmd_iface, iface_get_req] {
	if (cmd_iface->get_subcommands().empty())
	  cli_controller_->getInterfaceConfig(*iface_get_req);
  });

  // CMD: interface update

  auto cmd_iface_upd =
      cmd_iface->add_subcommand("update", "Update Wireguard interface configuration");
  auto iface_upd_req = std::make_shared<UpdateInterfaceRequest>();
  cmd_iface_upd->add_option("-e,--endpoint", iface_upd_req->endpoint, ENDPOINT_DESCRIPTION);
  cmd_iface_upd->add_option("-a,--address", iface_upd_req->address, "New listening address");
  cmd_iface_upd->add_option("-p,--private-key", iface_upd_req->private_key, "New private key");
  cmd_iface_upd->add_option("-l,--listen-port", iface_upd_req->listen_port, "New listen port");

  cmd_iface_upd->callback(
      [this, iface_upd_req] { cli_controller_->updateInterfaceConfig(*iface_upd_req); });

  // CMD: server

  auto cmd_srv = app_.add_subcommand("server", "Get HTTP server configuration. Linux only");
  cmd_srv->require_subcommand(0, 1);
  auto srv_get_cfg = std::make_shared<GetServerConfigRequest>();
  cmd_srv->add_option("interface", srv_get_cfg->iface, "Wireguard interface name");

  cmd_srv->callback([this, cmd_srv, srv_get_cfg] {
	if (cmd_srv->get_subcommands().empty())
	  cli_controller_->getServerConfig(*srv_get_cfg);
  });

  auto cmd_srv_upd =
      cmd_srv->add_subcommand("update", "Update HTTP server configuration. Linux only");
  auto srv_upd_req = std::make_shared<UpdateServerRequest>();
  cmd_srv_upd->add_option("interface", srv_upd_req->iface, "Wireguard interface name");
  cmd_srv_upd->add_option("-a,--address", srv_upd_req->address,
                          "New HTTP server address to listen on");
  cmd_srv_upd->add_option("-l,--listen-port", srv_upd_req->listen_port,
                          "New HTTP server listen port");

  cmd_srv_upd->callback(
      [this, srv_upd_req] { cli_controller_->updateServerConfig(std::move(*srv_upd_req)); });

  // CMD: server deploy
  auto cmd_srv_deploy = cmd_srv->add_subcommand("deploy",
                                                "Deploy WgForge Server container via Docker REST "
                                                "API. Local deploying is avalable on Linux only");
  auto deploy_req = std::make_shared<DeployServerRequest>();

  cmd_srv_deploy->add_option("--image", deploy_req->image,
                             "Docker image tag (default: fluffymax2005/wgforge-server:latest)");
  cmd_srv_deploy
      ->add_option("-i,--interface", deploy_req->iface, "Wireguard interface name (default: wg0)")
      ->required();
  cmd_srv_deploy->add_option("--config-dir", deploy_req->config_path,
                             "Host path for configs (default: /etc/wgforge)");
  cmd_srv_deploy->add_option("--data-dir", deploy_req->data_path,
                             "Host path for DB (default: /var/lib/wgforge)");
  cmd_srv_deploy->add_option("--ssh", deploy_req->ssh_host,
                             "Remote SSH host (e.g. root@192.168.1.100 or 192.168.1.100)");
  cmd_srv_deploy->add_option("--ssh-port", deploy_req->ssh_port, "Remote SSH port (default: 22)");
  cmd_srv_deploy->add_option("--ssh-key", deploy_req->ssh_key,
                             "Path to custom private SSH key file");
  cmd_srv_deploy->add_option("--ssh-password", deploy_req->ssh_password,
                             "SSH password (if key authentication is not configured)");

  cmd_srv_deploy->callback([this, deploy_req] {
	deploy_req->generateContainerName();
	cli_controller_->deployServer(*deploy_req);
  });

  // CMD: server teardown
  auto cmd_srv_teardown = cmd_srv->add_subcommand(
      "teardown",
      "Teardown WgForge Server container. Local teardowning is available on Linux only");
  auto teardown_req = std::make_shared<TeardownServerRequest>();

  cmd_srv_teardown
      ->add_option("name", teardown_req->container_name,
	               "WgForge Server container ID or name (e.g. wgforge-server_wg0)")
      ->required();
  cmd_srv_teardown->add_option("--ssh", teardown_req->ssh_host,
                               "Remote SSH host (e.g. root@192.168.1.100 or 192.168.1.100)");
  cmd_srv_teardown->add_option("--ssh-port", teardown_req->ssh_port,
                               "Remote SSH port (default: 22)");
  cmd_srv_teardown->add_option("--ssh-key", teardown_req->ssh_key,
                               "Path to custom private SSH key file");
  cmd_srv_teardown->add_option("--ssh-password", teardown_req->ssh_password,
                               "SSH password (if key authentication is not configured)");

  cmd_srv_teardown->callback(
      [this, teardown_req] { cli_controller_->teardownServer(*teardown_req); });
}

}  // namespace WgForge
