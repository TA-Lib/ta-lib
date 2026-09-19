import json
import os
import re
from urllib.error import HTTPError
from urllib.request import Request, urlopen
from xml.etree import ElementTree

from .files import path_join

LATEST_RELEASE_URL = "https://api.github.com/repos/ta-lib/ta-lib/releases/latest"
MAVEN_METADATA_URL = ("https://repo1.maven.org/maven2/io/github/ta-lib/ta-lib/"
                      "maven-metadata.xml")

# Each pattern names the version it owns `ver`, and only that group is rewritten,
# so a pattern may match as much surrounding text as it needs to be unambiguous.

# Delimited on both sides so only release-version tokens are rewritten (download
# URLs, filenames, the pip `version: "x.y.z"` snippet). Every x.y.z on the page
# becomes the release, so a page that ever needs another x.y.z must change this.
VERSION_PATTERN = r'(?<=[v/\["\'\-_\s])(?P<ver>\d+\.\d+\.\d+)(?=[/\]"\'\-_\s])'

# Anchored on the coordinate, not on `<version>` alone: that page is prose around
# snippets, and a second snippet naming any other artifact would otherwise have
# its version rewritten to ta-lib's.
MAVEN_VERSION_PATTERN = (r'<artifactId>ta-lib</artifactId>\s*'
                         r'<version>(?P<ver>\d+\.\d+\.\d+)</version>')

# A registry that has never published this artifact. Distinct from None, which
# means the lookup failed: nothing published is a steady state the page is
# already correct for, while a failed lookup is worth reporting and retrying.
NOT_PUBLISHED = "not-published"

# Every page that advertises a published version, and where that version comes
# from. Adding a backend before its first publish is safe only if its lookup
# reports NOT_PUBLISHED when the registry says the artifact does not exist. A
# lookup that answers None instead puts that backend in `unknown` on every run,
# so `--check` exits 2 and the nightly warns every night with nothing wrong.
PAGES = (
    ("C", ('website', 'src', 'install', 'c', 'README.md'), VERSION_PATTERN),
    ("Java", ('website', 'src', 'api', 'java', 'README.md'), MAVEN_VERSION_PATTERN),
)


def _get(url: str, headers: dict):
    """(body, http_status, error_text). body is None unless the fetch succeeded.

    Never raises. Every caller of this module can be run at any time, so an
    unreachable registry has to cost a warning and nothing else: the next run
    syncs what this one could not.
    """
    try:
        with urlopen(Request(url, headers=headers), timeout=15) as resp:
            return resp.read(), resp.status, None
    except HTTPError as e:
        return None, e.code, str(e)
    except Exception as e:
        return None, None, str(e)


