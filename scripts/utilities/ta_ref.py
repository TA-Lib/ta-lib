"""Build the frozen-release serves, one per member of ta_ref/.

A member is ta_ref/ta_ref_<X>_<Y>_<Z>.c. Its serve, bin/ta_ref_<X>_<Y>_<Z>_serve,
is the current generated transport linked against the libta-lib.a of tag
v<X>.<Y>.<Z>, built from the commit the member pins. Only VALUES are frozen:
metadata answers come from the current tree (#161), so no metadata gate may be
built on a serve.

The facts that make linking the current transport against an old library sound
are checked here for every member, never assumed: identical batch prototypes,
unstable-period ids that name the same functions, and the MAType ceiling.
"""

import hashlib
import os
import re
import shutil
import subprocess
import sys

# An oracle must differ from the tree in its code, never in how its compiler was
# told to behave (issue #150, #192). -ffp-contract=off makes explicit fma() the
# only fusion; without it a target with baseline FMA (any aarch64) fuses inside
# the release and the gate measures a compiler difference. -fno-math-errno
# cannot change a value, but a scalar sqrt left in the release reads as an
# algorithm difference under ta_bench.
FP_CONTRACT_FLAG = "-ffp-contract=off"
MATH_ERRNO_FLAG = "-fno-math-errno"
FROZEN_CFLAGS = f"{FP_CONTRACT_FLAG} {MATH_ERRNO_FLAG}"

MEMBER_RE = re.compile(r'^ta_ref_(\d+)_(\d+)_(\d+)\.c$')
SHARED_FILES = ('ta_ref.h', 'ta_ref_serve.c')


class RefError(Exception):
    pass


def _run_quiet(cmd, cwd, what):
    """Run cmd, swallowing output unless it fails."""
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stdout.write(r.stdout)
        sys.stderr.write(r.stderr)
        raise RefError(f"{what} failed (exit {r.returncode})")
    return r.stdout


def _build_frozen_lib(src, build, label):
    """Configure and build the release's static library. Unconditional: cmake's
    own tracking is the "build once" mechanism. A cache configured without
    FROZEN_CFLAGS is discarded rather than reconfigured in place, because
    mtime-based invalidation within one second recompiles only some objects."""
    cache = os.path.join(build, "CMakeCache.txt")
    if os.path.exists(cache):
        with open(cache) as f:
            flags = next((line for line in f if line.startswith("CMAKE_C_FLAGS:")), "")
        if any(flag not in flags for flag in FROZEN_CFLAGS.split()):
            print(f"  {label}: discarding a build tree configured without {FROZEN_CFLAGS}")
            shutil.rmtree(build)
    os.makedirs(build, exist_ok=True)
    _run_quiet(["cmake", src, "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_C_FLAGS={FROZEN_CFLAGS}"],
               build, f"{label}: cmake configure")
    out = _run_quiet(["cmake", "--build", ".", "--target", "ta-lib-static",
                      "-j", str(os.cpu_count() or 4)], build, f"{label}: cmake build")
    if "Building C object" in out:
        print(f"  {label}: rebuilt frozen libta-lib.a ({FROZEN_CFLAGS})")
    return os.path.join(build, "libta-lib.a")


def _func_names(root):
    """The function names of a tree's ta_func_list.txt. Never the archive's symbol
    table: v0.6.4 compiles unlisted NVI/PVI stubs whose prototypes differ from the
    current ones."""
    with open(os.path.join(root, "ta_func_list.txt")) as f:
        return {line.split()[0] for line in f if line.strip()}


# One list_functions entry; only the first carries no leading comma.
_LIST_ENTRY_RE = re.compile(
    r'^(?P<head>[^\n]*json_appendf\([^\n]*?, ")(?P<comma>,?)(?P<tail>\\"TA_[A-Z0-9_]+\\"[^\n]*)$',
    re.M)


