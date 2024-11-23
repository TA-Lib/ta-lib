#!/usr/bin/env python3

# Merge remote ta-lib-temp fork (main) into the "official" ta-lib repository (dev branch)
#
# Must be executed while current directory is in the official ta-lib repository.
#
# Optionally, a "--squash <comment>" allows to reduce all changes to a single commit.
#
# How to define the URL of the fork?
# With 'git remote add ta-lib-temp'. Example:
#   git remote add ta-lib-temp https://github.com/TA-Lib/ta-lib-temp.git
#
# Why this script?
# The ta-lib-temp fork is useful while developing/testing Github actions.
# It reduces the notification noise and risk while debugging the CI.
#
# (e.g. "nobody" watch a fork, while >20 watches are on the official ta-lib).

import argparse
import subprocess
import sys
from utilities.common import verify_git_repo

def run_command(command):
    """Run a shell command and return the output."""
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return result.stdout.strip()

def main():
    parser = argparse.ArgumentParser(description="Merge ta-lib-temp fork into the upstream ta-lib repository.")
    parser.add_argument('--squash', type=str, help="Squash all changes into a single commit with the specified comment.")
    args = parser.parse_args()


    try:
        # Verify that the current directory in within the official ta-lib repos.
        root_dir = verify_git_repo()
        remote_url = run_command(['git', 'remote', 'get-url', 'origin'])
        if not remote_url.endswith('ta-lib.git'):
            print("This script must be run from the official ta-lib repository.")
            sys.exit(1)

        # Check if ta-lib-temp is a remote
        remotes = run_command(['git', 'remote'])
        if 'ta-lib-temp' not in remotes:
            print(f"The remote ta-lib-temp is not configured.")
            print(f"Use 'git remote add ta-lib-temp <URL>' to add it.")
            print(f"Example: git remote add ta-lib-temp https://github.com/TA-Lib/ta-lib-temp.git")
            sys.exit(1)

        # Fetch the latest changes from the ta-lib-temp fork
        run_command(['git', 'fetch', 'ta-lib-temp'])

        # Checkout the main branch of the official repository
        run_command(['git', 'checkout', 'dev'])

        # Merge the cahnges from ta-lib-temp
        if args.squash:
            # Squash merge
            run_command(['git', 'merge', '--squash', 'ta-lib-temp/main'])
            run_command(['git', 'commit', '-m', args.squash])
        else:
            # Regular merge
            run_command(['git', 'merge', 'ta-lib-temp/main'])

        print("Merge completed successfully into official dev branch.\n")

    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()