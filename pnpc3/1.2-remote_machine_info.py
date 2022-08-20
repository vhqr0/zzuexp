#!/usr/bin/env python3

# 获取远程设备信息

import socket


def get_remote_machine_info(host_name):
    try:
        ip_address = socket.gethostbyname(host_name)
        print(f'Host name: {host_name}')
        print(f'IP address: {ip_address}')
    except OSError as e:
        print(f'{host_name}: {e}')


if __name__ == '__main__':
    import sys
    for host_name in sys.argv[1:]:
        get_remote_machine_info(host_name)
