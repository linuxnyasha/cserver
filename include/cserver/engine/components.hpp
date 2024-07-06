#pragma once
#include <cxxabi.h>

#include <boost/core/demangle.hpp>
#include <cserver/engine/basic/task_processor.hpp>
#include <cserver/engine/coroutine.hpp>
#include <utempl/loopholes/counter.hpp>
#include <utempl/utils.hpp>

namespace cserver {

inline constexpr utempl::ConstexprString kBasicTaskProcessorName = "basicTaskProcessor";

struct SeparatedComponent {};
template <typename... Ts>
struct Options : utempl::TypeList<Ts...> {};

template <utempl::ConstexprString Name, typename T, Options Options>
struct ComponentConfig {
  using Type = T;
  static constexpr auto kOptions = Options;
  static constexpr auto kName = Name;
};

namespace impl {

template <utempl::ConstexprString name, typename T>
struct NamedValue {
  T value;
};

template <typename T>
using GetTypeFromComponentConfig = decltype([]<utempl::ConstexprString name, typename TT, Options Options>(
                                                const ComponentConfig<name, TT, Options>&) -> TT {}(std::declval<T>()));

template <typename T>
inline constexpr utempl::ConstexprString kNameFromComponentConfig =
    decltype([]<utempl::ConstexprString name, typename TT, Options Options>(const ComponentConfig<name, TT, Options>&) {
      return utempl::Wrapper<name>{};
    }(std::declval<T>()))::kValue;

template <typename T>
inline constexpr auto TransformIfOk(T&& value, auto&& f) {
  if constexpr(requires { f(std::forward<T>(value)); }) {
    return f(std::forward<T>(value));
  } else {
    return value;
  };
};

}  // namespace impl

template <typename... Ts>
struct ConstexprConfig {
  utempl::Tuple<Ts...> data;
  template <utempl::ConstexprString Key>
  inline constexpr auto Get() const -> auto {
    return [&]<typename... TTs, utempl::ConstexprString... names>(const ConstexprConfig<impl::NamedValue<names, TTs>...>&) {
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

namespace impl {

template <typename InitFlag, std::size_t>
inline constexpr auto GetInitFlagFor(auto&... args) -> InitFlag& {
  static InitFlag flag{args...};
  return flag;
};

template <typename InitFlag, utempl::Tuple Tuple>
constexpr auto TransformDependencyGraphToTupleWithDependenciesToInitedFlagChanges(auto&&... args) {
  return utempl::Map(utempl::PackConstexprWrapper<Tuple>(), [&]<auto Array>(utempl::Wrapper<Array>) {
    return utempl::Map(
        utempl::PackConstexprWrapper<Array, utempl::Tuple<>>(),
        [&](auto elem) {
          return &GetInitFlagFor<InitFlag, static_cast<std::size_t>(elem)>(args...);
        },
        utempl::kType<std::array<InitFlag*, Array.size()>>);
  });
};

struct AsyncConditionVariable {
  explicit inline AsyncConditionVariable(boost::asio::io_context& ioContext) : mtx{}, deadlineTimer(ioContext) {};
  template <typename Token = boost::asio::use_awaitable_t<>>
  inline auto AsyncWait(Token&& token = {}) -> Task<> {
    return boost::asio::async_initiate<Token, void(void)>(
        [this]<typename Handler>(Handler&& handler) -> void {
          std::unique_lock lc(this->mtx);
          if(this->flag) {
            return handler();
          };
          this->deadlineTimer.async_wait([h = std::make_unique<Handler>(std::forward<Handler>(handler))](const boost::system::error_code&) {
            return (*h)();
          });
        },
        token);
  };
  inline auto Wait() -> void {
    this->deadlineTimer.wait();
  };
  inline auto NotifyOne() {
    std::unique_lock lc(this->mtx);
    this->deadlineTimer.cancel_one();
  };
  inline auto NotifyAll() {
    std::unique_lock lc(this->mtx);
    this->flag = true;
    this->deadlineTimer.cancel();
  };
  std::mutex mtx;
  boost::asio::deadline_timer deadlineTimer;
  bool flag{};
};

template <typename Component, typename T>
struct ServiceContextForComponent {
  T& context;
  static constexpr auto kConfig = T::kConfig;
  static constexpr auto kUtils = T::kUtils;
  static constexpr auto kName = Component::kName;

  template <std::size_t I>
  constexpr auto FindComponent() -> decltype(this->context.template FindComponent<Component, I>()) {
    return this->context.template FindComponent<Component, I>();
  };

  template <utempl::ConstexprString Name>
  constexpr auto FindComponent() -> auto& requires(Name == kBasicTaskProcessorName ||
                                                   requires { this->FindComponent<kUtils.template GetIndexByName(Name)>(); }) {
    if constexpr(Name == kBasicTaskProcessorName) {
      return this->context.taskProcessor;
    } else {
      return this->FindComponent<kUtils.template GetIndexByName(Name)>();
    };
  };

  template <template <typename...> typename F>
  constexpr auto FindComponent() -> decltype(this->FindComponent<kUtils.template GetFirstIndex<F>()>()) {
    return this->FindComponent<kUtils.template GetFirstIndex<F>()>();
  };
  template <typename TT>
  constexpr auto FindComponent() -> decltype(this->FindComponent<kUtils.template GetIndexByType<TT>()>()) {
    return this->FindComponent<kUtils.template GetIndexByType<TT>()>();
  };
  template <template <typename...> typename F>
  constexpr auto FindAllComponents() {
    return utempl::Unpack(utempl::PackConstexprWrapper<kUtils.template GetAllIndexes<F>(), utempl::Tuple<>>(),
                          [&](auto... is) -> utempl::Tuple<decltype(*Get<is>(context.storage))&...> {
                            return {context.template FindComponent<Component, is>()...};
                          });
  };
};

template <typename Component, typename T>
struct ServiceContextForComponentWithCliArgs : ServiceContextForComponent<Component, T> {
  int argc;
  const char** argv;

  constexpr ServiceContextForComponentWithCliArgs(T& context, int argc, const char** argv) :
      ServiceContextForComponent<Component, T>(context), argc(argc), argv(argv) {};
};

template <utempl::Tuple DependencyGraph, typename T>
inline constexpr auto InitComponents(T& ccontext, int ac, const char** av) -> void {
  static auto& context = ccontext;
  static auto& taskProcessor = context.taskProcessor;
  static auto& ioContext = taskProcessor.ioContext;
  static const auto argc = ac;
  static const auto argv = av;
  static constexpr utempl::ConstexprString cliArgs = [] -> decltype(auto) {
    if constexpr(!std::is_same_v<decltype(T::kConfig.template Get<"cliArgs">()), void>) {
      return T::kConfig.template Get<"cliArgs">();
    } else {
      return "";
    };
  }();
  static utempl::Tuple inited =
      TransformDependencyGraphToTupleWithDependenciesToInitedFlagChanges<AsyncConditionVariable, DependencyGraph>(ioContext);
  auto work = make_work_guard(ioContext);
  []<std::size_t... Is>(std::index_sequence<Is...>) {
    // clang-format off
    (boost::asio::co_spawn(ioContext, []() -> cserver::Task<> {
      using Current = decltype(Get<Is>(decltype(T::kUtils)::kComponentConfigs));
      auto& dependencies = Get<Is>(inited);
      for(auto* flag : dependencies) {
        co_await flag->AsyncWait();
      };
      try {
        if constexpr(Current::kName == cliArgs) {
          ServiceContextForComponentWithCliArgs<Current, T> currentContext{context, argc, argv};
          Get<Is>(context.storage).emplace(currentContext);
        } else {
          ServiceContextForComponent<Current, T> currentContext{context};
          Get<Is>(context.storage).emplace(currentContext);
        };
      } catch(std::exception& error) {
        auto typeName = boost::core::demangle(__cxxabiv1::__cxa_current_exception_type()->name());
        fmt::println("{} initialisation:\n  terminating due to uncaught exception of type {}: {}",
                     static_cast<std::string_view>(Current::kName),
                     typeName,
                     error.what());
        ioContext.stop();
      } catch(...) {
        auto typeName = boost::core::demangle(__cxxabiv1::__cxa_current_exception_type()->name());
        fmt::println("{} initialisation:\n  terminating due to uncaught exception of type {}",
                     static_cast<std::string_view>(Current::kName),
                     typeName);
        ioContext.stop();
      };
      auto& componentInitFlag = GetInitFlagFor<AsyncConditionVariable, Is>(ioContext);
      componentInitFlag.NotifyAll();
      if constexpr(requires{Get<Is>(context.storage)->Run();}) {
        Get<Is>(context.storage)->Run();
      };
    }(), boost::asio::detached), ...);
    // clang-format on
  }(std::make_index_sequence<utempl::kTupleSize<decltype(DependencyGraph)>>());
};

template <typename... Ts>
struct DependenciesUtils {
  static constexpr utempl::TypeList<typename Ts::Type...> kTypeList;
  static constexpr auto kOptions = utempl::Tuple{Ts::kOptions...};
  static constexpr auto kNames = utempl::Tuple{Ts::kName...};
  static constexpr utempl::TypeList<Ts...> kComponentConfigs;
  template <typename T>
  static consteval auto GetIndexByType() -> std::size_t {
    return utempl::Find<std::remove_cvref_t<T>>(kTypeList);
  };

  template <std::size_t N>
  static consteval auto GetIndexByName(const utempl::ConstexprString<N>& name) -> std::size_t {
    return utempl::Find(kNames, name);
  };

  template <template <typename...> typename F>
  static consteval auto GetFirstIndex() -> std::size_t {
    constexpr auto arr = std::array<std::size_t, sizeof...(Ts)>({F<typename Ts::Type>::value...});
    return std::ranges::find(arr, true) - arr.begin();
  };

  template <template <typename...> typename F>
  static consteval auto GetAllIndexes() {
    return [&](auto... is) {
      return utempl::TupleCat(std::array<std::size_t, 0>{}, [&] {
        if constexpr(F<typename Ts::Type>::value) {
          return std::to_array<std::size_t>({is});
        } else {
          return std::array<std::size_t, 0>{};
        };
      }()...);
    } | utempl::kSeq<sizeof...(Ts)>;
  };

  template <std::size_t I>
  static consteval auto GetName() {
    return Get<I>(kNames);
  };
};

}  // namespace impl

#define COMPONENT_REQUIRES(name, impl) /* NOLINT */ \
  template <typename T>                             \
  concept name = impl;                              \
  template <typename T>                             \
  struct name##M {                                  \
    static constexpr auto value = name<T>;          \
  }

template <ConstexprConfig config, utempl::Tuple DependencyGraph, typename... Ts>
struct ServiceContext {
  static constexpr auto kConfig = config;
  static constexpr auto kThreadsCount = config.template Get<"threads">();
  static constexpr impl::DependenciesUtils<Ts...> kUtils;
  engine::basic::TaskProcessor<kThreadsCount - 1> taskProcessor;
  utempl::Tuple<std::optional<typename Ts::Type>...> storage{};

  constexpr ServiceContext() : taskProcessor{utempl::Wrapper<utempl::ConstexprString{kBasicTaskProcessorName}>{}, *this} {};
  constexpr auto Run(int argc, const char** argv) {
    impl::InitComponents<DependencyGraph>(*this, argc, argv);
    this->taskProcessor.Run();
  };

  template <typename Component>
  constexpr auto GetContextFor() /* Internal Use Only */ {
    return impl::ServiceContextForComponent<Component, std::decay_t<decltype(*this)>>{*this};
  };

 private:
  static constexpr struct {
    constexpr auto operator=(auto&&) const noexcept {};
  } ComponentType{};
  static constexpr struct {
    constexpr auto operator=(auto&&) const noexcept {};
  } ComponentName{};
  static constexpr struct {
    constexpr auto operator=(auto&&) const noexcept {};
  } ComponentOptions{};
  static constexpr struct {
    constexpr auto operator=(auto&&) const noexcept {};
  } RequiredComponent;

 public:
  template <typename Current, std::size_t I>
  constexpr auto FindComponent() -> decltype(*Get<I>(this->storage))& {
    constexpr auto Index = kUtils.GetIndexByName(Current::kName);
    constexpr auto J = utempl::loopholes::Counter<Current, utempl::Wrapper<I>>();
    constexpr auto dependencies = Get<Index>(DependencyGraph);

    static_assert((ComponentType = utempl::kType<typename Current::Type>,
                   ComponentName = utempl::kWrapper<Current::kName>,
                   ComponentOptions = utempl::kWrapper<Current::kOptions>,
                   RequiredComponent = utempl::kType<decltype(Get<I>(kUtils.kComponentConfigs))>,
                   dependencies.size() > J),
                  "Constructor is not declared as constexpr");

    return *Get<I>(this->storage);
  };
};

namespace impl {

template <typename Current, std::size_t I>
struct DependencyInfoKey {};

template <typename T, auto... Args>
inline constexpr auto Use() {
  std::ignore = T{Args...};
};

template <typename...>
consteval auto Ignore() {};

template <typename Current, utempl::ConstexprString Name, ConstexprConfig Config, typename... Ts>
struct DependencyInfoInjector {
  int argc{};
  const char** argv{};
  static constexpr ConstexprConfig kConfig = Config;
  static constexpr DependenciesUtils<Ts...> kUtils;
  static constexpr utempl::ConstexprString kName = Name;
  template <utempl::ConstexprString name>
  static consteval auto FindComponentType() {
    if constexpr(name == kBasicTaskProcessorName) {
      return [] -> engine::basic::TaskProcessor<Config.template Get<"threads">() - 1> {
        std::unreachable();
      }
      ();
    } else {
      return [] -> decltype(Get<kUtils.GetIndexByName(name)>(kUtils.kTypeList)) {
        std::unreachable();
      }();
    };
  };

  template <utempl::ConstexprString name,
            typename...,
            std::size_t I = utempl::loopholes::Counter<Current, utempl::Wrapper<name>>(),
            auto = utempl::loopholes::Injector<DependencyInfoKey<Current, I>{}, name>{}>
  static auto FindComponent() -> decltype(FindComponentType<name>())&;

  template <typename T, typename..., typename R = decltype(FindComponent<Get<kUtils.template GetIndexByType<T>()>(kUtils.kNames)>())>
  static auto FindComponent() -> R;

  template <template <typename...> typename F,
            typename...,
            auto Arr = std::to_array<bool>({F<typename Ts::Type>::value...}),
            std::size_t I = std::ranges::find(Arr, true) - Arr.begin(),
            typename R = decltype(FindComponent<decltype(Get<I>(utempl::TypeList<Ts...>{}))::kName>())>
  static auto FindComponent() -> R;

 private:
  template <std::size_t... Ids>
  static auto FindAllComponentsImpl() -> utempl::Tuple<decltype(FindComponent<decltype(Get<Ids>(utempl::TypeList<Ts...>{}))::kName>())...> {
  };

 public:
  template <template <typename...> typename F,
            typename...,
            typename R = decltype(utempl::Unpack(utempl::PackConstexprWrapper<kUtils.template GetAllIndexes<F>(), utempl::Tuple<>>(),
                                                 [](auto... is) -> decltype(FindAllComponentsImpl<is...>()) {
                                                   std::unreachable();
                                                 }))>
  static auto FindAllComponents() -> R;

  template <auto = 0>
  static consteval auto GetDependencies() {
    return [](auto... is) {
      return ([&] {
        constexpr auto response = Magic(utempl::loopholes::Getter<DependencyInfoKey<Current, is>{}>{});
        if constexpr(response == kBasicTaskProcessorName) {
          return utempl::Tuple{};
        } else {
          return utempl::Tuple{response};
        };
      }() + ... +
              utempl::Tuple{});
    } | utempl::kSeq<utempl::loopholes::Counter<Current>()>;
  };
  static inline consteval auto Inject() {
    Ignore<decltype(Use<Current, DependencyInfoInjector<Current, Name, Config, Ts...>{}>())>();
  };
};

}  // namespace impl

template <utempl::ConstexprString Name, utempl::Tuple Dependencies>
struct DependencyGraphElement {};

template <typename... Ts>
struct DependencyGraph {
  static constexpr auto kValue = utempl::TypeList<Ts...>{};
};

namespace impl {

template <typename T, std::size_t Size>
struct CompileTimeStack {
  std::array<std::optional<T>, Size> data{};
  std::size_t current{};
  inline constexpr auto Push(T value) {
    if(this->current == Size) {
      throw std::out_of_range{nullptr};
    };
    this->data[current].emplace(std::move(value));
    this->current++;
  };
  inline constexpr auto Pop() {
    auto tmp = this->current;
    this->current--;
    return std::move(*this->data[tmp]);
  };
};
}  // namespace impl

template <utempl::ConstexprString... Names, utempl::Tuple... Dependencies>
consteval auto TopologicalSort(const DependencyGraph<DependencyGraphElement<Names, Dependencies>...>&) {
  constexpr utempl::Tuple names = {Names...};
  constexpr utempl::Tuple storage = Map(utempl::Tuple{Dependencies...}, [&]<utempl::TupleLike Tuple>(Tuple&& tuple) {
    static constexpr auto Size = utempl::kTupleSize<Tuple>;
    return utempl::Unpack(std::forward<Tuple>(tuple), [&]<typename... Args>(Args&&... args) -> std::array<std::size_t, Size> {
      return {Find(names, std::forward<Args>(args))...};
    });
  });
  constexpr auto Size = utempl::kTupleSize<decltype(storage)>;
  const std::array adj = utempl::Map(
      storage,
      [](std::ranges::range auto& arg) {
        return std::ranges::subrange(arg.data(), arg.data() + arg.size());
      },
      utempl::kType<std::array<std::size_t, 0>>);
  std::array<bool, Size> visited{};
  constexpr auto ResultSize = sizeof...(Dependencies);
  impl::CompileTimeStack<std::size_t, ResultSize> response{};
  for(std::size_t i = 0; i < Size; i++) {
    if(visited[i] == true) {
      continue;
    };
    [&](this auto&& self, std::size_t v) -> void {
      visited[v] = true;
      for(const auto& element : adj[v]) {
        if(!visited[element]) {
          self(element);
        };
      };
      response.Push(v);
    }(i);
  };
  return utempl::Map(std::move(response.data), [](auto&& data) {
    return *data;
  });
};

template <ConstexprConfig config = {}, typename... ComponentConfigs>
struct ServiceContextBuilder {
  static constexpr auto kList = utempl::kTypeList<ComponentConfigs...>;
  static constexpr impl::DependenciesUtils<ComponentConfigs...> kUtils;
  static constexpr auto kConfig = config;
  template <typename T, utempl::ConstexprString name = T::kName, typename... Os>
  static consteval auto Append(Options<Os...> = {})
      -> decltype(T::template Adder<name, Options<Os...>{}>(
          ServiceContextBuilder<config, ComponentConfigs..., ComponentConfig<name, T, Options<Os...>{}>>{})) {
    return {};
  };

  template <typename T, utempl::ConstexprString name = T::kName, typename... Os>
  static consteval auto Append(Options<Os...> = {})
      -> ServiceContextBuilder<config, ComponentConfigs..., ComponentConfig<name, T, Options<Os...>{}>>
    requires(!requires(ServiceContextBuilder<config, ComponentConfigs..., ComponentConfig<name, T, Options<Os...>{}>> builder) {
      T::template Adder<name, Options<Os...>{}>(builder);
    })
  {
    return {};
  };

  template <utempl::ConstexprString Key, auto Value>
  static consteval auto AppendConfigParam() -> ServiceContextBuilder<config.template Append<Key>(Value), ComponentConfigs...> {
    return {};
  };

  template <typename F>
  static consteval auto TransformComponents(F&& f)
      -> ServiceContextBuilder<config, decltype(impl::TransformIfOk(ComponentConfigs{}, f))...> {
    return {};
  };
  template <utempl::ConstexprString name>
  static consteval auto FindComponent() {
    if constexpr(name == kBasicTaskProcessorName) {
      return [] -> engine::basic::TaskProcessor<config.template Get<"threads">() - 1> {
        std::unreachable();
      }
      ();
    } else {
      return []<typename... TTs, utempl::ConstexprString... names, Options... Options>(
                 const ServiceContextBuilder<config, ComponentConfig<names, TTs, Options>...>&)
                 -> decltype(utempl::Get<Find(utempl::Tuple{names...}, name)>(utempl::TypeList<TTs...>{})) {
        std::unreachable();
      }(ServiceContextBuilder<config, ComponentConfigs...>{});
    };
  };

  template <template <typename...> typename F>
  static consteval auto FindComponent() {
    return Get<kUtils.template GetFirstIndex<F>()>(kUtils.kTypeList);
  };

  template <template <typename...> typename F>
  static consteval auto FindAllComponents() {
    return utempl::Unpack(utempl::PackConstexprWrapper<kUtils.template GetAllIndexes<F>(), utempl::Tuple<>>(),
                          [](auto... is) -> utempl::Tuple<typename decltype(utempl::Get<is>(utempl::kTypeList<ComponentConfigs...>))::Type...> {
                            std::unreachable();
                          });
  };

  static consteval auto Config() {
    return config;
  };

  static consteval auto GetDependencyGraph() {
    return [&]<utempl::ConstexprString... names, typename... Components, Options... Options>(
               ComponentConfig<names, Components, Options>...) {
      return DependencyGraph<DependencyGraphElement<names,
                                                    []<utempl::ConstexprString name, typename Component, ::cserver::Options OOptions>(
                                                        ComponentConfig<name, Component, OOptions>) {
                                                      impl::DependencyInfoInjector<Component, name, config, ComponentConfigs...> injector;
                                                      injector.Inject();
                                                      return injector.GetDependencies();
                                                    }(ComponentConfigs{})>...>{};
    }(ComponentConfigs{}...);
  };

  static consteval auto GetIndexDependencyGraph() {
    return []<utempl::ConstexprString... Names, utempl::Tuple... Dependencies>(
               DependencyGraph<DependencyGraphElement<Names, Dependencies>...>) {
      return utempl::Tuple{Unpack(
          Dependencies, [](auto... dependencies) -> std::array<std::size_t, sizeof...(dependencies)> {
            return {Find(utempl::Tuple{Names...}, dependencies)...};
          })...};
    }(GetDependencyGraph());
  };

  static consteval auto Sort() {
    constexpr auto sorted = TopologicalSort(GetDependencyGraph());
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      constexpr auto list = utempl::TypeList<ComponentConfigs...>{};
      return ServiceContextBuilder<config, decltype(Get<sorted[Is]>(list))...>{};
    }(std::index_sequence_for<ComponentConfigs...>{});
  };

  static constexpr auto GetServiceContext() {
    return ServiceContext<config, GetIndexDependencyGraph(), ComponentConfigs...>{};
  };

  static constexpr auto Run(int argc = 0, const char** argv = nullptr) -> void {
    static auto context = GetServiceContext();
    context.Run(argc, argv);
    context.taskProcessor.ioContext.run();
  };
};
}  // namespace cserver
