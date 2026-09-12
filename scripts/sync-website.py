#!/usr/bin/env python3

# Point the website install page (website/src/install/c/README.md) at the latest
# *published* GitHub release. scripts/sync.py already does this as part of its
# metadata refresh; this is the same step on its own, plus a check.
#
#   scripts/sync-website.py                     Rewrite the page. No-op when in sync.
#   scripts/sync-website.py --check             Exit 1 if the page is behind.
#   scripts/sync-website.py --check --warn-only Emit a GitHub ::warning:: instead, exit 0.
#
# If the latest release cannot be determined, --check exits 2: "could not look" is
# not "in sync".

import argparse
import sys

from utilities.common import verify_git_repo
from utilities.website import install_page_versions, latest_release_version, sync_install_page


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Synchronize the website install page with the latest GitHub release.")
    parser.add_argument('--check', action='store_true',
                        help="Do not modify; exit non-zero if the page is behind or the release is unknown.")
    parser.add_argument('--warn-only', action='store_true',
                        help="With --check: emit a ::warning:: instead of failing.")
    args = parser.parse_args()

    root_dir = verify_git_repo()
    latest = latest_release_version()

    if args.check or args.warn_only:
        if latest is None:
            message = "could not determine the latest GitHub release, so the website install page was NOT checked."
            stale = None
        else:
            stale = sorted(v for v in install_page_versions(root_dir) if v != latest)
            if not stale:
                print(f"Website install page is in sync with the latest release ({latest}).")
                return 0
            message = (f"website/src/install/c/README.md advertises {stale} but the latest "
                       f"release is {latest}. Run scripts/sync.py on dev, commit, and merge to main.")
        if args.warn_only:
            print(f"::warning title=Website install page::{message}")
            return 0
        print(f"Error: {message}")
        return 2 if stale is None else 1

    if latest is None:
        print("Error: website install page NOT synced.")
        return 2
    if sync_install_page(root_dir, latest):
        print(f"Updated website install page to [{latest}].")
    else:
        print(f"No changes to website install page [{latest}].")
    return 0


if __name__ == "__main__":
    sys.exit(main())
