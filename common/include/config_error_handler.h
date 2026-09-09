#ifndef __WGFORGE_CONFIG_ERROR_HANDLER_H__
#define __WGFORGE_CONFIG_ERROR_HANDLER_H__

#include <string>

#include "config_manager.h"

namespace WgForge {

class ConfigErrorHandler {
public:
  static std::string toString(ConfigManager::ConfigReturnCode rc,
                              bool *ok = nullptr);

  static void throwIfError(ConfigManager::ConfigReturnCode rc);
};

} // namespace WgForge

#endif // __WGFORGE_CONFIG_ERROR_HANDLER_H__
