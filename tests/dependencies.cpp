#include <gtest/gtest.h>

#include <cserver/engine/components.hpp>
#include <nameof.hpp>

struct SomeComponent {
  static constexpr utempl::ConstexprString kName = "some";
  explicit constexpr SomeComponent(auto&) {};
};

struct SomeOtherComponent {
  SomeComponent& component;
  static constexpr utempl::ConstexprString kName = "other";
  explicit constexpr SomeOtherComponent(auto& context) : component(context.template FindComponent<"some">()) {};
};

TEST(Dependencies, Get) {
  constexpr auto dependencies = cserver::ServiceContextBuilder{}
                                    .Append<SomeOtherComponent>()
                                    .Append<SomeComponent>()
                                    .AppendConfigParam<"threads", 8>()
                                    .Sort()
                                    .GetDependencyGraph();
  using Need = const cserver::DependencyGraph<cserver::DependencyGraphElement<"some", {}>,
                                              cserver::DependencyGraphElement<"other", {utempl::ConstexprString{"some"}}>>;
  EXPECT_EQ(NAMEOF_TYPE(decltype(dependencies)), NAMEOF_TYPE(Need));
};
