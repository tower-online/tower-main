import jwt
from datetime import datetime, timezone, timedelta


def encode(key: str, algorithm: str, platform: str, username: str, expire_hours: int) -> str:
    payload = {
        "platform": platform,
        "username": username,
        "exp": datetime.now(timezone.utc) + timedelta(hours=expire_hours)
    }
    return jwt.encode(payload, key=key, algorithm=algorithm)


def decode(token: str, key: str, algorithm: str) -> dict:
    return jwt.decode(token, key=key, algorithms=[algorithm])


def authenticate(token: str, key: str, algorithm: str, platform_expected: str, username_expected: str) -> bool:
    try:
        payload = jwt.decode(token, key=key, algorithms=[algorithm])
        if "platform" not in payload or "username" not in payload or "exp" not in payload:
            return False

        if payload["platform"] != platform_expected:
            return False
        if payload["username"] != username_expected:
            return False
    except Exception:
        return False

    return True
