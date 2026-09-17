#ifndef __WGFORGE_WGFORGESERVER_H__
#define __WGFORGE_WGFORGESERVER_H__

#include <boost/asio.hpp>
#include <string>

#include "connection.h"
#include "connection_manager.h"
#include "request_handler.h"

namespace WgForge {

class WgForgeServer {
public:
  WgForgeServer(const WgForgeServer&) = delete;
  WgForgeServer& operator=(const WgForgeServer&) = delete;

  explicit WgForgeServer(const std::string& address, const std::string& port);

  void setHandler(RequestHandler&& handler) noexcept;

  void run();

private:
  void do_accept();
  void do_await_stop();

  boost::asio::io_context io_context_;
  boost::asio::signal_set signals_;
  boost::asio::ip::tcp::acceptor acceptor_;
  ConnectionManager con_manager_;
  RequestHandler req_handler_;
};

} // namespace WgForge

#endif // __WGFORGE_WGFORGESERVER_H__
