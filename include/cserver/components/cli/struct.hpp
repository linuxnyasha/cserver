#pragma once
#include <cserver/engine/not_implemented.hpp>
#include <cserver/engine/components.hpp>
#include <utempl/attributes.hpp>

namespace cserver::cli {

template <typename Self, typename T>
struct Struct : T {
  static constexpr utempl::ConstexprString kCliManagerName = "cliManager";
  constexpr auto Parse(auto& manager) -> T {
    throw engine::NotImplemented{__PRETTY_FUNCTION__};
  };
  constexpr Struct(auto& context) : 
      T(Parse(context.template FindComponent<Self::kCliManagerName>())) {};

  static consteval auto GetConfigs() -> utempl::Tuple<> {
    return {}; /* Not Implemented */
  };

  template <utempl::ConstexprString Name, Options>
  static consteval auto CliStructAdder(const auto& context) {
    return context.TransformComponents([]<typename TT, Options Options>(ComponentConfig<Self::kCliManagerName, TT, Options>) {
      return ComponentConfig<Self::kCliManagerName, typename TT::template AddConfigs<GetConfigs()>, Options>{};
    });
  };
};

template <typename Self, typename T>
struct StructWithAdder : Struct<Self, T> {
  using Struct<Self, T>::Struct;
  template <utempl::ConstexprString Name, Options Options>
  static consteval auto Adder(const auto& context) {
    return Struct<Self, T>::template CliStructAdder<Name, Options>(context);
  };
};

} // namespace cserver::cli