def latest_release_version() -> str:
    """The latest PUBLISHED C release ("0.8.1"), or None if it cannot be determined.

    Drafts are invisible to this endpoint, so the website can never advertise a
    release that is not downloadable yet. Never NOT_PUBLISHED: the ABI gate reads
    this too and branches on None alone, so the sentinel stays where only the page
    table sees it. C has published releases, so the distinction is moot here.
    """
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "ta-lib-sync"}
    token = os.getenv("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    body, _status, error = _get(LATEST_RELEASE_URL, headers)
    if body is None:
        print(f"Warning: could not query the latest GitHub release ({error}).")
        return None
    try:
        tag = json.loads(body).get("tag_name") or ""
    except Exception as e:
        print(f"Warning: the latest GitHub release is not readable JSON ({e}).")
        return None
    match = re.fullmatch(r'v?(\d+\.\d+\.\d+)', tag)
    if not match:
        print(f"Warning: latest release tag '{tag}' is not vX.Y.Z.")
        return None
    return match.group(1)


def latest_maven_version() -> str:
    """The newest io.github.ta-lib:ta-lib on Central, NOT_PUBLISHED, or None."""
    body, status, error = _get(MAVEN_METADATA_URL, {"User-Agent": "ta-lib-sync"})
    if body is None:
        if status == 404:
            return NOT_PUBLISHED
        print(f"Warning: could not query Maven Central ({error}).")
        return None
    try:
        released = (ElementTree.fromstring(body).findtext('./versioning/release') or "").strip()
    except Exception as e:
        print(f"Warning: the Maven Central metadata is not readable XML ({e}).")
        return None
    if not re.fullmatch(r'\d+\.\d+\.\d+', released):
        print(f"Warning: Maven Central <release> '{released}' is not X.Y.Z.")
        return None
    return released


LOOKUPS = {"C": latest_release_version, "Java": latest_maven_version}


def page_path(root_dir: str, relpath) -> str:
    return path_join(root_dir, *relpath)


def _read(path: str):
    """The page's text, or None with a warning.

    The nightly runs the check with --warn-only, whose contract is to warn and
    never fail, so a page that moved must not raise out of here.
    """
    try:
        with open(path, 'r', newline='') as f:
            return f.read()
    except OSError as e:
        print(f"Warning: could not read {path} ({e}).")
        return None


def page_versions(root_dir: str, relpath, pattern: str):
    """The versions the page advertises, or None if it could not be read."""
    content = _read(page_path(root_dir, relpath))
    if content is None:
        return None
    return {m.group('ver') for m in re.finditer(pattern, content)}


def sync_page(root_dir: str, relpath, pattern: str, version: str):
    """Rewrite the page's `ver` groups to `version`.

    True if it changed, False if it was already in step, None if the page could
    not be read or written. None is not False: reporting an unreachable page as
    "no changes" would read as success to a caller about to commit.
    """
    path = page_path(root_dir, relpath)
    content = _read(path)
    if content is None:
        return None

    def replace(match):
        start, end = match.span('ver')
        text = match.group(0)
        return text[:start - match.start()] + version + text[end - match.start():]

    updated = re.sub(pattern, replace, content)
    if content == updated:
        return False
    try:
        with open(path, 'w', newline='') as f:
            f.write(updated)
    except OSError as e:
        print(f"Warning: could not write {path} ({e}).")
        return None
    return True


def sync_all(root_dir: str, known: dict = None) -> tuple:
    """Bring every page in PAGES into step.

    Returns (messages, unsynced): one report line per backend, and the backends
    whose version could not be looked up. A caller that goes on to commit must
    treat a non-empty `unsynced` as a failure. sync.py only reports it, because
    an unreachable registry must not stop it bringing everything else in step.

    `known` supplies versions the caller has already looked up, so sync.py does
    not pay for the C release twice.
    """
    known = known or {}
    messages, unsynced = [], []
    for backend, relpath, pattern in PAGES:
        version = known.get(backend) if backend in known else LOOKUPS[backend]()
        if version is None:
            unsynced.append(backend)
            messages.append(f"Warning: {backend} website page NOT synced; "
                            f"re-run once the registry is reachable.")
        elif version == NOT_PUBLISHED:
            messages.append(f"No {backend} release published yet; website page left alone.")
        else:
            changed = sync_page(root_dir, relpath, pattern, version)
            if changed is None:
                unsynced.append(backend)
                messages.append(f"Warning: {backend} website page NOT synced; its page "
                                f"could not be read or written.")
            elif changed:
                messages.append(f"Updated {backend} website page to [{version}]")
            else:
                messages.append(f"No changes to {backend} website page [{version}]")
    return messages, unsynced


def check_all(root_dir: str) -> tuple:
    """(stale, unknown) across every page in PAGES, looking at nothing else.

    stale is [(backend, [versions on the page], expected)]; unknown names the
    backends whose registry could not be reached or whose page could not be
    read, neither of which is the same as being in sync.
    """
    stale, unknown = [], []
    for backend, relpath, pattern in PAGES:
        version = LOOKUPS[backend]()
        if version is None:
            unknown.append(backend)
            continue
        if version == NOT_PUBLISHED:
            continue
        advertised = page_versions(root_dir, relpath, pattern)
        if advertised is None:
            unknown.append(backend)
            continue
        behind = sorted(v for v in advertised if v != version)
        if behind:
            stale.append((backend, behind, version))
    return stale, unknown
