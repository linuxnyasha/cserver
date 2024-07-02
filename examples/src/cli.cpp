#include <cserver/components/cli/manager.hpp>
#include <cserver/components/cli/struct.hpp>
#include <cserver/server/server/server.hpp>
#include <cserver/server/handlers/http_handler_base.hpp>


struct SomeStruct {
  static_assert(utempl::OpenStruct<SomeStruct>());
  utempl::FieldAttribute<std::string> field;
  static_assert(utempl::CloseStruct<SomeStruct>());
};

struct SomeComponent : cserver::cli::StructWithAdder<SomeComponent, SomeStruct> {
  using cserver::cli::StructWithAdder<SomeComponent, SomeStruct>::StructWithAdder;
};

struct SomeOtherComponent : public cserver::server::handlers::HttpHandlerBaseWithAdder<SomeOtherComponent> {
  static constexpr utempl::ConstexprString kPath = "/v1/some/";
  static constexpr utempl::ConstexprString kName = "name";
  static constexpr utempl::ConstexprString kHandlerManagerName = "server";
  SomeComponent& some;
  inline constexpr SomeOtherComponent(auto& context) :
      HttpHandlerBaseWithAdder(context),
      some(context.template FindComponent<"component">()) {};
 
  inline auto HandleRequestThrow(const cserver::server::http::HttpRequest&) -> cserver::Task<cserver::server::http::HttpResponse> {
    co_return cserver::server::http::HttpResponse{.body = this->some.field};
  };
};



auto main(int argc, const char** argv) -> int {
  cserver::ServiceContextBuilder{}
    .AppendConfigParam<"threads", 8>()
    .AppendConfigParam<"logging", cserver::ConstexprConfig{}
      .Append<"level">(cserver::LoggingLevel::kTrace)>()
    .AppendConfigParam<"cliArgs", utempl::ConstexprString{"cliManager"}>()
    .AppendConfigParam<"server", cserver::ConstexprConfig{}
      .Append<"taskProcessor">(utempl::ConstexprString{"basicTaskProcessor"})
      .Append<"port">(55555)>()
    .Append<cserver::Logging>()
    .Append<cserver::cli::Manager<>>()
    .Append<cserver::server::server::Server<>>()
    .Append<SomeComponent, "component">()
    .Append<SomeOtherComponent, "other">()
    .Sort()
  .Run(argc, argv);
};
