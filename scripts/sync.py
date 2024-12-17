#!/usr/bin/env python3

# Update *local* dev with latest from both dev and main *remote* branch.
#
# Such merging is often needed prior to a 'git push'.
#
# The local dev changes (if any) are temporarly stashed and merge back...
# so conflicts may need to be resolved manually (an error will be displayed).
#
# NOOP if nothing to merge or sync.
#
# Other processing done:
#  - Sync the TA-Lib versioning (See VERSION file) consistently with various other files.
#  - Update the TA-Lib source digest in ta_common.h (as needed).
#


from datetime import datetime
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

        # Do nothing if there are local commits not yet pushed.
        if run_command(['git', 'rev-list', '@{u}..HEAD']):
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
