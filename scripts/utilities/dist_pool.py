"""Content-addressed asset pool for dist/ release packages.

WHY THIS EXISTS
---------------
Release packages (.deb/.msi/.zip/.tar.gz) used to be committed into dist/. They are
already-compressed archives, so consecutive builds delta at ~0% and every nightly added
~15 MB of permanently-unique bytes to git history (dist/ reached 91% of the repo).

Now only dist/digests/*.digest is committed. Each digest carries a package_sha256, which
names the actual binary in this pool. The digest is the pointer; the pool is the store.

THE INVARIANT THIS PROTECTS
---------------------------
main REUSES the binary dev built and tested -- it does not rebuild it. See
package.py is_build_skipping_allowed(): scheduled nightlies rebuild, but a manual
workflow_dispatch (which is how a release is cut, README-DEVS.md step 6) SKIPS and reuses.
That is only possible if the exact tested bytes are retrievable, which is what the pool is
for. dev and main may legitimately hold different builds of the same filename, so assets are
named by content hash -- <sha256>__<original_name> -- and never collide.

DESIGN RULES (each one is load-bearing; see notes at each implementation site)
-----------------------------------------------------------------------------
1. ONE long-lived published pre-release, created once, NEVER deleted or recreated.
   Watchers are notified when a release is *published*; asset changes do not re-notify.
   Delete-and-recreate would re-notify on every nightly.
2. Published, not draft. Listing a draft requires push access, and main-nightly is
   deliberately contents:read. A draft would make the pool invisible to the very workflow
   that gates releases.
3. ALL listings paginate. The GitHub default page size is 30; the pool passes that quickly
   and an unpaginated read silently reports assets as absent.
4. Downloads are ANONYMOUS via browser_download_url and never send a token. The asset URL
   redirects to a CDN host; forwarding an Authorization header across that hop would leak
   GITHUB_TOKEN to a third party.
5. Uploads are idempotent. Content addressing means a name collision IS the same bytes, so a
   422 already_exists from a racing matrix leg is success, not failure.
"""

from __future__ import annotations

import hashlib
import json
import os
import sys
import time
import urllib.error
from urllib.parse import urlencode
from urllib.request import Request, urlopen

# The pool lives in the same repo as the source. Overridable for forks/testing.
POOL_REPO = os.getenv("TA_LIB_POOL_REPO", "TA-Lib/ta-lib")
POOL_TAG = os.getenv("TA_LIB_POOL_TAG", "ci-build-pool")

POOL_TITLE = "CI build pool (internal)"
POOL_BODY = (
    "Internal storage for CI-built release packages, addressed by content hash.\n\n"
    "**Not an installation source.** Install TA-Lib from a versioned release:\n"
    "https://github.com/TA-Lib/ta-lib/releases/latest\n\n"
    "Assets here are named `<sha256>__<filename>` and are referenced by the "
    "`package_sha256` field of `dist/digests/*.digest`. They are garbage-collected once "
    "no branch references them."
)

API_ROOT = "https://api.github.com"
UPLOAD_ROOT = "https://uploads.github.com"

_CHUNK = 1024 * 1024


class PoolError(RuntimeError):
    """Any pool operation that must stop the build."""


def _token() -> str:
    tok = os.getenv("GITHUB_TOKEN") or os.getenv("GH_TOKEN")
    if not tok:
        raise PoolError(
            "GITHUB_TOKEN is not set. The dist pool needs it to list and upload assets."
        )
    return tok


def _api(path: str, method: str = "GET", body: bytes | None = None,
         content_type: str | None = None, root: str = API_ROOT,
         expect: tuple[int, ...] = (200, 201)) -> tuple[int, dict | list | None]:
    """One authenticated GitHub API call. Returns (status, parsed_json_or_None).

    Raises PoolError on any status outside `expect`, EXCEPT 404 and 422 which are
    returned to the caller -- both are meaningful control flow here (release not yet
    created; asset already uploaded by a racing job).
    """
    url = path if path.startswith("http") else f"{root}{path}"
    headers = {
        "Authorization": f"Bearer {_token()}",
        "Accept": "application/vnd.github+json",
        "X-GitHub-Api-Version": "2022-11-28",
        "User-Agent": "ta-lib-dist-pool",
    }
    if content_type:
        headers["Content-Type"] = content_type

    req = Request(url, data=body, headers=headers, method=method)
    try:
        with urlopen(req) as resp:
            raw = resp.read()
            parsed = json.loads(raw) if raw else None
            return resp.status, parsed
    except urllib.error.HTTPError as e:
        raw = e.read()
        try:
            parsed = json.loads(raw) if raw else None
        except json.JSONDecodeError:
            parsed = {"raw": raw[:500].decode("utf-8", "replace")}
        if e.code in (404, 422) or e.code in expect:
            return e.code, parsed
        raise PoolError(f"{method} {url} -> HTTP {e.code}: {parsed}") from e
    except urllib.error.URLError as e:
        raise PoolError(f"{method} {url} -> {e}") from e


