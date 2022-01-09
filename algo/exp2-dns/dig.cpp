#include <iostream>
#include <string>
#include <vector>

#include <asio.hpp>

#include "dns.h"

using asio::ip::udp;

#define TIMEOUT 5

void dig(std::string ns, std::string domain) {
  asio::io_context context;
  udp::endpoint nsendpoint(asio::ip::address::from_string(ns), 53);
  udp::endpoint nsrendpoint;
  udp::socket socket(context, nsendpoint.address().is_v4() ? udp::v4() : udp::v6());

  DNS dns;
  dns.id = 1;
  dns.flags = DNS_FLAG_RD;
  dns.querys.push_back({dns_domain_split(domain), DNS_TYPE_A, DNS_CLASS_IN});
  std::vector<uint8_t> bytes;
  dns.push_to_bytes(bytes);
  socket.send_to(asio::buffer(bytes), nsendpoint);

  asio::steady_timer timer(context);
  timer.expires_after(asio::chrono::seconds(5));
  timer.async_wait([&context](std::error_code ec) {
    context.stop();
    std::cout << "timeout" << std::endl;
  });
  uint8_t buf[512];
  socket.async_receive_from(
      asio::buffer(buf, 512), nsrendpoint,
      [&buf, &context, &nsendpoint, &nsrendpoint](std::error_code ec,
                                                  std::size_t length) {
        context.stop();
        std::cout << "query to ns " << nsendpoint << std::endl;
        std::cout << "ns responsed from " << nsrendpoint << std::endl;
        DNS dns;
        dns.from_bytes((uint8_t *)buf, length, 0);
        dns.print();
      });

  context.run();
}
