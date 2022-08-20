#!/usr/bin/env python3

# 转换IPv4地址

import socket


def convert_ipv4_ipaddress(ip_address):
    packed_addr = socket.inet_aton(ip_address)
    unpacked_addr = socket.inet_ntoa(packed_addr)
    print('IP address: {} => Packed: {} => Unpacked: {}'.format(
        ip_address, packed_addr.hex(), unpacked_addr))


if __name__ == '__main__':
    import sys
    for ip_address in sys.argv[1:]:
        convert_ipv4_ipaddress(ip_address)
