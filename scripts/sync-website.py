#!/usr/bin/env python3

# Synchronize the website install page (website/src/install/README.md) so it
# advertises the latest *published* GitHub release.
#
# The source of truth is the GitHub "latest release" API, so the website can
# only ever reflect a real, published release -- never an in-development VERSION
# from the dev/main branch. On any failure (offline, API rate limit, missing
# dependency, running from a fork, no release yet) the operation is a *no-op*:
# the update is eventual and will succeed on a later run.
#
# Idempotent and safe to call from various points:
#
#   scripts/sync-website.py
#       Rewrite website/src/install/README.md to match the latest release. No-op
#       if already in sync (or if the release cannot be determined). Intended to
#       run where CI already commits back to the repo (see dev-nightly-tests.yml)
#       so the committed files and the deployed website always match -- we never
#       rewrite at deploy time.
#
#   scripts/sync-website.py --check
#       Do not modify anything. Exit non-zero if website/src/install/README.md
#       does not match the latest release. Intended for a manual, post-release-bump
#       verification.
#
#   scripts/sync-website.py --check --warn-only
#       Like --check but never fails: on a mismatch it emits a GitHub Actions
#       ::warning:: annotation and exits 0 (keeps the nightly badge green while
#       still surfacing that the website has not caught up yet).
#
import argparse
import os
import re
import sys

from utilities.files import path_join
from utilities.common import verify_git_repo_original

# Version tokens in the install page are always bounded by one of these delimiters
# (a leading 'v', bracket, quote, dash, underscore, slash or whitespace, and a
# similar trailing char). Matching only delimited "x.y.z" avoids rewriting
# unrelated numbers while still covering every release-version occurrence
# (download URLs, filenames, the pip 'version: "x.y.z"' snippet, ...).
VERSION_PATTERN = r'(?<=[v/\["\'\-_\s])\d+\.\d+\.\d+(?=[/\]"\'\-_\s])'


def get_latest_release_version(token: str):
    # Return the latest *published* GitHub release version (e.g. "0.7.1"), or
    # None if it cannot be determined. Never raises: callers treat None as no-op.
    try:
        from github import Github
    except ImportError:
        print("Warning: PyGithub not installed; skipping website sync (no-op).")
        return None

    try:
        gh = Github(token) if token else Github()
        release = gh.get_repo("ta-lib/ta-lib").get_latest_release()
        # The tag is "vX.Y.Z"; extract the bare version to be robust to prefixes.
        match = re.search(r'\d+\.\d+\.\d+', release.tag_name or "")
        if not match:
            print(f"Warning: no version in release tag '{release.tag_name}' (no-op).")
            return None
        return match.group(0)
    except Exception as e:
        print(f"Warning: could not query the latest GitHub release ({e}); skipping (no-op).")
        return None


def install_md_versions(file_path: str) -> set:
    # The set of "x.y.z" versions currently advertised in the install page.
    with open(file_path, 'r') as f:
        content = f.read()
    return set(re.findall(VERSION_PATTERN, content))


def replace_version(file_path: str, version: str) -> bool:
    # Rewrite every install-page version token to `version`. Returns True if the
    # file changed. Idempotent: a second call with the same version is a no-op.
    with open(file_path, 'r') as f:
        content = f.read()
    updated = re.sub(VERSION_PATTERN, version, content)
    if content == updated:
        return False
    with open(file_path, 'w') as f:
        f.write(updated)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Synchronize the website install page with the latest GitHub release.")
    parser.add_argument('--token', help="GitHub token (defaults to $GITHUB_TOKEN).")
    parser.add_argument('--check', action='store_true',
                        help="Do not modify; exit non-zero if the website is out of date.")
    parser.add_argument('--warn-only', action='store_true',
                        help="With --check: emit a ::warning:: on mismatch and exit 0 (never fails).")
    args = parser.parse_args()

    token = args.token or os.getenv('GITHUB_TOKEN')
    if not token:
        print("Warning: no GITHUB_TOKEN; the GitHub API may be rate-limited.")

    # Exits 0 (with a message) when run from a fork -> inherently a no-op there.
    root_dir = verify_git_repo_original()
    install_md = path_join(root_dir, 'website', 'src', 'install', 'README.md')

    latest = get_latest_release_version(token)
    if latest is None:
        # Could not determine the released version -> eventual; no-op for now.
        return 0

    if args.check or args.warn_only:
        stale = sorted(v for v in install_md_versions(install_md) if v != latest)
        if not stale:
            print(f"Website is in sync with the latest release ({latest}).")
            return 0
        message = (f"website/src/install/README.md still advertises {stale} but the "
                   f"latest release is {latest}; it will sync on the next dev-nightly "
                   f"+ dev->main merge.")
        if args.warn_only:
            print(f"::warning title=Website out of date::{message}")
            return 0
        print(f"Error: {message}")
        return 1

    if replace_version(install_md, latest):
        print(f"Updated website/src/install/README.md to {latest}.")
    else:
        print(f"website/src/install/README.md already at {latest} (no change).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
