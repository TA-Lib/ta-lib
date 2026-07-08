#!/usr/bin/env python3

# Merge dev into main branch

import subprocess
import sys
import os

import sync
from utilities.common import verify_git_repo
from utilities.versions import check_sources_digest

def run_command(command):
    """Run a shell command and return the output."""
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return result.stdout.strip()

def main():
    try:
        # Switch to dev branch if not already on it
        original_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        if original_branch != "dev":
            print("Switching to dev branch")
            run_command(['git', 'checkout', 'dev'])

        # Make sure the dev branch does not have uncommitted changes.
        try:
            run_command(['git', 'diff-index', '--quiet', 'HEAD', '--'])
        except subprocess.CalledProcessError:
            print("Uncommitted changes in the dev branch. Please commit or stash them before merging.")
            sys.exit(1)

        # Make sure the dev branch is up-to-date with the remote.
        run_command(['git', 'fetch', 'origin', 'dev'])
        if run_command(['git', 'rev-parse', 'dev']) != run_command(['git', 'rev-parse', 'origin/dev']):
            print("dev branch not up-to-date with remote. Do 'git pull'.")
            sys.exit(1)

        # Gate: dev must be self-consistent before it is propagated to main.
        #
        # The committed TA_LIB_SOURCES_DIGEST must already match what dev's
        # sources compute. If it does not, the dev-nightly job has not yet
        # regenerated and committed the digest -- and, in the same pass, the
        # dist assets -- for the current sources. Merging that state into main
        # makes main-nightly's "Verify dist assets are up-to-date" gate fail,
        # exactly how PR #33's source change broke main after it landed there
        # without its dist regeneration. Refuse rather than propagate the
        # inconsistency (this also pre-empts sync.main() below silently
        # rewriting a stale digest into a dirty ta_common.h).
        root_dir = verify_git_repo()
        print("Verifying dev sources digest is consistent...")
        if check_sources_digest(root_dir) is None:
            print("Error: dev is NOT consistent -- its committed "
                  "TA_LIB_SOURCES_DIGEST does not match its sources (mismatch "
                  "printed above).")
            print("Wait for the dev-nightly job to regenerate and commit the "
                  "digest + dist assets, or run 'scripts/package.py' on dev and "
                  "commit the result, then re-run this merge.")
            sys.exit(1)
        print("dev sources digest is consistent.")

        # Call sync to verify that dev is up-to-date with main.
        # This is to avoid conflicts when merging dev into main.
        sync.main()

        # Switch to main branch
        print("Switching to main branch")
        run_command(['git', 'checkout', 'main'])
        run_command(['git', 'fetch', 'origin', 'main'])

        # Proceed to merge dev into main. Detect if there are conflicts, if yes
        # give instruction to resolve them.

        # Find the common ancestor of dev and main
        merge_base = run_command(['git', 'merge-base', 'dev', 'main'])

        # Check if there are any changes from dev that are not in main
        try:
            run_command(['git', 'diff', '--quiet', merge_base, 'dev'])
            print("No changes to merge from dev to main.")
        except subprocess.CalledProcessError:
            # Perform the actual merge
            try:
                run_command(['git', 'merge', '--ff-only', 'dev'])
                print("Merged dev into main.")

                # Rebase dev to keep on same last commit (that merge that was just done).
                run_command(['git', 'checkout', 'dev'])
                run_command(['git', 'rebase', 'main'])
                run_command(['git', 'push', 'origin', 'dev'])
                run_command(['git', 'push', 'origin', 'main'])
            except subprocess.CalledProcessError:
                print("Merge failed due to conflicts.")
                print("To resolve the conflicts, follow these steps:")
                print("1. Identify conflicted files using 'git status'.")
                print("2. Resolve manually by editing conflicted files.")
                print("3. Mark conflicts as resolved using 'git add <file>'.")
                print("4. Complete merge with 'git commit' and 'push'.")
                sys.exit(1)

    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

    finally:
        # Restore to the branch the user was located before running this script
        current_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        if current_branch != original_branch:
            print(f"Switching back to {original_branch} branch")
            run_command(['git', 'checkout', original_branch])

if __name__ == "__main__":
    main()