#pragma once
#include <fmt/format.h>

#include <boost/url.hpp>
#include <sstream>
#include <unordered_map>

namespace cserver::server::http {

struct HttpRequest {
  std::string method = {};
  boost::urls::url url = {};
  std::unordered_map<std::string, std::string> headers = {};
  std::string body = {};
  [[nodiscard]] inline auto ToString() const -> std::string {
    std::ostringstream stream;
    stream << fmt::format("{} {} HTTP/1.1\r\n", this->method, this->url.path());
    for(const auto& header : this->headers) {
      stream << fmt::format("{}: {}\r\n", header.first, header.second);
    };
    stream << fmt::format("\r\n{}\r\n", this->body);
    return stream.str();
  };
};

}  // namespace cserver::server::http
