/*
 * config_error_handler.h
 * wgforge-core — Core library for wgforge-server (C) and wgforge-cli (C)
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

#ifndef WGFORGE_CONFIG_ERROR_HANDLER_H
#define WGFORGE_CONFIG_ERROR_HANDLER_H

#include <string>

#include "config_manager.hpp"

namespace WgForge {

/**
 * @brief Handle config IO operation errors by error code. Based on libeconf  .
 */
class ConfigErrorHandler {
public:
  /**
   * @brief Convert rc to string.
   * @param rc return code
   * @param ok sets true if <TT>rc == 0</TT>, false otherwise
   * @retval human-readable string representation of string if <TT>rc == 0</TT>
   * @retval zero string if <TT>rc != 0</TT>
   */
  static std::string toString(ConfigManager::ConfigReturnCode rc, bool* ok = nullptr);

  /**
   * @brief Convert read rc to string.
   * @param rc return code
   * @param group for which group instance string
   * @param option for which option instance string
   * @param ok sets true if <TT>rc == 0</TT>, false otherwise
   * @retval human-readable string representation of string if <TT>rc == 0</TT>
   * @retval zero string if <TT>rc != 0</TT>
   */
  static std::string readToString(ConfigManager::ConfigReturnCode rc, std::string_view group,
                                  std::string_view option, bool* ok = nullptr);

  /**
   * @brief Convert write rc to string.
   * @param rc return code
   * @param msg custom error message precending to ConfigErrorManager::toString
   * @param ok sets true if <TT>rc == 0</TT>, false otherwise
   * @retval human-readable string representation of string if <TT>rc == 0</TT>
   * @retval zero string if <TT>rc != 0</TT>
   */
  static std::string writeToString(ConfigManager::ConfigReturnCode rc, std::string_view msg,
                                   bool* ok = nullptr);

  /**
   * @brief Throw exception if operation failed. 
   * @param rc return code
   * @throw std::runtime_error if <TT>rc != 0</TT>
   */
  static void throwIfError(ConfigManager::ConfigReturnCode rc);
};

}  // namespace WgForge

#endif  // WGFORGE_CONFIG_ERROR_HANDLER_H
