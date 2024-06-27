#pragma once
#include <cserver/engine/coroutine.hpp>
#include <fmt/format.h>
#include <boost/asio.hpp>

namespace cserver::server::http {

struct HttpStream {
  boost::asio::ip::tcp::socket socket;
  std::stringstream stream = {};
  inline auto SetMethod(std::string method) -> Task<void> {
    method += " ";
    co_await boost::asio::async_write(this->socket, boost::asio::buffer(method.data(), method.size()), boost::asio::transfer_all(), boost::asio::use_awaitable);
  };
   inline auto SetStatus(std::string status) -> Task<void> {
    status = fmt::format("Http/1.1 {}\r\n", std::move(status));
    co_await boost::asio::async_write(this->socket, boost::asio::buffer(status.data(), status.size()), boost::asio::transfer_all(), boost::asio::use_awaitable);
  };
 
  inline auto SetHeader(std::string first, std::string second) -> Task<void> {
    this->stream << fmt::format("{}: {}\r\n", std::move(first), std::move(second));
    co_return;
  };
  inline auto SetEndOfHeaders() -> Task<void> {
    this->stream << "\r\n";
    auto str = this->stream.str();
    co_await boost::asio::async_write(this->socket, boost::asio::buffer(str.data(), str.size()), boost::asio::transfer_all(), boost::asio::use_awaitable);
  };
  inline auto PushBodyChunk(std::string_view chunk) -> Task<void> {
    co_await boost::asio::async_write(this->socket, boost::asio::buffer(chunk.data(), chunk.size()), boost::asio::transfer_all(), boost::asio::use_awaitable);
  };
  inline auto Close() -> Task<void> {
    this->socket.close();
    co_return;
  };
};

} // namespace cserver::server::http
