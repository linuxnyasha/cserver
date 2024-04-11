#pragma once
#include <utempl/constexpr_string.hpp>
#include <boost/asio.hpp>
#include <thread>

namespace cserver::engine::basic {

template <std::size_t Size = 0>
struct TaskProcessor {
  boost::asio::io_context ioContext;
  std::array<std::optional<std::thread>, Size> pool;
  static constexpr utempl::ConstexprString kName = "basicTaskProcessor";
  inline constexpr TaskProcessor(auto, auto&) :
      ioContext{},
      pool{} {
  };

  inline constexpr ~TaskProcessor() {
    for(auto& thread : this->pool) {
      if(thread && thread->joinable()) {
        thread->join();
      };
    };
  };

  inline auto Run() {
    for(auto& thread : this->pool) {
      if(this->ioContext.stopped()) {
        return;
      };
      thread = std::thread([&]{
        if(this->ioContext.stopped()) {
          return;
        };
        this->ioContext.run();
      });
    };
  };
};

}
