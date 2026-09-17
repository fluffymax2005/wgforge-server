#ifndef __WGFORGE_CONFIG_WRITE_REGISTER_H__
#define __WGFORGE_CONFIG_WRITE_REGISTER_H__

#include <libeconf.h>

#include <any>
#include <functional>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include "wgforge.h"

namespace WgForge {

class ConfigWriteRegister {
  public:
  using WriteReturnCode = int;

  using UniversalWriteFunc = std::function<WriteReturnCode(
      econf_file* config, std::string_view group, std::string_view option, const std::any& value)>;

  template<typename T>
  using WriteFunc = std::function<WriteReturnCode(econf_file* config, std::string_view group,
                                                  std::string_view option, const T& value)>;

  ConfigWriteRegister() = default;

  ConfigWriteRegister(const ConfigWriteRegister&) = default;
  ConfigWriteRegister& operator=(const ConfigWriteRegister&) = default;

  ConfigWriteRegister(ConfigWriteRegister&&) = default;
  ConfigWriteRegister& operator=(ConfigWriteRegister&&) = default;

  template<typename T>
  void registerWriter(WriteFunc<T> func);
  template<typename T>
  void removeWriter() noexcept;

  template<typename T>
  WriteReturnCode write(econf_file* config, std::string_view group, std::string_view option,
                        const T& value);

  protected:
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

} // namespace WgForge

#endif // __WGFORGE_CONFIG_WRITE_REGISTER_H__
