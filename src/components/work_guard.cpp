module;
#include <boost/asio.hpp>
export module cserver.components.work_guard;
import cserver.engine.components;

namespace cserver {

export struct StopBlocker {
  boost::asio::io_context::work guard;
  explicit constexpr StopBlocker(auto& context) : guard(context.template FindComponent<kBasicTaskProcessorName>().ioContext) {};
};

}  // namespace cserver
