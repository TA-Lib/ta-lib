#!/usr/bin/env python3
"""The public C ABI (ABI.manifest) and the shared library version derived from it.

ABI.released is the surface of the last published release. TALIB_LIBRARY_VERSION
follows from the two: additions keep the SONAME; removing or changing API that
shipped moves it, once per release, and is recorded only with
`scripts/sync.py --accept-break`. scripts/sync.py writes all three; `check` is the
gate.

Sizes, offsets, enumerator and macro values are measured by compiling a probe
against the installed headers (CMake's LIB_HEADERS), never parsed. The built ELF
export table is checked against the declared TA_LIB_API set: nothing else sees
-fvisibility=hidden and src/libta-lib.map actually apply.

Not seen: two same-typed parameters swapped (names are dropped, so a rename is
not a break), TA_*DIGEST macros, and a behaviour change behind an unchanged
signature.
"""

import os
import re
import subprocess
import sys
import tempfile

MANIFEST = "ABI.manifest"
RELEASED = "ABI.released"
CANONICAL_REMOTE = "https://github.com/TA-Lib/ta-lib.git"
# The data model CI measures on; the ABI files are only ever written on it.
CI_PLATFORM = "platform pointer=8 long=8 int=4 enum=4"
# Bump when the manifest's shape or coverage changes; the next scripts/sync.py
# re-measures both files, so they are never compared across formats. A break
# already accepted this cycle must then be accepted again.
FORMAT = 9


def repo_root() -> str:
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def installed_headers(root: str) -> list:
    """The headers CMake installs -- the definition of "public"."""
    text = open(os.path.join(root, "CMakeLists.txt")).read()
    at = text.index("set(LIB_HEADERS")
    block = text[at:text.index(")", at)]
    names = set(re.findall(r"/include/([A-Za-z0-9_.]+\.h)", block))
    # Anything appended later counts too -- `set(...)` is not the whole list.
    for m in re.finditer(r"list\s*\(\s*APPEND\s+LIB_HEADERS(.*?)\)", text, re.S):
        names |= set(re.findall(r"/include/([A-Za-z0-9_.]+\.h)", m.group(1)))
    # And what autotools installs, which is a separate list in three Makefile.am
    # files. The two build systems shipping different public headers is the same
    # class of defect as their shipping different sonames.
    auto = set()
    for rel in ("src/ta_abstract/Makefile.am", "src/ta_func/Makefile.am",
                "src/ta_common/Makefile.am"):
        path = os.path.join(root, rel)
        if not os.path.exists(path):
            continue
        am = open(path).read().replace("\\\n", " ")
        for m in re.finditer(r"^\w+_HEADERS\s*=([^\n]*)$", am, re.M):
            auto |= set(re.findall(r"include/([A-Za-z0-9_.]+\.h)", m.group(1)))
    if auto and auto != names:
        sys.exit("abi: CMake installs %s but autotools installs %s -- the two "
                 "build systems disagree about the public headers"
                 % (sorted(names), sorted(auto)))
    if len(names) < 3:
        sys.exit("abi: LIB_HEADERS parsed to %r -- the parse moved" % sorted(names))
    return sorted(names)


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def check_autotools_consumes_triple(root: str) -> None:
    """The 2018 regression was deleting this one line, and nothing noticed for
    seven years. CMake's half is checked against the built artifact; autotools'
    half is only ever exercised on a machine that runs autoreconf, so it is
    checked as text here."""
    am = os.path.join(root, "src", "Makefile.am")
    if not os.path.exists(am):
        return
    if "-version-info $(TALIB_LIBRARY_VERSION)" not in open(am).read():
        sys.exit("abi: src/Makefile.am no longer passes "
                 "`-version-info $(TALIB_LIBRARY_VERSION)`, so the autotools build "
                 "falls back to libtool's 0:0:0 and ships libta-lib.so.0 while CMake "
                 "ships what configure.ac declares")


def strip_preprocessor(text: str) -> str:
    """Directive lines blanked, continuations included, newlines kept.

    Declarations are found by splitting on `;`, and a directive carries none --
    so an unstripped `#define` runs straight into the declaration after it and
    is recorded as part of its signature.
    """
    out, continued = [], False
    for line in text.split("\n"):
        if continued or line.lstrip().startswith("#"):
            continued = line.rstrip().endswith("\\")
            out.append("")
        else:
            out.append(line)
    return "\n".join(out)


def abi_triple(root: str) -> tuple:
    """current:revision:age, and the SONAME libtool derives (current - age)."""
    m = re.search(r"^TALIB_LIBRARY_VERSION=(\d+):(\d+):(\d+)$",
                  open(os.path.join(root, "configure.ac")).read(), re.M)
    if not m:
        sys.exit("abi: configure.ac has no TALIB_LIBRARY_VERSION=c:r:a line. Put one back "
                 "with any value (e.g. TALIB_LIBRARY_VERSION=0:0:0) and run 'scripts/sync.py'; "
                 "it rewrites the value.")
    c, r, a = (int(x) for x in m.groups())
    return c, r, a, "libta-lib.so.%d" % (c - a)


