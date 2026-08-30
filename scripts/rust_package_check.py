#!/usr/bin/env python3
"""Package gate for the two publishable Rust crates (#179 E1).

Nothing in CI had ever run `cargo package`. Everything the Rust jobs do --
clippy, rustdoc, doctests, `cargo test` -- compiles the crate *in the
workspace*, where every file on disk is reachable whether or not it would ship.
The `.crate` tarball is a different artifact: cargo builds its file list from
git, filtered by the manifest's include/exclude and by every `.gitignore` above
it. So a file can be present, compiled, tested and documented here and still be
absent from what crates.io receives, and the first place that shows up is a
published release that does not build.

This gate runs the real packaging step and then checks the tarball itself:

  1. `cargo package -p ta-lib-dispatch -p ta-lib`. The pair, not `-p ta-lib`
     alone: the library pins `ta-lib-dispatch = "=0.1.2"`, which is not on
     crates.io yet, so packaged on its own it cannot resolve. Packaging both
     makes cargo build a temporary registry from the sibling (the manifest
     comment on that dependency says the same). Cargo's own verification pass
     then COMPILES each crate from its unpacked tarball -- that is what catches
     a source file that did not ship.

  2. Contents. For each crate, the entries in the `.crate` must be exactly the
     git-tracked files under that crate's directory, plus the three files cargo
     generates (Cargo.lock, Cargo.toml.orig, .cargo_vcs_info.json). Missing
     entries mean the release is short a file; unexpected extras mean something
     untracked is being shipped.

     Deriving the expectation from git rather than from a recorded count is the
     point: it needs no update when a function is added, and it still fails when
     one goes missing. As of this commit that is 191 tracked files for ta-lib
     (183 of them `src/ta_func/*.rs`) and 4 for ta-lib-dispatch -- 194 and 7
     entries in the two tarballs once the three generated files are counted.

  3. Anti-vacuity. An empty or near-empty expectation would make check 2 pass
     for the wrong reason (`git ls-files` outside a checkout returns nothing,
     and an empty set is a subset of anything). So: each crate's tracked set
     must be non-empty, ta-lib must carry its four structural files, and its
     packaged `src/ta_func/*.rs` count must clear a floor well below the real
     one. The floor is deliberately crude -- check 2 is what makes the count
     exact; this only refuses to run on nothing.

Run:  python3 scripts/rust_package_check.py [--allow-dirty]

`--allow-dirty` is forwarded to cargo, for running this over a working tree
with local edits. CI runs it without, so an unexpectedly dirty checkout is
itself a failure there.
"""

import argparse
import glob
import json
import os
import subprocess
import sys
import tarfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WORKSPACE = os.path.join(ROOT, "ta_codegen/output/rust")

# (package name, directory under the workspace). Versions are read back off the
# tarball cargo produces, so a version bump needs no edit here.
CRATES = [
    ("ta-lib-dispatch", "dispatch"),
    ("ta-lib", "library"),
]

# Cargo synthesises these into every tarball; they are not tracked in git.
CARGO_GENERATED = {"Cargo.lock", "Cargo.toml.orig", ".cargo_vcs_info.json"}

# Files that make the ta-lib tarball a usable crate at all. Each is required by
# something outside this repo: the manifest and lib.rs to build, README.md for
# the crates.io landing page, LICENSE because BSD-3-Clause requires the text to
# travel with the source (#179 A3 put it there; nothing has checked since).
REQUIRED_IN_LIBRARY = ["Cargo.toml", "LICENSE", "README.md", "src/lib.rs"]

# Anti-vacuity floor only -- see the module docstring. The real count is pinned
# exactly by the git-tracked comparison.
MIN_TA_FUNC_FILES = 100

_PACKAGE_DIR = None


def package_dir():
    """Where cargo will leave the `.crate` tarballs: <target-dir>/package.

    Asked, not assumed. `<workspace>/target` is only the default: CARGO_TARGET_DIR
    and `build.target-dir` both move it, and sharing one target directory across
    worktrees is the ordinary reason to set them. Hardcoding the default made
    this gate fail on such a checkout with "expected exactly one
    ta-lib-dispatch-<version>.crate ... found 0" -- cargo had packaged both
    crates correctly, into a directory this script was not looking at. It also
    pointed the stale-tarball sweep below at the wrong directory, so the run
    could not even clean up after itself. `cargo metadata` reports the directory
    cargo will actually use, whatever moved it.

    CI sets neither, so this changes nothing there; it is the local-reproduction
    path that was broken.
    """
    global _PACKAGE_DIR
    if _PACKAGE_DIR is None:
        out = subprocess.run(
            ["cargo", "metadata", "--format-version", "1", "--no-deps"],
            cwd=WORKSPACE, check=True, stdout=subprocess.PIPE,
        ).stdout
        _PACKAGE_DIR = os.path.join(json.loads(out)["target_directory"], "package")
    return _PACKAGE_DIR


