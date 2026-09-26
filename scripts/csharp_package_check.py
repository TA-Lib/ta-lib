#!/usr/bin/env python3
"""Package gate for the C# library: pack it, check what ships, then consume it.

  1. Pack. Into a fresh directory, from a library directory with no bin/ or
     obj/: `dotnet pack` exits 0 and writes nothing when the project is not
     packable, and a previous build's obj/ can stand in for a step that no
     longer works. Without --allow-dirty the tree must be clean and the
     library directory (test/ aside) must hold no untracked or ignored files:
     a stray .cs there compiles into the DLL while the nuspec's commit names
     HEAD.

  2. Contents. The .nupkg and .snupkg hold exactly the expected entries; the
     packed LICENSE, README and icon are byte-identical to their sources; the
     nuspec's version is VERSION and its repository is TA-Lib/ta-lib at HEAD.
     The id is whatever the csproj evaluates to: nothing here knows which id
     the project owns. The PDB's SourceLink must map /_/
     to that same commit: a fork's clone packs a SourceLink to the fork, with
     no warning.

  3. Consumer. scripts/csharp_consumer_check/ restores the package from the
     directory just packed (or from --from-source) into a throwaway package
     cache, runs, and is then published with NativeAOT and TrimMode=full and
     run again as a native binary; both runs must print the same thing. This
     is the only place anything runs the packed TALib.dll.

Run:  python3 scripts/csharp_package_check.py [--allow-dirty] [--output DIR]
      python3 scripts/csharp_package_check.py --from-source <feed or URL>

`--from-source` skips 1 and 2 and runs 3 against a published package.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile
from xml.sax.saxutils import quoteattr

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LIB_REL = "ta_codegen/output/csharp/library"
LIB_DIR = os.path.join(ROOT, LIB_REL)
CSPROJ = os.path.join(LIB_DIR, "TALib.csproj")
CONSUMER_DIR = os.path.join(ROOT, "scripts/csharp_consumer_check")
NUGET_ORG = "https://api.nuget.org/v3/index.json"

REPOSITORY = "https://github.com/TA-Lib/ta-lib"
SOURCELINK_PREFIX = "https://raw.githubusercontent.com/TA-Lib/ta-lib/"

# Package entry -> the file it must be a copy of.
PACKED_FROM = {
    "LICENSE": "LICENSE",
    "README.md": LIB_REL + "/README.md",
    "icon.png": "website/src/.vuepress/public/android-chrome-512x512.png",
}

OPC_ENTRY = re.compile(
    r"^(_rels/\.rels|\[Content_Types\]\.xml"
    r"|package/services/metadata/core-properties/[0-9a-f]+\.psmdcp)$"
)


def run(cmd, cwd=ROOT, env=None, capture=False):
    print("=== %s" % " ".join(cmd), flush=True)
    return subprocess.run(cmd, cwd=cwd, env=env, check=False, text=True,
                          stdout=subprocess.PIPE if capture else None,
                          stderr=subprocess.STDOUT if capture else None)


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True,
                          capture_output=True, text=True).stdout


def project():
    """(package id, assembly name, target frameworks), as MSBuild evaluates them."""
    props = ["PackageId", "AssemblyName", "TargetFramework", "TargetFrameworks"]
    out = subprocess.run(
        ["dotnet", "msbuild", CSPROJ, "-p:Configuration=Release"]
        + ["-getProperty:" + p for p in props],
        cwd=ROOT, check=True, capture_output=True, text=True).stdout
    p = json.loads(out)["Properties"]
    tfms = [t.strip() for t in (p["TargetFrameworks"] or p["TargetFramework"]).split(";")]
    return p["PackageId"], p["AssemblyName"], [t for t in tfms if t]


def version():
    with open(os.path.join(ROOT, "VERSION")) as f:
        return f.read().strip()


def pack(out_dir, allow_dirty):
    failures = []
    if not allow_dirty and git("status", "--porcelain", "--untracked-files=all"):
        failures.append("the tree is dirty (git status --porcelain); commit or "
                        "pass --allow-dirty")
    for sub in ("bin", "obj"):
        shutil.rmtree(os.path.join(LIB_DIR, sub), ignore_errors=True)
    if not allow_dirty:
        stray = [line for line in git("status", "--porcelain", "--ignored",
                                      "--untracked-files=all", "--", LIB_REL).splitlines()
                 if not line[3:].startswith(LIB_REL + "/test/")]
        if stray:
            failures.append("untracked or ignored files would compile or pack "
                            "into the library: %s" % ", ".join(stray[:10]))
    if failures:
        return failures
    if run(["dotnet", "pack", CSPROJ, "-c", "Release", "-o", out_dir, "--nologo"]).returncode:
        return ["dotnet pack failed; see its output above"]
    return []


def entries(path):
    with zipfile.ZipFile(path) as z:
        return {n: z.read(n) for n in z.namelist()}


def nuspec(files, pkg_id):
    """The nuspec's <metadata> children by local name."""
    root = ET.fromstring(files[pkg_id + ".nuspec"])
    meta = next(c for c in root if c.tag.endswith("metadata"))
    return {c.tag.split("}")[-1]: c for c in meta}