def _drop_param_name(param: str) -> str:
    """`const double inReal[]` -> `const double []`. A parameter's NAME is not
    part of the ABI, and recording it would make a rename demand a soname bump."""
    param = " ".join(param.split())
    if not param or param == "void" or "(" in param:      # empty, void, fn-pointer
        return param
    # `T x[]` and `T *x` are the same parameter type after adjustment.
    param = re.sub(r"\[[^\]]*\]", "*", param)
    m = re.match(r"^(.*?)(\b\w+\b)\s*(\**)$", param)
    if not m or not m.group(1).strip():                   # a bare type, no name
        return param
    # `unsigned int` and `const char *` name no parameter: the trailing word is
    # the TYPE, and eating it would record two different signatures as one.
    if m.group(2) in ("int", "char", "long", "short", "float", "double", "void",
                      "unsigned", "signed", "const") or m.group(2).startswith("TA_"):
        return param
    return " ".join(("%s %s" % (m.group(1), m.group(3))).split())


def public_functions(headers_text: str) -> list:
    """Every TA_LIB_API declaration as return type + parameter TYPES, sorted."""
    out = set()
    # Split on `;` so a declaration is found wherever TA_LIB_API sits in it --
    # `TA_RetCode TA_LIB_API f(int);` is legal and used to record with its
    # return type silently dropped.
    # A brace ends a statement too, or `extern "C" {` joins the declaration
    # after it and is recorded as part of that signature.
    for chunk in headers_text.replace("{", ";").replace("}", ";").split(";"):
        if "TA_LIB_API" not in chunk:
            continue
        decl = " ".join(chunk.replace("TA_LIB_API", " ").split())
        if "(" not in decl:
            # Exported DATA: no parameter list, still part of the ABI.
            name = decl.split()[-1].lstrip("*") if decl.split() else ""
            if name:
                out.add("%s /* data */" % decl)
            continue
        head, _, rest = decl.partition("(")
        args = rest.rsplit(")", 1)[0]
        out.add("%s(%s)" % (head.strip(),
                            ", ".join(_drop_param_name(a) for a in args.split(","))))
    return sorted(out)


def public_macros(headers_text: str) -> tuple:
    """`#define TA_*` constants, split by how they must be measured.

    They are baked into every caller binary, so renumbering one is exactly the
    silent break this exists to catch. Floats are kept apart because reading
    TA_REAL_MAX through `(long long)` is undefined and saturates -- it recorded
    3e+37 and 4e+37 as the same number until this told them apart.

    Returns (integer names, float names, skipped names). Nothing TA_*-shaped is
    dropped quietly: a value shape this cannot classify is reported, because a
    macro silently outside the manifest is a macro nothing protects.
    """
    ints, floats, skipped = [], [], []
    for m in re.finditer(r"^[ \t]*#[ \t]*define[ \t]+(TA_[A-Z0-9_]+)[ \t]+(\S[^\n]*)$",
                         headers_text, re.M):
        name, val = m.group(1), m.group(2).strip()
        if "DIGEST" in name:
            continue          # the source digest churns on every source change
        # Take the literals out first, then judge what is left: a float exponent
        # is lowercase and a hex digit is a-f, so testing the raw text for
        # "letters" cannot tell 3e+37 from __declspec(dllexport).
        is_float = "0x" not in val.lower() and re.search(r"\d\.\d|\d[eE][+-]?\d", val)
        rest = re.sub(r"0[xX][0-9a-fA-F]+|\d+\.?\d*[eE][+-]?\d+|\d*\.\d+|\d+", " ", val)
        if val.count("(") != val.count(")"):
            skipped.append(name)                       # a function-like macro
        elif re.search(r"[A-Za-z_]\w*\s*\(", val):
            skipped.append(name)                       # invokes something: TA_LIB_API
        elif re.search(r"[a-z]", rest):
            skipped.append(name)                       # not a constant expression
        elif is_float:
            floats.append(name)
        elif re.fullmatch(r"[\(\)A-Z_0-9+\-*/<>|~ ]*", rest.replace("\t", " ")) is not None:
            ints.append(name)
        else:
            skipped.append(name)
    return sorted(set(ints)), sorted(set(floats)), sorted(set(skipped))


def public_scalar_typedefs(headers_text: str) -> list:
    """Public scalar typedefs. Every signature spells `TA_Real`, not `double`,
    so nothing else in this manifest moves when the typedef is retyped -- and
    `typedef double TA_Real` -> `long long` changes the argument and return
    class of all 2449 entry points. Measured, never recorded by name alone."""
    out = set()
    for m in re.finditer(r"^\s*typedef\s+((?:unsigned|signed|const)\s+)*"
                         r"(?:int|char|long|short|float|double)(?:\s+(?:int|long))*\s+"
                         r"(TA_\w+|U?Int(?:32|64))\s*;", headers_text, re.M):
        out.add(m.group(2))
    return sorted(out)


def public_callbacks(headers_text: str) -> list:
    """Public function-pointer typedefs. `TA_ForEachFunc` takes one, so retyping
    its parameters changes no other line in the manifest and every existing
    caller's callback is then invoked with a mismatched signature."""
    out = set()
    for m in re.finditer(r"typedef\s+([^;{}]*?)\(\s*\*\s*(TA_\w+)\s*\)\s*\(([^;{}]*)\)\s*;",
                         headers_text, re.S):
        ret = " ".join(m.group(1).split())
        args = ", ".join(_drop_param_name(a) for a in m.group(3).split(","))
        out.add("%s (*%s)(%s)" % (ret, m.group(2), args))
    return sorted(out)


