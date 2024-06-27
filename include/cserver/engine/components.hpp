#pragma once
#include <cserver/engine/basic/task_processor.hpp>
#include <cserver/engine/coroutine.hpp>
#include <utempl/utils.hpp>
#include <utempl/loopholes/counter.hpp>

namespace cserver {

inline constexpr utempl::ConstexprString kBasicTaskProcessorName = "basicTaskProcessor";

struct SeparatedComponent {};
template <typename... Ts>
struct Options : utempl::TypeList<Ts...> {};

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

namespace impl {

template <typename InitFlag, std::size_t>
inline constexpr auto GetInitFlagFor(auto&... args) -> InitFlag& {
  static InitFlag flag{args...};
  return flag;
};

template <typename InitFlag, utempl::Tuple Tuple>
constexpr auto TransformDependencyGraphToTupleWithDependenciesToInitedFlagChanges(auto&&... args) {
  return utempl::Map(utempl::PackConstexprWrapper<Tuple>(), [&]<auto Array>(utempl::Wrapper<Array>) {
    return utempl::Map(utempl::PackConstexprWrapper<Array, utempl::Tuple<>>(), [&](auto elem) {
      return &GetInitFlagFor<InitFlag, static_cast<std::size_t>(elem)>(args...);
    }, utempl::kType<std::array<InitFlag*, Array.size()>>);
  });
};

struct AsyncConditionVariable {
  inline AsyncConditionVariable(boost::asio::io_context& ioContext) :
    mtx{},
    deadlineTimer(ioContext),
      flag{} {};
  template <typename Token = boost::asio::use_awaitable_t<>>
  inline auto AsyncWait(Token&& token = {}) -> Task<> {
    return boost::asio::async_initiate<
        Token,
        void(void)>([this]<typename Handler>(Handler&& handler) -> void {
      std::unique_lock lc(this->mtx);
      if(this->flag) {
        return handler();
      };
      this->deadlineTimer.async_wait([h = std::make_unique<Handler>(std::forward<Handler>(handler))](const boost::system::error_code&){
        return (*h)();
      });
    }, token);
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
  bool flag;
};

template <utempl::Tuple DependencyGraph, typename T>
inline constexpr auto InitComponents(T& ccontext) -> void {
  static auto& context = ccontext;
  static auto& taskProcessor = context.template FindComponent<kBasicTaskProcessorName>();
  static auto& ioContext = taskProcessor.ioContext;
  static utempl::Tuple inited = TransformDependencyGraphToTupleWithDependenciesToInitedFlagChanges<AsyncConditionVariable, DependencyGraph>(ioContext); 
  auto work = make_work_guard(ioContext);
  []<std::size_t... Is>(std::index_sequence<Is...>){
    (boost::asio::co_spawn(ioContext, []() -> cserver::Task<> {
      auto& dependencies = Get<Is>(inited);
      for(auto* flag : dependencies) {
        co_await flag->AsyncWait();
      };
      Get<Is>(context.storage).emplace(utempl::Wrapper<Get<Is>(T::kNames)>{}, context);
      auto& componentInitFlag = GetInitFlagFor<AsyncConditionVariable, Is>(ioContext);
      componentInitFlag.NotifyAll();
      if constexpr(requires{Get<Is>(context.storage)->Run();}) {
        Get<Is>(context.storage)->Run();
      };
    }(), boost::asio::detached), ...);
  }(std::make_index_sequence<utempl::kTupleSize<decltype(DependencyGraph)>>());
};

} // namespace impl

template <ConstexprConfig config, utempl::Tuple DependencyGraph, auto names, auto Options, typename... Ts>
struct ServiceContext {
  static constexpr auto kNames = names;
  static constexpr auto kConfig = config;
  static constexpr auto kList = utempl::TypeList<Ts...>{};
  static constexpr auto kThreadsCount = config.template Get<"threads">();
  engine::basic::TaskProcessor<kThreadsCount - 1> taskProcessor;
  utempl::Tuple<std::optional<Ts>...> storage;
  inline constexpr ServiceContext() :
      taskProcessor{utempl::Wrapper<utempl::ConstexprString{kBasicTaskProcessorName}>{}, *this}
      ,storage{} {
  };
  inline constexpr auto Run() {
    impl::InitComponents<DependencyGraph>(*this);
    this->taskProcessor.Run();
  };
  template <utempl::ConstexprString name>
  inline constexpr auto FindComponent() -> auto& {
    constexpr auto I = utempl::Find(names, name);
    return *Get<I>(this->storage);
  };
  template <>
  inline constexpr auto FindComponent<kBasicTaskProcessorName>() -> auto& {
    return this->taskProcessor;
  };
  template <typename T>
  inline constexpr auto FindComponent() -> T& {
    constexpr auto I = utempl::Find<std::remove_cvref_t<T>>(utempl::kTypeList<Ts...>);
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

template <typename Current, ConstexprConfig config, typename... Ts>
struct DependencyInfoInjector {
  static constexpr ConstexprConfig kConfig = config;
private:
  template <utempl::ConstexprString name, utempl::ConstexprString... names, typename... TTs, Options... Options>
  static consteval auto FindComponentTypeImpl(ComponentConfig<names, TTs, Options>...) {
    if constexpr(static_cast<std::string_view>(name) == kBasicTaskProcessorName) {
      return [&] -> engine::basic::TaskProcessor<config.template Get<"threads">() - 1> {
        std::unreachable();
      }();
    } else {
      constexpr auto I = utempl::Find(utempl::Tuple{std::string_view{names}...}, std::string_view{name});
      return [&] -> decltype(utempl::Get<I>(utempl::TypeList<TTs...>{})) {
        std::unreachable();
      }();
    };
  };
public:
  template <utempl::ConstexprString name>
  using FindComponentType = decltype(FindComponentTypeImpl<name>(Ts{}...));

  template <typename T>
  static constexpr auto FindComponentName =
    []<utempl::ConstexprString... names, typename... Tss, Options... Options>
    (ComponentConfig<names, Tss, Options>...) {
      return Get<utempl::Find<T>(utempl::TypeList<Tss...>{})>(utempl::Tuple{names...});
    }(Ts{}...);

  template <
    utempl::ConstexprString name,
    typename...,
    std::size_t I = utempl::loopholes::Counter<Current, utempl::Wrapper<name>>(),
    auto = utempl::loopholes::Injector<
      DependencyInfoKey<
        Current,
        I
      >{},
      name
    >{}
  >
  static auto FindComponent() -> FindComponentType<name>&;


  template <
    typename T,
    typename...,
    std::size_t I = utempl::loopholes::Counter<Current, utempl::Wrapper<FindComponentName<T>>>(),
    auto = utempl::loopholes::Injector<
      DependencyInfoKey<
        Current,
        I
      >{},
      FindComponentName<T>
    >{}
  >
  static auto FindComponent() -> T&;
  
  template <auto = 0>
  static consteval auto GetDependencies() {
    return [](auto... is) {
      return ([&]{
        constexpr auto response = Magic(utempl::loopholes::Getter<DependencyInfoKey<Current, is>{}>{});
        if constexpr(response == kBasicTaskProcessorName) {
          return utempl::Tuple{};
        } else {
          return utempl::Tuple{response};
        };
      }() + ... + utempl::Tuple{});
    } | utempl::kSeq<utempl::loopholes::Counter<Current>()>;
  };
  template <utempl::ConstexprString name>
  static inline consteval auto Inject() {
    Ignore<decltype(Use<Current, utempl::Wrapper<name>{}, DependencyInfoInjector<Current, config, Ts...>{}>())>();
  };
};

} // namespace impl

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
      throw std::out_of_range{0};
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

template <typename T>
struct RangeView {
  T* beginIterator;
  T* endIterator;
  template <typename Self>
  inline constexpr auto begin(this Self&& self) {
    return std::forward<Self>(self).beginIterator;
  };
  template <typename Self>
  inline constexpr auto end(this Self&& self) {
    return std::forward<Self>(self).endIterator;
  };
  inline constexpr RangeView(std::ranges::range auto&& range) :
    beginIterator(std::ranges::begin(range)),
    endIterator(std::ranges::end(range)) {};
};
RangeView(std::ranges::range auto&& range) -> RangeView<std::remove_reference_t<decltype(*std::ranges::begin(range))>>;

} // namespace impl

template <utempl::ConstexprString... Names, utempl::Tuple... Dependencies>
consteval auto TopologicalSort(const DependencyGraph<DependencyGraphElement<Names, Dependencies>...>&) {
  constexpr utempl::Tuple names = {Names...};
  constexpr utempl::Tuple storage = Map(utempl::Tuple{Dependencies...}, 
                                          [&]<utempl::TupleLike Tuple>(Tuple&& tuple){
                                            static constexpr auto Size = utempl::kTupleSize<Tuple>;
                                            return utempl::Unpack(std::forward<Tuple>(tuple),
                                              [&]<typename... Args>(Args&&... args) -> std::array<std::size_t, Size> {
                                                return {Find(names, std::forward<Args>(args))...};
                                              }
                                            );
                                          });
  constexpr auto Size = utempl::kTupleSize<decltype(storage)>;
  const std::array adj = utempl::Map(storage, 
                                    [](std::ranges::range auto& arg){
                                      return impl::RangeView{arg};
                                    }, utempl::kType<std::array<std::size_t, 0>>);
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
  return utempl::Map(std::move(response.data), [](auto&& data){
    return *data;
  });
};


template <ConstexprConfig config = {}, typename... ComponentConfigs>
struct ServiceContextBuilder {
  static constexpr auto kList = utempl::kTypeList<ComponentConfigs...>;
  static constexpr auto kConfig = config;
  template <typename T, utempl::ConstexprString name = T::kName, typename... Os>
  static consteval auto Append(Options<Os...> = {}) -> decltype(T::template Adder<name, Options<Os...>{}>(ServiceContextBuilder<config, ComponentConfigs..., ComponentConfig<name, T, Options<Os...>{}>>{})) {
    return {};
  };

