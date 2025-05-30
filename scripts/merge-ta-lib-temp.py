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
#!/usr/bin/env python3

import argparse
import subprocess
import sys
from utilities.common import verify_git_repo

def run_command(command, description=None):
    """Run a shell command and return the output, printing an optional description."""
    if description:
        print(f"> {description}...")
    try:
        result = subprocess.run(
            command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"❌ Command failed: {' '.join(command)}")
        print(e.stderr)
        raise

def main():
    parser = argparse.ArgumentParser(
        description="Merge 'ta-lib-temp' fork into the upstream TA-Lib repository's dev branch."
    )
    parser.add_argument(
        '--squash',
        type=str,
        help="Squash all changes into a single commit with this message."
    )
    args = parser.parse_args()

    try:
        # Step 1: Verify the current directory is within the official ta-lib repo.
        print("🔍 Verifying git repository...")
        root_dir = verify_git_repo()
        remote_url = run_command(['git', 'remote', 'get-url', 'origin'], "Checking origin remote URL")

        if not remote_url.endswith('ta-lib.git'):
            print("❌ This script must be run from the official TA-Lib repository (ending with ta-lib.git).")
            sys.exit(1)

        # Step 2: Ensure the 'ta-lib-temp' remote is configured.
        remotes = run_command(['git', 'remote'], "Checking configured remotes").splitlines()
        if 'ta-lib-temp' not in remotes:
            print("❌ Remote 'ta-lib-temp' is not configured.")
            print("💡 Use the following command to add it:")
            print("   git remote add ta-lib-temp https://github.com/TA-Lib/ta-lib-temp.git")
            sys.exit(1)

        # Step 3: Fetch latest from ta-lib-temp
        run_command(['git', 'fetch', 'ta-lib-temp'], "Fetching updates from ta-lib-temp")

        # Step 4: Checkout the official dev branch
        run_command(['git', 'checkout', 'dev'], "Checking out 'dev' branch")

        # Step 5: Merge or squash-merge ta-lib-temp/main
        if args.squash:
            print(f"🔨 Performing squash merge with commit message: '{args.squash}'")
            run_command(['git', 'merge', '--squash', 'ta-lib-temp/main'], "Squashing changes")
            run_command(['git', 'commit', '-m', args.squash], "Committing squash merge")
        else:
            run_command(['git', 'merge', 'ta-lib-temp/main'], "Merging changes")

        print("✅ Merge completed successfully into the 'dev' branch.")

    except subprocess.CalledProcessError:
        print("❌ Merge failed due to a command error.")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
