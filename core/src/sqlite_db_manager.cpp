/*
 * sqlite_db_manager.cpp
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

#include "sqlite_db_manager.h"

#include <sqlite3.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/connection.hxx>
#include <odb/sqlite/exceptions.hxx>
#include <odb/transaction.hxx>
#include <sstream>
#include <wgallowedip.hpp>
#include <wgprivatekey.hpp>
#include <wgpublickey.hpp>

#include "allowedip.hxx"
#include "generated/allowedip-odb.hxx"
#include "generated/peer-odb.hxx"
#include "wgforge.h"

namespace WgForge {

namespace {

// TODO
// Consider use std::string time instead of std::ostringstream
std::string getCurrentUtcTimestamp() {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  gmtime_r(&time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
  return oss.str();
}

SQLiteDbManager::SQLiteReturnCode translateOdbException(const odb::exception& e) {
  if (const auto* de = dynamic_cast<const odb::sqlite::database_exception*>(&e)) {
	int rc = de->error();
	if (rc == SQLITE_CONSTRAINT || rc == SQLITE_CONSTRAINT_UNIQUE) {
	  return WGFORGE_RECORD_EXIST;
	}
	return rc;
  }

  return WGFORGE_RUNTIME_ERROR;
}

}  // namespace

SQLiteDbManager::SQLiteDbManager(std::string_view path, bool readonly) noexcept {
  openConnection(path, readonly);
}

SQLiteDbManager::SQLiteDbManager(SQLiteDbManager&& other) noexcept {
  if (this != &other) {
	std::lock_guard lock(other.mtx_);

	__closeConnectionUnsafe();
	db_ = std::move(other.db_);
	readonly_ = other.readonly_;
  }
}

SQLiteDbManager& SQLiteDbManager::operator=(SQLiteDbManager&& other) noexcept {
  if (this != &other) {
	std::scoped_lock lock(mtx_, other.mtx_);

	__closeConnectionUnsafe();
	db_ = std::move(other.db_);
	readonly_ = other.readonly_;
  }

  return *this;
}

SQLiteDbManager::operator bool() const noexcept {
  return isOpen();
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::openConnection(std::string_view path,
                                                                  bool readonly) {
  std::lock_guard lock(mtx_);
  __closeConnectionUnsafe();

  try {
	int flags = SQLITE_OPEN_FULLMUTEX;
	if (readonly) {
	  flags |= SQLITE_OPEN_READONLY;
	} else {
	  flags |= SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	}

	db_ = std::make_unique<odb::sqlite::database>(std::string(path), flags);
	readonly_ = readonly;

	odb::transaction t(db_->begin());
	db_->execute("PRAGMA foreign_keys = ON;");
	if (!readonly) {
	  odb::schema_catalog::create_schema(*db_, "", false);  // false = IF NOT EXISTS
	}
	t.commit();

	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	db_.reset();
	return translateOdbException(e);
  }
}

inline void SQLiteDbManager::closeConnection() noexcept {
  std::lock_guard lock(mtx_);
  __closeConnectionUnsafe();
}

inline bool SQLiteDbManager::isOpen() const noexcept {
  std::lock_guard lock(mtx_);
  return db_ != nullptr;
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::rawSql(std::string_view query,
                                                          CallbackFunction strategy,
                                                          void* context) noexcept {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (context && !strategy)
	return WGFORGE_DATABASE_STRATEGY_NO_TARGET;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	if (strategy) {
	  auto& conn = static_cast<odb::sqlite::connection&>(t.connection());
	  sqlite3* raw_db = conn.handle();
	  int rc = sqlite3_exec(raw_db, query.data(), strategy, context, nullptr);
	  if (rc != SQLITE_OK) {
		t.rollback();
		return rc;
	  }
	} else {
	  db_->execute(query.data());
	}

	t.commit();
	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::getPeer(std::string_view public_key,
                                                           PeerConfig& peer) const {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
	return WGFORGE_INVALID_ARGUMENT;

  std::lock_guard lock(mtx_);
  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto result = db_->query<Peer>(peer_query::public_key == std::string(public_key));

	if (result.empty()) {
	  return WGFORGE_PEER_NOT_EXIST;
	}

	peer.record = *result.begin();
	t.commit();

	SQLiteReturnCode rc = SQLITE_OK;
	peer.allowed_ips = getAllowedIPs(public_key, &rc);
	return rc;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

std::vector<PeerConfig> SQLiteDbManager::getAllPeers(SQLiteReturnCode* rc) const {
  if (!isOpen()) {
	if (rc)
	  *rc = WGFORGE_CONNECTION_CLOSED;
	return {};
  }

  std::lock_guard lock(mtx_);
  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto result = db_->query<Peer>();

	std::vector<PeerConfig> peers;
	for (const auto& peer_rec : result) {
	  PeerConfig config;
	  config.record = peer_rec;

	  using ip_query = odb::query<AllowedIP>;
	  auto ip_result = db_->query<AllowedIP>(ip_query::peer_id == peer_rec.id);
	  for (const auto& ip_rec : ip_result) {
		config.allowed_ips.push_back(ip_rec.cidr);
	  }
	  peers.push_back(std::move(config));
	}

	t.commit();
	if (rc)
	  *rc = WGFORGE_DATABASE_OK;
	return peers;
  } catch (const odb::exception& e) {
	if (rc)
	  *rc = translateOdbException(e);
	return {};
  }
}

std::vector<std::string> SQLiteDbManager::getAllPeersAddresses(SQLiteReturnCode* rc) const {
  if (!isOpen()) {
	if (rc)
	  *rc = WGFORGE_CONNECTION_CLOSED;
	return {};
  }

  std::lock_guard lock(mtx_);
  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto result = db_->query<Peer>(peer_query::deleted_at.is_null());

	std::vector<std::string> addresses;
	for (const auto& peer_rec : result) {
	  addresses.push_back(peer_rec.tunnel_address);
	}

	t.commit();
	if (rc)
	  *rc = WGFORGE_DATABASE_OK;
	return addresses;
  } catch (const odb::exception& e) {
	if (rc)
	  *rc = translateOdbException(e);
	return {};
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::rotatePeerPublicKey(
    std::string_view old_public_key, std::string_view new_public_key) {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(old_public_key) ||
      !WgKey::validateB64StringKey(new_public_key)) {
	return WGFORGE_INVALID_ARGUMENT;
  }

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;

	// Check whether old peers exists
	auto old_res = db_->query<Peer>(peer_query::public_key == std::string(old_public_key) &&
	                                peer_query::deleted_at.is_null());
	if (old_res.empty()) {
	  return WGFORGE_PEER_NOT_EXIST;
	}

	// Check key is free
	auto new_res = db_->query<Peer>(peer_query::public_key == std::string(new_public_key));
	if (!new_res.empty()) {
	  return WGFORGE_PEER_EXIST;
	}

	// Update key
	Peer peer = *old_res.begin();
	peer.public_key = std::string(new_public_key);
	peer.updated_at = getCurrentUtcTimestamp();

	db_->update(peer);
	t.commit();

	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::addPeer(const PeerConfig& peer) noexcept {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(peer.record.public_key))
	return WGFORGE_INVALID_ARGUMENT;
  if (peer.record.preshared_key.has_value() &&
      !WgKey::validateB64StringKey(peer.record.preshared_key.value())) {
	return WGFORGE_INVALID_ARGUMENT;
  }

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto existing = db_->query<Peer>(peer_query::public_key == peer.record.public_key);
	if (!existing.empty()) {
	  return WGFORGE_PEER_EXIST;
	}

	// Save peer
	auto peer_ptr = std::make_shared<Peer>(peer.record);
	peer_ptr->created_at = getCurrentUtcTimestamp();
	db_->persist(peer_ptr);

	// Save allowed_ips
	for (const auto& cidr : peer.allowed_ips) {
	  AllowedIP ip_rec;
	  ip_rec.peer_id = peer_ptr->id;
	  ip_rec.cidr = cidr;
	  ip_rec.created_at = getCurrentUtcTimestamp();
	  db_->persist(ip_rec);
	}

	t.commit();
	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::updatePeer(std::string_view public_key,
                                                              const Peer& peer) noexcept {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
	return WGFORGE_INVALID_ARGUMENT;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto result = db_->query<Peer>(peer_query::public_key == std::string(public_key) &&
	                               peer_query::deleted_at.is_null());

	if (result.empty()) {
	  return WGFORGE_PEER_NOT_EXIST;
	}

	Peer target = *result.begin();

	if (peer.preshared_key.has_value())
	  target.preshared_key = peer.preshared_key;
	if (peer.persistent_keepalive)
	  target.persistent_keepalive = peer.persistent_keepalive;
	if (peer.description.length())
	  target.description = peer.description;

	target.updated_at = getCurrentUtcTimestamp();

	db_->update(target);
	t.commit();

	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

std::vector<std::string> SQLiteDbManager::getAllowedIPs(std::string_view public_key,
                                                        SQLiteReturnCode* rc) const {
  if (!isOpen()) {
	if (rc)
	  *rc = WGFORGE_CONNECTION_CLOSED;
	return {};
  }

  if (!WgKey::validateB64StringKey(public_key)) {
	if (rc)
	  *rc = WGFORGE_INVALID_ARGUMENT;
	return {};
  }

  std::lock_guard lock(mtx_);
  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto peer_res = db_->query<Peer>(peer_query::public_key == std::string(public_key) &&
	                                 peer_query::deleted_at.is_null());

	if (peer_res.empty()) {
	  if (rc)
		*rc = WGFORGE_PEER_NOT_EXIST;
	  return {};
	}

	Peer peer = *peer_res.begin();

	using ip_query = odb::query<AllowedIP>;
	auto ip_res =
	    db_->query<AllowedIP>(ip_query::peer_id == peer.id && ip_query::deleted_at.is_null());

	std::vector<std::string> cidrs;
	for (const auto& ip_rec : ip_res) {
	  cidrs.push_back(ip_rec.cidr);
	}

	t.commit();
	if (rc)
	  *rc = WGFORGE_DATABASE_OK;
	return cidrs;
  } catch (const odb::exception& e) {
	if (rc)
	  *rc = translateOdbException(e);
	return {};
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::deletePeer(
    std::string_view public_key) noexcept {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
	return WGFORGE_INVALID_ARGUMENT;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto result = db_->query<Peer>(peer_query::public_key == std::string(public_key) &&
	                               peer_query::deleted_at.is_null());

	if (result.empty()) {
	  return WGFORGE_PEER_NOT_EXIST;
	}

	Peer peer = *result.begin();
	const std::string now_str = getCurrentUtcTimestamp();

	// 1. Софт-удаление всех привязанных AllowedIPs
	using ip_query = odb::query<AllowedIP>;
	auto ip_result =
	    db_->query<AllowedIP>(ip_query::peer_id == peer.id && ip_query::deleted_at.is_null());

	for (auto ip_rec : ip_result) {
	  ip_rec.deleted_at = now_str;
	  ip_rec.updated_at = now_str;
	  db_->update(ip_rec);
	}

	peer.deleted_at = now_str;
	db_->update(peer);

	t.commit();
	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::addAllowedIP(std::string_view public_key,
                                                                std::string_view cidr) {
  return addAllowedIPs(public_key, {std::string(cidr)});
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::addAllowedIPs(std::string_view public_key,
                                                                 std::vector<std::string> cidrs) {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
	return WGFORGE_INVALID_ARGUMENT;
  for (const auto& cidr : cidrs) {
	if (!WgAllowedIP::validate(cidr.data()))
	  return WGFORGE_INVALID_ARGUMENT;
  }

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto peer_res = db_->query<Peer>(peer_query::public_key == std::string(public_key) &&
	                                 peer_query::deleted_at.is_null());

	if (peer_res.empty()) {
	  return WGFORGE_PEER_NOT_EXIST;
	}

	auto peer_ptr = std::make_shared<Peer>(*peer_res.begin());
	const std::string now_str = getCurrentUtcTimestamp();

	using ip_query = odb::query<AllowedIP>;

	for (const auto& cidr : cidrs) {
	  auto ip_res =
	      db_->query<AllowedIP>(ip_query::peer_id == peer_ptr->id && ip_query::cidr == cidr);

	  if (!ip_res.empty()) {
		AllowedIP existing_ip = *ip_res.begin();
		if (!existing_ip.deleted_at.has_value()) {
		  continue;
		}
		existing_ip.deleted_at = std::nullopt;
		existing_ip.updated_at = now_str;
		db_->update(existing_ip);
	  } else {
		AllowedIP new_ip;
		new_ip.peer_id = peer_ptr->id;
		new_ip.cidr = cidr;
		db_->persist(new_ip);
	  }
	}

	t.commit();
	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::removeAllowedIP(std::string_view public_key,
                                                                   std::string_view cidr) {
  return removeAllowedIPs(public_key, {std::string(cidr)});
}

SQLiteDbManager::SQLiteReturnCode SQLiteDbManager::removeAllowedIPs(
    std::string_view public_key, std::vector<std::string> cidrs) {
  if (!isOpen())
	return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
	return WGFORGE_INVALID_ARGUMENT;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_DATABASE_READONLY;

  try {
	odb::transaction t(db_->begin());

	using peer_query = odb::query<Peer>;
	auto peer_res = db_->query<Peer>(peer_query::public_key == std::string(public_key) &&
	                                 peer_query::deleted_at.is_null());

	if (peer_res.empty()) {
	  return WGFORGE_PEER_NOT_EXIST;
	}

	Peer peer = *peer_res.begin();
	const std::string now_str = getCurrentUtcTimestamp();

	using ip_query = odb::query<AllowedIP>;

	for (const auto& cidr : cidrs) {
	  auto ip_res = db_->query<AllowedIP>(ip_query::peer_id == peer.id && ip_query::cidr == cidr &&
	                                      ip_query::deleted_at.is_null());

	  if (ip_res.empty()) {
		if (cidrs.size() == 1)
		  return WGFORGE_ALLOWEDIP_NOT_EXIST;
		continue;
	  }

	  AllowedIP ip_rec = *ip_res.begin();
	  ip_rec.deleted_at = now_str;
	  ip_rec.updated_at = now_str;
	  db_->update(ip_rec);
	}

	t.commit();
	return WGFORGE_DATABASE_OK;
  } catch (const odb::exception& e) {
	return translateOdbException(e);
  }
}

inline void SQLiteDbManager::__closeConnectionUnsafe() noexcept {
  if (db_) {
	db_.reset();
  }
}

#undef __THREAD_SAFETY__

}  // namespace WgForge