  template <typename T, utempl::ConstexprString name = T::kName, typename... Os>
  static consteval auto Append(Options<Os...> = {}) -> ServiceContextBuilder<config, ComponentConfigs..., ComponentConfig<name, T, Options<Os...>{}>> 
        requires (!requires(ServiceContextBuilder<config, ComponentConfigs..., ComponentConfig<name, T, Options<Os...>{}>> builder) {T::template Adder<name, Options<Os...>{}>(builder);}) {
    return {};
  };


  template <utempl::ConstexprString Key, auto Value>
  static consteval auto AppendConfigParam() -> ServiceContextBuilder<config.template Append<Key>(Value), ComponentConfigs...> {
    return {};
  };

  template <typename F>
  static consteval auto TransformComponents(F&& f) -> ServiceContextBuilder<config, decltype(impl::TransformIfOk(ComponentConfigs{}, f))...> {
    return {};
  };
  template <utempl::ConstexprString name>
  static consteval auto FindComponent() {
    if constexpr(name == kBasicTaskProcessorName) {
      return [] -> engine::basic::TaskProcessor<config.template Get<"threads">() - 1> {}();
    } else {
      return []<typename... TTs, utempl::ConstexprString... names, Options... Options>
      (const ServiceContextBuilder<config, ComponentConfig<names, TTs, Options>...>&) 
          -> decltype(utempl::Get<Find(utempl::Tuple{names...}, name)>(utempl::TypeList<TTs...>{})) {
        std::unreachable();
      }(ServiceContextBuilder<config, ComponentConfigs...>{});
    };
  };
  static consteval auto Config() {
    return config;
  };

