#!/usr/bin/env python3
"""CI entry point for the dist/ content-addressed asset pool.

  dist-pool.py push     Upload every locally-built dist/ package into the pool.
                        Runs AFTER package.py + test-dist.py, so the digests it reads
                        already carry the real package_sha256.

  dist-pool.py pull     Download the packages referenced by dist/digests/ into dist/.
                        Runs BEFORE package.py, so is_build_skipping_allowed() and
                        test-dist.py find the binaries they expect on disk. Without this
                        a fresh checkout has no binaries, the skip is denied, and main
                        rebuilds instead of reusing the artifact dev tested.

  dist-pool.py verify   Check that every sha256 referenced by dist/digests/ resolves in
                        the pool. Changes nothing. This is the go/no-go gate before the
                        binaries stop being committed to git.

This is CI-internal. It is deliberately NOT a supported way for users or developers to
obtain packages: the pool is garbage-collected, so an old checkout's digests will point at
content that no longer exists. Users install from versioned releases; developers build
their own with package.py.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from utilities.common import verify_git_repo
from utilities.dist_pool import (
    PoolError,
    asset_age_days,
    delete_asset,
    download_and_verify,
    get_or_create_pool_release,
    list_pool_assets,
    pool_asset_name,
    resolve,
    sha256_of,
    upload_if_absent,
)
from utilities.files import path_join
from utilities.package_digest import PackageDigest


def _referenced(root_dir: str) -> list[tuple[str, str]]:
    """[(asset_file_name, sha256)] for every digest naming real pool content.

    Skips "Disabled" (github-* pseudo-assets track a branch, not a file) and "Unknown"
    (a digest written before its package was built, or before package_sha256 existed).
    """
    digests_dir = path_join(root_dir, "dist", "digests")
    if not os.path.isdir(digests_dir):
        print(f"Error: missing {digests_dir}")
        sys.exit(1)

    out: list[tuple[str, str]] = []
    for entry in sorted(os.listdir(digests_dir)):
        if not entry.endswith(".digest"):
            continue
        asset_file_name = entry[: -len(".digest")]
        pd = PackageDigest.read(root_dir, asset_file_name)
        if pd.package_sha256 in ("Disabled", "Unknown"):
            print(f"Info: skipping {asset_file_name} (package_sha256={pd.package_sha256})")
            continue
        out.append((asset_file_name, pd.package_sha256))
    return out


def cmd_push(root_dir: str, dry_run: bool) -> int:
    refs = _referenced(root_dir)
    if not refs:
        print("Error: no digest references a built package -- nothing to push.")
        return 1

    rel = None if dry_run else get_or_create_pool_release()
    failures = 0
    for asset_file_name, sha in refs:
        local = path_join(root_dir, "dist", asset_file_name)
        if not os.path.exists(local):
            print(f"Error: {asset_file_name} referenced by its digest but not in dist/")
            failures += 1
            continue
        # The digest is authoritative. If the file on disk hashes differently it was
        # modified after packaging, and uploading it would put content in the pool that
        # no digest describes.
        local_sha = sha256_of(local)
        if local_sha != sha:
            print(f"Error: {asset_file_name} hashes to {local_sha} but its digest says "
                  f"{sha} (file modified after packaging?)")
            failures += 1
            continue
        if dry_run:
            print(f"[dry-run] would upload {pool_asset_name(sha, asset_file_name)}")
            continue
        try:
            upload_if_absent(local, asset_file_name, sha256=sha, release=rel)
        except PoolError as e:
            print(f"Error: {e}")
            failures += 1

    return 1 if failures else 0


def cmd_pull(root_dir: str, dry_run: bool, allow_missing: bool = False) -> int:
    """Fetch referenced packages into dist/.

    Strict by default: a referenced package missing from the pool is an error, because
    on the release path it means the release would be built from content nobody can
    verify.

    --allow-missing is for the nightlies, where a pool miss is recoverable: package.py
    simply rebuilds the asset, re-tests it, and the following push re-uploads it. Failing
    the nightly there would turn a self-healing condition into a red build.
    """
    refs = _referenced(root_dir)
    if not refs:
        print("Info: no digest references pool content -- nothing to pull.")
        return 0

    assets = None if dry_run else list_pool_assets()
    failures = 0
    for asset_file_name, sha in refs:
        dest = path_join(root_dir, "dist", asset_file_name)
        # Already present and correct (a local build, or a previous pull) -- leave it.
        if os.path.exists(dest) and sha256_of(dest) == sha:
            print(f"Info: {asset_file_name} already present and matches digest")
            continue
        if dry_run:
            print(f"[dry-run] would fetch {pool_asset_name(sha, asset_file_name)}")
            continue
        try:
            download_and_verify(sha, asset_file_name, dest, assets=assets)
        except PoolError as e:
            if allow_missing:
                print(f"Warning: {e}")
                print(f"Warning: {asset_file_name} will be rebuilt from source.")
            else:
                print(f"Error: {e}")
                failures += 1

    return 1 if failures else 0


def _referenced_on_ref(ref: str) -> set[str]:
    """Every package_sha256 referenced by dist/digests/ on a git ref.

    Raises on ANY failure. GC must never guess at this set -- see cmd_gc.
    """
    listing = subprocess.run(
        ["git", "ls-tree", "--name-only", ref, "dist/digests/"],
        capture_output=True, text=True, check=True,
    ).stdout.split()
    if not listing:
        raise RuntimeError(f"{ref} has no dist/digests/ entries")

    out: set[str] = set()
    for path in listing:
        if not path.endswith(".digest"):
            continue
        blob = subprocess.run(["git", "show", f"{ref}:{path}"],
                              capture_output=True, text=True, check=True).stdout
        sha = json.loads(blob).get("package_sha256", "")
        if sha and sha not in ("Disabled", "Unknown"):
            out.add(sha)
    return out


def cmd_gc(root_dir: str, dry_run: bool, grace_days: float, refs: list[str]) -> int:
    """Delete pool assets no live branch references.

    Two failure modes this deliberately guards against:

    1. FAIL-CLOSED. If any ref cannot be read, abort without deleting anything.
       A partially-computed reference set looks exactly like "most assets are
       unreferenced", and acting on it would wipe the pool.

    2. GRACE PERIOD. A nightly uploads an asset and only afterwards pushes the
       digest naming it. In that window the asset is genuinely unreferenced but
       about to be referenced. Deleting it leaves a permanently dangling digest
       that no rebuild repairs, so anything younger than --grace-days is skipped.
    """
    referenced: set[str] = set()
    for ref in refs:
        try:
            got = _referenced_on_ref(ref)
        except Exception as e:  # noqa: BLE001 - deliberately fail closed
            print(f"Error: could not read digests from {ref}: {e}")
            print("Aborting without deleting anything: an incomplete reference set "
                  "is indistinguishable from an empty pool.")
            return 1
        print(f"Info: {ref} references {len(got)} package(s)")
        referenced |= got

    if not referenced:
        print("Error: no references found across any ref -- refusing to delete.")
        return 1

    assets = list_pool_assets()
    kept, deleted, young = 0, 0, 0
    for name, a in sorted(assets.items()):
        sha = name.split("__", 1)[0]
        if sha in referenced:
            kept += 1
            continue
        age = asset_age_days(a)
        if age < grace_days:
            print(f"Skip (age {age:.1f}d < {grace_days}d grace): {name}")
            young += 1
            continue
        if dry_run:
            print(f"[dry-run] would delete (age {age:.1f}d): {name}")
        else:
            delete_asset(a)
        deleted += 1

    print(f"\nreferenced-kept={kept}  within-grace={young}  "
          f"{'would-delete' if dry_run else 'deleted'}={deleted}")
    return 0


def cmd_verify(root_dir: str, dry_run: bool) -> int:
    refs = _referenced(root_dir)
    if not refs:
        print("Error: no digest references a built package -- nothing to verify.")
        return 1

    if dry_run:
        for asset_file_name, sha in refs:
            print(f"[dry-run] would resolve {pool_asset_name(sha, asset_file_name)}")
        return 0

    assets = list_pool_assets()
    missing = []
    for asset_file_name, sha in refs:
        try:
            a = resolve(sha, asset_file_name, assets)
            print(f"OK   {asset_file_name}  {sha[:16]}...  ({a.get('size')} bytes)")
        except PoolError:
            print(f"MISS {asset_file_name}  {sha[:16]}...")
            missing.append(asset_file_name)

    if missing:
        print(f"\nError: {len(missing)} referenced asset(s) missing from the pool: "
              f"{', '.join(missing)}")
        return 1
    print(f"\nAll {len(refs)} referenced assets present in the pool.")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["push", "pull", "verify", "gc"])
    ap.add_argument("--dry-run", action="store_true",
                    help="Show what would happen without any network call.")
    ap.add_argument("--grace-days", type=float, default=14.0,
                    help="gc only: never delete an asset younger than this. Covers the "
                         "window where a nightly has uploaded a package but not yet "
                         "pushed the digest that references it.")
    ap.add_argument("--refs", default="origin/dev,origin/main",
                    help="gc only: comma-separated git refs whose dist/digests/ define "
                         "the live reference set.")
    ap.add_argument("--allow-missing", action="store_true",
                    help="pull only: treat a package missing from the pool as a warning "
                         "instead of an error. For the nightlies, where package.py can "
                         "simply rebuild it. Never use on the release path.")
    args = ap.parse_args()

    root_dir = verify_git_repo()
    try:
        if args.command == "pull":
            return cmd_pull(root_dir, args.dry_run, allow_missing=args.allow_missing)
        if args.command == "push":
            return cmd_push(root_dir, args.dry_run)
        if args.command == "gc":
            return cmd_gc(root_dir, args.dry_run, args.grace_days,
                          [r.strip() for r in args.refs.split(",") if r.strip()])
        return cmd_verify(root_dir, args.dry_run)
    except PoolError as e:
        print(f"Error: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
