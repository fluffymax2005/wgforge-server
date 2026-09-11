#ifndef __WGFORGE_SQLITEDBMANAGER_H__
#define __WGFORGE_SQLITEDBMANAGER_H__

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <vector>

#include "wgforge.h"

namespace WgForge {

struct PeerRecord {
  int64_t id;
  std::string public_key;
  std::optional<std::string> preshared_key;
  std::optional<std::string> private_key;
  uint16_t persistent_keepalive;
  std::optional<std::string> description;
  std::string created_at;
  std::optional<std::string> updated_at;
  std::optional<std::string> deleted_at;
};

struct AllowedIPRecord {
  int64_t id;
  int64_t peer_id;
  std::string cidr;
  std::string created_at;
  std::optional<std::string> updated_at;
  std::optional<std::string> deleted_at;
};

struct PeerConfig {
  PeerRecord record;
  std::vector<std::string> allowed_ips;
};

class SQLiteDbManager {
public:
  using SQLiteReturnCode = int;
  using CallbackFunction = int (*)(void *, int, char **, char **);

  SQLiteDbManager() noexcept = default;
  explicit SQLiteDbManager(std::string_view path,
                           bool readonly = true) noexcept;

  SQLiteDbManager(const SQLiteDbManager &) = delete;
  SQLiteDbManager &operator=(const SQLiteDbManager &) = delete;

  SQLiteDbManager(SQLiteDbManager &&other) noexcept;
  SQLiteDbManager &operator=(SQLiteDbManager &&other) noexcept;

  ~SQLiteDbManager() noexcept = default;

  explicit operator bool() const noexcept;

  SQLiteReturnCode openConnection(std::string_view path, bool readonly = true);
  inline void closeConnection() noexcept;
  inline bool isOpen() const noexcept;

  SQLiteReturnCode rawSql(std::string_view query,
                          CallbackFunction strategy = nullptr,
                          void *context = nullptr) noexcept;

  SQLiteReturnCode getPeer(std::string_view public_key, PeerConfig &peer) const;
  std::vector<PeerConfig> getAllPeers(SQLiteReturnCode *rc = nullptr) const;
  SQLiteReturnCode addPeer(const PeerConfig &peer) noexcept;
  SQLiteReturnCode updatePeer(std::string_view publiс_key,
                              const PeerRecord &peer) noexcept;
  SQLiteReturnCode deletePeer(std::string_view public_key) noexcept;

  SQLiteReturnCode addAllowedIP(std::string_view public_key,
                                std::string_view cidr);
  SQLiteReturnCode removeAllowedIP(std::string_view public_key,
                                   std::string_view cidr);
  std::vector<std::string> getAllowedIPs(std::string_view public_key,
                                         SQLiteReturnCode *rc = nullptr) const;

private:
  struct DbDeleter {
    void operator()(sqlite3 *p) noexcept;
  };

  inline void __closeConnectionUnsafe() noexcept;
  inline void __convertText(const char **pstr, int col,
                            sqlite3_stmt *stmt) const noexcept;
  PeerRecord __fillPeerRecord(sqlite3_stmt *stmt) const;

  std::unique_ptr<sqlite3, DbDeleter> db_;
  bool readonly_;
  mutable std::mutex mtx_;
};

} // namespace WgForge

#endif // __WGFORGE_SQLITEDBMANAGER_H__
