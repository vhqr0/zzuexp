#!/usr/bin/env python3

# 端口转发
# 用 asyncio 实现，原书使用的 asyncore 已被废弃

import asyncio


class PortForwarder:

    def __init__(self, in_host, in_port, out_host, out_port):
        self.in_host = in_host
        self.in_port = in_port
        self.out_host = out_host
        self.out_port = out_port

    def run(self):
        asyncio.run(self.start_server())

    async def start_server(self):
        server = await asyncio.start_server(self.open_connection,
                                            self.in_host,
                                            self.in_port,
                                            reuse_address=True)
        addrs = ', '.join(str(sock.getsockname()) for sock in server.sockets)
        print(f'Serving on {addrs}')
        async with server:
            await server.serve_forever()

    async def open_connection(self, in_reader, in_writer):
        peername = in_writer.get_extra_info('peername')
        print(f'Accept from {peername}')
        out_reader, out_writer = await asyncio.open_connection(
            self.out_host, self.out_port)
        await asyncio.gather(self.forward(in_reader, out_writer),
                             self.forward(out_reader, in_writer))

    async def forward(self, reader, writer):
        while True:
            buf = await reader.read(4096)
            if len(buf) == 0:
                writer.close()
                await writer.wait_closed()
                break
            else:
                writer.write(buf)
                await writer.drain()


def main():
    import argparse
    from urllib.parse import urlparse

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--inbound')
    parser.add_argument('-o', '--outbound')
    args = parser.parse_args()
    iu = urlparse('//' + (args.inbound or ''))
    ou = urlparse('//' + (args.outbound or ''))
    in_host = iu.hostname or 'localhost'
    in_port = iu.port or 8080
    out_host = ou.hostname or 'localhost'
    out_port = ou.port or 80

    forwarder = PortForwarder(in_host, in_port, out_host, out_port)
    forwarder.run()


if __name__ == '__main__':
    main()
