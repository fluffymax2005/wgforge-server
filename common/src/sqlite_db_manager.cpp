#include "sqlite_db_manager.h"
#include <chrono>
#include <cstring>
#include <format>
#include <sqlite3.h>
#include <wgallowedip.hpp>
#include <wgprivatekey.hpp>
#include <wgpublickey.hpp>

#ifndef __THREAD_SAFETY__
#define __THREAD_SAFETY__ MultiThreaded
#endif

namespace WgForge {

SQLiteDbManager::SQLiteDbManager(std::string_view path,
                                 bool readonly) noexcept {
  openConnection(path, readonly);
}

SQLiteDbManager::SQLiteDbManager(SQLiteDbManager &&other) noexcept {
  if (this != &other) {
    std::lock_guard lock(other.mtx_);

    __closeConnectionUnsafe();
    db_ = std::move(other.db_);
  }
}

SQLiteDbManager &SQLiteDbManager::operator=(SQLiteDbManager &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock(mtx_, other.mtx_);

    __closeConnectionUnsafe();
    db_ = std::move(other.db_);
  }

  return *this;
}

SQLiteDbManager::operator bool() const noexcept { return isOpen(); }

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::openConnection(std::string_view path, bool readonly) {
  std::lock_guard lock(mtx_);

  if (db_)
    __closeConnectionUnsafe();
  sqlite3 *db;

  int flags = SQLITE_OPEN_FULLMUTEX;
  if (readonly)
    flags |= SQLITE_OPEN_READONLY;
  else
    flags |= SQLITE_OPEN_READWRITE;

  SQLiteReturnCode rc = sqlite3_open_v2(path.data(), &db, flags, nullptr);
  if (db) {
    db_ = std::unique_ptr<sqlite3, DbDeleter>(db);
    readonly_ = readonly;
  }
  return rc;
}

inline void SQLiteDbManager::closeConnection() noexcept {
  std::lock_guard lock(mtx_);
  __closeConnectionUnsafe();
}

