#include <gtest/gtest.h>
#include <cserver/engine/components.hpp>
#include <boost/type_index.hpp>

struct SomeComponent {
  static constexpr utempl::ConstexprString kName = "some";
  constexpr SomeComponent(auto, auto&) {};
};


struct SomeOtherComponent {
  SomeComponent& component;
  static constexpr utempl::ConstexprString kName = "other";
  constexpr SomeOtherComponent(auto, auto& context) : component(context.template FindComponent<"some">()) {
  };
};


TEST(Dependencies, Get) {
  constexpr auto dependencies = cserver::ServiceContextBuilder{}
    .Append<SomeOtherComponent>()
    .Append<SomeComponent>()
    .Sort()
    .GetDependencyGraph();
  using Need = const cserver::DependencyGraph<
                  cserver::DependencyGraphElement<
                    "some",
                    {}>,
                  cserver::DependencyGraphElement<
                    "other",
                    {utempl::ConstexprString{"some"}}>>;
  EXPECT_EQ(boost::typeindex::type_id<decltype(dependencies)>().pretty_name(),
            boost::typeindex::type_id<Need>().pretty_name());
};
