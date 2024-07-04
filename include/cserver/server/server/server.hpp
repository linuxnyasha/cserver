#pragma once
#include <boost/asio.hpp>
#include <cserver/components/work_guard.hpp>
#include <cserver/engine/components.hpp>
#include <cserver/engine/coroutine.hpp>
#include <cserver/server/http/http_request_parser.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/server/http/http_stream.hpp>

namespace cserver::server::server {

template <utempl::ConstexprString TPName = "basicTaskProcessor", typename TaskProcessor = int, typename... Ts>
struct Server : StopBlocker {
  TaskProcessor& taskProcessor;
  utempl::Tuple<impl::GetTypeFromComponentConfig<Ts>&...> handlers;
  static constexpr utempl::ConstexprString kName = "server";
  std::uint16_t port;
  static constexpr utempl::Tuple kNames = {impl::kNameFromComponentConfig<Ts>...};
  static constexpr utempl::Tuple kPaths = {impl::GetTypeFromComponentConfig<Ts>::kPath...};
  template <utempl::ConstexprString name, Options Options, typename T>
  static consteval auto Adder(const T& context) {
    constexpr utempl::ConstexprString tpName = [&] {
      if constexpr(requires { T::kConfig.template Get<name>().template Get<"taskProcessor">(); }) {
        return T::kConfig.template Get<name>().template Get<"taskProcessor">();
      } else {
        return TPName;
      };
    }();
    using TP = decltype(context.template FindComponent<tpName>());
    return context.TransformComponents([&](const ComponentConfig<name, Server<TPName, int, Ts...>, Options>&)
                                           -> ComponentConfig<name, Server<tpName, TP, Ts...>, Options> {
      return {};
    });
  };
  template <typename T, std::size_t... Is>
  constexpr Server(std::index_sequence<Is...>, T& context) :
      StopBlocker(context),
      taskProcessor(context.template FindComponent<TPName>()),
      handlers{context.template FindComponent<Get<Is>(kNames)>()...},
      port(T::kConfig.template Get<T::kName>().template Get<"port">()){

      };
  explicit constexpr Server(auto& context) : Server(std::index_sequence_for<Ts...>{}, context) {};
  template <auto I, typename Socket>
  inline auto ProcessHandler(Socket&& socket, http::HttpRequest request) -> Task<void> {
    if constexpr(requires(http::HttpStream& stream) { Get<I>(this->handlers).HandleRequestStream(std::move(request), stream); }) {
      http::HttpStream stream{std::move(socket)};
      co_await Get<I>(this->handlers).HandleRequestStream(std::move(request), stream);
      co_return;
    } else {
      http::HttpResponse response = co_await Get<I>(this->handlers).HandleRequest(std::move(request));
      response.headers["Content-Length"] = std::to_string(response.body.size());
      response.headers["Server"] = "cserver/1";
      auto data = response.ToString();
      co_await boost::asio::async_write(socket, boost::asio::buffer(data.data(), data.size()), boost::asio::use_awaitable);
      socket.close();
    };
  };
  auto Reader(boost::asio::ip::tcp::socket socket) -> Task<void> {
    std::string buffer;
    buffer.reserve(socket.available());
    co_await boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(buffer), "\r\n\r\n", boost::asio::use_awaitable);
    http::HttpRequest request = http::HttpRequestParser{buffer};  // NOLINT
    bool flag = false;
    co_await [&]<auto... Is>(std::index_sequence<Is...>) -> cserver::Task<void> {
      (
          co_await [&] -> cserver::Task<void> {
            if(request.url.path().substr(0, Get<Is>(kPaths).size()) == Get<Is>(kPaths)) {
              co_await this->ProcessHandler<Is>(std::move(socket), std::move(request));
            };
          }(),
          ...);
    }(std::index_sequence_for<Ts...>());
    constexpr std::string_view error404 =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    if(!flag) {
      co_await boost::asio::async_write(socket, boost::asio::buffer(error404.data(), error404.size()), boost::asio::use_awaitable);
    };
  };
  inline auto Task() -> Task<void> {
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
  inline ~Server() = default;
  template <typename Handler>
  using AddHandler = Server<TPName, TaskProcessor, Ts..., Handler>;
};
}  // namespace cserver::server::server
