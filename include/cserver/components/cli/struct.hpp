#pragma once
#include <cserver/engine/not_implemented.hpp>
#include <cserver/engine/components.hpp>
#include <cserver/components/cli/manager.hpp>
#include <utempl/attributes.hpp>
#include <boost/pfr.hpp>

namespace cserver::cli {

template <utempl::ConstexprString Value>
struct Name {
  static constexpr utempl::ConstexprString kValue = Value;
};

template <utempl::ConstexprString Value>
struct Description {
  static constexpr utempl::ConstexprString kValue = Value;
};


template <typename T>
struct IsNameM {
  static constexpr bool value = utempl::Overloaded(
    []<utempl::ConstexprString V>(utempl::TypeList<Name<V>>) {
      return true;
    },
    [](auto) {return false;}
  )(utempl::kType<T>);
};

template <typename T>
struct IsDescriptionM {
  static constexpr bool value = utempl::Overloaded(
    []<utempl::ConstexprString V>(utempl::TypeList<Description<V>>) {
      return true;
    },
    [](auto) {return false;}
  )(utempl::kType<T>);
};






template <typename Self, typename T>
struct Struct : T {
  static constexpr utempl::ConstexprString kCliManagerName = "cliManager";
  using Type = T;
  template <std::size_t I, template <typename...> typename F, typename Default>
  static consteval auto GetFirstOrDefault(Default&& def) {
    constexpr auto options = Get<I>(utempl::GetAttributes<T>());
    if constexpr(std::is_same_v<decltype(options), const utempl::NoInfo>) {
      return std::forward<Default>(def);
    } else {
      if constexpr(Find<F>(options) == Size(options)) {
        return std::forward<Default>(def);
      } else {
        return decltype(utempl::Get<Find<F>(options)>(options))::kValue;
      };
    };
  };


  static consteval auto GetConfig() {
    static constexpr auto names = boost::pfr::names_as_array<T>();
    return [&](auto... is) {
      return StructConfig<Self, [&] {
        return CreateOptionConfig<std::remove_reference_t<decltype(boost::pfr::get<is>(std::declval<T&>()))>>(
          GetFirstOrDefault<is, IsNameM>(utempl::ConstexprString<names[is].size() + 1>(names[is].data())),
          GetFirstOrDefault<is, IsDescriptionM>(utempl::ConstexprString<0>{}));
      }()...>{};
    } | utempl::kSeq<boost::pfr::tuple_size_v<T>>;
  };


  constexpr Struct(auto& context) : 
      T(context.template FindComponent<Self::kCliManagerName>().template Get<GetConfig()>()) {};



  template <utempl::ConstexprString Name, Options>
  static consteval auto CliStructAdder(const auto& context) {
    return context.TransformComponents([]<typename TT, Options Options>(ComponentConfig<Self::kCliManagerName, TT, Options>) {
      return ComponentConfig<Self::kCliManagerName, typename TT::template AddStructConfig<GetConfig()>, Options>{};
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
