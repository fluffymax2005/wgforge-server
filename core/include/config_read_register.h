/*
 * config_read_register.h
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

#ifndef __WGFORGE_CONFIG_READ_REGISTER_H__
#define __WGFORGE_CONFIG_READ_REGISTER_H__

#include <libeconf.h>

#include <any>
#include <format>
#include <functional>
#include <optional>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include "wgforge.h"

namespace WgForge {

class ConfigReadRegister {
public:
  using ReadReturnCode = int;

  using UniversalReadFunc = std::function<std::any(econf_file* config, std::string_view group,
                                                   std::string_view option, ReadReturnCode* rc)>;

  template<typename T>
  using ReadFunc = std::function<T(econf_file* config, std::string_view group,
                                   std::string_view option, ReadReturnCode* rc)>;

  ConfigReadRegister() = default;

  ConfigReadRegister(const ConfigReadRegister&) = default;
  ConfigReadRegister& operator=(const ConfigReadRegister&) = default;

  ConfigReadRegister(ConfigReadRegister&&) = default;
  ConfigReadRegister& operator=(ConfigReadRegister&&) = default;

  template<typename T>
  void registerReader(ReadFunc<T> func);

  template<typename T>
  void removeReader();

  template<typename T>
  T read(econf_file* config, std::string_view group, std::string_view option) const;

  template<typename T>
  T read(econf_file* config, std::string_view group, std::string_view option,
         ReadReturnCode& rc) const;

  template<typename T>
  std::optional<T> tryRead(econf_file* config, std::string_view group, std::string_view option,
                           ReadReturnCode* rc = nullptr) const;

protected:
  std::unordered_map<std::type_index, UniversalReadFunc> readers_;

  template<typename T>
  T readWithCode(econf_file* config, std::string_view group, std::string_view option,
                 ReadReturnCode& rc) const;
};

template<typename T>
void ConfigReadRegister::registerReader(ReadFunc<T> func) {
  readers_[std::type_index(typeid(T))] = [func](econf_file* config, std::string_view group,
                                                std::string_view option,
                                                ReadReturnCode* rc) -> std::any {
	return std::any(func(config, group, option, rc));
  };
}

template<typename T>
void ConfigReadRegister::removeReader() {
  auto it = readers_.find(std::type_index(typeid(T)));
  if (it != readers_.end())
	readers_.erase(it);
}

template<typename T>
T ConfigReadRegister::read(econf_file* config, std::string_view group,
                           std::string_view option) const {
  ReadReturnCode rc = 0;
  T result = readWithCode<T>(config, group, option, rc);
  if (rc)
	throw std::runtime_error(std::format("Read config error with code: {}", rc));
  return result;
}

template<typename T>
T ConfigReadRegister::read(econf_file* config, std::string_view group, std::string_view option,
                           ReadReturnCode& rc) const {
  return readWithCode<T>(config, group, option, rc);
}

template<typename T>
std::optional<T> ConfigReadRegister::tryRead(econf_file* config, std::string_view group,
                                             std::string_view option, ReadReturnCode* rc) const {
  ReadReturnCode lrc;
  T result = readWithCode<T>(config, group, option, lrc);

  if (lrc) {
	if (rc)
	  *rc = lrc;
	return std::nullopt;
  }

  return result;
}

template<typename T>
T ConfigReadRegister::readWithCode(econf_file* config, std::string_view group,
                                   std::string_view option, ReadReturnCode& rc) const {
  auto it = readers_.find(std::type_index(typeid(T)));
  if (it == readers_.end()) {
	rc = WGFORGE_CONFIG_READER_NOT_FOUND;
	return T{};
  }

  rc = 0;
  std::any result = it->second(config, group, option, &rc);
  try {
	return std::any_cast<T>(result);
  } catch (const std::bad_any_cast&) {
	rc = WGFORGE_CONFIG_INVALID_TYPE;
	return T{};
  }
}

}; // namespace WgForge

#endif // __WGFORGE_CONFIG_READ_REGISTER_H__
