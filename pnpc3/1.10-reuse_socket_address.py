#!/usr/bin/env python3

# 重用套接字地址

import socket


def reuse_socket_address(port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    old_state = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
    new_state = sock.getsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR)
    print(f'Old sock state: {old_state}')
    print(f'New sock state: {new_state}')

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.bind(('localhost', port))
    srv.listen()
    print(f'Listen on port: {port}')
    while True:
        csock, caddr = srv.accept()
        print(f'Accept from: {caddr}')


if __name__ == '__main__':
    import sys
    assert len(sys.argv) == 2
    reuse_socket_address(int(sys.argv[1]))
