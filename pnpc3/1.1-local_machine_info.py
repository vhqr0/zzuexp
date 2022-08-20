#!/usr/bin/env python3

# 获取本地设备信息

import socket


def get_local_machine_info():
    host_name = socket.gethostname()
    ip_address = socket.gethostbyname(host_name)
    print(f'Host name: {host_name}')
    print(f'IP address: {ip_address}')


if __name__ == '__main__':
    get_local_machine_info()
