#ifndef __WGFORGE_HANDLERS_H__
#define __WGFORGE_HANDLERS_H__

#include "defines.h"
#include <iostream>

namespace WgForge {

// GET
void interface__get(Request req, Response res, EndpointArgs &args);
void peers(Request req, Response res, EndpointArgs &args);
void peer__get(Request req, Response res, EndpointArgs &args);
void peer_config(Request req, Response res, EndpointArgs &args);

// POST
void peer__post(Request req, Response res, EndpointArgs &args);

// PATCH
void interface__patch(Request req, Response res, EndpointArgs &args);
void peer__patch(Request req, Response res, EndpointArgs &args);

// DELETE
void peer__delete(Request req, Response res, EndpointArgs &args);

// Utils
template <typename... Args> void log_info_to(std::ostream &os, Args &&...args) {
  os << BLUE_COLOR << "[INFO]" << RESET_COLOR << " WgForge: ";
  (os << ... << args);
  os << std::endl;
}

template <typename... Args> void log_info(Args &&...args) {
  log_info_to(std::clog, args...);
}

template <typename... Args>
void log_error_to(std::ostream &os, Args &&...args) {
  os << RED_COLOR << "[ERROR]" << RESET_COLOR << " WgForge: ";
  (os << ... << args);
  os << std::endl;
}

template <typename... Args> void log_error(Args &&...args) {
  log_error_to(std::cerr, args...);
}

} // namespace WgForge

#endif // __WGFORGE_HANDLERS_H__
