#include <iostream>
#include <string>
#include <vector>

#include <asio.hpp>
#include <asio/ssl.hpp>

#include "dns.h"

void digot(std::string ns, std::string domain) {
  asio::io_context context;
  asio::ssl::context ssl_context(asio::ssl::context::sslv23);
  ssl_context.set_verify_mode(asio::ssl::verify_peer);
  ssl_context.set_default_verify_paths();
  asio::ssl::stream<asio::ip::tcp::socket> stream(context, ssl_context);
  stream.lowest_layer().connect({asio::ip::address::from_string(ns), 853});
  stream.handshake(stream.client);

  std::vector<uint8_t> bytes;
  bytes.push_back(0);
  bytes.push_back(0);
  DNS dns;
  dns.id = 1;
  dns.flags = DNS_FLAG_RD;
  dns.querys.push_back({dns_domain_split(domain), DNS_TYPE_A, DNS_CLASS_IN});
  dns.push_to_bytes(bytes);
  bytes[1] = bytes.size() - 2;

  stream.write_some(asio::buffer(bytes));
  char buf[512];
  stream.read_some(asio::buffer(buf, 2));
  std::size_t length = stream.read_some(asio::buffer(buf, 512));
  std::cout << "query to ns " << ns << " over tls" << std::endl;
  dns.from_bytes((uint8_t *)buf, length, 0);
  dns.print();
}
