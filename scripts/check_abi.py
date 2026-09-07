#!/usr/bin/env python3
"""The public C ABI, recorded and diffed, so the SONAME is a checked promise.

A SONAME is the only thing telling a loader whether a library can stand in for
the one a binary was built against, and nothing enforces it: `ld.so` matches the
string and loads. Get it wrong and a removed symbol is a loud startup failure,
but a reordered struct field or a renumbered enum resolves fine and then reads
the wrong bytes. TA-Lib has shipped exactly that -- 0.8.1 removed 162 public
entry points and dropped `TA_FuncInfo.camelCaseName`, moving every field after
`hint` eight bytes earlier, and neither the build nor any test noticed.

So this records the surface in `ABI.manifest` and fails when the build no longer
matches it. Then an ABI change cannot be accidental: you either put it back, or
you regenerate the manifest, and regenerating makes you edit
`TALIB_LIBRARY_VERSION` in the same breath because the manifest names the
SONAME that triple derives.

MEASURED, not parsed, wherever the answer is a number. Sizes, field offsets and
enumerator values come from compiling a probe against the installed headers and
running it, so padding, a widened `int`, an implicit enumerator and a reordered
field are all visible. Parsing would see the text and miss all four.

The surface is what the INSTALLED headers declare -- CMake's `LIB_HEADERS`, read
from CMakeLists.txt rather than listed here -- and specifically the
`TA_LIB_API`-marked functions. Not the `.so`'s symbol table: nothing passes
`-fvisibility=hidden`, so every non-static symbol lands in `.dynsym` and gating
that would fail on internal churn nobody promised anything about.
"""

import os
import re
import subprocess
import sys
import tempfile

MANIFEST = "ABI.manifest"
# Bump when the manifest's SHAPE or its COVERAGE changes -- widening what is
# measured adds surface lines, which is not an ABI change either. A format change rewrites every line,
# which the surface comparison would otherwise read as "the ABI removed things"
# and answer with an instruction to bump the soname for nothing.
FORMAT = 6


def repo_root() -> str:
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def installed_headers(root: str) -> list:
    """The headers CMake installs -- the definition of "public"."""
    text = open(os.path.join(root, "CMakeLists.txt")).read()
    at = text.index("set(LIB_HEADERS")
    block = text[at:text.index(")", at)]
    names = re.findall(r"/include/([A-Za-z0-9_.]+\.h)", block)
    if len(names) < 3:
        sys.exit("check_abi: LIB_HEADERS parsed to %r -- the parse moved" % names)
    return names


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//[^\n]*", " ", text)


def abi_triple(root: str) -> tuple:
    """current:revision:age, and the SONAME libtool derives (current - age)."""
    m = re.search(r"^TALIB_LIBRARY_VERSION=(\d+):(\d+):(\d+)$",
                  open(os.path.join(root, "configure.ac")).read(), re.M)
    if not m:
        sys.exit("check_abi: no TALIB_LIBRARY_VERSION=c:r:a in configure.ac")
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


RELEASED_SONAME = "libta-lib.so.0"      # what v0.7.1 shipped, from autotools


def released_soname(root: str) -> str:
    """Carried forward from the committed manifest, seeded at what shipped."""
    path = os.path.join(root, MANIFEST)
    if os.path.exists(path):
        m = re.search(r"^released-soname (\S+)$", open(path).read(), re.M)
        if m:
            return m.group(1)
    return RELEASED_SONAME


def public_functions(headers_text: str) -> list:
    """Every TA_LIB_API declaration as return type + parameter TYPES, sorted."""
    out = set()
    for m in re.finditer(r"TA_LIB_API\s+([^;{]+?)\(([^;{]*?)\)\s*;", headers_text, re.S):
        head = " ".join(m.group(1).split())
        args = ", ".join(_drop_param_name(a) for a in m.group(2).split(","))
        out.add("%s(%s)" % (head, args))
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


def measure(root: str, structs: dict, enums: dict, macros: list, fmacros: list) -> list:
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
    src.append("return 0;}")
    with tempfile.TemporaryDirectory() as d:
        c, exe = os.path.join(d, "p.c"), os.path.join(d, "p")
        open(c, "w").write("\n".join(src) + "\n")
        try:
            cc = subprocess.run(["gcc", "-I", os.path.join(root, "include"), "-o", exe, c],
                                capture_output=True, text=True)
        except FileNotFoundError:
            sys.exit("check_abi: needs a C compiler -- sizes and offsets are measured, "
                     "not parsed, so `gcc` has to be on PATH")
        if cc.returncode != 0:
            sys.exit("check_abi: the probe did not compile -- a public type is no "
                     "longer spellable from the installed headers:\n" + cc.stderr[:2000])
        run = subprocess.run([exe], capture_output=True, text=True)
        lines = run.stdout.splitlines()
        # A probe that died mid-run would otherwise hand back a TRUNCATED
        # surface, and `--update` would write that as the baseline.
        expected = 1 + len(structs) + len(enums) + sum(len(v) for v in enums.values()) \
                   + len(macros) + len(fmacros)
        if run.returncode != 0 or len(lines) != expected:
            sys.exit("check_abi: the probe exited %d with %d of %d expected line(s) -- "
                     "the measurement is incomplete, not a clean ABI"
                     % (run.returncode, len(lines), expected))
        return lines


