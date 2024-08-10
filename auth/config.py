from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    debug: bool
    token_expire_hours: int
    db_port: int

    model_config = SettingsConfigDict(env_file=".env")
