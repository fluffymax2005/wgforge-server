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

#include <memory>
#include <mutex>
#include <odb/sqlite/database.hxx>
#include <string>
#include <string_view>
#include <vector>

#include "peer.hxx"
#include "peer_config.h"

namespace WgForge {

/**
 * @brief Manage SQLite3 database. Thread-safety: safe.
 */
class SQLiteDbManager {
public:
  /**
   * @brief Return code for instance methods. Non zero if method call succeded.
   */
  using SQLiteReturnCode = int;

  /**
   * @brief Callback function provided to handle raw sql queries manually.
   * @see SQLiteDbManager::rawSql.
   */
  using CallbackFunction = int (*)(void*, int, char**, char**);

  SQLiteDbManager() noexcept = default;

  /**
   * @brief Instance manager.
   * @param path full path to db file
   * @param readonly whether open in readonly mode
   * @see SQLiteDbManager::openConnection.
   */
  explicit SQLiteDbManager(std::string_view path, bool readonly = true) noexcept;

  SQLiteDbManager(const SQLiteDbManager&) = delete;
  SQLiteDbManager& operator=(const SQLiteDbManager&) = delete;

  /**
   * @brief Move ctor. Moves other containment to this.
   */
  SQLiteDbManager(SQLiteDbManager&& other) noexcept;

  /**
   * @brief Move assignment. Closed current connection if exists and moved other containment to this one.
   */
  SQLiteDbManager& operator=(SQLiteDbManager&& other) noexcept;

  ~SQLiteDbManager() noexcept = default;

  /**
   * @brief Effectively calls SQLiteDbManager::isOpen.
   */
  explicit operator bool() const noexcept;

  /**
   * @brief Open new database connection. Closes existing one. Unsuccessful opening leads to closed old connection.
   * @param path full path to database file
   * @param readonly whether open database in readonly mode.
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   */
  SQLiteReturnCode openConnection(std::string_view path, bool readonly = true);

  /**
   * @brief Closed current connection. If not present does nothing.
   */
  inline void closeConnection() noexcept;

  /**
   * @brief Check current connection state.
   * @retval true if opened
   * @retval false otherwise
   */
  inline bool isOpen() const noexcept;

  /**
   * @brief Execute raw sql query.
   * @param query null terminated string of query
   * @param strategy callback function to handle data after request
   * @param context user context given to strategy as 1st argument.
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   */
  SQLiteReturnCode rawSql(std::string_view query, CallbackFunction strategy = nullptr,
                          void* context = nullptr) noexcept;

  /**
   * @brief Extract peer's configuration from database.
   * @param public_key null terminated peer's public key
   * @param peer reference to method filled result structure
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   */
  SQLiteReturnCode getPeer(std::string_view public_key, PeerConfig& peer) const;

  /**
   * @brief Extract all peers' configurations from database.
   * @param rc optional return code indicating success. Returns WGFORGE_DATABASE_OK on success otherwise error code  
   */
  std::vector<PeerConfig> getAllPeers(SQLiteReturnCode* rc = nullptr) const;

  /**
   * @brief Add peer to database.
   * @param peer peer configuration
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note Only PeerConfig::public_key, PeerConfig::preshared_key, PeerConfig::cidr, PeerConfig::persistent_keepalive and PeerConfig::description took into account. 
   */
  SQLiteReturnCode addPeer(const PeerConfig& peer) noexcept;

  /**
   * @brief Update peer's config in database.
   * @param public_key null terminated peer's public key
   * @param peer peer configuration
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note Only PeerConfig::preshared_key, PeerConfig::persistent_keepalive and PeerConfig::description took into account. 
   */
  SQLiteReturnCode updatePeer(std::string_view publiс_key, const Peer& peer) noexcept;

  /**
   * @brief "Delete" peer from database.
   * @param public_key null terminated peer's public key
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note Effectively sets peer's "Delete_at" field as CURRENT_TIMESTAMP   
   */
  SQLiteReturnCode deletePeer(std::string_view public_key) noexcept;

  /**
   * @brief Extract all peers' CIDRs from database.
   * @param rc optional return code indicating success. Returns WGFORGE_DATABASE_OK on success otherwise error code
   * @retval array of string implementations of clients's tunnel addresses. 
   */
  std::vector<std::string> getAllPeersAddresses(SQLiteReturnCode* rc = nullptr) const;

  /**
   * @brief Change peer's public key. Usually used when client's private key is lost and new one must be generated.
   * @param old_public_key current peer's public key
   * @param new_public_key new peer's public key
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   */
  SQLiteReturnCode rotatePeerPublicKey(std::string_view old_public_key,
                                       std::string_view new_public_key);

  /**
   * @brief Append allowed ip to peer.
   * @param public_key null terminated peer's public key
   * @param cidr null terminated allowed ip string to add
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note If cidr already exists then nothing is done but error code is return indicating error.
   */
  SQLiteReturnCode addAllowedIP(std::string_view public_key, std::string_view cidr);


  /**
   * @brief Atomically append allowed ips to peer.
   * @param public_key null terminated peer's public key
   * @param cidrs array of null terminated strings to add
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note If cidr already exists then nothing is done but error code is return indicating error.
   */
  SQLiteReturnCode addAllowedIPs(std::string_view public_key, std::vector<std::string> cidrs);

  /**
   * @brief Soft delete allowed ip.
   * @param public_key null terminated peer's public key
   * @param cidr null terminated allowed ip string to remove
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note Actually sets "Delete_at" to CURRENT_TIMESTAMP.
   */
  SQLiteReturnCode removeAllowedIP(std::string_view public_key, std::string_view cidr);

  /**
   * @brief Atomically performs soft delete to provided allowed ips.
   * @param public_key null terminated peer's public key
   * @param cidrs array of null terminated strings to remove
   * @retval returns WGFORGE_DATABASE_OK on success otherwise error code
   * @note Actually sets "Delete_at" to CURRENT_TIMESTAMP for found records.
   */
  SQLiteReturnCode removeAllowedIPs(std::string_view public_key, std::vector<std::string> cidrs);

  /**
   * @brief Get all peer's allowed ips.
   * @param public_key null terminated peer's public key
   * @param rc optional return code indicating success
   * @retval array of found allowed ips
   */
  std::vector<std::string> getAllowedIPs(std::string_view public_key,
                                         SQLiteReturnCode* rc = nullptr) const;

private:
  /**
   * @brief Thread unsafe version of closing connection.
   */
  inline void __closeConnectionUnsafe() noexcept;

  /**
   * @brief Database ptr to connection.
   */
  std::unique_ptr<odb::sqlite::database> db_;

  /**
   * @brief Readonly flag to interact with database.
   */
  bool readonly_;

  /**
   * @brief Mutex for thread-safety.
   */
  mutable std::mutex mtx_;
};

}  // namespace WgForge

#endif  // __WGFORGE_SQLITEDBMANAGER_H__
