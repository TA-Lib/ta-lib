#!/usr/bin/env python3

# Promote dev to main: fast-forward origin/main to origin/dev.
#
# Checks out no branch, so it runs from any worktree, detached HEAD included,
# and whichever worktree holds main cannot block it. The tree it runs in must
# BE origin/dev, clean: the gates read the working files, and what they pass is
# the exact commit that gets pushed.

import argparse
import subprocess
import sys

import sync
from utilities.common import verify_git_repo, run_command
from utilities.versions import check_sources_digest


def parse_args():
    """Takes no options: running it pushes, so a stray flag must stop it here."""
    parser = argparse.ArgumentParser(
        prog="scripts/merge.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=(
            "Fast-forward origin/main to origin/dev.\n"
            "\n"
            "Takes no options: invoking it performs the push.\n"
            "\n"
            "Run it from a clean tree checked out at origin/dev (the dev branch,\n"
            "or 'git switch --detach origin/dev' in any worktree). It refuses\n"
            "unless the committed TA_LIB_SOURCES_DIGEST matches the sources,\n"
            "i.e. the dev-nightly job has already regenerated and committed the\n"
            "dist assets, scripts/sync.py has nothing left to change, and main\n"
            "has no commit that dev lacks."
        ),
        epilog="Usage: run it with no arguments, after a green dev nightly.",
    )
    parser.parse_args()


def tracked_changes() -> str:
    return run_command(['git', 'status', '--porcelain', '--untracked-files=no'])


def realign_local_main(root_dir: str, pushed: str):
    """Fast-forward the local main branch, unless a worktree has it checked out:
    moving a checked-out branch would leave that worktree's files behind it.

    Never fatal: it runs after the push, and a failure here must not report the
    promotion as failed."""
    exists = subprocess.run(['git', 'show-ref', '--verify', '--quiet', 'refs/heads/main'])
    if exists.returncode != 0 or run_command(['git', 'rev-parse', 'main']) == pushed:
        return
    holder = sync.worktree_holding('main', root_dir)
    if holder:
        print(f"Local main is checked out in {holder} and is not at origin/main.")
        print(f"Update it with: git -C {holder} merge --ff-only origin/main")
        return
    moved = subprocess.run(['git', 'fetch', 'origin', 'main:main'],
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if moved.returncode == 0:
        print("Local main fast-forwarded.")
    else:
        print("Local main could not be fast-forwarded to origin/main; update it by hand.")


def main():
    root_dir = verify_git_repo()

    if tracked_changes():
        print("Uncommitted changes. Commit or stash them before merging.")
        sys.exit(1)

    run_command(['git', 'fetch', 'origin',
                 '+refs/heads/dev:refs/remotes/origin/dev',
                 '+refs/heads/main:refs/remotes/origin/main'])
    dev = run_command(['git', 'rev-parse', 'origin/dev'])
    if run_command(['git', 'rev-parse', 'HEAD']) != dev:
        print(f"This tree is not at origin/dev ({dev[:9]}).")
        print("On the dev branch: push or pull until it matches origin/dev.")
        print("In any other worktree: git switch --detach origin/dev")
        sys.exit(1)

    # Not redundant with the "sync.py changed these files" check below: this
    # also requires every dist asset to have been built from these sources,
    # which sync.py neither checks nor repairs. main-nightly's dist verification
    # fails on a main promoted without it. Keep it ahead of sync.py, which
    # rewrites a stale digest.
    print("Verifying dev sources digest is consistent...")
    if check_sources_digest(root_dir) is None:
        print("Error: dev is NOT consistent: its committed TA_LIB_SOURCES_DIGEST "
              "does not match its sources (mismatch printed above).")
        print("Wait for the dev-nightly job to regenerate and commit the "
              "digest + dist assets, or run 'scripts/package.py' on dev and "
              "commit the result, then re-run this merge.")
        sys.exit(1)
    print("dev sources digest is consistent.")

    sync.main([])

    dirty = tracked_changes()
    if dirty:
        print("sync.py changed these files on dev:")
        print(dirty)
        print("Commit them, push dev, then re-run this merge.")
        sys.exit(1)
    if run_command(['git', 'rev-parse', 'HEAD']) != dev:
        print("dev moved while sync.py ran: it merged main into dev or pulled a newer origin/dev.")
        print("Push dev if it is ahead of origin/dev, then re-run this merge.")
        sys.exit(1)

    if run_command(['git', 'rev-parse', 'origin/main']) == dev:
        print("No changes to merge from dev to main.")
        realign_local_main(root_dir, dev)
        return

    ancestor = subprocess.run(['git', 'merge-base', '--is-ancestor', 'origin/main', dev])
    if ancestor.returncode != 0:
        print("origin/main has commits that origin/dev lacks, so main cannot fast-forward.")
        print("Merge origin/main into dev, push dev, then re-run this merge.")
        sys.exit(1)

    # The hash, not the branch name: dev may have moved since the fetch, and
    # only this commit went through the gates.
    run_command(['git', 'push', 'origin', f'{dev}:refs/heads/main'])
    print(f"Merged dev into main ({dev[:9]}).")

    realign_local_main(root_dir, dev)


if __name__ == "__main__":
    parse_args()
    main()
