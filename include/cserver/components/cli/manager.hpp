#pragma once
#include <utempl/optional.hpp>
#include <cserver/engine/not_implemented.hpp>
#include <cserver/engine/components.hpp>
#include <boost/program_options.hpp>
#include <boost/type_index.hpp>
#include <iostream>

namespace cserver::cli {

template <std::size_t N, std::size_t NN, typename T>
struct OptionConfig {
  utempl::ConstexprString<N> name;

  utempl::ConstexprString<NN> description;
  using Type = T;
};

template <typename T, OptionConfig... Configs>
struct StructConfig {
  using Type = T;
  static constexpr auto kValue = utempl::Tuple{Configs...};
};




template <typename T, std::size_t N, std::size_t NN>
consteval auto CreateOptionConfig(utempl::ConstexprString<N> name,
                                  utempl::ConstexprString<NN> description) -> OptionConfig<N, NN, T> {
  return {name, description};
};



template <StructConfig... Configs>
struct Manager {
  boost::program_options::variables_map variableMap;
  static constexpr utempl::ConstexprString kName = "cliManager";
  constexpr Manager(auto& context) {
    boost::program_options::options_description general("General options");
    general.add_options()
      ("help,h", "Show help");
    ([&]{
      using Current = decltype(Configs)::Type;
      boost::program_options::options_description desc(fmt::format("{} options",
                                                                  boost::typeindex::type_id<Current>().pretty_name()));
      utempl::Unpack(utempl::PackConstexprWrapper<decltype(Configs)::kValue>(), [&](auto... vs) {
        auto&& add = desc.add_options();
        ([&]{
          if constexpr((*vs).description.size() == 0) {
            add((*vs).name.data.begin(),
                boost::program_options::value<typename decltype(*vs)::Type>());
          } else {
             add((*vs).name.data.begin(),
                 boost::program_options::value<typename decltype(*vs)::Type>(),
                 (*vs).description.data.begin());          
          };
        }(), ...);
      });
      general.add(desc);
    }(), ...);
    boost::program_options::store(boost::program_options::parse_command_line(context.argc, context.argv, general), this->variableMap);
    boost::program_options::notify(this->variableMap);
    if(this->variableMap.count("help")) {
      std::cout << general << std::endl;
      context.template FindComponent<kBasicTaskProcessorName>().ioContext.stop();
      return;
    };
  };

  template <StructConfig Config>
  constexpr auto Get() {
    constexpr utempl::Tuple fieldConfigs = Config.kValue;
    return [&](auto... is) -> decltype(Config)::Type {
      return {[&]{
        constexpr auto fieldConfig = utempl::Get<is>(fieldConfigs);
        if(this->variableMap.contains(fieldConfig.name.data.begin())) {
          return this->variableMap[fieldConfig.name.data.begin()].template as<typename decltype(fieldConfig)::Type>();
        };
        throw std::runtime_error(fmt::format("Not found cli option {}", static_cast<std::string_view>(fieldConfig.name)));
      }()...};
    } | utempl::kSeq<utempl::kTupleSize<decltype(fieldConfigs)>>;
  };


  template <StructConfig Config>
  using AddStructConfig = Manager<Configs..., Config>;
};

} // namespace cserver::cli
