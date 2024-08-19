import asyncio
import requests
from logging import Logger

from connection import Connection
from tower.net.packet.PacketBase import *
from tower.net.packet.PacketType import PacketType
from tower.net.packet.ClientJoinRequest import *
from tower.net.packet.ClientJoinResponse import *
from tower.net.packet.ClientJoinResult import ClientJoinResult
from tower.net.packet.HeartBeat import *
from tower.net.packet.ClientPlatform import ClientPlatform

class Client:
    def __init__(self, logger: Logger, username: str):
        self.logger = logger
        self.username = username
        self.connection = Connection(self.handle_packet)

    def run(self):
        self.logger.info(f"[{self.username}] Running...")
        token = self.request_token(port=8000)
        if not token:
            return       

        async def send_token(token: str):
            # Send ClientJoinRequest
            builder = flatbuffers.Builder(256)

            username_flat = builder.CreateString(self.username)
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

            await self.connection.send_packet(builder.Output())


        async def io(token: str):
            self.logger.info(f"[{self.username}] Connecting to {'localhost'}:{30000} ...")
            if not await self.connection.connect("localhost", 30000):
                self.logger.error(f"[{self.username}] Error connecting main server")

            await send_token(token)

            await self.connection.start(username=self.username, token=token)

        asyncio.run(io(token=token))
        self.logger.info(f"[{self.username}] Terminating...")

    def stop(self):
        self.connection.stop()


    def request_token(self, port: int) -> str | None:
        url = f"http://localhost:{port}/token/test/?username={self.username}"

        try:
            response = requests.post(url)
        except requests.exceptions.ConnectionError:
            self.logger.error(f"[{self.username}] Error connecting auth server")
            return None

        if response.status_code != 200:
            self.logger.error(f"[{self.username}] POST {url} failed")
            return None

        payload = response.json()
        if "jwt" not in payload:
            self.logger.error(f"[{self.username}] Invalid response")
            return None

        # self.logger.info(f"[{self.username}] Token: {payload['jwt']}")
        return payload["jwt"]
    
    async def handle_packet(self, buffer: bytes):
        packet_base = PacketBase.GetRootAs(buffer, 0)
        packet_type = packet_base.PacketBaseType()

        if packet_type == PacketType.ClientJoinResponse:
            packet_table = packet_base.PacketBase()
            response = ClientJoinResponse()
            response.Init(packet_table.Bytes, packet_table.Pos)

            if response.Result() != ClientJoinResult.OK:
                self.logger.warn(f"[{self.username}] [ClientJoinResult] Failed")
                self.stop()
                return
            else:
                self.logger.info(f"[{self.username}] [ClientJoinResult] OK")

        elif packet_type == PacketType.HeartBeat:
            builder = flatbuffers.Builder(64)
            HeartBeatStart(builder)
            heart_beat = HeartBeatEnd(builder)

            PacketBaseStart(builder)
            PacketBaseAddPacketBaseType(builder, PacketType.HeartBeat)
            PacketBaseAddPacketBase(builder, heart_beat)
            packet_base = PacketBaseEnd(builder)

            builder.FinishSizePrefixed(packet_base)

            await self.connection.send_packet(builder.Output())