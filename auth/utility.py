from datetime import datetime, timedelta, timezone
from typing import Tuple
import jwt


def encode_token(
    username: str, expires_delta: timedelta, key: str, algorithm: str = "HS256"
) -> str:
    data = {
        "sub": username,
        "exp": datetime.now(timezone.utc) + expires_delta,
    }
    return jwt.encode(data, key=key, algorithm=algorithm)


def decode_token(
    token: str, key: str, algorithm: str = "HS256"
) -> Tuple[str, datetime]:
    payload = jwt.decode(token, key=key, algorithm=algorithm)
    return payload.get("sub"), payload.get("exp")
