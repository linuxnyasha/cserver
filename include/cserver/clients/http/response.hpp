#pragma once
#include <boost/asio/ssl.hpp>
#include <cserver/engine/coroutine.hpp>
#include <cserver/server/http/http_response.hpp>

namespace cserver::clients::http {

template <typename HttpClient, typename Socket>
class Response : public server::http::HttpResponse {
  HttpClient& client;
  Socket socket;

 public:
  inline auto ReadChunk() -> cserver::Task<bool> {
    this->body.resize(4479);  // NOLINT

    auto [ec, n] = co_await this->socket.async_read_some(boost::asio::buffer(this->body.data(), 4479),  // NOLINT
                                                         boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec == boost::asio::error::eof || ec == boost::asio::error::operation_aborted || ec == boost::asio::ssl::error::stream_truncated) {
      co_return false;
    };
    if(ec) {
      throw boost::system::error_code(ec);
    };
    this->body.resize(n);
    co_return true;
  };
  constexpr Response(Response&&) = default;
  constexpr Response(const Response&) = default;
  inline Response(HttpClient& client, Socket socket, server::http::HttpResponse response) :
      client(client), socket(std::move(socket)), HttpResponse(std::move(response)) {};
};

}  // namespace cserver::clients::http
