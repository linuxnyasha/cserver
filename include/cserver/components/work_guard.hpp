#pragma once
#include <cserver/engine/components.hpp>

namespace cserver {

struct StopBlocker {
  boost::asio::io_service::work work;
  inline constexpr StopBlocker(auto, auto& context) : 
      work(context.template FindComponent<kBasicTaskProcessorName>().ioContext) {};
};

} // namespace cserver