def sha256_of(filepath: str) -> str:
    """Streaming sha256 of a file. Must match PackageDigest.calculate_sha256()."""
    h = hashlib.sha256()
    with open(filepath, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            h.update(chunk)
    return h.hexdigest()


def pool_asset_name(sha256: str, asset_file_name: str) -> str:
    """Hash-first, matching the convention for content-addressed stores (Nix, OCI).

    Hash-first also reads as obviously machine-generated, which discourages installing
    from the pool by hand. '__' is the separator because the package filenames contain
    '-' and '_' but never '__', so the split back is unambiguous.
    """
    return f"{sha256}__{asset_file_name}"


def get_or_create_pool_release() -> dict:
    """Fetch the pool release, creating it only if it does not exist yet.

    RULE 1: never delete-and-recreate. Publishing a release notifies every watcher who
    subscribed to Releases; uploading assets to an already-published release does not.
    Recreating would turn a one-time notification into nightly spam.

    A published release is addressable by tag, so this is a plain GET -- unlike a draft,
    which is invisible to the by-tag endpoint and to any token without push access.
    """
    status, rel = _api(f"/repos/{POOL_REPO}/releases/tags/{POOL_TAG}")
    if status == 200 and isinstance(rel, dict):
        return rel

    # Not there yet -- create it. Two matrix legs can reach here at once.
    payload = json.dumps({
        "tag_name": POOL_TAG,
        "name": POOL_TITLE,
        "body": POOL_BODY,
        "draft": False,        # RULE 2: draft would hide the pool from contents:read jobs
        "prerelease": True,    # keeps the 'Latest' badge on the real versioned release
        "make_latest": "false",
    }).encode()

    status, rel = _api(f"/repos/{POOL_REPO}/releases", method="POST", body=payload,
                       content_type="application/json")
    if status == 201 and isinstance(rel, dict):
        return rel

    # 422 => the other matrix leg won the race and the tag now exists. Re-read it.
    # Unlike a draft release, a published tag is unique, so there is exactly one winner.
    if status == 422:
        status, rel = _api(f"/repos/{POOL_REPO}/releases/tags/{POOL_TAG}")
        if status == 200 and isinstance(rel, dict):
            return rel

    raise PoolError(f"Could not get or create pool release '{POOL_TAG}': HTTP {status} {rel}")


def list_pool_assets(release: dict | None = None) -> dict[str, dict]:
    """All pool assets as {asset_name: asset}. Fully paginated.

    RULE 3: the API default is 30 items per page. The pool passes 30 within days, and an
    unpaginated read would report present assets as missing -- silently re-uploading
    duplicates and, worse, making 'resolve' fail at release time.
    """
    rel = release or get_or_create_pool_release()
    out: dict[str, dict] = {}
    page = 1
    while True:
        q = urlencode({"per_page": 100, "page": page})
        status, assets = _api(f"/repos/{POOL_REPO}/releases/{rel['id']}/assets?{q}")
        if status != 200 or not isinstance(assets, list):
            raise PoolError(f"Listing pool assets failed: HTTP {status} {assets}")
        for a in assets:
            out[a["name"]] = a
        if len(assets) < 100:
            return out
        page += 1
        if page > 200:  # 20k assets: a runaway guard, never reached in practice
            raise PoolError("Pool asset listing exceeded 200 pages; aborting")


def upload_if_absent(local_path: str, asset_file_name: str,
                     sha256: str | None = None, release: dict | None = None) -> str:
    """Upload local_path into the pool unless its content is already there.

    Returns the pool asset name. Idempotent by construction: the name IS the content hash,
    so an existing name means identical bytes and there is nothing to do.
    """
    if not os.path.exists(local_path):
        raise PoolError(f"Cannot upload: {local_path} does not exist")

    digest = sha256 or sha256_of(local_path)
    name = pool_asset_name(digest, asset_file_name)
    rel = release or get_or_create_pool_release()
    existing = list_pool_assets(rel)

    prior = existing.get(name)
    if prior is not None:
        # An asset stuck in 'starter' state is a failed/partial upload from an earlier run.
        # It occupies the name but has no usable bytes, so it must be cleared and redone.
        if prior.get("state") == "uploaded":
            print(f"Info: pool already has {name} ({prior.get('size')} bytes)")
            return name
        print(f"Warning: pool asset {name} is in state "
              f"'{prior.get('state')}' -- deleting and re-uploading")
        _api(f"/repos/{POOL_REPO}/releases/assets/{prior['id']}",
             method="DELETE", expect=(204,))

    with open(local_path, "rb") as f:
        blob = f.read()

    q = urlencode({"name": name})
    upload_url = f"{UPLOAD_ROOT}/repos/{POOL_REPO}/releases/{rel['id']}/assets?{q}"
    status, res = _api(upload_url, method="POST", body=blob,
                       content_type="application/octet-stream")

    if status in (200, 201):
        print(f"Info: uploaded {name} ({len(blob)} bytes) to pool")
        return name

    # RULE 5: a racing matrix leg uploaded the same content first. Same name means same
    # bytes, so this is success, not a conflict to resolve.
    if status == 422:
        errs = (res or {}).get("errors") if isinstance(res, dict) else None
        codes = {e.get("code") for e in errs} if isinstance(errs, list) else set()
        if "already_exists" in codes:
            print(f"Info: {name} already uploaded by a concurrent job")
            return name

    raise PoolError(f"Upload of {name} failed: HTTP {status} {res}")


def delete_asset(asset: dict) -> None:
    """Remove one asset from the pool. Used only by garbage collection."""
    status, res = _api(f"/repos/{POOL_REPO}/releases/assets/{asset['id']}",
                       method="DELETE", expect=(204,))
    if status not in (204, 200):
        raise PoolError(f"Could not delete {asset['name']}: HTTP {status} {res}")
    print(f"Deleted {asset['name']} ({asset.get('size')} bytes)")


def asset_age_days(asset: dict) -> float:
    """Age of a pool asset in days, from its created_at timestamp."""
    from datetime import datetime, timezone
    created = asset.get("created_at")
    if not created:
        # No timestamp means we cannot prove the asset is old enough to be safe
        # to delete. Report it as brand new so GC leaves it alone.
        return 0.0
    dt = datetime.strptime(created, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
    return (datetime.now(timezone.utc) - dt).total_seconds() / 86400.0


def resolve(sha256: str, asset_file_name: str,
            assets: dict[str, dict] | None = None) -> dict:
    """Find a pool asset by content hash. Raises if absent -- never returns None.

    A missing asset means a committed digest points at content that is gone (GC'd too
    eagerly, or a digest committed before its upload landed). That must stop the build
    loudly; silently continuing would produce a release with missing or stale assets.
    """
    name = pool_asset_name(sha256, asset_file_name)
    pool = assets if assets is not None else list_pool_assets()
    a = pool.get(name)
    if a is None:
        raise PoolError(
            f"Pool asset not found: {name}\n"
            f"  A committed digest references content that is not in the pool.\n"
            f"  Either the upload never completed, or GC removed a still-referenced asset."
        )
    if a.get("state") != "uploaded":
        raise PoolError(f"Pool asset {name} is in state '{a.get('state')}', not usable")
    return a


def download_and_verify(sha256: str, asset_file_name: str, dest_path: str,
                        assets: dict[str, dict] | None = None,
                        retries: int = 3) -> str:
    """Download a pool asset and verify its content hash before it is usable.

    RULE 4: fetched anonymously from browser_download_url with NO Authorization header.
    That URL 302s to a CDN host; sending a token would hand GITHUB_TOKEN to a third party.
    This works because the pool is a *published* release -- a draft would have forced the
    authenticated asset endpoint and exactly that leak.

    The hash is verified before the file is moved into place, so dest_path never exists
    holding unverified bytes.
    """
    a = resolve(sha256, asset_file_name, assets)
    url = a["browser_download_url"]
    tmp = f"{dest_path}.partial"
    os.makedirs(os.path.dirname(os.path.abspath(dest_path)), exist_ok=True)

    last: Exception | None = None
    for attempt in range(1, retries + 1):
        try:
            h = hashlib.sha256()
            # Deliberately no auth header, and no custom headers that would survive the
            # redirect to the CDN.
            req = Request(url, headers={"User-Agent": "ta-lib-dist-pool"})
            with urlopen(req) as resp, open(tmp, "wb") as f:
                while True:
                    chunk = resp.read(_CHUNK)
                    if not chunk:
                        break
                    h.update(chunk)
                    f.write(chunk)
            got = h.hexdigest()
            if got != sha256:
                raise PoolError(
                    f"Hash mismatch for {asset_file_name}: pool content is {got}, "
                    f"digest says {sha256}"
                )
            os.replace(tmp, dest_path)
            print(f"Info: fetched {asset_file_name} from pool (sha256 verified)")
            return dest_path
        except Exception as e:  # noqa: BLE001 - retried, then re-raised below
            last = e
            if os.path.exists(tmp):
                os.remove(tmp)
            if attempt < retries:
                time.sleep(2 * attempt)

    raise PoolError(f"Could not fetch {asset_file_name} from pool after "
                    f"{retries} attempts: {last}")
