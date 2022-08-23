#!/usr/bin/env python3

# 列出网络接口

import platform
import socket
import fcntl
import array
import struct

SIOCGIFCONF = 0x8912

ARCH = platform.architecture()[0]
assert ARCH in ('32bit', '64bit')

IFREQSIZE = 40 if ARCH == '64bit' else 32
MAX_INTERFACES = 1024


def list_interfaces():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    buf = array.array('B', bytes(IFREQSIZE * MAX_INTERFACES))
    ifconf = fcntl.ioctl(sock.fileno(), SIOCGIFCONF,
                         struct.pack('@iL', 1024, buf.buffer_info()[0]))
    nread, _ = struct.unpack('@iL', ifconf)
    buf = buf.tobytes()
    interfaces = []
    for i in range(0, nread, IFREQSIZE):
        ifrname = buf[i:i + 16]
        interface = ifrname[:ifrname.find(b'\x00')].decode()
        interfaces.append(interface)
    print(f'This machine has {len(interfaces)} interfaces: {interfaces}')


if __name__ == '__main__':
    list_interfaces()
