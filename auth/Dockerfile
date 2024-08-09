FROM alpine:latest

RUN apk add --no-cache \
    python3 \
    py3-pip \
    py3-virtualenv

WORKDIR /app
COPY . .

RUN mkdir venv && \
    python -m venv venv && \
    source venv/bin/activate && \
    python -m pip install --upgrade pip && \
    pip install fastapi

ENTRYPOINT ["python3 /app/app.py"]