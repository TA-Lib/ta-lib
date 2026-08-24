#!/usr/bin/env python3

# Prepare the tree for a commit. Two independent halves:
#
#  (1) BRANCH MERGE. Update *local* dev with the latest from both the dev and
#      main *remote* branches. Often needed prior to a 'git push'. Local dev
#      changes (if any) are temporarily stashed and merged back, so conflicts
#      may need to be resolved manually (an error will be displayed).
#
#  (2) METADATA. Sync the TA-Lib versioning (see the VERSION file) consistently
#      across the files that need it, and update the TA-Lib source digest in
#      ta_common.h as needed.
#
# NOOP if nothing to merge or sync.
#
# HALF (1) IS SKIPPED AUTOMATICALLY WHERE IT CANNOT RUN. It has to check out dev
# and main, and git refuses to check out a branch that a second worktree already
# holds -- which is the normal state here, because dev lives in the main
# worktree and feature work happens in `git worktree` clones beside it. Rather
# than fail, this script detects that and does half (2) only, which needs no
# branch switching and is the half a feature branch actually wants before a
# commit. The same skip covers a detached HEAD, where there would be no branch
# to return to.
#
# So this is safe to run from anywhere: on dev in the main worktree it does
# both halves; from a feature worktree it does the metadata and says why it
# stopped there. Half (1) is not needed from a feature branch anyway -- it
# updates dev, not the branch you are on.
#
# A stale digest is not a defect, incidentally: dev-nightly regenerates and
# commits it (with the dist assets) and merge.py refuses dev->main while the two
# disagree. Running this first just saves that round trip.
#


from datetime import datetime
import os
import random
import string
import subprocess
import sys

from utilities.common import verify_git_repo, run_command
from utilities.versions import sync_sources_digest, sync_versions

def generate_short_unique_id(length=20) -> str:
    # Generate a "unique enough" short identifier.
    timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
    random_str = ''.join(random.choices(string.ascii_letters + string.digits, k=length - len(timestamp)))
    return timestamp + '-' + random_str

def worktree_holding(branch: str, root_dir: str) -> str:
    """Path of ANOTHER worktree that currently has `branch` checked out, else None.

    git refuses `git checkout <branch>` when a second worktree already holds it,
    so this is what decides whether the branch-merging half can run at all.
    """
    current = os.path.realpath(root_dir)
    path = None
    for line in run_command(['git', 'worktree', 'list', '--porcelain']).splitlines():
        line = line.strip()
        if line.startswith('worktree '):
            path = line[len('worktree '):].strip()
        elif line == f'branch refs/heads/{branch}':
            if path and os.path.realpath(path) != current:
                return path
    return None


def branch_merge_blocked_by(root_dir: str, current_branch: str) -> str:
    """Why the dev/main merge cannot run here, or None if it can."""
    if current_branch == 'HEAD':
        return "HEAD is detached, so there would be no branch to return to"
    for branch in ('dev', 'main'):
        holder = worktree_holding(branch, root_dir)
        if holder:
            return f"'{branch}' is checked out in another worktree ({holder})"
    return None


