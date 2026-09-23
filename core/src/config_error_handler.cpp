/*
 * config_error_handler.cpp
 * wgforge-core — Core library for wgforge-server (C)
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

#include "config_error_handler.h"

#include <libeconf.h>

namespace WgForge {

std::string ConfigErrorHandler::toString(ConfigManager::ConfigReturnCode rc, bool* ok) {
  if (ok)
	*ok = rc == ECONF_SUCCESS;

  if (rc > 0)
	return econf_errString(static_cast<econf_err>(rc));

  switch (rc) {
  case WGFORGE_CONFIG_OK      : return "";
  case WGFORGE_CONFIG_READONLY: return "Config is opened in read-only mode";
  case WGFORGE_CONFIG_CLOSED  : return "Cannot access to config. Config is closed";
  case WGFORGE_CONFIG_STRATEGY_NO_TARGET:
	return "Creating config failed. Functor with no target provided";
  case WGFORGE_CONFIG_WRITER_NOT_FOUND:
	return "Writing into config failed. Writer not found for specified type";
  case WGFORGE_CONFIG_READER_NOT_FOUND:
	return "Reading from config failed. Reader not found for specified type";
  case WGFORGE_CONFIG_INVALID_TYPE:
	return "Reading from config failed. Inapproriate reader provided for "
	       "specified type";
  case WGFORGE_CONFIG_NOMEM               : return "Creating config failed. Not enough memory";
  case WGFORGE_CONFIG_TRANSACTION_ACTIVE  : return "Config transaction is active";
  case WGFORGE_CONFIG_TRANSACTION_INACTIVE: return "Config transaction is inactive";
  case WGFORGE_CONFIG_RUNTIME_ERROR       : return "Config runtime operation error";
  default: return "Unknown libeconf error code: " + std::to_string(rc);
  }
}

std::string ConfigErrorHandler::readToString(ConfigManager::ConfigReturnCode rc,
                                             std::string_view group, std::string_view option,
                                             bool* ok) {
  std::string err_msg;
  switch (rc) {
  case ECONF_SUCCESS: return "";
  case ECONF_NOKEY:
	err_msg = std::format("Error config parsing, no \"{}\" option in [{}] found", option, group);
	break;
  case ECONF_NOGROUP: err_msg = std::format("Error config parsing, no [{}] found", group); break;
  case ECONF_MISSING_BRACKET:
	err_msg = std::format(
	    "Error config parsing, missing closing section "
	    "bracket for \"{}\" group",
	    group);
	break;
  case ECONF_MISSING_DELIMITER:
	err_msg =
	    std::format("Error config parsing, missing delimiter for \"{}\" in [{}]", option, group);
	break;
  case ECONF_EMPTY_SECTION_NAME: err_msg = "Error config parsing, empty section name found"; break;
  case ECONF_TEXT_AFTER_SECTION: err_msg = "Error config parsing, text after section found"; break;
  case ECONF_OPTION_NOT_FOUND:
	err_msg = std::format("Error config parsing, option \"{}\" not found", option);
	break;
  default:
	err_msg =
	    std::format("Error occures while reading file for option \"{}\" in [{}]", option, group);
  }

  if (ok)
	*ok = rc == ECONF_SUCCESS;

  return std::format("{}: {}. Error code (libeconf): {}", err_msg,
                     econf_errString(static_cast<econf_err>(rc)), rc);
}

std::string ConfigErrorHandler::writeToString(ConfigManager::ConfigReturnCode rc,
                                              std::string_view msg, bool* ok) {
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
