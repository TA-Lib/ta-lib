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
"""

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.request

API = "https://api.github.com"
# Each caller, and the one reusable definition a run of it must have run.
WORKFLOWS = {
    "main": {"main-nightly-tests.yml": "nightly-jobs.yml",
             "main-macos-nightly-tests.yml": "macos-jobs.yml"},
    "dev": {"dev-nightly-tests.yml": "nightly-jobs.yml",
            "dev-macos-nightly-tests.yml": "macos-jobs.yml"},
}
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
    with urllib.request.urlopen(urllib.request.Request(API + path, headers=headers),
                                timeout=30) as r:
        return json.load(r)


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


def check(branch, sha, allow_tail=False, waive_absent_from=None, repo="TA-Lib/ta-lib"):
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
        hit = next((g for g in green if g == sha or (allow_tail and only_tail_since(g, sha))),
                   None)
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
    p.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", "TA-Lib/ta-lib"))
    a = p.parse_args()
    if check(a.branch, a.sha, a.allow_tail, a.waive_absent_from, a.repo):
        return 0
    print(f"::error::Not every {a.branch} nightly passed on {a.sha}; run the red one(s) on it.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
