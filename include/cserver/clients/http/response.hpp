#pragma once
#include <cserver/server/http/http_response.hpp>
#include <cserver/engine/coroutine.hpp>
#include <cserver/utils/boost_error_wrapper.hpp>
#include <boost/asio/ssl.hpp>

namespace cserver::clients::http {

template <typename HttpClient, typename Socket>
class Response : public server::http::HTTPResponse {
  HttpClient& client;
  Socket socket;

public:
  inline auto ReadChunk() -> cserver::Task<bool> {
    this->body.resize(4479);
    auto [ec, n] = co_await this->socket.async_read_some(boost::asio::buffer(this->body.data(), 4479), boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec == boost::asio::error::eof || ec == boost::asio::error::operation_aborted || ec == boost::asio::ssl::error::stream_truncated) {
      co_return false;
    };
    if(ec) {
      throw BoostErrorWrapper{ec};
    };

    this->body.resize(n);
    co_return true;
  };
  inline constexpr Response(Response&&) = default;
  inline constexpr Response(const Response&) = default;
  inline constexpr Response(HttpClient& client, Socket socket, server::http::HTTPResponse response) : 
      client(client),
      socket(std::move(socket)),
      HTTPResponse(std::move(response)) {};
};

} // namespace cserver::clients::http
