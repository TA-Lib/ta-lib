#!/usr/bin/env python3

# Updates the brew formula to matche the latest release assets.
# (done with a PR to the homebrew-core repository)
#
# Brew updates are CI automated, therefore this script should be
# called only by Github actions.
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

@dataclass
class BrewInfo:
    version: str
    sha256: str

def download_latest_github_asset_src_tar_gz(repo: any, dest_dir: str) -> str:
    # The repo parameter is the object return by PyGithub get_repo()
    latest_release = repos.get_latest_release()
    asset = None

    for a in latest_release.get_assets():
        if a.name.endswith('.tar.gz'):
            asset = a
            break

    if not asset:
        print(f"No src.tar.gz asset found")
        sys.exit(1)

    # Download the asset (unless already exists and verified same)
    asset_filepath = path_join(dest_dir, asset.name)
    if os.path.exists(asset_filepath):
        print(f"Asset already downloaded: {asset_filepath}")
        return asset.name

    asset_url = asset.browser_download_url
    os.makedirs(dest_dir, exist_ok=True)
    with requests.get(asset_url, stream=True) as r:
        r.raise_for_status()
        with open(asset_filepath, 'wb') as f:
            for chunk in r.iter_content(chunk_size=8192):
                f.write(chunk)

    print(f"Downloaded {tarball_filename} to {dest_dir}")
    return asset.name

def is_open_pr_for_formula(repo: any, formula_name: str) -> bool:
    # The repo parameter is the object return by PyGithub get_repo()
    pulls = repo.get_pulls(state='open', base='master')
    formula_prs = [pr for pr in pulls if formula_name in pr.title]

    first_letter = formula_name[0].lower()
    for pr in formula_prs:
        files = pr.get_files()
        for file in files:
            if file.filename == f"Formula/{first_letter}/{formula_name}.rb":
                return True
    return False

def verify_tarball(tarball_path: str) -> bool:
    # Sanity check that the tarball is valid.
    try:
        output = run_command(['tar', '-tf', tarball_path])
        # Verify one of the line of the output match the pattern
        # '*/src/tools/ta_regtest/ta_regtest.c'
        # Building ta_regtest is a requirement for brew.
        if not any(re.match(r".*/src/tools/ta_regtest/ta_regtest.c", line) for line in output.splitlines()):
            print(f"Error: tarball does not contain ta_regtest.c")
            return False

        return True
    except Exception as e:
        print(f"Error: {e}")
        return False

def get_ta_lib_brew_info() -> BrewInfo:
    brew_formula = "ta-lib"
    result_json = run_command(["brew", "info", "--json", brew_formula])
    brew_version = None

    # Extract the stable version and sha256. Example of JSON output:
    #[
    #  {
    #    "name": "ta-lib",
    #    "versions": {
    #      "stable": "0.4.0",
    #     },
    #    "urls":{"stable":{checksum: "sha256"}},
    #  }
    #]
    try:
        data = json.loads(result_json)
        brew_version = data[0]["versions"]["stable"]
        if brew_version is None:
            print(f"Error: Could not find the stable version for {brew_formula}")
            sys.exit(1)

        # Verify that version is the pattern x.y.z
        if not re.match(r"\d+\.\d+\.\d+", brew_version):
            print(f"Error: Invalid version found for {brew_formula}: {brew_version}")
            sys.exit(1)

        sha256 = data[0]["urls"]["stable"]["checksum"]
        if sha256 is None:
            print(f"Error: Could not find the sha256 for {brew_formula}")
            sys.exit(1)
        return BrewInfo(version=brew_version, sha256=sha256)
    except (KeyError, IndexError, json.JSONDecodeError) as e:
        print(f"Error parsing JSON: {e}")
        sys.exit(1)

    return None

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Post-release brew formula update')
    parser.add_argument('--token', help='GitHub token for authentication', required=False)
    args = parser.parse_args()

    if not is_brew_installed():
        print("Error: Brew needs to be installed and be on the PATH")
        sys.exit(1)

    # if an env GITHUB_TOKEN is defined... use it.
    if not args.token and 'GITHUB_TOKEN' in os.environ:
        args.token = os.getenv('GITHUB_TOKEN')
    else:
        print("Warning: No GITHUB_TOKEN env variable defined. Rate limit may fail the script.")

    if args.token:
        print("Using GitHub token for authentication")

    brew_info = get_ta_lib_brew_info()
    if not brew_info:
        print("Error: Could not get the brew info for ta-lib")
        sys.exit(1)
    print(f"Homebrew formula version: {brew_info.version}")

    # Will exit if run from a fork.
    root_dir = verify_git_repo_original()

    try:
        g = Github(args.token) if args.token else Github()
        repos = g.get_repo("ta-lib/ta-lib")
        gh_version_obj = repos.get_latest_release()
        # Remove the 'v' prefix as needed
        gh_version = gh_version_obj.title.lstrip('v')

        print(f"Github release version  : {gh_version}")
        if brew_info.version == gh_version:
            print("Homebrew formula is up-to-date")
            print("No update needed.")
            sys.exit(0)

        print("Update to Homebrew formula needed")
        old_sha256 = brew_info.sha256
        print(f"Old sha256: {old_sha256}")

        # Download the release asset in temp/
        dest_dir = path_join(root_dir, 'temp', 'post-release-brew')
        tarball_filename = download_latest_github_asset_src_tar_gz(repos,dest_dir)
        tarball_filepath = path_join(dest_dir, tarball_filename)

        # Untar just to verify that the tarball is OK.
        # Double check for consistency that the version are matching
        # (verify in case of "race condition" with github).
        #
        # Validate with regex that the filename is "ta-lib-{version}-src.tar.gz"
        # and extract the version part.
        m = re.match(r"ta-lib-(\d+\.\d+\.\d+)-src.tar.gz", tarball_filename)
        if not m:
            print(f"Error: Unexpected tarball filename: {tarball_filename}")
            sys.exit(1)
        # extract the version part
        gh_version_downloaded = m.group(1)
        if gh_version_downloaded != gh_version:
            print(f"Error: Inconsistent versions: {gh_version_downloaded} != {gh_version}")
            sys.exit(1)


        if not verify_tarball(tarball_filepath):
            print("Error: tarball verification failed")
            sys.exit(1)
        else:
            print("Tarball verification OK")

        # Calculate the sha256 of the downloaded tarball.
        new_sha256 = calculate_file_sha256(tarball_filepath)
        print(f"New sha256: {new_sha256}")

        if old_sha256 == new_sha256:
            print("Homebrew formula src.tar.gz sha256 is same")
            print("No update needed")
            sys.exit(1)

        # Check if a ta-lib PR already exists on homebrew-core
        # (to avoid creating a duplicate PR)
        hb_repo = g.get_repo("homebrew/homebrew-core")
        if is_open_pr_for_formula(hb_repo, "ta-lib"):
            print(f"PR already pending for ta-lib formula")
            print("No update needed")
            sys.exit(0)

        print("Creating PR for ta-lib formula")

        bump_cmd = ['brew', 'bump-formula-pr', '--strict']
        #bump_cmd.extend(['--dry-run'])
        gh_url = f"https://github.com/ta-lib/ta-lib/releases/download/{gh_version_obj.title}/ta-lib-{gh_version}-src.tar.gz"
        bump_cmd.extend(['--url', gh_url])
        bump_cmd.extend(['--sha256', new_sha256])
        bump_cmd.extend(['ta-lib'])

        output = run_command(bump_cmd)
        print(output)


    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)










