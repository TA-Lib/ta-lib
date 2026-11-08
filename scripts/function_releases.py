#!/usr/bin/env python3
"""Derive the release in which each function first shipped, from the release tags.

Rule: tags only, never the presence of a source file. A tag `vM.m.p` is a
release; `vM.m.p-rc.N` is attributed to that final release. Tags are walked in
version order and a function belongs to the first release that registers it:
`ta_func_list.txt` where the tag has one (0.6.0 on), else the uncommented
`ADD_TO_TABLE(NAME)` lines of `src/ta_abstract/tables/table_*.c`.

A row whose version has no tag is pending: the functions added since the last
release, kept under the current VERSION. `--out` writes the tagged rows only;
`--sync` (what scripts/sync.py runs) appends the functions missing from every row under VERSION;
`--check` holds every tagged row to the tags and the pending row to the corpus.

  function_releases.py --out ta_codegen/input/releases.yaml   # tagged rows, from the tags
  function_releases.py --sync                                 # the ensure step of sync.py
  function_releases.py --check ta_codegen/input/releases.yaml # exit 1 on drift

Every mode prints the unreleased functions (in the corpus, in no tagged release)
and reports anomalies on stderr.
"""
import argparse
import difflib
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LEDGER = Path("ta_codegen/input/releases.yaml")
TAG_RE = re.compile(r"^v(\d+)\.(\d+)\.(\d+)(?:-rc\.(\d+))?$")
ROW_RE = re.compile(r"^(\d+\.\d+\.\d+): \[(.*)\]$")
TABLES = "src/ta_abstract/tables/"
HEADER = (
    "# Release in which each function first shipped. Derived from the release tags by\n"
    "# scripts/function_releases.py: a tag's ta_func_list.txt, else its uncommented\n"
    "# ADD_TO_TABLE lines; an rc counts for its final release. The row whose version\n"
    "# has no tag is pending: scripts/sync.py keeps it at VERSION and appends there\n"
    "# every function of ta_codegen/input missing here. From 0.8.1 on, ta-lib-python\n"
    "# ships each function under the same version; its earlier releases did not.\n"
)


def git(*args):
    return subprocess.run(["git", "-C", str(REPO), *args], check=True,
                          capture_output=True, text=True).stdout


def version_key(version):
    return tuple(int(x) for x in version.split("."))


def release_tags():
    """[(release, tag)] in version order, an rc before its final release."""
    tags = []
    for tag in git("tag", "--list", "v*").split():
        m = TAG_RE.match(tag)
        if m:
            major, minor, patch, rc = m.groups()
            key = (int(major), int(minor), int(patch), int(rc) if rc else sys.maxsize)
            tags.append((key, f"{major}.{minor}.{patch}", tag))
    tags.sort()
    finals = {rel for key, rel, _ in tags if key[3] == sys.maxsize}
    for _, rel, tag in tags:
        if rel not in finals:
            sys.exit(f"{tag}: release candidate without a final release")
    return [(rel, tag) for _, rel, tag in tags]


def registered(tag):
    if subprocess.run(["git", "-C", str(REPO), "cat-file", "-e", f"{tag}:ta_func_list.txt"],
                      capture_output=True).returncode == 0:
        return {line.split()[0] for line in git("show", f"{tag}:ta_func_list.txt").splitlines()
                if line.strip()}
    files = git("ls-tree", "--name-only", tag, TABLES).split()
    if not files:
        sys.exit(f"{tag}: neither ta_func_list.txt nor {TABLES}")
    source = git("show", *(f"{tag}:{f}" for f in files))
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.S)
    return set(re.findall(r"^\s*ADD_TO_TABLE\((\w+)\)", source, flags=re.M))


def corpus(root=REPO):
    return {d.name.upper() for d in (root / "ta_codegen/input").iterdir()
            if d.is_dir() and d.name not in ("helpers", "lib")}


def parse_ledger(text, where):
    """{version: set(names)}; the header comment is the script's, not the file's."""
    rows = {}
    for line in text.splitlines():
        if not line.strip() or line.startswith("#"):
            continue
        m = ROW_RE.match(line)
        if not m:
            sys.exit(f"{where}: not a `M.m.p: [NAME, ...]` row: {line}")
        rows[m.group(1)] = {n.strip() for n in m.group(2).split(",") if n.strip()}
    return rows


def format_ledger(rows):
    return HEADER + "".join(f"{v}: [{', '.join(sorted(rows[v]))}]\n"
                            for v in sorted(rows, key=version_key))


