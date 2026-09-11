#ifndef __WGFORGE_CONFIG_ERROR_HANDLER_H__
#define __WGFORGE_CONFIG_ERROR_HANDLER_H__

#include <string>

#include "config_manager.h"

namespace WgForge {

class ConfigErrorHandler {
public:
  static std::string toString(ConfigManager::ConfigReturnCode rc,
                              bool *ok = nullptr);

  static std::string readToString(ConfigManager::ConfigReturnCode rc,
                                  std::string_view group,
                                  std::string_view option, bool *ok = nullptr);
  static std::string writeToString(ConfigManager::ConfigReturnCode rc,
                                   std::string_view msg, bool *ok = nullptr);

  static void throwIfError(ConfigManager::ConfigReturnCode rc);
};

} // namespace WgForge

#endif // __WGFORGE_CONFIG_ERROR_HANDLER_H__
