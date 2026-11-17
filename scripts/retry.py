#!/usr/bin/env python3
"""Run a command again when it exits non-zero, for a fetch that can fail transiently.

Run:  python3 scripts/retry.py [--attempts N] [--delay SECONDS] -- <command> [args...]

Attempt k that fails waits k * delay seconds before the next. The exit status is
the last attempt's. Only for a fetch; never wrap a build or a test, whose
failure can be a real defect. The command must be safe to run again after a
failed attempt.
"""

import argparse
import subprocess
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(usage="%(prog)s [--attempts N] [--delay SECONDS] -- "
                                           "<command> [args...]")
    parser.add_argument("--attempts", type=int, default=3)
    parser.add_argument("--delay", type=float, default=5)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command or args.attempts < 1:
        parser.error("needs a command after --, and --attempts >= 1")

    name = " ".join(command)
    for attempt in range(1, args.attempts + 1):
        try:
            code = subprocess.call(command)
        except OSError as e:
            print(f"retry.py: cannot run {command[0]}: {e}", file=sys.stderr)
            return 127
        if code < 0:
            code = 128 - code
        if code == 0:
            return 0
        if attempt == args.attempts:
            print(f"retry.py: {name}: attempt {attempt}/{args.attempts} failed (exit {code})",
                  file=sys.stderr)
            return code
        delay = args.delay * attempt
        print(f"retry.py: {name}: attempt {attempt}/{args.attempts} failed (exit {code}); "
              f"retrying in {delay:g}s", file=sys.stderr)
        sys.stderr.flush()
        time.sleep(delay)
    return 1


if __name__ == "__main__":
    sys.exit(main())
