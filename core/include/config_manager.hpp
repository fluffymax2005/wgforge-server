/*
 * config_manager.hpp
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

#ifndef WGFORGE_CONFIG_MANAGER_HPP
#define WGFORGE_CONFIG_MANAGER_HPP

#include <libeconf.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string_view>

#include "config_read_write_register.h"
#include "wgforge.h"

namespace WgForge {

/**
 * @brief Manages configurations via libeconf. Thread-safety: safe.
 */
class ConfigManager {
public:
  /**
   * @brief Return code for instance methods. Non zero if method call succeded.
   */
  using ConfigReturnCode = int;

  /**
   * @brief Function wrapper which provides strategy to create config.
   * @param path full path to config
   * @param rc return code to indicating success
   * @note Requires noexcept qualificator.
   * @see ConfigManager::create.
   */
  using CreateConfigStrategy =
      std::move_only_function<econf_file*(std::string_view path, ConfigReturnCode& rc) noexcept>;

  /**
   * @brief Default constructor. Initializes default readers and writers.
   * @see ConfigManager::setReadWriteRegister.
   */
  ConfigManager();

  /**
   * @brief Open config by path.
   * @param path full path to file
   * @param readonly whether config opens in readonly mode.
   */
  explicit ConfigManager(std::string_view path, bool readonly = true) noexcept;

  /**
   * @brief Construct instance setting config context. Effectively calls ConfigManager::open.
   */
  explicit ConfigManager(econf_file* config, bool readonly = true) noexcept;


  /**
   * @brief Set read-writer register to permorm IO operations
   * @param reg register. Requrements: ConfigReadWriteRegister must be base class of reg
   */
  template<typename TRegister>
  void setReadWriteRegister(TRegister&& reg);

  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  ConfigManager(ConfigManager&& other) noexcept;
  ConfigManager& operator=(ConfigManager&& other) noexcept;

  ~ConfigManager() = default;

  /**
   * @brief Check whether config is opened.
   * @retval true if opened
   * @retval false otherwise
   */
  inline bool isOpen() const noexcept;

  /**
   * @see ConfigManager::isOpen.
   */
  explicit operator bool() const noexcept;

  /**
   * @brief Open config. Closes current connection if <TT>ConfigManager::isOpen == true</TT>.
   * @param config libeconf config file instance
   * @param readonly whether config opens in readonly mode.
   * @retval WGFORGE_OK if succeded
   */
  ConfigReturnCode open(std::string_view path, bool readonly = true);

  /**
   * @brief Close config. If <TT>ConfigManager::isOpen == false</TT> then does nothing
   */
  inline void close() noexcept;

  /**
   * @brief Set manager context. If <TT>ConfigManager::isOpen == true</TT> closes current connection.
   * @param config libeconf config file instance
   * @param readonly whether config opens in readonly mode.
   * @warning config is captured and it's lifetime is managed by instance. If you want to release ownership use ConfigManager::release. 
   */
  void set(econf_file* config, bool readonly = true) noexcept;

  /**
   * @brief Release ownership of current config context.
   * @retval config context pointer. May be nullptr if <TT>ConfigManager::isOpen == false</TT> 
   */
  inline econf_file* release() noexcept;

  /**
   * @brief Starts transaction to config.
   * @retval WGFORGE_OK if succeded
   */
  ConfigReturnCode beginTransaction() noexcept;

  /**
   * @brief Commit current changes.
   * @retval WGFORGE_OK if succeded
   */
  ConfigReturnCode commit();

  /**
   * @brief Rolls back changes to config.
   * @retval WGFORGE_OK if succeded
   */
  ConfigReturnCode rollBack() noexcept;

  /**
   * @brief Check whether instance is in transaction state.
   * @retval true if in transaction
   * @retval false otherwise
   */
  inline bool inTransaction() const noexcept;

  /**
   * @brief Get config filepath.
   * @retval filepath if presented
   * @retval zero string otherwise
   */
  std::string path() const;

