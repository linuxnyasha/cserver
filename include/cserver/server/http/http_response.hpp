#pragma once
#include <unordered_map>
#include <fmt/format.h>
#include <sstream>

namespace cserver::server::http {

struct HTTPResponse {
  unsigned short statusCode = 200;
  std::string statusMessage = "OK";
  std::unordered_map<std::string, std::string> headers = {};
  std::string body = {};
  inline auto ToString() const -> std::string {
    std::ostringstream stream;
    stream << fmt::format("HTTP/1.1 {} {}\r\n", this->statusCode, this->statusMessage);
    for(const auto& header : this->headers) {
      stream << fmt::format("{}: {}\r\n", header.first, header.second);
    };
    if(!this->body.empty()) {
      stream << fmt::format("\r\n{}\r\n", this->body);
    };
    return stream.str();
  };
};

} // namespace cserver::server::http
