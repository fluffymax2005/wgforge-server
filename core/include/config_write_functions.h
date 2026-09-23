/*
 * config_write_function.h
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

#ifndef __WGFORGE_LIBECONF_WRITERS_H__
#define __WGFORGE_LIBECONF_WRITERS_H__

#include <libeconf.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "config_write_register.h"

namespace WgForge {
namespace LibeconfWriters {

inline ConfigWriteRegister::WriteReturnCode writeBool(econf_file* config, std::string_view group,
                                                      std::string_view option, const bool& value) {
  const char* s = value ? "true" : "false";
  return econf_setBoolValue(config, group.data(), option.data(), s);
}

inline ConfigWriteRegister::WriteReturnCode writeInt(econf_file* config, std::string_view group,
                                                     std::string_view option, const int& value) {
  return econf_setIntValue(config, group.data(), option.data(), value);
}

inline ConfigWriteRegister::WriteReturnCode writeInt64(econf_file* config, std::string_view group,
                                                       std::string_view option,
                                                       const int64_t& value) {
  return econf_setInt64Value(config, group.data(), option.data(), value);
}

inline ConfigWriteRegister::WriteReturnCode writeUInt(econf_file* config, std::string_view group,
                                                      std::string_view option,
                                                      const uint32_t& value) {
  return econf_setUIntValue(config, group.data(), option.data(), value);
}

inline ConfigWriteRegister::WriteReturnCode writeUInt64(econf_file* config, std::string_view group,
                                                        std::string_view option,
                                                        const uint64_t& value) {
  return econf_setUInt64Value(config, group.data(), option.data(), value);
}

inline ConfigWriteRegister::WriteReturnCode writeFloat(econf_file* config, std::string_view group,
                                                       std::string_view option,
                                                       const float& value) {
  return econf_setFloatValue(config, group.data(), option.data(), value);
}

inline ConfigWriteRegister::WriteReturnCode writeDouble(econf_file* config, std::string_view group,
                                                        std::string_view option,
                                                        const double& value) {
  return econf_setDoubleValue(config, group.data(), option.data(), value);
}

inline ConfigWriteRegister::WriteReturnCode writeString(econf_file* config, std::string_view group,
                                                        std::string_view option,
                                                        const std::string& value) {
  return econf_setStringValue(config, group.data(), option.data(), value.c_str());
}

inline ConfigWriteRegister::WriteReturnCode writeStringView(econf_file* config,
                                                            std::string_view group,
                                                            std::string_view option,
                                                            const std::string_view& value) {
  return econf_setStringValue(config, group.data(), option.data(), value.data());
}

} // namespace LibeconfWriters
} // namespace WgForge

#endif // __WGFORGE_LIBECONF_WRITERS_H__
