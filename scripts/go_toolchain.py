#!/usr/bin/env python3
"""Print the bin directory of the pinned Go toolchain of one key, fetching it on first use.

Run:  python3 scripts/go_toolchain.py <key>

go_toolchain.json, next to this script, pins one exact version per key and the
SHA-256 of each platform's official .tar.gz. The tarball is verified before
anything is unpacked, and a mismatch deletes it and exits 1. The tree lands in
$TA_GO_CACHE, else ~/.cache/ta-lib/go, by a single rename, so a failed run
leaves nothing a later run would take for a cache hit. Only the bin path goes
to stdout.

To bump a pin, copy the sums from https://go.dev/dl/?mode=json&include=all only:
the download is trusted exactly as far as the committed sum is.
"""

import hashlib
import json
import os
import platform
import shutil
import sys
import tarfile
import tempfile
import urllib.request

PIN_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "go_toolchain.json")
DIST_URL = "https://go.dev/dl"


def fail(message: str):
    print(f"go_toolchain: {message}", file=sys.stderr)
    sys.exit(1)


def host_platform() -> str:
    os_name = {"Linux": "linux", "Darwin": "darwin"}.get(platform.system())
    arch = {"x86_64": "amd64", "amd64": "amd64", "aarch64": "arm64", "arm64": "arm64"}.get(
        platform.machine().lower())
    if os_name and arch:
        return f"{os_name}-{arch}"
    return f"{platform.system()} {platform.machine()}"


def usable(tree: str) -> bool:
    go = os.path.join(tree, "bin", "go")
    return os.path.isfile(go) and os.access(go, os.X_OK)


def download(url: str, path: str) -> str:
    digest = hashlib.sha256()
    with urllib.request.urlopen(url, timeout=60) as response, open(path, "wb") as out:
        while chunk := response.read(1 << 20):
            digest.update(chunk)
            out.write(chunk)
    return digest.hexdigest()


def install(name: str, sha256: str, cache: str, dest: str):
    os.makedirs(cache, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix=f".{name}.", dir=cache)
    try:
        url = f"{DIST_URL}/{name}.tar.gz"
        archive = os.path.join(tmp, f"{name}.tar.gz")
        print(f"go_toolchain: downloading {url}", file=sys.stderr)
        actual = download(url, archive)
        if actual != sha256:
            fail(f"SHA-256 mismatch for {url}\n  expected {sha256}\n  actual   {actual}\n"
                 f"The download was deleted.")
        with tarfile.open(archive, "r:gz") as tar:
            if hasattr(tarfile, "data_filter"):
                tar.extractall(tmp, filter="data")
            else:
                tar.extractall(tmp)
        os.remove(archive)
        unpacked = os.path.join(tmp, "go")
        if not usable(unpacked):
            fail(f"{url} holds no go/bin/go")
        if usable(dest):
            return
        if os.path.lexists(dest):
            shutil.rmtree(dest)
        try:
            os.rename(unpacked, dest)
        except OSError:
            if not usable(dest):
                raise
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    if len(sys.argv) != 2:
        fail("usage: go_toolchain.py <key>")
    with open(PIN_FILE, encoding="utf-8") as f:
        pins = json.load(f)["versions"]
    key = sys.argv[1]
    if key not in pins:
        fail(f"no pinned Go {key!r}; pinned keys: {', '.join(sorted(pins))}")
    version = pins[key]["version"]
    sums = pins[key]["tar_gz_sha256"]
    plat = host_platform()
    if plat not in sums:
        fail(f"no pinned Go tarball for {plat}; supported: {', '.join(sorted(sums))}")

    name = f"go{version}.{plat}"
    cache = os.path.abspath(os.path.expanduser(
        os.environ.get("TA_GO_CACHE") or "~/.cache/ta-lib/go"))
    dest = os.path.join(cache, name)
    if not usable(dest):
        try:
            install(name, sums[plat], cache, dest)
        except (OSError, tarfile.TarError) as e:
            fail(f"installing Go {version} failed: {e}")
    print(os.path.join(dest, "bin"))


if __name__ == "__main__":
    main()
