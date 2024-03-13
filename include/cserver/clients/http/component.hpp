#pragma once
#include <cserver/clients/http/request.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/engine/components.hpp>
#include <cserver/engine/coroutine.hpp>
#include <cserver/utils/boost_error_wrapper.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <utempl/constexpr_string.hpp>

namespace cserver::server::http {

inline constexpr auto ParseHttpHeader(std::string header) -> std::pair<std::string, std::string> {
  size_t pos = header.find(":");
  std::string key = header.substr(0, pos);
  std::string value = header.substr(pos + 2);
  return std::make_pair(key, value);

};
} // namespace cserver::server::http
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
    resolver(this->taskProcessor.ioContext) {}
  
  template <utempl::ConstexprString name, typename T>
  static consteval auto Adder(const T& context) { 
    using Type = std::remove_cvref_t<decltype(context.template FindComponent<"basicTaskProcessor">())>;
    return context.TransformComponents(
      [&](const ComponentConfig<name, HttpClient<>>&) -> ComponentConfig<name, HttpClient<Type>> {
        return {};
      });
  };
  template <typename T>
  inline auto PerformRequest(T&& request) -> cserver::Task<server::http::HTTPResponse> {
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(this->taskProcessor.ioContext, this->ctx);
    boost::asio::ip::tcp::resolver::iterator endpoint = 
      co_await this->resolver.async_resolve({request.request.url.host(), request.request.url.has_port() ? request.request.url.port() : request.request.url.scheme()}, boost::asio::use_awaitable);
    auto [ec, _] = co_await boost::asio::async_connect(socket.lowest_layer(), endpoint, boost::asio::as_tuple(boost::asio::use_awaitable));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
    co_await socket.async_handshake(boost::asio::ssl::stream_base::client, boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
    std::string req(request.request.ToString());
    co_await boost::asio::async_write(socket, boost::asio::buffer(req.data(), req.size()), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
    std::string serverResponse;
    co_await boost::asio::async_read_until(socket, boost::asio::dynamic_buffer(serverResponse), "\r\n\r\n", boost::asio::redirect_error(boost::asio::use_awaitable, ec));
    if(ec) {
      throw BoostErrorWrapper{ec};
    };
    server::http::HTTPResponse response;
    std::istringstream responseStream(std::move(serverResponse));
    std::string httpVersion;
    responseStream >> httpVersion >> response.statusCode;
    std::getline(responseStream, response.statusMessage);
    std::string header;
    while(std::getline(responseStream, header) && header.find(":") != std::string::npos) {
      response.headers.insert(server::http::ParseHttpHeader(std::move(header)));
    };
    if(response.statusCode != 200) {
      co_return response;
    };
    if(response.headers.contains("Content-Length")) {
      auto size = std::stoi(response.headers["Content-Length"]);
      response.body.reserve(size);
      co_await boost::asio::async_read(socket, boost::asio::dynamic_buffer(response.body), boost::asio::transfer_at_least(size), boost::asio::redirect_error(boost::asio::use_awaitable, ec));
      if(ec) {
        throw BoostErrorWrapper{ec};
      };
      co_return response;
    };
    for(;;) {
      auto [ec, n] = co_await boost::asio::async_read(socket, boost::asio::dynamic_buffer(response.body), boost::asio::transfer_at_least(1), boost::asio::as_tuple(boost::asio::use_awaitable));
      if(ec && ec == boost::asio::error::eof) {
        break;
      };
      if(ec) {
        throw BoostErrorWrapper{ec};
      };
    };
    co_return response;
  };
};

} // namespace cserver::clients::http
