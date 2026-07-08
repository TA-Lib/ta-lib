#!/usr/bin/env python3
"""
Post-release helper: submit a TA-Lib update PR to microsoft/vcpkg.

What this script does
---------------------
1) Fetches the latest PUBLISHED TA-Lib release from GitHub (version + source tarball).
2) Computes the SHA512 of the tarball (required by vcpkg).
3) Prints the update plan.
4) With confirmation, updates a local microsoft/vcpkg checkout and opens a PR.
5) Opens a '[monitor] VCPkg release <version>' tracking issue in the TA-Lib repo
   (linking the vcpkg PR) so a maintainer can watch it through to merge — vcpkg
   reviews often take several days.

Usage
-----
  # Default: interactive full pipeline (clone vcpkg, update files, open PR)
  python post-release-vcpkg.py

  # Plan-only / non-interactive (used by CI, no side-effects)
  python post-release-vcpkg.py --plan

  # Use an existing local vcpkg checkout instead of auto-cloning
  python post-release-vcpkg.py --vcpkg-root /path/to/vcpkg

Safety guards (applied before performing any write operations)
-------------------------------------------------------------
- Must run from the original TA-Lib repository (not a fork).
- Must be on the 'main' branch.
- Checks for an existing open PR in microsoft/vcpkg to avoid duplicates.

Idempotency
-----------
Safe to run multiple times for the same release:
- If an open PR already exists for this version, the script reconciles the
  monitor issue (opening it only if missing) and exits cleanly.
- Updating the vcpkg port files is idempotent (re-sets the same version/SHA512).
- git checkout -B + "no changes to commit" guard prevents duplicate commits.
- The monitor issue is opened only if none already exists (open or closed) for
  this version, matched by exact title against the immediately-consistent issue
  list (not the eventually-consistent search index).

Version source
--------------
The authoritative version is the LATEST PUBLISHED RELEASE from the GitHub API.
The local git tag is checked as a sanity test; a mismatch prints an [info]
message with instructions on how to align your checkout (not an error), since
the tarball — not local source — is what vcpkg uses.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import Request, urlopen

from utilities.common import run_command, verify_git_repo_original
from utilities.files import path_join

API_RELEASE_LATEST = "https://api.github.com/repos/TA-Lib/ta-lib/releases/latest"
ASSET_PATTERN = re.compile(r"ta-lib-(\d+\.\d+\.\d+)-github-tag-archive\.tar\.gz$")
VCPKG_PORT_NAME = "talib"

# After the microsoft/vcpkg PR is opened, a tracking ("[monitor]") issue is opened in
# the TA-Lib repo so a maintainer can watch the vcpkg PR through to merge — vcpkg
# reviews often take several days. The maintainer closes the issue once the port is live.
MONITOR_REPO = "TA-Lib/ta-lib"
# "me" (per the maintainer's request) is mario4tier; greenTableWork is the second
# tracker. Both must have TA-Lib/ta-lib access for assignment to succeed — a failure
# to assign is a non-fatal warning (the issue is still opened).
MONITOR_ASSIGNEES = ["mario4tier", "greenTableWork"]


def sha512_file(path: Path) -> str:
    h = hashlib.sha512()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, out: Path) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    req = Request(url, headers={"User-Agent": "ta-lib-post-release-script"})
    with urlopen(req) as r, out.open("wb") as f:
        while True:
            chunk = r.read(1024 * 1024)
            if not chunk:
                break
            f.write(chunk)


def fetch_json(url: str) -> dict:
    req = Request(url, headers={"Accept": "application/vnd.github+json", "User-Agent": "ta-lib-post-release-script"})
    with urlopen(req) as r:
        return json.loads(r.read().decode("utf-8"))


def version_key(v: str) -> tuple[int, int, int]:
    m = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", v)
    if not m:
        raise ValueError(f"Invalid version: {v}")
    return int(m.group(1)), int(m.group(2)), int(m.group(3))


def read_latest_release_src_tarball() -> tuple[str, str, str, str]:
    """Return (version, tarball_name, download_url, tag_name) for the latest published release.

    IMPORTANT: the download_url is the GitHub TAG ARCHIVE
    (github.com/.../archive/<tag>.tar.gz), NOT the release's src.tar.gz
    asset. vcpkg_from_github() downloads the tag archive, so the SHA512 in
    portfile.cmake must be computed over those bytes — hashing the release
    asset produced a hash-mismatch failure on every triplet (0.7.1 PR).
    """
    data = fetch_json(API_RELEASE_LATEST)
    if data.get("draft", False):
        raise RuntimeError("Latest GitHub release is still a draft (not yet published).")
    tag_name = data.get("tag_name", "")
    if not tag_name:
        raise RuntimeError("Latest release has no tag_name")
    version = tag_name.lstrip("v")
    archive_url = f"https://github.com/TA-Lib/ta-lib/archive/{tag_name}.tar.gz"
    tarball_name = f"ta-lib-{version}-github-tag-archive.tar.gz"
    return version, tarball_name, archive_url, tag_name


def read_cached_latest_release_src_tarball(out_dir: Path) -> tuple[str, str, Path]:
    candidates: list[tuple[tuple[int, int, int], str, Path]] = []
    for p in out_dir.glob("ta-lib-*-github-tag-archive.tar.gz"):
        m = ASSET_PATTERN.search(p.name)
        if not m:
            continue
        version = m.group(1)
        candidates.append((version_key(version), version, p))
    if not candidates:
        raise RuntimeError("No cached ta-lib-<version>-src.tar.gz found")
    _, version, path = sorted(candidates)[-1]
    return version, path.name, path


def _check_local_at_release_tag(tag_name: str, version: str) -> None:
    """
    Sanity-check that the local checkout is at the release tag.
    This is informational only — the script builds from the downloaded tarball,
    not from local source.  A mismatch means the user may be confused about
    which version they are updating vcpkg for.

    Prints an info message (not an error) because the tarball is the authority.
    """
    # Resolve the commit the release tag points to.
    tag_result = subprocess.run(
        ["git", "rev-parse", f"refs/tags/{tag_name}^{{}}"],
        capture_output=True,
        text=True,
        cwd=Path(__file__).resolve().parent,
    )
    if tag_result.returncode != 0:
        # Tag not found locally — common if the repo was cloned without tags.
        print(
            f"[info] Release tag '{tag_name}' not found in the local repository.\n"
            f"[info] To align with the release, run:\n"
            f"[info]   git fetch --tags\n"
            f"[info]   git checkout {tag_name}\n"
            f"[info] (To return to the latest main afterwards: git checkout main)"
        )
        return

    tag_commit = tag_result.stdout.strip()

    head_result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        capture_output=True,
        text=True,
        cwd=Path(__file__).resolve().parent,
    )
    head_commit = head_result.stdout.strip() if head_result.returncode == 0 else ""

    if head_commit != tag_commit:
        print(
            f"[info] Local HEAD ({head_commit[:12]}) does not match release tag '{tag_name}' ({tag_commit[:12]}).\n"
            f"[info] The vcpkg update will still use the downloaded tarball for version {version},\n"
            f"[info] but if you want your local checkout to match the release:\n"
            f"[info]   git checkout {tag_name}\n"
            f"[info] (To return to the latest main afterwards: git checkout main)"
        )


def _current_branch() -> str:
    """Return the current git branch name, or '(unknown)' on failure."""
    result = subprocess.run(
        ["git", "rev-parse", "--abbrev-ref", "HEAD"],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        return "(unknown)"
    return result.stdout.strip()


def _find_existing_vcpkg_pr(version: str) -> str | None:
    """
    Return the html_url of an OPEN microsoft/vcpkg PR for this version, or None if
    there is none.  Uses the public GitHub search API (no auth required).

    Raises RuntimeError if the API call itself fails — the caller cannot safely
    proceed (it might open a duplicate PR), so it should retry later.
    """
    # Search for the port token "[talib]" — the PR title is "[talib] update to <ver>"
    # (vcpkg's [portname] convention; the port is VCPKG_PORT_NAME). NOT "[ta-lib]":
    # GitHub tokenizes "ta-lib" -> {ta, lib}, which never matches the single token
    # "talib", so an "[ta-lib]" query would silently miss the script's own PRs.
    query = f"repo:microsoft/vcpkg is:pr is:open [{VCPKG_PORT_NAME}] in:title"
    url = "https://api.github.com/search/issues?" + urlencode({"q": query, "per_page": "10"})
    # Match the version as a whole token (word boundaries) to avoid false positives
    # such as 0.6.1 matching 0.6.10 or 10.6.1.
    version_re = re.compile(r"\b" + re.escape(version) + r"\b")
    try:
        data = fetch_json(url)
    except Exception as e:
        raise RuntimeError(
            f"Could not verify existing vcpkg PRs: {e}\n"
            "Cannot proceed without confirming no duplicate PR exists. Please retry later."
        ) from e
    for item in data.get("items", []):
        title = item.get("title", "")
        if version_re.search(title):
            return item.get("html_url", "")
    return None


def _gh_capture(args: list[str], cwd: str | None = None) -> tuple[int, str, str]:
    """
    Run a `gh` command capturing output; return (returncode, stdout, stderr).

    Unlike run_command(), this NEVER exits or raises. The monitor issue is a
    best-effort convenience opened AFTER the vcpkg PR already exists, so its gh
    calls must degrade to warnings rather than aborting the release run — even if
    gh vanishes from PATH or exec fails between shutil.which() and here.
    """
    try:
        result = subprocess.run(args, capture_output=True, text=True, cwd=cwd)
    except Exception as e:
        return 1, "", str(e)
    return result.returncode, result.stdout.strip(), result.stderr.strip()


def create_monitor_issue(version: str, pr_url: str) -> None:
    """
    Open (idempotently) a '[monitor] VCPkg release <version>' tracking issue in the
    TA-Lib repo, linking the microsoft/vcpkg PR and assigning the maintainers so it
    can be watched through to merge and closed once the port is live.

    Best-effort: the vcpkg PR is already open by the time this runs, so every failure
    here prints a warning with manual-fallback instructions and returns — never fatal.
    """
    title = f"[monitor] VCPkg release {version}"
    gh_path = shutil.which("gh")
    if gh_path is None:
        print("[warn] GitHub CLI not found; skipping the vcpkg monitor issue.")
        print(f"       Open '{title}' manually in {MONITOR_REPO},")
        print(f"       link {pr_url or 'the microsoft/vcpkg PR'}, assign {', '.join(MONITOR_ASSIGNEES)}.")
        return

    # Idempotency: don't open a second monitor issue for the same version. Use the
    # plain issue-list (REST) endpoint with an exact title match — NOT `--search`,
    # whose index is eventually consistent (a prompt re-run could miss a seconds-old
    # issue -> duplicate) and tokenized (bracket/dot fuzziness). `--state all` also
    # suppresses re-creation once the maintainer has closed it.
    rc, out, _ = _gh_capture([gh_path, "issue", "list", "--repo", MONITOR_REPO,
                              "--state", "all", "--limit", "200",
                              "--json", "title,state,url"])
    if rc == 0 and out:
        try:
            for issue in json.loads(out):
                if issue.get("title", "") == title:
                    state = (issue.get("state") or "").lower()
                    print(f"[ok] vcpkg monitor issue already exists ({state}): {issue.get('url', '')}")
                    return
        except (json.JSONDecodeError, TypeError):
            pass  # Unparseable list -> fall through and create; a duplicate is harmless.

    pr_line = pr_url if pr_url else "see the open PRs at https://github.com/microsoft/vcpkg/pulls"
    body = (
        f"Tracking the microsoft/vcpkg port update for TA-Lib **{version}**.\n\n"
        f"- vcpkg PR: {pr_line}\n\n"
        "vcpkg PRs are reviewed and merged by the vcpkg maintainers, which usually "
        "takes a few days. This issue is a reminder to watch that PR through to merge.\n\n"
        f"**Close this issue** once the vcpkg PR is merged and `vcpkg install {VCPKG_PORT_NAME}` "
        f"installs {version}.\n\n"
        "_Opened automatically by `scripts/post-release-vcpkg.py`._"
    )
    rc, out, err = _gh_capture([gh_path, "issue", "create", "--repo", MONITOR_REPO,
                                "--title", title, "--body", body])
    if rc != 0:
        print(f"[warn] Could not open the vcpkg monitor issue: {err or 'unknown error'}")
        print(f"       Open '{title}' manually in {MONITOR_REPO} linking {pr_line}.")
        return
    issue_url = out.splitlines()[-1].strip() if out else ""
    print(f"[ok] Opened vcpkg monitor issue: {issue_url or '(created)'}")

    if not issue_url:
        print(f"[warn] Could not determine the monitor issue URL; assign "
              f"{', '.join(MONITOR_ASSIGNEES)} manually.")
        return

    # Assign the maintainers. Assignment requires TA-Lib/ta-lib access; gh applies
    # --add-assignee atomically (one non-assignable login drops them ALL), so if the
    # combined call fails, retry each login on its own — that way "me" still gets
    # assigned even if greenTableWork is not yet a collaborator. Never fatal.
    def _assign(logins: list[str]) -> bool:
        edit_args: list[str] = []
        for lg in logins:
            edit_args += ["--add-assignee", lg]
        rc_a, _, _ = _gh_capture([gh_path, "issue", "edit", issue_url] + edit_args)
        return rc_a == 0

    if _assign(MONITOR_ASSIGNEES):
        print(f"[ok] Assigned {', '.join(MONITOR_ASSIGNEES)} to the monitor issue.")
    else:
        assigned = [lg for lg in MONITOR_ASSIGNEES if _assign([lg])]
        failed = [lg for lg in MONITOR_ASSIGNEES if lg not in assigned]
        if assigned:
            print(f"[ok] Assigned {', '.join(assigned)} to the monitor issue.")
        if failed:
            print(f"[warn] Could not assign {', '.join(failed)} (needs {MONITOR_REPO} access).")
            print(f"       Assign manually if needed: {issue_url}")


def update_vcpkg_files(vcpkg_root: Path, version: str, sha512: str) -> None:
    """
    Idempotent: updates the vcpkg port files for talib to the given version + sha512.
    If expected files are missing, print guidance and return.
    """
    portfile = vcpkg_root / "ports" / VCPKG_PORT_NAME / "portfile.cmake"
    vcpkg_json = vcpkg_root / "ports" / VCPKG_PORT_NAME / "vcpkg.json"

    if not portfile.exists() or not vcpkg_json.exists():
        print(f"[warn] Could not find ports/{VCPKG_PORT_NAME} files in provided vcpkg checkout.")
        print("       Please update the port manually in microsoft/vcpkg.")
        return

    # Update version in vcpkg.json while preserving the current versioning scheme.
    # vcpkg manifests must contain exactly one of:
    # version, version-date, version-semver, version-string.
    data = json.loads(vcpkg_json.read_text())
    version_keys = ["version", "version-date", "version-semver", "version-string"]
    if "version-semver" in data:
        target_version_key = "version-semver"
    elif "version-string" in data:
        target_version_key = "version-string"
    elif "version-date" in data:
        target_version_key = "version-date"
    elif "version" in data:
        target_version_key = "version"
    else:
        target_version_key = "version"

    for key in version_keys:
        data.pop(key, None)
    data[target_version_key] = version
    vcpkg_json.write_text(json.dumps(data, indent=2) + "\n")

    # Replace SHA512 in portfile.cmake (first SHA512 occurrence)
    text = portfile.read_text()
    # \g<1> (not \1): a SHA starting with a digit would otherwise merge into
    # the group reference (\1 + "12..." parsed as \112 -> garbage in the file).
    text_new = re.sub(r"(SHA512\s+)[0-9a-fA-F]{64,128}", rf"\g<1>{sha512}", text, count=1)
    if text_new == text:
        print("[warn] Could not auto-replace SHA512 in portfile.cmake. Update manually.")
    else:
        portfile.write_text(text_new)

    print(f"[ok] Updated local vcpkg {VCPKG_PORT_NAME} port files.")


def ensure_vcpkg_checkout(vcpkg_root: Path) -> Path:
    vcpkg_root = vcpkg_root.resolve()
    if (vcpkg_root / ".git").exists():
        print(f"[ok] Using existing vcpkg checkout: {vcpkg_root}")
        try:
            run_command(["git", "fetch", "--all", "--prune"], cwd=str(vcpkg_root))
        except Exception as e:
            print(f"[warn] Could not fetch vcpkg remote: {e}")
        return vcpkg_root

    # Recover from older path bug where clone target was created nested under out_dir.
    nested_candidates = [
        p for p in vcpkg_root.parent.rglob("vcpkg")
        if p != vcpkg_root and (p / ".git").exists()
    ]
    if nested_candidates:
        nested = sorted(nested_candidates, key=lambda p: len(str(p)))[0]
        print(f"[warn] Found nested vcpkg checkout at: {nested}")
        print("[warn] Reusing it for this run.")
        return nested

    vcpkg_root.parent.mkdir(parents=True, exist_ok=True)
    try:
        run_command(
            ["git", "clone", "https://github.com/microsoft/vcpkg.git", str(vcpkg_root)],
            cwd=str(vcpkg_root.parent),
        )
        print(f"[ok] Cloned vcpkg into: {vcpkg_root}")
        return vcpkg_root
    except Exception as e:
        raise RuntimeError(
            f"Could not clone microsoft/vcpkg into {vcpkg_root}: {e}. "
            "Pass --vcpkg-root to an existing local checkout."
        )


def run_x_add_version(vcpkg_root: Path) -> None:
    vcpkg_exe = vcpkg_root / "vcpkg"
    if not vcpkg_exe.exists():
        bootstrap = vcpkg_root / "bootstrap-vcpkg.sh"
        if bootstrap.exists():
            try:
                run_command([str(bootstrap)], cwd=str(vcpkg_root))
            except Exception as e:
                print(f"[warn] bootstrap-vcpkg.sh failed: {e}")
                print("[warn] Skipping x-add-version. Run it manually when network/toolchain is available.")
                return
        if not vcpkg_exe.exists():
            print(f"[warn] Could not find {vcpkg_exe}; skipping x-add-version")
            return
    manifest_path = vcpkg_root / "ports" / VCPKG_PORT_NAME / "vcpkg.json"
    if manifest_path.exists():
        try:
            run_command([str(vcpkg_exe), "format-manifest", str(manifest_path)], cwd=str(vcpkg_root))
        except Exception as e:
            print(f"[warn] format-manifest failed: {e}")
            print(f"[warn] Run manually later: ./vcpkg format-manifest {manifest_path}")
            return
    try:
        run_command([str(vcpkg_exe), "x-add-version", VCPKG_PORT_NAME], cwd=str(vcpkg_root))
    except Exception as e:
        print(f"[warn] x-add-version failed: {e}")
        print(f"[warn] Run manually later: ./vcpkg x-add-version {VCPKG_PORT_NAME}")
        return
    print(f"[ok] Ran ./vcpkg x-add-version {VCPKG_PORT_NAME}")


def commit_and_open_pr(vcpkg_root: Path, version: str) -> None:
    run_command(["git", "checkout", "-B", f"ta-lib-{version}"], cwd=str(vcpkg_root))
    run_command(["git", "add", "-A"], cwd=str(vcpkg_root))

    status = run_command(["git", "status", "--porcelain"], cwd=str(vcpkg_root))
    if not status.strip():
        print("[warn] No changes to commit — port files already up-to-date for this version.")
        return

    run_command(["git", "commit", "-m", f"[ta-lib] update to {version}"], cwd=str(vcpkg_root))

    gh_path = shutil.which("gh")
    if gh_path is None:
        print("[warn] GitHub CLI not found; cannot fork/push/PR.")
        print("       Push the branch to YOUR vcpkg fork and run:")
        print("       gh pr create --repo microsoft/vcpkg --fill")
        return

    # Contributors cannot push branches to microsoft/vcpkg directly (403).
    # Push to the user's fork and open a cross-fork PR.
    login = run_command([gh_path, "api", "user", "--jq", ".login"]).strip()
    run_command([gh_path, "repo", "fork", "microsoft/vcpkg", "--clone=false"])  # idempotent
    remotes = run_command(["git", "remote"], cwd=str(vcpkg_root)).split()
    fork_url = f"https://github.com/{login}/vcpkg.git"
    if "fork" in remotes:
        run_command(["git", "remote", "set-url", "fork", fork_url], cwd=str(vcpkg_root))
    else:
        run_command(["git", "remote", "add", "fork", fork_url], cwd=str(vcpkg_root))
    run_command(["git", "push", "-u", "fork", f"ta-lib-{version}"], cwd=str(vcpkg_root))

    pr_output = run_command([gh_path, "pr", "create", "--repo", "microsoft/vcpkg",
                 "--head", f"{login}:ta-lib-{version}",
                 "--title", f"[talib] update to {version}", "--fill-first"],
                cwd=str(vcpkg_root))
    print("[ok] Opened PR in microsoft/vcpkg")

    # gh prints the new PR URL on stdout; grab it to link from the monitor issue.
    pr_url = ""
    for line in reversed(pr_output.splitlines()):
        line = line.strip()
        if line.startswith("http"):
            pr_url = line
            break
    create_monitor_issue(version, pr_url)


def main() -> int:
    p = argparse.ArgumentParser(
        description="Submit a TA-Lib update PR to microsoft/vcpkg after a release.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Run with no arguments to update the local vcpkg checkout and open a PR.\n"
            "Run with --plan to only print the update plan (used by CI, no side-effects)."
        ),
    )
    p.add_argument("--plan", action="store_true", help="Print the update plan only; do not modify anything.")
    p.add_argument("--vcpkg-root", help="Path to an existing local microsoft/vcpkg checkout (auto-cloned if omitted).")
    p.add_argument("--out-dir", default="temp/post-release-vcpkg", help="Directory for downloading/caching the release tarball.")
    args = p.parse_args()

    out_dir = Path(args.out_dir).resolve()

    # --- Step 1: resolve the authoritative version from the published release ---
    try:
        release_version, tar_name, asset_url, tag_name = read_latest_release_src_tarball()
    except Exception as e:
        raise RuntimeError(
            f"Could not fetch latest TA-Lib release from GitHub: {e}\n"
            "Make sure the release has been published (not a draft) and you have internet access."
        )

    # Sanity-check: verify the local checkout is at the release tag (informational only).
    try:
        _check_local_at_release_tag(tag_name, release_version)
    except Exception:
        pass  # Never block on this check — the tarball is the authority.

    version = release_version

    # --- Step 2: download (or use cached) tarball and compute SHA512 ---
    tar_path = out_dir / tar_name
    if not tar_path.exists():
        print(f"Downloading {asset_url}")
        download(asset_url, tar_path)
    else:
        print(f"Using cached tarball: {tar_path}")

    sha512 = sha512_file(tar_path)

    # --- Step 3: print the plan ---
    print("\n=== TA-Lib vcpkg update plan ===")
    print(f"Version : {version}")
    print(f"Asset   : {asset_url}")
    print(f"SHA512  : {sha512}")

    if args.plan:
        print("\n[plan mode] No changes made.")
        return 0

    # --- Step 4: safety guards (only when actually performing operations) ---
    verify_git_repo_original()
    branch = _current_branch()
    if branch != "main":
        raise RuntimeError(
            f"Must be on the 'main' branch (currently on '{branch}').\n"
            "The vcpkg port update should only be submitted after the official TA-Lib\n"
            "release has been published from main."
        )

    existing_pr = _find_existing_vcpkg_pr(version)
    if existing_pr:
        print(
            f"[info] An open PR for ta-lib {version} already exists in microsoft/vcpkg:\n"
            f"       {existing_pr}\n"
            "[info] No new vcpkg PR is needed; ensuring the monitor issue exists."
        )
        # Reconcile: open the monitor issue if it is missing (idempotent), so the
        # already-open vcpkg PR can still be tracked and closed out.
        create_monitor_issue(version, existing_pr)
        return 0

    # --- Step 5: confirmation prompt ---
    print(
        "\nThis will:\n"
        f"  1) Clone/update microsoft/vcpkg locally\n"
        f"  2) Update ports/{VCPKG_PORT_NAME}/{{portfile.cmake,vcpkg.json}} to {version}\n"
        f"  3) Run ./vcpkg x-add-version {VCPKG_PORT_NAME}\n"
        f"  4) Commit, push branch ta-lib-{version}, and open a PR to microsoft/vcpkg\n"
        f"  5) Open a '[monitor] VCPkg release {version}' issue in {MONITOR_REPO}\n"
        f"     (assigned to {', '.join(MONITOR_ASSIGNEES)}) to track that PR\n"
    )
    confirm = input("Proceed? (yes/NO): ")
    if confirm.strip().lower() != "yes":
        print("Operation cancelled.")
        return 0

    # --- Step 6: perform the update ---
    vcpkg_root = Path(args.vcpkg_root) if args.vcpkg_root else Path(path_join(out_dir, "vcpkg"))
    vcpkg_root = ensure_vcpkg_checkout(vcpkg_root)
    update_vcpkg_files(vcpkg_root, version, sha512)
    run_x_add_version(vcpkg_root)
    commit_and_open_pr(vcpkg_root, version)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as e:
        print(f"Error: {e}")
        raise SystemExit(1)

