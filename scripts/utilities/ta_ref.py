"""Build the frozen-release serves, one per member of ta_ref/.

A member is ta_ref/ta_ref_<X>_<Y>_<Z>.c. Its serve, bin/ta_ref_<X>_<Y>_<Z>_serve,
is the current generated transport linked against release v<X>.<Y>.<Z>'s
libta-lib.a: the one its Linux package shipped, authenticated by the commit the
member pins, or a source build of that commit where no shipped library is sound
on this host. Only VALUES are frozen: metadata answers come from the current
tree (#161), so no metadata gate may be built on a serve.

The facts that make linking the current transport against an old library sound
are checked here for every member, never assumed: identical batch prototypes,
unstable-period ids that name the same functions, and the MAType ceiling.

Libraries and serves are cached per machine under $XDG_CACHE_HOME/ta-lib/ta_ref
(default ~/.cache), keyed by content, so a worktree whose generated transport
matches one already built reuses its serve. Deleting that directory is safe.
"""

import glob
import hashlib
import io
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

# An oracle's values must differ from the tree's only through its code (#150).
# -ffp-contract=off makes explicit fma() the only fusion: a release built without
# it fuses inside its own library on a host with baseline FMA (any aarch64), so
# there its package is refused and a source build stands in. FROZEN_CFLAGS apply
# to source builds only; a shipped library keeps its release's flags, so its
# ta_bench timings can include an errno-guarded sqrt (#192).
FP_CONTRACT_FLAG = "-ffp-contract=off"
MATH_ERRNO_FLAG = "-fno-math-errno"
FROZEN_CFLAGS = f"{FP_CONTRACT_FLAG} {MATH_ERRNO_FLAG}"

MEMBER_RE = re.compile(r'^ta_ref_(\d+)_(\d+)_(\d+)\.c$')
SHARED_FILES = ('ta_ref.h', 'ta_ref_serve.c')

RELEASE_URL = 'https://github.com/TA-Lib/ta-lib/releases/download/{tag}/{asset}'
DEB_ARCH = {'x86_64': 'amd64', 'aarch64': 'arm64'}
SERVES_KEPT = 32


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


def cache_dir():
    base = os.environ.get('XDG_CACHE_HOME') or os.path.join(os.path.expanduser('~'), '.cache')
    return os.path.join(base, 'ta-lib', 'ta_ref')


def _mkdtemp(parent, prefix):
    os.makedirs(parent, exist_ok=True)
    return tempfile.mkdtemp(prefix=prefix, dir=parent)


def _publish(tmp, final):
    """Moves a complete tmp directory into place. A concurrent builder of the same
    key may have got there first; its content is equivalent, so ours is dropped."""
    try:
        os.rename(tmp, final)
    except OSError:
        if not os.path.isdir(final):
            raise
        shutil.rmtree(tmp, ignore_errors=True)


def _sha256(data):
    return hashlib.sha256(data if isinstance(data, bytes) else data.encode()).hexdigest()


def _cc_id():
    return subprocess.run(['cc', '--version'], capture_output=True, text=True).stdout.split('\n')[0]


def _func_names(text):
    """The function names of a ta_func_list.txt. Never an archive's symbol table:
    v0.6.4 compiles unlisted NVI/PVI stubs whose prototypes differ from the
    current ones."""
    return {line.split()[0] for line in text.splitlines() if line.strip()}


# One list_functions entry; only the first carries no leading comma.
_LIST_ENTRY_RE = re.compile(
    r'^(?P<head>[^\n]*json_appendf\([^\n]*?, ")(?P<comma>,?)(?P<tail>\\"TA_[A-Z0-9_]+\\"[^\n]*)$',
    re.M)


def _filter_list_functions(text, absent):
    """Drops the absent functions from the transport's list_functions payload,
    then re-normalizes the leading commas: removing the first entry would leave
    the next one's comma dangling."""
    removed = dict.fromkeys(absent, 0)

    def drop(m):
        if m.group(1) not in removed:
            return m.group(0)
        removed[m.group(1)] += 1
        return ''
    text = re.sub(r'^[^\n]*\\"TA_([A-Z0-9_]+)\\"[^\n]*\n', drop, text, flags=re.M)
    for name, n in removed.items():
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


def _tag(v):
    return 'v' + v.replace('_', '.')


def _pinned_commit(root, v):
    with open(_member_path(root, v)) as f:
        m = re.search(r'\.commit\s*=\s*"([0-9a-f]{40})"', f.read())
    if not m:
        raise RefError(f"ta_ref_{v}.c pins no commit (.commit = \"<40 hex>\")")
    return m.group(1)


