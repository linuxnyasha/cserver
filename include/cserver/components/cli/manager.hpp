#pragma once
#include <utempl/optional.hpp>
#include <cserver/engine/not_implemented.hpp>
#include <cserver/engine/components.hpp>

namespace cserver::cli {

template <std::size_t N, std::size_t NN, std::size_t NNN, typename T>
struct OptionConfig {
  utempl::ConstexprString<N> longName;
  utempl::Optional<utempl::ConstexprString<NN>> shortName;
  utempl::ConstexprString<NNN> description;
  using Type = T;
};


template <OptionConfig... Configs>
struct Manager {
  int argc;
  const char** argv;
  static constexpr utempl::ConstexprString kName = "cliManager";
  constexpr Manager(auto& context) : 
      argc(context.argc),
      argv(context.argv) {};
  template <utempl::ConstexprString LongName>
  constexpr auto Get() -> decltype(Get<Find(utempl::Tuple{Configs.longName...}, LongName)>(utempl::kTypeList<typename decltype(Configs)::Type...>)) {
    throw engine::NotImplemented{__PRETTY_FUNCTION__};
  };
  template <utempl::Tuple NewConfigs>
  using AddConfigs = decltype(utempl::Unpack(utempl::PackConstexprWrapper<NewConfigs>(), 
                                              [](auto... vs) -> Manager<Configs..., vs...> {
                                                std::unreachable();
                                              }));
};

} // namespace cserver::cli
