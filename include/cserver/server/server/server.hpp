#pragma once
#include <cserver/engine/components.hpp>
#include <cserver/server/http/http_request_parser.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/engine/coroutine.hpp>
#include <boost/asio.hpp>


namespace cserver::server::server {
namespace impl {
template <typename T>
using GetTypeFromComponentConfig = decltype([]<utempl::ConstexprString name, typename TT>(const ComponentConfig<name, TT>&) -> TT {}(std::declval<T>()));

template <typename T>
inline constexpr utempl::ConstexprString kNameFromComponentConfig = 
  decltype([]<utempl::ConstexprString name, typename TT>(const ComponentConfig<name, TT>&) {
    return utempl::Wrapper<name>{};
  } (std::declval<T>()))::kValue;
} // namespace impl

template <utempl::ConstexprString TPName = "basicTaskProcessor", typename TaskProcessor = int, typename... Ts>
struct Server {
  TaskProcessor& taskProcessor;
  utempl::Tuple<impl::GetTypeFromComponentConfig<Ts>&...> handlers;
  static constexpr utempl::ConstexprString kName = "server";
  unsigned short port;
  static constexpr utempl::Tuple kNames = {impl::kNameFromComponentConfig<Ts>...};
  static constexpr utempl::Tuple kPaths = {impl::GetTypeFromComponentConfig<Ts>::kPath...};
  template <utempl::ConstexprString name, typename T>
  static consteval auto Adder(const T& context) {
    constexpr utempl::ConstexprString tpName = [&]{
      if constexpr(requires{T::kConfig.template Get<name>().template Get<"taskProcessor">();}) {
        return T::kConfig.template Get<name>().template Get<"taskProcessor">();
      } else {
        return TPName;
      };
    }();
    using TP = decltype(context.template FindComponent<tpName>());
    return context.TransformComponents(
      [&](const ComponentConfig<name, Server<TPName, int, Ts...>>&) -> ComponentConfig<name, Server<tpName, TP, Ts...>> {
        return {};
      });
  };
  template <utempl::ConstexprString name, typename T>
  inline constexpr Server(utempl::Wrapper<name>, T& context) :
      taskProcessor(context.template FindComponent<TPName>()),
      handlers{
        [&]<auto... Is>(std::index_sequence<Is...>) -> utempl::Tuple<impl::GetTypeFromComponentConfig<Ts>&...> {
          return {context.template FindComponent<Get<Is>(kNames)>()...};
        }(std::index_sequence_for<Ts...>())
      },
      port(T::kConfig.template Get<name>().template Get<"port">()) {
  };
  auto Reader(boost::asio::ip::tcp::socket socket) -> Task<void> {
    std::string buffer;
    buffer.reserve(socket.available());
    co_await boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(buffer), "\r\n\r\n", boost::asio::use_awaitable);
    http::HTTPRequest request = http::HTTPRequestParser{buffer};
    bool flag = false;
    co_await [&]<auto... Is>(std::index_sequence<Is...>) -> cserver::Task<void> {
      (co_await [&]<auto I>(utempl::Wrapper<I>) -> cserver::Task<void> {
        if(request.url.path().substr(0, Get<I>(kPaths).size()) == Get<I>(kPaths)) {
          flag = true;
          http::HTTPResponse response{};
          response.body = co_await Get<I>(this->handlers).HandleRequest(std::move(request), response);

          response.headers["Content-Length"] = std::to_string(response.body.size());
          response.headers["Server"] = "cserver/1";
          auto data = response.ToString();
          co_await boost::asio::async_write(socket, boost::asio::buffer(data.data(), data.size()), boost::asio::use_awaitable);
          socket.close();
        };
      }(utempl::Wrapper<Is>{}), ...);
    }(std::index_sequence_for<Ts...>());
    constexpr std::string_view error404 = "HTTP/1.1 404 Not Found\r\n"
                                          "Content-Length: 0\r\n"
                                          "\r\n";
    if(!flag) {
      co_await boost::asio::async_write(socket, boost::asio::buffer(error404.data(), error404.size()), boost::asio::use_awaitable);
    };
  };
  auto Task() -> cserver::Task<void> {
    auto executor = co_await boost::asio::this_coro::executor;
    boost::asio::ip::tcp::acceptor acceptor{executor, {boost::asio::ip::tcp::v6(), this->port}};
    for(;;) {
      auto socket = co_await acceptor.async_accept(boost::asio::use_awaitable);
      boost::asio::co_spawn(executor, this->Reader(std::move(socket)), boost::asio::detached);
    };
  };
  inline constexpr auto Run() -> void {
    boost::asio::co_spawn(this->taskProcessor.ioContext, this->Task(), boost::asio::detached);
  };
  inline ~Server() {
  };
  template <typename Handler>
  using AddHandler = Server<TPName, TaskProcessor, Ts..., Handler>;
};
} // namespace cserver::server::server