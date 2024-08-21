import argparse
import asyncio
import multiprocessing
import logging
import sys
import urllib3

from client import Client


def make_client(logger: logging.Logger, username: str, mode: str):
    if mode == "dummy":
        client = Client(logger=logger, username=username, strategy=mode)
    elif mode == "fuzz":
        client = Client(logger=logger, username=username, strategy=mode)
    else:
        return
    
    asyncio.run(client.run())


def main():
    parser = argparse.ArgumentParser(
        prog="Dummy Clients",
        description="Spawn dummy client processes and run")
    parser.add_argument("--clients", type=int, default=1)
    parser.add_argument("--debug", type=bool, default=True)
    parser.add_argument("--mode", type=str, choices=("dummy", "fuzz"), default="dummy")
    args = parser.parse_args()

    logger = logging.getLogger(__name__)
    if args.debug:
        logger.setLevel(logging.DEBUG)
    logger.addHandler(logging.StreamHandler(sys.stdout))

    # Disable warning from self-signed certification
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

    processes = []
    for i in range(args.clients):
        username = f"dummy_{str(i+1).rjust(5, '0')}"

        p = multiprocessing.Process(target=make_client, args=(logger, username, args.mode))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()


if __name__ == "__main__":
    main()