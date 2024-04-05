#pragma once
#include <cserver/engine/components.hpp>
#include <cserver/server/http/http_request.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/server/http/http_stream.hpp>
#include <cserver/engine/coroutine.hpp>

namespace cserver::server::handlers {

struct HTTPHandlerBase {
  template <typename T, utempl::ConstexprString name, Options Options>
  static consteval auto Adder(const auto& context) {
    return context.TransformComponents([]<typename TT>(const ComponentConfig<T::kHandlerManagerName, TT, Options>) {
      return ComponentConfig<T::kHandlerManagerName, typename TT::template AddHandler<ComponentConfig<name, T, Options>>, Options>{};
    });
  };
  template <typename Self>
  inline auto HandleRequest(this Self&& self, cserver::server::http::HTTPRequest&& request,
                            cserver::server::http::HTTPResponse& response) -> Task<std::string> requires requires{self.HandleRequestThrow(std::move(request), response);} {
    using T = std::remove_cvref_t<Self>;
    try {
      co_return co_await std::forward<Self>(self).HandleRequestThrow(std::move(request), response);
    } catch(const std::exception& err) {
      fmt::println("Error in handler with default name {}: {}", T::kName, err.what());
    } catch(...) {
      fmt::println("Error in handler with default name {}: Unknown Error", T::kName);
    };
    response.statusCode = 500;
    response.statusMessage = "Internal Server Error";
    co_return "Internal Server Error";
  };
  template <typename Self>
  inline auto HandleRequestStream(this Self&& self, cserver::server::http::HTTPRequest&& request,
                                  cserver::server::http::HTTPStream& stream) -> Task<void> requires requires{self.HandleRequestStreamThrow(std::move(request), stream);} {
    using T = std::remove_cvref_t<Self>;
    try {
      co_await std::forward<Self>(self).HandleRequestStreamThrow(std::move(request), stream);
    } catch(const std::exception& err) {
      fmt::println("Error in handler with default name {}: {}", T::kName, err.what());
    } catch(...) {
      fmt::println("Error in handler with default name {}: Unknown Error", T::kName);
    };
    co_await stream.Close();
  };
  inline constexpr HTTPHandlerBase(auto, auto&) {};
};
template <typename T>
struct HTTPHandlerAdder {
  template <utempl::ConstexprString Name, Options Options>
  static consteval auto Adder(const auto& context) {
    return HTTPHandlerBase::template Adder<T, Name, Options>(context);
  };
};
template <typename T>
struct HTTPHandlerBaseWithAdder : HTTPHandlerBase, HTTPHandlerAdder<T> {
  inline constexpr HTTPHandlerBaseWithAdder(auto name, auto& context) :
      HTTPHandlerBase(name, context),
      HTTPHandlerAdder<T>{} {};
};

} // namespace cserver::server::handlers
