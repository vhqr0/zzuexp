#include <array>
#include <iostream>
#include <string>

#include <asio.hpp>

#define BUF_SIZE 4096

int main(int argc, char **argv) {
  asio::io_context io_context;
  asio::ip::udp::endpoint endpoint;
  asio::ip::udp::socket socket(io_context);
  std::array<char, BUF_SIZE> buf;

  if (argc != 3) {
    fprintf(stderr, "usage: echos IP PORT\n");
    return -1;
  }
  endpoint = *asio::ip::udp::resolver(io_context).resolve(argv[1], argv[2]);
  socket.open(endpoint.protocol());
  socket.bind(endpoint);
  while (true) {
    std::size_t n = socket.receive_from(asio::buffer(buf, BUF_SIZE - 1), endpoint);
    buf[n] = 0;
    std::cout << "receive from " << endpoint << ": " << &buf[0] << std::endl;
    socket.send_to(asio::buffer(buf, n), endpoint);
  }
}