def _git(root, *args):
    return subprocess.run(['git', *args], cwd=root, capture_output=True, text=True)


def _git_blob(root, commit, path):
    r = subprocess.run(['git', 'cat-file', 'blob', f'{commit}:{path}'], cwd=root,
                       capture_output=True)
    return r.stdout if r.returncode == 0 else None


def _verify_tag(root, v, commit):
    tag = _tag(v)
    r = _git(root, 'rev-parse', '--verify', '--quiet', f'refs/tags/{tag}^{{commit}}')
    if r.returncode != 0:
        raise RefError(f"tag {tag} is unavailable; fetch tags (git fetch --tags, or "
                       f"actions/checkout with fetch-depth: 0)")
    if r.stdout.strip() != commit:
        raise RefError(f"tag {tag} is {r.stdout.strip()} but ta_ref_{v}.c pins {commit}")


def _shipped(root, v, commit):
    """(asset, sha256, bytes or None) for the release's package on this host, or a
    string saying why a source build stands in. The pinned commit authenticates
    the package: it holds either the package itself or its dist/digests record."""
    arch = DEB_ARCH.get(platform.machine())
    if platform.system() != 'Linux' or platform.libc_ver()[0] != 'glibc' or arch is None:
        return f"no release package for {platform.system()} {platform.machine()}"
    if arch == 'arm64' and FP_CONTRACT_FLAG.encode() not in (_git_blob(root, commit, 'CMakeLists.txt') or b''):
        return "its build fuses multiply-adds on aarch64 (#150)"
    asset = f"ta-lib_{v.replace('_', '.')}_{arch}.deb"
    blob = _git_blob(root, commit, f'dist/{asset}')
    if blob is not None:
        return asset, _sha256(blob), blob
    record = _git_blob(root, commit, f'dist/digests/{asset}.digest')
    if record is None:
        return f"the release shipped no {asset}"
    return asset, json.loads(record)['package_sha256'], None


def _download(v, asset, sha):
    url = RELEASE_URL.format(tag=_tag(v), asset=asset)
    print(f"  downloading {url}")
    try:
        with urllib.request.urlopen(url, timeout=120) as r:
            data = r.read()
    except OSError as e:
        raise RefError(f"download of {url} failed: {e}")
    if _sha256(data) != sha:
        raise RefError(f"{asset}: sha256 {_sha256(data)} is not the {sha} its release recorded")
    return data


def _unpack_deb(data, out):
    """libta-lib.a and the public headers of a .deb (an ar archive holding a
    data.tar.*), without dpkg."""
    if not data.startswith(b'!<arch>\n'):
        raise RefError("the package is not an ar archive")
    pos, tarball = 8, None
    while pos + 60 <= len(data):
        size = int(data[pos + 48:pos + 58])
        if data[pos:pos + 16].startswith(b'data.tar'):
            tarball = data[pos + 60:pos + 60 + size]
        pos += 60 + size + (size & 1)
    if tarball is None:
        raise RefError("the package has no data.tar member")
    os.makedirs(os.path.join(out, 'include'))
    with tarfile.open(fileobj=io.BytesIO(tarball)) as tar:
        for m in tar:
            path = os.path.normpath(m.name)
            if not m.isfile():
                continue
            if path == 'usr/lib/libta-lib.a':
                dest = os.path.join(out, 'libta-lib.a')
            elif os.path.dirname(path) == 'usr/include/ta-lib' and path.endswith('.h'):
                dest = os.path.join(out, 'include', os.path.basename(path))
            else:
                continue
            with open(dest, 'wb') as f:
                f.write(tar.extractfile(m).read())
    for need in ('libta-lib.a', os.path.join('include', 'ta_func.h'), os.path.join('include', 'ta_defs.h')):
        if not os.path.exists(os.path.join(out, need)):
            raise RefError(f"the package has no {need}")


