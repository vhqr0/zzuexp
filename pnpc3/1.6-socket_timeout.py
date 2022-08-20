#!/usr/bin/env python3

# 测试套接字超时

import socket


def test_socket_timeout(timeout):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    default_timeout = sock.gettimeout()
    sock.settimeout(timeout)
    current_timeout = sock.gettimeout()
    print(f'Default timeout: {default_timeout}')
    print(f'Current timeout: {current_timeout}')


if __name__ == '__main__':
    test_socket_timeout(10)