def public_types(headers_text: str) -> tuple:
    """(struct name -> field names, enum name -> enumerator names).

    Only `typedef ... } TA_Name;` forms: an opaque handle has no layout to pin
    and a non-typedef'd tag is not part of the surface callers can spell.
    """
    structs, enums = {}, {}
    for m in re.finditer(r"typedef\s+struct[^{;]*\{(.*?)\}\s*(TA_\w+)\s*;",
                         headers_text, re.S):
        body, name = m.group(1), m.group(2)
        fields = []
        for decl in body.split(";"):
            decl = " ".join(decl.split())
            if not decl:
                continue
            fp = re.search(r"\(\s*\*\s*(\w+)\s*\)", decl)     # function pointer
            if fp:
                fields.append((fp.group(1), "fnptr"))
                continue
            # `unsigned int a, b;` declares two fields, and recording only the
            # last would leave the first with no offset in the manifest at all.
            lead = " ".join(decl.split(",")[0].split()[:-1])
            for part in decl.split(","):
                names = re.findall(r"[\w\*\[\]]+", part)
                last = re.sub(r"[\*\[\]0-9]", "", names[-1]) if names else ""
                if last and not last.isdigit():
                    # The TYPE too: an offset alone cannot see `TA_Integer` become
                    # `float`, which keeps every offset and changes what the bytes
                    # mean. Text-derived, unlike the offsets beside it.
                    # A later declarator carries no type of its own; it shares
                    # the first one's, and `or part` would record its NAME.
                    ty = " ".join(part.split()[:-1]) or lead
                    stars = part.count("*") + ("[" in part)
                    ty = re.sub(r"[\*\[\]]", " ", ty)
                    fields.append((last, "_".join((" ".join(ty.split())
                                                   + " *" * stars).split())))
        if fields:
            structs[name] = fields
    for m in re.finditer(r"typedef\s+enum[^{;]*\{(.*?)\}\s*(TA_\w+)\s*;",
                         headers_text, re.S):
        body, name = m.group(1), m.group(2)
        vals = re.findall(r"(\w+)\s*(?:=[^,}]*)?", body)
        enums[name] = [v for v in vals if v and not v.isdigit()]
    return structs, enums


def measure(root: str, structs: dict, enums: dict, macros: list, fmacros: list,
            typedefs: list) -> list:
    """Compile a probe and read the real sizes, offsets, enumerators and macros."""
    src = ['#include <stdio.h>', '#include <stddef.h>', '#include "ta_libc.h"',
           "int main(void){",
           'printf("platform pointer=%zu long=%zu int=%zu enum=%zu\\n",'
           ' sizeof(void*), sizeof(long), sizeof(int), sizeof(TA_RetCode));']
    for name in sorted(structs):
        src.append('printf("type %s sizeof=%%zu", sizeof(%s));' % (name, name))
        for f, ty in structs[name]:
            src.append('printf(" %s:%s=%%zu", offsetof(%s,%s));' % (f, ty, name, f))
        src.append('puts("");')
    for name in sorted(enums):
        src.append('printf("enum %s sizeof=%%zu\\n", sizeof(%s));' % (name, name))
        for v in enums[name]:
            src.append('printf("enum %s %s = %%lld\\n", (long long)%s);' % (name, v, v))
    for name in macros:
        src.append('printf("macro %s = %%lld\\n", (long long)(%s));' % (name, name))
    for name in fmacros:
        src.append('printf("macro %s = %%.17g\\n", (double)(%s));' % (name, name))
    for name in typedefs:
        # size, signedness and floatness: enough to separate double from
        # long long, int from unsigned int, and int from float.
        src.append('printf("typedef %s sizeof=%%zu signed=%%d float=%%d\\n", sizeof(%s),'
                   ' (int)((%s)-1 < (%s)0), (int)((%s)1/(%s)2 != (%s)0));'
                   % (name, name, name, name, name, name, name), )
    src.append("return 0;}")
    with tempfile.TemporaryDirectory() as d:
        c, exe = os.path.join(d, "p.c"), os.path.join(d, "p")
        open(c, "w").write("\n".join(src) + "\n")
        try:
            cc = subprocess.run(["gcc", "-I", os.path.join(root, "include"), "-o", exe, c],
                                capture_output=True, text=True)
        except FileNotFoundError:
            sys.exit("abi: needs gcc on PATH, on Linux x86_64 where CI measures: sizes and "
                     "offsets are measured, not parsed")
        if cc.returncode != 0:
            sys.exit("abi: the probe did not compile -- a public type is no "
                     "longer spellable from the installed headers:\n" + cc.stderr[:2000])
        run = subprocess.run([exe], capture_output=True, text=True)
        lines = run.stdout.splitlines()
        # A probe that died mid-run would otherwise hand back a TRUNCATED
        # surface, and sync.py would write that as the baseline.
        expected = 1 + len(structs) + len(enums) + sum(len(v) for v in enums.values()) \
                   + len(macros) + len(fmacros) + len(typedefs)
        if run.returncode != 0 or len(lines) != expected:
            sys.exit("abi: the probe exited %d with %d of %d expected line(s) -- "
                     "the measurement is incomplete, not a clean ABI"
                     % (run.returncode, len(lines), expected))
        return lines


