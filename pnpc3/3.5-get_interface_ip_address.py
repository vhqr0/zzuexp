#!/usr/bin/env python3

# 获取网络接口地址

import socket
import fcntl
import struct

SIOCGIFADDR = 0x8915


def get_ip_address(interface):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ifaddr = fcntl.ioctl(sock.fileno(), SIOCGIFADDR,
                         struct.pack('256s',
                                     interface.encode()[:15]))
    addr = socket.inet_ntop(socket.AF_INET, ifaddr[20:24])
    print(f'Interface [{interface}] => IP: {addr}')


if __name__ == '__main__':
    import sys
    for interface in sys.argv[1:]:
        get_ip_address(interface)
