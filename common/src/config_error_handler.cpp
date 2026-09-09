#include <libeconf.h>

#include "config_error_handler.h"

namespace WgForge {

std::string ConfigErrorHandler::toString(ConfigManager::ConfigReturnCode rc,
                                         bool *ok) {
  if (ok)
    *ok = rc == ECONF_SUCCESS;

  if (rc > 0)
    return econf_errString(static_cast<econf_err>(rc));

  switch (rc) {
  case ECONF_SUCCESS:
    return "";
  case WGFORGE_CONFIG_READONLY:
    return "Config is opened in read-only mode";
  case WGFORGE_CONFIG_CLOSED:
    return "Cannot access to config. Config is closed";
  case WGFORGE_CONFIG_STRATEGY_NO_TARGET:
    return "Creating config failed. Functor with no target provided";
  case WGFORGE_CONFIG_WRITER_NOT_FOUND:
    return "Writing into config failed. Writer not found for specified type";
  case WGFORGE_CONFIG_READER_NOT_FOUND:
    return "Reading from config failed. Reader not found for specified type";
  case WGFORGE_CONFIG_INVALID_TYPE:
    return "Reading from config failed. Inapproriate reader provided for "
           "specified type";
  }
}

void ConfigErrorHandler::throwIfError(ConfigManager::ConfigReturnCode rc) {
  if (rc != ECONF_SUCCESS)
    throw std::runtime_error(toString(rc));
}

} // namespace WgForge
