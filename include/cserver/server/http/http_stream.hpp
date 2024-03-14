#pragma once
#include <cserver/engine/coroutine.hpp>
#include <cserver/utils/boost_error_wrapper.hpp>
#include <fmt/format.h>
#include <boost/asio.hpp>

namespace cserver::server::http {

struct HTTPStream {
  boost::asio::ip::tcp::socket socket;
  std::stringstream stream = {};
  inline auto SetMethod(std::string method) -> Task<void> {
    method += " ";
    auto [ec, n] = co_await boost::asio::async_write(this->socket, boost::asio::buffer(method.data(), method.size()), boost::asio::transfer_all(), boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
  };
   inline auto SetStatus(std::string status) -> Task<void> {
    status = fmt::format("HTTP/1.1 {}\r\n", std::move(status));
    auto [ec, n] = co_await boost::asio::async_write(this->socket, boost::asio::buffer(status.data(), status.size()), boost::asio::transfer_all(), boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
  };
 
  inline auto SetHeader(std::string first, std::string second) -> Task<void> {
    this->stream << fmt::format("{}: {}\r\n", std::move(first), std::move(second));
    co_return;
  };
  inline auto SetEndOfHeaders() -> Task<void> {
    this->stream << "\r\n";
    auto str = this->stream.str();
    auto [ec, n] = co_await boost::asio::async_write(this->socket, boost::asio::buffer(str.data(), str.size()), boost::asio::transfer_all(), boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
  };
  inline auto PushBodyChunk(std::string_view chunk) -> Task<void> {
    auto [ec, n] = co_await boost::asio::async_write(this->socket, boost::asio::buffer(chunk.data(), chunk.size()), boost::asio::transfer_all(), boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
  };
  inline auto Close() -> Task<void> {
    this->socket.close();
    co_return;
  };
};

} // namespace cserver::server::http