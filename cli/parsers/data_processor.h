/*
 * data_processor.h
 * wgforge-cli — CLI frontend for wgforge-server (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)rver (C)
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

#ifndef WGFORGE_DATA_PROCESSOR_H
#define WGFORGE_DATA_PROCESSOR_H

#include <string_view>
#include <unordered_map>

#include "idataprocessor.h"

namespace WgForge {

class DataProcessor : public IDataProcessor {
public:
  virtual std::string peersToString(const std::vector<Peer>& peers, Display display,
                                    bool short_keys) const override;
  virtual std::string rotatedPeerKeyToString(const RotatePeerPublicKeyResponse& res) const override;
  virtual std::string interfaceToString(const InterfaceConfig& iface/*,
                                        Display display*/) const override;
  virtual std::string serverToString(
      const ServerConfig& server /*, Display display*/) const override;

private:
  static std::string truncate_key(const std::string& key);

  static std::string peersToGrid(const std::vector<Peer>& peers, bool short_keys);
  static std::string peersToList(const std::vector<Peer>& peers, bool short_keys);

  // Grid cell contains several values for Allowed IPs
  struct GridCell {
	std::vector<std::string> lines;
  };

  using GridRow = std::vector<GridCell>;

  static size_t getUtf8DisplayWidth(std::string_view str);
  static std::vector<size_t> calc_col_widths(const std::vector<GridRow>& rows,
                                             const std::vector<std::string>& headers);

  std::unordered_map<Display, std::function<std::string(const std::vector<Peer>&, bool)>>
      peer_procs_{
          {Display::GRID, peersToGrid},
          {Display::LIST, peersToList}
  };
};

}  // namespace WgForge

#endif  // WGFORGE_DATA_PROCESSOR_H
