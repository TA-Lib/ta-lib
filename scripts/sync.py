#!/usr/bin/env python3

# Make local dev branch "catch up" with the remote main branch.
#
# Often needed to be done prior to push "dev".
#
# NOOP if nothing to merge

import subprocess
import sys

def run_command(command):
    """Run a shell command and return the output."""
    result = subprocess.run(command, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return result.stdout.strip()

def main():
    try:
        # Fetch the latest changes from the origin
        run_command(['git', 'fetch', 'origin'])
        run_command(['git', 'pull', 'origin', 'main'])

        # Checkout the dev branch
        run_command(['git', 'checkout', 'dev'])

        # Find the common ancestor of dev and main
        merge_base = run_command(['git', 'merge-base', 'dev', 'main'])

        # Check if there are any changes from main that are not in dev
        diff_output = subprocess.run(['git', 'diff', '--quiet', merge_base, 'main'], stderr=subprocess.DEVNULL)
        if diff_output.returncode == 0:
            print("No changes to merge from main to dev.")
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
                print("1. Identify conflicts with 'git status'.")
                print("2. Resolve manually by editing the conflicted files.")
                print("3. Mark as resolved using 'git add <file>'.")
                print("4. Complete merge with 'git commit'.")
                sys.exit(1)
    except subprocess.CalledProcessError as e:
        print(f"An error occurred: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
