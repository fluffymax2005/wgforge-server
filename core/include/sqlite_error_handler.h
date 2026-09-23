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

class SQLiteErrorHandler {
public:
  static std::string toString(SQLiteDbManager::SQLiteReturnCode rc, bool* ok = nullptr);
  static void throwIfError(SQLiteDbManager::SQLiteReturnCode rc);
};

} // namespace WgForge

#endif // __WGFORGE_SQLITE_ERROR_H__
