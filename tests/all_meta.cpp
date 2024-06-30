#include <gtest/gtest.h>
#include <cserver/engine/components.hpp>
#include <boost/type_index.hpp>

COMPONENT_REQUIRES(Some, requires(T t){{t.f()} -> std::same_as<void>;});

struct SomeComponent {
  static constexpr utempl::ConstexprString kName = "some";
  constexpr SomeComponent(auto, auto& context) {
    context.template FindAllComponents<SomeM>();
  };
};

struct OtherComponent {
  static constexpr utempl::ConstexprString kName = "other";
  auto f() -> void {};
  constexpr OtherComponent(auto, auto& context) {};
};

struct OtherComponent2 {
  static constexpr utempl::ConstexprString kName = "other2";
  auto f() -> void {};
  constexpr OtherComponent2(auto, auto& context) {};
};




TEST(Meta, AllDependencies) { 
  constexpr auto builder = cserver::ServiceContextBuilder{}
    .Append<SomeComponent>()
    .Append<OtherComponent>()
    .Append<OtherComponent2>()
    .AppendConfigParam<"threads", 8>()
    .Sort();

  constexpr auto dependencies = builder.GetDependencyGraph();
  using Need = utempl::Tuple<OtherComponent&, OtherComponent2&>;
  using R = decltype(
    builder
      .GetServiceContext()
      .GetContextFor<cserver::ComponentConfig<"some", SomeComponent, {}>>()
      .FindAllComponents<SomeM>());

  EXPECT_EQ(boost::typeindex::type_id<R>().pretty_name(),
            boost::typeindex::type_id<Need>().pretty_name());
  
  using DependenciesNeed = const cserver::DependencyGraph<
                  cserver::DependencyGraphElement<
                    "other",
                    {}>,
                  cserver::DependencyGraphElement<
                    "other2",
                    {}>,
                  cserver::DependencyGraphElement<
                    "some",
                    {utempl::ConstexprString{"other"}, utempl::ConstexprString{"other2"}}>>;

  EXPECT_EQ(boost::typeindex::type_id<decltype(dependencies)>().pretty_name(),
            boost::typeindex::type_id<DependenciesNeed>().pretty_name());
};
