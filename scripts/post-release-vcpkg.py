#!/usr/bin/env python3
"""
Post-release helper: update the talib port in microsoft/vcpkg, in reviewable stages.
`prepare` and `submit` are POSIX only (they drive bootstrap-vcpkg.sh and ./vcpkg).

Usage
-----
  post-release-vcpkg.py plan [--out-dir DIR]
      Read-only. Resolve the latest published TA-Lib release, download its source
      asset into DIR, print version, asset URL and SHA512.

  post-release-vcpkg.py prepare [--out-dir DIR] [--vcpkg-root DIR]
      No network writes. In the vcpkg checkout (cloned --depth=1 when missing),
      branch ta-lib-<ver> from microsoft/vcpkg master, bootstrap the pinned
      vcpkg-tool, bump the version and first SHA512, format the manifest.
      Prints the diffstat and the local gate to run before `submit`.

  post-release-vcpkg.py submit [--out-dir DIR] [--vcpkg-root DIR] [--yes]
      Registers the version (x-add-version), commits "[talib] update to <ver>",
      verifies the version DB, then after a prompt: syncs your fork's master,
      pushes the branch, opens a DRAFT PR and a '[monitor] VCPkg release <ver>'
      issue in TA-Lib/ta-lib.

--out-dir defaults to <repo>/temp/post-release-vcpkg and --vcpkg-root to
<out-dir>/vcpkg. `prepare` and `submit` need `gh auth login`, and act for the
TA-Lib checkout holding this script whatever the current directory.

Re-running
----------
- `prepare` refuses a dirty checkout, a version already in microsoft/vcpkg
  master, your open or merged PR from ta-lib-<ver>, and anyone's open
  "[talib] ... <ver>" PR. Otherwise it resets ta-lib-<ver> to current master.
- `submit` keeps the branch at one commit: a later port edit is amended into
  it, and a branch carrying any other commit is refused. If your PR is already
  open or merged, it only offers to open a missing monitor issue.
  After a failed `gh pr create`, a re-run pushes again and retries the PR.
- The monitor issue is opened only when the full issue list, open and closed,
  has none with its exact title. If that list cannot be read, it is not opened
  and `submit` exits 1.
- The PR body file is generated once; edit it freely, delete it to regenerate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
from pathlib import Path
from urllib.request import Request, urlopen

from utilities.common import verify_git_repo_original

REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUT_DIR = REPO_ROOT / "temp" / "post-release-vcpkg"

API_RELEASE_LATEST = "https://api.github.com/repos/TA-Lib/ta-lib/releases/latest"
VCPKG_UPSTREAM_URL = "https://github.com/microsoft/vcpkg.git"
VCPKG_UPSTREAM_REPO = "microsoft/vcpkg"
UPSTREAM_REF = "refs/post-release-vcpkg/master"
VCPKG_PORT_NAME = "talib"
PORT_PATHS = [f"ports/{VCPKG_PORT_NAME}", "versions/t-/talib.json", "versions/baseline.json"]
VCPKG_MARKERS = ["scripts/vcpkg-tool-metadata.txt", f"ports/{VCPKG_PORT_NAME}/vcpkg.json"]
GATE_TRIPLETS = ["x64-linux", "x64-linux-dynamic"]
VERSION_KEYS = ["version", "version-date", "version-semver", "version-string"]

MONITOR_REPO = "TA-Lib/ta-lib"
# Both need TA-Lib/ta-lib access; an assignment failure is only a warning.
MONITOR_ASSIGNEES = ["mario4tier", "greenTableWork"]
MONITOR_LIST_LIMIT = 10000


# --- process helpers ---------------------------------------------------------

def _run(cmd: list[str], cwd: Path | None = None, echo: bool = False) -> str:
    """Run cmd; return stdout without trailing whitespace. Raise RuntimeError carrying stdout+stderr on failure."""
    if echo:
        print(f"$ {shlex.join(cmd)}")
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
    except OSError as e:
        raise RuntimeError(f"'{shlex.join(cmd)}' could not start: {e}") from e
    if r.returncode != 0:
        raise RuntimeError(
            f"'{shlex.join(cmd)}' exited {r.returncode}\n"
            f"--- stdout ---\n{r.stdout.rstrip()}\n--- stderr ---\n{r.stderr.rstrip()}"
        )
    return r.stdout.rstrip()


def _succeeds(cmd: list[str], cwd: Path | None = None) -> bool:
    try:
        return subprocess.run(cmd, capture_output=True, cwd=cwd).returncode == 0
    except OSError:
        return False


def _gh() -> str:
    gh = shutil.which("gh")
    if gh is None:
        raise RuntimeError("GitHub CLI 'gh' not found on PATH; install it and run 'gh auth login'.")
    return gh


def _confirm(yes: bool) -> bool:
    if yes:
        return True
    try:
        answer = input("Proceed? (yes/NO): ")
    except EOFError:
        answer = ""
    return answer.strip().lower() == "yes"


def _require_posix() -> None:
    if os.name == "nt":
        raise RuntimeError("prepare/submit drive bootstrap-vcpkg.sh and ./vcpkg; run them from Linux or WSL.")


# --- release -----------------------------------------------------------------

def _hexdigest(path: Path, algo: str) -> str:
    h = hashlib.new(algo)
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url: str, out: Path) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    tmp = out.with_name(out.name + ".part")
    req = Request(url, headers={"User-Agent": "ta-lib-post-release-script"})
    with urlopen(req) as r, tmp.open("wb") as f:
        shutil.copyfileobj(r, f, 1024 * 1024)
    tmp.replace(out)


def fetch_json(url: str) -> dict:
    req = Request(url, headers={"Accept": "application/vnd.github+json", "User-Agent": "ta-lib-post-release-script"})
    with urlopen(req) as r:
        return json.loads(r.read().decode("utf-8"))


def read_latest_release() -> dict:
    """Latest published release: version, tag, html_url, and the source asset's name, URL, size and digest.

    The SHA512 covers the release ASSET, which is what a vcpkg_download_distfile()
    port fetches. A vcpkg_from_github() port hashes GitHub's tag archive instead.
    """
    data = fetch_json(API_RELEASE_LATEST)
    tag = data.get("tag_name", "")
    if not tag:
        raise RuntimeError("Latest release has no tag_name")
    version = tag.lstrip("v")
    tarball = f"ta-lib-{version}-src.tar.gz"
    for asset in data.get("assets", []):
        if asset.get("name") == tarball and asset.get("browser_download_url"):
            return {
                "version": version,
                "tag": tag,
                "html_url": data.get("html_url") or f"https://github.com/{MONITOR_REPO}/releases/tag/{tag}",
                "tarball": tarball,
                "asset_url": asset["browser_download_url"],
                "size": asset.get("size"),
                "digest": asset.get("digest") or "",
            }
    raise RuntimeError(
        f"Release {tag} has no downloadable '{tarball}' asset.\n"
        "release-step-1 attaches it; re-run that workflow before updating the port."
    )


def _tarball_matches(path: Path, release: dict) -> bool:
    if not path.exists():
        return False
    if isinstance(release["size"], int) and path.stat().st_size != release["size"]:
        return False
    algo, _, want = release["digest"].partition(":")
    return algo != "sha256" or _hexdigest(path, "sha256") == want


def release_sha512(release: dict, out_dir: Path) -> str:
    tar_path = out_dir / release["tarball"]
    if _tarball_matches(tar_path, release):
        print(f"Using cached tarball: {tar_path}")
    else:
        print(f"Downloading {release['asset_url']}")
        download(release["asset_url"], tar_path)
        if not _tarball_matches(tar_path, release):
            raise RuntimeError(f"{tar_path} does not match the release asset's size/digest.")
    return _hexdigest(tar_path, "sha512")


# --- guards ------------------------------------------------------------------

def ta_lib_repo() -> Path:
    """The TA-Lib checkout holding this script, which must be TA-Lib/ta-lib itself."""
    os.chdir(REPO_ROOT)
    try:
        return Path(verify_git_repo_original())
    except SystemExit as e:
        raise RuntimeError(f"{REPO_ROOT} is not a checkout of {MONITOR_REPO} (a fork, or no origin).") from e


def guard_release_tag_in_head(repo: Path, tag: str) -> None:
    if not _succeeds(["git", "rev-parse", "--verify", "--quiet", f"refs/tags/{tag}"], cwd=repo):
        raise RuntimeError(f"Release tag '{tag}' is not in this repository; run 'git fetch --tags'.")
    if not _succeeds(["git", "merge-base", "--is-ancestor", f"refs/tags/{tag}", "HEAD"], cwd=repo):
        raise RuntimeError(f"Release tag '{tag}' is not an ancestor of HEAD; update this checkout first.")


def guard_version_not_in(vcpkg_root: Path, ref: str, version: str) -> None:
    data = json.loads(_run(["git", "show", f"{ref}:versions/t-/talib.json"], cwd=vcpkg_root))
    for entry in data.get("versions", []):
        if any(entry.get(k) == version for k in VERSION_KEYS):
            raise RuntimeError(f"talib {version} is already in microsoft/vcpkg master; nothing to submit.")


def gh_login() -> str:
    login = _run([_gh(), "api", "user", "--jq", ".login"])
    if not login:
        raise RuntimeError("'gh api user' returned no login; run 'gh auth login'.")
    return login


def pr_title_matches(title: str, version: str) -> bool:
    return re.search(rf"\[{re.escape(VCPKG_PORT_NAME)}\].*(?<![\w.])v?{re.escape(version)}(?!\.?\w)",
                     title, re.IGNORECASE) is not None


def find_prs(version: str, login: str) -> tuple[tuple[str, str] | None, list[str]]:
    """(your OPEN or MERGED PR from ta-lib-<ver> as (state, url), or None;
    URLs of any other OPEN microsoft/vcpkg PR titled for this port and version).

    Your PR closed without merging does not count: re-submitting after one is legitimate.
    """
    gh = _gh()
    own = None
    out = _run([gh, "pr", "list", "--repo", VCPKG_UPSTREAM_REPO, "--head", f"ta-lib-{version}",
                "--state", "all", "--limit", "100", "--json", "url,state,headRepositoryOwner"])
    for pr in json.loads(out or "[]"):
        owner = ((pr.get("headRepositoryOwner") or {}).get("login") or "").lower()
        state = (pr.get("state") or "").upper()
        if owner != login.lower():
            continue
        if state in ("OPEN", "MERGED"):
            own = (state, pr.get("url", ""))
            break
        print(f"[info] Ignoring your {state.lower()} unmerged PR {pr.get('url', '')}")
    out = _run([gh, "pr", "list", "--repo", VCPKG_UPSTREAM_REPO, "--state", "open", "--limit", "100",
                "--search", f"{VCPKG_PORT_NAME} in:title", "--json", "url,title"])
    others = [pr.get("url", "") for pr in json.loads(out or "[]")
              if pr_title_matches(pr.get("title", ""), version) and (own is None or pr.get("url") != own[1])]
    return own, others


# --- vcpkg checkout ----------------------------------------------------------

def ensure_vcpkg_checkout(vcpkg_root: Path) -> None:
    if not vcpkg_root.exists():
        vcpkg_root.parent.mkdir(parents=True, exist_ok=True)
        _run(["git", "clone", "--depth=1", VCPKG_UPSTREAM_URL, str(vcpkg_root)], echo=True)
    require_vcpkg_checkout(vcpkg_root)
    print(f"[ok] Using vcpkg checkout: {vcpkg_root}")


def require_vcpkg_checkout(vcpkg_root: Path) -> None:
    if not (vcpkg_root / ".git").exists():
        raise RuntimeError(f"{vcpkg_root} is not a git checkout.")
    for marker in VCPKG_MARKERS:
        if not _succeeds(["git", "cat-file", "-e", f"HEAD:{marker}"], cwd=vcpkg_root):
            raise RuntimeError(f"{vcpkg_root} does not look like microsoft/vcpkg: HEAD has no {marker}.")


def fetch_upstream_master(vcpkg_root: Path) -> str:
    """Fetch microsoft/vcpkg master into a private ref; return its commit id."""
    _run(["git", "fetch", "--no-tags", VCPKG_UPSTREAM_URL, f"+master:{UPSTREAM_REF}"], cwd=vcpkg_root, echo=True)
    return _run(["git", "rev-parse", UPSTREAM_REF], cwd=vcpkg_root)


def ensure_vcpkg_tool(vcpkg_root: Path) -> Path:
    """Bootstrap unless ./vcpkg is the tool release the checkout pins; an older binary crashes on master's scripts."""
    meta = (vcpkg_root / "scripts" / "vcpkg-tool-metadata.txt").read_text()
    m = re.search(r"^VCPKG_TOOL_RELEASE_TAG=(\S+)", meta, re.MULTILINE)
    if not m:
        raise RuntimeError("VCPKG_TOOL_RELEASE_TAG not found in scripts/vcpkg-tool-metadata.txt")
    tag = m.group(1)
    exe = vcpkg_root / "vcpkg"

    def matches() -> bool:
        if not exe.exists():
            return False
        try:
            return tag in _run([str(exe), "version"], cwd=vcpkg_root)
        except RuntimeError:
            return False

    if not matches():
        _run([str(vcpkg_root / "bootstrap-vcpkg.sh"), "-disableMetrics"], cwd=vcpkg_root, echo=True)
        if not matches():
            raise RuntimeError(f"{exe} still does not report tool release {tag} after bootstrap.")
    print(f"[ok] vcpkg-tool {tag}")
    return exe


