#!/usr/bin/env python3

# 测试socketpair

import os
import socket


def test_socketpair():
    parent, child = socket.socketpair()
    if os.fork() == 0:
        print(f'@Child, waiting message from parent')
        parent.close()
        msg = child.recv(4096)
        print(f'@Child, receive message from parent: {msg.decode()}')
        child.sendall(b'Hello from child')
        child.close()
    else:
        print(f'@Parent, sending message...')
        child.close()
        parent.sendall(b'Hello from parent')
        msg = parent.recv(4096)
        print(f'@Parent, receive message from child: {msg.decode()}')
        parent.close()


if __name__ == '__main__':
    test_socketpair()
