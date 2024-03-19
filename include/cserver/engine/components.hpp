#pragma once
#include <utempl/utils.hpp>
#include <thread>

namespace cserver {
struct SeparatedComponent {};
template <typename... Ts>
using Options = utempl::TypeList<Ts...>;

template <utempl::ConstexprString name, typename T, Options Options>
struct ComponentConfig {};

namespace impl {

template <utempl::ConstexprString name, typename T>
struct NamedValue {
  T value;
};

template <typename T>
using GetTypeFromComponentConfig = decltype(
  []<utempl::ConstexprString name, typename TT, Options Options>(const ComponentConfig<name, TT, Options>&) -> TT {
  }(std::declval<T>()));

template <typename T>
inline constexpr utempl::ConstexprString kNameFromComponentConfig = 
  decltype([]<utempl::ConstexprString name, typename TT, Options Options>(const ComponentConfig<name, TT, Options>&) {
    return utempl::Wrapper<name>{};
  }(std::declval<T>()))::kValue;

template <typename T>
inline constexpr auto TransformIfOk(T&& value, auto&& f) {
  if constexpr(requires{f(std::forward<T>(value));}) {
    return f(std::forward<T>(value));
  } else {
    return value;
  };
};


} // namespace impl

template <typename... Ts>
struct ConstexprConfig {
  utempl::Tuple<Ts...> data;
  template <utempl::ConstexprString Key>
  inline constexpr auto Get() const -> auto {
    return [&]<typename... TTs, utempl::ConstexprString... names>(const ConstexprConfig<impl::NamedValue<names, TTs>...>&){
      constexpr auto list = utempl::TypeList<utempl::Wrapper<names>...>{};
      constexpr std::size_t I = Find<utempl::Wrapper<Key>>(list);
      if constexpr(I < sizeof...(Ts)) {
        return utempl::Get<I>(this->data).value;
      };
    }(*this);
  };
  template <utempl::ConstexprString Key, typename T>
  inline constexpr auto Append(T&& value) const -> ConstexprConfig<Ts..., impl::NamedValue<Key, std::remove_cvref_t<T>>> {
    return {.data = TupleCat(this->data, utempl::MakeTuple(impl::NamedValue<Key, std::remove_cvref_t<T>>{std::forward<T>(value)}))};
  };
};

template <ConstexprConfig config, auto names, auto Options, typename... Ts>
struct ServiceContext {
  static constexpr auto kConfig = config;
  static constexpr auto kList = utempl::TypeList<Ts...>{};
  static constexpr auto kThreadsCount = config.template Get<"threads">();
  utempl::Tuple<Ts...> storage;
  inline constexpr ServiceContext() :
      storage{
        [&]<auto... Is>(std::index_sequence<Is...>) -> utempl::Tuple<Ts...> {
          return {[&]<auto I>(utempl::Wrapper<I>) {
            return [&]() -> std::remove_cvref_t<decltype(Get<I>(storage))> {
              return {utempl::Wrapper<Get<I>(names)>{}, *this};
            };
          }(utempl::Wrapper<Is>{})...};
        }(std::index_sequence_for<Ts...>())
      } {};
  inline constexpr auto Run() {
    [&]<auto... Is>(std::index_sequence<Is...>) {
      ([](auto& component){
        if constexpr(requires{component.Run();}) {
          component.Run();
        }; 
      }(Get<Is>(this->storage)), ...);
    }(std::index_sequence_for<Ts...>());
  };
  template <utempl::ConstexprString name>
  inline constexpr auto FindComponent() -> auto& {
    constexpr auto I = utempl::Find(names, name);
    return Get<I>(this->storage);
  };
  template <typename T>
  inline constexpr auto FindComponent() -> T& {
    constexpr auto I = utempl::Find<std::remove_cvref_t<T>>(utempl::kTypeList<Ts...>);
    return Get<I>(this->storage);
  };

};



template <ConstexprConfig config = {}, typename... Ts>
struct ServiceContextBuilder {
  static constexpr auto kList = utempl::kTypeList<Ts...>;
  static constexpr auto kConfig = config;
  template <typename T, utempl::ConstexprString name = T::kName, typename... Os>
  static consteval auto Append(Options<Os...> = {}) -> decltype(T::template Adder<name, Options<Os...>{}>(ServiceContextBuilder<config, Ts..., ComponentConfig<name, T, Options<Os...>{}>>{})) {
    return {};
  };

  template <typename T, utempl::ConstexprString name = T::kName, typename... Os>
  static consteval auto Append(Options<Os...> = {}) -> ServiceContextBuilder<config, Ts..., ComponentConfig<name, T, Options<Os...>{}>> 
        requires (!requires(ServiceContextBuilder<config, Ts..., ComponentConfig<name, T, Options<Os...>{}>> builder) {T::template Adder<name, Options<Os...>{}>(builder);}) {
    return {};
  };


  template <utempl::ConstexprString Key, auto Value>
  static consteval auto AppendConfigParam() -> ServiceContextBuilder<config.template Append<Key>(Value), Ts...> {
    return {};
  };

  template <typename F>
  static consteval auto TransformComponents(F&& f) -> ServiceContextBuilder<config, decltype(impl::TransformIfOk(Ts{}, f))...> {
    return {};
  };
  template <utempl::ConstexprString name>
  static consteval auto FindComponent() {
    return []<typename... TTs, utempl::ConstexprString... names, Options... Options>
    (const ServiceContextBuilder<config, ComponentConfig<names, TTs, Options>...>&) 
        -> decltype(utempl::Get<Find<utempl::Wrapper<name>>(utempl::TypeList<utempl::Wrapper<names>...>{})>(utempl::TypeList<TTs...>{})) {
      std::unreachable();
    }(ServiceContextBuilder<config, Ts...>{});
  };

  static consteval auto Config() {
    return config;
  };

  static constexpr auto Run() -> void {
    []<utempl::ConstexprString... names, typename... TTs, Options... Options>
    (utempl::TypeList<ComponentConfig<names, TTs, Options>...>) {
      ServiceContext<config, utempl::Tuple{names...}, utempl::Tuple{Options...}, TTs...> context;
      context.Run();
      for(;;) {
        std::this_thread::sleep_for(std::chrono::minutes(1));
      };
    }(utempl::TypeList<Ts...>{});
  };
};
} // namespace cserver
