#!/usr/bin/env python3
"""Print the bin directory of the pinned Node.js of one major, fetching it on first use.

Run:  python3 scripts/node_toolchain.py <major>

node_toolchain.json, next to this script, pins one exact version per major and
the SHA-256 of each platform's official .tar.gz. The tarball is verified before
anything is unpacked, and a mismatch deletes it and exits 1. The tree lands in
$TA_NODE_CACHE, else ~/.cache/ta-lib/node, by a single rename, so a failed run
leaves nothing a later run would take for a cache hit. Only the bin path goes
to stdout.

To bump a pin, copy the sums from that version's SHASUMS256.txt only after its
SHASUMS256.txt.asc verifies against github.com/nodejs/release-keys: the
download is trusted exactly as far as the committed sum is.
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

PIN_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "node_toolchain.json")
DIST_URL = "https://nodejs.org/dist"


def fail(message: str):
    print(f"node_toolchain: {message}", file=sys.stderr)
    sys.exit(1)


def host_platform() -> str:
    os_name = {"Linux": "linux", "Darwin": "darwin"}.get(platform.system())
    arch = {"x86_64": "x64", "amd64": "x64", "aarch64": "arm64", "arm64": "arm64"}.get(
        platform.machine().lower())
    if os_name and arch:
        return f"{os_name}-{arch}"
    return f"{platform.system()} {platform.machine()}"


def usable(tree: str) -> bool:
    node = os.path.join(tree, "bin", "node")
    return os.path.isfile(node) and os.access(node, os.X_OK)


def download(url: str, path: str) -> str:
    digest = hashlib.sha256()
    with urllib.request.urlopen(url, timeout=60) as response, open(path, "wb") as out:
        while chunk := response.read(1 << 20):
            digest.update(chunk)
            out.write(chunk)
    return digest.hexdigest()


def install(version: str, name: str, sha256: str, cache: str, dest: str):
    os.makedirs(cache, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix=f".{name}.", dir=cache)
    try:
        url = f"{DIST_URL}/v{version}/{name}.tar.gz"
        archive = os.path.join(tmp, f"{name}.tar.gz")
        print(f"node_toolchain: downloading {url}", file=sys.stderr)
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
        unpacked = os.path.join(tmp, name)
        if not usable(unpacked):
            fail(f"{url} holds no {name}/bin/node")
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
        fail("usage: node_toolchain.py <major>")
    with open(PIN_FILE, encoding="utf-8") as f:
        pins = json.load(f)["versions"]
    major = sys.argv[1]
    if major not in pins:
        fail(f"no pinned Node {major!r}; pinned majors: {', '.join(sorted(pins))}")
    version = pins[major]["version"]
    sums = pins[major]["tar_gz_sha256"]
    plat = host_platform()
    if plat not in sums:
        fail(f"no pinned Node tarball for {plat}; supported: {', '.join(sorted(sums))}")

    name = f"node-v{version}-{plat}"
    cache = os.path.abspath(os.path.expanduser(
        os.environ.get("TA_NODE_CACHE") or "~/.cache/ta-lib/node"))
    dest = os.path.join(cache, name)
    if not usable(dest):
        try:
            install(version, name, sums[plat], cache, dest)
        except (OSError, tarfile.TarError) as e:
            fail(f"installing Node {version} failed: {e}")
    print(os.path.join(dest, "bin"))


if __name__ == "__main__":
    main()
