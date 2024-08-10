FROM alpine:latest

RUN apk add --no-cache \
    python3 \
    py3-pip \
    py3-virtualenv

WORKDIR /app
COPY . .

RUN python -m venv venv && \
    source venv/bin/activate && \
    pip install -r requirements.txt

ENTRYPOINT ["fastapi run app.py"]