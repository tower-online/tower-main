#!/bin/sh

mkdir -p /app/data

if [ -f /app/.env ];
then
    source /app/.env
else
    ".env file not found"
fi

INIT_FLAG="/app/data/.init_flag"

if [ ! -f "$INIT_FLAG" ];
then
    echo "Running initialization..."

    psql -U "$TOWER_DB_USER" -W "$TOWER_DB_PASSWORD" < /app/config/users.sql

    touch "$INIT_FLAG"

    echo "Initialization complete."
else
    echo "Already initialized. Skipping."
fi

exec "$@"