def bind_enum_bounds(lines: list) -> list:
    """`macro TA_<STEM>_MIN/_MAX/_COUNT` recorded as a relation to its enum when the
    value matches, so appending an enumerator is an addition and not a break. A
    bound that stops matching keeps its number, and so reads as a break."""
    enums = {}
    for line in lines:
        m = re.match(r"^enum (\w+) (\w+) = (-?\d+)$", line)
        if m:
            enums.setdefault(m.group(1), []).append((m.group(2), int(m.group(3))))
    relations = {}
    for name, members in enums.items():
        prefix = os.path.commonprefix([n for n, _ in members])
        prefix = prefix[:prefix.rfind("_") + 1]
        if len(members) < 2 or prefix in ("", "TA_"):
            continue
        stem, values = prefix[:-1].upper(), {v for _, v in members}
        dense = next(i for i in range(len(values) + 1) if i not in values)
        relations[stem + "_MIN"] = (min(values), "min(%s)" % name)
        relations[stem + "_MAX"] = (max(values), "max(%s)" % name)
        relations[stem + "_COUNT"] = (dense, "dense-count(%s)" % name)
    out = []
    for line in lines:
        m = re.match(r"^macro (TA_\w+) = (-?\d+)$", line)
        if m and relations.get(m.group(1), (None,))[0] == int(m.group(2)):
            line = "macro %s = %s" % (m.group(1), relations[m.group(1)][1])
        out.append(line)
    return out


def build_manifest(root: str) -> str:
    check_autotools_consumes_triple(root)
    headers = installed_headers(root)
    text = strip_comments("\n".join(
        open(os.path.join(root, "include", h)).read() for h in headers))
    # Macros need the directives; declarations and layouts must not see them, or
    # a `#define` with no `;` runs into the declaration after it.
    code = strip_preprocessor(text)
    c, r, a, soname = abi_triple(root)
    structs, enums = public_types(code)
    funcs = public_functions(code)
    macros, fmacros, skipped = public_macros(text)
    callbacks = public_callbacks(code)
    typedefs = public_scalar_typedefs(code)
    if len(funcs) < 2000 or "TA_FuncInfo" not in structs or "TA_RetCode" not in enums:
        sys.exit("abi: surface looks wrong (%d functions, %d structs, %d enums) "
                 "-- the header parse moved" % (len(funcs), len(structs), len(enums)))
    lines = [
        "# Public C ABI of this tree. Written by scripts/sync.py; do not edit.",
        "# TALIB_LIBRARY_VERSION is derived from this file and %s." % RELEASED,
        "format %d" % FORMAT,
        "soname %s" % soname,
        "version-info %d:%d:%d" % (c, r, a),
        "headers %s" % " ".join(sorted(headers)),
        "function-count %d" % len(funcs),
    ]
    lines += bind_enum_bounds(measure(root, structs, enums, macros, fmacros, typedefs))
    lines += ["callback %s" % c for c in callbacks]
    lines += ["function %s" % f for f in funcs]
    # Named, not dropped: a macro this cannot classify is one the gate does not
    # protect, and saying so is the difference between a limit and a hole.
    lines += ["unmeasured-macro %s" % m for m in skipped]
    return "\n".join(lines) + "\n"


SURFACE = ("type ", "enum ", "macro ", "callback ", "typedef ", "function ")


def _surface(manifest: str) -> set:
    return {l for l in manifest.splitlines() if l.startswith(SURFACE)}


def _triple(manifest: str) -> tuple:
    m = re.search(r"^version-info (\d+):(\d+):(\d+)$", manifest, re.M)
    return tuple(int(x) for x in m.groups()) if m else None


def _platform(manifest: str) -> str:
    m = re.search(r"^platform .*$", manifest, re.M)
    return m.group(0) if m else ""


def _format(manifest: str) -> int:
    m = re.search(r"^format (\d+)$", manifest, re.M)
    return int(m.group(1)) if m else None


def _release_of(released: str) -> tuple:
    m = re.search(r"^release (v\d+\.\d+\.\d+) ([0-9a-f]{40})$", released, re.M)
    if not m:
        sys.exit("abi: %s has no `release <tag> <sha>` line. Delete it and run "
                 "scripts/sync.py." % RELEASED)
    return m.group(1), m.group(2)


def _vtuple(version: str) -> tuple:
    return tuple(int(x) for x in version.lstrip("v").split("."))


def _read(root: str, name: str) -> str:
    path = os.path.join(root, name)
    return open(path).read() if os.path.exists(path) else None


def _git(root: str, *args) -> subprocess.CompletedProcess:
    try:
        return subprocess.run(["git", "-C", root] + list(args), capture_output=True, text=True)
    except FileNotFoundError:
        return subprocess.CompletedProcess(args, 127, "", "git is not on PATH")


def compute_triple(released: str, manifest: str) -> tuple:
    """(what, (c, r, a), lost lines) that `manifest` requires, measured from the
    last published release, never from the previous commit."""
    if _format(released) != _format(manifest) or _platform(released) != _platform(manifest):
        sys.exit("abi: %s and %s differ in format or platform. Run "
                 "'scripts/sync.py'." % (RELEASED, MANIFEST))
    c, r, a = _triple(released)
    base, now = _surface(released), _surface(manifest)
    lost = sorted(base - now)
    if lost:
        # From `current`, not from the soname: hosts that version on `current`
        # alone must never reuse a soname across a break.
        return "break", (c + 1, 0, 0), lost
    if now != base:
        return "added", (c + 1, 0, a + 1), []
    return "unchanged", (c, r, a), []


