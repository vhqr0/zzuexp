#!/usr/bin/env python3

# sntp客户端实现
# test: ./1.12-sntp_client.py 1.debian.pool.ntp.org

import sys
import time
import socket
import struct

TIME1970 = 2208988800


def sntp_client(sntp_server, port=123):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(5)
    data = b'\x1b' + 47 * b'\x00'
    sock.sendto(data, (sntp_server, port))
    data, addr = sock.recvfrom(4096)
    rtime = struct.unpack('!12I', data)[10]
    rtime -= TIME1970
    print(f'Receive time={time.ctime(rtime)} from {addr}')


if __name__ == '__main__':
    assert len(sys.argv) == 2
    sntp_client(sys.argv[1])
