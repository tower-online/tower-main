import asyncio
import os

from connection import Connection, request_token

async def main():
    username = "dummy"

    token = request_token(port=8000, username=username)
    if not token:
        exit(1)

    connection = Connection()
    if not await connection.connect("localhost", 30000):
        exit(1)

    await connection.start(username=username, token=token)

asyncio.run(main())