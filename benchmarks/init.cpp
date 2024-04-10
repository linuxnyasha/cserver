#include <benchmark/benchmark.h>
#include <cserver/engine/components.hpp>

struct SomeComponent1 {
  static constexpr utempl::ConstexprString kName = "1";
  constexpr SomeComponent1(auto, auto& context) {
    context.template FindComponent<cserver::kBasicTaskProcessorName>();
  };
};


struct SomeComponent2 {
  static constexpr utempl::ConstexprString kName = "2";
  constexpr SomeComponent2(auto, auto& context) {
    context.template FindComponent<cserver::kBasicTaskProcessorName>();
  };
};


struct SomeComponent3 {
  static constexpr utempl::ConstexprString kName = "3";
  constexpr SomeComponent3(auto, auto& context) {
    context.template FindComponent<cserver::kBasicTaskProcessorName>();
  };
};


struct SomeComponent4 {
  static constexpr utempl::ConstexprString kName = "4";
  constexpr SomeComponent4(auto, auto& context) {
    context.template FindComponent<cserver::kBasicTaskProcessorName>();
  };
};


struct SomeComponent5 {
  static constexpr utempl::ConstexprString kName = "5";
  constexpr SomeComponent5(auto, auto& context) {
    context.template FindComponent<cserver::kBasicTaskProcessorName>();
  };
};


struct SomeComponent6 {
  SomeComponent1& first;
  SomeComponent2& second;
  static constexpr utempl::ConstexprString kName = "6";
  constexpr SomeComponent6(auto, auto& context) : 
      first(context.template FindComponent<"1">()),
      second(context.template FindComponent<"2">()) {};
};

struct SomeComponent7 {
  SomeComponent4& first;
  SomeComponent6& second;
  static constexpr utempl::ConstexprString kName = "6";
  constexpr SomeComponent7(auto, auto& context) : 
      first(context.template FindComponent<"4">()),
      second(context.template FindComponent<"6">()) {};
};


struct SomeComponent8 {
  SomeComponent3& first;
  SomeComponent5& second;
  static constexpr utempl::ConstexprString kName = "6";
  constexpr SomeComponent8(auto, auto& context) : 
      first(context.template FindComponent<"3">()),
      second(context.template FindComponent<"5">()) {};
};


auto BMInitialise(benchmark::State& state) {
  using Builder = decltype(cserver::ServiceContextBuilder{}
      .AppendConfigParam<"threads", 8>()
      .Append<SomeComponent1>()
      .Append<SomeComponent2>()
      .Append<SomeComponent3>()
      .Append<SomeComponent4>()
      .Append<SomeComponent5>()
      .Append<SomeComponent6>()
      .Append<SomeComponent7>()
      .Append<SomeComponent8>()
      .Sort());
  using ServiceContextType = decltype(Builder::GetServiceContext());
  alignas(ServiceContextType) std::byte context[sizeof(ServiceContextType)];
  for(auto _  : state) {
    auto* ptr = new (context) ServiceContextType{};
    ptr->Run();
    ptr->FindComponent<cserver::kBasicTaskProcessorName>().ioContext.run();
    state.PauseTiming();
    ptr->~ServiceContextType();
    state.ResumeTiming();
  };
};
auto BMInitialiseWithoutSort(benchmark::State& state) {
  using Builder = decltype(cserver::ServiceContextBuilder{}
      .AppendConfigParam<"threads", 8>()
      .Append<SomeComponent1>()
      .Append<SomeComponent2>()
      .Append<SomeComponent3>()
      .Append<SomeComponent4>()
      .Append<SomeComponent5>()
      .Append<SomeComponent6>()
      .Append<SomeComponent7>()
      .Append<SomeComponent8>());
  using ServiceContextType = decltype(Builder::GetServiceContext());
  alignas(ServiceContextType) std::byte context[sizeof(ServiceContextType)];
  for(auto _  : state) {
    auto* ptr = new (context) ServiceContextType{};
    ptr->Run();
    ptr->FindComponent<cserver::kBasicTaskProcessorName>().ioContext.run();
    state.PauseTiming();
    ptr->~ServiceContextType();
    state.ResumeTiming();
  };
};


BENCHMARK(BMInitialise);
BENCHMARK(BMInitialiseWithoutSort);

BENCHMARK_MAIN();
