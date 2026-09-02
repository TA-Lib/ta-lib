#!/usr/bin/env python3
"""Synthetic cross-language gate — the standing "does new generator surface
actually work in every backend" check.

Copies every ta_codegen/generator/input_synth/synth<n>/ (committed template functions
that are NEVER shipped) into ta_codegen/input/ inside a throwaway git
worktree, regenerates all backends there, and runs the usual gates scoped to
the SYNTH family:

  1. scripts/regtest.py --codegen --function=SYNTH
       builds the C library + ta_regtest + all four language servers +
       ta_ref_serve in the worktree, then runs the codegen sweep. SYNTH
       functions are absent from the frozen oracle (subset gate), but the
       stream_verify / OpenAndFill legs are current-vs-current and run for
       them in all four servers, bit-exact batch-vs-stream.
  2. ta_regtest --xlang-hash --function=SYNTH
       batch output parity: Rust/Java/C# against the in-process C golden,
       bitwise, across the fuzz shapes/seeds/sizes/params.
  3. input_synth/synth_values.py
       hand-derived golden VALUES for every fixture, against all four servers.
       Legs 1 and 2 only compare implementations against each other, so a
       fixture that computes the wrong thing in all four backends passes both;
       this is the only leg that knows what the numbers should be.

Leg 0 is the generator's own `cargo test` on the injected tree — the static
sweeps that read emitted text for structural properties, the class no value leg
below can see. Added in #327; before it, every sweep claiming to hold "over the
whole corpus" had only ever seen the 177 shipped functions.

Anti-vacuity: the script asserts from the gate output that EXACTLY the
expected number of SYNTH functions were exercised by each leg in each
language. A refactor that silently stops enumerating them fails the gate
rather than passing an empty run.

No flags, no modes. Local runs snapshot your dirty tree (git stash create),
CI runs test HEAD — same script, byte-for-byte. Runs nightly from
.github/workflows/dev-nightly-tests.yml (job: synth-gate).

On failure the worktree is kept for inspection (path printed); remove it with
`git worktree remove --force <path>`.
"""

import fcntl
import os
import re
import shutil
import subprocess
import sys

LANGS = 4  # C, Rust, Java, C# — servers exercised by the stream leg


# Held for the life of the process: an flock lives on the descriptor, so this
# global is what keeps the descriptor -- and the lock -- alive.
_GATE_LOCK_FD = None


def acquire_gate_lock(wt):
    """Refuse to start when another run already owns this gate worktree.

    The cleanup below force-removes whatever sits at `wt`. That is safe against
    OUR OWN leftovers and unsafe against a live run, and the path alone cannot
    tell those apart: it is derived from the caller's worktree, so two sessions
    sharing one checkout derive the same path. Deleting a running gate's tree
    out from under it is not a clean failure -- the JVM reports "Could not
    determine current working directory", or the build is simply killed.

    This lock CANNOT go stale. It lives on an open file descriptor, so the
    kernel releases it when this process exits -- crash and SIGKILL included.
    There is no PID file to leave behind and nothing to clean up by hand. And
    Python opens descriptors non-inheritable (PEP 446), so the build's own
    children cannot keep it open after we are gone -- which matters, because
    `dotnet` leaves a VBCSCompiler daemon running long after the build returns.

    Non-blocking on purpose: a second run says so and stops, rather than
    silently waiting on a gate that takes tens of minutes.
    """
    global _GATE_LOCK_FD
    path = wt + ".lock"
    fd = os.open(path, os.O_CREAT | os.O_RDWR, 0o644)
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        os.close(fd)
        sys.exit(
            f"synth_gate: another run already owns {wt}\n"
            f"       Two sessions sharing one checkout is the usual cause -- the\n"
            f"       cleanup step would delete the live run's worktree.\n"
            f"       Wait for it to finish, or run from your own worktree.\n"
            f"       See who holds it with:  fuser -v {path}"
        )
    _GATE_LOCK_FD = fd
    return fd


def synth_worktree_path(root):
    """Where this run's throwaway worktree goes: a sibling named after the caller.

    Derived, not fixed. A fixed sibling meant every worktree in the tree shared one
    directory, and the "clear any stale worktree" step below force-removes whatever
    is there — so a second run would delete the first one's tree mid-build and both
    would die with `could not write output ... No such file or directory`, cwd shown
    as `(deleted)`. Observed with two sessions running this gate at once.

    The main checkout keeps its historic path (ta-lib -> ta-lib-synth-gate), so CI
    and habit are unaffected; ta-lib-172 gets ta-lib-172-synth-gate.
    """
    return os.path.join(os.path.dirname(root),
                        os.path.basename(root) + "-synth-gate")


def find_repo_root():
    d = os.path.dirname(os.path.abspath(__file__))
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "ta_codegen", "input")):
            return d
        d = os.path.dirname(d)
    sys.exit("synth_gate: cannot locate repo root (ta_codegen/input marker)")


