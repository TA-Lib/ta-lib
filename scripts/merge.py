#!/usr/bin/env python3

# Merge dev into main branch

import subprocess
import sys
import os

import sync

def run_command(command):
    """Run a shell command and return the output."""
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return result.stdout.strip()

def main():
    try:
        # Switch to dev branch if not already on it
        current_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])
        if current_branch != "dev":
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
            print("dev branch not up-to-date with remote. Do 'git push'.")
            sys.exit(1)

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

if __name__ == "__main__":
    main()