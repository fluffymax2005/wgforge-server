/*
 * boost_beast_http_sender.h
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-cli is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-cli is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WGFORGE_BOOST_BEAST_HTTP_SENDER_H
#define WGFORGE_BOOST_BEAST_HTTP_SENDER_H

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast.hpp>

#include "ihttpsender.h"

namespace WgForge {

namespace beast = boost::beast;
namespace net = beast::net;
namespace http = beast::http;
using tcp = net::ip::tcp;

class BoostBeastHttpSender : public IHttpSender {
public:
  BoostBeastHttpSender();

  virtual HttpResponse send(const HttpRequest& req) override;

private:
  net::io_context ioc_;
  tcp::resolver resolver_;
  beast::tcp_stream stream_;
};

}  // namespace WgForge

#endif  // WGFORGE_BOOST_BEAST_HTTP_SENDER_H
