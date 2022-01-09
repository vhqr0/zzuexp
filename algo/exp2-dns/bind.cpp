#include <iostream>
#include <memory>

#include <asio.hpp>

#include "dns.h"

using asio::ip::udp;

#define TIMEOUT 5
#define HLINE "----------------------------------------"

class DNSProxy {
public:
  DNSProxy(asio::io_context &context, std::string addr, std::string ns);
  void start();

private:
  asio::io_context &context_;
  udp::endpoint nsendpoint_;
  udp::socket socket_;
};

class DNSProxySession : public std::enable_shared_from_this<DNSProxySession> {
public:
  DNSProxySession(DNSProxy &proxy, asio::io_context &context,
                  udp::endpoint nsendpoint)
      : proxy_(proxy), nsendpoint_(nsendpoint), socket_(context, udp::v4()),
        timer_(context) {}

  void do_receive_from_remote(udp::socket &socket) {
    auto self(shared_from_this());
    socket.async_receive_from(
        asio::buffer(data_, max_length), rendpoint_,
        [this, self](std::error_code ec, std::size_t length) {
          proxy_.start();
          length_ = length;
#ifdef DNS_VERBSE
          std::cout << HLINE << std::endl;
          std::cout << "receive from remote " << rendpoint_ << std::endl;
          DNS dns;
          dns.from_bytes((uint8_t *)data_, length, 0);
          dns.print();
          std::cout << HLINE << std::endl;
#endif
          do_send_to_ns();
        });
  }

private:
  DNSProxy &proxy_;
  enum { max_length = 512 };
  char data_[max_length];
  std::size_t length_;
  udp::endpoint rendpoint_;
  udp::endpoint nsendpoint_;
  udp::endpoint nsrendpoint_;
  udp::socket socket_;
  asio::steady_timer timer_;

  void do_send_to_ns() {
    auto self(shared_from_this());
    socket_.async_send_to(asio::buffer(data_, length_), nsendpoint_,
                          [this, self](std::error_code ec, std::size_t length) {
                            do_receive_from_ns_with_timeout();
                          });
  }

  void do_receive_from_ns_with_timeout() {
    auto self(shared_from_this());
    timer_.expires_after(asio::chrono::seconds(TIMEOUT));
    timer_.async_wait([this, self](std::error_code ec) {
      if (ec)
        return;
      socket_.cancel();
      std::cout << HLINE << std::endl;
      std::cout << "receive from ns " << nsendpoint_ << " timeout" << std::endl;
      std::cout << HLINE << std::endl;
    });
    socket_.async_receive_from(
        asio::buffer(data_, 512), nsrendpoint_,
        [this, self](std::error_code ec, std::size_t length) {
          if (ec)
            return;
          timer_.cancel();
          length_ = length;
#ifdef DNS_VERBSE
          std::cout << HLINE << std::endl;
          std::cout << "receive from ns " << nsrendpoint_ << std::endl;
          DNS dns;
          dns.from_bytes((uint8_t *)data_, length, 0);
          dns.print();
          std::cout << HLINE << std::endl;
#endif
          do_send_to_remote();
        });
  }

  void do_send_to_remote() {
    auto self(shared_from_this());
    socket_.async_send_to(
        asio::buffer(data_, length_), rendpoint_,
        [this, self](std::error_code ec, std::size_t length) {});
  }
};

DNSProxy::DNSProxy(asio::io_context &context, std::string addr, std::string ns)
    : context_(context), nsendpoint_(asio::ip::address::from_string(ns), 53),
      socket_(context,
              udp::endpoint(asio::ip::address::from_string(addr), 53)) {
  start();
}

void DNSProxy::start() {
  std::make_shared<DNSProxySession>(*this, context_, nsendpoint_)
      ->do_receive_from_remote(socket_);
}

void bind(std::string addr, std::string ns) {
  asio::io_context context;
  DNSProxy proxy(context, addr, ns);
  context.run();
}
