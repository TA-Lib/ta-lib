#!/usr/bin/env python3
"""Print the path of the pinned Binaryen wasm-opt, fetching it on first use.

Run:  python3 scripts/binaryen_toolchain.py

binaryen_toolchain.json, next to this script, pins one Binaryen release and the
SHA-256 of each platform's official .tar.gz. The tarball is verified before
anything is unpacked, and a mismatch deletes it and exits 1. Only wasm-opt and
the shared library it loads on macOS are kept. The tree lands in
$TA_BINARYEN_CACHE, else ~/.cache/ta-lib/binaryen, by a single rename, so a
failed run leaves nothing a later run would take for a cache hit; after that
no network is needed. Only the wasm-opt path goes to stdout.

To bump the pin, copy each platform's sum from the release's .sha256 asset:
Binaryen signs nothing, so the download is trusted exactly as far as the
committed sum is.
"""

import json
import os
import platform
import shutil
import sys
import tarfile
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from node_toolchain import download

PIN_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "binaryen_toolchain.json")
RELEASE_URL = "https://github.com/WebAssembly/binaryen/releases/download"


class ToolchainError(Exception):
    pass


def host_platform() -> str:
    os_name = {"Linux": "linux", "Darwin": "macos", "Windows": "windows"}.get(platform.system())
    arch = {"x86_64": "x86_64", "amd64": "x86_64"}.get(platform.machine().lower())
    if arch is None and platform.machine().lower() in ("aarch64", "arm64"):
        arch = "aarch64" if os_name == "linux" else "arm64"
    if os_name and arch:
        return f"{arch}-{os_name}"
    return f"{platform.system()} {platform.machine()}"


def executable(tree: str) -> str:
    return os.path.join(tree, "bin", "wasm-opt.exe" if os.name == "nt" else "wasm-opt")


def usable(tree: str) -> bool:
    exe = executable(tree)
    return os.path.isfile(exe) and os.access(exe, os.X_OK)


def kept(member: tarfile.TarInfo, top: str) -> bool:
    rel = member.name.removeprefix(top + "/")
    if rel in ("bin/wasm-opt", "bin/wasm-opt.exe"):
        return True
    return rel.startswith("lib/") and rel.endswith((".dylib", ".so", ".dll"))


def install(version: str, plat: str, sha256: str, cache: str, dest: str):
    os.makedirs(cache, exist_ok=True)
    top = f"binaryen-version_{version}"
    tmp = tempfile.mkdtemp(prefix=f".{top}.", dir=cache)
    try:
        url = f"{RELEASE_URL}/version_{version}/{top}-{plat}.tar.gz"
        archive = os.path.join(tmp, "binaryen.tar.gz")
        print(f"binaryen_toolchain: downloading {url}", file=sys.stderr)
        actual = download(url, archive)
        if actual != sha256:
            raise ToolchainError(f"SHA-256 mismatch for {url}\n  expected {sha256}\n"
                                 f"  actual   {actual}\nThe download was deleted.")
        with tarfile.open(archive, "r:gz") as tar:
            members = [m for m in tar.getmembers() if m.isfile() and kept(m, top)]
            if hasattr(tarfile, "data_filter"):
                tar.extractall(tmp, members=members, filter="data")
            else:
                tar.extractall(tmp, members=members)
        os.remove(archive)
        unpacked = os.path.join(tmp, top)
        if not usable(unpacked):
            raise ToolchainError(f"{url} holds no {top}/bin/wasm-opt")
        if os.path.lexists(dest):
            shutil.rmtree(dest)
        try:
            os.rename(unpacked, dest)
        except OSError:
            if not usable(dest):
                raise
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def wasm_opt() -> str:
    """The pinned wasm-opt, installed into the cache when missing. Raises ToolchainError."""
    with open(PIN_FILE, encoding="utf-8") as f:
        pin = json.load(f)
    version, sums = pin["version"], pin["tar_gz_sha256"]
    plat = host_platform()
    if plat not in sums:
        raise ToolchainError(f"no pinned Binaryen tarball for {plat}; "
                             f"supported: {', '.join(sorted(sums))}")
    cache = os.path.abspath(os.path.expanduser(
        os.environ.get("TA_BINARYEN_CACHE") or "~/.cache/ta-lib/binaryen"))
    dest = os.path.join(cache, f"binaryen-version_{version}-{plat}")
    if not usable(dest):
        try:
            install(version, plat, sums[plat], cache, dest)
        except (OSError, tarfile.TarError) as e:
            raise ToolchainError(f"installing Binaryen {version} failed: {e}") from e
    return executable(dest)


def main():
    if len(sys.argv) != 1:
        print("binaryen_toolchain: usage: binaryen_toolchain.py", file=sys.stderr)
        sys.exit(1)
    try:
        print(wasm_opt())
    except ToolchainError as e:
        print(f"binaryen_toolchain: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
