#pragma once
#include <cserver/engine/components.hpp>
#include <cserver/engine/coroutine.hpp>
#include <boost/asio.hpp>

namespace cserver::engine::basic {

template <std::size_t Size = 0>
struct TaskProcessor {
  boost::asio::io_context ioContext;
  std::array<std::jthread, Size> pool;
  static constexpr utempl::ConstexprString kName = "basicTaskProcessor";
  inline constexpr TaskProcessor(auto, auto&) :
      ioContext{},
      pool{} {
  };

  template <utempl::ConstexprString name, Options Options, typename T>
  static consteval auto Adder(const T& context) { 
    constexpr std::size_t Count = T::kConfig.template Get<name>().template Get<"threadPoolSize">();
    return context.TransformComponents(
      [&](const ComponentConfig<name, TaskProcessor<>, Options>&) -> ComponentConfig<name, TaskProcessor<Count>, Options> {
        return {};
      });
  };

  inline auto Run() {
    for(auto& thread : this->pool) {
      thread = std::jthread([&]{
        boost::asio::executor_work_guard<decltype(this->ioContext.get_executor())> guard{this->ioContext.get_executor()};
        this->ioContext.run();
      });
    };
  };
};

}
