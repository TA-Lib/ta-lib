#!/usr/bin/env python3
"""Keep ta-lib-python's dev branch ready for the C library in THIS worktree.

    scripts/python-dev.py           check: has the wrapper drifted? writes nothing
    scripts/python-dev.py sync      regenerate what drifted, into the wrapper tree
    scripts/python-dev.py wheel     build a wheel and run the release test command

Run `check` after anything that moves ta-lib's public surface — a new
indicator, a new TA_MAType or unstable-period id, a new flag — and `sync` when
it reports drift. Assumes ~/ta-lib and ~/ta-lib-python are both on dev
(--any-branch to override, TALIB_PYTHON and TALIB_PYTHON_DEV_CACHE to relocate).
Nothing is ever committed, in either repo.

Everything is idempotent: the C build and the work copy live under
~/.cache/talib-python-dev and are reused. `sync` regenerates from the headers, so
running it twice changes nothing the second time.

Run it with the Python whose packages the build and the suite should use; a
virtualenv is repaired in place when a package is missing. `sync` writes
_ta_lib.c with <wrapper>/.venv/bin/cython, at the Python and Cython versions the
committed file names, and creates that venv if it is missing: the file embeds
paths relative to it.

This is the dev-vs-dev pairing. `test-dist.py` is the other one: it builds the
PUBLISHED wrapper against a release candidate, as a user would.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

TA_LIB = Path(__file__).resolve().parent.parent
WRAPPER = Path(os.environ.get("TALIB_PYTHON", Path.home() / "ta-lib-python")).expanduser().resolve()
CACHE = Path(os.environ.get("TALIB_PYTHON_DEV_CACHE",
                            Path.home() / ".cache" / "talib-python-dev")).expanduser().resolve()
PREFIX, CBUILD, WORK, NOCYTHON = (CACHE / n for n in ("prefix", "cbuild", "work", "nocython"))

FAILURES = []


def fail(what):
    FAILURES.append(what)
    print("  FAIL  %s" % what)


def ok(what):
    print("  ok    %s" % what)


def run(cmd, cwd=None, env=None, check=True, quiet=True):
    r = subprocess.run(cmd, cwd=cwd, env=env, text=True,
                       stdout=subprocess.PIPE if quiet else None,
                       stderr=subprocess.STDOUT if quiet else None)
    if check and r.returncode:
        print(r.stdout or "")
        sys.exit("command failed: %s" % " ".join(str(c) for c in cmd))
    return r


def git(repo, *args):
    return subprocess.run(["git", "-C", str(repo)] + list(args),
                          capture_output=True, text=True).stdout.strip()


def require_branches(any_branch):
    for repo in (TA_LIB, WRAPPER):
        if not repo.is_dir():
            sys.exit("not a directory: %s" % repo)
        branch = git(repo, "rev-parse", "--abbrev-ref", "HEAD")
        head = git(repo, "rev-parse", "--short", "HEAD")
        dirty = " (dirty)" if git(repo, "status", "--porcelain") else ""
        print("%-22s %s @ %s%s" % (repo.name, branch, head, dirty))
        if branch != "dev" and not any_branch:
            sys.exit("%s is on %r, not dev. Pass --any-branch to proceed anyway."
                     % (repo.name, branch))


def build_c():
    """This worktree's C library, installed into a private prefix."""
    run(["cmake", "-S", str(TA_LIB), "-B", str(CBUILD), "-DCMAKE_BUILD_TYPE=Release",
         "-DBUILD_DEV_TOOLS=OFF", "-DCMAKE_INSTALL_PREFIX=" + str(PREFIX)])
    run(["cmake", "--build", str(CBUILD), "-j", str(os.cpu_count() or 4)])
    run(["cmake", "--install", str(CBUILD)])
    lib = next(PREFIX.glob("lib/libta-lib.so.*"), None)
    print("C library: %s" % (lib.name if lib else "installed"))


def env_for(work, hide_cython=False):
    env = dict(os.environ)
    env["TA_INCLUDE_PATH"] = str(PREFIX / "include")
    env["TA_LIBRARY_PATH"] = str(PREFIX / "lib")
    # PYTHONPATH=. is load-bearing: a pip-installed ta-lib would otherwise
    # shadow the checkout, and the generators would read ITS defaults.
    path = [str(work)]
    if hide_cython:
        path.insert(0, str(NOCYTHON))
    env["PYTHONPATH"] = os.pathsep.join(path)
    return env