def update_port_files(vcpkg_root: Path, version: str, sha512: str) -> list[str]:
    """Bump the manifest version and the portfile's first SHA512; return warnings for the maintainer."""
    port = vcpkg_root / "ports" / VCPKG_PORT_NAME
    portfile, manifest = port / "portfile.cmake", port / "vcpkg.json"
    if not portfile.exists() or not manifest.exists():
        raise RuntimeError(f"{port} has no portfile.cmake/vcpkg.json.")

    data = json.loads(manifest.read_text())
    key = next((k for k in ("version-semver", "version-string", "version-date", "version") if k in data), "version")
    old = data.get(key)
    for k in VERSION_KEYS:
        data.pop(k, None)
    data[key] = version
    if old != version:
        data.pop("port-version", None)
    manifest.write_text(json.dumps(data, indent=2) + "\n")

    text = portfile.read_text()
    # \g<1>, not \1: a SHA starting with a digit would merge into the group number.
    new_text = re.sub(r"(SHA512\s+)[0-9a-fA-F]{64,128}", rf"\g<1>{sha512}", text, count=1)
    portfile.write_text(new_text)
    print(f"[ok] ports/{VCPKG_PORT_NAME}/vcpkg.json: {key} {version}")

    warnings = []
    if new_text == text and sha512 not in text:
        warnings.append("portfile.cmake has no SHA512 to replace; set it by hand.")
    if "vcpkg_download_distfile" not in text:
        warnings.append("portfile.cmake does not use vcpkg_download_distfile(), but the SHA512 written is the "
                        "release asset's. A vcpkg_from_github() port needs the tag archive's hash, or the port "
                        "moved onto the asset. Fix the port before gating.")
    return warnings


