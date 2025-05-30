#!/usr/bin/env python3

"""
Enhanced Git Merge Utility
--------------------------
Safely merges dev branch into main with comprehensive checks,
conflict handling, and synchronization features.
"""

import subprocess
import sys
import os
import argparse
import re
from typing import Tuple, Optional

def run_command(command: list, capture_output: bool = True, check: bool = True) -> Tuple[bool, str]:
    """Execute a shell command with robust error handling."""
    try:
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE if capture_output else None,
            stderr=subprocess.PIPE,
            text=True,
            check=check
        )
        output = result.stdout.strip() if capture_output else ""
        return True, output
    except subprocess.CalledProcessError as e:
        error_msg = e.stderr.strip() if e.stderr else str(e)
        return False, f"Command failed: {' '.join(command)}\nError: {error_msg}"

def get_current_branch() -> Tuple[bool, str]:
    """Get the current Git branch name."""
    return run_command(['git', 'rev-parse', '--abbrev-ref', 'HEAD'])

def switch_branch(target_branch: str, dry_run: bool = False) -> Tuple[bool, str]:
    """Switch to a different Git branch."""
    if dry_run:
        return True, f"Would switch to branch '{target_branch}'"
    return run_command(['git', 'checkout', target_branch], capture_output=False)

def check_clean_working_tree(branch: str, dry_run: bool = False) -> Tuple[bool, str]:
    """Verify there are no uncommitted changes."""
    if dry_run:
        return True, "Would check for uncommitted changes"
    
    success, output = run_command(['git', 'status', '--porcelain'])
    if not success:
        return False, output
    
    if output:
        return False, f"Uncommitted changes in {branch} branch. Please commit or stash them."
    return True, "Working tree is clean"

def fetch_branch(branch: str, dry_run: bool = False) -> Tuple[bool, str]:
    """Fetch the latest version of a branch from remote."""
    if dry_run:
        return True, f"Would fetch origin/{branch}"
    return run_command(['git', 'fetch', 'origin', branch], capture_output=False)

def is_branch_up_to_date(local_branch: str, remote_branch: str) -> Tuple[bool, str]:
    """Check if local branch matches its remote counterpart."""
    success, local_commit = run_command(['git', 'rev-parse', local_branch])
    if not success:
        return False, local_commit
    
    success, remote_commit = run_command(['git', 'rev-parse', f'origin/{remote_branch}'])
    if not success:
        return False, remote_commit
    
    if local_commit == remote_commit:
        return True, "Branch is up-to-date"
    return False, f"{local_branch} not up-to-date with remote. Do 'git pull'."

def has_changes_to_merge(source: str, target: str) -> Tuple[bool, str]:
    """Check if there are changes to merge between branches."""
    success, merge_base = run_command(['git', 'merge-base', source, target])
    if not success:
        return False, merge_base
    
    success, diff_output = run_command(['git', 'diff', merge_base, source])
    if not success:
        return False, diff_output
    
    return (False, "No changes to merge") if not diff_output else (True, "Changes found to merge")

def handle_merge_conflict() -> str:
    """Provide detailed instructions for resolving merge conflicts."""
    instructions = [
        "\nMerge conflict detected! Resolve conflicts with:",
        "1. Identify conflicted files: 'git status'",
        "2. Resolve conflicts in affected files",
        "3. Mark resolved files: 'git add <file>'",
        "4. Complete merge: 'git commit'",
        "5. Push resolved merge: 'git push'",
        "6. Rebase dev branch: 'git checkout dev && git rebase main'",
        "7. Push dev branch: 'git push origin dev'"
    ]
    return "\n".join(instructions)

def main():
    parser = argparse.ArgumentParser(description='Merge dev into main branch with safety checks')
    parser.add_argument('--dry-run', action='store_true', help='Simulate actions without making changes')
    parser.add_argument('--skip-sync', action='store_true', help='Skip syncing main into dev')
    parser.add_argument('--auto-push', action='store_true', help='Automatically push after successful merge')
    args = parser.parse_args()

    try:
        # Store original branch for restoration
        success, original_branch = get_current_branch()
        if not success:
            print(f"Error: {original_branch}")
            sys.exit(1)

        print(f"Current branch: {original_branch}")

        # Switch to dev branch
        if original_branch != "dev":
            print("Switching to dev branch")
            success, output = switch_branch("dev", args.dry_run)
            if not success:
                print(output)
                sys.exit(1)

        # Check for uncommitted changes in dev
        success, output = check_clean_working_tree("dev", args.dry_run)
        if not success:
            print(output)
            sys.exit(1)

        # Ensure dev is up-to-date with remote
        success, output = fetch_branch("dev", args.dry_run)
        if not success:
            print(output)
            sys.exit(1)

        success, output = is_branch_up_to_date("dev", "dev")
        if not success:
            print(output)
            sys.exit(1)

        # Sync main into dev (unless skipped)
        if not args.skip_sync:
            print("Syncing main into dev branch...")
            # This would call your sync functionality
            # sync.main()  # Uncomment to enable
            if args.dry_run:
                print("Would sync main into dev")
        elif args.dry_run:
            print("Would skip syncing main into dev")

        # Switch to main branch
        print("Switching to main branch")
        success, output = switch_branch("main", args.dry_run)
        if not success:
            print(output)
            sys.exit(1)

        # Check if there are changes to merge
        success, output = has_changes_to_merge("dev", "main")
        if not success:
            print(output)
            sys.exit(1)
        if "No changes" in output:
            print(output)
        else:
            # Perform the merge
            print("Merging dev into main...")
            if args.dry_run:
                print("Would run: git merge --no-ff dev")
            else:
                success, output = run_command(['git', 'merge', '--no-ff', 'dev'], capture_output=False)
                if not success:
                    print("Merge conflict detected!")
                    print(handle_merge_conflict())
                    sys.exit(1)

            # Update dev branch to match main
            print("Updating dev branch to match main...")
            success, output = switch_branch("dev", args.dry_run)
            if not success:
                print(output)
                sys.exit(1)

            if args.dry_run:
                print("Would run: git rebase main")
            else:
                success, output = run_command(['git', 'rebase', 'main'], capture_output=False)
                if not success:
                    print("Rebase failed!")
                    print(output)
                    sys.exit(1)

            # Push changes if requested
            if args.auto_push and not args.dry_run:
                print("Pushing changes to remote...")
                for branch in ["dev", "main"]:
                    success, output = run_command(['git', 'push', 'origin', branch], capture_output=False)
                    if not success:
                        print(f"Failed to push {branch}: {output}")
                        sys.exit(1)
            elif args.dry_run:
                print("Would push dev and main branches")

        print("\nMerge completed successfully!")

    except Exception as e:
        print(f"Unexpected error: {str(e)}")
        sys.exit(1)

    finally:
        # Restore original branch
        if 'original_branch' in locals() and original_branch:
            current_success, current_branch = get_current_branch()
            if current_success and current_branch != original_branch:
                print(f"\nSwitching back to {original_branch} branch")
                switch_branch(original_branch, args.dry_run)

if __name__ == "__main__":
    main()