def _filter_list_functions(text, absent):
    """Drops the absent functions from the transport's list_functions payload,
    then re-normalizes the leading commas: removing the first entry would leave
    the next one's comma dangling."""
    for name in absent:
        text, n = re.subn(r'[^\n]*\\"TA_' + re.escape(name) + r'\\"[^\n]*\n', '', text)
        if n != 1:
            raise RefError(f"list_functions: expected 1 TA_{name} entry, removed {n}")
    entries = list(_LIST_ENTRY_RE.finditer(text))
    if not entries:
        raise RefError("list_functions: no entry left")
    for i, m in reversed(list(enumerate(entries))):
        repl = m.group('head') + (',' if i > 0 else '') + m.group('tail')
        if repl != m.group(0):
            text = text[:m.start()] + repl + text[m.end():]
    return text


def _stub_definitions(absent, header):
    """A `{ return TA_BAD_PARAM; }` definition, on the exact ta_func.h prototype,
    for every batch symbol of an absent function, so the transport links. Plain C,
    so no linker-specific flag. Never reached: the serve refuses those names."""
    with open(header) as f:
        text = f.read()
    defs = []
    for name in absent:
        for sym in (f"TA_{name}", f"TA_S_{name}", f"TA_{name}_Lookback", f"TA_S_{name}_Lookback"):
            m = re.search(r'(TA_LIB_API\s+[\w ]+?' + re.escape(sym) + r'\s*\([^;]*\))\s*;', text)
            if m:
                defs.append(m.group(1).strip() + " { return TA_BAD_PARAM; }")
    return "\n" + "\n".join(defs) + "\n" if defs else ""


def ref_dir(root):
    return os.path.join(root, 'ta_ref')


def members(root):
    """Every member version ('0_6_4', ...), oldest first. Anything in ta_ref/
    that is neither a member nor a shared file is refused, so a misnamed member
    cannot silently drop out."""
    found = []
    for name in os.listdir(ref_dir(root)):
        if name in SHARED_FILES:
            continue
        m = MEMBER_RE.match(name)
        if not m:
            raise RefError(f"ta_ref/{name} is neither a member (ta_ref_<X>_<Y>_<Z>.c) "
                           f"nor one of {', '.join(SHARED_FILES)}")
        found.append(tuple(int(g) for g in m.groups()))
    if not found:
        raise RefError("ta_ref/ has no member")
    return ['_'.join(str(p) for p in v) for v in sorted(found)]


def select(root, csv):
    """The members named by csv (dots or underscores), or all of them."""
    available = members(root)
    if not csv or csv == 'all':
        return available
    picked = []
    for token in csv.split(','):
        v = token.strip().lstrip('v').replace('.', '_')
        if v not in available:
            raise RefError(f"no member {token!r}; ta_ref/ has {', '.join(available)}")
        if v not in picked:
            picked.append(v)
    return picked


def newest(root):
    return members(root)[-1]


def serve_name(v):
    return f"ta_ref_{v}_serve"


def _member_path(root, v):
    return os.path.join(ref_dir(root), f"ta_ref_{v}.c")


def _pinned_commit(root, v):
    with open(_member_path(root, v)) as f:
        m = re.search(r'\.commit\s*=\s*"([0-9a-f]{40})"', f.read())
    if not m:
        raise RefError(f"ta_ref_{v}.c pins no commit (.commit = \"<40 hex>\")")
    return m.group(1)


def _git(root, *args):
    return subprocess.run(['git', *args], cwd=root, capture_output=True, text=True)


def _verify_tag(root, v, commit):
    tag = 'v' + v.replace('_', '.')
    r = _git(root, 'rev-parse', '--verify', '--quiet', f'refs/tags/{tag}^{{commit}}')
    if r.returncode != 0:
        raise RefError(f"tag {tag} is unavailable; fetch tags (git fetch --tags, or "
                       f"actions/checkout with fetch-depth: 0)")
    if r.stdout.strip() != commit:
        raise RefError(f"tag {tag} is {r.stdout.strip()} but ta_ref_{v}.c pins {commit}")