def format_manifest(vcpkg_root: Path, exe: Path) -> None:
    _run([str(exe), "format-manifest", f"ports/{VCPKG_PORT_NAME}/vcpkg.json"], cwd=vcpkg_root, echo=True)


def _in_port_paths(path: str) -> bool:
    return any(path == p or path.startswith(p + "/") for p in PORT_PATHS)


def strays(vcpkg_root: Path) -> str:
    excludes = [f":(exclude){p}" for p in PORT_PATHS]
    return _run(["git", "status", "--porcelain", "--untracked-files=all", "--", "."] + excludes, cwd=vcpkg_root)


def commits_over(vcpkg_root: Path, base: str) -> tuple[int, str]:
    ahead = int(_run(["git", "rev-list", "--count", f"{base}..HEAD"], cwd=vcpkg_root))
    subject = _run(["git", "log", "-1", "--format=%s"], cwd=vcpkg_root) if ahead else ""
    return ahead, subject


def verify_version_db(vcpkg_root: Path, version: str) -> str:
    tree = _run(["git", "rev-parse", f"HEAD:ports/{VCPKG_PORT_NAME}"], cwd=vcpkg_root)
    first = json.loads(_run(["git", "show", "HEAD:versions/t-/talib.json"], cwd=vcpkg_root))["versions"][0]
    if first.get("git-tree") != tree:
        raise RuntimeError(f"versions/t-/talib.json git-tree {first.get('git-tree')} != HEAD:ports/talib {tree}")
    if not any(first.get(k) == version for k in VERSION_KEYS):
        raise RuntimeError(f"versions/t-/talib.json first entry is not {version}: {first}")
    baseline = json.loads(_run(["git", "show", "HEAD:versions/baseline.json"], cwd=vcpkg_root))
    if baseline.get("default", {}).get(VCPKG_PORT_NAME, {}).get("baseline") != version:
        raise RuntimeError(f"versions/baseline.json does not set {VCPKG_PORT_NAME} to {version}")
    leftover = _run(["git", "status", "--porcelain", "--"] + PORT_PATHS, cwd=vcpkg_root)
    if leftover:
        raise RuntimeError(f"Uncommitted port changes after commit:\n{leftover}")
    print(f"[ok] TREE-OK: HEAD:ports/{VCPKG_PORT_NAME} == versions git-tree {tree}")
    return tree


