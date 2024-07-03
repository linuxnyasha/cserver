#pragma once
#include <cserver/engine/coroutine.hpp>
namespace cserver::network {

class DeadLineTimer;

class TcpSocket;
class TcpResolver;

class IoContext {
  boost::asio::io_context impl;

 public:
  inline IoContext() : impl() {};
  class Work {
    boost::asio::io_context::work impl;

   public:
    explicit inline Work(IoContext& ioContext) : impl(ioContext.impl) {};
    inline Work(Work&&) = default;
    inline Work(const Work&) = default;
  };

  friend DeadLineTimer;
  friend TcpSocket;

  friend TcpResolver;
};

class DeadLineTimer {
  boost::asio::deadline_timer impl;

 public:
  explicit inline DeadLineTimer(IoContext& ioContext) : impl(ioContext.impl) {};
  inline auto AsyncWait() -> Task<> {
    return this->impl.async_wait(boost::asio::use_awaitable);
  };
  inline constexpr auto Cancel() -> void {
    this->impl.cancel();
  };
  inline constexpr auto CancelOne() -> void {
    this->impl.cancel_one();
  };
};

class TcpSocket {
  boost::asio::ip::tcp::socket impl;

 public:
  explicit inline TcpSocket(IoContext& ioContext) : impl(ioContext.impl) {};
  inline TcpSocket(TcpSocket&& other) : impl(std::move(other.impl)) {};

  template <typename Buffer, typename CompletionCondition>
  friend inline constexpr auto AsyncWrite(TcpSocket& socket, Buffer buffer, CompletionCondition completion);
  template <typename Buffer>
  friend inline constexpr auto AsyncWrite(TcpSocket& socket, Buffer buffer);

  template <typename Buffer, typename CompletionCondition>
  friend inline constexpr auto AsyncRead(TcpSocket& socket, Buffer buffer, CompletionCondition completion);
  template <typename Buffer>
  friend inline constexpr auto AsyncRead(TcpSocket& socket, Buffer buffer);

  template <typename Buffer, typename Match>
  friend inline constexpr auto AsyncReadUntil(TcpSocket& socket, Buffer buffer, Match match);
};
class TcpEntry;

class TcpEndpoint {
  boost::asio::ip::tcp::endpoint impl;
  explicit inline TcpEndpoint(boost::asio::ip::tcp::endpoint impl) : impl(std::move(impl)) {};

 public:
  friend TcpEntry;
};

class TcpEntry {
  boost::asio::ip::tcp::resolver::results_type::value_type impl;

 public:
  inline TcpEntry() = default;
  inline TcpEntry(const TcpEndpoint& ep, std::string_view host, std::string_view service) : impl(ep.impl, host, service) {};
  inline auto Endpoint() -> TcpEndpoint {
    return TcpEndpoint{this->impl.endpoint()};
  };
  inline auto HostName() -> std::string {
    return this->impl.host_name();
  };
  inline constexpr auto ServiceName() -> std::string {
    return this->impl.service_name();
  };
  explicit inline operator TcpEndpoint() {
    return TcpEndpoint{this->impl};
  };
};

class TcpResolver {
  boost::asio::ip::tcp::resolver impl;

 public:
  explicit inline TcpResolver(IoContext& ioContext) : impl(ioContext.impl) {};
};

template <typename T>
inline constexpr auto Buffer(const T* ptr, std::size_t size) {
  return boost::asio::buffer(ptr, size);
};

template <typename T>
inline constexpr auto DynamicBuffer(T&& arg) -> decltype(boost::asio::dynamic_buffer(std::forward<T>(arg))) {
  return boost::asio::dynamic_buffer(std::forward<T>(arg));
};

template <typename Socket, typename Buffer>
inline auto AsyncWrite(Socket&& socket, Buffer buffer) -> Task<> {
  return boost::asio::async_write(std::forward<Socket>(socket).impl, std::move(buffer), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer, typename CompletionCondition>
inline auto AsyncWrite(Socket&& socket, Buffer buffer, CompletionCondition completion) -> Task<> {
  return boost::asio::async_write(std::forward<Socket>(socket).impl, std::move(buffer), std::move(completion), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer>
inline auto AsyncRead(Socket&& socket, Buffer buffer) -> Task<> {
  return boost::asio::async_read(std::forward<Socket>(socket).impl, std::move(buffer), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer, typename CompletionCondition>
inline auto AsyncRead(Socket&& socket, Buffer buffer, CompletionCondition completion) -> Task<> {
  return boost::asio::async_read(std::forward<Socket>(socket).impl, std::move(buffer), std::move(completion), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer, typename Match>
inline auto AsyncReadUntil(Socket&& socket, Buffer buffer, Match match) -> Task<> {
  return boost::asio::async_read_until(std::forward<Socket>(socket).impl, std::move(buffer), std::move(match), boost::asio::use_awaitable);
};

inline auto TransferAll() {
  return boost::asio::transfer_all();
};

}  // namespace cserver::network
