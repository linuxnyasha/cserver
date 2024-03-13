#pragma once
#include <boost/system/error_code.hpp>

namespace cserver {

struct BoostErrorWrapper : std::exception, boost::system::error_code {
  std::string errorMessage;
  BoostErrorWrapper(boost::system::error_code ec) : errorMessage(ec.what()) {};
  auto what() const noexcept -> const char* override {
    return this->errorMessage.c_str();
  };
};

} // namespace cserver
