#!/usr/bin/env python3

# 等待远程服务上线

import socket
import time


class NetServiceChecker:
    def __init__(self, host, port, timeout=120):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(timeout)

    def check(self):
        while True:
            try:
                self.sock.connect((self.host, self.port))
            except TimeoutError:
                print('Timeout, retry...')
            except OSError as e:
                print(f'Failed: {e}, retry...')
                time.sleep(3)
            else:
                print('Service is avaliable again!')
                self.sock.close()
                break



if __name__ == '__main__':
    import sys
    assert len(sys.argv) == 3
    checker = NetServiceChecker(sys.argv[1], int(sys.argv[2]))
    checker.check()
