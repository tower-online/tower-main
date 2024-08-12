from pydantic_settings import BaseSettings
import os


def _read_file(path: str) -> str:
    with open(path, "r") as f:
        return f.read().strip()


class Settings(BaseSettings):
    debug: bool = os.environ["TOWER_DEBUG"] == "true"
    token_expire_hours: int = int(os.environ["TOKEN_EXPIRE_HOURS"])

    db_host: str = os.environ["TOWER_DB_HOST"]
    db_port: str = os.environ["TOWER_DB_PORT"]
    db_user: str = os.environ["TOWER_DB_USER"]
    db_password: str = _read_file(os.environ["TOWER_DB_PASSWORD_FILE"])
    db_name: str = os.environ["TOWER_DB_NAME"]