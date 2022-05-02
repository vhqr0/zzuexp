#!/usr/bin/env python3

import argparse
import ipaddress
import signal
import socket
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument('-c', action='store_const', dest='mode', const='client')
parser.add_argument('-s', action='store_const', dest='mode', const='server')
parser.add_argument('-4',
                    action='store_const',
                    dest='family',
                    const=socket.AF_INET)
parser.add_argument('-6',
                    action='store_const',
                    dest='family',
                    const=socket.AF_INET6)
parser.add_argument('-U',
                    action='store_const',
                    dest='socktype',
                    const=socket.SOCK_DGRAM)
parser.add_argument('-T',
                    action='store_const',
                    dest='socktype',
                    const=socket.SOCK_STREAM)
parser.add_argument('-p', '--port', default='13')
parser.add_argument('-t', '--timeout', type=int, default=1)
parser.add_argument('address')
args = parser.parse_args()

mode = args.mode or 'server'
family = args.family or socket.AF_INET
socktype = args.socktype or socket.SOCK_DGRAM
ep = socket.getaddrinfo(args.address, args.port, family=family,
                        type=socktype)[0][-1]
timeout = args.timeout


def client():
    sockfd = socket.socket(family, socktype)
    if socktype == socket.SOCK_STREAM:
        sockfd.connect(ep)
        sys.stdout.buffer.write(sockfd.recv(4096))
    elif socktype == socket.SOCK_DGRAM:
        if family == socket.AF_INET and ep[0] == '255.255.255.255':
            sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sockfd.sendto(b'\x00', ep)
        if timeout > 0:
            signal.signal(signal.SIGALRM, lambda _no, _f: sys.exit(0))
            signal.alarm(timeout)
        while True:
            buf, servep = sockfd.recvfrom(4096)
            sys.stdout.buffer.write(
                f'recvfrom {servep[0]}, port {servep[1]}: '.encode())
            sys.stdout.buffer.write(buf)
            sys.stdout.flush()
    sockfd.close()


def server():
    servep = ep
    sockfd = socket.socket(family, socktype)
    if family == socket.AF_INET:
        if ipaddress.IPv4Address(ep[0]).is_multicast:
            sockfd.setsockopt(
                socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
                socket.inet_pton(socket.AF_INET, ep[0]) + b'\x00' * 4)
            servep = ('', ep[1])
    elif family == socket.AF_INET6:
        if ipaddress.IPv6Address(ep[0]).is_multicast:
            sockfd.setsockopt(
                socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP,
                socket.inet_pton(socket.AF_INET6, ep[0]) + b'\x00' * 16)
            servep = ('', ep[1])
    sockfd.bind(servep)
    if socktype == socket.SOCK_STREAM:
        sockfd.listen()
    signal.signal(signal.SIGINT, lambda _no, _f: sys.exit(0))
    while True:
        if socktype == socket.SOCK_STREAM:
            clifd, cliep = sockfd.accept()
            print(f'accept {cliep[0]}, port {cliep[1]}')
            clifd.send(f'{time.ctime()}\r\n'.encode())
            clifd.close()
        elif socktype == socket.SOCK_DGRAM:
            _, cliep = sockfd.recvfrom(4096)
            print(f'recvfrom {cliep[0]}, port {cliep[1]}')
            sockfd.sendto(f'{time.ctime()}\n'.encode(), cliep)


if mode == 'client':
    client()
elif mode == 'server':
    server()
