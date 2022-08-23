#!/usr/bin/env python3

# require: netifaces

# 检查设备是否支持 IPv6

import socket
import netifaces


def inspect_ipv6_support():
    print(f'IPv6 support built into Python: {socket.has_ipv6}')
    ipv6_addrs = {}
    for interface in netifaces.interfaces():
        all_addrs = netifaces.ifaddresses(interface)
        for family, addrs in all_addrs.items():
            family_name = netifaces.address_families[family]
            print(f'Address family: {family_name}')
            for addr in addrs:
                if family_name == 'AF_INET6':
                    ipv6_addrs.setdefault(interface, []).append(addr['addr'])
                print(f'\tAddress: {addr.get("addr", None)}')
                print(f'\tNetmask: {addr.get("netmask", None)}')
                print(f'\tBroadcast: {addr.get("broadcast", None)}')
    print(f'Found IPv6 addresses: {ipv6_addrs}')


if __name__ == '__main__':
    inspect_ipv6_support()
