#!/usr/bin/env python3

# require: netifaces

# 提取 IPv6 前缀
# 用 python3 内置的 ipaddress 实现

import socket
import netifaces
import ipaddress


def extract_ipv6_info():
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
                if family_name == 'AF_INET6':
                    mask = addr.get('netmask', '::/128')
                    mask = ipaddress.IPv6Network(mask)
                    prefixlen = mask.prefixlen
                    mask = int(mask.netmask)
                    addr = addr.get('addr', '::')
                    addr = int(ipaddress.IPv6Address(addr))
                    network = ipaddress.IPv6Address(addr & mask)
                    print(f'\tNetwork: {network}/{prefixlen}')
    print(f'Found IPv6 addresses: {ipv6_addrs}')


if __name__ == '__main__':
    extract_ipv6_info()