def hide_cython_dir():
    (NOCYTHON / "Cython").mkdir(parents=True, exist_ok=True)
    (NOCYTHON / "Cython" / "__init__.py").write_text(
        "raise ImportError('Cython hidden by scripts/python-dev.py')\n")


def work_copy():
    """A clean copy of the wrapper's working tree — tracked and new files."""
    if WORK.exists():
        shutil.rmtree(WORK)
    files = git(WRAPPER, "ls-files", "-co", "--exclude-standard").splitlines()
    for rel in files:
        src, dst = WRAPPER / rel, WORK / rel
        if src.is_file():
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
    return WORK


def ensure_packages(modules, requirements=()):
    """The packages this target imports or builds with, in this interpreter.

    A missing one is installed when this runs in a virtualenv; outside one it
    is reported, never installed into a system Python."""
    probe = ("import importlib.util, sys\n"
             "print(' '.join(m for m in sys.argv[1:] if importlib.util.find_spec(m) is None))")
    missing = run([sys.executable, "-c", probe] + list(modules)).stdout.split()
    if not missing:
        return
    if sys.prefix == sys.base_prefix:
        sys.exit("missing in %s: %s\nUse a virtualenv, or install them: "
                 "%s -m pip install %s" % (sys.executable, " ".join(missing),
                                          sys.executable, " ".join(missing)))
    print("installing into %s: %s" % (sys.prefix, " ".join(missing)))
    extra = []
    for req in requirements:
        extra += ["-r", str(WRAPPER / req)]
    run([sys.executable, "-m", "pip", "install", "--quiet"] + extra + missing)


def ta_func_declarations():
    """`TA_<N>` and `TA_<N>_Lookback` as _ta_lib.pxd spells them, in header order."""
    decls, pending = [], []
    for line in (PREFIX / "include" / "ta-lib" / "ta_func.h").read_text().splitlines():
        if line.startswith("TA_LIB_API"):
            line = line[len("TA_LIB_API"):]
        line = re.sub(r"/\*.*?\*/", "", line).strip()
        if pending or line.startswith(("TA_RetCode TA_", "int TA_")):
            pending.append(line)
            if line.endswith(";"):
                decls.append(re.sub(r"\s+", " ", " ".join(pending)))
                pending = []
    out = {}
    for d in decls:
        d = re.sub(r"\(\s+", "(", re.sub(r"\s+\)", ")", d.rstrip(";").strip()))
        d = re.sub(r"\s*,\s*", ", ", d).replace("(void)", "()")
        m = re.match(r"(TA_RetCode|int) (TA_\w+)\(", d)
        if not m:
            continue
        name = m.group(2)
        if (name.startswith(("TA_S_", "TA_Set", "TA_Restore")) or "_Stream" in d
                or name.endswith("_DisplayShift")
                or (m.group(1) == "int" and not name.endswith("_Lookback"))):
            continue
        out[name] = "    " + d
    return out


PXD_BLOCK = re.compile(r'(cdef extern from "ta-lib/ta_func.h" nogil:\n)(.*?\n)(\n    # TALIB functions)',
                       re.S)


def pxd_block(tree):
    path = tree / "talib" / "_ta_lib.pxd"
    m = PXD_BLOCK.search(path.read_text())
    if not m:
        sys.exit("cannot find the ta_func.h block in %s" % path)
    return path, m


def sync_pxd(tree):
    """Declare in _ta_lib.pxd every batch function and lookback ta_func.h has.

    An existing line is kept as written; one the header no longer has is dropped.
    Returns the names added."""
    path, m = pxd_block(tree)
    have = {re.search(r"(TA_\w+)\(", l).group(1): l
            for l in m.group(2).splitlines() if l.strip()}
    want = ta_func_declarations()
    block = "".join(have.get(n, d) + "\n" for n, d in want.items())
    if block != m.group(2):
        text = path.read_text()
        path.write_text(text[:m.start(2)] + block + text[m.end(2):])
    return sorted(set(want) - set(have))


def build_wrapper(work, hide_cython=False):
    for junk in ("build",):
        shutil.rmtree(work / junk, ignore_errors=True)
    for so in work.glob("talib/*.so"):
        so.unlink()
    if not hide_cython:
        # A copied _ta_lib.c keeps its mtime, so Cython can take a stale one as
        # current and build the extension from it.
        (work / "talib" / "_ta_lib.c").unlink(missing_ok=True)
    run([sys.executable, "setup.py", "build_ext", "--inplace"],
        cwd=work, env=env_for(work, hide_cython))


