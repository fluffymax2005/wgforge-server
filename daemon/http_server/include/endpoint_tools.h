#ifndef __WGFORGE_ENDPOINT_TOOLS_H__
#define __WGFORGE_ENDPOINT_TOOLS_H__

#include <assert.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string_view>

#include "defines.h"
#include "endpoint_invalid_argument.h"

namespace WgForge {

template<typename T>
std::optional<T> get_optional_field(const json::object& j, std::string_view key,
                                    bool mandatory = true) {
  const auto* val = j.if_contains(key);
  if (val == nullptr) {
	if (mandatory)
	  throw EndpointInvalidArgument(std::format("\"{}\" missing", key));
	return std::nullopt;
  }

  if constexpr (std::is_same_v<T, std::string_view>) {
	if (mandatory) {
	  if (val->is_string())
		return val->as_string();
	  else
		throw EndpointInvalidArgument(std::format("\"{}\" must be a string", key));
	}
	if (val->is_string())
	  return val->as_string();
	return std::nullopt;
  } else if constexpr (std::is_same_v<T, std::uint64_t>) {
	if (mandatory) {
	  if (val->as_uint64())
		return val->as_uint64();
	  else
		throw EndpointInvalidArgument(std::format("\"{}\" must be a unsigned integer", key));
	}
	if (val->is_uint64())
	  return val->as_uint64();
	return std::nullopt;
  } else if constexpr (std::is_same_v<T, std::int64_t>) {
	if (mandatory) {
	  if (val->is_int64())
		return val->as_int64();
	  else
		throw EndpointInvalidArgument(std::format("\"{}\" must be a integer", key));
	}
	if (val->is_int64())
	  return val->as_int64();
	return std::nullopt;
  } else if constexpr (std::is_same_v<T, bool>) {
	if (mandatory) {
	  if (val->is_bool())
		return val->as_bool();
	  else
		throw EndpointInvalidArgument(std::format("\"{}\" must be a bool", key));
	}
	if (val->is_bool())
	  return val->as_bool();
	return std::nullopt;
  } else if constexpr (std::is_same_v<T, double>) {
	if (mandatory) {
	  if (val->is_double())
		return val->as_double();
	  else
		throw EndpointInvalidArgument(std::format("\"{}\" must be a bool", key));
	}
	if (val->is_double())
	  return val->as_double();
	return std::nullopt;
  } else if constexpr (std::is_same_v<T, json::array>) {
	if (mandatory) {
	  if (val->is_array())
		return val->as_array();
	  else
		throw EndpointInvalidArgument(std::format("\"{}\" must be a array", key));
	}
	if (val->is_array())
	  return val->as_array();
	return std::nullopt;
  } else {
	static_assert(false, "Unsupported type");
  }
}

} // namespace WgForge

#endif // __WGFORGE_ENDPOINT_TOOLS_H__
