#!/usr/bin/env python3

# Update *local* dev with latest from both dev and main *remote* branch.
#
# Such merging is often needed prior to a 'git push'.
#
# The local dev changes (if any) are temporarly stashed and merge back...
# so conflicts may need to be resolved manually (an error will be displayed).
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
        # Switch to dev branch if not already on it
        original_branch = run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])

        # Fetch the latest branch information from origin
        run_command(['git', 'fetch', 'origin'])

        # Stash any local dev changes
        if original_branch != "dev":
            print("Switching to dev branch")
            run_command(['git', 'checkout', 'dev'])
        run_command(['git', 'stash', 'push', '-m', 'sync-script-stash'])

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
        if 'sync-script-stash' in stash_list:
            try:
                run_command(['git', 'stash', 'pop'])
            except subprocess.CalledProcessError:
                print("Conflict occurred while applying stashed changes. Resolve conflicts manually.")
                print("1. Identify conflicts with 'git status'")
                print("2. Resolve manually by editing the conflicted files")
                print("3. Mark as resolved using 'git add <file>'")
                print("4. Complete merge with a 'git commit'")
                sys.exit(1)

        # Ensure the local main branch is up-to-date
        run_command(['git', 'checkout', 'main'])
        run_command(['git', 'pull', 'origin', 'main'])

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
