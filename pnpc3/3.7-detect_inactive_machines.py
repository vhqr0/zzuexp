#!/usr/bin/env python3

# require: scapy

# 监测未开启设备

import time
import sched
import scapy.all as sp

scheduler = sched.scheduler(time.time, time.sleep)


def detect_inactive_hosts(hosts):
    scheduler.enter(10, 1, detect_inactive_hosts, (hosts, ))
    inactive_hosts = []
    ans, unans = sp.sr(sp.IP(dst=hosts) / sp.ICMP() / b'xxxx', timeout=1)
    ans.summary()
    for inactive in unans:
        print(f'{inactive.dst} is inactive')
        inactive_hosts.append(inactive.dst)
    print(f'Total {len(inactive_hosts)} hosts are inactive')


if __name__ == '__main__':
    import sys
    scheduler.enter(1, 1, detect_inactive_hosts, (sys.argv[1:], ))
    scheduler.run()
