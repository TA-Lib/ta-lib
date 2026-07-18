#!/usr/bin/env python3
"""Rust stream debug-build sweep — the Rust sibling of stream_sanitize.py.

Rust needs no ASan for memory safety, but a RELEASE-verified stream can still
hide debug-only panics: usize subtract-with-overflow, `as` conversions on
sentinel arithmetic, slice-bound edge cases (the recorded IMI/APO/PPO
`--rust-debug` underflow class). This gate builds the Rust JSON-RPC server in
DEBUG (overflow checks + debug_asserts ON), pipes the same `stream_verify`
request set stream_sanitize.py uses (defaults / min-period / large-period /
warm-unstable / min-under-K / candle shape / zero-sum shape), and fails on:
  - a server crash or panic (non-zero exit, stderr panic marker),
  - any response with "ok":0 or "peek_ok":0 (bit-exactness must ALSO hold in
    debug — a debug/release numeric divergence would be a real defect),
  - vacuity (no response with a non-zero "legs" count).

Run:  python3 scripts/rust_stream_debug.py
"""

import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from stream_sanitize import load_streamable, requests_for  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORKSPACE = os.path.join(ROOT, "ta_codegen/output/rust")
SERVER = os.path.join(WORKSPACE, "target/debug/ta_codegen_serve")


def build_debug_server():
    print("=== Building Rust server (debug: overflow checks ON) ===")
    subprocess.run(
        ["cargo", "build", "--bin", "ta_codegen_serve"],
        cwd=WORKSPACE,
        check=True,
    )


def main():
    build_debug_server()
    funcs = load_streamable()
    if not funcs:
        print("FAIL: no streamable functions found")
        return 1
    requests = []
    for f in funcs:
        requests.append((f["name"], requests_for(f)))
    payload = "\n".join(r for _, reqs in requests for r in reqs) + "\n"
    n_reqs = sum(len(reqs) for _, reqs in requests)
    print(f"=== Driving {n_reqs} stream_verify requests for {len(funcs)} functions ===")

    # Pipe everything, then close stdin so the server exits cleanly.
    proc = subprocess.run(
        [SERVER],
        input=payload.encode(),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        cwd=WORKSPACE,
    )
    err = proc.stderr.decode(errors="replace")
    if proc.returncode != 0 or "panicked at" in err:
        print(err[-4000:])
        print(f"FAIL: debug server crashed (exit {proc.returncode})")
        return 1

    lines = proc.stdout.decode(errors="replace").strip().splitlines()
    flat = [r for _, reqs in requests for r in reqs]
    if len(lines) != len(flat):
        print(f"FAIL: {len(flat)} requests but {len(lines)} responses")
        return 1

    failures = 0
    legs_total = 0
    not_streamable = 0
    for req, line in zip(flat, lines):
        m = re.search(r'"legs":(\d+)', line)
        if m:
            legs_total += int(m.group(1))
        if '"error":"not_streamable"' in line:
            # A tier not yet emitted for Rust: visible, not fatal here (the
            # ta_regtest SET-MISMATCH gate owns completeness).
            not_streamable += 1
            continue
        if '"ok":0' in line or '"peek_ok":0' in line or '"fill_ok":0' in line:
            print(f"FAIL: {req}\n  -> {line}")
            failures += 1
    if legs_total <= 0:
        print("FAIL: vacuous run (no legs executed)")
        return 1
    print(
        f"=== Rust debug stream sweep: {len(lines)} responses, {legs_total} legs, "
        f"{not_streamable} not-yet-streamable, {failures} failures ==="
    )
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