# --- PR ----------------------------------------------------------------------

def pr_body(vcpkg_root: Path, version: str, release_url: str) -> str:
    head = f"Update talib to TA-Lib [{version}]({release_url})."
    template = vcpkg_root / ".github" / "pull_request_template.md"
    text = template.read_text() if template.exists() else ""
    m = re.search(r"please uncomment and fill out this checklist:[^\n]*\n(.*?)^[^\n]*END OF PORT UPDATE CHECKLIST",
                  text, re.DOTALL | re.MULTILINE)
    checklist = m.group(1).strip() if m else ""
    if not checklist:
        return head + "\n"
    return f"{head}\n\n{checklist}\n"


def _normalize_github_url(url: str) -> str:
    u = re.sub(r"^[a-z+]+://", "", url.strip().lower())
    u = re.sub(r"^[^@/]+@", "", u)
    u = u.replace("github.com:", "github.com/", 1)
    return re.sub(r"(\.git)?/*$", "", u)


def fork_remote(vcpkg_root: Path, login: str) -> tuple[str, str, list[str] | None]:
    """(remote name, its URL, the git remote command to run first or None) for github.com/<login>/vcpkg."""
    want = f"github.com/{login.lower()}/vcpkg"
    names = _run(["git", "remote"], cwd=vcpkg_root).split()
    for name in names:
        url = _run(["git", "remote", "get-url", name], cwd=vcpkg_root)
        if _normalize_github_url(url) == want:
            return name, url, None
    url = f"https://github.com/{login}/vcpkg.git"
    verb = "set-url" if "fork" in names else "add"
    return "fork", url, ["git", "remote", verb, "fork", url]


