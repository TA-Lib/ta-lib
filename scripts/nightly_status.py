#!/usr/bin/env python3
"""Refuse a commit unless every nightly of its branch passed on it.

A nightly tests the commit its reusable definition was pinned at when it ran
(the run's referenced_workflows sha), never its head_sha: a scheduled run's
head is main's tip whichever branch it tests.

With --allow-tail, a green commit followed only by commits that touch
TAIL_PATHS also passes: the dev nightly commits its dist packages and
instruction-count baseline back to dev after testing, plus the
TA_LIB_SOURCES_DIGEST line of include/ta_common.h.

--waive-absent-from=<ref> passes a nightly whose workflow file <ref> lacks:
GitHub runs a scheduled workflow only from the default branch, so a nightly
added on dev cannot have run before its first promotion to main.

--unacquired-days=<n> also passes a macOS nightly whose runs on the commit
failed only by jobs never given a runner, when the newest run before them is
green, tested an ancestor of the commit and started at most <n> days ago, and
no newer run is in progress. A release never passes it.
"""

import argparse
import datetime
import json
import os
import re
import subprocess
import sys
import urllib.request

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from utilities.net import fetch_retrying  # noqa: E402

API = "https://api.github.com"
# Each caller, and the one reusable definition a run of it must have run.
WORKFLOWS = {
    "main": {"main-nightly-tests.yml": "nightly-jobs.yml",
             "main-macos-nightly-tests.yml": "macos-jobs.yml"},
    "dev": {"dev-nightly-tests.yml": "nightly-jobs.yml",
            "dev-macos-nightly-tests.yml": "macos-jobs.yml"},
}
MACOS = "macos-jobs.yml"
TAIL_PATHS = ("dist/", ".github/perf/")
DIGEST_HEADER = "include/ta_common.h"
DIGEST_LINE = re.compile(r"^[+-]#define TA_LIB_SOURCES_DIGEST [0-9a-f]+$")


def token():
    t = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if t:
        return t
    try:
        return subprocess.run(["gh", "auth", "token"], capture_output=True, text=True,
                              check=True).stdout.strip() or None
    except (OSError, subprocess.CalledProcessError):
        return None


def get(path, tok):
    headers = {"Accept": "application/vnd.github+json", "X-GitHub-Api-Version": "2022-11-28"}
    if tok:
        headers["Authorization"] = f"Bearer {tok}"

    def once():
        with urllib.request.urlopen(urllib.request.Request(API + path, headers=headers),
                                    timeout=30) as r:
            return json.load(r)
    return fetch_retrying(once, API + path)


def tested_green(repo, workflow, reusable, branch, tok):
    """Commits of `branch` that a successful run of `workflow` tested, newest first."""
    runs = get(f"/repos/{repo}/actions/workflows/{workflow}/runs?status=success&per_page=100",
               tok)["workflow_runs"]
    want = f"{repo}/.github/workflows/{reusable}@{branch}"
    shas = []
    for run in runs:
        for ref in run.get("referenced_workflows") or []:
            if ref.get("path") == want and ref["sha"] not in shas:
                shas.append(ref["sha"])
    return shas


def git(*args):
    return subprocess.run(["git", *args], capture_output=True, text=True)


def only_tail_since(base, sha):
    if git("merge-base", "--is-ancestor", base, sha).returncode != 0:
        return False
    diff = git("diff", "--no-renames", "--name-only", base, sha)
    if diff.returncode != 0:
        return False
    for path in diff.stdout.split():
        if path == DIGEST_HEADER:
            hunk = git("diff", "--no-renames", "-U0", base, sha, "--", DIGEST_HEADER)
            lines = hunk.stdout.splitlines()
            body = lines[next((i for i, x in enumerate(lines) if x.startswith("@@")), len(lines)):]
            changed = [line for line in body if line[:1] in "+-"]
            if hunk.returncode != 0 or not all(DIGEST_LINE.match(line) for line in changed):
                return False
        elif not path.startswith(TAIL_PATHS):
            return False
    return True


