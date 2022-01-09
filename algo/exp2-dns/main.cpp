#include <iostream>
#include <string>

#include <asio.hpp>

#include "dns.h"

using asio::ip::udp;

int main(int argc, const char **argv) {
  if (argc < 2) {
    std::cerr << "wrong argument" << std::endl;
    return -1;
  }
  std::string cmd(argv[1]);
  if (cmd == "dig") {
    if (argc != 4) {
      std::cerr << "wrong argument" << std::endl;
      return -1;
    }
    dig(argv[2], argv[3]);
  } else if (cmd == "digot") {
    if (argc != 4) {
      std::cerr << "wrong argument" << std::endl;
      return -1;
    }
    digot(argv[2], argv[3]);
  } else if (cmd == "bind") {
    if (argc != 4) {
      std::cerr << "wrong argument" << std::endl;
      return -1;
    }
    bind(argv[2], argv[3]);
  } else if (cmd == "bindot") {
    if (argc != 4) {
      std::cerr << "wrong argument" << std::endl;
      return -1;
    }
    bindot(argv[2], argv[3]);
  } else {
    std::cerr << "wrong argument" << std::endl;
  }
}