inline bool SQLiteDbManager::isOpen() const noexcept {
  std::lock_guard lock(mtx_);
  return db_.get();
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::rawSql(std::string_view query, CallbackFunction strategy,
                        void *context) noexcept {
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;
  if (!strategy)
    return WGFORGE_DATABASE_STRATEGY_NO_TARGET;
  std::lock_guard lock(mtx_);
  if (readonly_)
    return WGFORGE_DATABASE_READONLY;

  return sqlite3_exec(db_.get(), query.data(), strategy, context, nullptr);
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::getPeer(std::string_view public_key, PeerConfig &peer) const {
  using WgKey = WgKey<__THREAD_SAFETY__>;
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
    return WGFORGE_INVALID_ARGUMENT;

  SQLiteReturnCode rc;
  sqlite3_stmt *stmt = nullptr;
  constexpr const char QUERY[] =
      R"(
        SELECT 
            p.Id, 
            p.Public_key,
            p.Preshared_key,
            p.Private_key,
            p.Persistent_keepalive, 
            p.Description, 
            p.Created_at, 
            p.Updated_at
         FROM Peers p
         WHERE p.Deleted_at IS NOT NULL AND p.Public_key = ?
      )";

  auto allowed_ips = getAllowedIPs(public_key, &rc);
  if (rc != SQLITE_OK) {
    return rc;
  }

  {
    std::lock_guard lock(mtx_);
    if ((rc = sqlite3_prepare_v2(db_.get(), QUERY, -1, &stmt, nullptr)) !=
        SQLITE_OK)
      return rc;
  }

  if ((rc = sqlite3_bind_text(stmt, 1, public_key.data(), -1, SQLITE_STATIC)) !=
      SQLITE_OK) {
    sqlite3_finalize(stmt);
    return rc;
  }

  if ((rc = sqlite3_step(stmt)) != SQLITE_ROW) {
    sqlite3_finalize(stmt);
    return rc;
  }

  peer.record = __fillPeerRecord(stmt);
  peer.allowed_ips = std::move(allowed_ips);

  sqlite3_finalize(stmt);

  return SQLITE_OK;
}

std::vector<PeerConfig>
SQLiteDbManager::getAllPeers(SQLiteReturnCode *rc) const {
  if (!isOpen()) {
    if (rc)
      *rc = WGFORGE_CONNECTION_CLOSED;
    return {};
  }

  SQLiteReturnCode ec;
  sqlite3_stmt *stmt;
  constexpr const char QUERY[] =
      R"(
        SELECT 
            p.Id, 
            p.Public_key,
            p.Preshared_key,
            p.Private_key,
            p.Persistent_keepalive, 
            p.Description, 
            p.Created_at, 
            p.Updated_at 
         FROM Peers p
         WHERE p.Deleted_at IS NOT NULL
      )";

  {
    std::lock_guard lock(mtx_);
    if ((ec = sqlite3_prepare_v2(db_.get(), QUERY, -1, &stmt, nullptr)) !=
        SQLITE_OK) {
      if (rc)
        *rc = ec;
      return {};
    }
  }

  const char *pstr;
  std::vector<PeerConfig> peers;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    PeerConfig peer;
    peer.record = __fillPeerRecord(stmt);
    peer.allowed_ips = getAllowedIPs(peer.record.public_key, &ec);
    if (ec != SQLITE_OK) {
      sqlite3_finalize(stmt);
      if (rc)
        *rc = WGFORGE_RUNTIME_ERROR;
      return {};
    }

    peers.push_back(std::move(peer));
  }

  sqlite3_finalize(stmt);
  if (rc)
    *rc = SQLITE_OK;
  return peers;
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::addPeer(const PeerConfig &peer) noexcept {
  using WgKey = WgKey<__THREAD_SAFETY__>;
  using WgPrivateKey = WgPrivateKey<__THREAD_SAFETY__>;
  using WgPublicKey = WgPublicKey<__THREAD_SAFETY__>;

  // Sequential checks
  if (readonly_)
    return WGFORGE_DATABASE_READONLY;
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(peer.record.public_key))
    return WGFORGE_INVALID_ARGUMENT;
  if (peer.record.preshared_key.has_value() &&
      !WgKey::validateB64StringKey(peer.record.preshared_key.value()))
    return WGFORGE_INVALID_ARGUMENT;
  if (peer.record.private_key.has_value() &&
      !WgKey::validateB64StringKey(peer.record.private_key.value()))
    return WGFORGE_INVALID_ARGUMENT;

  // Test whether given public key is made from private key
  if (peer.record.private_key.has_value()) {
    WgPublicKey pbl_key(WgPrivateKey(peer.record.private_key.value()));
    if (pbl_key != peer.record.public_key)
      return WGFORGE_INVALID_ARGUMENT;
  }

  SQLiteReturnCode rc;
  sqlite3_stmt *stmt;

  std::lock_guard lock(mtx_);

  // Begin transaction
  if ((rc = sqlite3_exec(db_.get(), "BEGIN TRANSACTION;", nullptr, nullptr,
                         nullptr)) != SQLITE_OK)
    return rc;

  constexpr const char INSERT_PEER[] = R"(
    INSERT INTO Peers (Public_key, Preshared_key, Private_key,
                       Persistent_keepalive, Description)
    VALUES (?, ?, ?, ?, ?)
  )";

  // Prepare inserting peer
  if ((rc = sqlite3_prepare_v2(db_.get(), INSERT_PEER, -1, &stmt, nullptr)) !=
      SQLITE_OK) {
    sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    return rc;
  }

  // Bind fields
  sqlite3_bind_text(stmt, 1, peer.record.public_key.c_str(), -1, SQLITE_STATIC);

  if (peer.record.preshared_key.has_value())
    sqlite3_bind_text(stmt, 2, peer.record.preshared_key->c_str(), -1,
                      SQLITE_STATIC);
  else
    sqlite3_bind_null(stmt, 2);

  if (peer.record.private_key.has_value())
    sqlite3_bind_text(stmt, 3, peer.record.private_key->c_str(), -1,
                      SQLITE_STATIC);
  else
    sqlite3_bind_null(stmt, 3);

  sqlite3_bind_int(stmt, 4, peer.record.persistent_keepalive);

  if (peer.record.description.has_value())
    sqlite3_bind_text(stmt, 5, peer.record.description->c_str(), -1,
                      SQLITE_STATIC);
  else
    sqlite3_bind_null(stmt, 5);

  // Make step to insert
  if ((rc = sqlite3_step(stmt)) != SQLITE_OK) {
    sqlite3_finalize(stmt);
    sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    if (rc == SQLITE_CONSTRAINT_UNIQUE)
      return WGFORGE_PEER_EXIST;
    return rc;
  }

  sqlite3_finalize(stmt);

  // Get inserted id
  auto peer_id = sqlite3_last_insert_rowid(db_.get());

  // Insert AllowedIPs
  constexpr const char INSERT_ALLOWEDIPS[] =
      "INSERT INTO AllowedIPs (Peer_id, CIDR) VALUES (?, ?)";
  for (const auto &cidr : peer.allowed_ips) {
    if ((rc = sqlite3_prepare_v2(db_.get(), INSERT_ALLOWEDIPS, -1, &stmt,
                                 nullptr)) != SQLITE_OK) {
      sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
      return rc;
    }

    sqlite3_bind_int64(stmt, 1, peer_id);
    sqlite3_bind_text(stmt, 2, cidr.c_str(), -1, SQLITE_STATIC);

    if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
      return rc;
    }

    sqlite3_finalize(stmt);
  }

  sqlite3_exec(db_.get(), "COMMIT;", nullptr, nullptr, nullptr);
  return SQLITE_OK;
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::updatePeer(std::string_view public_key,
                            const PeerRecord &peer) noexcept {
  using WgKey = WgKey<__THREAD_SAFETY__>;

  if (readonly_)
    return WGFORGE_DATABASE_READONLY;
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;

  // Строим запрос с плейсхолдерами
  std::string query = "UPDATE Peers SET Updated_at = CURRENT_TIMESTAMP";

  if (peer.preshared_key.has_value())
    query += ", Preshared_key = ?";
  if (peer.persistent_keepalive)
    query += ", Persistent_keepalive = ?";
  if (peer.description.has_value())
    query += ", Description = ?";

  query += " WHERE Public_key = ? AND Deleted_at IS NULL";

  std::lock_guard lock(mtx_);

  sqlite3_stmt *stmt = nullptr;
  SQLiteReturnCode rc =
      sqlite3_prepare_v2(db_.get(), query.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
    return rc;

  uint8_t param_index = 1;

  // Bind параметров в том же порядке
  if (peer.preshared_key.has_value())
    sqlite3_bind_text(stmt, param_index++, peer.preshared_key->c_str(), -1,
                      SQLITE_STATIC);
  if (peer.persistent_keepalive)
    sqlite3_bind_int(stmt, param_index++, peer.persistent_keepalive);
  if (peer.description.has_value())
    sqlite3_bind_text(stmt, param_index++, peer.description->c_str(), -1,
                      SQLITE_STATIC);

  sqlite3_bind_text(stmt, param_index, public_key.data(), -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  if (rc != SQLITE_DONE) {
    return rc;
  }

  return sqlite3_changes(db_.get()) ? SQLITE_OK : WGFORGE_PEER_NOT_EXIST;
}

std::vector<std::string>
SQLiteDbManager::getAllowedIPs(std::string_view public_key,
                               SQLiteReturnCode *rc) const {

  using WgKey = WgKey<__THREAD_SAFETY__>;
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

  sqlite3_stmt *stmt;
  SQLiteReturnCode ec;
  constexpr const char QUERY[] = R"(
        SELECT 
            a.CIDR
        FROM 
            AllowedIPs a,
            Peers p
        Where a.Peer_id = p.Id and a.Deleted_at IS NOT NULL and p.Deleted_at IS NOT NULL AND p.Public_key = ?
    )";

  std::lock_guard lock(mtx_);
  if ((ec = sqlite3_prepare_v2(db_.get(), QUERY, -1, &stmt, nullptr)) !=
      SQLITE_OK) {
    if (rc)
      *rc = ec;
    return {};
  }

  if ((ec = sqlite3_bind_text(stmt, 1, public_key.data(), -1, SQLITE_STATIC)) !=
      SQLITE_OK) {
    sqlite3_finalize(stmt);
    if (rc)
      *rc = ec;
    return {};
  }

  std::vector<std::string> cidrs;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *pstr;
    __convertText(&pstr, 1, stmt);
    cidrs.emplace_back(pstr);
  }

  sqlite3_finalize(stmt);
  if (rc)
    *rc = SQLITE_OK;
  return cidrs;
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::deletePeer(std::string_view public_key) noexcept {
  using WgKey = WgKey<__THREAD_SAFETY__>;

  if (readonly_)
    return WGFORGE_DATABASE_READONLY;
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;
  if (!WgKey::validateB64StringKey(public_key))
    return WGFORGE_INVALID_ARGUMENT;

  std::lock_guard lock(mtx_);
  sqlite3_exec(db_.get(), "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

  sqlite3_stmt *stmt;
  SQLiteReturnCode rc;

  // 1. Get Peers Id
  constexpr const char GET_ID[] =
      "SELECT Id FROM Peers WHERE Public_key = ? AND Deleted_at IS NULL";

  if ((rc = sqlite3_prepare_v2(db_.get(), GET_ID, -1, &stmt, nullptr)) !=
      SQLITE_OK) {
    sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    return rc;
  }

  sqlite3_bind_text(stmt, 1, public_key.data(), -1, SQLITE_STATIC);

  int64_t peer_id;
  if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    peer_id = sqlite3_column_int64(stmt, 0);
  } else {
    sqlite3_finalize(stmt);
    sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    return rc;
  }
  sqlite3_finalize(stmt);

  // 2. Delete all connected AllowedIPs
  constexpr const char UPDATE_ALLOWEDIPS[] = R"(
      UPDATE AllowedIPs SET Delete_at = CURRENT_TIMESTAMP
      WHERE Peer_id = ? AND Deleted_at IS NULL
  )";

  if ((rc = sqlite3_prepare_v2(db_.get(), UPDATE_ALLOWEDIPS, -1, &stmt,
                               nullptr)) != SQLITE_OK) {
    sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    return rc;
  }

  sqlite3_bind_int64(stmt, 1, peer_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  // 3. Delete peer softly
  constexpr const char UPDATE_PEER[] = R"(
    UPDATE Peers SET Deleted_at = CURRENT_TIMESTAMP
    WHERE Id = ? AND Deleted_at IS NULL
  )";

  if ((rc = sqlite3_prepare_v2(db_.get(), UPDATE_PEER, -1, &stmt, nullptr)) !=
      SQLITE_OK) {
    sqlite3_exec(db_.get(), "ROLLBACK;", nullptr, nullptr, nullptr);
    return rc;
  }

  sqlite3_bind_int64(stmt, 1, peer_id);
  sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  // Commit changes
  sqlite3_exec(db_.get(), "COMMIT;", nullptr, nullptr, nullptr);

  return SQLITE_OK;
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::addAllowedIP(std::string_view public_key,
                              std::string_view cidr) {
  using WgKey = WgKey<__THREAD_SAFETY__>;
  using WgAllowedIP = WgAllowedIP<__THREAD_SAFETY__>;

  if (readonly_)
    return WGFORGE_DATABASE_READONLY;
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;

  // Validate input data
  if (!WgKey::validateB64StringKey(public_key) ||
      !WgAllowedIP::validate(cidr.data()))
    return WGFORGE_INVALID_ARGUMENT;

  sqlite3_stmt *stmt;
  SQLiteReturnCode rc;

  // 1. Get peer's Id
  constexpr const char GET_ID[] =
      "SELECT Id FROM Peers WHERE Public_key = ? AND Deleted_at IS NULL";
  std::lock_guard lock(mtx_);
  if ((rc = sqlite3_prepare_v2(db_.get(), GET_ID, -1, &stmt, nullptr)) !=
      SQLITE_OK) {
    return rc;
  }

  sqlite3_bind_text(stmt, 1, public_key.data(), -1, SQLITE_STATIC);

  int64_t peer_id;
  if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    peer_id = sqlite3_column_int64(stmt, 0);
  } else {
    sqlite3_finalize(stmt);
    return rc;
  }
  sqlite3_finalize(stmt);

  // 2. Check whether such allowed ip for concrete peer exist or not
  constexpr const char GET_ALLOWEDIP[] = R"(
    SELECT
        COUNT(Deleted_at) AS deleted,
        COUNT(*) - COUNT(Deleted_at) as active
    FROM    
        AllowedIPs a
    WHERE a.Peer_id = ? AND a.CIDR = ?
  )";

  if ((rc = sqlite3_prepare_v2(db_.get(), GET_ALLOWEDIP, -1, &stmt, nullptr)) !=
      SQLITE_OK)
    return rc;

  sqlite3_bind_int64(stmt, 1, peer_id);
  sqlite3_bind_text(stmt, 2, cidr.data(), -1, SQLITE_STATIC);

  int64_t deleted_count, active_count;
  if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    deleted_count = sqlite3_column_int64(stmt, 0);
    active_count = sqlite3_column_int64(stmt, 1);
  } else {
    sqlite3_finalize(stmt);
    return rc;
  }
  sqlite3_finalize(stmt);

  if (active_count)
    return WGFORGE_ALLOWEDIP_EXIST;

  // 3. If AllowedIP exists but "deleted" then recover it. Otherwise create new
  // one
  if (deleted_count) {
    constexpr const char RECOVER_ALLOWEDIP[] = R"(
        UPDATE AllowedIPs
        SET Deleted_at = NULL,
            Updated_at = CURRENT_TIMESTAMP
            WHERE Peer_id = ? AND CIDR = ? AND Deleted_at IS NOT NULL
      )";

    if ((rc = sqlite3_prepare_v2(db_.get(), RECOVER_ALLOWEDIP, -1, &stmt,
                                 nullptr)) != SQLITE_OK)
      return rc;

  } else {
    constexpr const char INSERT_ALLOWEDIP[] =
        "INSERT INTO AllowedIPs (Peer_id, CIDR) "
        "VALUES (?, ?)";
    if ((rc = sqlite3_prepare_v2(db_.get(), INSERT_ALLOWEDIP, -1, &stmt,
                                 nullptr)) != SQLITE_OK)
      return rc;
  }

  sqlite3_bind_int64(stmt, 1, peer_id);
  sqlite3_bind_text(stmt, 2, cidr.data(), -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc;
}

