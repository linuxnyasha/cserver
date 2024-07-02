#pragma once

#include <stdexcept>
#include <fmt/format.h>
#include <fmt/compile.h>

namespace cserver::engine {

struct NotImplemented : std::runtime_error {
  NotImplemented(std::string context) : 
      std::runtime_error{fmt::format(FMT_COMPILE("Not Implemented Error [{}]"), context)} {};
};

} // namespace cserver::engine
