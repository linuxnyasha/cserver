#pragma once
#include <cserver/engine/components.hpp>
#include <cserver/server/http/http_request.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/server/http/http_stream.hpp>
#include <cserver/engine/coroutine.hpp>

namespace cserver::server::handlers {

template <typename T>
struct HTTPHandlerBase {
  template <utempl::ConstexprString name>
  static consteval auto Adder(const auto& context) {
    return context.TransformComponents([]<typename TT>(const ComponentConfig<T::kHandlerManagerName, TT>) {
      return ComponentConfig<T::kHandlerManagerName, typename TT::template AddHandler<ComponentConfig<name, T>>>{};
    });
  };
  inline auto HandleRequest(cserver::server::http::HTTPRequest&& request,
                            cserver::server::http::HTTPResponse& response) -> Task<std::string> requires requires(T t) {t.HandleRequestThrow(std::move(request), response);} {
    try {
      co_return co_await static_cast<T&>(*this).HandleRequestThrow(std::move(request), response);
    } catch(const std::exception& err) {
      fmt::println("Error in handler with default name {}: {}", T::kName, err.what());
    } catch(...) {
      fmt::println("Error in handler with default name {}: Unknown Error", T::kName);
    };
    response.statusCode = 500;
    response.statusMessage = "Internal Server Error";
    co_return "Internal Server Error";
  };
  inline auto HandleRequestStream(cserver::server::http::HTTPRequest&& request,
                                  cserver::server::http::HTTPStream& stream) -> Task<void> requires requires(T t) {t.HandleRequestStreamThrow(std::move(request), stream);} {
    try {
      co_await static_cast<T&>(*this).HandleRequestStreamThrow(std::move(request), stream);
    } catch(const std::exception& err) {
      fmt::println("Error in handler with default name {}: {}", T::kName, err.what());
    } catch(...) {
      fmt::println("Error in handler with default name {}: Unknown Error", T::kName);
    };
    co_await stream.Close();
  };
  inline constexpr HTTPHandlerBase(auto, auto&) {};

};

} // namespace cserver::server::handlers
