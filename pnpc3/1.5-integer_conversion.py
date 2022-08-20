#!/usr/bin/env python3

# 网络序、主机序转换

import socket


def convert_integer(integer):
    print('Origin: {} => Long host order: {} => Long network order: {}'.format(
        integer, socket.htonl(integer), socket.ntohl(integer)))
    print(
        'Origin: {} => Short host order: {} => Short network order: {}'.format(
            integer, socket.htons(integer), socket.ntohs(integer)))


if __name__ == '__main__':
    import sys
    for integer in sys.argv[1:]:
        convert_integer(int(integer))
