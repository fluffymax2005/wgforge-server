#ifndef __WGFORGE_ENDPOINT_INVALID_ARGUMENT_H__
#define __WGFORGE_ENDPOINT_INVALID_ARGUMENT_H__

#include <stdexcept>

namespace WgForge {

class EndpointInvalidArgument : public std::invalid_argument {
public:
  EndpointInvalidArgument(const std::string &msg)
      : std::invalid_argument(msg) {}
  EndpointInvalidArgument(const char *msg) : std::invalid_argument(msg) {}
};

} // namespace WgForge

#endif // __WGFORGE_ENDPOINT_INVALID_ARGUMENT_H__
