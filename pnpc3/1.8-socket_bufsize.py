#!/usr/bin/env python3

# 测试套接字缓存大小

import socket


def test_socket_bufsize(bufsize):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    default_rcv_bufsize = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    default_snd_bufsize = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, bufsize)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, bufsize)
    current_rcv_bufsize = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    current_snd_bufsize = sock.getsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF)
    print(f'Default rcv bufsize: {default_rcv_bufsize}')
    print(f'Current rcv bufsize: {current_rcv_bufsize}')
    print(f'Default snd bufsize: {default_snd_bufsize}')
    print(f'Current snd bufsize: {current_snd_bufsize}')


if __name__ == '__main__':
    test_socket_bufsize(4096)
