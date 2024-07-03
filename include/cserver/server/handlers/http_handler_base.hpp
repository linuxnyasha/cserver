#pragma once
#include <cserver/engine/components.hpp>
#include <cserver/server/http/http_request.hpp>
#include <cserver/server/http/http_response.hpp>
#include <cserver/server/http/http_stream.hpp>
#include <cserver/engine/coroutine.hpp>
#include <cserver/components/loggable_component_base.hpp>

namespace cserver::server::handlers {


struct HttpHandlerBase : ComponentBase {
  template <typename T, utempl::ConstexprString name, Options>
  static consteval auto HttpHandlerAdder(const auto& context) {
    return context.TransformComponents([]<typename TT, Options Options>(const ComponentConfig<T::kHandlerManagerName, TT, Options>) {
      return ComponentConfig<T::kHandlerManagerName, typename TT::template AddHandler<ComponentConfig<name, T, Options>>, Options>{};
    });
  };
  template <typename Self>
  inline auto HandleRequest(this Self&& self, http::HttpRequest&& request
                            ) -> Task<http::HttpResponse> requires requires{self.HandleRequestThrow(std::move(request));} {
    using T = std::remove_cvref_t<Self>;
    try {
      co_return co_await std::forward<Self>(self).HandleRequestThrow(std::move(request));
    } catch(const std::exception& err) {
      auto typeName = boost::core::demangle(__cxxabiv1::__cxa_current_exception_type()->name());
      if(self.logging.level <= LoggingLevel::kWarning)
        self.logging.template Warning<"In handler with default name {} uncaught exception of type {}: {}">(T::kName, typeName, err.what());
    } catch(...) {
      auto typeName = boost::core::demangle(__cxxabiv1::__cxa_current_exception_type()->name());
      if(self.logging.level <= LoggingLevel::kWarning)
        self.logging.template Warning<"In handler with default name {} uncaught exception of type {}">(T::kName, typeName);
    };
    co_return http::HttpResponse{.statusCode = 500, .statusMessage = "Internal Server Error", .body = "Internal Server Error"};
  };
  template <typename Self>
  inline auto HandleRequestStream(this Self&& self, cserver::server::http::HttpRequest&& request,
                                  cserver::server::http::HttpStream& stream) -> Task<void> requires requires{self.HandleRequestStreamThrow(std::move(request), stream);} {
    using T = std::remove_cvref_t<Self>;
    try {
      co_await std::forward<Self>(self).HandleRequestStreamThrow(std::move(request), stream);
    } catch(const std::exception& err) {
      self.logging.template Warning<"Error in handler with default name {}: {}">(T::kName, err.what());
    } catch(...) {
      self.logging.template Warning<"Error in handler with default name {}: Unknown Error">(T::kName);
    };
    co_await stream.Close();
  };
  inline constexpr HttpHandlerBase(auto& context) :
      ComponentBase(context) {};
};
template <typename T>
struct HttpHandlerAdderType {
  template <utempl::ConstexprString Name, Options Options>
  static consteval auto Adder(const auto& context) {
    return HttpHandlerBase::template HttpHandlerAdder<T, Name, Options>(context);
  };
};
template <typename T>
struct HttpHandlerBaseWithAdder : HttpHandlerBase, HttpHandlerAdderType<T> {
  inline constexpr HttpHandlerBaseWithAdder(auto& context) :
      HttpHandlerBase(context),
      HttpHandlerAdderType<T>{} {};
};

} // namespace cserver::server::handlers
