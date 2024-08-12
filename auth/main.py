from contextlib import asynccontextmanager
from datetime import datetime
from enum import Enum, StrEnum
from fastapi import Depends, FastAPI, HTTPException, Query, status
from functools import lru_cache
from pydantic import BaseModel
from typing import Annotated, Any, Tuple

from psycopg_pool import AsyncConnectionPool

from config import Settings
from utility import *


class Platform(StrEnum):
    TEST = "TEST"
    STEAM = "STEAM"


class TokenRequest(BaseModel):
    platform: Platform
    username: Annotated[str, Query(min_length=6, max_length=30)]
    id: Annotated[str | None, Query(max_length=64)] = None
    key: Annotated[str | None, Query(max_length=64)] = None


class TokenResponse(BaseModel):
    jwt: Annotated[str, Query()]
    expire: Annotated[datetime | None, Query()] = None


class User(BaseModel):
    class Status(StrEnum):
        ACTIVE = "ACTIVE"
        INACTIVE = "INACTIVE"
        BLOCKED = "BLOCKED"

    username: str
    platform: Platform
    status: Status


@asynccontextmanager
async def lifespan(app: FastAPI):
    await pool.open(timeout=5)
    await pool.wait()

    yield

    await pool.close()


app = FastAPI(lifespan=lifespan)
settings = Settings()
pool = AsyncConnectionPool(
    f"postgresql://{settings.db_user}:{settings.db_password}@{settings.db_host}:{settings.db_port}/{settings.db_name}",
    open=False,
)


@app.post("/token", response_model=TokenResponse)
async def get_token(request: TokenRequest) -> Any:
    result, error = await authenticate_user(
        request.platform, request.username, request.key
    )
    if not result:
        raise error

    # TODO: timezone
    if request.platform is Platform.TEST:
        jwt, expire = encode_token(request.username, timedelta(hours=1))
        return TokenResponse(jwt=jwt, expire=expire)
    if request.platform is Platform.STEAM:
        jwt, expire = encode_token(
            request.username, timedelta(hours=settings.token_expire_hours)
        )
        return TokenResponse(jwt=jwt, expire=expire)


async def authenticate_user(
    platform: Platform, username: str, key: str | None
) -> Tuple[bool, HTTPException]:
    user = await get_user(platform, username)

    if not user:
        return False, HTTPException(
            status_code=400,
            detail="Incorrect username or platform",
        )

    if platform is Platform.TEST and not settings.debug:
        return False, HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Test user is disabled",
        )

    if user.status is not User.Status.ACTIVE:
        return False, HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Inactive user",
        )

    if platform is Platform.STEAM:
        url = f"https://partner.steam-api.com/ISteamUserAuth/AuthenticateUserTicket/v1/"

    return True, HTTPException(status_code=status.HTTP_200_OK)


async def get_user(platform: Platform, username: str) -> User | None:
    async with pool.connection() as connection:
        async with connection.cursor() as cursor:
            await cursor.execute(
                "SELECT username, platform, status FROM users WHERE username=%s AND platform=%s",
                (username, platform),
            )

            record = await cursor.fetchone()
            return (
                User(username=record[0], platform=record[1], status=record[2])
                if record
                else None
            )
