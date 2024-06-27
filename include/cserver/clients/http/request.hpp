#pragma once
#include <cserver/server/http/http_request.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/engine/coroutine.hpp>

namespace cserver::clients::http {
template <typename HttpClient>
struct Request {
  HttpClient& client;
  server::http::HttpRequest request;
  Request(HttpClient& client) :
      client(client) {
    this->AddHeader("User-Agent", "cserver/1");
  };
  inline constexpr auto Post(this auto&& self) -> decltype(auto) {
    self.request.method = "POST";
    return self;
  };
  inline constexpr auto Get(this auto&& self) -> decltype(auto) {
    self.request.method = "GET";
    return self;
  };
  inline constexpr auto Put(this auto&& self) -> decltype(auto) {
    self.request.method = "PUT";
    return self;
  };
  inline constexpr auto SetCustomMethod(this auto&& self, std::string method) -> decltype(auto) {
    self.request.method = std::move(method);
    return self;
  };
  inline constexpr auto AddHeader(this auto&& self, std::string first, std::string second) -> decltype(auto) {
    self.request.headers.emplace(std::move(first), std::move(second));
    return self;
  };
  template <typename Self>
  inline constexpr auto AddHeaderIfNotExists(this Self&& self,
                                             std::string check,
                                             std::string first,
                                             std::string second) -> decltype(auto) {
    if(!self.request.headers.contains(std::move(check))) {
      return std::forward<Self>(self).AddHeader(std::move(first), std::move(second));
    };
    return self;
  };
  template <typename Self>
  inline constexpr auto Url(this Self&& self,
                            std::string url) -> auto&& {
    self.request.url = boost::urls::url{std::move(url)};
    auto authority = self.request.url.authority();
    return std::forward<Self>(self).AddHeader("Host", std::string{authority.data(), authority.size()});
  };
  inline constexpr auto Data(this auto&& self, std::string data) -> decltype(auto) {
    self.request.body = std::move(data);
    return self;
  };
  inline constexpr auto ToString() const -> std::string {
    return this->request.ToString();
  };
  template <typename Self, typename... Flags>
  inline auto Perform(this Self&& self, Flags&&... flags)
    -> decltype(self.client.PerformRequest(std::forward<Self>(self).AddHeaderIfNotExists("Transfer-Encoding", "Content-Length", std::to_string(self.request.body.size())), std::forward<Flags>(flags)...)) {
    HttpClient& client = self.client;
    std::string size = std::to_string(self.request.body.size());
    co_return co_await client.PerformRequest(std::forward<Self>(self).AddHeaderIfNotExists("Transfer-Encoding", "Content-Length", std::move(size)), std::forward<Flags>(flags)...);
  };

};

};
