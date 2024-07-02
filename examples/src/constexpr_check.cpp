#include <cserver/components/loggable_component_base.hpp>


struct SomeComponent {
  static constexpr utempl::ConstexprString kName = "component";

  constexpr SomeComponent(auto&) {};
};


struct SomeComponent2 {
  static constexpr utempl::ConstexprString kName = "component2";

  constexpr SomeComponent2(auto&) {};
};

struct OtherComponent {
  static constexpr utempl::ConstexprString kName = "other";

  auto Foo(auto& context) -> void {
    context.template FindComponent<"component2">(); // UB on runtime with old check
                                                    // Compile time error with new check(check via loopholes)
  };

  constexpr OtherComponent(auto& context) {
    context.template FindComponent<"component">();
    Foo(context);
  };
};


auto main() -> int {
  cserver::ServiceContextBuilder{}
    .AppendConfigParam<"threads", 8>()
    .Append<SomeComponent>()
    .Append<SomeComponent2>()
    .Append<OtherComponent>()
    .Sort()
  .Run();
};