def build_manifest(root: str) -> str:
    headers = installed_headers(root)
    text = strip_comments("\n".join(
        open(os.path.join(root, "include", h)).read() for h in headers))
    c, r, a, soname = abi_triple(root)
    structs, enums = public_types(text)
    funcs = public_functions(text)
    macros, fmacros, skipped = public_macros(text)
    callbacks = public_callbacks(text)
    if len(funcs) < 2000 or "TA_FuncInfo" not in structs or "TA_RetCode" not in enums:
        sys.exit("check_abi: surface looks wrong (%d functions, %d structs, %d enums) "
                 "-- the header parse moved" % (len(funcs), len(structs), len(enums)))
    lines = [
        "# TA-Lib public C ABI. Regenerate with: scripts/build.py check-abi --update",
        "# A diff here means the ABI moved. Put it back, or bump",
        "# TALIB_LIBRARY_VERSION in configure.ac -- see the rules beside it.",
        "format %d" % FORMAT,
        "soname %s" % soname,
        # What the last RELEASE shipped. libtool's rules are per release, so a
        # second ABI-breaking commit in one cycle must not demand a third soname
        # for a break nobody ever shipped -- the requirement is measured from
        # here, not from the previous commit. Bump it when a release goes out.
        "released-soname %s" % released_soname(root),
        "version-info %d:%d:%d" % (c, r, a),
        "headers %s" % " ".join(sorted(headers)),
        "function-count %d" % len(funcs),
    ]
    lines += measure(root, structs, enums, macros, fmacros)
    lines += ["callback %s" % c for c in callbacks]
    lines += ["function %s" % f for f in funcs]
    # Named, not dropped: a macro this cannot classify is one the gate does not
    # protect, and saying so is the difference between a limit and a hole.
    lines += ["unmeasured-macro %s" % m for m in skipped]
    return "\n".join(lines) + "\n"


SURFACE = ("type ", "enum ", "macro ", "callback ", "function ")


def _surface(manifest: str) -> set:
    return {l for l in manifest.splitlines() if l.startswith(SURFACE)}


def _triple(manifest: str) -> tuple:
    m = re.search(r"^version-info (\d+):(\d+):(\d+)$", manifest, re.M)
    return tuple(int(x) for x in m.groups()) if m else None


def _platform(manifest: str) -> str:
    m = re.search(r"^platform .*$", manifest, re.M)
    return m.group(0) if m else ""


def _released_major(committed: str) -> int:
    m = re.search(r"^released-soname \S*\.(\d+)$", committed, re.M)
    return int(m.group(1)) if m else 0


def required_bump(committed: str, fresh: str) -> tuple:
    """(what changed, the c:r:a the change demands) per libtool's own rules.

    A REMOVED surface line is the whole test for "changed": a struct whose layout
    moved, a renumbered enumerator and a deleted function all drop their old line,
    so they land together in the arm that must increment the soname.
    """
    old_s, new_s = _surface(committed), _surface(fresh)
    prev = _triple(committed)
    if prev is None:
        sys.exit("check_abi: %s has no `version-info` line -- it is malformed; "
                 "regenerate it." % MANIFEST)
    c, r, a = prev
    if old_s == new_s:
        return ("nothing", None)
    if old_s - new_s:
        # The NEXT soname after the released one, not after `current`. Additions
        # earlier in the cycle raise `current` without moving the soname, and
        # basing the break on `current` would skip the sonames they inflated past.
        return ("removed or changed", (_released_major(committed) + 1, 0, 0))
    return ("added", (c + 1, 0, a + 1))                       # soname unchanged


def _soname_of(triple: tuple) -> int:
    return triple[0] - triple[2]


def acceptable(committed: str, have: tuple, what: str) -> bool:
    """Is this triple a correct answer to `what`?

    Judged on the SONAME the triple derives, not on the components: two
    additions leave `current` and `age` both high while the soname has not
    moved, so a component-wise comparison would accept an unbumped removal.
    """
    released, prev = _released_major(committed), _triple(committed)
    if what == "removed or changed":
        return _soname_of(have) > released         # measured from the last RELEASE
    return (_soname_of(have) == _soname_of(prev)   # an addition must not move it
            and have[0] > prev[0] and have[2] > prev[2])


