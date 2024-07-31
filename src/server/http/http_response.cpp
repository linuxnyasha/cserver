export module cserver.server.http.http_response;
import fmt;
import std;

namespace cserver::server::http {

struct HttpResponse {
  std::uint16_t statusCode = 200;  // NOLINT
  std::string statusMessage = "OK";
  std::unordered_map<std::string, std::string> headers = {};
  std::string body = {};
  [[nodiscard]] inline auto ToString() const -> std::string {
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

}  // namespace cserver::server::http
