#!/usr/bin/env python3

# require: ntplib

# 获取时间
# test: ./1.11-machine_time.py 1.debian.pool.ntp.org

import ntplib
import time


def print_time(ntp_server):
    ntp_client = ntplib.NTPClient()
    res = ntp_client.request(ntp_server)
    print(time.ctime(res.tx_time))


if __name__ == '__main__':
    import sys
    assert len(sys.argv) == 2
    print_time(sys.argv[1])