def _frozen_src(root, build_dir, v, commit):
    """The member's tree, extracted once per pinned commit. git archive stamps
    every file with the commit time, older than any object already built, so a
    tree is never refreshed in place: a new pin gets a new directory."""
    base = os.path.join(build_dir, 'ta_ref', v, commit[:12])
    src = os.path.join(base, 'src')
    if not os.path.isdir(src):
        tmp = src + '.tmp'
        shutil.rmtree(tmp, ignore_errors=True)
        os.makedirs(tmp)
        archive = subprocess.Popen(['git', 'archive', commit], cwd=root, stdout=subprocess.PIPE)
        untar = subprocess.run(['tar', '-x', '-C', tmp], stdin=archive.stdout)
        archive.stdout.close()
        if archive.wait() != 0 or untar.returncode != 0:
            raise RefError(f"git archive {commit} failed")
        os.rename(tmp, src)
    return base, src


def _enum_values(include_dir, prefix, work, tag):
    """{name: value} for every `<prefix><NAME>` the header's enum defines,
    evaluated by the compiler rather than parsed."""
    with open(os.path.join(include_dir, 'ta_defs.h')) as f:
        names = sorted(set(re.findall(r'\b' + prefix + r'(\w+)\b', f.read())))
    probe = os.path.join(work, f'enum_{tag}.c')
    with open(probe, 'w') as f:
        f.write('#include <stdio.h>\n#include "ta_defs.h"\nint main(void) {\n')
        for n in names:
            f.write(f'   printf("%s %d\\n", "{n}", (int){prefix}{n});\n')
        f.write('   return 0;\n}\n')
    exe = probe[:-2]
    r = subprocess.run(['cc', '-w', f'-I{include_dir}', '-o', exe, probe],
                       capture_output=True, text=True)
    if r.returncode != 0:
        raise RefError(f"enum probe failed:\n{r.stderr}")
    out = subprocess.run([exe], capture_output=True, text=True, check=True).stdout
    return {k: int(v) for k, v in (line.split() for line in out.splitlines())}


_DECL_RE = re.compile(r'TA_LIB_API\s+[^;{]*?\b(TA_\w+)\s*\(([^;]*?)\)\s*;', re.S)


def _batch_decls(header):
    with open(header) as f:
        text = re.sub(r'/\*.*?\*/', ' ', f.read(), flags=re.S)
    return {m.group(1): ' '.join(m.group(0).split()) for m in _DECL_RE.finditer(text)}


def _check_abi(root, src, v, work):
    """Refuses a member whose frozen library the current transport cannot call
    soundly. Returns the MAType ceiling."""
    common = _func_names(root) & _func_names(src)
    cur = _batch_decls(os.path.join(root, 'include', 'ta_func.h'))
    old = _batch_decls(os.path.join(src, 'include', 'ta_func.h'))
    for name in sorted(common):
        for sym in (f'TA_{name}', f'TA_S_{name}', f'TA_{name}_Lookback'):
            if sym in cur and cur.get(sym) != old.get(sym):
                raise RefError(f"ta_ref_{v}: {sym} differs from the current prototype; "
                               f"the transport would call it through the wrong one")

    cur_unst = {val: n for n, val in
                _enum_values(os.path.join(root, 'include'), 'TA_FUNC_UNST_', work, 'cur').items()}
    old_unst = {val: n for n, val in
                _enum_values(os.path.join(src, 'include'), 'TA_FUNC_UNST_', work, v).items()}
    with open(os.path.join(root, 'ta_codegen', 'output', 'c', 'tools', 'ta_codegen_serve.c')) as f:
        transport = f.read()
    dispatch = transport[transport.index('static void handle_request('):]
    branches = re.split(r'strncmp\(method, "TA_(\w+)", \d+\) == 0 \) \{', dispatch)
    unst = {}
    for name, body in zip(branches[1::2], branches[2::2]):
        if name not in common:
            continue
        ids = sorted(set(int(i) for i in re.findall(r'TA_SetUnstablePeriod\((\d+),', body)))
        for uid in ids:
            if cur_unst.get(uid) != old_unst.get(uid):
                raise RefError(f"ta_ref_{v}: TA_{name}'s handler sets unstable id {uid}, "
                               f"which is {cur_unst.get(uid)} now and {old_unst.get(uid)} "
                               f"in the release")
        if ids:
            unst[name] = ids

    return max(_enum_values(os.path.join(src, 'include'), 'TA_MAType_', work, v).values()), unst


