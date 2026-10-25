/*
 * boost_beast_http_sender.cpp
 * wgforge-cli — CLI frontend for wgforge-server (C)
 * copyright (c) 2026  ledovskiy maksim aka fluffymax2005
 * <santech_montage@mail.ru>
 *
 * this file is part of wgforge.
 *
 * wgforge-server is free software: you can redistribute it and/or
 * modify it under the terms of the gnu lesser general public
 * license as published by the free software foundation,
 * either version 3 of the license, or (at your option) any later version.
 *
 * wgforge-server is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose. see the gnu
 * lesser general public license for more details.
 *
 * you should have received a copy of the gnu lesser general public
 * license along with wgforge-server. if not, see <https://www.gnu.org/licenses/>.
 */

#include "boost_beast_http_sender.h"

#include <algorithm>
#include <boost/url.hpp>
#include <format>
#include <iterator>

namespace WgForge {

BoostBeastHttpSender::BoostBeastHttpSender() : resolver_(ioc_), stream_(ioc_) {
  stream_.expires_after(std::chrono::seconds(15));
}

HttpResponse BoostBeastHttpSender::send(const HttpRequest& req) {
  namespace urls = boost::urls;
  urls::url u(req.target);

  const std::string port = u.has_port() ? std::string(u.port()) : "80";
  const std::string host = std::string(u.host());

  boost::system::error_code ec{};
  const auto results = resolver_.resolve(host, port, ec);
  if (ec) {
	throw std::runtime_error(
	    std::format("Failed to resolve remote host: {}:{}. Boost error ({}): {}", host, port,
		            ec.value(), ec.message()));
  }

  stream_.connect(results, ec);
  if (ec) {
	throw std::runtime_error(
	    std::format("Failed to connect to remote host: {}:{}. Boost error ({}): {}", host, port,
		            ec.value(), ec.message()));
  }

  http::request<http::string_body> http_req{};
  http_req.method(http::string_to_verb(req.method));
  http_req.target(u.encoded_target());
  http_req.version(11);
  http_req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  http_req.set(http::field::host, host + ":" + port);
  http_req.body() = req.body;
  for (const auto& [header, value] : req.headers)
	http_req.set(header, value);

  http_req.prepare_payload();

  http::write(stream_, http_req, ec);
  if (ec) {
	throw std::runtime_error(
	    std::format("Failed to send request to remote host: {}:{}. Boost error ({}): {}", host,
		            port, ec.value(), ec.message()));
  }

  beast::flat_buffer buffer;
  http::response<http::dynamic_body> http_res;
  http::read(stream_, buffer, http_res, ec);
  if (ec) {
	throw std::runtime_error(
	    std::format("Failed to read response from remote host: {}:{}. Boost error ({}): {}", host,
		            port, ec.value(), ec.message()));
  }

  HttpResponse res{};
  res.status = http_res.result_int();
  res.body = beast::buffers_to_string(http_res.body().data());

  res.headers.reserve(std::distance(http_res.base().begin(), http_res.base().end()));
  for (auto& field : http_res.base())
	res.headers.emplace_back(std::string(field.name_string()), std::string(field.value()));

  stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
  if (ec && ec != beast::errc::not_connected)
	throw std::runtime_error(
	    std::format("Failed to close connection to remote host: {}:{}. Boost error ({}): {}", host,
		            port, ec.value(), ec.message()));

  return res;
}

}  // namespace WgForge
