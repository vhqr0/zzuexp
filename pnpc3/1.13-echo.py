#!/usr/bin/env python3

# 回显客户端/服务端

import socket


def echo_client(host, port, msg):
    sock = socket.create_connection((host, port))
    print(f'Connect to {host}:{port}')
    sock.sendall(msg.encode())
    msg = sock.recv(4096)
    print('Receive: ' + msg.decode())


def echo_server(host, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
    sock.bind((host, port))
    sock.listen()
    while True:
        csock, caddr = sock.accept()
        print(f'Accept from {caddr}')
        msg = csock.recv(4096)
        print(f'Receive: ' + msg.decode())
        csock.sendall(msg)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='localhost')
    parser.add_argument('--port', type=int, default=1313)
    parser.add_argument('-m', '--msg', default='hello, world')
    parser.add_argument('-s', '--server', action='store_true')
    args = parser.parse_args()
    if args.server:
        echo_server(args.host, args.port)
    else:
        echo_client(args.host, args.port, args.msg)