def _soname(manifest: str) -> str:
    m = re.search(r"^soname (\S+)$", manifest or "", re.M)
    return m.group(1) if m else None


def _atoms(surface: set) -> set:
    """A `type` line split per field, so a second change to a struct whose layout
    already changed this cycle is still seen."""
    out = set()
    for line in surface:
        if line.startswith("type "):
            head = line.split()[:3]
            out.add(" ".join(head))
            out.update("%s %s" % (" ".join(head[:2]), f) for f in line.split()[3:])
        else:
            out.add(line)
    return out


def _needs_accept_break(released: str, committed: str, fresh: str) -> list:
    """Released API that `fresh` loses and the committed manifest has not already
    accepted. Accepted means the committed triple is this release's break triple."""
    what, want, lost = compute_triple(released, fresh)
    if what != "break":
        return []
    if committed is None or _triple(committed) != want:
        return lost
    if _format(committed) == _format(fresh) and _platform(committed) == _platform(fresh):
        # A further removal in a cycle that already accepted one.
        return sorted(_atoms(_surface(committed)) - _atoms(_surface(fresh))
                      & _atoms(_surface(released)))
    return lost


def write_triple(root: str, manifest: str, triple: tuple) -> str:
    """Writes the triple into configure.ac; returns `manifest` carrying it."""
    path = os.path.join(root, "configure.ac")
    text = open(path).read()
    new, n = re.subn(r"^TALIB_LIBRARY_VERSION=\d+:\d+:\d+$",
                     "TALIB_LIBRARY_VERSION=%d:%d:%d" % triple, text, flags=re.M)
    if n != 1:
        sys.exit("abi: configure.ac must hold exactly one TALIB_LIBRARY_VERSION=c:r:a "
                 "line, found %d. After a merge conflict, keep either side, commit the merge, "
                 "then run 'scripts/sync.py'; it rewrites the value." % n)
    if new != text:
        open(path, "w").write(new)
    manifest = re.sub(r"^soname .*$", "soname libta-lib.so.%d" % (triple[0] - triple[2]),
                      manifest, count=1, flags=re.M)
    return re.sub(r"^version-info .*$", "version-info %d:%d:%d" % triple,
                  manifest, count=1, flags=re.M)


def measure_release(root: str, tag: str, sha: str) -> str:
    """The release's surface as THIS script measures it, so a FORMAT change never
    compares two formats."""
    import io
    import tarfile
    archive = subprocess.run(["git", "-C", root, "archive", "--format=tar", sha,
                              "configure.ac", "CMakeLists.txt", "include", "src"],
                             capture_output=True)
    if archive.returncode != 0:
        sys.exit("abi: cannot read %s (%s) from git: %s. Run: git fetch %s tag %s"
                 % (tag, sha, archive.stderr.decode().strip(), CANONICAL_REMOTE, tag))
    with tempfile.TemporaryDirectory() as d:
        with tarfile.open(fileobj=io.BytesIO(archive.stdout)) as tar:
            if hasattr(tarfile, "data_filter"):
                tar.extractall(d, filter="data")
            else:
                tar.extractall(d)
        body = [l for l in build_manifest(d).splitlines() if not l.startswith("#")]
    return "\n".join(["# Public C ABI of the last published release. Written by "
                      "scripts/sync.py; do not edit.",
                      "release %s %s" % (tag, sha)] + body) + "\n"


def advance_released(root: str, published: str, remote: str = CANONICAL_REMOTE) -> str:
    """Records `published` as the baseline once it is out. Returns a message, or
    None when there was nothing to do. Never raises: pre-release-checks catches a
    baseline that failed to move."""
    try:
        if published is None:
            return "Warning: ABI baseline NOT synced: the latest published release is unknown."
        current = _read(root, RELEASED)
        if current is not None and _vtuple(_release_of(current)[0]) >= _vtuple(published):
            return None
        tag = "v" + published
        # Forced: a local tag left by a draft release must not be measured.
        fetch = _git(root, "fetch", "--no-tags", remote, "+refs/tags/%s:refs/tags/%s" % (tag, tag))
        if fetch.returncode != 0:
            return "Warning: ABI baseline NOT synced: cannot fetch %s: %s" % (tag, fetch.stderr.strip())
        sha = _git(root, "rev-parse", tag + "^{commit}").stdout.strip()
        # Without the release's own commits its additions would read as removals.
        if _git(root, "merge-base", "--is-ancestor", sha, "HEAD").returncode != 0:
            return ("Warning: ABI baseline NOT synced: this branch does not contain %s. "
                    "Merge dev and re-run scripts/sync.py." % tag)
        released = measure_release(root, tag, sha)
        if _platform(released) != CI_PLATFORM:
            return _foreign_host(_platform(released))
        open(os.path.join(root, RELEASED), "w").write(released)
        return "Recorded %s as the ABI baseline (%s)." % (tag, RELEASED)
    except (Exception, SystemExit) as e:
        return "Warning: ABI baseline NOT synced: %s" % e


def _lost_message(lost: list) -> str:
    shown = "\n".join("  - " + l for l in lost[:20])
    more = "\n  ... %d more" % (len(lost) - 20) if len(lost) > 20 else ""
    return shown + more


