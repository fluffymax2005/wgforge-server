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

} // namespace LibeconfWriters
} // namespace WgForge

#endif // __WGFORGE_LIBECONF_WRITERS_H__
