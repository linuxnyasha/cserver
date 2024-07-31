module;
#include <boost/program_options.hpp>
#include <nameof.hpp>
export module cserver.components.cli.manager;
import cserver.engine.components;
import cserver.engine.not_implemented;
import std;
import utempl;


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
consteval auto CreateOptionConfig(utempl::ConstexprString<N> name, utempl::ConstexprString<NN> description) -> OptionConfig<N, NN, T> {
  return {name, description};
};

template <StructConfig... Configs>
struct Manager {
  boost::program_options::variables_map variableMap;
  static constexpr utempl::ConstexprString kName = "cliManager";
  explicit constexpr Manager(auto& context) {
    boost::program_options::options_description general("General options");
    general.add_options()("help,h", "Show help");
    // clang-format off
    ([&]{
      using Current = decltype(Configs)::Type;
      boost::program_options::options_description desc(fmt::format("{} options",
                                                                  NAMEOF_TYPE(Current)));
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
    // clang-format-on
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
    return [&](auto... is) -> decltype(Config)::Type::Type {
      return {[&]{
        static constexpr auto fieldConfig = utempl::Get<is>(fieldConfigs);
        static constexpr std::string_view nnname = static_cast<std::string_view>(fieldConfig.name);
        constexpr std::string_view nname = nnname.substr(0, nnname.find(','));
        utempl::ConstexprString<nname.size() + 1> name;
        std::ranges::copy_n(nname.begin(), nname.size(), name.begin());
        name.data[nname.size()] = '\0';
        if(this->variableMap.count(name.begin())) {
          return this->variableMap[name.begin()].template as<typename decltype(fieldConfig)::Type>();
        };
        throw std::runtime_error(fmt::format("Not found cli option {}", name));
      }()...};
    } | utempl::kSeq<utempl::kTupleSize<decltype(fieldConfigs)>>;
  };


  template <StructConfig Config>
  using AddStructConfig = Manager<Configs..., Config>;
};

} // namespace cserver::cli
