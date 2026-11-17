#!/usr/bin/env python3
"""Run the C library on several threads at once under ThreadSanitizer.

No other gate executes TA-Lib C code on more than one thread. Comparing the
answers of threads cannot find the defects that matter: a function-static
scratch, or a Peek that stores into its handle, returns correct values on
every thread. ThreadSanitizer reports them.

  1. Compile scripts/thread_sanitize.c with every library source as ONE clang
     command, `-fsanitize=thread`.
  2. Run it with address-space randomization off (`setarch -R`): the sanitizer
     refuses some randomized layouts with "unexpected memory mapping".
  3. Fail on any ThreadSanitizer report, on a mismatch against the
     single-thread answer, or when a call the driver makes refused or a leg
     ran fewer calls than it should.

clang, not gcc: a gcc ThreadSanitizer binary crashes at startup as soon as it
holds a `target_clones` function, which the library's FMA dispatch uses under
gcc. Under clang that dispatch compiles away, so the fused clones are not the
code this run exercises; the bitwise gates cover them.
"""

import glob
import os
import platform
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DRIVER = os.path.join(ROOT, "scripts/thread_sanitize.c")
BINARY = os.path.join(ROOT, "cmake-build/bin/ta_thread_sanitize")

# src/tools/ta_regtest holds the generated tables the driver walks to reach
# every function's typed and stream entry points.
INCLUDE_DIRS = ["include", "src/ta_common", "src/ta_abstract",
                "src/ta_abstract/frames", "src/ta_func", "src/tools/ta_regtest"]

# The driver's own exit codes. Not 1: setarch answers 1 when it cannot run it.
EXIT_MISMATCH = 10
EXIT_VACUOUS = 11
SOURCE_GLOBS = ["src/ta_common/*.c", "src/ta_func/*.c", "src/ta_abstract/*.c",
                "src/ta_abstract/frames/*.c", "src/ta_abstract/tables/*.c"]


def build():
    clang = shutil.which("clang")
    if not clang:
        print("!!! clang not found; ThreadSanitizer needs it (see the header).")
        return False
    sources = [p for g in SOURCE_GLOBS for p in sorted(glob.glob(os.path.join(ROOT, g)))]
    os.makedirs(os.path.dirname(BINARY), exist_ok=True)
    cmd = [clang, "-o", BINARY, DRIVER] + sources
    cmd += [f"-I{os.path.join(ROOT, d)}" for d in INCLUDE_DIRS]
    cmd += ["-O1", "-g", "-fsanitize=thread", "-fno-omit-frame-pointer", "-pthread",
            "-ffp-contract=off", "-fno-math-errno", "-lm"]
    print(f"Building the thread probe with {len(sources)} library sources "
          "(clang, -fsanitize=thread)...")
    subprocess.run(cmd, check=True, cwd=ROOT)
    return True


def main():
    if not build():
        return 1
    setarch = shutil.which("setarch")
    if not setarch:
        print("!!! setarch not found; it is what turns address-space randomization off.")
        return 1
    env = dict(os.environ)
    env["TSAN_OPTIONS"] = "halt_on_error=0:exitcode=66:second_deadlock_stack=1"
    proc = subprocess.run([setarch, platform.machine(), "-R", BINARY],
                          capture_output=True, cwd=ROOT, env=env)
    stdout = proc.stdout.decode(errors="replace")
    stderr = proc.stderr.decode(errors="replace")
    print(stdout.strip())
    reports = stderr.count("WARNING: ThreadSanitizer")
    if reports or "ThreadSanitizer" in stderr:
        print(f"\n!!! {reports} ThreadSanitizer report(s):")
        print(stderr[:12000])
        return 1
    if proc.returncode == EXIT_MISMATCH:
        print("\n!!! a thread's answer differs from the single-thread answer.")
        return 1
    if proc.returncode == EXIT_VACUOUS:
        print("\n!!! the probe could not set itself up, a call it makes refused, "
              "or a leg ran fewer calls than it should.")
        print(stderr[-4000:])
        return 1
    if proc.returncode != 0:
        print(f"\n!!! exit {proc.returncode}, not one of the probe's own: "
              "setarch could not run it, or it crashed.")
        print(stderr[-4000:])
        return 1
    print("PASS: no data race reported, and every thread reproduced the "
          "single-thread answer.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