def _break_refusal(tag: str, lost: list, command: str) -> str:
    return ("abi: this removes or changes public C API that shipped in %s:\n%s\n"
            "If that is not intended, put the header back. If it is, run '%s': the "
            "soname then changes at the next release." % (tag, _lost_message(lost), command))


def _verify_released(root: str, released: str, required: bool) -> None:
    tag, sha = _release_of(released)
    tagged = _git(root, "rev-parse", "-q", "--verify", tag + "^{commit}").stdout.strip()
    if tagged and tagged != sha:
        sys.exit("abi: %s records %s at %s, but the tag is %s here. Refresh the tag (git fetch "
                 "--force %s tag %s); if it still differs, restore %s from dev (git checkout "
                 "origin/dev -- %s)." % (RELEASED, tag, sha, tagged, CANONICAL_REMOTE, tag,
                                         RELEASED, RELEASED))
    if _git(root, "cat-file", "-e", sha + "^{commit}").returncode != 0:
        if required:
            sys.exit("abi: %s (%s) is not in this clone. Run: git fetch %s tag %s"
                     % (tag, sha, CANONICAL_REMOTE, tag))
        return
    if measure_release(root, tag, sha) != released:
        sys.exit("abi: %s is not a measurement of %s. It is written by scripts/sync.py "
                 "only: restore it from dev (git checkout origin/dev -- %s). If "
                 "scripts/abi.py now measures differently, bump its FORMAT and run "
                 "'scripts/sync.py'." % (RELEASED, tag, RELEASED))


def _foreign_host(here: str) -> str:
    return ("Warning: shared library version NOT synced: this host measures '%s'; the ABI "
            "files are measured on Linux x86_64. Run scripts/sync.py there." % here)


def _platform_exit(name: str, recorded: str, here: str) -> None:
    if here == CI_PLATFORM:
        sys.exit("abi: %s was measured on '%s', not where CI measures. Run 'scripts/sync.py' "
                 "to re-measure it." % (name, recorded))
    sys.exit("abi: this host measures '%s'; the ABI files are measured on Linux x86_64. "
             "Run abi.py and scripts/sync.py there." % here)


def check(root: str, require_artifact: bool) -> int:
    released = _read(root, RELEASED)
    if released is None:
        sys.exit("abi: %s is missing. Run 'scripts/sync.py'; it records the latest "
                 "published release (needs network, gcc and git)." % RELEASED)
    committed = _read(root, MANIFEST)
    if committed is None:
        sys.exit("abi: %s is missing. Run 'scripts/sync.py'." % MANIFEST)
    fresh = build_manifest(root)
    for name, text in ((MANIFEST, committed), (RELEASED, released)):
        if _format(text) != FORMAT:
            sys.exit("abi: %s records format %s, this script writes format %d. Run "
                     "'scripts/sync.py'." % (name, _format(text), FORMAT))
        if _platform(text) != _platform(fresh):
            _platform_exit(name, _platform(text), _platform(fresh))
    _verify_released(root, released, require_artifact and os.environ.get("GITHUB_ACTIONS") == "true")
    tag = _release_of(released)[0]

    if committed != fresh:
        import difflib
        diff = list(difflib.unified_diff(committed.splitlines(), fresh.splitlines(),
                                         MANIFEST, "built", lineterm=""))
        print("\n".join(diff[:120]))
        if len(diff) > 120:
            print("... %d more line(s)" % (len(diff) - 120))
        lost = _needs_accept_break(released, committed, fresh)
        if lost:
            sys.exit("\n" + _break_refusal(tag, lost, "scripts/sync.py --accept-break"))
        sys.exit("\nabi: %s is out of date. Run 'scripts/sync.py' and commit what it "
                 "wrote." % MANIFEST)

    what, want, lost = compute_triple(released, committed)
    if _triple(committed) != want:
        unaccepted = _needs_accept_break(released, committed, committed)
        command = "scripts/sync.py" + (" --accept-break" if unaccepted else "")
        if unaccepted:
            sys.exit(_break_refusal(tag, unaccepted, command))
        sys.exit("abi: configure.ac says %d:%d:%d, but %s (%s) and %s require %d:%d:%d. "
                 "Run '%s'. On a pull request, merge the current dev first: CI checks "
                 "the merge result." % (_triple(committed) + (RELEASED, tag, MANIFEST) + want
                                         + (command,)))

    soname = "libta-lib.so.%d" % (want[0] - want[2])
    built = check_built_soname(root, soname, require_artifact)
    exports = check_export_table(root, committed, require_artifact)
    print("abi: public ABI matches %s (soname %s, %s; %s)."
          % (MANIFEST, soname, built, exports))
    if what == "break":
        print("abi: BREAKS the %s ABI (soname %s -> %s): %d line(s) removed or changed."
              % (tag, _soname(released), soname, len(lost)))
        if os.environ.get("GITHUB_ACTIONS") == "true":
            print("::warning file=%s::Breaks the %s C ABI (%s -> %s): %s"
                  % (MANIFEST, tag, _soname(released), soname, "; ".join(lost[:8])))
    return 0


