#!/usr/bin/env python3

# require: python-nmap

# 探测网络接口状态

import socket
import fcntl
import struct
import nmap

SIOCGIFADDR = 0x8915


def get_interface_status(interface):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ifaddr = fcntl.ioctl(sock.fileno(), SIOCGIFADDR,
                         struct.pack('256s',
                                     interface.encode()[:15]))
    addr = socket.inet_ntop(socket.AF_INET, ifaddr[20:24])
    scanner = nmap.PortScanner()
    res = scanner.scan(addr, arguments='-sS -F')['scan'][addr]['tcp']
    for port in res:
        print(f'[{interface}] {addr}:{port} ({res[port]["name"]}) {res[port]["state"]}')


if __name__ == '__main__':
    import sys
    for interface in sys.argv[1:]:
        get_interface_status(interface)
