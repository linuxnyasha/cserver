#pragma once
#include <cserver/server/http/http_request.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/engine/coroutine.hpp>

namespace cserver::clients::http {
template <typename HttpClient>
struct Request {
  HttpClient& client;
  server::http::HTTPRequest request;
  Request(HttpClient& client) :
      client(client) {
    this->AddHeader("User-Agent", "cserver/1");
  };
  inline constexpr auto Post() && -> Request<HttpClient>&& {
    this->request.method = "POST";
    return std::move(*this);
  };
  inline constexpr auto Post() & -> Request<HttpClient>& {
    this->request.method = "POST";
    return *this;
  };
  inline constexpr auto Get() && -> Request<HttpClient>&& {
    this->request.method = "GET";
    return std::move(*this);
  };
  inline constexpr auto Get() & -> Request<HttpClient>& {
    this->request.method = "GET";
    return *this;
  };
  inline constexpr auto Put() && -> Request<HttpClient>&& {
    this->request.method = "PUT";
    return std::move(*this);
  };
  inline constexpr auto Put() & -> Request<HttpClient>& {
    this->request.method = "PUT";
    return *this;
  };
  inline constexpr auto SetCustomMethod(std::string method) & -> Request<HttpClient>& {
    this->request.method = std::move(method);
    return *this;
  };
  inline constexpr auto SetCustomMethod(std::string method) && -> Request<HttpClient>&& { 
    this->request.method = std::move(method);
    return std::move(*this);
  };
  inline constexpr auto AddHeader(std::string first, std::string second) && -> Request<HttpClient>&& {
    this->request.headers.emplace(std::move(first), std::move(second));
    return std::move(*this);
  };
  inline constexpr auto AddHeader(std::string first, std::string second) & -> Request<HttpClient>& {
    this->request.headers.emplace(std::move(first), std::move(second));
    return *this;
  };
  inline constexpr auto AddHeaderIfNotExists(std::string check, std::string first, std::string second) & -> Request<HttpClient>& {
    if(!this->request.headers.contains(std::move(check))) {
      return this->AddHeader(std::move(first), std::move(second));
    };
    return *this;
  };
  inline constexpr auto AddHeaderIfNotExists(std::string check, std::string first, std::string second) && -> Request<HttpClient>&& {
    if(!this->request.headers.contains(std::move(check))) {
      return std::move(*this).AddHeader(std::move(first), std::move(second));
    };
    return std::move(*this);
  };
  inline constexpr auto Url(std::string url) & -> Request<HttpClient>& {
    this->request.url = boost::urls::url{std::move(url)};
    auto authority = this->request.url.authority();
    return this->AddHeader("Host", std::string{authority.data(), authority.size()});
  };
  inline constexpr auto Url(std::string url) && -> Request<HttpClient>&& {
    this->request.url = boost::urls::url{std::move(url)};
    auto authority = this->request.url.authority();
    return std::move(*this).AddHeader("Host", std::string{authority.data(), authority.size()});
  };
  inline constexpr auto Data(std::string data) && -> Request<HttpClient>&& {
    this->request.body = std::move(data);
    return std::move(*this);
  };
  inline constexpr auto ToString() const -> std::string {
    return this->request.ToString();
  };
  template <typename... Flags>
  inline auto Perform(Flags&&... flags) &&
    -> decltype(this->client.PerformRequest(std::move(*this).AddHeaderIfNotExists("Transfer-Encoding", "Content-Length", std::to_string(this->request.body.size())), std::forward<Flags>(flags)...)) {
    co_return co_await this->client.PerformRequest(std::move(*this).AddHeaderIfNotExists("Transfer-Encoding", "Content-Length", std::to_string(this->request.body.size())), std::forward<Flags>(flags)...);
  };
  template <typename... Flags>
  inline auto Perform(Flags&&... flags) & 
    -> decltype(this->client.PerformRequest(this->AddHeaderIfNotExists("Transfer-Encoding", "Content-Length", std::to_string(this->request.body.size())), std::forward<Flags>(flags)...)) {
    co_return co_await this->client.PerformRequest(this->AddHeaderIfNotExists("Transfer-Encoding", "Content-Length", std::to_string(this->request.body.size())), std::forward<Flags>(flags)...);
  };
};

};
