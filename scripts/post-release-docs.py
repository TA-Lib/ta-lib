#!/usr/bin/env python3

# Updates the website/documentation to match the latest release.
#
# Can be called directly, but more intended to be called by Github Actions.
#
import argparse
from dataclasses import dataclass
import json
import os
import re
import sys
import requests

from utilities.files import calculate_file_sha256, path_join
from utilities.common import is_brew_installed, run_command, verify_git_repo_original

try:
    from github import Github, Auth
except ImportError:
    print("This script requires the PyGithub library and it is not installed. Please do 'pip install PyGithub'")
    sys.exit(1)

def replace_version(file_path: str, version: str):
    # Replace ALL "x.y.z" pattern with "version".
    #
    # This happens only if the pattern is surrounded by characters such as "v", "[",-", "_" or whitespaces...
    # this is an heuristic logic to minimize changing unexpected text.
    with open(file_path, 'r') as f:
        content = f.read()

    # Define the regex pattern to match version numbers
    pattern = r'(?<=[v/\[\-_\s])\d+\.\d+\.\d+(?=[/\]\-_\s])'

    # Replace the matched patterns with the new version
    updated_content = re.sub(pattern, version, content)

    # Write the updated content back to the file only if there is a change
    if content != updated_content:
        with open(file_path, 'w') as f:
            f.write(updated_content)
        print(f"Updated version in {file_path} to {version}")

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Post-release documentation update')
    parser.add_argument('--token', help='GitHub token for authentication', required=False)
    args = parser.parse_args()

    # if an env GITHUB_TOKEN is defined... use it.
    if not args.token and 'GITHUB_TOKEN' in os.environ:
        args.token = os.getenv('GITHUB_TOKEN')
    else:
        print("Warning: No GITHUB_TOKEN env variable defined. Rate limit may fail the script.")

    if args.token:
        print("Using GitHub token for authentication")

    # if run from a fork, will exit with a message, but not an error.
    root_dir = verify_git_repo_original()

    try:
        g = Github(args.token) if args.token else Github()
        repos = g.get_repo("ta-lib/ta-lib")
        gh_version_obj = repos.get_latest_release()
        # Remove the 'v' prefix as needed
        gh_version = gh_version_obj.title.lstrip('v')

        print(f"Github release version  : {gh_version}")

        # Update version on the website installation page
        website_dir = path_join(root_dir, 'docs')
        install_md_file = path_join(website_dir, 'install.md')
        replace_version(install_md_file, gh_version)

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)










