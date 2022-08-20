#!/usr/bin/env python3

# 测试套接字错误处理

import sys
import socket

def main(host, port, filename):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    except OSError as e:
        print(f'Creating socket error: {e}')
        sys.exit(-1)

    try:
        sock.connect((host, port))
    except OSError as e:
        print(f'Connection error: {e}')
        sys.exit(-1)

    try:
        req = f'GET {filename} HTTP/1.1\r\nHost: {host}:{port}\r\n\r\n'
        sock.sendall(req.encode())
    except OSError as e:
        print(f'Sending data error: {e}')
        sys.exit(-1)

    try:
        res = sock.recv(4096)
    except OSError as e:
        print(f'Receiving data error: {e}')
        sys.exit(-1)

    print(res.decode())


if __name__ == '__main__':
    assert len(sys.argv) == 4
    main(sys.argv[1], int(sys.argv[2]), sys.argv[3])