def pytest(work):
    r = run([sys.executable, "-m", "pytest", "-q"],
            cwd=work, env=env_for(work), check=False)
    tail = (r.stdout or "").strip().splitlines()[-1:] or [""]
    return r.returncode == 0, tail[0]


def cython_pin():
    """<wrapper>/.venv/bin/cython at the version that produced the COMMITTED
    _ta_lib.c, created or repaired on demand.

    Read from git, not the tree: a build regenerates the file with whatever
    Cython is installed, and refreshing with that would bury the real diff
    under a compiler-version rewrite. The file embeds the venv-relative paths of
    Cython's and numpy's .pxd files, so only this venv reproduces it."""
    stamp = git(WRAPPER, "show", "HEAD:talib/_ta_lib.c")[:200]
    m = re.search(r"Generated by Cython ([\d.]+)", stamp)
    if not m:
        sys.exit("cannot read the Cython version stamp from talib/_ta_lib.c")
    version = m.group(1)
    m = re.search(r"\.venv/lib/(python\d+\.\d+)/site-packages/",
                  git(WRAPPER, "show", "HEAD:talib/_ta_lib.c"))
    if not m:
        sys.exit("cannot read the venv's Python version from talib/_ta_lib.c")
    python = m.group(1)
    venv = WRAPPER / ".venv"
    py = venv / "bin" / "python"
    if not py.exists():
        if shutil.which(python) is None:
            sys.exit("%s is needed to create %s" % (python, venv))
        print("creating %s" % venv)
        run([python, "-m", "venv", str(venv)])
    got = run([str(py), "-c", "import sys; print('python%d.%d' % sys.version_info[:2])"]).stdout.strip()
    if got != python:
        sys.exit("%s runs %s, but the committed _ta_lib.c was written from a %s venv; "
                 "move it aside and rerun" % (venv, got, python))
    have = run([str(py), "-c", "import Cython; print(Cython.__version__)"],
               check=False).stdout.strip()
    if have != version or run([str(py), "-c", "import numpy"], check=False).returncode:
        print("installing Cython %s and numpy into %s" % (version, venv))
        run([str(py), "-m", "pip", "install", "--quiet", "Cython==" + version, "numpy"])
    return version, venv / "bin" / "cython"


def stub_text(work, env, *tool):
    """A generator's stdout; anything on stderr would otherwise land in the stub."""
    r = subprocess.run([sys.executable] + list(tool), cwd=work, env=env,
                       capture_output=True, text=True)
    if r.returncode or r.stderr.strip():
        print(r.stderr)
        sys.exit("%s failed" % " ".join(tool))
    return r.stdout


def regenerate(work, into):
    """Regenerate until it settles, then mirror the result into `into`.

    The generators import talib, so what they can see is what the extension
    currently binds: over a wrapper missing a function, the first pass emits it
    with no defaults and no docstring, and only a rebuild lets the next pass
    see it. Writing straight to `into` would leave that first pass as the
    answer."""
    env = env_for(work)
    tools = (("generate_func.py", "_func.pxi"), ("generate_stream.py", "_stream.pxi"))
    sync_pxd(work)
    for pass_no in (1, 2):
        changed = False
        for tool, out in tools:
            r = subprocess.run([sys.executable, "tools/" + tool], cwd=work, env=env,
                               capture_output=True, text=True)
            if r.returncode:
                print(r.stderr)
                sys.exit("%s failed" % tool)
            # a temp name, not a redirect: a truncating redirect loses the file on a crash
            target = work / "talib" / out
            changed = changed or target.read_text() != r.stdout
            tmp = work / (out + ".new")
            tmp.write_text(r.stdout)
            tmp.replace(target)
            if pass_no == 2 and r.stderr.strip():
                fail("%s still reports missing defaults:\n%s" % (tool, r.stderr.strip()))
        if not changed:
            break
        build_wrapper(work)
    stub, stream_stub = (stub_text(work, env, *tool) for tool in
                         (["tools/generate_abstract_stub.py"], ["tools/generate_stream.py", "--stub"]))
    if into != work:
        for _, out in tools:
            shutil.copy2(work / "talib" / out, into / "talib" / out)
        (into / "talib" / "abstract.pyi").write_text(stub)
        (into / "talib" / "stream.pyi").write_text(stream_stub)
    return stub, stream_stub


def header_enum(path, pattern):
    return dict(re.findall(pattern, (TA_LIB / "include" / path).read_text()))


