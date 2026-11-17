#!/usr/bin/env python3
"""Run the generated C streaming API under AddressSanitizer/UBSan/LeakSanitizer.

The batch tier is already exercised under sanitizers by the ASan/UBSan nightly
(`ta_regtest` C-reference + boundary sweep), but that never calls the STREAM
functions (Open/Update/Peek/Close, rings, sub-handles). This drives them:

  1. Build the C JSON-RPC server as ONE translation unit with
     `-fsanitize=address,undefined,float-cast-overflow` (address bundles LeakSanitizer on Linux).
  2. Feed it `stream_verify` requests for every stream-flagged function (defaults,
     the minimum period — the smallest ring — a large period, and, for recursive
     functions, an unstable-period leg), built from the input YAML metadata.
  3. Close stdin so the server hits EOF and exits its read loop CLEANLY — this is
     what lets LeakSanitizer run at process exit (driving it through ta_regtest
     kills the server, so LSan stays silent). ASan/UBSan errors abort mid-run.
  4. Fail if the server's stderr shows any sanitizer diagnostic, if it exits
     non-zero, if a request went unanswered, or if a request compared nothing.
  5. Fail on the VALUE signal the responses already carry (`ok` / `peek_ok` /
     `fill_ok`): a leg whose stream disagrees with batch must not read as clean.

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
SERVER_SRC = os.path.join(ROOT, "ta_codegen/output/c/tools/ta_codegen_serve.c")
SERVER_BIN = os.path.join(ROOT, "cmake-build/bin/ta_codegen_serve_c_asan")

INCLUDE_DIRS = [
    "ta_codegen/output/c/tools", "src/ta_abstract", "src/ta_abstract/frames",
    "include", "src", "src/ta_func", "src/ta_common",
    "ta_codegen/generator/templates/c", "src/tools/ta_regtest",
]


def build_server():
    os.makedirs(os.path.dirname(SERVER_BIN), exist_ok=True)
    cmd = ["gcc", "-o", SERVER_BIN, SERVER_SRC]
    cmd += [f"-I{os.path.join(ROOT, d)}" for d in INCLUDE_DIRS]
    cmd += ["-O1", "-g", "-fsanitize=address,undefined,float-cast-overflow", "-fno-omit-frame-pointer",
            "-ffp-contract=off", "-fno-math-errno",
            "-Wno-parentheses-equality", "-lm"]
    print("Building sanitized C stream server (single TU, -fsanitize=address,undefined,float-cast-overflow)...")
    subprocess.run(cmd, check=True, cwd=ROOT)
    print(f"  built {SERVER_BIN}")


# What the large leg adds to each integer default. Odd: a parity flip against the
# (often even) default, so a parity-branched dual mode (TRIMA) sanitizes its odd arm.
LARGE_STEP = 41
# The functions whose large leg compares nothing at LARGE_STEP, and the step that
# does. A new function in that position fails the run until it has a row here.
LARGE_STEP_FOR = {
    "FRAMA": 40,  # refuses an odd period
    "T3": 21,     # the lookback must leave bars on the server's series
}


def opt_value(oi, which, large_step):
    """`which` in {'default','min','large'}; return the value in the param's type.

    Falls back to the default when the range bound is a non-numeric sentinel
    (`TA_REAL_MIN`, `TA_INTEGER_MIN`, ...) — those are unbounded markers, not
    values to feed a stream open. `'large'` is default+large_step (clamped to
    the range) for a plain integer param, exercising a big ring/window under
    the sanitizers; enum/real params keep their default there.
    """
    typ = oi.get("type", "integer")
    is_int = typ.startswith("enum") or typ == "integer"
    cast = int if is_int else float
    default = cast(oi.get("default", 0))
    rng = oi.get("range")
    if which == "min" and rng:
        try:
            return cast(rng[0])
        except (TypeError, ValueError):
            return default
    if which == "large" and typ == "integer":
        big = int(default) + large_step
        if rng:
            try:
                big = min(big, int(rng[1]))
            except (TypeError, ValueError):
                pass
        return big
    return default


def requests_for(func):
    """stream_verify requests for one streamable function definition."""
    name = func["name"]
    large_step = LARGE_STEP_FOR.get(name, LARGE_STEP)
    opts = func.get("optional_inputs") or []
    unstable = "unstable_period" in (func.get("flags") or [])
    reqs = []

    def build(which, seed, unst, shape=0):
        params = {
            "funcName": f"TA_{name}", "gen_shape": shape, "gen_seed": seed,
            "gen_n": 320, "unstablePeriod": unst,
        }
        for oi in opts:
            params[oi["name"]] = opt_value(oi, which, large_step)
        return json.dumps({"method": "stream_verify", "params": params},
                          separators=(",", ":"))

    has_int = any(oi.get("type", "integer") == "integer" for oi in opts)
    reqs.append(build("default", 101, 0))   # defaults
    if opts:
        reqs.append(build("min", 202, 0))   # minimum period = smallest ring
    if has_int:
        # A large period => big ring/window (wraparound), and for a fast-path-skip
        # function (MIDPRICE) a period above its perf threshold — under the sanitizers.
        reqs.append(build("large", 505, 0))
    if unstable:
        reqs.append(build("default", 303, 5))  # a warm unstable-period leg
        if opts:
            # Minimum period UNDER a warm unstable period: a dual-mode function
            # (DI/DM) runs its degenerate arm here — which ignores K while the
            # general arm honors it — so the sanitizers cover that arm at K>0.
            reqs.append(build("min", 404, 5))
    if "candlestick" in (func.get("flags") or []):
        # FUZZ_CANDLE (shape 7 in fuzz_data.h): pattern-rich inside-bar data so a
        # candlestick stream's ring/state/confirmation paths run under ASan/UBSan/
        # LSan on FIRING patterns, not just the all-zero no-pattern path.
        reqs.append(build("default", 606, 0, shape=7))
    if name == "ACCBANDS":
        # FUZZ_ZEROSUM (shape 8): high+low==0 bars exercise the degenerate else
        # branch of the fused 3-sum ring under ASan/UBSan/LSan (its ring read/
        # recompute path on the divide-avoiding arm).
        reqs.append(build("default", 707, 0, shape=8))
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


def read_responses(stdout):
    """Parse the server's stream_verify responses.

    Returns `(legs, value_fails)`: each response's leg count in request order
    (0 for one that compared nothing or could not be read), and a list of
    human-readable failures.
    """
    legs = []
    fails = []
    for ln in stdout.splitlines():
        ln = ln.strip()
        if not ln:
            continue
        legs.append(0)
        try:
            r = json.loads(ln)
        except ValueError:
            fails.append("unparseable response: %s" % ln[:200])
            continue
        if "error" in r:
            # Every request is built from a `stream` YAML flag, so the server
            # answering "not_streamable" is a set mismatch, not a skip.
            fails.append("server error: %s  (%s)" % (r["error"], ln[:160]))
            continue
        legs[-1] = r.get("legs", 0)
        for flag in ("ok", "peek_ok"):
            if r.get(flag, 1) != 1:
                fails.append("%s=0 in %s" % (flag, ln[:200]))
        if r.get("fill_checked") == 1 and r.get("fill_ok") != 1:
            fails.append("fill_ok=0 in %s" % ln[:200])
    return legs, fails


def main():
    build_server()
    funcs = load_streamable()
    reqs = [r for func in funcs for r in requests_for(func)]
    print(f"Driving {len(funcs)} stream-flagged functions with {len(reqs)} stream_verify legs...")

    # Apple clang has no LeakSanitizer, and asking for it aborts the server at startup.
    leaks = sys.platform.startswith("linux")
    env = dict(os.environ)
    env["ASAN_OPTIONS"] = "detect_leaks=%d:halt_on_error=1:abort_on_error=1" % leaks
    env["UBSAN_OPTIONS"] = "print_stacktrace=1:halt_on_error=1"
    # Clean EOF -> the server exits its read loop -> LeakSanitizer runs at exit.
    proc = subprocess.run([SERVER_BIN], input=("\n".join(reqs) + "\n").encode(),
                          capture_output=True, cwd=ROOT, env=env)

    stderr = proc.stderr.decode(errors="replace")
    markers = ("ERROR: AddressSanitizer", "ERROR: LeakSanitizer",
               "runtime error:", "detected memory leaks", "SUMMARY: ")
    hits = [ln for ln in stderr.splitlines() if any(m in ln for m in markers)]

    stdout = proc.stdout.decode(errors="replace")
    legs, value_fails = read_responses(stdout)

    print(f"server exit={proc.returncode}  responses={len(legs)}/{len(reqs)}  "
          f"value-mismatches={len(value_fails)}")
    if hits:
        print("\n!!! SANITIZER DIAGNOSTIC(S):")
        print(stderr)
        return 1
    if proc.returncode != 0:
        print("\n!!! server exited non-zero under sanitizers:")
        print(stderr[-4000:])
        return 1
    # One response per request. A short count is the server dying or wedging
    # part-way; without this the legs that never ran read as "clean".
    if len(legs) != len(reqs):
        print(f"\n!!! {len(reqs) - len(legs)} request(s) went unanswered — the "
              f"server stopped part-way through the sweep.")
        return 1
    if value_fails:
        print(f"\n!!! {len(value_fails)} leg(s) reported a batch-vs-stream "
              f"mismatch:")
        for f in value_fails[:20]:
            print("   %s" % f)
        if len(value_fails) > 20:
            print("   ... and %d more" % (len(value_fails) - 20))
        print("\nThese are VALUE failures, not memory ones — the same signal "
              "ta_regtest --codegen reports.")
        return 1
    # A request whose stream open is refused answers with every flag set and no leg run.
    dead = [req for req, n in zip(reqs, legs) if not n]
    if dead or not reqs:
        print(f"\n!!! {len(dead)} of {len(reqs)} request(s) answered and compared "
              f"nothing (for a large leg, add a LARGE_STEP_FOR row):")
        for req in dead:
            print("   %s" % req)
        return 1
    print("PASS — C stream API is ASan/UBSan%s clean, and every leg it drove "
          "matched batch bitwise." % ("/LSan" if leaks else " (no LeakSanitizer here)"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