def run(cmd, cwd, capture=False):
    """Run a command, streaming output; optionally also return it."""
    print(f"+ {' '.join(cmd)}  (cwd={cwd})", flush=True)
    if not capture:
        return subprocess.run(cmd, cwd=cwd).returncode, ""
    proc = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True)
    lines = []
    for line in proc.stdout:
        sys.stdout.write(line)
        lines.append(line)
    proc.wait()
    sys.stdout.flush()
    return proc.returncode, "".join(lines)


def main():
    root = find_repo_root()
    synth_src = os.path.join(root, "ta_codegen", "generator", "input_synth")

    fixtures = sorted(
        d for d in os.listdir(synth_src)
        if re.fullmatch(r"synth\d+", d)
        and os.path.isdir(os.path.join(synth_src, d))
    )
    if not fixtures:
        sys.exit("synth_gate: no synth<n>/ directories in ta_codegen/generator/input_synth/")
    for f in fixtures:
        for ext in ("yaml", "c", "md"):
            p = os.path.join(synth_src, f, f"{f}.{ext}")
            if not os.path.isfile(p):
                sys.exit(f"synth_gate: fixture {f} is missing {f}.{ext}")
    nfix = len(fixtures)
    print(f"synth_gate: {nfix} synthetic function(s): {', '.join(fixtures)}")

    # Snapshot the CURRENT tree: dirty local changes are included (stash
    # create); a clean tree (CI) resolves to HEAD.
    sha = subprocess.run(["git", "stash", "create"], cwd=root,
                         capture_output=True, text=True).stdout.strip()
    if not sha:
        sha = "HEAD"
    print(f"synth_gate: testing tree {sha}")

    wt = synth_worktree_path(root)
    # The lock is what makes the removal below safe: the path is derived from
    # `root`, so it identifies our own leftovers only while one run at a time
    # owns that `root`.
    acquire_gate_lock(wt)
    # Clear the leftovers of a PREVIOUS RUN OF THIS SAME CALLER.
    if os.path.exists(wt):
        subprocess.run(["git", "worktree", "remove", "--force", wt], cwd=root)
        shutil.rmtree(wt, ignore_errors=True)
        subprocess.run(["git", "worktree", "prune"], cwd=root)

    rc = subprocess.run(["git", "worktree", "add", "--detach", wt, sha],
                        cwd=root).returncode
    if rc != 0:
        sys.exit(f"synth_gate: git worktree add failed (exit {rc})")

    ok = False
    try:
        for f in fixtures:
            shutil.copytree(os.path.join(synth_src, f),
                            os.path.join(wt, "ta_codegen", "input", f))
        print(f"synth_gate: injected {nfix} fixture(s) into worktree input/")

        # Build everything in the worktree via the usual pipeline, UNFILTERED:
        # a --function-scoped generate skips the full-registry surfaces
        # (Core.java, metadata registry), which would leave the synthetic
        # functions unregistered in some backends. regtest.py here is the
        # builder only (--no-regtest); the scoped gates run below.
        rc, _ = run([sys.executable, os.path.join(wt, "scripts", "regtest.py"),
                     "--no-regtest", "--no-perftest", "--no-direct-bench"],
                    cwd=wt, capture=True)
        if rc != 0:
            sys.exit(f"synth_gate: FAIL — worktree build exited {rc}")

        # Leg 0: the generator's OWN test suite, against the injected tree.
        #
        # Placed here rather than at the end because it needs nothing the
        # build above has not already produced -- just the generate -- so a
        # break skips the post-generate C rebuild and all three value legs
        # below. It cannot come earlier: the build IS what generates.
        #
        # These are the STATIC sweeps: they read emitted text for structural
        # properties, which is the class of defect no value gate below can see
        # (a peek that copies the handle and then writes the copy still answers
        # correctly). They ran only against the shipped corpus until #327, so
        # every sweep claiming to hold "over the whole corpus" had never seen a
        # fixture.
        rc, out0 = run(["cargo", "test", "--no-fail-fast",
                        "--manifest-path",
                        os.path.join(wt, "ta_codegen", "generator", "Cargo.toml")],
                       cwd=wt, capture=True)
        if rc != 0:
            sys.exit(f"synth_gate: FAIL — the generator's test suite exited {rc} "
                     f"on the injected tree")

        # Anti-vacuity for this leg, in the same spirit as the per-leg sweep
        # counts below: a suite that compiled and ran nothing would exit 0. The
        # floor is on tests EXECUTED, not on fixtures, because the sweeps that
        # care about fixtures carry their own conditional assertions (the
        # bounded-accumulator copy is asserted non-empty only when fixtures are
        # present, so it is those sweeps that prove the corpus was seen).
        ran = sum(int(m) for m in re.findall(r"^test result: ok\. (\d+) passed",
                                             out0, re.M))
        if ran < 500:
            sys.exit(f"synth_gate: VACUOUS — the generator suite ran {ran} test(s), "
                     f"expected at least 500; it did not sweep the injected tree")
        print(f"synth_gate: generator suite OK ({ran} tests on the injected tree)")

        # regtest.py builds the C library BEFORE generating (the generated C is
        # normally committed, so that order is fine for real functions). The
        # injected SYNTH sources only exist in src/ta_func AFTER the generate
        # step, so rebuild the library + ta_regtest to register them in the
        # in-process golden (TA_ForEachFunc enumerates from ta_abstract).
        rc, _ = run([sys.executable, os.path.join(wt, "scripts", "build.py")],
                    cwd=wt, capture=True)
        if rc != 0:
            sys.exit(f"synth_gate: FAIL — post-generate C rebuild exited {rc}")

        # Leg 1: codegen sweep + stream_verify/OpenAndFill, scoped to SYNTH.
        rc, out1 = run(["./ta_regtest", "--codegen", "--function=SYNTH"],
                       cwd=os.path.join(wt, "bin"), capture=True)
        if rc != 0:
            sys.exit(f"synth_gate: FAIL — --codegen exited {rc}")

        # Leg 2: batch bitwise parity vs the in-process C golden.
        rc, out2 = run(["./ta_regtest", "--xlang-hash", "--function=SYNTH"],
                       cwd=os.path.join(wt, "bin"), capture=True)
        if rc != 0:
            sys.exit(f"synth_gate: FAIL — --xlang-hash exited {rc}")

        # Leg 3: hand-derived golden VALUES, all four servers. Legs 1 and 2 are
        # both comparative — stream against batch, and three languages against
        # C — so a fixture that is wrong the same way everywhere passes both.
        # This is the only leg with an opinion about what the numbers should be.
        # Run the ORIGINAL tree's copy, not the worktree's: the worktree comes
        # from `git stash create`, which snapshots tracked files only, so an
        # as-yet-uncommitted synth_values.py would simply not be there (the
        # fixtures dodge this by being copytree'd in explicitly). Reading it
        # from `synth_src` also means a local edit to the oracle is what runs,
        # which is what you want while writing a new fixture's row.
        rc, _ = run([sys.executable,
                     os.path.join(synth_src, "synth_values.py"),
                     os.path.join(wt, "bin")],
                    cwd=wt, capture=True)
        if rc != 0:
            sys.exit(f"synth_gate: FAIL — synth_values exited {rc}")

        # Anti-vacuity: prove the SYNTH functions were actually exercised.
        #
        # Read off the per-language tally, one line per server:
        #   `  Java: 0 passed, 0 failed, 14 skipped`
        # Their sum is what the --function filter admitted, so a filter that
        # matched nothing reads 0 and fails here. That matters more than it
        # sounds: a `--function=` naming nothing exits 0 (the filter matches a
        # GROUP TAG as well as a name), so the exit code alone is no evidence.
        #
        # `skipped` is expected, not a problem: the SYNTH functions are absent
        # from the frozen pre-cutover oracle, so their VALUE comparison has no
        # reference and is skipped by design (the subset gate). The stream and
        # OpenAndFill legs are current-vs-current and run regardless.
        #
        # This replaces a parse of "Stream verify: N functions, M legs", which
        # `d8af8b5d9` removed when ta_regtest stopped printing its coverage and
        # started asserting it internally. The old parse could no longer match,
        # so this leg would have failed VACUOUS on every run from that commit
        # on -- the first nightly after it had not run yet when this was found.
        # The internal assertion is why a per-language tally is enough now:
        # ta_regtest fails itself if a leg goes uncovered.
        tally = re.findall(
            r"^  (C|Rust|Java|C#): (\d+) passed, (\d+) failed, (\d+) skipped",
            out1, re.M)
        if len(tally) != LANGS:
            sys.exit(f"synth_gate: VACUOUS — {len(tally)} per-language tally "
                     f"line(s) in the --codegen output, expected {LANGS}")
        for lang, passed, failed, skipped in tally:
            seen = int(passed) + int(failed) + int(skipped)
            if seen != nfix:
                sys.exit(f"synth_gate: VACUOUS — {lang} saw {seen} SYNTH "
                         f"function(s), expected {nfix}")

        # Hash leg: the PASS line prints the number of functions actually
        # swept past the --function filter (funcsSwept in test_codegen.c).
        m = re.search(r"PASS — (\d+) function\(s\) swept", out2)
        if not m:
            sys.exit("synth_gate: VACUOUS — no counted PASS line in "
                     "--xlang-hash output")
        if int(m.group(1)) != nfix:
            sys.exit(f"synth_gate: VACUOUS — parity gate swept {m.group(1)} "
                     f"function(s), expected {nfix}")

        ok = True
        print(f"\nsynth_gate: PASS — {nfix} synthetic function(s), "
              f"{LANGS} languages, stream + batch bitwise legs all exercised")
    finally:
        if ok:
            subprocess.run(["git", "worktree", "remove", "--force", wt], cwd=root)
            subprocess.run(["git", "worktree", "prune"], cwd=root)
        else:
            print(f"\nsynth_gate: worktree kept for inspection: {wt}\n"
                  f"  remove with: git worktree remove --force {wt}")


if __name__ == "__main__":
    main()