def run_cargo_package(allow_dirty):
    # Stale tarballs from an earlier version would otherwise be picked up by the
    # glob below and checked instead of what this run produced.
    for stale in glob.glob(os.path.join(package_dir(), "*.crate")):
        os.remove(stale)

    cmd = ["cargo", "package", "--allow-dirty"] if allow_dirty else ["cargo", "package"]
    for name, _subdir in CRATES:
        cmd.extend(["-p", name])
    print("=== %s (cwd=%s) ===" % (" ".join(cmd), WORKSPACE), flush=True)
    # A non-zero exit here is a real result, not a crash: it is cargo refusing
    # to package, or its verification build failing to compile the tarball --
    # e.g. a source file that did not ship (rustc E0583). Report it as this
    # gate's own failure rather than as a Python traceback.
    result = subprocess.run(cmd, cwd=WORKSPACE, check=False)
    if result.returncode != 0:
        print("\n=== PACKAGE GATE FAILED ===")
        print("  cargo package exited %d -- see its output above. Either the "
              "crates could not be packaged, or the packaged tarball did not "
              "compile in cargo's verification pass." % result.returncode)
        return False
    return True


def packaged_entries(name):
    """Paths inside the crate's tarball, relative to the crate root."""
    # `[0-9]*` and not `*`: the latter would make "ta-lib" also match
    # ta-lib-dispatch-0.1.2.crate, and the two would check each other.
    found = glob.glob(os.path.join(package_dir(), "%s-[0-9]*.crate" % name))
    if len(found) != 1:
        raise SystemExit(
            "FAIL: expected exactly one %s-<version>.crate in %s, found %d"
            % (name, package_dir(), len(found))
        )
    path = found[0]
    prefix = os.path.basename(path)[: -len(".crate")] + "/"
    entries = set()
    with tarfile.open(path, "r:gz") as tar:
        for member in tar.getmembers():
            if not member.isfile():
                continue
            if not member.name.startswith(prefix):
                raise SystemExit(
                    "FAIL: %s carries an entry outside %s: %s"
                    % (os.path.basename(path), prefix, member.name)
                )
            entries.add(member.name[len(prefix):])
    return entries


def tracked_files(subdir):
    """git-tracked files under the crate directory, relative to it."""
    out = subprocess.run(
        ["git", "ls-files", "--", subdir],
        cwd=WORKSPACE,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.split("\n")
    prefix = subdir + "/"
    return {line[len(prefix):] for line in out if line.startswith(prefix)}


def check_crate(name, subdir):
    """Returns a list of failure strings (empty when the crate is clean)."""
    failures = []
    packaged = packaged_entries(name)
    tracked = tracked_files(subdir)

    print("--- %s: %d packaged entries, %d tracked files"
          % (name, len(packaged), len(tracked)))

    if not tracked:
        failures.append(
            "%s: `git ls-files -- %s` returned nothing, so there is no "
            "expectation to check against (not a real pass)" % (name, subdir)
        )
        return failures

    missing = sorted(tracked - packaged)
    if missing:
        failures.append(
            "%s: %d git-tracked file(s) did not make it into the .crate: %s"
            % (name, len(missing), ", ".join(missing[:20]))
        )

    extra = sorted(packaged - tracked - CARGO_GENERATED)
    if extra:
        failures.append(
            "%s: %d packaged entry(ies) are not tracked in git and are not "
            "cargo-generated: %s" % (name, len(extra), ", ".join(extra[:20]))
        )

    return failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--allow-dirty",
        action="store_true",
        help="forward --allow-dirty to cargo package (for local working trees)",
    )
    args = parser.parse_args()

    if not run_cargo_package(args.allow_dirty):
        return 1

    failures = []
    for name, subdir in CRATES:
        failures.extend(check_crate(name, subdir))

    library = packaged_entries("ta-lib")
    for required in REQUIRED_IN_LIBRARY:
        if required not in library:
            failures.append("ta-lib: the .crate is missing %s" % required)

    ta_func = sorted(
        e for e in library if e.startswith("src/ta_func/") and e.endswith(".rs")
    )
    print("--- ta-lib: %d packaged src/ta_func/*.rs" % len(ta_func))
    if len(ta_func) < MIN_TA_FUNC_FILES:
        failures.append(
            "ta-lib: only %d src/ta_func/*.rs in the .crate, under the %d floor "
            "-- the contents check above cannot be trusted on a set this small"
            % (len(ta_func), MIN_TA_FUNC_FILES)
        )

    if failures:
        print("\n=== PACKAGE GATE FAILED ===")
        for failure in failures:
            print("  " + failure)
        return 1

    print("\n=== PACKAGE GATE PASSED: both crates package, verify-build, and "
          "ship exactly their tracked files ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