def fork_branch_sha(vcpkg_root: Path, url: str, branch: str) -> str:
    """Commit id of <branch> on the fork, or "" when the branch does not exist."""
    out = _run(["git", "ls-remote", url, f"refs/heads/{branch}"], cwd=vcpkg_root)
    return out.split()[0] if out else ""


def monitor_title(version: str) -> str:
    return f"[monitor] VCPkg release {version}"


def find_monitor_issue(version: str) -> str | None:
    """URL of the issue titled exactly for this version, open or closed; None when there is none.

    Raises RuntimeError or ValueError when the full list cannot be read.
    """
    # The plain issue list, not `--search`: the search index lags, so a prompt
    # re-run could miss a seconds-old issue and open a duplicate.
    out = _run([_gh(), "issue", "list", "--repo", MONITOR_REPO, "--state", "all",
                "--limit", str(MONITOR_LIST_LIMIT), "--json", "title,url"])
    issues = json.loads(out or "[]")
    if len(issues) >= MONITOR_LIST_LIMIT:
        raise RuntimeError(f"{MONITOR_REPO} has at least {MONITOR_LIST_LIMIT} issues; the list may be truncated.")
    title = monitor_title(version)
    return next((i.get("url", "") for i in issues if i.get("title", "") == title), None)


def _monitor_manual_step(version: str, pr_url: str) -> None:
    print(f"       Re-run submit, or open '{monitor_title(version)}' in {MONITOR_REPO} by hand,")
    print(f"       linking {pr_url or 'the microsoft/vcpkg PR'} and assigning {', '.join(MONITOR_ASSIGNEES)}.")


def open_monitor_issue(version: str, pr_url: str) -> bool:
    """Open the monitor issue and assign the maintainers, without checking for an existing one."""
    title = monitor_title(version)
    body = (
        f"Tracking the microsoft/vcpkg port update for TA-Lib **{version}**.\n\n"
        f"- vcpkg PR: {pr_url}\n\n"
        "vcpkg PRs are reviewed and merged by the vcpkg maintainers, which usually "
        "takes a few days. This issue is a reminder to watch that PR through to merge.\n\n"
        f"**Close this issue** once the vcpkg PR is merged and `vcpkg install {VCPKG_PORT_NAME}` "
        f"installs {version}.\n\n"
        "_Opened automatically by `scripts/post-release-vcpkg.py`._"
    )
    try:
        gh = _gh()
        out = _run([gh, "issue", "create", "--repo", MONITOR_REPO, "--title", title, "--body", body])
    except RuntimeError as e:
        print(f"[warn] Could not open the vcpkg monitor issue: {e}")
        _monitor_manual_step(version, pr_url)
        return False
    issue_url = out.splitlines()[-1].strip() if out else ""
    print(f"[ok] Opened vcpkg monitor issue: {issue_url or '(created)'}")
    if not issue_url:
        print(f"[warn] Could not determine the monitor issue URL; assign {', '.join(MONITOR_ASSIGNEES)} manually.")
        return True

    # gh applies --add-assignee atomically: one non-assignable login drops them all,
    # so retry each login alone when the combined call fails.
    def _assign(logins: list[str]) -> bool:
        edit_args: list[str] = []
        for lg in logins:
            edit_args += ["--add-assignee", lg]
        try:
            _run([gh, "issue", "edit", issue_url] + edit_args)
            return True
        except RuntimeError:
            return False

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
    return True


