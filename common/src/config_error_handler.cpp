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

  default:
    return "Unknown libeconf error code: " + std::to_string(rc);
  }
}

std::string ConfigErrorHandler::readToString(ConfigManager::ConfigReturnCode rc,
                                             std::string_view group,
                                             std::string_view option,
                                             bool *ok) {
  std::string err_msg;
  switch (rc) {
  case ECONF_SUCCESS:
    return "";
  case ECONF_NOKEY:
    err_msg = std::format(
        "Error config parsing, no \"{}\" option in [{}] found", option, group);
    break;
  case ECONF_NOGROUP:
    err_msg = std::format("Error config parsing, no [{}] found", group);
    break;
  case ECONF_MISSING_BRACKET:
    err_msg = std::format("Error config parsing, missing closing section "
                          "bracket for \"{}\" group",
                          group);
    break;
  case ECONF_MISSING_DELIMITER:
    err_msg = std::format(
        "Error config parsing, missing delimiter for \"{}\" in [{}]", option,
        group);
    break;
  case ECONF_EMPTY_SECTION_NAME:
    err_msg = "Error config parsing, empty section name found";
    break;
  case ECONF_TEXT_AFTER_SECTION:
    err_msg = "Error config parsing, text after section found";
    break;
  case ECONF_OPTION_NOT_FOUND:
    err_msg =
        std::format("Error config parsing, option \"{}\" not found", option);
    break;
  default:
    err_msg = std::format(
        "Error occures while reading file for option \"{}\" in [{}]", option,
        group);
  }

  if (ok)
    *ok = rc == ECONF_SUCCESS;

  return std::format("{}: {}. Error code (libeconf): {}", err_msg,
                     econf_errString(static_cast<econf_err>(rc)), rc);
}

std::string
ConfigErrorHandler::writeToString(ConfigManager::ConfigReturnCode rc,
                                  std::string_view msg, bool *ok) {
  if (ok)
    *ok = rc == ECONF_SUCCESS;
  return std::format("%s: %s. Error code (libeconf): %i", msg,
                     econf_errString(static_cast<econf_err>(rc)), rc);
}

void ConfigErrorHandler::throwIfError(ConfigManager::ConfigReturnCode rc) {
  if (rc != ECONF_SUCCESS) {
    throw std::runtime_error(toString(rc));
  }
}

} // namespace WgForge