def sync(root: str, accept_break: bool = False) -> tuple:
    """(ok, message): rewrites ABI.manifest and TALIB_LIBRARY_VERSION from the headers,
    re-measuring ABI.released after a FORMAT change. Idempotent; writes only what
    changed. ok is False when shipped API is removed without accept_break, or when
    the tree itself is inconsistent; an unusable host is only a warning."""
    import shutil
    released = _read(root, RELEASED)
    if released is None:
        return True, ("Warning: shared library version NOT synced: %s is missing and the "
                      "latest published release could not be recorded (see the warning "
                      "above); re-run scripts/sync.py." % RELEASED)
    if shutil.which("gcc") is None:
        return True, ("Warning: shared library version NOT synced: gcc is not on PATH; the "
                      "ABI files are measured with gcc on Linux x86_64. Run scripts/sync.py there.")
    try:
        committed = _read(root, MANIFEST)
        fresh = build_manifest(root)
        if _platform(fresh) != CI_PLATFORM:
            return True, _foreign_host(_platform(fresh))
        stale = _format(released) != FORMAT or _platform(released) != CI_PLATFORM
        if stale:
            released = measure_release(root, *_release_of(released))
        tag = _release_of(released)[0]
        lost = _needs_accept_break(released, committed, fresh)
        if lost and not accept_break:
            return False, (_break_refusal(tag, lost, "scripts/sync.py --accept-break")
                           + "\nABI.manifest and TALIB_LIBRARY_VERSION were NOT updated.")
        what, want, _ = compute_triple(released, fresh)
        written = []
        before = open(os.path.join(root, "configure.ac")).read()
        manifest = write_triple(root, fresh, want)
        if open(os.path.join(root, "configure.ac")).read() != before:
            written.append("configure.ac")
        if stale:
            open(os.path.join(root, RELEASED), "w").write(released)
            written.append(RELEASED)
        if manifest != committed:
            open(os.path.join(root, MANIFEST), "w").write(manifest)
            written.append(MANIFEST)
        if not written:
            return True, None
        change = {"unchanged": "no API change", "added": "API added",
                  "break": "API removed or changed"}[what]
        msg = ("Updated %s: TALIB_LIBRARY_VERSION=%d:%d:%d, soname libta-lib.so.%d (%s since "
               "%s)." % ((", ".join(written),) + want + (want[0] - want[2], change, tag)))
        if lost:
            msg += " Accepted removal or change of:\n" + _lost_message(lost)
        return True, msg
    except SystemExit as e:
        return False, "abi: shared library version NOT synced: %s" % str(e).replace("abi: ", "", 1)


def release_gate(root: str, version: str, published: str) -> list:
    """Errors that block releasing `version`; empty when the ABI baseline is sound."""
    if published is None:
        return ["could not look up the latest published release, so the ABI baseline "
                "cannot be verified. Re-run once GitHub is reachable."]
    released = _read(root, RELEASED)
    if released is None:
        return ["%s is missing. Run scripts/sync.py on dev." % RELEASED]
    try:
        tag, sha = _release_of(released)
        if _vtuple(tag) < _vtuple(published):
            return ["%s records %s but v%s is the latest published release. Run "
                    "scripts/sync.py on dev, commit, and merge to main." % (RELEASED, tag, published)]
        if _vtuple(tag) > _vtuple(published):
            return ["%s records %s, newer than the latest published release v%s. If %s is "
                    "published, mark it Latest on GitHub; if it never shipped, delete %s and "
                    "run scripts/sync.py on dev." % (RELEASED, tag, published, tag, RELEASED)]
        if _vtuple(tag) >= _vtuple(version):
            return ["VERSION %s is not above the last published release %s. Bump VERSION and "
                    "run scripts/sync.py." % (version, tag)]
        actual = _git(root, "rev-parse", tag + "^{commit}").stdout.strip()
        if actual != sha:
            return ["tag %s is %s here but %s records %s. Refresh the tag (git fetch --force "
                    "%s tag %s)." % (tag, actual or "missing", RELEASED, sha, CANONICAL_REMOTE, tag)]
        check(root, False)
    except SystemExit as e:
        return [str(e)]
    return []


def _built_library(root: str) -> str:
    """The file CMake's unversioned dev link resolves to. Not a glob (`1.10.0` sorts
    before `1.9.0`) and not the expected soname's link, which a build stamping the
    wrong soname never creates."""
    cache = os.path.join(root, "cmake-build", "CMakeCache.txt")
    if os.path.exists(cache) and "BUILD_SHARED_LIBS:BOOL=OFF" in open(cache).read():
        return None
    link = os.path.join(root, "cmake-build", "libta-lib.so")
    return os.path.realpath(link) if os.path.exists(link) else None


def check_built_soname(root: str, want: str, required: bool = False) -> str:
    """The manifest and CMake both DERIVE the soname from configure.ac, so
    neither notices a target property reverted to `SOVERSION ${PROJECT_VERSION}`.
    Only the artifact can say what was actually stamped."""
    lib = _built_library(root)
    if lib is None:
        if required:
            sys.exit("abi: --require-artifact was given but cmake-build/ holds no "
                     "shared library. Only the artifact can show a reverted SOVERSION, so "
                     "this is a gate that did not run, not a pass.")
        return "ARTIFACT UNCHECKED: no built library"
    try:
        got = subprocess.run(["objdump", "-p", lib], capture_output=True,
                             text=True).stdout
    except FileNotFoundError:
        if required:
            sys.exit("abi: --require-artifact was given but objdump is not on PATH")
        return "ARTIFACT UNCHECKED: no objdump"
    m = re.search(r"SONAME\s+(\S+)", got)
    if not m:
        return "ARTIFACT UNCHECKED: built library has no SONAME"
    if m.group(1) != want:
        sys.exit("abi: %s has DT_SONAME %s but the manifest says %s -- rebuild "
                 "(scripts/build.py); if it persists, the build no longer stamps what "
                 "configure.ac declares"
                 % (os.path.basename(lib), m.group(1), want))
    return "%s stamped it too" % os.path.basename(lib)