  static consteval auto GetDependencyGraph() {
    return [&]<utempl::ConstexprString... names, typename... Components, Options... Options>
    (ComponentConfig<names, Components, Options>...){
      return DependencyGraph<DependencyGraphElement<names, 
        []<utempl::ConstexprString name, typename Component, ::cserver::Options OOptions>
        (ComponentConfig<name, Component, OOptions>) {
          impl::DependencyInfoInjector<Component, config, ComponentConfigs...> injector;
          injector.template Inject<name>();
          return injector.GetDependencies();
        }(ComponentConfigs{})>...>{};
    }(ComponentConfigs{}...);
  };

  static consteval auto GetIndexDependencyGraph() {
    return []<utempl::ConstexprString... Names, utempl::Tuple... Dependencies>
    (DependencyGraph<DependencyGraphElement<Names, Dependencies>...>){
      return utempl::Tuple{Unpack(Dependencies, [](auto... dependencies) -> std::array<std::size_t, sizeof...(dependencies)> {
        return {Find(utempl::Tuple{Names...}, dependencies)...};
      })...};
    }(GetDependencyGraph());
  };

  static consteval auto Sort() {
    constexpr auto sorted = TopologicalSort(GetDependencyGraph());
    return [&]<std::size_t... Is>(std::index_sequence<Is...>){
      constexpr auto list = utempl::TypeList<ComponentConfigs...>{};
      return ServiceContextBuilder<config, decltype(Get<sorted[Is]>(list))...>{};
    }(std::index_sequence_for<ComponentConfigs...>{});
  };

  static constexpr auto GetServiceContext() {
    return []<utempl::ConstexprString... names, typename... TTs, Options... Options>
    (utempl::TypeList<ComponentConfig<names, TTs, Options>...>) {
      return ServiceContext<config, GetIndexDependencyGraph(), utempl::Tuple{names...}, utempl::Tuple{Options...}, TTs...>{};
    }(utempl::TypeList<ComponentConfigs...>{});
  };

  static constexpr auto Run() -> void {
    static auto context = GetServiceContext();
    context.Run();
    context.template FindComponent<kBasicTaskProcessorName>().ioContext.run();
  };
};
} // namespace cserver