SQLiteDbManager::SQLiteReturnCode
SQLiteDbManager::removeAllowedIP(std::string_view public_key,
                                 std::string_view cidr) {
  using WgKey = WgKey<__THREAD_SAFETY__>;
  using WgAllowedIP = WgAllowedIP<__THREAD_SAFETY__>;

  if (readonly_)
    return WGFORGE_DATABASE_READONLY;
  if (!isOpen())
    return WGFORGE_CONNECTION_CLOSED;

  // Validate input data
  if (!WgKey::validateB64StringKey(public_key) ||
      !WgAllowedIP::validate(cidr.data()))
    return WGFORGE_INVALID_ARGUMENT;

  sqlite3_stmt *stmt;
  SQLiteReturnCode rc;

  // 1. Get peer's Id
  constexpr const char GET_ID[] =
      "SELECT Id FROM Peers WHERE Public_key = ? AND Deleted_at IS NULL";
  std::lock_guard lock(mtx_);
  if ((rc = sqlite3_prepare_v2(db_.get(), GET_ID, -1, &stmt, nullptr)) !=
      SQLITE_OK) {
    return rc;
  }

  sqlite3_bind_text(stmt, 1, public_key.data(), -1, SQLITE_STATIC);

  int64_t peer_id;
  if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    peer_id = sqlite3_column_int64(stmt, 0);
  } else {
    sqlite3_finalize(stmt);
    return rc;
  }
  sqlite3_finalize(stmt);

  // 2. Check whether allowed ip to "delete" exist
  constexpr const char GET_ALLOWEDIP[] = R"(
    SELECT
        a.Id
    FROM
        AllowedIPs a
    WHERE a.Peer_id = ? AND a.CIDR = ? AND a.Deleted_at IS NULL
 )";

  if ((rc = sqlite3_prepare_v2(db_.get(), GET_ALLOWEDIP, -1, &stmt, nullptr)) !=
      SQLITE_OK)
    return rc;

  sqlite3_bind_int64(stmt, 1, peer_id);
  sqlite3_bind_text(stmt, 2, cidr.data(), -1, SQLITE_STATIC);

  int64_t allowed_ip_id;
  if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    allowed_ip_id = sqlite3_column_int64(stmt, 0);
  } else {
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE)
      return WGFORGE_ALLOWEDIP_NOT_EXIST;
    return rc;
  }
  sqlite3_finalize(stmt);

  // 3. "Delete" allowedip
  constexpr const char DELETE_ALLOWEDIP[] = R"(
    UPDATE AllowedIPs
        SET Deleted_at = CURRENT_TIMESTAMP,
            Updated_at = CURRENT_TIMESTAMP
            WHERE Peer_id = ? AND CIDR = ? AND Deleted_at IS NULL  
  )";

  if ((rc = sqlite3_prepare_v2(db_.get(), DELETE_ALLOWEDIP, -1, &stmt,
                               nullptr)) != SQLITE_OK)
    return rc;

  sqlite3_bind_int64(stmt, 1, peer_id);
  sqlite3_bind_text(stmt, 2, cidr.data(), -1, SQLITE_STATIC);

  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);

  return rc;
}