def sync_ledger(root, version):
    """Ensure every corpus function has a row: the missing ones are appended under VERSION.

    Returns the messages to print. Writes only on a change. A row whose version has no
    tag here is reported, never moved: a clone may simply lack the tag.
    """
    path = root / LEDGER
    existing = path.read_text() if path.exists() else ""
    rows = parse_ledger(existing, path)
    tagged = {rel for rel, _ in release_tags()}
    messages = []
    for v in sorted((v for v in rows if v != version and v not in tagged), key=version_key):
        messages.append(f"{LEDGER}: no tag v{v} here; its row is left as it is")
    listed = set().union(*rows.values()) if rows else set()
    missing = sorted(corpus(root) - listed)
    if missing:
        rows.setdefault(version, set()).update(missing)
        messages.append(f"{LEDGER}: added under {version}: {' '.join(missing)}")
    text = format_ledger(rows)
    if text != existing:
        path.write_text(text)
        messages.append(f"Updated {LEDGER}")
    else:
        messages.append(f"No changes to {LEDGER}")
    return messages


def derive():
    funcs = corpus()
    current = {line.split()[0] for line in (REPO / "ta_func_list.txt").read_text().splitlines()
               if line.strip()}
    for name in sorted(funcs - current):
        print(f"anomaly: {name} is in ta_codegen/input but not in ta_func_list.txt", file=sys.stderr)
    for name in sorted(current - funcs):
        print(f"anomaly: {name} is in ta_func_list.txt but not in ta_codegen/input", file=sys.stderr)

    first, seen = {}, set()
    for rel, tag in release_tags():
        names = registered(tag)
        for name in sorted(seen - names):
            print(f"anomaly: {name} registered before {tag} but absent at it", file=sys.stderr)
        for name in sorted(names - funcs):
            print(f"anomaly: {name} registered at {tag} but not in ta_codegen/input", file=sys.stderr)
        for name in names & funcs:
            first.setdefault(name, rel)
        seen |= names

    rows = {}
    for name, rel in first.items():
        rows.setdefault(rel, set()).add(name)
    return rows, sorted(funcs - set(first))


def check(path, derived, unreleased):
    """Tagged rows must equal the derivation; the pending rows must hold exactly the
    unreleased functions. Returns the exit status."""
    committed = path.read_text() if path.exists() else ""
    rows = parse_ledger(committed, path)
    tagged = set(derived)
    kept = {v: names for v, names in rows.items() if v in tagged}
    pending = {v: names for v, names in rows.items() if v not in tagged}
    status = 0
    if format_ledger(kept) != format_ledger(derived):
        sys.stdout.writelines(difflib.unified_diff(
            format_ledger(kept).splitlines(True), format_ledger(derived).splitlines(True),
            f"{path} (tagged rows)", "derived from the release tags"))
        status = 1
    in_pending = set().union(*pending.values()) if pending else set()
    corpus_names = corpus()
    for name in sorted(set(unreleased) - in_pending):
        print(f"{path}: {name} is in no row; run scripts/sync.py")
        status = 1
    for name in sorted(in_pending - set(unreleased)):
        if name not in corpus_names:
            print(f"{path}: {name} is not in ta_codegen/input; remove it from the ledger")
        else:
            print(f"{path}: {name} is in a pending row but shipped in a tagged release")
        status = 1
    if len(pending) > 1:
        print(f"{path}: {len(pending)} untagged rows ({', '.join(sorted(pending, key=version_key))}); "
              "fetch the tags, or fold the stale row into VERSION by hand")
        status = 1
    for v in pending:
        print(f"pending (untagged): {v}: {' '.join(sorted(pending[v])) or '(none)'}")
    if status:
        print(f"{path} is stale")
    return status


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = ap.add_mutually_exclusive_group(required=True)
    mode.add_argument("--out", type=Path, help="write the tagged rows here")
    mode.add_argument("--sync", action="store_true",
                      help=f"add the pending row to {LEDGER} at VERSION (what scripts/sync.py runs)")
    mode.add_argument("--check", type=Path, help="compare the ledger here with the derivation")
    args = ap.parse_args()
    if args.sync:
        version = (REPO / "VERSION").read_text().strip()
        for message in sync_ledger(REPO, version):
            print(message)
        return 0
    derived, unreleased = derive()
    print("unreleased:", " ".join(unreleased) or "(none)")
    if args.out:
        args.out.write_text(format_ledger(derived))
        return 0
    return check(args.check, derived, unreleased)


if __name__ == "__main__":
    sys.exit(main())