def check_entries(label, files, expected, failures):
    names = {n for n in files if not OPC_ENTRY.match(n)}
    missing = sorted(expected - names)
    extra = sorted(names - expected)
    if missing:
        failures.append("%s is missing %s" % (label, ", ".join(missing)))
    if extra:
        failures.append("%s carries unexpected %s" % (label, ", ".join(extra)))


def sourcelink(pdb):
    m = re.search(rb'\{"documents":\{[^{}]*\}\}', pdb)
    return json.loads(m.group(0))["documents"] if m else None


def check_package(out_dir, pkg_id, asm, tfms, ver, head):
    failures = []
    stem = "%s.%s" % (pkg_id, ver)
    produced = sorted(os.listdir(out_dir))
    if produced != [stem + ".nupkg", stem + ".snupkg"]:
        return ["expected exactly %s.nupkg and %s.snupkg in %s, found %s"
                % (stem, stem, out_dir, produced or "nothing")]

    nupkg = entries(os.path.join(out_dir, stem + ".nupkg"))
    snupkg = entries(os.path.join(out_dir, stem + ".snupkg"))
    check_entries("the .nupkg", nupkg,
                  {pkg_id + ".nuspec", *PACKED_FROM}
                  | {"lib/%s/%s.%s" % (t, asm, e) for t in tfms for e in ("dll", "xml")},
                  failures)
    check_entries("the .snupkg", snupkg,
                  {pkg_id + ".nuspec"} | {"lib/%s/%s.pdb" % (t, asm) for t in tfms},
                  failures)
    if failures:
        return failures

    for entry, source in PACKED_FROM.items():
        with open(os.path.join(ROOT, source), "rb") as f:
            if nupkg[entry] != f.read():
                failures.append("the packed %s differs from %s" % (entry, source))

    for label, files in (("nupkg", nupkg), ("snupkg", snupkg)):
        m = nuspec(files, pkg_id)
        for field, want in (("id", pkg_id), ("version", ver)):
            got = m[field].text if field in m else None
            if got != want:
                failures.append("%s nuspec %s is %r, expected %r" % (label, field, got, want))
    m = nuspec(nupkg, pkg_id)
    repo = m["repository"].attrib if "repository" in m else {}
    if repo.get("url") != REPOSITORY or repo.get("commit") != head:
        failures.append("nuspec repository is %r, expected url=%s commit=%s"
                        % (repo, REPOSITORY, head))
    for field, want in (("license", "LICENSE"), ("readme", "README.md"), ("icon", "icon.png")):
        got = m[field].text if field in m else None
        if got != want:
            failures.append("nuspec %s is %r, expected %r" % (field, got, want))
    if m.get("license") is not None and m["license"].get("type") != "file":
        failures.append("nuspec license is not type=\"file\": the text would not ship")

    local_path = ROOT.encode()
    for t in tfms:
        dll = nupkg["lib/%s/%s.dll" % (t, asm)]
        pdb = snupkg["lib/%s/%s.pdb" % (t, asm)]
        if local_path in dll or local_path in pdb:
            failures.append("%s: the DLL or PDB embeds the build path %s" % (t, ROOT))
        docs = sourcelink(pdb)
        want = SOURCELINK_PREFIX + head + "/*"
        if not docs or any(not k.startswith("/_/") or v != want for k, v in docs.items()):
            failures.append("%s: the PDB's SourceLink is %r, expected /_/* -> %s"
                            % (t, docs, want))
    return failures


