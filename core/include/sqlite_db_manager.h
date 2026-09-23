/*
 * sqlite_db_manager.h
 * wgforge-core — Core library for wgforge-server (C)
 * Copyright (C) 2026  Ledovskiy Maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * This file is part of wgforge.
 *
 * wgforge-core is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * wgforge-core is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with wgforge-core. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __WGFORGE_SQLITEDBMANAGER_H__
#define __WGFORGE_SQLITEDBMANAGER_H__

#include <sqlite3.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "peer_config.h"
#include "wgforge.h"

namespace WgForge {

class SQLiteDbManager {
public:
  using SQLiteReturnCode = int;
  using CallbackFunction = int (*)(void*, int, char**, char**);

  SQLiteDbManager() noexcept = default;
  explicit SQLiteDbManager(std::string_view path, bool readonly = true) noexcept;

  SQLiteDbManager(const SQLiteDbManager&) = delete;
  SQLiteDbManager& operator=(const SQLiteDbManager&) = delete;

  SQLiteDbManager(SQLiteDbManager&& other) noexcept;
  SQLiteDbManager& operator=(SQLiteDbManager&& other) noexcept;

  ~SQLiteDbManager() noexcept = default;

  explicit operator bool() const noexcept;

  SQLiteReturnCode openConnection(std::string_view path, bool readonly = true);
  inline void closeConnection() noexcept;
  inline bool isOpen() const noexcept;

  SQLiteReturnCode rawSql(std::string_view query, CallbackFunction strategy = nullptr,
                          void* context = nullptr) noexcept;

  SQLiteReturnCode getPeer(std::string_view public_key, PeerConfig& peer) const;
  std::vector<PeerConfig> getAllPeers(SQLiteReturnCode* rc = nullptr) const;
  SQLiteReturnCode addPeer(const PeerConfig& peer) noexcept;
  SQLiteReturnCode updatePeer(std::string_view publiс_key, const PeerRecord& peer) noexcept;
  SQLiteReturnCode deletePeer(std::string_view public_key) noexcept;
  std::vector<std::string> getAllPeersAddresses(SQLiteReturnCode* rc = nullptr) const;
  SQLiteReturnCode rotatePeerPublicKey(std::string_view old_public_key,
                                       std::string_view new_public_key);

  SQLiteReturnCode addAllowedIP(std::string_view public_key, std::string_view cidr);
  SQLiteReturnCode addAllowedIPs(std::string_view public_key, std::vector<std::string> cidrs);

  SQLiteReturnCode removeAllowedIP(std::string_view public_key, std::string_view cidr);
  SQLiteReturnCode removeAllowedIPs(std::string_view public_key, std::vector<std::string> cidrs);
  std::vector<std::string> getAllowedIPs(std::string_view public_key,
                                         SQLiteReturnCode* rc = nullptr) const;

private:
  struct DbDeleter {
	void operator()(sqlite3* p) noexcept;
  };

  inline void __closeConnectionUnsafe() noexcept;
  inline void __convertText(const char** pstr, int col, sqlite3_stmt* stmt) const noexcept;
  PeerRecord __fillPeerRecord(sqlite3_stmt* stmt) const;

  std::unique_ptr<sqlite3, DbDeleter> db_;
  bool readonly_;
  mutable std::mutex mtx_;
};

} // namespace WgForge

#endif // __WGFORGE_SQLITEDBMANAGER_H__
