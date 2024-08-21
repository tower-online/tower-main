from abc import ABC, abstractmethod
import asyncio
import requests
from logging import Logger
from typing import Any, Callable, Coroutine
from datetime import datetime, timedelta
import random
import os

from connection import Connection
from tower.net.packet.PacketBase import *
from tower.net.packet.PacketType import PacketType
from tower.net.packet.ClientJoinRequest import *
from tower.net.packet.ClientJoinResponse import *
from tower.net.packet.ClientJoinResult import ClientJoinResult
from tower.net.packet.ClientPlatform import ClientPlatform
from tower.net.packet.HeartBeat import *


class UpdateStrategy(ABC):
    tick_interval: timedelta

    def __init__(self):
        self.last_tick = datetime.now()

    @abstractmethod
    async def update(self, client) -> None:
        pass

    @abstractmethod
    async def tick(self, client) -> None:
        pass


class DummyUpdate(UpdateStrategy):
    async def update(self, client) -> None:
        pass

    async def tick(self, client) -> None:
        pass


class FuzzUpdate(UpdateStrategy):
    def __init__(self):
        super().__init__()

    async def update(self, client) -> None:
        self.tick_interval = timedelta(milliseconds=random.randrange(10, 150))
        if datetime.now() < self.last_tick + self.tick_interval:
            return
        
        await self.tick(client)

    async def tick(self, client) -> None:
        for _ in range(random.randrange(1, 10)):
            buffer = os.urandom(random.randrange(1, 65536))
            await client.connection.send_packet(buffer)

            if not client.connection.is_running:
                client.stop()
                return


class PacketHandleStrategy(ABC):
    @abstractmethod
    async def handle_packet(self, buffer: bytes, client) -> None:
        pass


class DummyPacketHandle(PacketHandleStrategy):
    async def handle_packet(self, buffer: bytes, client) -> None:
        #TODO: implement
        pass


class FuzzPacketHandle(PacketHandleStrategy):
    async def handle_packet(self, buffer: bytes, client) -> None:
        # Ignore incoming packets
        pass


class Client:
    def __init__(self, logger: Logger, username: str, strategy: str):
        self.logger = logger
        self.username = username
        self.connection = Connection(self.handle_packet)

        if strategy == "dummy":
            self.packet_handle_strategy = DummyPacketHandle()
            self.update_strategy = DummyUpdate()
        elif strategy == "fuzz":
            self.packet_handle_strategy = FuzzPacketHandle()
            self.update_strategy = FuzzUpdate()
        else:
            raise ValueError("Strategy must be 'dummy' or 'fuzz'")

        self.is_running = False

    async def run(self):
        self.logger.info(f"[{self.username}] Running...")
        self.is_running = True

        token = self.request_token(port=8000)
        if not token:
            self.stop()
            return
        
        self.logger.info(f"[{self.username}] Connecting to {'localhost'}:{30000} ...")
        if not await self.connection.connect("localhost", 30000):
            self.logger.error(f"[{self.username}] Error connecting main server")
            self.stop()
            return

        await self.send_token(token=token)

        async def update_loop():
            while self.is_running:
                await self.update_strategy.update(self)


        async def receive_loop():
            while self.is_running:
                await self.connection.receive_packet()

        await asyncio.gather(
            update_loop(),
            receive_loop()
        )

        self.logger.info(f"[{self.username}] Terminating...")

    def stop(self):
        self.is_running = False
        self.connection.stop()


    def request_token(self, port: int) -> str | None:
        url = f"https://localhost:{port}/token/test"
        json = {
            "username": self.username
        }

        try:
            response = requests.post(url, json=json, verify=False)
        # except requests.exceptions.SS.
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
    
    async def send_token(self, token: str) -> None:
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
        else:
            await self.packet_handle_strategy.handle_packet(buffer, self)
