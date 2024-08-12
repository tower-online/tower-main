#!/bin/sh

if [ "$TOWER_DEBUG" = "true" ]; then
    exec fastapi dev main.py
else
    exec fastapi run main.py
fi