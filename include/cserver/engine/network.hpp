#pragma once
#include <cserver/engine/coroutine.hpp>
namespace cserver::network {

class DeadLineTimer;

class TcpSocket;
class TcpResolver;

class IoContext {
  boost::asio::io_context impl;
public:
  inline constexpr IoContext() : impl() {};
  class Work {
    boost::asio::io_context::work impl;
  public:
    inline constexpr Work(IoContext& ioContext) :
      impl(ioContext.impl) {};
    inline constexpr Work(Work&&) = default;
    inline constexpr Work(const Work&) = default;
  };

  friend DeadLineTimer;
  friend TcpSocket;

  friend TcpResolver;
};


class DeadLineTimer {
  boost::asio::deadline_timer impl;
public:
  inline constexpr DeadLineTimer(IoContext& ioContext) : 
      impl(ioContext.impl) {};
  inline constexpr auto AsyncWait() -> Task<> {
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
  inline constexpr TcpSocket(IoContext& ioContext) :
    impl(ioContext.impl) {};
  inline constexpr TcpSocket(TcpSocket&& other) :
    impl(std::move(other.impl)) {};

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
  inline constexpr TcpEndpoint(boost::asio::ip::tcp::endpoint impl) : impl(std::move(impl)) {};
public:
  friend TcpEntry;
};


class TcpEntry {
  boost::asio::ip::tcp::resolver::results_type::value_type impl;
public:
  inline constexpr TcpEntry() = default;
  inline constexpr TcpEntry(const TcpEndpoint& ep,
                            std::string_view host,
                            std::string_view service) : impl(ep.impl, host, service) {};
  inline constexpr auto Endpoint() -> TcpEndpoint {
    return {this->impl.endpoint()};
  };
  inline constexpr auto HostName() -> std::string {
    return this->impl.host_name();
  };
  inline constexpr auto ServiceName() -> std::string {
    return this->impl.service_name();
  };
  inline constexpr operator TcpEndpoint() {
    return {this->impl};
  };
};


class TcpResolver {
  boost::asio::ip::tcp::resolver impl;
public:
  inline constexpr TcpResolver(IoContext& ioContext) : impl(ioContext.impl) {};
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
inline constexpr auto AsyncWrite(Socket&& socket, Buffer buffer) -> Task<> {
  return boost::asio::async_write(std::forward<Socket>(socket).impl, std::move(buffer), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer, typename CompletionCondition>
inline constexpr auto AsyncWrite(Socket&& socket, Buffer buffer, CompletionCondition completion) -> Task<> {
  return boost::asio::async_write(std::forward<Socket>(socket).impl, std::move(buffer), std::move(completion), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer>
inline constexpr auto AsyncRead(Socket&& socket, Buffer buffer) -> Task<> {
  return boost::asio::async_read(std::forward<Socket>(socket).impl, std::move(buffer), boost::asio::use_awaitable);
};

template <typename Socket, typename Buffer, typename CompletionCondition>
inline constexpr auto AsyncRead(Socket&& socket, Buffer buffer, CompletionCondition completion) -> Task<> {
  return boost::asio::async_read(std::forward<Socket>(socket).impl, std::move(buffer), std::move(completion), boost::asio::use_awaitable);
};

template <typename Buffer, typename Match>
inline constexpr auto AsyncReadUntil(TcpSocket& socket, Buffer buffer, Match match) -> Task<> {
  return boost::asio::async_read_until(socket.impl, std::move(buffer), std::move(match), boost::asio::use_awaitable);
};

inline constexpr auto TransferAll() {
  return boost::asio::transfer_all();
};

} // namespace cserver::network
