/*
 * config_read_functions.hpp
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

#ifndef WGFORGE_LIBECONF_READERS_HPP
#define WGFORGE_LIBECONF_READERS_HPP

#include <libeconf.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "config_read_register.h"

namespace WgForge {
namespace LibeconfReaders {

inline int readInt(econf_file* config, std::string_view group, std::string_view path,
                   ConfigReadRegister::ReadReturnCode* rc) {
  int result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getIntValue(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

inline int64_t readInt64(econf_file* config, std::string_view group, std::string_view path,
                         ConfigReadRegister::ReadReturnCode* rc) {
  int64_t result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getInt64Value(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

inline bool readBool(econf_file* config, std::string_view group, std::string_view path,
                     ConfigReadRegister::ReadReturnCode* rc) {
  bool result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getBoolValue(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

inline uint32_t readUInt(econf_file* config, std::string_view group, std::string_view path,
                         ConfigReadRegister::ReadReturnCode* rc) {
  uint32_t result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getUIntValue(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

inline uint64_t readUInt64(econf_file* config, std::string_view group, std::string_view path,
                           ConfigReadRegister::ReadReturnCode* rc) {
  uint64_t result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getUInt64Value(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

inline float readFloat(econf_file* config, std::string_view group, std::string_view path,
                       ConfigReadRegister::ReadReturnCode* rc) {
  float result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getFloatValue(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

inline double readDouble(econf_file* config, std::string_view group, std::string_view path,
                         ConfigReadRegister::ReadReturnCode* rc) {
  double result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getDoubleValue(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  return result;
}

std::string readString(econf_file* config, std::string_view group, std::string_view path,
                       ConfigReadRegister::ReadReturnCode* rc) {
  char* result;
  ConfigReadRegister::ReadReturnCode lrc =
      econf_getStringValue(config, group.data(), path.data(), &result);
  if (rc)
	*rc = lrc;
  std::string str(result);
  free(result);
  return str;
}

}  // namespace LibeconfReaders
}  // namespace WgForge

#endif  // WGFORGE_LIBECONF_READERS_HPP
