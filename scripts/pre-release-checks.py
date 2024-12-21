#!/usr/bin/env python3

# This is called from Github Actions to:
#  - Detect VERSION inconsistenties among the files.
#  - Detect if some release candidate assets are missing in dist/
#  - Detect if a top entry for VERSION is missing in CHANGELOG.md
#
# If no problem are found, the script will create a temp/DRAFT_RELEASE_NOTES.md
# which is intended for the CI when creating the initial draft release.
#
# You can also call this script locally for early detection of problems that
# would prevent the release to proceed smoothly.

import argparse
import os
import re

from utilities.files import path_join
from utilities.common import get_release_assets, verify_git_repo
from utilities.versions import check_sources_digest, check_versions

if __name__ == "__main__":

    root_dir = verify_git_repo()
    print(f"Running pre-release checks")

    parser = argparse.ArgumentParser(description='Pre-release checks')
    args = parser.parse_args()

    version = check_versions(root_dir)
    if not version:
        print("Error: Version inconsistencies found. Did you forget to run scripts/sync.py?")
        exit(1)

    sources_digest = check_sources_digest(root_dir)
    if not sources_digest:
        print("Error: Source digest inconsistencies found. Did you forget to run scripts/sync.py?")
        exit(1)

    expected_assets = get_release_assets(version)
    missing_assets = [asset for asset in expected_assets if not os.path.exists(path_join(root_dir, 'dist', asset))]
    if missing_assets:
        print("Error: Missing assets in dist/:")
        for asset in missing_assets:
            print(f"  - {asset}")

        print("Did you forget to wait for all release candidates assets be auto-generated in the dev branch?")
        exit(1)

    # Verify CHANGELOG.md exists and there is an entry for version.
    # At same accumulates all lines until end of top entry for latest version.

    changelog_path = path_join(root_dir, 'CHANGELOG.md')
    version_pattern = re.compile(r'##\s+\[\d+\.\d+\.\d+\].*')
    top_version_found = False
    release_notes = []

    with open(changelog_path, 'r') as f:
        for line in f:
            if version_pattern.match(line):
                if not top_version_found:
                    # Extract the 0.0.0 part of the pattern
                    changelog_version = line.split('[')[1].split(']')[0]
                    if changelog_version != version:
                        print(f"Error: Found top entry for version {changelog_version}, expected {version}.")
                        print("Did you forget to update CHANGELOG.md?")
                        exit(1)
                    top_version_found = True
                else:
                    break
            # Skip from writing the header lines "## Changelog" in the release notes.
            if line.startswith('# Changelog'):
                continue

            release_notes.append(line)
    if not top_version_found:
        print(f"Error: No entry found in CHANGELOG.md for version {version}.")
        print("Did you forget to update CHANGELOG.md?")
        exit(1)

    # Write the release_notes in temp/DRAFT_RELEASE_NOTES.md
    temp_dir = path_join(root_dir, 'temp')
    os.makedirs(temp_dir, exist_ok=True)
    release_notes_path = path_join(temp_dir, 'DRAFT_RELEASE_NOTES.md')
    with open(release_notes_path, 'w') as f:
        f.write(''.join(release_notes))

    # Verify that the release notes file was indeed written.
    if not os.path.exists(release_notes_path):
        print(f"Error: Failed to write draft release notes to {release_notes_path}.")
        exit(1)

    print(f"Draft release notes written to {release_notes_path}")

    print("pre-release checks completed successfully.")
