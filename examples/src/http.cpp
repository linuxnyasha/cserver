#include <cserver/engine/basic/task_processor.hpp>
#include <cserver/server/handlers/http_handler_base.hpp>
#include <cserver/server/server/server.hpp>

struct SomeComponent : public cserver::server::handlers::HttpHandlerBaseWithAdder<SomeComponent> {
  static constexpr utempl::ConstexprString kPath = "/v1/some/";
  static constexpr utempl::ConstexprString kName = "name";
  static constexpr utempl::ConstexprString kHandlerManagerName = "server";
  explicit constexpr SomeComponent(auto& context) : HttpHandlerBaseWithAdder(context) {};

  inline auto HandleRequestThrow(const cserver::server::http::HttpRequest& request) -> cserver::Task<cserver::server::http::HttpResponse> {
    co_return cserver::server::http::HttpResponse{.body = request.url.data()};
  };
};

// clang-format off
// NOLINTBEGIN

auto main() -> int {
  cserver::ServiceContextBuilder{}
    .AppendConfigParam<"threads", 8>()
    .AppendConfigParam<"logging", cserver::ConstexprConfig{}
      .Append<"level">(cserver::LoggingLevel::kTrace)>()
    .AppendConfigParam<"server", cserver::ConstexprConfig{}
      .Append<"taskProcessor">(utempl::ConstexprString{"basicTaskProcessor"})
      .Append<"port">(55556)>()
    .Append<cserver::Logging>()
    .Append<cserver::server::server::Server<>>()
    .Append<SomeComponent, "component">()
    .Sort()
  .Run();
};

// NOLINTEND
// clang-format on
