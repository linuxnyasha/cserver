#include <gtest/gtest.h>

#include <cserver/engine/components.hpp>
#include <nameof.hpp>

COMPONENT_REQUIRES(
    Some, requires(T t) {
      { t.f() } -> std::same_as<void>;
    });

struct SomeComponent {
  static constexpr utempl::ConstexprString kName = "some";
  explicit constexpr SomeComponent(auto& context) {
    context.template FindComponent<SomeM>();
  };
};

struct OtherComponent {
  static constexpr utempl::ConstexprString kName = "other";
  auto f() -> void {};
  explicit constexpr OtherComponent(auto& context) {};
};

struct OtherComponent2 {
  static constexpr utempl::ConstexprString kName = "other2";
  auto f() -> void {};
  explicit constexpr OtherComponent2(auto& context) {};
};

TEST(Meta, OneDependency) {
  constexpr auto builder = cserver::ServiceContextBuilder{}
                               .Append<SomeComponent>()
                               .Append<OtherComponent>()
                               .Append<OtherComponent2>()
                               .AppendConfigParam<"threads", 8>()
                               .Sort();
  constexpr auto dependencies = builder.GetDependencyGraph();
  using Need = const cserver::DependencyGraph<cserver::DependencyGraphElement<"other", {}>,
                                              cserver::DependencyGraphElement<"some", {utempl::ConstexprString{"other"}}>,
                                              cserver::DependencyGraphElement<"other2", {}>>;

  EXPECT_EQ(NAMEOF_TYPE(decltype(dependencies)), NAMEOF_TYPE(Need));
};

struct SomeStruct2 {
  constexpr SomeStruct2(auto, auto& context) {
    context.template FindAllComponents<SomeM>();
  };
};