def nuget_config(source, pkg_id):
    # Only the source may answer for the id: TALib is also someone else's id on nuget.org.
    sources = {"talib": source}
    mapping = {"talib": pkg_id}
    if source == NUGET_ORG:
        mapping["talib"] = "*"
    else:
        sources["nuget.org"] = NUGET_ORG
        mapping["nuget.org"] = "*"
    adds = "".join('    <add key="%s" value=%s />\n' % (k, quoteattr(v)) for k, v in sources.items())
    maps = "".join('    <packageSource key="%s"><package pattern=%s /></packageSource>\n'
                   % (k, quoteattr(v)) for k, v in mapping.items())
    return ('<?xml version="1.0" encoding="utf-8"?>\n<configuration>\n'
            "  <packageSources>\n    <clear />\n%s  </packageSources>\n"
            "  <packageSourceMapping>\n%s  </packageSourceMapping>\n</configuration>\n"
            % (adds, maps))


def consume(source, pkg_id, ver, head):
    """Returns a list of failures."""
    work = tempfile.mkdtemp(prefix="talib-consumer-")
    try:
        for name in os.listdir(CONSUMER_DIR):
            shutil.copy(os.path.join(CONSUMER_DIR, name), work)
        shutil.copy(os.path.join(ROOT, "global.json"), work)
        with open(os.path.join(work, "nuget.config"), "w") as f:
            f.write(nuget_config(source, pkg_id))
        env = dict(os.environ,
                   NUGET_PACKAGES=os.path.join(work, "packages"),
                   NUGET_HTTP_CACHE_PATH=os.path.join(work, "http-cache"))
        props = ["-p:TalibId=" + pkg_id, "-p:TalibVersion=" + ver]

        jit = os.path.join(work, "jit")
        if run(["dotnet", "build", "-c", "Release", "--nologo", "-o", jit] + props,
               cwd=work, env=env).returncode:
            return ["the consumer did not restore or build against %s %s from %s"
                    % (pkg_id, ver, source)]
        jit_run = run(["dotnet", os.path.join(jit, "ConsumerCheck.dll")], cwd=work, capture=True)
        print(jit_run.stdout, end="")
        if jit_run.returncode or not jit_run.stdout.endswith("OK\n"):
            return ["the consumer failed under the JIT"]
        loaded = jit_run.stdout.splitlines()[0]
        if not loaded.startswith("TALib %s+%s" % (ver, head or "")):
            return ["the consumer loaded %r, not %s %s at %s" % (loaded, pkg_id, ver, head or "any commit")]

        aot = os.path.join(work, "aot")
        if run(["dotnet", "publish", "-c", "Release", "--nologo", "-o", aot,
                "-p:PublishAot=true", "-p:TrimMode=full"] + props,
               cwd=work, env=env).returncode:
            return ["the NativeAOT publish failed (a trim or AOT warning is an error here)"]
        if os.path.exists(os.path.join(aot, "ConsumerCheck.dll")):
            return ["the NativeAOT publish produced a JIT application"]
        aot_run = run([os.path.join(aot, "ConsumerCheck")], cwd=work, capture=True)
        print(aot_run.stdout, end="")
        if aot_run.returncode or aot_run.stdout != jit_run.stdout:
            return ["the NativeAOT consumer failed, or printed something the JIT run did not"]
        return []
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--allow-dirty", action="store_true",
                        help="pack a working tree with local edits or stray files")
    parser.add_argument("--output", help="pack into this new or empty directory and keep it")
    parser.add_argument("--from-source",
                        help="skip packing; consume the package from this feed or URL")
    args = parser.parse_args()

    pkg_id, asm, tfms = project()
    ver = version()
    print("--- %s %s (%s), %s" % (pkg_id, ver, asm, ";".join(tfms)))
    if not tfms:
        print("\n=== PACKAGE GATE FAILED ===\n  MSBuild reports no target framework")
        return 1

    if args.from_source:
        failures = consume(args.from_source, pkg_id, ver, None)
    else:
        head = git("rev-parse", "HEAD").strip()
        out_dir = os.path.abspath(args.output) if args.output else tempfile.mkdtemp(
            prefix="talib-pack-")
        if os.path.isdir(out_dir) and os.listdir(out_dir):
            print("\n=== PACKAGE GATE FAILED ===\n  %s is not empty" % out_dir)
            return 1
        try:
            failures = (pack(out_dir, args.allow_dirty)
                        or check_package(out_dir, pkg_id, asm, tfms, ver, head)
                        or consume(out_dir, pkg_id, ver, head))
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
