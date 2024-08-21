FROM ubuntu:24.04

RUN apt update -y && \
    apt install -y \
    python3 \
    python3-pip \
    python3-venv \
    libmariadb-dev

WORKDIR /app
COPY . .

RUN python3 -m venv venv
RUN venv/bin/pip install --no-cache-dir --upgrade -r requirements.txt
ENV PATH=/app/venv/bin:${PATH}

#CMD ["fastapi", "run", "main.py"]
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--ssl-keyfile", "/run/secrets/key.pem", "--ssl-certfile", "/run/secrets/cert.pem"]