# Exported although no installed header declares it. Each name needs a reason
# that is about the SHIPPED library, not about convenience: see
# src/ta_common/ta_global.h. A name added here widens the ABI without widening
# any header, which is exactly the drift this file exists to make deliberate.
EXPORTED_BUT_UNDECLARED = {
    "TA_Globals": "ta_regtest pokes it; the autotools tools link the .so (Homebrew, Debian)",
}


def _manifest_function_names(manifest: str) -> set:
    """The declared function names, from the manifest's own `function ` lines."""
    out = set()
    for line in manifest.splitlines():
        if not line.startswith("function "):
            continue
        head = line.split("(", 1)[0]
        name = head.rsplit(None, 1)[-1].lstrip("*") if head.split() else ""
        if name:
            out.add(name)
    return out


def check_export_table(root: str, manifest: str, required: bool = False) -> str:
    """The built library exports the declared set, and nothing else.

    Catches what neither the header parse nor the soname check can: the
    visibility flag or the linker map failing to apply (the surface silently
    widens), and a declared function absent from the library (it silently
    narrows -- `--no-undefined-version` only catches a map naming a symbol that
    does not exist, never a name the map forgot).
    """
    want = re.search(r"^soname (\S+)$", manifest, re.M)
    if not want:
        return "EXPORTS UNCHECKED: manifest has no soname line"
    # ELF only, deliberately: `nm -D` and this name shape are GNU. A macOS build
    # produces libta-lib.<major>.dylib and needs `nm -gU`, so the export set is
    # UNMEASURED there rather than measured-and-equal -- say so, because
    # "UNCHECKED" reads like "no library was built" and on macOS one was.
    lib = _built_library(root)
    if lib is None:
        if required:
            sys.exit("abi: --require-artifact was given but cmake-build/ holds no "
                     "ELF shared library, so the export table was never read. That is a "
                     "gate that did not run, not a pass. (Mach-O and PE are not covered "
                     "here at all -- do not pass --require-artifact on those.)")
        return "EXPORTS UNCHECKED: no ELF shared library (Mach-O/PE not covered)"
    try:
        out = subprocess.run(["nm", "-D", "--defined-only", lib],
                             capture_output=True, text=True)
    except FileNotFoundError:
        if required:
            sys.exit("abi: --require-artifact was given but nm is not on PATH")
        return "EXPORTS UNCHECKED: no nm"
    if out.returncode != 0:
        if required:
            sys.exit("abi: nm failed on %s: %s"
                     % (os.path.basename(lib), out.stderr.strip()))
        return "EXPORTS UNCHECKED: nm failed"

    exported = {ln.split()[-1] for ln in out.stdout.splitlines() if len(ln.split()) >= 3}
    if not exported:
        sys.exit("abi: nm read 0 symbols from %s -- the parse moved, and an empty "
                 "set would compare clean against nothing"
                 % os.path.basename(lib))
    declared = _manifest_function_names(manifest)
    # Non-vacuity: the names parsed here must be the count the manifest states,
    # so a regex that quietly stopped matching cannot read as agreement.
    stated = re.search(r"^function-count (\d+)$", manifest, re.M)
    if stated and len(declared) != int(stated.group(1)):
        sys.exit("abi: parsed %d function name(s) but the manifest states "
                 "function-count %s -- the parse moved"
                 % (len(declared), stated.group(1)))

    allowed = declared | set(EXPORTED_BUT_UNDECLARED)
    extra = sorted(exported - allowed)
    missing = sorted(allowed - exported)
    if extra or missing:
        msg = ["abi: the shipped export table is not the declared surface (%s)."
               % os.path.basename(lib)]
        if extra:
            msg.append("  %d symbol(s) exported that no header declares, e.g. %s"
                       % (len(extra), ", ".join(extra[:8])))
            msg.append("  Either -fvisibility=hidden or src/libta-lib.map did not apply,")
            msg.append("  or something new needs TA_LIB_API dropped from it.")
        if missing:
            msg.append("  %d declared symbol(s) NOT exported, e.g. %s"
                       % (len(missing), ", ".join(missing[:8])))
            msg.append("  A caller that links these gets an undefined reference.")
        sys.exit("\n".join(msg))
    return "%d exported, all declared" % len(exported)


def main() -> int:
    import argparse
    parser = argparse.ArgumentParser(
        prog="scripts/abi.py",
        description="Verifies the public C ABI and the shared library version. "
                    "scripts/sync.py is what updates them.")
    sub = parser.add_subparsers(dest="command", required=True)
    chk = sub.add_parser("check", help="verify, writing nothing (what CI runs)")
    chk.add_argument("--require-artifact", action="store_true",
                     help="fail unless a built cmake-build/ shared library is there to read")
    args = parser.parse_args()
    return check(repo_root(), args.require_artifact)


if __name__ == "__main__":
    sys.exit(main())
