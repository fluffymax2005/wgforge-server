#include "endpoint_tools.h"

#include "endpoint_invalid_argument.h"
#include "endpoint_runtime_error.h"
#include "handlers.h"

namespace WgForge {

void handleEndpointException(std::exception_ptr ep, Response res) {
  try {
	if (ep)
	  std::rethrow_exception(ep);
  } catch (const EndpointInvalidArgument& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const EndpointRuntimeError& e) {
	log_error(e.getUri(), " [", e.getMethod(), "]: ", e.what());
	res.result(http::status::internal_server_error);
  } catch (const boost::system::system_error& e) {
	json::object j;
	j["error"] = e.what();
	res.result(http::status::bad_request);
	res.set(http::field::content_type, "application/json");
	res.body() = json::serialize(j);
  } catch (const std::exception& e) {
	log_error("Unexpected error: ", e.what());
	res.result(http::status::internal_server_error);
	res.body() = "Internal server error";
  }
}

}  // namespace WgForge