def _transport(root, src, work, post_funcs):
    """The generated server with the indicator and ta_common sources stripped,
    so the frozen library provides them."""
    with open(os.path.join(root, 'ta_codegen', 'output', 'c', 'tools', 'ta_codegen_serve.c')) as f:
        text = f.read()
    missing = [n for n in post_funcs if ('\\"TA_%s\\"' % n) not in text]
    if missing:
        raise RefError(f"the generated server has no list_functions entry for "
                       f"{', '.join(missing)}: it predates them. Run scripts/build.py "
                       f"generate (a --func-filtered generate skips the whole-corpus files).")
    if '"%016llx", bits' not in text:
        raise RefError("the transport's output serializer is not the hex-bits writer")
    text = re.sub(r'#include "ta_func/[^"]*\.c"\n', '', text)
    text = re.sub(r'#include "ta_common/[^"]*\.c"\n', '', text)
    text = text.replace('#include <stdio.h>',
                        '#include <stdio.h>\n#include "ta_func.h"\n'
                        '#include "ta_memory.h"\n#include "ta_utility.h"\n', 1)
    if post_funcs:
        text = _filter_list_functions(text, post_funcs)
        stubs = _stub_definitions(post_funcs, os.path.join(root, 'include', 'ta_func.h'))
        text = text.replace('int main(void) {', stubs + 'int main(void) {', 1)
    text = text.replace('int main(void) {',
                        'int main(void) { TA_Initialize(); '
                        'TA_RestoreCandleDefaultSettings(TA_AllCandleSettings);', 1)
    path = os.path.join(work, 'transport.c')
    _write_if_changed(path, text)
    absent = '\n'.join(f'   "{n}",' for n in post_funcs)
    _write_if_changed(os.path.join(work, 'ta_ref_absent.h'),
                      f'static const char *const ta_ref_absent[] = {{\n{absent}\n   NULL\n}};\n')
    return path


def _write_if_changed(path, text):
    if os.path.exists(path):
        with open(path) as f:
            if f.read() == text:
                return
    with open(path, 'w') as f:
        f.write(text)


def _deps(depfile):
    with open(depfile) as f:
        text = f.read().replace('\\\n', ' ')
    return text.split(':', 1)[1].split()


def _fingerprint(cmds, depfiles, lib_a):
    h = hashlib.sha256()
    for c in cmds:
        h.update('\0'.join(c).encode())
    for d in depfiles:
        if not os.path.exists(d):
            return None
        for path in _deps(d):
            if not os.path.exists(path):
                return None
            h.update(path.encode())
            with open(path, 'rb') as f:
                h.update(f.read())
    with open(lib_a, 'rb') as f:
        h.update(f.read())
    return h.hexdigest()


def _include_dirs(root, work):
    c_out = os.path.join(root, 'ta_codegen', 'output', 'c')
    return [
        work,
        ref_dir(root),
        os.path.join(root, 'src', 'tools', 'ta_regtest'),
        os.path.join(c_out, 'tools'),
        os.path.join(root, 'include'),
        os.path.join(c_out, 'ta_common'),
        os.path.join(c_out, 'ta_abstract'),
        os.path.join(c_out, 'ta_abstract', 'frames'),
        os.path.join(root, 'ta_codegen', 'generator', 'templates', 'c'),
        os.path.join(root, 'src', 'ta_common'),
        os.path.join(root, 'src', 'ta_func'),
        os.path.join(root, 'src'),
        os.path.join(root, 'src', 'ta_abstract'),
        os.path.join(root, 'src', 'ta_abstract', 'frames'),
    ]


def _gnu_ld():
    r = subprocess.run(['cc', '-Wl,--version', '-o', os.devnull, '-x', 'c', '-'],
                       input='int main(void){return 0;}', capture_output=True, text=True)
    return 'GNU ld' in r.stdout or 'GNU gold' in r.stdout


