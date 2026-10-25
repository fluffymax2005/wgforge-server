/*
 * config_write_register.hpp
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

#ifndef WGFORGE_CONFIG_WRITE_REGISTER_HPP
#define WGFORGE_CONFIG_WRITE_REGISTER_HPP

#include <libeconf.h>

#include <any>
#include <functional>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include "wgforge.h"

namespace WgForge {

/**
 * @brief Register containing writing strategies.
 */
class ConfigWriteRegister {
public:
  /**
   * @brief Result of writing operation. Non zero if not succeded.
   */
  using WriteReturnCode = int;

  /**
   * @brief High level wrapper to provide writing. Do **not** use directly.
   * @see ConfigReadRegister::ReadFunc.
   */
  using UniversalWriteFunc = std::function<WriteReturnCode(
      econf_file* config, std::string_view group, std::string_view option, const std::any& value)>;

  /**
   * @brief Function wrapper to provide strategy to write to config.
   * @param config config context
   * @param group group to write to
   * @param option option to assign value to
   * @param value target value
   * @retval rc return code indication success
   */
  template<typename T>
  using WriteFunc = std::function<WriteReturnCode(econf_file* config, std::string_view group,
                                                  std::string_view option, const T& value)>;

  ConfigWriteRegister() = default;

  ConfigWriteRegister(const ConfigWriteRegister&) = default;
  ConfigWriteRegister& operator=(const ConfigWriteRegister&) = default;

  ConfigWriteRegister(ConfigWriteRegister&&) = default;
  ConfigWriteRegister& operator=(ConfigWriteRegister&&) = default;

  /**
   * @brief Register writer. If <TT>func == false</TT> does nothing.
   * @param func reader strategy
   */
  template<typename T>
  void registerWriter(WriteFunc<T> func);

  /**
   * @brief Remove writer of T type. If T reader is not presented does nothing.
   */
  template<typename T>
  void removeWriter() noexcept;

  /**
   * @brief Write value.
   * @param config config context
   * @param group group to read from
   * @param option option to extract value from
   * @param value target value
   * @retval return code indicating success
   * @throw std::runtime_error if reading failed.
   */
  template<typename T>
  WriteReturnCode write(econf_file* config, std::string_view group, std::string_view option,
                        const T& value);

protected:
  /**
   * @brief Map to writers.
   */
  std::unordered_map<std::type_index, UniversalWriteFunc> writers_;
};

template<typename T>
void ConfigWriteRegister::registerWriter(WriteFunc<T> func) {
  writers_[std::type_index(typeid(T))] = [func](econf_file* config, std::string_view group,
                                                std::string_view option,
                                                const std::any& value) -> WriteReturnCode {
	try {
	  const T& typed = std::any_cast<const T&>(value);
	  return func(config, group, option, typed);
	} catch (const std::bad_any_cast&) {
	  return WGFORGE_CONFIG_INVALID_TYPE;
	}
  };
}

template<typename T>
void ConfigWriteRegister::removeWriter() noexcept {
  auto it = writers_.find(std::type_index(typeid(T)));
  if (it != writers_.end())
	writers_.erase(it);
}

template<typename T>
ConfigWriteRegister::WriteReturnCode ConfigWriteRegister::write(econf_file* config,
                                                                std::string_view group,
                                                                std::string_view option,
                                                                const T& value) {
  auto it = writers_.find(std::type_index(typeid(T)));
  if (it == writers_.end())
	return WGFORGE_CONFIG_WRITER_NOT_FOUND;

  WriteReturnCode rc = it->second(config, group, option, std::any(value));
  return rc;
}

}  // namespace WgForge

#endif  // WGFORGE_CONFIG_WRITE_REGISTER_HPP