def create_monitor_issue(version: str, pr_url: str) -> bool:
    """Open the '[monitor] VCPkg release <version>' issue unless one exists. False when it is not in place."""
    try:
        existing = find_monitor_issue(version)
    except (RuntimeError, ValueError) as e:
        print(f"[warn] Could not list {MONITOR_REPO} issues, so no monitor issue was opened: {e}")
        _monitor_manual_step(version, pr_url)
        return False
    if existing:
        print(f"[ok] vcpkg monitor issue already exists: {existing}")
        return True
    return open_monitor_issue(version, pr_url)


# --- subcommands -------------------------------------------------------------

def cmd_plan(args) -> int:
    release = read_latest_release()
    sha512 = release_sha512(release, args.out_dir)
    print("\n=== TA-Lib vcpkg update plan ===")
    print(f"Version : {release['version']}")
    print(f"Asset   : {release['asset_url']}")
    print(f"SHA512  : {sha512}")
    return 0


def cmd_prepare(args) -> int:
    _require_posix()
    vcpkg_root: Path = args.vcpkg_root
    release = read_latest_release()
    version = release["version"]
    branch = f"ta-lib-{version}"

    repo = ta_lib_repo()
    guard_release_tag_in_head(repo, release["tag"])
    own, others = find_prs(version, gh_login())
    if own:
        state, url = own
        raise RuntimeError(f"Your {state.lower()} PR for {version} already exists: {url}\n"
                           "Run 'submit' to reconcile the monitor issue.")
    if others:
        raise RuntimeError(f"Open microsoft/vcpkg PR(s) already update {VCPKG_PORT_NAME} to {version}:\n  "
                           + "\n  ".join(others))

    sha512 = release_sha512(release, args.out_dir)

    ensure_vcpkg_checkout(vcpkg_root)
    dirty = _run(["git", "status", "--porcelain", "--untracked-files=all"], cwd=vcpkg_root)
    if dirty:
        raise RuntimeError(f"{vcpkg_root} has uncommitted changes; commit, stash or discard them first:\n{dirty}")
    base = fetch_upstream_master(vcpkg_root)
    guard_version_not_in(vcpkg_root, base, version)
    old_tip = subprocess.run(["git", "rev-parse", "--verify", "--quiet", f"refs/heads/{branch}"],
                             capture_output=True, text=True, cwd=vcpkg_root).stdout.strip()
    _run(["git", "checkout", "-B", branch, base], cwd=vcpkg_root, echo=True)
    if old_tip and old_tip != base:
        print(f"[info] {branch} was at {old_tip[:12]}; 'git reflog' recovers it.")

    exe = ensure_vcpkg_tool(vcpkg_root)
    warnings = update_port_files(vcpkg_root, version, sha512)
    format_manifest(vcpkg_root, exe)

    print()
    print(_run(["git", "diff", "--stat"], cwd=vcpkg_root) or "(no changes)")
    for w in warnings:
        print(f"\n[WARN] {w}")
    gate = "\n".join(
        f"     ./vcpkg x-test-features --for-merge-with {base} --triplet={t} \\\n"
        f"       --x-buildtrees-root=$T/bt --x-install-root=$T/inst --x-packages-root=$T/pk --binarysource=clear"
        for t in GATE_TRIPLETS
    )
    root_flag = "" if vcpkg_root == DEFAULT_OUT_DIR / "vcpkg" else f" --vcpkg-root {shlex.quote(str(vcpkg_root))}"
    print(f"""
Next steps:
  1. Edit {vcpkg_root}/ports/{VCPKG_PORT_NAME} if {version} needs more than the
     version/SHA512 bump (e.g. delete patches that no longer apply).
  2. Gate. Each run must print "Building {VCPKG_PORT_NAME}:<triplet>@{version}" and
     "All feature tests passed", and exit 0. `./vcpkg install {VCPKG_PORT_NAME}` is not a gate:
     its post-build lint only warns.
     cd {shlex.quote(str(vcpkg_root))} && T=$(mktemp -d)
{gate}
     rm -rf "$T"
  3. {repo}/scripts/{Path(__file__).name} submit{root_flag}
""")
    return 0


