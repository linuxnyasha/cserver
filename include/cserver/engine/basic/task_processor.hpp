#pragma once
#include <utempl/constexpr_string.hpp>
#include <boost/asio.hpp>

namespace cserver::engine::basic {

template <std::size_t Size = 0>
struct TaskProcessor {
  boost::asio::io_context ioContext;
  std::array<std::thread, Size> pool;
  static constexpr utempl::ConstexprString kName = "basicTaskProcessor";
  inline constexpr TaskProcessor(auto, auto&) :
      ioContext{},
      pool{} {
  };

  inline constexpr ~TaskProcessor() {
    this->ioContext.stop();
    for(auto& thread : this->pool) {
      thread.join();
    };
  };

  inline auto Run() {
    for(auto& thread : this->pool) {
      thread = std::thread([&]{
        boost::asio::executor_work_guard<decltype(this->ioContext.get_executor())> guard{this->ioContext.get_executor()};
        this->ioContext.run();
      });
    };
  };
};

}
