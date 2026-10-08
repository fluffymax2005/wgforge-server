/*
 * sqlite_error_handler.cpp
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

#include "sqlite_error_handler.h"

#include <stdexcept>

#include "wgforge.h"

namespace WgForge {

std::string SQLiteErrorHandler::toString(SQLiteDbManager::SQLiteReturnCode rc, bool* ok) {
  if (ok)
	*ok = rc == SQLITE_OK;

  switch (rc) {
  case SQLITE_OK                  : return "";
  case SQLITE_ERROR               : return "SQLite error";
  case SQLITE_BUSY                : return "Database is busy";
  case SQLITE_CONSTRAINT          : return "Constraint violation";
  case SQLITE_NOTFOUND            : return "Record not found";
  case SQLITE_READONLY            : return "Database is read-only";
  case SQLITE_IOERR               : return "I/O error";
  case SQLITE_CORRUPT             : return "Database corrupted";
  case SQLITE_CANTOPEN            : return "Cannot open database";
  case SQLITE_NOMEM               : return "Out of memory";

  case WGFORGE_INVALID_ARGUMENT   : return "Invalid argument provided";
  case WGFORGE_RUNTIME_ERROR      : return "Runtime error";
  case WGFORGE_RECORD_EXIST       : return "Record already exists";
  case WGFORGE_CONNECTION_CLOSED  : return "Database connection is closed";
  case WGFORGE_PEER_EXIST         : return "Peer already exists";
  case WGFORGE_PEER_NOT_EXIST     : return "Peer not found";
  case WGFORGE_ALLOWEDIP_EXIST    : return "AllowedIP already exists";
  case WGFORGE_ALLOWEDIP_NOT_EXIST: return "AllowedIP not found";
  case WGFORGE_DATABASE_READONLY  : return "Database is read-only";
  case WGFORGE_DATABASE_STRATEGY_NO_TARGET:
	return "No callback function provided for given context";

  default: return "Unknown SQLITE error code: " + std::to_string(rc);
  }
}

void SQLiteErrorHandler::throwIfError(SQLiteDbManager::SQLiteReturnCode rc) {
  if (rc != SQLITE_OK)
	throw std::runtime_error(toString(rc));
}

}  // namespace WgForge