def reconcile_monitor_issue(version: str, pr_url: str, state: str, branch: str, yes: bool) -> int:
    if state == "OPEN":
        print(f"[info] To change the open PR, edit the port, then in the vcpkg checkout:\n"
              f"       ./vcpkg x-add-version {VCPKG_PORT_NAME} --overwrite-version && "
              f"git add -- {' '.join(PORT_PATHS)} && git commit --amend --no-edit && "
              f"git push --force-with-lease <fork remote> {branch}")
    try:
        existing = find_monitor_issue(version)
    except (RuntimeError, ValueError) as e:
        print(f"[warn] Could not list {MONITOR_REPO} issues: {e}")
        _monitor_manual_step(version, pr_url)
        return 1
    if existing:
        print(f"[ok] vcpkg monitor issue already exists: {existing}")
        return 0
    print(f"\nThis will open '{monitor_title(version)}' in {MONITOR_REPO}, linking {pr_url},"
          f" assigned to {', '.join(MONITOR_ASSIGNEES)}.")
    if not _confirm(yes):
        print("Cancelled; nothing was created.")
        return 0
    return 0 if open_monitor_issue(version, pr_url) else 1


def cmd_submit(args) -> int:
    _require_posix()
    vcpkg_root: Path = args.vcpkg_root
    release = read_latest_release()
    version = release["version"]
    branch = f"ta-lib-{version}"
    title = f"[{VCPKG_PORT_NAME}] update to {version}"

    ta_lib_repo()
    login = gh_login()
    own, others = find_prs(version, login)
    if own:
        state, url = own
        print(f"[info] Your {state.lower()} PR for {version} already exists: {url}")
        return reconcile_monitor_issue(version, url, state, branch, args.yes)
    if others and args.yes:
        raise RuntimeError(f"Open microsoft/vcpkg PR(s) already update {VCPKG_PORT_NAME} to {version}:\n  "
                           + "\n  ".join(others) + "\nRe-run without --yes to review and confirm.")

    require_vcpkg_checkout(vcpkg_root)
    on = _run(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=vcpkg_root)
    if on != branch:
        raise RuntimeError(f"{vcpkg_root} is on '{on}', not '{branch}'; run 'prepare' first.")
    stray = strays(vcpkg_root)
    if stray:
        raise RuntimeError(f"Changes outside {', '.join(PORT_PATHS)}; remove them first:\n{stray}")

    base = fetch_upstream_master(vcpkg_root)
    guard_version_not_in(vcpkg_root, base, version)
    ahead, subject = commits_over(vcpkg_root, base)
    if ahead > 1 or (ahead == 1 and subject != title):
        raise RuntimeError(f"{branch} carries {ahead} commit(s) over microsoft/vcpkg master (HEAD: '{subject}'); "
                           f"expected none, or one '{title}'.")

    exe = ensure_vcpkg_tool(vcpkg_root)
    format_manifest(vcpkg_root, exe)
    _run([str(exe), "x-add-version", VCPKG_PORT_NAME, "--overwrite-version"], cwd=vcpkg_root, echo=True)
    _run(["git", "add", "--"] + PORT_PATHS, cwd=vcpkg_root)
    if _succeeds(["git", "diff", "--cached", "--quiet"], cwd=vcpkg_root):
        print("[ok] Nothing new to commit.")
    elif ahead == 1:
        _run(["git", "commit", "--amend", "--no-edit"], cwd=vcpkg_root, echo=True)
    else:
        _run(["git", "commit", "-m", title], cwd=vcpkg_root, echo=True)

    verify_version_db(vcpkg_root, version)
    ahead, subject = commits_over(vcpkg_root, base)
    if ahead != 1 or subject != title:
        raise RuntimeError(f"{branch} must be exactly one '{title}' commit over master {base[:12]}.")
    outside = [n for n in _run(["git", "diff", "--name-only", f"{base}...HEAD"], cwd=vcpkg_root).splitlines()
               if not _in_port_paths(n)]
    if outside:
        raise RuntimeError("The commit touches files outside the port:\n  " + "\n  ".join(outside))

    gh = "gh"
    if not _succeeds([_gh(), "repo", "view", f"{login}/vcpkg", "--json", "name"]):
        raise RuntimeError(f"No fork {login}/vcpkg; create it with 'gh repo fork {VCPKG_UPSTREAM_REPO} --clone=false'.")

    args.out_dir.mkdir(parents=True, exist_ok=True)
    body_file = args.out_dir / f"vcpkg-pr-body-{version}.md"
    if not body_file.exists():
        body_file.write_text(pr_body(vcpkg_root, version, release["html_url"]))

    remote, remote_url, remote_cmd = fork_remote(vcpkg_root, login)
    lease = fork_branch_sha(vcpkg_root, remote_url, branch)
    network = [
        [gh, "repo", "sync", f"{login}/vcpkg", "--branch", "master"],
        ["git", "push", f"--force-with-lease=refs/heads/{branch}:{lease}", "-u", remote, branch],
        [gh, "pr", "create", "--repo", VCPKG_UPSTREAM_REPO, "--draft", "--base", "master",
         "--head", f"{login}:{branch}", "--title", title, "--body-file", str(body_file)],
    ]

    print()
    print(_run(["git", "diff", "--stat", "HEAD^", "HEAD"], cwd=vcpkg_root))
    print(f"\n--- PR body ({body_file}; edit it before answering if you like) ---")
    print(body_file.read_text().rstrip())
    print(f"---\n\n{remote_url} {branch}: {lease or 'absent'}")
    if others:
        print(f"\n[WARN] Open microsoft/vcpkg PR(s) already update {VCPKG_PORT_NAME} to {version}:")
        for u in others:
            print(f"  {u}")
    print(f"\nThis will run, in {vcpkg_root}")
    for c in ([remote_cmd] if remote_cmd else []) + network:
        print(f"  {shlex.join(c)}")
    print(f"  then open '{monitor_title(version)}' in {MONITOR_REPO} if missing,"
          f" assigned to {', '.join(MONITOR_ASSIGNEES)}")
    if not _confirm(args.yes):
        print("Cancelled; nothing was pushed.")
        return 0

    if remote_cmd:
        _run(remote_cmd, cwd=vcpkg_root, echo=True)
    # A push from a shallow clone is refused until the fork's master holds the base.
    _run(network[0], echo=True)
    _run(network[1], cwd=vcpkg_root, echo=True)
    out = _run(network[2], cwd=vcpkg_root, echo=True)
    pr_url = next((ln.strip() for ln in reversed(out.splitlines()) if ln.strip().startswith("http")), "")
    print(f"[ok] Opened draft PR: {pr_url or out}")
    monitored = create_monitor_issue(version, pr_url)
    print(f"""
Before 'gh pr ready {pr_url or '<pr>'}':
  - vcpkg CI is green, and
  - the Azure "file lists for <triplet>" artifacts show {VCPKG_PORT_NAME} was actually built.
Then tick the checklist in the PR body.""")
    return 0 if monitored else 1


