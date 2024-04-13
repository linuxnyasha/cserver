#pragma once
#include <cserver/engine/components.hpp>

namespace cserver {


struct StopBlocker {
  boost::asio::io_context::work guard;
  inline constexpr StopBlocker(auto, auto& context) : 
      guard(context.template FindComponent<kBasicTaskProcessorName>().ioContext) {};
};

} // namespace cserver
