import asyncio
from asyncio import IncompleteReadError
import requests

import flatbuffers
from flatbuffers.util import GetSizePrefix
from tower.net.packet.PacketBase import *
from tower.net.packet.PacketType import PacketType
from tower.net.packet.ClientJoinRequest import *
from tower.net.packet.ClientPlatform import ClientPlatform


class Connection:
    _reader: asyncio.StreamReader
    _writer: asyncio.StreamWriter

    is_running: bool = False

    async def connect(self, addr: str, port: int) -> bool:
        print(f"Connecting to {addr}:{port}...")

        try:
            self._reader, self._writer = await asyncio.open_connection(
                addr, port
            )
        except ConnectionError:
            print("Error connecting main server")
            return False

        return True

    async def start(self, username: str, token: str):
        print("Starting connection...")

        # Send ClientJoinRequest
        builder = flatbuffers.Builder(256)

        username_flat = builder.CreateString(username)
        token_flat = builder.CreateString(token)

        ClientJoinRequestStart(builder)
        ClientJoinRequestAddPlatform(builder, ClientPlatform.TEST)
        ClientJoinRequestAddUsername(builder, username_flat)
        ClientJoinRequestAddToken(builder, token_flat)
        request = ClientJoinRequestEnd(builder)

        PacketBaseStart(builder)
        PacketBaseAddPacketBaseType(builder, PacketType.ClientJoinRequest)
        PacketBaseAddPacketBase(builder, request)
        packet_base = PacketBaseEnd(builder)

        builder.FinishSizePrefixed(packet_base)

        await self.send_packet(builder.Output())

        self.is_running = True
        while self.is_running:
            await self.receive_packet()

    def stop(self):
        print("Stopping connection...")

        self.is_running = False
        self._writer.close()

    async def receive_packet(self):
        PACKET_SIZE_PREFIX_BYTES = 4

        try:
            header_buffer = await self._reader.readexactly(PACKET_SIZE_PREFIX_BYTES)
        except IncompleteReadError:
            print("Error reading header")
            self.stop()
            return

        length = GetSizePrefix(header_buffer, 0)
        try:
            body_buffer = await self._reader.readexactly(length - PACKET_SIZE_PREFIX_BYTES)
        except IncompleteReadError:
            print("Error reading body")
            self.stop()
            return

        self.handle_packet(body_buffer)

    async def send_packet(self, buffer: bytes):
        self._writer.write(buffer)
        await self._writer.drain()

    def handle_packet(self, buffer: bytes):
        print(f"Handling {len(buffer)} bytes of packet")


def request_token(port: int, username: str) -> str | None:
    url = f"http://localhost:{port}/token/test/?username={username}"
    print(f"Requesting token as username={username} ...")

    try:
        response = requests.post(url)
    except requests.exceptions.ConnectionError:
        print("Error connecting auth server")
        return None

    if response.status_code != 200:
        print(f"POST {url} failed")
        return None

    payload = response.json()
    if "jwt" not in payload:
        print("Invalid response")
        return None

    print("Got token:", payload["jwt"])
    return payload["jwt"]
