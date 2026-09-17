#ifndef __WGFORGE_SQLITE_ERROR_H__
#define __WGFORGE_SQLITE_ERROR_H__

#include <string>

#include "sqlite_db_manager.h"

namespace WgForge {

class SQLiteErrorHandler {
public:
  static std::string toString(SQLiteDbManager::SQLiteReturnCode rc, bool* ok = nullptr);
  static void throwIfError(SQLiteDbManager::SQLiteReturnCode rc);
};

} // namespace WgForge

#endif // __WGFORGE_SQLITE_ERROR_H__
