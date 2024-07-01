#include <gtest/gtest.h>
#include <cserver/engine/components.hpp>
#include <boost/type_index.hpp>

COMPONENT_REQUIRES(Some, requires(T t){{t.f()} -> std::same_as<void>;});

struct SomeComponent {
  static constexpr utempl::ConstexprString kName = "some";
  constexpr SomeComponent(auto& context) {
    context.template FindComponent<SomeM>();
  };
};

struct OtherComponent {
  static constexpr utempl::ConstexprString kName = "other";
  auto f() -> void {};
  constexpr OtherComponent(auto& context) {};
};

struct OtherComponent2 {
  static constexpr utempl::ConstexprString kName = "other2";
  auto f() -> void {};
  constexpr OtherComponent2(auto& context) {};
};


TEST(Meta, OneDependency) {
  constexpr auto builder = cserver::ServiceContextBuilder{}
    .Append<SomeComponent>()
    .Append<OtherComponent>()
    .Append<OtherComponent2>()
    .AppendConfigParam<"threads", 8>()
    .Sort();
  constexpr auto dependencies = builder.GetDependencyGraph();
  using Need = const cserver::DependencyGraph<
                  cserver::DependencyGraphElement<
                    "other",
                    {}>,
                  cserver::DependencyGraphElement<
                    "some",
                    {utempl::ConstexprString{"other"}}>,
                  cserver::DependencyGraphElement<
                    "other2",
                    {}>>;

  EXPECT_EQ(boost::typeindex::type_id<decltype(dependencies)>().pretty_name(),
            boost::typeindex::type_id<Need>().pretty_name());
};

struct SomeStruct2 {
  constexpr SomeStruct2(auto, auto& context) {
    context.template FindAllComponents<SomeM>();
  };
};