def check_built_soname(root: str, want: str, required: bool = False) -> str:
    """The manifest and CMake both DERIVE the soname from configure.ac, so
    neither notices a target property reverted to `SOVERSION ${PROJECT_VERSION}`.
    Only the artifact can say what was actually stamped."""
    import glob
    libs = sorted(glob.glob(os.path.join(root, "cmake-build", "libta-lib.so.*.*.*")),
                  key=os.path.getmtime)
    if not libs:
        if required:
            sys.exit("check_abi: --require-artifact was given but cmake-build/ holds no "
                     "shared library. Only the artifact can show a reverted SOVERSION, so "
                     "this is a gate that did not run, not a pass.")
        return "ARTIFACT UNCHECKED: no built library"
    try:
        got = subprocess.run(["objdump", "-p", libs[-1]], capture_output=True,
                             text=True).stdout
    except FileNotFoundError:
        if required:
            sys.exit("check_abi: --require-artifact was given but objdump is not on PATH")
        return "ARTIFACT UNCHECKED: no objdump"
    m = re.search(r"SONAME\s+(\S+)", got)
    if not m:
        return "ARTIFACT UNCHECKED: built library has no SONAME"
    if m.group(1) != want:
        sys.exit("check_abi: %s has DT_SONAME %s but the manifest says %s -- the build "
                 "no longer stamps what configure.ac declares"
                 % (os.path.basename(libs[-1]), m.group(1), want))
    return "%s stamped it too" % os.path.basename(libs[-1])


def main() -> int:
    root = repo_root()
    update = "--update" in sys.argv
    path = os.path.join(root, MANIFEST)
    fresh = build_manifest(root)

    if not os.path.exists(path):
        if update:
            open(path, "w").write(fresh)
            print("check_abi: wrote %s" % MANIFEST)
            return 0
        sys.exit("check_abi: %s is missing -- create it with --update" % MANIFEST)
    committed = open(path).read()

    # An offset measured on ILP32 is not the LP64 one. Comparing them reads as an
    # ABI change, and `--update` from the wrong host would write that baseline in.
    # Only when the manifest RECORDS one: a manifest predating this line is stale
    # in format, not measured elsewhere, and blocking it would block the
    # regeneration that adds the line.
    if _platform(committed) and _platform(committed) != _platform(fresh):
        sys.exit("check_abi: %s was measured on a different platform\n  manifest: %s\n"
                 "  here:     %s\nSizes and offsets are host-specific; regenerate on the "
                 "platform the manifest records." % (MANIFEST, _platform(committed),
                                                     _platform(fresh)))

    same_format = re.search(r"^format (\d+)$", committed, re.M)
    same_format = bool(same_format) and int(same_format.group(1)) == FORMAT

    if update:
        if not same_format:
            # Nothing to compare: every line moved because the shape did.
            open(path, "w").write(fresh)
            print("check_abi: wrote %s -- manifest FORMAT changed, so the surface was "
                  "re-baselined rather than diffed. The ABI itself is unverified by this "
                  "run; review the diff." % MANIFEST)
            return 0
        what, need = required_bump(committed, fresh)
        have = _triple(fresh)
        # `>=` component-wise, not `==`: libtool's rules are written per RELEASE,
        # and a second ABI-breaking commit in the same cycle would otherwise
        # demand a third soname for a second break nobody ever shipped.
        if need is not None and not acceptable(committed, have, what):
            sys.exit(
                "check_abi: the public ABI %s, so TALIB_LIBRARY_VERSION must move with it.\n"
                "  configure.ac says %d:%d:%d; this change requires at least %d:%d:%d "
                "(soname libta-lib.so.%d).\n"
                "Set it, then run --update again. Refusing to record an ABI change under a\n"
                "version-info that does not describe it -- that is the whole point of the file."
                % ((what,) + have + need + (need[0] - need[2],)))
        open(path, "w").write(fresh)
        print("check_abi: wrote %s (%s)" % (MANIFEST, what))
        return 0
    if committed == fresh:
        want = re.search(r"^soname (\S+)$", fresh, re.M).group(1)
        built = check_built_soname(root, want, "--require-artifact" in sys.argv)
        print("check_abi: public ABI matches %s (soname %s, %s)." % (MANIFEST, want, built))
        return 0
    import difflib
    diff = list(difflib.unified_diff(committed.splitlines(), fresh.splitlines(),
                                     MANIFEST, "built", lineterm=""))
    print("\n".join(diff[:120]))
    if len(diff) > 120:
        print("... %d more line(s)" % (len(diff) - 120))
    print("\ncheck_abi: the public ABI no longer matches %s.\n"
          "  If this change is intended, it is an ABI change: bump\n"
          "  TALIB_LIBRARY_VERSION in configure.ac by the rules beside it, then\n"
          "  run 'scripts/build.py check-abi --update' and commit the result.\n"
          "  If it is not intended, put the header back." % MANIFEST)
    return 1


if __name__ == "__main__":
    sys.exit(main())
