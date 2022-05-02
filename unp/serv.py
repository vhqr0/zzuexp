#!/usr/bin/env python3

import argparse
import fcntl
import ipaddress
import signal
import socket
import struct
import sys
import time

SIOCGIFADDR = 0x8915
SIOCGIFADDR_SLICE = (20, 24)

parser = argparse.ArgumentParser(
    description='Daytime Server&Client, by IPv4&IPv6, TCP&UDP.',
    usage='%(prog)s [-h] [-cs] [-46] [-UT] [-p PORT]'
    ' [-i INTERFACE] [-t TIMEOUT] ADDRESS')
parser.add_argument('-c',
                    action='store_const',
                    dest='mode',
                    const='client',
                    help='Mode: Client')
parser.add_argument('-s',
                    action='store_const',
                    dest='mode',
                    const='server',
                    help='Mode: Server')
parser.add_argument('-4',
                    action='store_const',
                    dest='family',
                    const=socket.AF_INET,
                    help='Family: IPv4')
parser.add_argument('-6',
                    action='store_const',
                    dest='family',
                    const=socket.AF_INET6,
                    help='Family: IPv6')
parser.add_argument('-U',
                    action='store_const',
                    dest='socktype',
                    const=socket.SOCK_DGRAM,
                    help='Type: UDP')
parser.add_argument('-T',
                    action='store_const',
                    dest='socktype',
                    const=socket.SOCK_STREAM,
                    help='Type: TCP')
parser.add_argument('-p', '--port', default='13', help='TCP|UDP Port')
parser.add_argument('-i', '--interface', help='Interface')
parser.add_argument('-t',
                    '--timeout',
                    type=int,
                    default=1,
                    help='UDP Client Timeout')
parser.add_argument('address', help='IP Address')
args = parser.parse_args()

mode = args.mode or 'server'
family = args.family or socket.AF_INET
socktype = args.socktype or socket.SOCK_DGRAM
ep = socket.getaddrinfo(args.address, args.port, family=family,
                        type=socktype)[0][-1]
interface = args.interface
timeout = args.timeout


def client():
    sockfd = socket.socket(family, socktype)
    if interface:
        sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE,
                          interface.encode())
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
    sockfd = socket.socket(family, socktype)
    servep = ep
    if family == socket.AF_INET:
        if ipaddress.IPv4Address(ep[0]).is_multicast:
            ifaddr = b'\x00\x00\x00\x00'
            if interface:
                ifaddr = fcntl.ioctl(
                    sockfd, SIOCGIFADDR,
                    struct.pack('64s',
                                interface.encode()))[SIOCGIFADDR_SLICE[0],
                                                     SIOCGIFADDR_SLICE[1]]
            sockfd.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
                              socket.inet_pton(socket.AF_INET, ep[0]) + ifaddr)
            servep = ('', ep[1])
    elif family == socket.AF_INET6:
        if ipaddress.IPv6Address(ep[0]).is_multicast:
            ifindex = 0
            if interface:
                ifindex = socket.if_nametoindex(interface)
            sockfd.setsockopt(
                socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP,
                socket.inet_pton(socket.AF_INET6, ep[0]) +
                struct.pack('@i', ifindex))
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