def _build_from_source(root, commit, out):
    """The release's static library and public headers, built from the pinned
    commit with FROZEN_CFLAGS."""
    work = _mkdtemp(os.path.dirname(out), '.src-')
    try:
        src = os.path.join(work, 'src')
        os.makedirs(src)
        archive = subprocess.Popen(['git', 'archive', commit], cwd=root, stdout=subprocess.PIPE)
        untar = subprocess.run(['tar', '-x', '-C', src], stdin=archive.stdout)
        archive.stdout.close()
        if archive.wait() != 0 or untar.returncode != 0:
            raise RefError(f"git archive {commit} failed")
        build = os.path.join(work, 'build')
        os.makedirs(build)
        _run_quiet(["cmake", src, "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_C_FLAGS={FROZEN_CFLAGS}"],
                   build, "cmake configure")
        _run_quiet(["cmake", "--build", ".", "--target", "ta-lib-static",
                    "-j", str(os.cpu_count() or 4)], build, "cmake build")
        os.makedirs(os.path.join(out, 'include'))
        for h in glob.glob(os.path.join(src, 'include', '*.h')):
            shutil.copy2(h, os.path.join(out, 'include'))
        shutil.copy2(os.path.join(build, 'libta-lib.a'), out)
    finally:
        shutil.rmtree(work, ignore_errors=True)


def _frozen_lib(root, v, commit):
    """(include dir, libta-lib.a) of the release, filling the machine cache on a miss."""
    shipped = _shipped(root, v, commit)
    if isinstance(shipped, str):
        key = f"src-{commit[:12]}-{_sha256(FROZEN_CFLAGS + _cc_id())[:12]}"
        how = f"source build of {commit[:12]}: {shipped}"
    else:
        asset, sha, blob = shipped
        key = f"deb-{sha[:16]}"
        how = f"{asset} (sha256 {sha[:12]})"
    final = os.path.join(cache_dir(), 'lib', key)
    if not os.path.isdir(final):
        tmp = _mkdtemp(os.path.dirname(final), '.tmp-')
        try:
            if isinstance(shipped, str):
                print(f"  building the library from source ({shipped})")
                _build_from_source(root, commit, tmp)
            else:
                _unpack_deb(blob if blob is not None else _download(v, asset, sha), tmp)
            _publish(tmp, final)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
    print(f"  library: {how}")
    return os.path.join(final, 'include'), os.path.join(final, 'libta-lib.a')


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


def _check_abi(root, include, common, v, work):
    """Refuses a member whose frozen library the current transport cannot call
    soundly. Returns the MAType ceiling."""
    cur = _batch_decls(os.path.join(root, 'include', 'ta_func.h'))
    old = _batch_decls(os.path.join(include, 'ta_func.h'))
    for name in sorted(common):
        for sym in (f'TA_{name}', f'TA_S_{name}', f'TA_{name}_Lookback'):
            if sym in cur and cur.get(sym) != old.get(sym):
                raise RefError(f"ta_ref_{v}: {sym} differs from the current prototype; "
                               f"the transport would call it through the wrong one")

    cur_unst = {val: n for n, val in
                _enum_values(os.path.join(root, 'include'), 'TA_FUNC_UNST_', work, 'cur').items()}
    old_unst = {val: n for n, val in
                _enum_values(include, 'TA_FUNC_UNST_', work, v).items()}
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

    return max(_enum_values(include, 'TA_MAType_', work, v).values()), unst


def _transport(root, work, post_funcs):
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


def _preprocessed(cmd, source):
    """The translation unit cmd compiles, preprocessed without line markers, so no
    path of this worktree reaches the cache key."""
    r = subprocess.run([*cmd, '-E', '-P', source], capture_output=True)
    if r.returncode != 0:
        sys.stderr.write(r.stderr.decode(errors='replace'))
        raise RefError(f"preprocessing {source} failed")
    return r.stdout


def _prune(parent, keep):
    entries = sorted((e for e in os.scandir(parent) if e.is_dir() and not e.name.startswith('.')),
                     key=lambda e: e.stat().st_mtime, reverse=True)
    for e in entries[keep:]:
        shutil.rmtree(e.path, ignore_errors=True)


def _install(exe, bin_exe):
    """Copies exe to bin_exe unless it is already there. The copy is renamed into
    place, so a serve that is running keeps its own file."""
    if os.path.exists(bin_exe):
        with open(exe, 'rb') as a, open(bin_exe, 'rb') as b:
            if a.read() == b.read():
                return False
    os.makedirs(os.path.dirname(bin_exe), exist_ok=True)
    tmp = f"{bin_exe}.{os.getpid()}.tmp"
    shutil.copy2(exe, tmp)
    os.replace(tmp, bin_exe)
    return True


