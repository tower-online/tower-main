from contextlib import asynccontextmanager
from datetime import datetime
from enum import StrEnum
from fastapi import Depends, FastAPI, HTTPException, Query, status
import logging
from pydantic import BaseModel
from typing import Annotated, Any, Tuple, List

import aiomysql
import redis.asyncio as redis

from config import Settings
from utility import *

USERNAME_PATTERN = "^[a-zA-Z0-9_]{6,30}$"


class User(BaseModel):
    class Platform(StrEnum):
        TEST = "TEST"
        STEAM = "STEAM"

    class Status(StrEnum):
        ACTIVE = "ACTIVE"
        INACTIVE = "INACTIVE"
        BLOCKED = "BLOCKED"

    username: Annotated[str, Query()]
    platform: Annotated[Platform, Query()]
    status: Annotated[Status, Query()]


class Characters(BaseModel):
    class Character(BaseModel):
        name: Annotated[str, Query()]
        # level: Annotated[int, Query()]

    characters: Annotated[List[Character], Query()] = []


class TokenRequest(BaseModel):
    username: Annotated[str, Query(pattern=USERNAME_PATTERN)]


class TokenResponse(BaseModel):
    jwt: Annotated[str, Query()]


class RequestBase(BaseModel):
    platform: Annotated[User.Platform, Query()]
    username: Annotated[str, Query(pattern=USERNAME_PATTERN)]
    jwt: Annotated[str, Query()]


@asynccontextmanager
async def lifespan(app: FastAPI):
    global db_pool

    db_pool = await aiomysql.create_pool(
        host=settings.db_host,
        port=settings.db_port,
        user=settings.db_user,
        password=settings.db_password,
        db=settings.db_name,
    )

    yield

    db_pool.close()
    await db_pool.wait_closed()
    await redis_pool.aclose()


app = FastAPI(lifespan=lifespan)
settings = Settings()
db_pool: aiomysql.Pool = None
redis_pool = redis.ConnectionPool.from_url(
    f"redis://:{settings.redis_password}@{settings.redis_host}"
)
logger = logging.getLogger("uvicorn.error")

if settings.debug:
    logger.setLevel(logging.DEBUG)


@app.post("/token/test", response_model=TokenResponse)
async def issue_token_test(request: TokenRequest) -> Any:
    if not settings.debug:
        raise HTTPException(
            status_code=400,
            detail="Testing is disabled",
        )

    jwt = encode_token(
        request.username, User.Platform.TEST, timedelta(hours=1), settings.token_key
    )
    return TokenResponse(jwt=jwt)


@app.post("/token/steam", response_model=TokenResponse)
async def issue_token_steam(username: Annotated[str, Query()]) -> Any:
    # TODO: Validate with Steam Web API

    user = await get_active_user(User.Platform.STEAM, username)

    jwt = encode_token(
        username,
        User.Platform.STEAM,
        timedelta(hours=settings.token_expire_hours),
        settings.token_key,
    )
    return TokenResponse(jwt=jwt)


@app.post("/register/steam")
async def register_steam(
    username: Annotated[str, Query(pattern="^[a-zA-Z0-9_]{6,30}$")]
) -> Any:
    pass


@app.post("/characters", response_model=Characters)
async def request_characters(request: RequestBase) -> Any:
    if not (payload := decode_token(request.jwt, settings.token_key)):
        raise HTTPException(
            status_code=400,
            detail="Invalid token",
        )

    if payload["username"] != request.username:
        raise HTTPException(
            status_code=400,
            detail="Invalid username",
        )

    async with db_pool.acquire() as connection, connection.cursor() as cursor:
        await cursor.execute(
            """SELECT c.name
            FROM characters c
            JOIN users u ON c.user_id = u.id
            WHERE username = %s
            """,
            (request.username,),
        )

        characters = Characters()
        if not (r := await cursor.fetchall()):
            return characters
        for (character_name,) in r:
            characters.characters.append(Characters.Character(name=character_name))

        return characters


async def get_active_user(platform: User.Platform, username: str) -> User:
    if not (user := await get_user(platform, username)):
        raise HTTPException(
            status_code=400,
            detail="Incorrect username or platform",
        )

    if user.status != User.Status.ACTIVE:
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Inactive user",
        )

    return user


async def get_user(platform: User.Platform, username: str) -> User | None:
    async with db_pool.acquire() as connection, connection.cursor() as cursor:
        await cursor.execute(
            "SELECT status FROM users WHERE username=%s AND platform=%s",
            (username, platform),
        )

        if r := await cursor.fetchone():
            (status,) = r
        else:
            return None

        return User(
            username=username,
            platform=platform,
            status=status,
        )