def unacquired(repo, run, tok):
    """Whether a failed run failed only by jobs that were never given a runner.
    Every attempt counts: a rerun left unacquired must not hide a job that ran
    and failed in an earlier one."""
    if run.get("conclusion") != "failure":
        return False
    page = get(f"/repos/{repo}/actions/runs/{run['id']}/jobs?filter=all&per_page=100", tok)
    jobs = page["jobs"]
    if page.get("total_count", len(jobs)) > len(jobs):
        return False
    bad = [j for j in jobs if j.get("conclusion") not in ("success", "skipped")]
    return bool(bad) and all(j.get("conclusion") == "cancelled" and not j.get("steps")
                             and not j.get("runner_id") for j in bad)


def last_ran_green(repo, workflow, reusable, branch, same, sha, days, tok):
    """The commit the newest run with no unacquired job passed on, if `sha` was
    tried and left unacquired; None otherwise. `same(tested)` says a run tested
    `sha`."""
    runs = get(f"/repos/{repo}/actions/workflows/{workflow}/runs?per_page=100",
               tok)["workflow_runs"]
    want = f"{repo}/.github/workflows/{reusable}@{branch}"
    oldest = datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(days=days)
    tried = False
    for run in runs:
        tested = next((ref["sha"] for ref in run.get("referenced_workflows") or []
                       if ref.get("path") == want), None)
        if tested is None:
            continue
        if run.get("status") != "completed":
            return None
        if run.get("conclusion") == "success":
            started = datetime.datetime.fromisoformat(run["created_at"].replace("Z", "+00:00"))
            fresh = tried and started >= oldest
            return tested if fresh and git("merge-base", "--is-ancestor",
                                           tested, sha).returncode == 0 else None
        if not unacquired(repo, run, tok):
            return None
        tried = tried or same(tested)
    return None


def check(branch, sha, allow_tail=False, waive_absent_from=None, repo="TA-Lib/ta-lib",
          unacquired_days=None):
    """Print one line per nightly; True only if every one passed on `sha`."""
    tok = token()
    ok = True
    for wf, reusable in WORKFLOWS[branch].items():
        if waive_absent_from and git("cat-file", "-e",
                                     f"{waive_absent_from}:.github/workflows/{wf}").returncode:
            print(f"  waived     {wf}: not on {waive_absent_from} yet, so it has never run.")
            continue
        try:
            green = tested_green(repo, wf, reusable, branch, tok)
        except OSError as e:
            print(f"  ERROR      {wf}: could not read its runs ({e})")
            ok = False
            continue
        def same(g):
            return g == sha or (allow_tail and only_tail_since(g, sha))

        hit = next((g for g in green if same(g)), None)
        if hit is None and unacquired_days is not None and reusable == MACOS:
            try:
                older = last_ran_green(repo, wf, reusable, branch, same, sha,
                                       unacquired_days, tok)
            except OSError as e:
                print(f"  ERROR      {wf}: could not read its runs ({e})")
                ok = False
                continue
            if older:
                print(f"  no runner  {wf}: jobs left without a runner on {sha[:9]}; "
                      f"its last complete run is green, on {older[:9]}")
                continue
        if hit is None:
            print(f"  NOT GREEN  {wf} has no successful run on {sha[:9]}"
                  f"{' or on a commit before only what the nightly commits back' if allow_tail else ''}.")
            ok = False
        else:
            print(f"  green      {wf} on {hit[:9]}")
    return ok


def main():
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    p.add_argument("--branch", required=True, choices=sorted(WORKFLOWS))
    p.add_argument("--sha", required=True)
    p.add_argument("--allow-tail", action="store_true")
    p.add_argument("--waive-absent-from")
    p.add_argument("--unacquired-days", type=int)
    p.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", "TA-Lib/ta-lib"))
    a = p.parse_args()
    if check(a.branch, a.sha, a.allow_tail, a.waive_absent_from, a.repo, a.unacquired_days):
        return 0
    print(f"::error::Not every {a.branch} nightly passed on {a.sha}; run the red one(s) on it.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