def check_mirrors(work):
    """The hand-maintained tables in the wrapper, against this worktree's headers.

    This is the part the wrapper's own suite cannot see: it only knows what the
    wrapper exposes, not what the header added yesterday."""
    env = env_for(work)
    probe = r"""
import json, talib, talib._ta_lib as L
d = lambda x: x.decode() if isinstance(x, bytes) else x
print(json.dumps({
    'functions': sorted({d(f) for g in L._ta_getGroupTable() for f in L._ta_getFuncTable(g)}),
    'grouped': sorted(talib.get_functions()),
    'bound': sorted(talib.__TA_FUNCTION_NAMES__),
    'matype': {k: v for k, v in vars(type(talib.MA_Type)).items() if k.isupper()},
    'unst': sorted(set(L._ta_func_unst_ids) | set(L._ta_func_unst_retired)),
    'func_flags': sorted(L.TA_FUNC_FLAGS),
    'out_flags': sorted(L.TA_OUTPUT_FLAGS),
    'in_flags': sorted(L.TA_INPUT_FLAGS),
}))
"""
    import json
    got = json.loads(run([sys.executable, "-c", probe], cwd=work, env=env).stdout)

    c, bound, grouped = (set(got[k]) for k in ("functions", "grouped", "bound"))
    _, block = pxd_block(work)
    undeclared = sorted(set(ta_func_declarations())
                        - set(re.findall(r"(TA_\w+)\(", block.group(2))))
    if undeclared:
        fail("_ta_lib.pxd does not declare: %s" % " ".join(undeclared))
    else:
        ok("_ta_lib.pxd declares every function in ta_func.h")
    if not c:
        fail("the C library reported no functions at all")
    elif c == bound == grouped:
        ok("%d functions, and the group dict agrees with the extension" % len(c))
    else:
        fail("function sets disagree: C=%d bound=%d grouped=%d | unbound: %s | "
             "ungrouped: %s | in the wrapper but not in C: %s"
             % (len(c), len(bound), len(grouped),
                sorted(c - bound) or "-", sorted(c - grouped) or "-",
                sorted((bound | grouped) - c) or "-"))

    want = {n: int(v, 0) for n, v in header_enum("ta_defs.h", r"TA_MAType_(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)").items()}
    if want == {k: int(v) for k, v in got["matype"].items()}:
        ok("MA_Type mirrors TA_MAType (%d members)" % len(want))
    else:
        fail("MA_Type drift: header has %s, wrapper has %s"
             % (sorted(want), sorted(got["matype"])))

    unst = {n for n in header_enum("ta_defs.h", r"TA_FUNC_UNST_(\w+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)")
            if not n.startswith("UNUSED")} | {"ALL", "NONE"}
    if unst <= set(got["unst"]):
        ok("unstable-period ids cover TA_FuncUnstId (%d)" % len(unst))
    else:
        fail("unstable-period ids missing: %s" % sorted(unst - set(got["unst"])))

    for label, macro, key in (("function", r"TA_FUNC_FLG_\w+", "func_flags"),
                              ("output", r"TA_OUT_\w+", "out_flags"),
                              ("input", r"TA_IN_PRICE_\w+", "in_flags")):
        bits = {int(v, 16) for _, v in re.findall(
            r"#define\s+(%s)\s+(0x[0-9A-Fa-f]+)" % macro,
            (TA_LIB / "include" / "ta_abstract.h").read_text())}
        missing = bits - set(got[key])
        if missing:
            fail("%s flags with no description: %s"
                 % (label, sorted(hex(b) for b in missing)))
        else:
            ok("%s flags all described (%d)" % (label, len(bits)))

    block = re.search(r"typedef enum\s*\{(.*?)\}\s*TA_RetCode;",
                      (TA_LIB / "include" / "ta_defs.h").read_text(), re.S).group(1)
    codes = {int(v) for _, v in re.findall(r"(TA_\w+)\s*=\s*(\d+)", block)}
    unknown = run([sys.executable, "-c", """
import talib._ta_lib as L, sys
bad = []
for code in %s:
    try:
        L._ta_check_success('probe', code)
    except Exception as e:
        if 'Unknown Error' in str(e):
            bad.append(code)
print(','.join(str(b) for b in bad))
""" % sorted(codes)], cwd=work, env=env).stdout.strip()
    if unknown:
        fail("TA_RetCode values with no description: %s" % unknown)
    else:
        ok("every TA_RetCode has a description")


