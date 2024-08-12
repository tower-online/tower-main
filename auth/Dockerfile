FROM python:alpine

WORKDIR /app
COPY . .

RUN python3 -m venv venv
RUN venv/bin/pip install --no-cache-dir --upgrade -r requirements.txt
ENV PATH=/app/venv/bin:${PATH}

ENTRYPOINT ["fastapi", "run", "main.py"]