#ifndef __WGFORGE_CONFIG_MANAGER_H__
#define __WGFORGE_CONFIG_MANAGER_H__

#include <libeconf.h>
#include <memory>
#include <mutex>
#include <string_view>

#include "config_read_write_register.h"
#include "wgforge.h"

namespace WgForge {

class ConfigManager {
public:
  using ConfigReturnCode = int;
  using CreateConfigStrategy = std::move_only_function<econf_file *(
      std::string_view path, ConfigReturnCode &rc) noexcept>;

  ConfigManager();
  explicit ConfigManager(std::string_view path, bool readonly = true) noexcept;

  template <typename TRegister> void setReadWriteRegister(TRegister &&reg);

  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;

  ConfigManager(ConfigManager &&other) noexcept;
  ConfigManager &operator=(ConfigManager &&other) noexcept;

  inline bool isOpen() const noexcept;
  explicit operator bool() const noexcept;

  ConfigReturnCode open(std::string_view path, bool readonly = true);
  inline void close() noexcept;

  ConfigReturnCode create(std::string_view path,
                          CreateConfigStrategy strategy) noexcept;

  template <typename T>
  ConfigWriteRegister::WriteReturnCode
  write(std::string_view group, std::string_view option, const T &value);

  template <typename T>
  std::optional<T> tryRead(std::string_view group, std::string_view option,
                           ConfigReadRegister::ReadReturnCode *rc = nullptr);

  template <typename T> T read(std::string_view group, std::string_view option);

private:
  struct ConfigDeleter {
    void operator()(econf_file *p) noexcept;
  };

  std::unique_ptr<econf_file, ConfigDeleter> cfg_;
  ConfigReadWriteRegister rw_reg_;
  bool readonly_;
  std::mutex mtx_;

  void registerDefaultReadersImpl();
  void registerDefaultWritersImpl();
};

template <typename T>
ConfigWriteRegister::WriteReturnCode
ConfigManager::write(std::string_view group, std::string_view option,
                     const T &value) {
  if (!isOpen())
    return WGFORGE_CONFIG_CLOSED;

  std::lock_guard lock(mtx_);
  if (readonly_)
    return WGFORGE_CONFIG_READONLY;

  return rw_reg_.write(cfg_.get(), group, option, value);
}

template <typename TRegister>
void ConfigManager::setReadWriteRegister(TRegister &&reg) {
  std::lock_guard lock(mtx_);
  rw_reg_ = std::forward<TRegister>(reg);
}

template <typename T>
std::optional<T>
ConfigManager::tryRead(std::string_view group, std::string_view option,
                       ConfigReadRegister::ReadReturnCode *rc) {
  if (!isOpen())
    return WGFORGE_CONFIG_CLOSED;

  ConfigReadRegister::ReadReturnCode lrc = 0;
  std::lock_guard lock(mtx_);
  auto result = rw_reg_.tryRead<T>(cfg_.get(), group, option, &lrc);

  if (rc)
    *rc = lrc;
  return result;
}

template <typename T>
T ConfigManager::read(std::string_view group, std::string_view option) {
  if (!isOpen())
    return WGFORGE_CONFIG_CLOSED;

  return rw_reg_.read<T>(cfg_.get(), group, option);
}

} // namespace WgForge

#endif // __WGFORGE_CONFIG_MANAGER_H__
