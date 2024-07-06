#include <cserver/components/loggable_component_base.hpp>

struct SomeComponent : cserver::ComponentBase {
  static constexpr utempl::ConstexprString kName = "component";
  using cserver::ComponentBase::ComponentBase;

  constexpr auto Run() -> void {
    LOG_DEBUG<"Hello {}">("world!");
  };
};

// clang-format off
// NOLINTBEGIN
auto main() -> int {
  cserver::ServiceContextBuilder{}
    .AppendConfigParam<"threads", 8>()
    .AppendConfigParam<"logging", cserver::ConstexprConfig{}
      .Append<"level">(cserver::LoggingLevel::kTrace)>()
    .Append<cserver::Logging>()
    .Append<SomeComponent>()
    .Sort()
  .Run();
};

// NOLINTEND
// clang-format on
