import argparse
import multiprocessing
import logging
import sys

from client import Client


def make_client(logger: logging.Logger, username: str):
    client = Client(logger=logger, username=username)
    client.run()


def main():
    parser = argparse.ArgumentParser(
        prog="Dummy Clients",
        description="Spawn dummy client processes and run")
    parser.add_argument("--clients", type=int, choices=range(1,1001))
    parser.add_argument("--debug", type=bool, default=True)
    args = parser.parse_args()

    logger = logging.getLogger(__name__)
    if args.debug:
        logger.setLevel(logging.DEBUG)
    logger.addHandler(logging.StreamHandler(sys.stdout))

    processes = []
    for i in range(args.clients):
        username = f"dummy_{str(i+1).rjust(len(str(args.clients)), '0')}"

        p = multiprocessing.Process(target=make_client, args=(logger, username))
        processes.append(p)
        p.start()

    for p in processes:
        p.join()


if __name__ == "__main__":
    main()