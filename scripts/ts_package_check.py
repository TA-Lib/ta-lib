#!/usr/bin/env python3
"""Package gate for the TypeScript library: pack it, check what ships, then consume it.

  1. Pack. `npm ci` in the tooling root when its lock stamp is stale, a fresh
     `tsc` into an emptied library/dist, then `npm pack`. Without --allow-dirty
     the library directory (test/ and dist/ aside) must match HEAD: a stray .ts
     under src/ compiles into dist/ and ships.

  2. Contents. The tarball holds exactly package.json, README.md, LICENSE and
     one .js plus one .d.ts per src/ module; LICENSE and README are
     byte-identical to their sources; the version is VERSION; every `exports`
     subpath lists "types" first and "default" last, each target ships, and
     each non-root subpath has a typesVersions entry naming its types.
     No name tagged @internal in src/ appears in a packed .d.ts, as a member of
     its class's declaration, or among any subpath's runtime exports. When
     ta_codegen/output/ts/PublicAPI.Shipped.txt exists, every declaration line
     it records must still be packed; until the first publish it is absent and
     that diff is skipped.

  3. Consumer. scripts/ts_consumer_check/ installs the tarball into a throwaway
     project, type-checks a TypeScript caller against the packed declarations
     (skipLibCheck off), and loads every subpath by import and by require(),
     which fails on a top-level await anywhere in the graph. Both must export
     the same names and compute the same SMA.

Run:  python3 scripts/ts_package_check.py [--allow-dirty] [--output DIR]
      python3 scripts/ts_package_check.py --write-api-snapshot

`--write-api-snapshot` records the packed declarations as the shipped API; the
publish runbook runs it once per publish.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TS_REL = "ta_codegen/output/ts"
TS_ROOT = os.path.join(ROOT, TS_REL)
LIB_REL = TS_REL + "/library"
LIB_DIR = os.path.join(ROOT, LIB_REL)
# npm and the tools it installs are .cmd shims on Windows; subprocess does not
# resolve a bare name to one.
NPM = "npm.cmd" if os.name == "nt" else "npm"
TSC = os.path.join(TS_ROOT, "node_modules", ".bin", "tsc.cmd" if os.name == "nt" else "tsc")
SRC_DIR = os.path.join(LIB_DIR, "src")
CONSUMER_DIR = os.path.join(ROOT, "scripts", "ts_consumer_check")
SNAPSHOT = os.path.join(TS_ROOT, "PublicAPI.Shipped.txt")

PACKED_FROM = {
    "package/LICENSE": "LICENSE",
    "package/README.md": LIB_REL + "/README.md",
}

TOP_DECL = re.compile(
    r"^export\s+(?:declare\s+)?(?:abstract\s+)?(?:async\s+)?"
    r"(?:function\*?|const|let|var|class|interface|type)\s+([A-Za-z_$][\w$]*)")
MEMBER_DECL = re.compile(
    r"^\s+(?:(?:public|protected|static|readonly|override|abstract|declare|get|set|async)\s+)*"
    r"([A-Za-z_$][\w$]*)\s*[?!]?\s*[:=(<;]")
CLASS_START = re.compile(r"^(?:export\s+)?(?:declare\s+)?(?:abstract\s+)?class\s+([A-Za-z_$][\w$]*)")


def run(cmd, cwd=ROOT, capture=False):
    print("=== %s" % " ".join(cmd), flush=True)
    return subprocess.run(cmd, cwd=cwd, check=False, text=True,
                          stdout=subprocess.PIPE if capture else None,
                          stderr=subprocess.PIPE if capture else None)


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True,
                          capture_output=True, text=True).stdout


def version():
    with open(os.path.join(ROOT, "VERSION")) as f:
        return f.read().strip()


def src_modules():
    """Every src/ module, as its path relative to src/ without the .ts."""
    found = []
    for dirpath, _dirs, files in os.walk(SRC_DIR):
        for name in files:
            if name.endswith(".ts"):
                rel = os.path.relpath(os.path.join(dirpath, name), SRC_DIR)
                found.append(rel[:-3].replace(os.sep, "/"))
    return sorted(found)


def ensure_tooling():
    """`npm ci` unless node_modules/.ta-lock.sha256 holds the lockfile's SHA-256,
    the stamp `ta_codegen build` keeps."""
    with open(os.path.join(TS_ROOT, "package-lock.json"), "rb") as f:
        want = hashlib.sha256(f.read()).hexdigest()
    stamp = os.path.join(TS_ROOT, "node_modules", ".ta-lock.sha256")
    try:
        with open(stamp) as f:
            if f.read().strip() == want:
                return True
    except OSError:
        pass
    if run([NPM, "ci", "--no-audit", "--no-fund"], cwd=TS_ROOT).returncode:
        return False
    with open(stamp, "w") as f:
        f.write(want)
    return True


def pack(out_dir, allow_dirty):
    if not allow_dirty:
        stray = [line for line in git("status", "--porcelain", "--ignored",
                                      "--untracked-files=all", "--", LIB_REL).splitlines()
                 if not line[3:].startswith((LIB_REL + "/test/", LIB_REL + "/dist/"))]
        if stray:
            return ["the library directory differs from HEAD, and what differs would pack "
                    "(pass --allow-dirty to pack it anyway): %s" % ", ".join(stray[:10])]
    if not ensure_tooling():
        return ["npm ci failed in %s; see its output above" % TS_REL]
    shutil.rmtree(os.path.join(LIB_DIR, "dist"), ignore_errors=True)
    if run([TSC, "-p", LIB_DIR]).returncode:
        return ["tsc failed on the library; see its output above"]
    if run([NPM, "pack", "--pack-destination", out_dir], cwd=LIB_DIR).returncode:
        return ["npm pack failed; see its output above"]
    return []


def tarball_entries(path):
    with tarfile.open(path, "r:gz") as tar:
        return {m.name: tar.extractfile(m).read() for m in tar.getmembers() if m.isfile()}


def internal_names():
    """({top-level name: module}, [(module, class, member)], failures) for every
    declaration whose doc comment carries @internal."""
    tops, members, failures = {}, [], []
    for module in src_modules():
        path = os.path.join(SRC_DIR, module + ".ts")
        with open(path, encoding="utf-8") as f:
            lines = f.read().splitlines()
        cls = None
        i = 0
        while i < len(lines):
            line = lines[i]
            m = CLASS_START.match(line)
            if m:
                cls = m.group(1)
            elif line.startswith("}"):
                cls = None
            if line.lstrip().startswith("/**"):
                start = i
                while "*/" not in lines[i]:
                    i += 1
                block = "\n".join(lines[start:i + 1])
                if re.search(r"(?:^|[\s*])@internal\b", block):
                    j = i + 1
                    while j < len(lines) and not lines[j].strip():
                        j += 1
                    decl = lines[i].split("*/", 1)[1].strip() or (lines[j] if j < len(lines) else "")
                    top = TOP_DECL.match(decl)
                    reexport = re.match(r"^export\s+(?:type\s+)?\{([^}]*)\}", decl)
                    member = MEMBER_DECL.match(decl) if decl[:1].isspace() else None
                    if top:
                        tops[top.group(1)] = module
                    elif reexport:
                        for part in reexport.group(1).split(","):
                            if part.strip():
                                tops[part.split(" as ")[-1].strip()] = module
                    elif member and cls:
                        members.append((module, cls, member.group(1)))
                    elif not decl.startswith(("function ", "const ", "let ", "class ")):
                        failures.append("%s/src/%s.ts:%d: cannot read the declaration "
                                        "@internal tags: %r" % (LIB_REL, module, j + 1, decl))
            i += 1
    return tops, members, failures


def class_body(dts, cls):
    m = re.search(r"^(?:export\s+)?declare\s+(?:abstract\s+)?class\s+%s\b.*?^}" % re.escape(cls),
                  dts, re.S | re.M)
    return m.group(0) if m else ""


def declarations(entries):
    """{packed .d.ts path: its lines with comments and blank lines dropped}."""
    out = {}
    for name in sorted(n for n in entries if n.endswith(".d.ts")):
        text = re.sub(r"/\*.*?\*/", "", entries[name].decode("utf-8"), flags=re.S)
        out[name] = [line.rstrip() for line in text.splitlines()
                     if line.strip() and not line.lstrip().startswith("//")]
    return out


def write_snapshot(entries):
    with open(SNAPSHOT, "w", encoding="utf-8", newline="\n") as f:
        for name, lines in declarations(entries).items():
            f.write("// %s\n" % name)
            for line in lines:
                f.write(line + "\n")
    print("Wrote %s" % os.path.relpath(SNAPSHOT, ROOT))


def snapshot_failures(entries):
    if not os.path.exists(SNAPSHOT):
        print("--- API snapshot: %s is absent (nothing published yet); diff skipped"
              % os.path.relpath(SNAPSHOT, ROOT))
        return []
    shipped, section = {}, None
    with open(SNAPSHOT, encoding="utf-8") as f:
        for line in f.read().splitlines():
            if line.startswith("// "):
                section = line[3:]
                shipped[section] = []
            elif section is not None:
                shipped[section].append(line)
    current = declarations(entries)
    failures = []
    for name, lines in shipped.items():
        if name not in current:
            failures.append("%s shipped in the last publish and is no longer packed" % name)
            continue
        remaining = list(current[name])
        missing = []
        for line in lines:
            if line in remaining:
                remaining.remove(line)
            else:
                missing.append(line.strip())
        if missing:
            failures.append("%s no longer declares what shipped: %s"
                            % (name, "; ".join(missing[:10])))
    return failures


def check_package(out_dir, ver, diff_snapshot):
    produced = [n for n in os.listdir(out_dir) if n.endswith(".tgz")]
    if len(produced) != 1:
        return ["expected exactly one .tgz in %s, found %s" % (out_dir, produced or "nothing")], None
    entries = tarball_entries(os.path.join(out_dir, produced[0]))

    failures = []
    expected = {"package/package.json", *PACKED_FROM}
    for module in src_modules():
        expected |= {"package/dist/%s.js" % module, "package/dist/%s.d.ts" % module}
    missing = sorted(expected - set(entries))
    extra = sorted(set(entries) - expected)
    if missing:
        failures.append("the tarball is missing %s" % ", ".join(missing[:20]))
    if extra:
        failures.append("the tarball carries unexpected %s" % ", ".join(extra[:20]))
    if failures:
        return failures, None

    for entry, source in PACKED_FROM.items():
        with open(os.path.join(ROOT, source), "rb") as f:
            if entries[entry] != f.read():
                failures.append("the packed %s differs from %s" % (entry[len("package/"):], source))

    manifest = json.loads(entries["package/package.json"])
    if manifest.get("version") != ver:
        failures.append("the packed version is %r, VERSION is %r" % (manifest.get("version"), ver))
    exports = manifest.get("exports")
    if not isinstance(exports, dict) or "." not in exports:
        return failures + ["the packed package.json has no exports \".\" entry"], None
    for subpath, conditions in exports.items():
        keys = list(conditions) if isinstance(conditions, dict) else []
        if not keys or keys[0] != "types" or keys[-1] != "default":
            failures.append("exports %r lists %s; \"types\" must come first and \"default\" last"
                            % (subpath, keys))
        for target in (conditions.values() if isinstance(conditions, dict) else []):
            if "package/" + target.removeprefix("./") not in entries:
                failures.append("exports %r points at %s, which is not packed" % (subpath, target))
    types = manifest.get("types")
    if not types or "package/" + types.removeprefix("./") not in entries:
        failures.append("the packed package.json \"types\" (%r) is not packed" % types)
    # moduleResolution node10 (TS 5.x's default under module commonjs) ignores "exports":
    # a subpath without this mapping still runs, but its import fails to type-check.
    mapped = (manifest.get("typesVersions") or {}).get("*") or {}
    for subpath, conditions in exports.items():
        if subpath == "." or not isinstance(conditions, dict):
            continue
        if mapped.get(subpath.removeprefix("./")) != [conditions.get("types")]:
            failures.append("exports %r has no typesVersions \"*\" entry naming %s"
                            % (subpath, conditions.get("types")))

    tops, members, parse_failures = internal_names()
    failures += parse_failures
    if not tops:
        failures.append("no exported declaration in %s/src carries @internal, so the leak "
                        "check below checked nothing; its parser no longer matches" % LIB_REL)
    dts = {n: "\n".join(lines) for n, lines in declarations(entries).items()}
    for name, module in sorted(tops.items()):
        leaks = sorted(n for n, text in dts.items() if re.search(r"(?<![\w$])%s(?![\w$])"
                                                                % re.escape(name), text))
        if leaks:
            failures.append("@internal %s (src/%s.ts) appears in %s"
                            % (name, module, ", ".join(leaks[:5])))
    for module, cls, member in members:
        body = class_body(dts.get("package/dist/%s.d.ts" % module, ""), cls)
        if re.search(r"^\s+(?:(?:readonly|static|protected|public|abstract|override|get|set)\s+)*"
                     r"%s\s*[?!]?\s*[:(<]" % re.escape(member), body, re.M):
            failures.append("@internal %s.%s (src/%s.ts) is declared in dist/%s.d.ts"
                            % (cls, member, module, module))
    print("--- @internal declarations checked: %d exported, %d members" % (len(tops), len(members)))

    if diff_snapshot:
        failures += snapshot_failures(entries)
    return failures, (manifest, entries, tops)


def consume(out_dir, manifest, tops):
    tarball = os.path.join(out_dir, [n for n in os.listdir(out_dir) if n.endswith(".tgz")][0])
    name = manifest["name"]
    specs = [name if s == "." else name + s.removeprefix(".") for s in manifest["exports"]]
    work = tempfile.mkdtemp(prefix="talib-ts-consumer-")
    try:
        for entry in os.listdir(CONSUMER_DIR):
            shutil.copy(os.path.join(CONSUMER_DIR, entry), work)
        if run([NPM, "install", "--no-audit", "--no-fund", "--ignore-scripts", tarball],
               cwd=work).returncode:
            return ["the consumer could not install %s" % os.path.basename(tarball)]
        if run([TSC, "-p", work]).returncode:
            return ["a TypeScript consumer does not type-check against the packed declarations"]
        results = {}
        for loader in ("import.mjs", "require.cjs"):
            r = run(["node", loader, *specs], cwd=work, capture=True)
            if r.returncode:
                return ["%s failed to load %s:\n%s" % (loader, ", ".join(specs), r.stderr.strip())]
            try:
                results[loader] = json.loads(r.stdout.strip().splitlines()[-1])
            except (ValueError, IndexError):
                return ["%s printed no result: %r" % (loader, r.stdout)]
        if results["import.mjs"] != results["require.cjs"]:
            return ["import and require() disagree: %r vs %r"
                    % (results["import.mjs"], results["require.cjs"])]
        got = results["import.mjs"]
        failures = []
        if got["sma"] != [2, 8, 2, 3, 4, 5, 6, 7, 8, 9]:
            failures.append("the consumer's SMA(3) over 1..10 is %r" % got["sma"])
        for spec, keys in got["exports"].items():
            leaked = sorted(set(keys) & set(tops))
            if leaked:
                failures.append("%s exports @internal %s at run time" % (spec, ", ".join(leaked[:10])))
            if not keys:
                failures.append("%s exports nothing" % spec)
        print("--- consumer loaded %s by import and require()" % ", ".join(specs))
        return failures
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--allow-dirty", action="store_true",
                        help="pack a library directory with local edits or stray files")
    parser.add_argument("--output", help="pack into this new or empty directory and keep it")
    parser.add_argument("--write-api-snapshot", action="store_true",
                        help="record the packed declarations in %s" % os.path.relpath(SNAPSHOT, ROOT))
    args = parser.parse_args()

    for tool in ("node", "npm"):
        if not shutil.which(tool):
            print("\n=== PACKAGE GATE FAILED ===\n  no %s on PATH" % tool)
            return 1
    node_version = subprocess.run(["node", "--version"], capture_output=True, text=True).stdout.strip()
    ver = version()
    print("--- TypeScript package %s, Node %s" % (ver, node_version))

    out_dir = os.path.abspath(args.output) if args.output else tempfile.mkdtemp(prefix="talib-ts-pack-")
    if os.path.isdir(out_dir) and os.listdir(out_dir):
        print("\n=== PACKAGE GATE FAILED ===\n  %s is not empty" % out_dir)
        return 1
    os.makedirs(out_dir, exist_ok=True)
    try:
        failures = pack(out_dir, args.allow_dirty)
        checked = None
        if not failures:
            failures, checked = check_package(out_dir, ver, not args.write_api_snapshot)
        if checked and args.write_api_snapshot:
            write_snapshot(checked[1])
        if checked and not failures:
            failures = consume(out_dir, checked[0], checked[2])
    finally:
        if not args.output:
            shutil.rmtree(out_dir, ignore_errors=True)

    if failures:
        print("\n=== PACKAGE GATE FAILED ===")
        for failure in failures:
            print("  " + failure)
        return 1
    print("\n=== PACKAGE GATE PASSED ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
