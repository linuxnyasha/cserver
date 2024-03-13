#pragma once
#include <boost/asio.hpp>

namespace cserver {

template <typename T>
using Task = boost::asio::awaitable<T>;

} // namespace cserver
