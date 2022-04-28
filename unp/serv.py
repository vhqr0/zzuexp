#!/usr/bin/env python3

import argparse
import ipaddress
import socket
import time
import sys

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
parser.add_argument('address')
args = parser.parse_args()

family = args.family or socket.AF_INET
socktype = args.socktype or socket.SOCK_DGRAM
endpoint = socket.getaddrinfo(args.address,
                              args.port,
                              family=family,
                              type=socktype)[0][-1]
mode = args.mode or 'server'

sockfd = socket.socket(family, socktype)

if mode == 'client':
    if socktype == socket.SOCK_STREAM:
        sockfd.connect(endpoint)
        sys.stdout.buffer.write(sockfd.recv(4096))
    elif socktype == socket.SOCK_DGRAM:
        if family == socket.AF_INET and endpoint[0] == '255.255.255.255':
            sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        sockfd.sendto(b'\x00', endpoint)
        while True:
            buf, servendpoint = sockfd.recvfrom(4096)
            sys.stdout.buffer.write(
                f'recvfrom {servendpoint[0]}, port {servendpoint[1]}: '\
                .encode())
            sys.stdout.buffer.write(buf)
            sys.stdout.flush()
    sockfd.close()
else:
    if family == socket.AF_INET:
        if ipaddress.IPv4Address(endpoint[0]).is_multicast:
            sockfd.setsockopt(
                socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP,
                socket.inet_pton(socket.AF_INET, endpoint[0]) + b'\x00' * 4)
            endpoint = ('', endpoint[1])
    elif family == socket.AF_INET6:
        if ipaddress.IPv6Address(endpoint[0]).is_multicast:
            sockfd.setsockopt(
                socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP,
                socket.inet_pton(socket.AF_INET6, endpoint[0]) + b'\x00' * 16)
            endpoint = ('', endpoint[1])
    sockfd.bind(endpoint)
    if socktype == socket.SOCK_STREAM:
        sockfd.listen()
    while True:
        if socktype == socket.SOCK_STREAM:
            clifd, cliendpoint = sockfd.accept()
            print(f'accept {cliendpoint[0]}, port {cliendpoint[1]}')
            clifd.send(f'{time.ctime()}\r\n'.encode())
            clifd.close()
        elif socktype == socket.SOCK_DGRAM:
            _, cliendpoint = sockfd.recvfrom(4096)
            print(f'recvfrom {cliendpoint[0]}, port {cliendpoint[1]}')
            sockfd.sendto(f'{time.ctime()}\n'.encode(), cliendpoint)