def main() -> int:
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--out-dir", default=str(DEFAULT_OUT_DIR),
                        help="Release tarball cache and PR body file (default: <repo>/temp/post-release-vcpkg).")
    vcpkg = argparse.ArgumentParser(add_help=False)
    vcpkg.add_argument("--vcpkg-root",
                       help="microsoft/vcpkg checkout; prepare clones it --depth=1 if missing (default: <out-dir>/vcpkg).")

    p = argparse.ArgumentParser(
        description="Update the talib port in microsoft/vcpkg after a TA-Lib release.",
        epilog="Order: plan, prepare, local x-test-features gate, submit.",
    )
    sub = p.add_subparsers(dest="cmd", metavar="{plan,prepare,submit}")
    sub.add_parser("plan", parents=[common], help="Print version, asset URL and SHA512 (read-only).")
    sub.add_parser("prepare", parents=[common, vcpkg],
                   help="Branch from vcpkg master and bump the port locally (no network writes).")
    s = sub.add_parser("submit", parents=[common, vcpkg],
                       help="x-add-version, commit, then push to your fork and open a draft PR.")
    s.add_argument("--yes", action="store_true", help="Skip the confirmation prompt.")
    args = p.parse_args()

    if args.cmd is None:
        p.print_help()
        return 2

    args.out_dir = Path(args.out_dir).expanduser().resolve()
    if hasattr(args, "vcpkg_root"):
        root = Path(args.vcpkg_root).expanduser() if args.vcpkg_root else args.out_dir / "vcpkg"
        args.vcpkg_root = root.resolve()

    return {"plan": cmd_plan, "prepare": cmd_prepare, "submit": cmd_submit}[args.cmd](args)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as e:
        print(f"Error: {e}")
        raise SystemExit(1)
