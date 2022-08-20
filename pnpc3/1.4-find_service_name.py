#!/usr/bin/env python3

# 通过端口和协议找到服务名

import socket


def find_service_name(port, protocol):
    try:
        serv = socket.getservbyport(port, protocol)
        print(f'Protocol: {protocol}, Port: {port} => Service: {serv}')
    except OSError as e:
        print(f'Protocol: {protocol}, Port: {port} => {e}')


if __name__ == '__main__':
    import sys
    assert len(sys.argv) == 3
    find_service_name(int(sys.argv[1]), sys.argv[2])
