import argparse
import subprocess
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-o", type=str, default=".", help="Output path")
    parser.add_argument("--db", required=True, help="DB password")
    parser.add_argument("--redis", required=True, help="Redis password")
    parser.add_argument("--jwt", type=str, help="JWT key; Default is random 32-length hex")
    parser.add_argument("--ssl", action="store_true", help="Generate self-signed SSL certifications")
    args = parser.parse_args()

    try:
        Path(args.o).resolve(strict=True)
    except FileNotFoundError:
        pass
    except RuntimeError:
        print("Invalid output path")
        exit(1)
    Path(args.o).mkdir(parents=True, exist_ok=True)

    with open(f"{args.o}/db-password", "w+") as f:
        f.write(args.db)
        print(f"db-password: {args.db}")

    with open(f"{args.o}/redis-password", "w+") as f:
        f.write(args.redis)
        print(f"redis-password: {args.redis}")

    with open(f"{args.o}/auth-jwt-key", "w+") as f:
        if args.jwt is not None:
            f.write(args.jwt)
            print(f"auth-jwt-key: {args.jwt}")
        else:
            p = subprocess.run(["openssl", "rand", "-hex", "32"], capture_output=True, text=True)
            f.write(p.stdout)
            print(f"auth-jwt-key: {p.stdout.rstrip()}")

    if args.ssl:
        subprocess.call(["openssl", "req", "-x509", "-newkey", "rsa:4096", "-keyout", f"{args.o}/key.pem",
                         "-out", f"{args.o}/cert.pem", "-sha256", "-days", "3650", "-nodes", "-subj", "/CN=localhost"])
        print(f"{args.o}/key.pem")
        print(f"{args.o}/cert.pem")
