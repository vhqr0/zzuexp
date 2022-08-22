#!/usr/bin/env python3

import argparse
import math
import os
import struct
import signal
import socket
import sys
import time

from icmp6filter import *

ICMP_ECHO = 8
ICMP_ECHOREPLY = 0

parser = argparse.ArgumentParser()
parser.add_argument('-4',
                    action='store_const',
                    dest='family',
                    const=socket.AF_INET)
parser.add_argument('-6',
                    action='store_const',
                    dest='family',
                    const=socket.AF_INET6)
parser.add_argument('-a', '--arping', action='store_true', default=False)
parser.add_argument('-i', '--interface')
parser.add_argument('-l', '--length', type=int, default=64)
parser.add_argument('address')
args = parser.parse_args()

family = args.family or socket.AF_INET
ep = socket.getaddrinfo(args.address,
                        None,
                        family=family,
                        type=socket.SOCK_RAW)[0][-1]
arping = args.arping
interface = args.interface
length = max(args.length - 8, 0)

pid = os.getpid() & 0xffff
seq = 0


def gettimeofday():
    t = time.time()
    sec = math.floor(t)
    t -= sec
    t *= 10**6
    usec = math.floor(t)
    return sec & 0xffffffff, usec & 0xffffffff


def cksum(buf):
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


def ping4():
    sockfd = socket.socket(family, socket.SOCK_RAW, socket.IPPROTO_ICMP)
    if interface:
        sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE,
                          interface.encode())
    if ep[0] == '255.255.255.255':
        sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    def send_ping4():
        global seq
        sec, usec = gettimeofday()
        buf = struct.pack('!BBHHHII', ICMP_ECHO, 0, 0, pid, seq, sec, usec)
        seq += 1
        if length:
            buf += b'\xa5' * length
        buf = buf[:2] + struct.pack('!H', cksum(buf)) + buf[4:]
        sockfd.sendto(buf, ep)
        signal.alarm(1)

    signal.signal(signal.SIGALRM, lambda _no, _f: send_ping4())
    signal.signal(signal.SIGINT, lambda _no, _f: sys.exit(0))
    signal.alarm(1)
    while True:
        buf, rep = sockfd.recvfrom(4096)
        hlen = (buf[0] & 0xf) << 2
        ttl, = struct.unpack_from('!B', buffer=buf, offset=8)
        icmptype, = struct.unpack_from('!B', buffer=buf, offset=hlen)
        rid, rseq, rsec, rusec = struct.unpack_from('!HHII',
                                                    buffer=buf,
                                                    offset=hlen + 4)
        if icmptype != ICMP_ECHOREPLY or rid != pid or \
           len(buf) != hlen + 16 + length:
            continue
        sec, usec = gettimeofday()
        sec -= rsec
        usec -= rusec
        rtt = sec * 1000 + usec / 1000
        print(f'recvfrom {rep[0]}, {8 + length} bytes,'
              f' seq: {rseq}, rtt: {rtt}ms, ttl: {ttl}')


def ping6():
    sockfd = socket.socket(family, socket.SOCK_RAW, socket.IPPROTO_ICMPV6)
    if interface:
        sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE,
                          interface.encode())
    icmp6f = ICMP6Filter()
    icmp6f.setblockall()
    icmp6f.setpass(ICMP6_ECHOREP)
    icmp6f.setsockopt(sockfd)
    sockfd.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_RECVHOPLIMIT, 1)

    def send_ping6():
        global seq
        sec, usec = gettimeofday()
        buf = struct.pack('!BBHHHII', ICMP6_ECHOREQ, 0, 0, pid, seq, sec, usec)
        seq += 1
        if length:
            buf += b'\xa5' * length
        sockfd.sendto(buf, ep)
        signal.alarm(1)

    signal.signal(signal.SIGALRM, lambda _no, _f: send_ping6())
    signal.signal(signal.SIGINT, lambda _no, _f: sys.exit(0))
    signal.alarm(1)
    while True:
        buf, cmsgs, flags, rep = sockfd.recvmsg(4096, 4096)
        rid, rseq, rsec, rusec = struct.unpack_from('!HHII',
                                                    buffer=buf,
                                                    offset=4)
        if rid != pid or len(buf) != 16 + length:
            continue
        hlim = '???'
        for cmsg in cmsgs:
            if cmsg[0] == socket.IPPROTO_IPV6 and \
               cmsg[1] == socket.IPV6_HOPLIMIT:
                hlim = int.from_bytes(cmsg[2], sys.byteorder)
                break
        sec, usec = gettimeofday()
        sec -= rsec
        usec -= rusec
        rtt = sec * 1000 + usec / 1000
        print(f'recvfrom {rep[0]}, {8 + length} bytes,'
              f' seq: {rseq}, rtt: {rtt}ms, hlim: {hlim}')


def arping6():
    sockfd = socket.socket(family, socket.SOCK_RAW, socket.IPPROTO_ICMPV6)
    if interface:
        sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE,
                          interface.encode())
    icmp6f = ICMP6Filter()
    icmp6f.setblockall()
    icmp6f.setpass(ICMP6ND_NA)
    icmp6f.setsockopt(sockfd)
    sockfd.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_HOPS, 255)

    def send_ns():
        tgt = socket.inet_pton(socket.AF_INET6, ep[0])
        _sep = b'\xff\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\xff' + \
            tgt[-3:]
        sep = (socket.inet_ntop(socket.AF_INET6, _sep), 0)
        buf = struct.pack('!BBHI16s', ICMP6ND_NS, 0, 0, 0, tgt)
        # cmsg = [(socket.IPPROTO_IPV6, socket.IPV6_HOPLIMIT,
        #          struct.pack('@I', 255))]
        # sockfd.sendmsg([buf], cmsg, 0, sep)
        sockfd.sendto(buf, sep)
        signal.alarm(1)

    signal.signal(signal.SIGALRM, lambda _no, _f: send_ns())
    signal.signal(signal.SIGINT, lambda _no, _f: sys.exit(0))
    signal.alarm(1)
    while True:
        buf, rep = sockfd.recvfrom(4096)
        flags = buf[4]
        print(f'recvfrom {rep[0]}, r: {(flags & 0x80) >> 7}, '
              f's: {(flags & 0x40) >> 6}, o: {(flags & 0x20) >> 5}')


if family == socket.AF_INET:
    ping4()
elif family == socket.AF_INET6:
    if arping:
        arping6()
    else:
        ping6()
