import asyncio
from asyncio import IncompleteReadError
from typing import Any, Callable, Coroutine

from flatbuffers.util import GetSizePrefix


class Connection:
    _reader: asyncio.StreamReader
    _writer: asyncio.StreamWriter

    def __init__(self, packet_handler: Callable[[bytes], Coroutine[Any, Any, None]]):
        self.packet_handler = packet_handler
        self.is_running = False

    async def connect(self, addr: str, port: int) -> bool:
        try:
            self._reader, self._writer = await asyncio.open_connection(
                addr, port
            )
        except ConnectionError:
            # logging.error(f"Error connecting main server")
            return False

        return True

    def stop(self):
        # logging.info("Stopping connection...")

        self.is_running = False
        try:
            self._writer.close()
        except Exception:
            pass

    async def receive_packet(self):
        PACKET_SIZE_PREFIX_BYTES = 4

        try:
            header_buffer = await self._reader.readexactly(PACKET_SIZE_PREFIX_BYTES)
        except IncompleteReadError:
            # logging.error("Error reading header")
            self.stop()
            return

        length = GetSizePrefix(header_buffer, 0)
        try:
            body_buffer = await self._reader.readexactly(length)
        except IncompleteReadError:
            # logging.error("Error reading body")
            self.stop()
            return
        
        await self.packet_handler(body_buffer)

    async def send_packet(self, buffer: bytes):
        try:
            self._writer.write(buffer)
            await self._writer.drain()
        except ConnectionError:
            self.stop()
            return
