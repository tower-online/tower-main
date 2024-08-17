from tower.net.packet import *
import flatbuffers
import requests
import os

def get_token() -> str | None:
    url = f"http://localhost:{8000}?username=dummy"

    try:
        response = requests.post(url)
    except requests.exceptions.ConnectionError:
        print("Error connecting auth server")
        return None

    if response.status_code != 200:
        print(f"POST {url} failed")
        return None

    payload = response.json()
    if "jwt" not in payload:
        print("Invalid response")
        return None

    print("token: ", payload["jwt"])
    return payload["jwt"]

token = get_token()
if not token:
    exit(1)
