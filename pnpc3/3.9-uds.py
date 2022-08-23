#!/usr/bin/env python3

# 测试 Unix 域套接字

import os
import socket


def uds_server(name):
    server_path = f'/tmp/uds_{name}'
    if os.path.exists(server_path):
        os.remove(server_path)

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.bind(server_path)
    print(f'Unix domain server start at {server_path}')
    while True:
        msg = sock.recv(4096)
        print(f'Receive message: {msg.decode()}')


def uds_client(name, msg):
    server_path = f'/tmp/uds_{name}'
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    sock.sendto(msg.encode(), server_path)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-s', '--server', action='store_true')
    parser.add_argument('-m', '--message', default='hello')
    parser.add_argument('name')
    args = parser.parse_args()
    if args.server:
        uds_server(args.name)
    else:
        uds_client(args.name, args.message)
