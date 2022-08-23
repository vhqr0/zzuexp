#!/usr/bin/env python3

# ping

import time
import select
import socket
import struct
import random

ICMP = socket.getprotobyname('icmp')
ICMP_ECHOREQ = 8
ICMP_ECHOREP = 0


def checksum(buf):
    s, c = 0, 0

    while c + 1 < len(buf):
        w, = struct.unpack_from('!H', buffer=buf, offset=c)
        s += w
        c += 2

    if c + 1 == len(buf):
        s += buf[c] << 8

    s &= 0xffffffff
    s = (s >> 16) + (s & 0xffff)
    s += s >> 16
    return ~s & 0xffff


class Pinger:

    def __init__(self, target, count=5, timeout=2):
        self.target = target
        self.count = count
        self.timeout = timeout
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_RAW, ICMP)

    def send_ping(self, pid, seq):
        pkt = struct.pack('!BBHHH4s', ICMP_ECHOREQ, 0, 0, pid, seq, b'xxxx')
        pkt = struct.pack('!BBHHH4s', ICMP_ECHOREQ, 0, checksum(pkt), pid, seq,
                          b'xxxx')
        self.sock.sendto(pkt, (self.target, 0))

    def recv_pong(self, pid, seq):
        time_beg = time.time()
        time_end = time_beg + self.timeout
        time_sel = self.timeout
        while time_sel > 0:
            rsocks, _, _ = select.select([self.sock], [], [], time_sel)
            if not rsocks:
                return  # timeout
            pkt, addr = self.sock.recvfrom(4096)
            try:
                itype, code, checksum, rpid, rseq, payload = \
                    struct.unpack_from('!BBHHH4s', buffer=pkt, offset=20)
                if itype == ICMP_ECHOREP and \
                   rpid == pid and \
                   rseq == seq and \
                   payload == b'xxxx':
                    return addr, time.time() - time_beg  # recv
            except:
                pass
            time_sel = time_end - time.time()

    def ping_once(self, pid, seq):
        self.send_ping(pid, seq)
        res = self.recv_pong(pid, seq)
        if res:
            addr, rtt = res
            print(f'Ping-Pong from {addr}, seq={seq}, rtt={rtt}s')
        else:
            print(f'Ping timeout, seq={seq}')

    def ping(self):
        for seq in range(self.count):
            pid = random.getrandbits(16)
            self.ping_once(pid, seq)


if __name__ == '__main__':
    import sys
    assert len(sys.argv) == 2
    pinger = Pinger(sys.argv[1])
    pinger.ping()
