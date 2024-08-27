from datetime import datetime, timedelta, timezone
from typing import Tuple
import jwt


def encode_token(
    username: str,
    platform: str,
    expires_delta: timedelta,
    key: str,
    algorithm: str = "HS256",
) -> str:
    data = {
        "username": username,
        "platform": platform,
        "exp": datetime.now(timezone.utc) + expires_delta,
    }
    return jwt.encode(data, key=key, algorithm=algorithm)


def decode_token(token: str, key: str, algorithm: str = "HS256") -> dict | None:
    payload = {}
    try:
        payload = jwt.decode(token, key=key, algorithms=[algorithm])
    except jwt.InvalidTokenError:
        print("Invalid token")

    return payload