inline void SQLiteDbManager::__convertText(const char **pstr, int col,
                                           sqlite3_stmt *stmt) const noexcept {
  *pstr = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
}

PeerRecord SQLiteDbManager::__fillPeerRecord(sqlite3_stmt *stmt) const {
  PeerRecord peer;
  peer.id = sqlite3_column_int64(stmt, 0);

  const char *pstr;
  __convertText(&pstr, 1, stmt);
  if (std::strlen(pstr))
    peer.public_key = pstr;

  __convertText(&pstr, 2, stmt);
  if (std::strlen(pstr))
    peer.private_key = pstr;

  peer.persistent_keepalive = sqlite3_column_int(stmt, 3);

  __convertText(&pstr, 4, stmt);
  if (std::strlen(pstr))
    peer.description = pstr;

  __convertText(&pstr, 5, stmt);
  if (std::strlen(pstr))
    peer.created_at = pstr;

  __convertText(&pstr, 6, stmt);
  if (std::strlen(pstr))
    peer.updated_at = pstr;

  return peer;
}

inline void SQLiteDbManager::__closeConnectionUnsafe() noexcept {
  if (db_) {
    sqlite3_close_v2(db_.get());
    db_.reset();
  }
}

void SQLiteDbManager::DbDeleter::operator()(sqlite3 *p) noexcept {
  if (p)
    sqlite3_close_v2(p);
}

#undef __THREAD_SAFETY__

} // namespace WgForge