def build_serve(root, build_dir, v):
    """Builds bin/ta_ref_<v>_serve unless every input is unchanged."""
    print(f"=== {serve_name(v)} ===")
    commit = _pinned_commit(root, v)
    _verify_tag(root, v, commit)
    base, src = _frozen_src(root, build_dir, v, commit)
    work = os.path.join(base, 'serve')
    os.makedirs(work, exist_ok=True)

    lib_a = _build_frozen_lib(src, os.path.join(base, 'build'), serve_name(v))
    matype_max, unst = _check_abi(root, src, v, work)
    post_funcs = sorted(_func_names(root) - _func_names(src))
    transport = _transport(root, src, work, post_funcs)
    # The unstable-period ids each function's own handler sets, verified above
    # to name the same functions in the release: abstract_call applies them.
    rows = '\n'.join('   { "%s", %d, { %s } },' % (n, len(ids), ', '.join(map(str, ids)))
                     for n, ids in sorted(unst.items()))
    width = max((len(ids) for ids in unst.values()), default=1)
    _write_if_changed(os.path.join(work, 'ta_ref_unst.h'),
                      f'#define TA_REF_MAX_UNST_IDS {width}\n'
                      f'static const struct {{ const char *func; int nb; int ids[TA_REF_MAX_UNST_IDS]; }}\n'
                      f'ta_ref_unst[] = {{\n{rows}\n   {{ NULL, 0, {{ 0 }} }}\n}};\n')

    # FP_CONTRACT_FLAG is load-bearing even where the tag's own build sets it:
    # this TU compiles fuzz_data.h, whose FP_CONTRACT pragma GCC ignores, and the
    # seeded inputs must be generated exactly as ta_regtest generates them.
    flags = ['-O3', '-flto', '-DNDEBUG', FP_CONTRACT_FLAG, MATH_ERRNO_FLAG]
    t_obj = os.path.join(work, 'transport.o')
    m_obj = os.path.join(work, 'member.o')
    compile_t = (['cc', *flags, '-Wno-everything', '-DTA_REF_SERVE',
                  f'-DTA_REF_VERSION="{v}"', f'-DTA_REF_MATYPE_MAX={matype_max}']
                 + [f'-I{d}' for d in _include_dirs(root, work)]
                 + ['-MMD', '-MF', t_obj + '.d', '-c', transport, '-o', t_obj])
    compile_m = (['cc', *flags, '-Wall', '-Wextra', f'-I{ref_dir(root)}',
                  '-MMD', '-MF', m_obj + '.d', '-c', _member_path(root, v), '-o', m_obj])
    exe_tmp = os.path.join(work, serve_name(v))
    link = ['cc', *flags, '-o', exe_tmp, t_obj, m_obj, lib_a, '-lm']
    if _gnu_ld():
        link.append('-Wl,--trace-symbol=TA_Globals')

    bin_exe = os.path.join(root, 'bin', serve_name(v))
    stamp = os.path.join(work, 'stamp')
    depfiles = [t_obj + '.d', m_obj + '.d']
    fp = _fingerprint([compile_t, compile_m, link], depfiles, lib_a)
    if fp and os.path.exists(bin_exe) and os.path.exists(stamp):
        with open(stamp) as f:
            if f.read() == fp + _file_sha(bin_exe):
                print(f"  {serve_name(v)}: up to date")
                return

    for cmd in (compile_t, compile_m):
        if subprocess.run(cmd).returncode != 0:
            raise RefError(f"{serve_name(v)}: compile failed")
    r = subprocess.run(link, capture_output=True, text=True)
    sys.stdout.write(r.stdout if r.returncode else '')
    sys.stderr.write(r.stderr if r.returncode else '')
    if r.returncode != 0:
        raise RefError(f"{serve_name(v)}: link failed")
    # The frozen TA_Globals has the release's layout, not the current headers'.
    for line in (r.stdout + r.stderr).splitlines():
        if 'reference to TA_Globals' in line and os.path.basename(lib_a) not in line:
            raise RefError(f"{serve_name(v)}: the transport reads TA_Globals ({line.strip()})")

    os.makedirs(os.path.dirname(bin_exe), exist_ok=True)
    os.replace(exe_tmp, bin_exe)
    fp = _fingerprint([compile_t, compile_m, link], depfiles, lib_a)
    with open(stamp, 'w') as f:
        f.write(fp + _file_sha(bin_exe))
    print(f"  {serve_name(v)}: built from {commit[:12]}"
          + (f", {len(post_funcs)} newer function(s) refused" if post_funcs else ""))


def _file_sha(path):
    with open(path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()
