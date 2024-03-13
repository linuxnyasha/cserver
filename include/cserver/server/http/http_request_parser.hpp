#pragma once
#include <cserver/server/http/http_request.hpp>
#include <llhttp.h>

namespace cserver::server::http {

struct HTTPRequestParser : private llhttp_t, public HTTPRequest {
  bool err = false;
  bool done = false;
  std::string headerField = {};
  std::string headerValue = {};
  std::string urlString = {};
  inline HTTPRequestParser(std::string_view data) {
    llhttp_settings_t settings;
    llhttp_settings_init(&settings);
    settings.on_method = HTTPRequestParser::OnMethod;
    settings.on_url = HTTPRequestParser::OnUrl;
    settings.on_url_complete = HTTPRequestParser::OnUrlComplete;
    settings.on_header_field = HTTPRequestParser::OnHeaderField;
    settings.on_header_value = HTTPRequestParser::OnHeaderValue;
    settings.on_headers_complete = HTTPRequestParser::OnHeaderComplete;
    settings.on_body = HTTPRequestParser::OnBody;
    settings.on_message_complete = HTTPRequestParser::OnMessageComplete;
    llhttp_init(this, HTTP_BOTH, &settings);
    llhttp_execute(this, data.data(), data.size());
  };
  static inline auto OnMethod(llhttp_t* parser, const char* data, std::size_t size) -> int {
    auto* self = static_cast<HTTPRequest*>(static_cast<HTTPRequestParser*>(parser));
    self->method.append(data, size);
    return 0;
  };
  static inline auto OnUrl(llhttp_t* parser, const char* data, std::size_t size) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->urlString.append(data, size);
    return 0;
  };
  static inline auto OnUrlComplete(llhttp_t* parser) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->url = boost::urls::url(self->urlString);
    return 0;
  };
  static inline auto OnHeaderField(llhttp_t* parser, const char* data, std::size_t size) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->headerField.append(data, size);
    return 0;
  };
  static inline auto OnHeaderValue(llhttp_t* parser, const char* data, std::size_t size) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->headerValue.append(data, size);
    return 0;
  };
  static inline auto OnHeaderComplete(llhttp_t* parser) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->headers.emplace(std::move(self->headerField), std::move(self->headerValue));
    self->headerValue.clear();
    self->headerField.clear();
    return 0;
  };
  static inline auto OnBody(llhttp_t* parser, const char* data, std::size_t size) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->body.append(data, size);
    return 0;
  };
  static inline auto OnMessageComplete(llhttp_t* parser) -> int {
    auto* self = static_cast<HTTPRequestParser*>(parser);
    self->done = true;
    return 0;
  };
};

}  // namespace cserver::server::http
