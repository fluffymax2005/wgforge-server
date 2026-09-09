#include "config_manager.h"
#include "config_read_functions.h"
#include "config_write_functions.h"

namespace WgForge {

ConfigManager::ConfigManager() {
  registerDefaultReadersImpl();
  registerDefaultWritersImpl();
}

void ConfigManager::registerDefaultReadersImpl() {
  rw_reg_.registerReader<bool>(LibeconfReaders::readBool);
  rw_reg_.registerReader<int>(LibeconfReaders::readInt);
  rw_reg_.registerReader<int64_t>(LibeconfReaders::readInt64);
  rw_reg_.registerReader<uint32_t>(LibeconfReaders::readUInt);
  rw_reg_.registerReader<uint64_t>(LibeconfReaders::readUInt64);
  rw_reg_.registerReader<std::string>(LibeconfReaders::readString);
  rw_reg_.registerReader<float>(LibeconfReaders::readFloat);
  rw_reg_.registerReader<double>(LibeconfReaders::readDouble);
}

void ConfigManager::registerDefaultWritersImpl() {
  rw_reg_.registerWriter<bool>(LibeconfWriters::writeBool);
  rw_reg_.registerWriter<int>(LibeconfWriters::writeInt);
  rw_reg_.registerWriter<int64_t>(LibeconfWriters::writeInt64);
  rw_reg_.registerWriter<uint32_t>(LibeconfWriters::writeUInt);
  rw_reg_.registerWriter<std::string>(LibeconfWriters::writeString);
  rw_reg_.registerWriter<float>(LibeconfWriters::writeFloat);
  rw_reg_.registerWriter<double>(LibeconfWriters::writeDouble);
}

ConfigManager::ConfigManager(std::string_view path, bool readonly) noexcept {
  open(path, readonly);
}

ConfigManager::ConfigManager(ConfigManager &&other) noexcept {
  if (this != &other) {
    std::lock_guard lock(other.mtx_);

    readonly_ = other.readonly_;
    rw_reg_ = std::move(rw_reg_);
    cfg_ = std::move(other.cfg_);
  }
}

ConfigManager &ConfigManager::operator=(ConfigManager &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock(mtx_, other.mtx_);

    readonly_ = other.readonly_;
    rw_reg_ = std::move(rw_reg_);
    cfg_ = std::move(other.cfg_);
  }

  return *this;
}

inline bool ConfigManager::isOpen() const noexcept {
  std::lock_guard lock(mtx_);
  return cfg_.get();
}

ConfigManager::operator bool() const noexcept { return isOpen(); }

ConfigManager::ConfigReturnCode ConfigManager::open(std::string_view path,
                                                    bool readonly) {
  std::lock_guard lock(mtx_);
  econf_file *config = cfg_.get();
  econf_err ec = econf_readFile(&config, path.data(), "=", "#");
  if (ec == ECONF_SUCCESS) {
    readonly_ = readonly;
    cfg_.reset(config);
  }
  return ec;
}

inline void ConfigManager::close() noexcept {
  std::lock_guard lock(mtx_);
  cfg_.reset();
}

ConfigManager::ConfigReturnCode
ConfigManager::create(std::string_view path,
                      CreateConfigStrategy strategy) noexcept {
  if (!strategy)
    return WGFORGE_CONFIG_STRATEGY_NO_TARGET;

  ConfigReturnCode rc;
  econf_file *config = strategy(path, rc);
  if (rc != ECONF_SUCCESS || config == nullptr)
    return rc;

  std::lock_guard lock(mtx_);
  cfg_.reset(config);
  return rc;
}

void ConfigManager::ConfigDeleter::operator()(econf_file *p) noexcept {
  if (p)
    econf_freeFile(p);
}

} // namespace WgForge
