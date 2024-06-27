#include <cserver/server/server/server.hpp>
#include <cserver/engine/basic/task_processor.hpp>
#include <cserver/server/handlers/http_handler_base.hpp>
 
struct SomeComponent : public cserver::server::handlers::HttpHandlerBaseWithAdder<SomeComponent> {
  static constexpr utempl::ConstexprString kPath = "/v1/some/";
  static constexpr utempl::ConstexprString kName = "name";
  static constexpr utempl::ConstexprString kHandlerManagerName = "server";
  inline constexpr SomeComponent(auto name, auto& context) :
      HttpHandlerBaseWithAdder(name, context) {};
 
  inline auto HandleRequestThrow(const cserver::server::http::HttpRequest& request) -> cserver::Task<cserver::server::http::HttpResponse> {
    co_return cserver::server::http::HttpResponse{.body = request.url.data()};
  };
};
 
auto main() -> int {
  cserver::ServiceContextBuilder{}
    .AppendConfigParam<"threads", 8>()
    .AppendConfigParam<"server", cserver::ConstexprConfig{}
      .Append<"taskProcessor">(utempl::ConstexprString{"basicTaskProcessor"})
      .Append<"port">(55555)>()
    .Append<cserver::server::server::Server<>>()
    .Append<SomeComponent, "component">()
    .Sort()
  .Run();
};
