#!/usr/bin/env python3
"""Tolerate GitHub having no macOS runner, never a macOS test that fails.

nightly: one macOS job of this run. Each macOS job uploads a `started` and,
  as its last step, a `passed` artifact, each one file of that name holding
  the run attempt; the gate merges them into --records. A pass at least as
  recent as the start: green, and the time goes to --pass-record for the
  cache. A start with no such pass: red. Neither: the job never got a
  runner, green with a warning while --pass-record is younger than --hours.

release: refuse a commit unless every macOS job of main's nightly reached
  PASSED_STEP on it, in some run. A job-level continue-on-error may report a
  failed job as a success, so the job's own conclusion is not trusted.
"""

import argparse
import json
import os
import sys
import time
import urllib.request

API = "https://api.github.com"
PASSED_STEP = "Upload the 'passed' record for macos-gate"


def attempt(records, name):
    try:
        with open(os.path.join(records, name)) as f:
            return int(f.read().strip())
    except (OSError, ValueError):
        return None


def nightly(args):
    started = attempt(args.records, "started")
    passed = attempt(args.records, "passed")
    stamp = os.path.join(args.pass_record, "passed_at")

    if passed is not None and (started is None or passed >= started):
        os.makedirs(args.pass_record, exist_ok=True)
        with open(stamp, "w") as f:
            f.write(f"{int(time.time())}\n")
        print(f"{args.job}: passed (attempt {passed}).")
        out = os.environ.get("GITHUB_OUTPUT")
        if out:
            with open(out, "a") as f:
                f.write("passed=true\n")
        return 0
    if started is not None:
        print(f"::error::{args.job} ran on macOS (attempt {started}) and failed;"
              " see that job's log. Re-run that job itself: \"re-run failed jobs\""
              " may not count a continue-on-error job as failed.")
        return 1

    try:
        with open(stamp) as f:
            last = int(f.read().strip())
    except (OSError, ValueError):
        print(f"::error::{args.job} never started on a macOS runner, and no pass is on"
              f" record for this nightly: nothing tested it on macOS within {args.hours} h.")
        return 1
    age_h = (time.time() - last) / 3600
    when = time.strftime("%Y-%m-%d %H:%M UTC", time.gmtime(last))
    if age_h > args.hours:
        print(f"::error::{args.job} never started on a macOS runner, and its last pass"
              f" was {when} ({age_h:.0f} h ago, limit {args.hours} h).")
        return 1
    print(f"::warning::{args.job} never started on a macOS runner; tolerated:"
          f" last pass {when}, {age_h:.0f} h ago, limit {args.hours} h.")
    return 0


def get(path, token):
    req = urllib.request.Request(API + path, headers={
        "Authorization": f"Bearer {token}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
    })
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def paged(path, key, token):
    page = 1
    while True:
        sep = "&" if "?" in path else "?"
        items = get(f"{path}{sep}per_page=100&page={page}", token)[key]
        yield from items
        if len(items) < 100:
            return
        page += 1


def release(args):
    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        print("::error::GITHUB_TOKEN is not set.")
        return 1
    repo = args.repo
    # branch=main: a dispatch on another ref records that ref's tip as head_sha
    # while testing main's.
    runs = list(paged(f"/repos/{repo}/actions/workflows/{args.workflow}/runs"
                      f"?head_sha={args.sha}&branch=main&status=completed",
                      "workflow_runs", token))
    seen = {}
    for run in runs:
        # filter=all: a re-run attempt that passed counts as much as the first.
        for job in paged(f"/repos/{repo}/actions/runs/{run['id']}/jobs?filter=all",
                         "jobs", token):
            if not any(label.startswith("macos") for label in job.get("labels") or []):
                continue
            seen.setdefault(job["name"], False)
            if any(s["name"] == PASSED_STEP and s["conclusion"] == "success"
                   for s in job.get("steps") or []):
                seen[job["name"]] = True

    if not seen:
        print(f"::error::No completed {args.workflow} run on main at {args.sha} ran a"
              f" macOS job ({len(runs)} run(s) found). Run that workflow on this commit first.")
        return 1
    for name, ok in sorted(seen.items()):
        print(f"{'passed' if ok else 'NOT PASSED':>10}  {name}")
    missing = [name for name, ok in seen.items() if not ok]
    if missing:
        print(f"::error::{len(missing)} macOS job(s) never passed on {args.sha};"
              f" re-run {args.workflow} on it before releasing.")
        return 1
    return 0


def main():
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = p.add_subparsers(dest="cmd", required=True)
    n = sub.add_parser("nightly")
    n.add_argument("--job", required=True)
    n.add_argument("--records", required=True)
    n.add_argument("--pass-record", required=True)
    n.add_argument("--hours", type=int, default=72)
    r = sub.add_parser("release")
    r.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", "TA-Lib/ta-lib"))
    r.add_argument("--sha", required=True)
    r.add_argument("--workflow", default="main-nightly-tests.yml")
    args = p.parse_args()
    return nightly(args) if args.cmd == "nightly" else release(args)


if __name__ == "__main__":
    sys.exit(main())
