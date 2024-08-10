from functools import lru_cache
from datetime import datetime, timedelta, timezone
from typing import Tuple
import jwt

# openssl rand -hex 32
_SECRET_KEY = "6d29e2082cbcba56e2e9189a90fc701bf2cc12c6eab2dda57ef0f94ac2452e4e"
_ALGORITHM = "HS256"


def encode_token(username: str, expires_delta: timedelta = timedelta(hours=1)) -> str:
    data = {
        "sub": username,
        "exp": datetime.now(timezone.utc) + expires_delta,
    }
    return jwt.encode(data, _SECRET_KEY, algorithm=_ALGORITHM)


def decode_token(token: str) -> Tuple[str, datetime]:
    payload = jwt.decode(token, _SECRET_KEY, algorithms=[_ALGORITHM])
    return payload.get("sub"), payload.get("exp")