def build_serve(root, build_dir, v):
    """Installs bin/ta_ref_<v>_serve, compiling only on a machine-cache miss."""
    print(f"=== {serve_name(v)} ===")
    commit = _pinned_commit(root, v)
    _verify_tag(root, v, commit)
    include, lib_a = _frozen_lib(root, v, commit)
    work = os.path.join(build_dir, 'ta_ref', v)
    os.makedirs(work, exist_ok=True)

    with open(os.path.join(root, 'ta_func_list.txt')) as f:
        current = _func_names(f.read())
    listed = _git_blob(root, commit, 'ta_func_list.txt')
    if listed is None:
        raise RefError(f"{commit[:12]} has no ta_func_list.txt")
    release = _func_names(listed.decode())
    matype_max, unst = _check_abi(root, include, current & release, v, work)
    post_funcs = sorted(current - release)
    transport = _transport(root, work, post_funcs)
    # The unstable-period ids each function's own handler sets, verified above
    # to name the same functions in the release: abstract_call applies them.
    rows = '\n'.join('   { "%s", %d, { %s } },' % (n, len(ids), ', '.join(map(str, ids)))
                     for n, ids in sorted(unst.items()))
    width = max((len(ids) for ids in unst.values()), default=1)
    _write_if_changed(os.path.join(work, 'ta_ref_unst.h'),
                      f'#define TA_REF_MAX_UNST_IDS {width}\n'
                      f'static const struct {{ const char *func; int nb; int ids[TA_REF_MAX_UNST_IDS]; }}\n'
                      f'ta_ref_unst[] = {{\n{rows}\n   {{ NULL, 0, {{ 0 }} }}\n}};\n')

    # FP_CONTRACT_FLAG is load-bearing even where the release's own build sets it:
    # this TU compiles fuzz_data.h, whose FP_CONTRACT pragma GCC ignores, and the
    # seeded inputs must be generated exactly as ta_regtest generates them.
    flags = ['-O3', '-flto', '-DNDEBUG', FP_CONTRACT_FLAG, MATH_ERRNO_FLAG]
    cc_t = (['cc', *flags, '-Wno-everything', '-DTA_REF_SERVE',
             f'-DTA_REF_VERSION="{v}"', f'-DTA_REF_MATYPE_MAX={matype_max}']
            + [f'-I{d}' for d in _include_dirs(root, work)])
    cc_m = ['cc', *flags, '-Wall', '-Wextra', f'-I{ref_dir(root)}']
    member = _member_path(root, v)

    key = hashlib.sha256()
    for part in (_cc_id().encode(), ' '.join(flags).encode(),
                 _preprocessed(cc_t, transport), _preprocessed(cc_m, member)):
        key.update(_sha256(part).encode())
    with open(lib_a, 'rb') as f:
        key.update(_sha256(f.read()).encode())
    serves = os.path.join(cache_dir(), 'serve')
    entry = os.path.join(serves, key.hexdigest()[:24])
    cached = os.path.join(entry, serve_name(v))

    if os.path.exists(cached):
        os.utime(entry)
        how = "cached"
    else:
        t_obj = os.path.join(work, 'transport.o')
        m_obj = os.path.join(work, 'member.o')
        for cmd in (cc_t + ['-c', transport, '-o', t_obj], cc_m + ['-c', member, '-o', m_obj]):
            if subprocess.run(cmd).returncode != 0:
                raise RefError(f"{serve_name(v)}: compile failed")
        exe = os.path.join(work, serve_name(v))
        link = ['cc', *flags, '-o', exe, t_obj, m_obj, lib_a, '-lm']
        if _gnu_ld():
            link.append('-Wl,--trace-symbol=TA_Globals')
        r = subprocess.run(link, capture_output=True, text=True)
        sys.stdout.write(r.stdout if r.returncode else '')
        sys.stderr.write(r.stderr if r.returncode else '')
        if r.returncode != 0:
            raise RefError(f"{serve_name(v)}: link failed")
        # The frozen TA_Globals has the release's layout, not the current headers'.
        for line in (r.stdout + r.stderr).splitlines():
            if 'reference to TA_Globals' in line and os.path.basename(lib_a) not in line:
                raise RefError(f"{serve_name(v)}: the transport reads TA_Globals ({line.strip()})")
        tmp = _mkdtemp(serves, '.tmp-')
        try:
            shutil.copy2(exe, os.path.join(tmp, serve_name(v)))
            _publish(tmp, entry)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)
        _prune(serves, SERVES_KEPT)
        how = "built"

    installed = _install(cached, os.path.join(root, 'bin', serve_name(v)))
    print(f"  {serve_name(v)}: {how}" + ("" if installed else ", bin/ up to date")
          + (f"; {len(post_funcs)} newer function(s) refused" if post_funcs else ""))
