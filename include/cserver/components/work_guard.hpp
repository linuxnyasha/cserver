#pragma once
#include <cserver/engine/components.hpp>

namespace cserver {

struct StopBlocker {
  boost::asio::io_context::work guard;
  explicit constexpr StopBlocker(auto& context) : guard(context.template FindComponent<kBasicTaskProcessorName>().ioContext) {};
};

}  // namespace cserver
