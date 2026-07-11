#!/usr/bin/env python3
"""Run the generated C streaming API under AddressSanitizer/UBSan/LeakSanitizer.

The batch tier is already exercised under sanitizers by the ASan/UBSan nightly
(`ta_regtest` C-reference + boundary sweep), but that never calls the STREAM
functions (Open/Update/Peek/Close, rings, sub-handles). This drives them:

  1. Build the C JSON-RPC server as ONE translation unit with
     `-fsanitize=address,undefined` (address bundles LeakSanitizer on Linux).
  2. Feed it `stream_verify` requests for every stream-flagged function (defaults,
     the minimum period — the smallest ring — and, for recursive functions, an
     unstable-period leg), built from the input YAML metadata.
  3. Close stdin so the server hits EOF and exits its read loop CLEANLY — this is
     what lets LeakSanitizer run at process exit (driving it through ta_regtest
     kills the server, so LSan stays silent). ASan/UBSan errors abort mid-run.
  4. Fail if the server's stderr shows any sanitizer diagnostic, if it exits
     non-zero, or if no leg actually ran (vacuous).

This is the "sanitizer legs" follow-up for the stream servers. It only detects
issues on paths that actually execute — allocation-failure branches (which
require malloc to fail) are guarded instead by generate-time generator tests.
"""

import json
import os
import subprocess
import sys
import glob

import yaml

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SERVER_SRC = os.path.join(ROOT, "ta_codegen/output/c/ta_codegen_serve.c")
SERVER_BIN = os.path.join(ROOT, "cmake-build/bin/ta_codegen_serve_c_asan")

INCLUDE_DIRS = [
    "ta_codegen/output/c", "src/ta_abstract", "src/ta_abstract/frames",
    "include", "src", "src/ta_func", "src/ta_common",
    "ta_codegen/generator/templates/c", "src/tools/ta_regtest",
]


def build_server():
    os.makedirs(os.path.dirname(SERVER_BIN), exist_ok=True)
    cmd = ["gcc", "-o", SERVER_BIN, SERVER_SRC]
    cmd += [f"-I{os.path.join(ROOT, d)}" for d in INCLUDE_DIRS]
    cmd += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-Wno-parentheses-equality", "-lm"]
    print("Building sanitized C stream server (single TU, -fsanitize=address,undefined)...")
    subprocess.run(cmd, check=True, cwd=ROOT)
    print(f"  built {SERVER_BIN}")


def opt_value(oi, which):
    """`which` in {'default','min'}; return the value in the param's type.

    Falls back to the default when the range bound is a non-numeric sentinel
    (`TA_REAL_MIN`, `TA_INTEGER_MIN`, ...) — those are unbounded markers, not
    values to feed a stream open.
    """
    typ = oi.get("type", "integer")
    is_int = typ.startswith("enum") or typ == "integer"
    cast = int if is_int else float
    default = cast(oi.get("default", 0))
    if which == "min":
        rng = oi.get("range")
        if rng:
            try:
                return cast(rng[0])
            except (TypeError, ValueError):
                return default
    return default


def requests_for(func):
    """stream_verify requests for one streamable function definition."""
    name = func["name"]
    opts = func.get("optional_inputs") or []
    unstable = "unstable_period" in (func.get("flags") or [])
    reqs = []

    def build(which, seed, unst):
        params = {
            "funcName": f"TA_{name}", "gen_shape": 0, "gen_seed": seed,
            "gen_n": 320, "unstablePeriod": unst, "compatibility": 0,
        }
        for oi in opts:
            params[oi["name"]] = opt_value(oi, which)
        return json.dumps({"method": "stream_verify", "params": params},
                          separators=(",", ":"))

    reqs.append(build("default", 101, 0))   # defaults
    if opts:
        reqs.append(build("min", 202, 0))   # minimum period = smallest ring
    if unstable:
        reqs.append(build("default", 303, 5))  # a warm unstable-period leg
    return reqs


def load_streamable():
    funcs = []
    for path in sorted(glob.glob(os.path.join(ROOT, "ta_codegen/input/*/*.yaml"))):
        try:
            with open(path) as f:
                d = yaml.safe_load(f)
        except Exception:
            continue
        if not isinstance(d, dict) or "name" not in d:
            continue
        if "stream" in (d.get("flags") or []):
            funcs.append(d)
    return funcs


def main():
    build_server()
    funcs = load_streamable()
    reqs = [r for func in funcs for r in requests_for(func)]
    print(f"Driving {len(funcs)} stream-flagged functions with {len(reqs)} stream_verify legs...")

    env = dict(os.environ)
    env["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=1"
    env["UBSAN_OPTIONS"] = "print_stacktrace=1:halt_on_error=1"
    # Clean EOF -> the server exits its read loop -> LeakSanitizer runs at exit.
    proc = subprocess.run([SERVER_BIN], input=("\n".join(reqs) + "\n").encode(),
                          capture_output=True, cwd=ROOT, env=env)

    stderr = proc.stderr.decode(errors="replace")
    markers = ("ERROR: AddressSanitizer", "ERROR: LeakSanitizer",
               "runtime error:", "detected memory leaks", "SUMMARY: ")
    hits = [ln for ln in stderr.splitlines() if any(m in ln for m in markers)]

    # Non-vacuity: at least some legs must have actually verified.
    verified = sum(1 for ln in proc.stdout.decode(errors="replace").splitlines()
                   if '"legs":' in ln and '"legs":0' not in ln)

    print(f"server exit={proc.returncode}  legs-verified(non-zero)={verified}")
    if hits:
        print("\n!!! SANITIZER DIAGNOSTIC(S):")
        print(stderr)
        return 1
    if proc.returncode != 0:
        print("\n!!! server exited non-zero under sanitizers:")
        print(stderr[-4000:])
        return 1
    if verified == 0:
        print("\n!!! vacuous: no stream leg verified — broken server or filter?")
        return 1
    print("PASS — C stream API is ASan/UBSan/LSan clean on all exercised paths.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