def main():
    try:
        original_branch = None

        # Switch to dev branch if not already on it
        root_dir = verify_git_repo()
        original_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])

        # Do nothing if there is staged changes.
        try:
            run_command(['git', 'diff', '--cached', '--exit-code'])
        except subprocess.CalledProcessError:
            print("Info: staged git changes detected. This script is intended to be run **before** any staging. No sync done.")
            sys.exit(1)

        # Can the branch-merging half run here at all? (See the notes at the top
        # of this file.) Decided BEFORE anything is fetched, stashed or checked
        # out, so the answer costs nothing when it is "no".
        blocked = branch_merge_blocked_by(root_dir, original_branch)

        if blocked:
            print(f"Skipping the dev/main merge: {blocked}.")
            print("Doing the versions + sources digest refresh only. That half needs "
                  "no branch switching, and updating dev is not what a feature branch "
                  "wants before a commit anyway.")
        else:
            # Do nothing if there are local commits not yet pushed. Only meaningful
            # on the merge path: on a feature branch, unpushed commits are the
            # normal state and must not block the metadata refresh below.
            #
            # A branch with no upstream has nothing to compare against, and
            # asking anyway is not harmless -- `git rev-list @{u}..HEAD` exits
            # non-zero there and run_command() turns that into sys.exit(1), so a
            # fresh branch used to kill the script outright.
            has_upstream = subprocess.run(
                ['git', 'rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{u}'],
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
            if has_upstream and run_command(['git', 'rev-list', '@{u}..HEAD']):
                print("Info: local commits need to be pushed. This script is intended to be run **before** any local staging/commits. No sync done.")
                sys.exit(1)

            # Fetch the latest branch information from origin
            run_command(['git', 'fetch', 'origin'])

            # Stash any local dev changes with a unique message
            stash_message = f'sync-script-stash-{generate_short_unique_id()}'

            if original_branch != "dev":
                print("Switching to dev branch")
                run_command(['git', 'checkout', 'dev'])
            run_command(['git', 'stash', 'push', '-m', stash_message])

            # Since everything in dev is now stashed, it is now possible
            # to switch to main branch and sync it.
            run_command(['git', 'checkout', 'main'])
            run_command(['git', 'pull', 'origin', 'main'])
            run_command(['git', 'checkout', 'dev'])

            # Get the local dev commit hash before pulling
            # This is later used to detect if any changes were pulled.
            local_dev_commit_before = run_command(['git', 'rev-parse', 'dev'])

            # Pull the latest dev changes
            run_command(['git', 'pull', 'origin', 'dev'])

            # Check if there were any changes pulled
            local_dev_commit_after = run_command(['git', 'rev-parse', 'dev'])
            if local_dev_commit_before == local_dev_commit_after:
                print("No changes to merge from origin/dev")
            else:
                print("Pulled latest changes from origin/dev")

            # Apply the stashed changes
            stash_list = run_command(['git', 'stash', 'list'])
            if stash_message in stash_list:
                try:
                    run_command(['git', 'stash', 'pop'])
                except subprocess.CalledProcessError:
                    print("Conflict occurred while applying stashed changes. Resolve conflicts manually.")
                    print("1. Identify conflicts with 'git status'")
                    print("2. Resolve manually by editing the conflicted files")
                    print("3. Mark as resolved using 'git add <file>'")
                    print("4. Complete merge with a 'git commit'")
                    sys.exit(1)


            # Switch back to dev branch
            run_command(['git', 'checkout', 'dev'])

            # Find the common ancestor of dev and main
            merge_base = run_command(['git', 'merge-base', 'dev', 'main'])

            # Check if there are any changes from main that are not in dev
            diff_output = subprocess.run(['git', 'diff', '--quiet', merge_base, 'main'], stderr=subprocess.DEVNULL)
            if diff_output.returncode == 0:
                print("No changes to merge from origin/main")
            else:
                # Perform the actual merge
                merge_output = subprocess.run(['git', 'merge', '--no-commit', '--no-ff', 'main'], stderr=subprocess.DEVNULL)
                if merge_output.returncode == 0:
                    # Check if there are any changes to commit
                    diff_index_output = subprocess.run(['git', 'diff-index', '--quiet', 'HEAD', '--'], stderr=subprocess.DEVNULL)
                    if diff_index_output.returncode == 0:
                        print("No changes to merge from main to dev.")
                        run_command(['git', 'merge', '--abort'])
                    else:
                        # Commit the merge if there are changes
                        run_command(['git', 'commit', '-m', 'Merged main into dev'])
                        print("Merged main into dev.")
                else:
                    print("Merge failed due to conflicts. Next steps:")
                    print("1. Identify conflicts with 'git status'")
                    print("2. Resolve manually by editing the conflicted files")
                    print("3. Mark as resolved using 'git add <file>'")
                    print("4. Complete merge with a 'git commit'")
                    sys.exit(1)

        # Make sure TA-Lib versioning is consistent in various files
        # used for building packages.
        is_updated, version = sync_versions(root_dir)
        if not is_updated:
            print(f"No changes to version [{version}]")

        # Update TA_LIB_SOURCES_DIGEST in ta_common.h (as needed)
        is_updated, digest = sync_sources_digest(root_dir)
        if is_updated:
            print(f"Updated sources digest (ta_common.h): [{digest}]")
        else:
            print(f"No changes to sources digest (ta_common.h) [{digest}]")

    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

    finally:
        # Restore to the branch the user was located before running this script
        if original_branch:
            current_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
            if current_branch != original_branch:
                print(f"Switching back to {original_branch} branch")
                run_command(['git', 'checkout', original_branch])

if __name__ == "__main__":
    main()
