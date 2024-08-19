FROM python:alpine

# RUN apk add --no-cache redis

WORKDIR /app
COPY . .

RUN python3 -m venv venv
RUN venv/bin/pip install --no-cache-dir --upgrade -r requirements.txt
ENV PATH=/app/venv/bin:${PATH}

#CMD ["fastapi", "run", "main.py"]
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--ssl-keyfile", "/run/secrets/key.pem", "--ssl-certfile", "/run/secrets/cert.pem"]