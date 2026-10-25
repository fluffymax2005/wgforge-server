/*
 * data_processor.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
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

#include "data_processor.h"

#include <format>
#include <sstream>
#include <string_view>

namespace WgForge {

std::string DataProcessor::peersToString(const std::vector<Peer>& peers, Display display,
                                         bool short_keys) const {
  auto it = peer_procs_.find(display);
  if (it != peer_procs_.end())
	return it->second(peers, short_keys);
  return peersToGrid(peers, short_keys);
}

std::string DataProcessor::truncate_key(const std::string& key) {
  if (key.length() < 8)
	return key;
  return std::format("{}...{}", key.substr(0, 8), key.substr(key.length() - 4));
}

std::string DataProcessor::peersToGrid(const std::vector<Peer>& peers, bool short_keys) {
  const std::vector<std::string> headers = {
      "Id",        "Public key",  "Preshared key", "Status",     "Tunnel address", "Allowed IPs",
      "Keepalive", "Description", "Created at",    "Updated at", "Deleted at"};

  std::vector<GridRow> rows;
  rows.reserve(peers.size());

  for (const auto& p : peers) {
	GridRow row;

	row.push_back({{std::to_string(p.id)}});
	row.push_back({{short_keys ? truncate_key(p.public_key) : p.public_key}});
	row.push_back({{short_keys ? truncate_key(p.preshared_key.value_or("NULL"))
	                           : p.preshared_key.value_or("NULL")}});
	row.push_back({{Peer::statusToString(p.status)}});
	row.push_back({{p.tunnel_address}});

	GridCell ips_cell;
	ips_cell.lines.reserve(p.allowed_ips->size());
	for (const auto& ip : *p.allowed_ips)
	  ips_cell.lines.push_back(ip);
	row.push_back(std::move(ips_cell));

	row.push_back({{std::format("{} s", p.persistent_keepalive)}});
	row.push_back({{p.description}});
	row.push_back({{p.created_at}});
	row.push_back({{p.updated_at.value_or("NULL")}});
	row.push_back({{p.deleted_at.value_or("NULL")}});

	rows.push_back(std::move(row));
  }

  auto col_widths = calc_col_widths(rows, headers);

  // Border chars
  constexpr std::string_view TOP_LEFT = "╭";
  constexpr std::string_view TOP_RIGHT = "╮";
  constexpr std::string_view BOTTOM_LEFT = "╰";
  constexpr std::string_view BOTTOM_RIGHT = "╯";
  constexpr std::string_view HORIZONTAL = "─";
  constexpr std::string_view VERTICAL = "│";
  constexpr std::string_view TOP_CROSS = "┬";
  constexpr std::string_view BOTTOM_CROSS = "┴";
  constexpr std::string_view MID_CROSS = "┼";
  constexpr std::string_view LEFT_CROSS = "├";
  constexpr std::string_view RIGHT_CROSS = "┤";

  std::ostringstream oss;

  auto render_border = [&](std::string_view left, std::string_view mid, std::string_view right) {
	oss << left;
	for (auto col = 0; col < col_widths.size(); col++) {
	  for (auto k = 0; k < col_widths[col] + 2; k++)
		oss << HORIZONTAL;
	  if (col + 1 < col_widths.size())
		oss << mid;
	}
	oss << right << '\n';
  };

  auto render_multi_line_row = [&](const GridRow& row) {
	size_t row_height = 1;
	for (size_t col = 0; col < col_widths.size(); ++col) {
	  row_height = std::max(row_height, row[col].lines.size());
	}

	for (size_t line_idx = 0; line_idx < row_height; ++line_idx) {
	  oss << VERTICAL;
	  for (size_t col = 0; col < col_widths.size(); ++col) {
		std::string cell_text = (line_idx < row[col].lines.size()) ? row[col].lines[line_idx] : "";
		size_t vis_w = getUtf8DisplayWidth(cell_text);
		size_t padding = col_widths[col] - vis_w;

		oss << ' ' << cell_text << std::string(padding, ' ') << ' ' << VERTICAL;
	  }
	  oss << '\n';
	}
  };

  // Table building

  // 1. Top border
  render_border(TOP_LEFT, TOP_CROSS, TOP_RIGHT);

  // 2. Header
  for (auto col = 0; col < col_widths.size(); col++) {
	auto vis_w = getUtf8DisplayWidth(headers[col]);
	size_t padding = col_widths[col] - vis_w;
	if (col > 0)
	  oss << ' ' << headers[col] << std::string(padding, ' ') << ' ' << VERTICAL;
	else
	  oss << VERTICAL << ' ' << headers[col] << std::string(padding, ' ') << ' ' << VERTICAL;
  }
  oss << '\n';

  // 3. Header delimiter
  render_border(LEFT_CROSS, MID_CROSS, RIGHT_CROSS);

  // 4. Rows rendering
  for (auto i = 0; i < rows.size(); i++) {
	render_multi_line_row(rows[i]);

	// Render subrow delimiter for all peers but last one
	if (i + 1 < rows.size())
	  render_border(LEFT_CROSS, MID_CROSS, RIGHT_CROSS);
  }

  // 5. Bottom border
  render_border(BOTTOM_LEFT, BOTTOM_CROSS, BOTTOM_RIGHT);

  return oss.str();
}

std::string DataProcessor::rotatedPeerKeyToString(const RotatePeerPublicKeyResponse& res) const {
  return std::format(
      "Newly updated key pair:\n"
      "Public key: {}\n"
      "Private key (KEEP it SECRET): {}",
      res.public_key, res.private_key);
}

std::vector<size_t> DataProcessor::calc_col_widths(const std::vector<GridRow>& rows,
                                                   const std::vector<std::string>& headers) {
  std::vector<size_t> widths(headers.size(), 0);

  for (auto col = 0; col < headers.size(); col++) {
	widths[col] = getUtf8DisplayWidth(headers[col]);
	for (const auto& row : rows)
	  for (const auto& line : row[col].lines)
		widths[col] = std::max(widths[col], getUtf8DisplayWidth(line));
  }

  return widths;
}

size_t DataProcessor::getUtf8DisplayWidth(std::string_view str) {
  size_t width = 0;
  for (unsigned char c : str)
	if ((c & 0xC0) != 0x80)
	  width++;

  return width;
}

std::string DataProcessor::peersToList(const std::vector<Peer>& peers, bool short_keys) {
  constexpr std::string_view IP_PADDING = "                ";
  std::ostringstream oss;

  oss << "------------------------------------------------------------------\n\n";
  for (auto i = 0; i < peers.size(); i++) {
	const auto& p = peers[i];
	oss << "Id:             " << p.id << '\n'
	    << "Public key:     " << (short_keys ? truncate_key(p.public_key) : p.public_key) << '\n'
	    << "Preshared key:  "
	    << (short_keys ? truncate_key(p.preshared_key.value_or("NULL"))
	                   : p.preshared_key.value_or("NULL"))
	    << '\n'
	    << "Status:         " << Peer::statusToString(p.status) << '\n'
	    << "Tunnel address: " << p.tunnel_address << '\n'
	    << "Allowed IPs:    ";

	for (auto k = 0; k < p.allowed_ips->size(); k++) {
	  if (k > 0)
		oss << IP_PADDING;
	  oss << (*p.allowed_ips)[k] << '\n';
	}


	oss << "Keepalive:      " << p.persistent_keepalive << '\n'
	    << "Description:    " << p.description << '\n'
	    << "Created at:     " << p.created_at << '\n'
	    << "Updated at:     " << p.updated_at.value_or("NULL") << '\n'
	    << "Deleted at:     " << p.deleted_at.value_or("NULL") << "\n\n";

	oss << "------------------------------------------------------------------\n\n";
  }

  return oss.str();
}

std::string DataProcessor::interfaceToString(
    const InterfaceConfig& iface /*, Display display*/) const {
  std::ostringstream oss;
  oss << "Wireguard interface \"" << iface.name << "\" configuration:\n\n"
      << "Address: " << iface.addr << '\n'
      << "Protocol version: " << (iface.proto_type == Protocol::IPv4 ? "IPv4\n" : "IPv6\n")
      << "Listen port: " << iface.listenPort << '\n'
      << "Private key (KEEP THIS SECRET): " << iface.private_key.toB64String();

  return oss.str();
}

std::string DataProcessor::serverToString(const ServerConfig& server /*, Display display*/) const {
  std::ostringstream oss;
  oss << "HTTP daemon config:\n"
      << "Address: " << server.addr << '\n'
      << "Listen port: " << server.listenPort;

  return oss.str();
}

}  // namespace WgForge