def target_check():
    work = work_copy()
    pristine_c = (work / "talib" / "_ta_lib.c").read_text()

    print("\n[1/4] build with Cython (the .pyx path)")
    build_wrapper(work)
    passed, line = pytest(work)
    (ok if passed else fail)("suite, built from _ta_lib.pyx: %s" % line)

    # Against THIS build: the committed .c may predate the .pxi sources.
    print("\n[2/4] hand-maintained tables vs this worktree's headers")
    check_mirrors(work)

    print("\n[3/4] regenerating must change nothing")
    before = {n: (work / "talib" / n).read_text() for n in ("_func.pxi", "_stream.pxi")}
    stub, stream_stub = regenerate(work, work)
    after = {n: (work / "talib" / n).read_text() for n in ("_func.pxi", "_stream.pxi")}
    for name in before:
        (ok if before[name] == after[name] else fail)(
            "%s matches what the headers generate" % name)
    for name, want, tool in (("abstract.pyi", stub, "generate_abstract_stub.py"),
                             ("stream.pyi", stream_stub, "generate_stream.py --stub")):
        if want.strip() == (work / "talib" / name).read_text().strip():
            ok("%s matches %s" % (name, tool))
        else:
            fail("%s is stale" % name)

    print("\n[4/4] build without Cython (the sdist path, using the committed .c)")
    hide_cython_dir()
    (work / "talib" / "_ta_lib.c").write_text(pristine_c)
    build_wrapper(work, hide_cython=True)
    passed, line = pytest(work)
    (ok if passed else fail)("suite, built from _ta_lib.c: %s" % line)


def target_sync():
    version, cython = cython_pin()
    added = sync_pxd(WRAPPER)
    if added:
        print("declared in _ta_lib.pxd: %s" % " ".join(added))
    work = work_copy()
    build_wrapper(work)
    print("\nregenerating into %s" % WRAPPER)
    regenerate(work, WRAPPER)
    print("refreshing _ta_lib.c with Cython %s" % version)
    # Cython embeds paths RELATIVE to the cwd, so this reproduces the committed
    # file byte for byte only from the wrapper's root.
    run([str(cython), "talib/_ta_lib.pyx"], cwd=WRAPPER)
    print("\nwritten, not committed:")
    print(git(WRAPPER, "status", "--short") or "  (nothing changed)")
    print("\nA new function, moving average or unstable-period id still needs its "
          "line by hand in __function_groups__, MA_Type, the unstable table, "
          "_ta_lib.pyi and docs/. `check` names the ones that are missing.")


def target_wheel():
    work = work_copy()
    shutil.rmtree(work / "dist", ignore_errors=True)
    run([sys.executable, "-m", "build", "--wheel", "--no-isolation"],
        cwd=work, env=env_for(work))
    wheel = next((work / "dist").glob("*.whl"), None)
    if wheel is None:
        sys.exit("no wheel in %s" % (work / "dist"))
    target = CACHE / "wheeltest"
    shutil.rmtree(target, ignore_errors=True)
    run([sys.executable, "-m", "pip", "install", "--quiet", "--target", str(target), str(wheel)])
    # ta-lib-python's own CIBW_TEST_COMMAND: run from outside the source tree.
    r = subprocess.run([sys.executable, "-m", "pytest", "-q",
                        "-k", "not RSI and not threading", str(work / "tests")],
                       cwd=str(CACHE), text=True, capture_output=True,
                       env=dict(os.environ, PYTHONPATH=str(target)))
    line = (r.stdout or "").strip().splitlines()[-1:] or [""]
    print("wheel: %s" % wheel.name)
    (ok if r.returncode == 0 else fail)("release test command: %s" % line[0])


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("target", nargs="?", default="check",
                        choices=["check", "sync", "wheel"])
    parser.add_argument("--any-branch", action="store_true",
                        help="skip the dev-branch assertion on both repos")
    args = parser.parse_args()

    CACHE.mkdir(parents=True, exist_ok=True)
    require_branches(args.any_branch)
    ensure_packages(["numpy", "Cython", "setuptools", "pytest", "pandas", "polars"]
                    + (["build", "wheel", "cmake"] if args.target == "wheel" else []),
                    requirements=("requirements_test.txt",))
    build_c()

    {"check": target_check, "sync": target_sync, "wheel": target_wheel}[args.target]()

    print()
    if FAILURES:
        print("%d problem(s). `%s sync` regenerates what it can."
              % (len(FAILURES), Path(__file__).name))
        sys.exit(1)
    if args.target == "check":
        print("ta-lib-python/dev is ready for this worktree's C library.")


if __name__ == "__main__":
    main()
