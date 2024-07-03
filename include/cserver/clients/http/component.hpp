#pragma once
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <cserver/clients/http/request.hpp>
#include <cserver/clients/http/response.hpp>
#include <cserver/engine/components.hpp>
#include <cserver/engine/coroutine.hpp>
#include <cserver/engine/use_streaming.hpp>
#include <utempl/constexpr_string.hpp>

namespace cserver::server::http {

inline constexpr auto ParseHttpHeader(std::string header) -> std::pair<std::string, std::string> {
  size_t pos = header.find(":");
  std::string key = header.substr(0, pos);
  std::string value = header.substr(pos + 2, header.size() - 1);
  return std::make_pair(key, value);
};
}  // namespace cserver::server::http
namespace cserver::components {
template <typename TaskProcessor = int>
struct HttpClient {
  TaskProcessor& taskProcessor;
  boost::asio::ssl::context ctx;
  boost::asio::ip::tcp::resolver resolver;
  static constexpr utempl::ConstexprString kName = "httpClient";
  inline constexpr auto CreateRequest() {
    return clients::http::Request{*this};
  };
  inline constexpr HttpClient(auto, auto& context) :
      taskProcessor(context.template FindComponent<"basicTaskProcessor">()),
      ctx(boost::asio::ssl::context::method::sslv23_client),
      resolver(this->taskProcessor.ioContext) {};

  template <utempl::ConstexprString name, Options Options, typename T>
  static consteval auto Adder(const T& context) {
    using Type = std::remove_cvref_t<decltype(context.template FindComponent<"basicTaskProcessor">())>;
    return context.TransformComponents(
        [&](const ComponentConfig<name, HttpClient<>, Options>&) -> ComponentConfig<name, HttpClient<Type>, Options> {
          return {};
        });
  };

 private:
  template <typename Socket, typename... Flags>
  static consteval auto GetPerformReturnType(Flags...) {
    constexpr auto kUseStreaming = utempl::Find<UseStreaming>(utempl::kTypeList<Flags...>) != sizeof...(Flags);
    if constexpr(kUseStreaming) {
      return [] -> clients::http::Response<HttpClient<TaskProcessor>, Socket> {
        std::unreachable();
      }();
    } else {
      return [] -> server::http::HttpResponse {
        std::unreachable();
      }();
    };
  };
  template <typename Socket>
  inline auto ReadHeaders(Socket&& socket, auto&&...) const -> cserver::Task<server::http::HttpResponse> {
    std::string serverResponse;
    co_await boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(serverResponse), "\r\n\r\n", boost::asio::use_awaitable);
    server::http::HttpResponse response;
    std::istringstream responseStream(std::move(serverResponse));
    std::string httpVersion;
    responseStream >> httpVersion >> response.statusCode;
    std::getline(responseStream, response.statusMessage);
    std::string header;
    while(std::getline(responseStream, header) && header.find(":") != std::string::npos) {
      response.headers.insert(server::http::ParseHttpHeader(std::move(header)));
    };
    co_return response;
  };
  template <typename Socket>
  inline auto ReadBody(Socket&& socket, std::size_t length, auto&&...) const -> cserver::Task<std::string> {
    std::string response;
    response.reserve(length);
    co_await boost::asio::async_read(
        socket, boost::asio::dynamic_buffer(response), boost::asio::transfer_at_least(length), boost::asio::use_awaitable);
    co_return response;
  };
  template <typename Socket>
  inline auto ReadBody(Socket&& socket, auto&&...) const -> cserver::Task<std::string> {
    std::string response;
    for(;;) {
      auto [ec, n] = co_await boost::asio::async_read(socket,
                                                      boost::asio::dynamic_buffer(response),
                                                      boost::asio::transfer_at_least(1),
                                                      boost::asio::as_tuple(boost::asio::use_awaitable));
      if(ec && ec == boost::asio::error::eof) {
        break;
      };
      if(ec) {
        throw boost::system::error_code{ec};
      };
    };
    co_return response;
  };

 public:
  template <typename... Flags, typename T>
  inline auto PerformRequest(T&& request, Flags... flags)
      -> cserver::Task<decltype(this->GetPerformReturnType<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(flags...))> {
    constexpr bool kUseStreaming = utempl::Find<UseStreaming>(utempl::kTypeList<Flags...>) != sizeof...(Flags);
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(this->taskProcessor.ioContext, this->ctx);
    boost::asio::ip::tcp::resolver::iterator endpoint = co_await this->resolver.async_resolve(
        {request.request.url.host(), request.request.url.has_port() ? request.request.url.port() : request.request.url.scheme()},
        boost::asio::use_awaitable);
    co_await boost::asio::async_connect(socket.lowest_layer(), endpoint, boost::asio::use_awaitable);

    co_await socket.async_handshake(boost::asio::ssl::stream_base::client, boost::asio::use_awaitable);

    std::string req(request.request.ToString());
    co_await boost::asio::async_write(
        socket, boost::asio::buffer(req.data(), req.size()), boost::asio::transfer_all(), boost::asio::use_awaitable);
    auto response = co_await this->ReadHeaders(socket, flags...);
    if constexpr(kUseStreaming) {
      co_return clients::http::Response<HttpClient<TaskProcessor>, decltype(socket)>{*this, std::move(socket), std::move(response)};
    } else {
      if(response.headers.contains("Content-Length")) {
        auto size = std::stoi(response.headers["Content-Length"]);
        response.body = co_await ReadBody(std::move(socket), size, flags...);
      };
      response.body = co_await ReadBody(std::move(socket), flags...);
      co_return response;
    };
  };
};

}  // namespace cserver::components
