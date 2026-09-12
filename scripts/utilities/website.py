import json
import os
import re
from urllib.request import Request, urlopen

from .files import path_join

LATEST_RELEASE_URL = "https://api.github.com/repos/ta-lib/ta-lib/releases/latest"

# Delimited on both sides so only release-version tokens are rewritten (download
# URLs, filenames, the pip `version: "x.y.z"` snippet). Every x.y.z on the page
# becomes the release, so a page that ever needs another x.y.z must change this.
VERSION_PATTERN = r'(?<=[v/\["\'\-_\s])\d+\.\d+\.\d+(?=[/\]"\'\-_\s])'


def install_page_path(root_dir: str) -> str:
    return path_join(root_dir, 'website', 'src', 'install', 'c', 'README.md')


def latest_release_version() -> str:
    """The latest PUBLISHED release ("0.8.1"), or None if it cannot be determined.

    Drafts are invisible to this endpoint, so the website can never advertise a
    release that is not downloadable yet. Never raises: None is the caller's to report.
    """
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "ta-lib-sync"}
    token = os.getenv("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    try:
        with urlopen(Request(LATEST_RELEASE_URL, headers=headers), timeout=15) as resp:
            tag = json.load(resp).get("tag_name") or ""
    except Exception as e:
        print(f"Warning: could not query the latest GitHub release ({e}).")
        return None
    match = re.fullmatch(r'v?(\d+\.\d+\.\d+)', tag)
    if not match:
        print(f"Warning: latest release tag '{tag}' is not vX.Y.Z.")
        return None
    return match.group(1)


def install_page_versions(root_dir: str) -> set:
    with open(install_page_path(root_dir), 'r') as f:
        return set(re.findall(VERSION_PATTERN, f.read()))


def sync_install_page(root_dir: str, version: str) -> bool:
    """Rewrite every version token on the install page to `version`. True if it changed."""
    path = install_page_path(root_dir)
    with open(path, 'r', newline='') as f:
        content = f.read()
    updated = re.sub(VERSION_PATTERN, version, content)
    if content == updated:
        return False
    with open(path, 'w', newline='') as f:
        f.write(updated)
    return True
