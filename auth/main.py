from contextlib import asynccontextmanager
from datetime import datetime
from enum import StrEnum
from fastapi import Depends, FastAPI, HTTPException, Query, status
from pydantic import BaseModel
from typing import Annotated, Any, Tuple

import psycopg_pool as pyscorg
import redis.asyncio as redis

from config import Settings
from utility import *


class User(BaseModel):
    class Platform(StrEnum):
        TEST = "TEST"
        STEAM = "STEAM"

    class Status(StrEnum):
        ACTIVE = "ACTIVE"
        INACTIVE = "INACTIVE"
        BLOCKED = "BLOCKED"

    def __init__(self, username: str, platform: Platform, status: Status):
        self.username = username
        self.platform = platform
        self.status = status


class TokenRequest(BaseModel):
    username: Annotated[str, Query(pattern="^[a-zA-Z0-9_]{6,30}$")]

class TokenResponse(BaseModel):
    jwt: Annotated[str, Query()]


@asynccontextmanager
async def lifespan(app: FastAPI):
    await db_pool.open(timeout=5)
    await db_pool.wait()

    yield

    await db_pool.close()
    await redis_pool.aclose()


app = FastAPI(lifespan=lifespan)
settings = Settings()
db_pool = pyscorg.AsyncConnectionPool(
    f"postgresql://{settings.db_user}:{settings.db_password}@{settings.db_host}:{settings.db_port}/{settings.db_name}",
    open=False,
)
redis_pool = redis.ConnectionPool.from_url(
    f"redis://:{settings.redis_password}@{settings.redis_host}"
)


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

    user, error = await get_active_user(User.Platform.STEAM, username)
    if not user:
        raise error

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


async def get_active_user(
    platform: User.Platform, username: str
) -> Tuple[User | None, HTTPException]:
    user = await get_user(platform, username)

    if not user:
        return None, HTTPException(
            status_code=400,
            detail="Incorrect username or platform",
        )

    if user.status is not User.Status.ACTIVE:
        return None, HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Inactive user",
        )

    return None, HTTPException(status_code=status.HTTP_200_OK)


async def get_user(platform: User.Platform, username: str) -> User | None:
    async with db_pool.connection() as connection:
        async with connection.cursor() as cursor:
            await cursor.execute(
                "SELECT username, platform, status FROM users WHERE username=%s AND platform=%s",
                (username, platform),
            )

            record = await cursor.fetchone()
            return (
                User(
                    username=record["username"],
                    platform=record["platform"],
                    status=record["status"],
                )
                if record
                else None
            )
