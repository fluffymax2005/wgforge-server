/*
 * sqlite_error_handler.h
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

#ifndef __WGFORGE_SQLITE_ERROR_H__
#define __WGFORGE_SQLITE_ERROR_H__

#include <string>

#include "sqlite_db_manager.h"

namespace WgForge {

/**
 * @brief Handle database IO operation errors by error code. Based on SQLite3. 
 */
class SQLiteErrorHandler {
public:
  /**
   * @brief Convert rc to string.
   * @param rc return code
   * @param ok sets true if <TT>rc == 0</TT>, false otherwise
   * @retval human-readable string representation of string if <TT>rc == 0</TT>
   * @retval zero string if <TT>rc != 0</TT>
   */
  static std::string toString(SQLiteDbManager::SQLiteReturnCode rc, bool* ok = nullptr);

  /**
   * @brief Throw exception if operation failed. 
   * @param rc return code
   * @throw std::runtime_error if <TT>rc != 0</TT>
   */
  static void throwIfError(SQLiteDbManager::SQLiteReturnCode rc);
};

} // namespace WgForge

#endif // __WGFORGE_SQLITE_ERROR_H__
