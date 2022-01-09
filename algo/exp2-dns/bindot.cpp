#include <iostream>
#include <memory>

#include <asio.hpp>
#include <asio/ssl.hpp>

#include "dns.h"

#define HLINE "----------------------------------------"

class DNSOTProxy {
public:
  DNSOTProxy(asio::io_context &context, asio::ssl::context &ssl_context,
             std::string addr, std::string ns);
  void start();

private:
  asio::io_context &context_;
  asio::ssl::context &ssl_context_;
  asio::ip::udp::socket socket_;
  asio::ip::tcp::endpoint nsendpoint_;
};

class DNSOTProxySession
    : public std::enable_shared_from_this<DNSOTProxySession> {
public:
  DNSOTProxySession(DNSOTProxy &proxy, asio::io_context &context,
                    asio::ssl::context &ssl_context,
                    asio::ip::tcp::endpoint nsendpoint)
      : proxy_(proxy), nsendpoint_(nsendpoint),
        socket_(context, asio::ip::udp::v4()), stream_(context, ssl_context) {}

  void do_receive_from_remote(asio::ip::udp::socket &socket) {
    auto self(shared_from_this());
    socket.async_receive_from(
        asio::buffer(data_ + 2, max_length), rendpoint_,
        [this, self](std::error_code ec, std::size_t length) {
          proxy_.start();
          data_[0] = (length & 0xff00) >> 8;
          data_[1] = length & 0xff;
          length_ = length + 2;
#ifdef DNS_VERBSE
          std::cout << HLINE << std::endl;
          std::cout << "receive from remote " << rendpoint_ << std::endl;
          DNS dns;
          dns.from_bytes((uint8_t *)data_ + 2, length, 0);
          dns.print();
          std::cout << HLINE << std::endl;
#endif
          do_connect();
        });
  }

private:
  DNSOTProxy &proxy_;
  enum { max_length = 512 };
  char data_[2 + max_length];
  std::size_t length_;
  asio::ip::udp::endpoint rendpoint_;
  asio::ip::tcp::endpoint nsendpoint_;
  asio::ip::udp::socket socket_;
  asio::ssl::stream<asio::ip::tcp::socket> stream_;

  void do_connect() {
    auto self(shared_from_this());
    stream_.lowest_layer().async_connect(nsendpoint_,
                                         [this, self](std::error_code ec) {
                                           if (ec)
                                             return;
                                           do_handshake();
                                         });
  }

  void do_handshake() {
    auto self(shared_from_this());
    stream_.async_handshake(stream_.client, [this, self](std::error_code ec) {
      if (ec)
        return;
      do_write();
    });
  }

  void do_write() {
    auto self(shared_from_this());
    stream_.async_write_some(
        asio::buffer(data_, length_),
        [this, self](std::error_code ec, std::size_t length) {
          if (ec)
            return;
          do_read_1();
        });
  }

  void do_read_1() {
    auto self(shared_from_this());
    asio::async_read(stream_, asio::buffer(data_, 2),
                     [this, self](std::error_code ec, std::size_t length) {
                       if (ec)
                         return;
                       do_read_2();
                     });
  }

  void do_read_2() {
    auto self(shared_from_this());
    length_ = (data_[0] << 8) + data_[1];
    if (length_ > max_length)
      return;
    asio::async_read(stream_, asio::buffer(data_, length_),
                     [this, self](std::error_code ec, std::size_t length) {
                       if (ec)
                         return;
#ifdef DNS_VERBSE
                       std::cout << HLINE << std::endl;
                       std::cout << "receive from ns " << nsendpoint_
                                 << std::endl;
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

DNSOTProxy::DNSOTProxy(asio::io_context &context,
                       asio::ssl::context &ssl_context, std::string addr,
                       std::string ns)
    : context_(context), ssl_context_(ssl_context),
      nsendpoint_(asio::ip::address::from_string(ns), 853),
      socket_(context, asio::ip::udp::endpoint(
                           asio::ip::address::from_string(addr), 53)) {
  start();
}

void DNSOTProxy::start() {
  std::make_shared<DNSOTProxySession>(*this, context_, ssl_context_,
                                      nsendpoint_)
      ->do_receive_from_remote(socket_);
}

void bindot(std::string addr, std::string ns) {
  asio::io_context context;
  asio::ssl::context ssl_context(asio::ssl::context::sslv23);
  ssl_context.set_verify_mode(asio::ssl::verify_peer);
  ssl_context.set_default_verify_paths();
  DNSOTProxy proxy(context, ssl_context, addr, ns);
  context.run();
}
