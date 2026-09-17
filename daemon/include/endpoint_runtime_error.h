#ifndef __WGFORGE_ENDPOINT_RUNTIME_ERROR_H__
#define __WGFORGE_ENDPOINT_RUNTIME_ERROR_H__

#include <stdexcept>

namespace WgForge {

class EndpointRuntimeError : public std::runtime_error {
  public:
  EndpointRuntimeError(const std::string& msg) : std::runtime_error(msg) {}

  EndpointRuntimeError(const char* msg) : std::runtime_error(msg) {}
};

} // namespace WgForge

#endif // __WGFORGE_ENDPOINT_RUNTIME_ERROR_H__
