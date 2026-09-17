#ifndef __WGFORGE_LIBECONF_READERS_H__
#define __WGFORGE_LIBECONF_READERS_H__

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

} // namespace LibeconfReaders
} // namespace WgForge

#endif // __WGFORGE_LIBECONF_READERS_H__