  /**
   * @brief Creates new config with given strategy. If creation failed, then ownership of current config is not released.
   * @param path full path to file
   * @param strategy functor to create new config.
   * @see ConfigManager::CreateConfigStrategy for requrements.
   */
  ConfigReturnCode create(std::string_view path, CreateConfigStrategy strategy) noexcept;

  /**
   * @brief Write value to config.
   * @param group group to write.
   * @param option to assign value to.
   * @param value value to assign to.
   * @retval WGFORGE_OK if succeded
   */
  template<typename T>
  ConfigWriteRegister::WriteReturnCode write(std::string_view group, std::string_view option,
                                             const T& value);

  /**
   * @brief Try to read value.
   * @param group group to read value from
   * @param option option to extract value from 
   * @param rc optional return code indicating success
   * @retval std::nullopt if reading failed
   * @retval std::optional containing target value
   */
  template<typename T>
  std::optional<T> tryRead(std::string_view group, std::string_view option,
                           ConfigReadRegister::ReadReturnCode* rc = nullptr) const;
  /**
   * @brief Read value.
   * @param group group to read value from
   * @param option option to extract value from 
   * @retval value instance
   * @throw std::runtime_error if reading failed.
   */
  template<typename T>
  T read(std::string_view group, std::string_view option) const;

private:
  struct ConfigDeleter {
	void operator()(econf_file* p) noexcept;
  };

  std::unique_ptr<econf_file, ConfigDeleter> cfg_;
  std::unique_ptr<econf_file, ConfigDeleter> snapshot_;

  ConfigReadWriteRegister rw_reg_;
  bool readonly_;
  bool in_transaction_ = false;
  mutable std::mutex mtx_;

  void registerDefaultReadersImpl();
  void registerDefaultWritersImpl();
};

template<typename T>
ConfigWriteRegister::WriteReturnCode ConfigManager::write(std::string_view group,
                                                          std::string_view option, const T& value) {
  if (!isOpen())
	return WGFORGE_CONFIG_CLOSED;

  std::lock_guard lock(mtx_);
  if (readonly_)
	return WGFORGE_CONFIG_READONLY;

  return rw_reg_.write(cfg_.get(), group, option, value);
}

template<typename TRegister>
void ConfigManager::setReadWriteRegister(TRegister&& reg) {
  std::lock_guard lock(mtx_);
  rw_reg_ = std::forward<TRegister>(reg);
}

template<typename T>
std::optional<T> ConfigManager::tryRead(std::string_view group, std::string_view option,
                                        ConfigReadRegister::ReadReturnCode* rc) const {
  if (!isOpen()) {
	if (rc)
	  *rc = WGFORGE_CONFIG_CLOSED;
	return std::nullopt;
  }

  ConfigReadRegister::ReadReturnCode lrc = 0;
  std::lock_guard lock(mtx_);
  auto result = rw_reg_.tryRead<T>(cfg_.get(), group, option, &lrc);

  if (rc)
	*rc = lrc;
  return result;
}

template<typename T>
T ConfigManager::read(std::string_view group, std::string_view option) const {
  if (!isOpen())
	throw std::runtime_error("Config is not opened");

  return rw_reg_.read<T>(cfg_.get(), group, option);
}

inline bool ConfigManager::isOpen() const noexcept {
  std::lock_guard lock(mtx_);
  return cfg_.get();
}

inline void ConfigManager::close() noexcept {
  std::lock_guard lock(mtx_);
  cfg_.reset();
}

inline econf_file* ConfigManager::release() noexcept {
  std::lock_guard lock(mtx_);
  return cfg_.release();
}

inline bool ConfigManager::inTransaction() const noexcept {
  std::lock_guard lock(mtx_);
  return in_transaction_;
}

}  // namespace WgForge

#endif  // WGFORGE_CONFIG_MANAGER_HPP
