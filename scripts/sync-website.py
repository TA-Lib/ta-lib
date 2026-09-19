#!/usr/bin/env python3

# Point every website page that advertises a version at the latest *published*
# release of that backend: the C install page at the GitHub release, the Java
# API page at Maven Central. scripts/sync.py already does this as part of its
# metadata refresh; this is the same step on its own, plus a check.
#
#   scripts/sync-website.py                     Rewrite the pages. No-op when in sync.
#   scripts/sync-website.py --check             Exit 1 if a page is behind.
#   scripts/sync-website.py --check --warn-only Emit a GitHub ::warning:: instead, exit 0.
#
# If a registry cannot be reached, both modes exit 2: "could not look" is not
# "in sync", and a rewrite that silently skipped a page must not read as done to
# a caller that goes on to commit. A backend that has never published is not
# behind, so it is neither.

import argparse
import sys

from utilities.common import verify_git_repo
from utilities.website import check_all, sync_all


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Synchronize the website pages with the latest published release of each backend.")
    parser.add_argument('--check', action='store_true',
                        help="Do not modify; exit non-zero if a page is behind or a registry is unreachable.")
    parser.add_argument('--warn-only', action='store_true',
                        help="With --check: emit a ::warning:: instead of failing.")
    args = parser.parse_args()

    root_dir = verify_git_repo()

    if args.check or args.warn_only:
        stale, unknown = check_all(root_dir)
        if not stale and not unknown:
            print("Every website page is in sync with its latest published release.")
            return 0
        parts = [f"the {backend} page advertises {behind} but the latest release is {latest}"
                 for backend, behind, latest in stale]
        parts += [f"the {backend} registry could not be reached, so its page was NOT checked"
                  for backend in unknown]
        message = "; ".join(parts)
        if stale:
            message += ". Run scripts/sync.py on dev, commit, and merge to main."
        if args.warn_only:
            print(f"::warning title=Website version sync::{message}")
            return 0
        print(f"Error: {message}")
        return 1 if stale else 2

    messages, unsynced = sync_all(root_dir)
    for message in messages:
        print(message)
    if unsynced:
        print(f"Error: {', '.join(unsynced)} NOT synced, so the pages are not "
              f"pointed at the latest release.")
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
