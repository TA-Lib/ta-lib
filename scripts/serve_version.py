"""Shared helpers for building a "serve of another version" — a JSON-RPC server
whose TRANSPORT is the current tree's generated `ta_codegen_serve.c` but whose
ORACLE is a frozen older libta-lib.a (a pinned worktree: ../ta-lib-064 @ v0.6.4,
../ta-lib-ref @ reference-pre-cutover, ...).

The current transport enumerates every CURRENT function (list_functions) and its
frame dispatch references every CURRENT indicator symbol (TA_<NAME>, TA_S_<NAME>,
TA_<NAME>_Lookback). A frozen library predates functions added since — so those
symbols are undefined at link, and advertising them in list_functions would make
a differential gate try (and fail) to diff a function the frozen version cannot
compute.

Both problems are handled the SAME way for every version serve (this is the
--fuzz-064 subset mechanism, generalized):

  * post_version_funcs(): functions the current tree has but the frozen version
    does not, from a plain ta_func_list.txt diff.
  * filter_list_functions(): drop those from the serve's list_functions payload,
    so a driver's subset gate (strstr on list_functions) skips them.
  * stub_definitions(): define the missing symbols with a `{ return TA_BAD_PARAM; }`
    body reusing their EXACT ta_func.h prototype, injected into the serve TU, so
    the serve links. Pure C (no linker-specific flag) => works on gcc/clang and
    GNU ld / lld / Apple ld64 alike. A skipped function is never dispatched, so the
    stub is never actually called; if it were, it returns an error, not a crash.
"""

import os
import re


def _func_names(root):
    """The set of function names in a tree's ta_func_list.txt (first column)."""
    path = os.path.join(root, "ta_func_list.txt")
    names = set()
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line:
                names.add(line.split()[0])
    return names


def post_version_funcs(current_root, version_root):
    """Sorted names present in current_root but not in version_root.

    Empty when the frozen version already has every current function (the
    historical case, before any post-freeze function existed)."""
    return sorted(_func_names(current_root) - _func_names(version_root))


def filter_list_functions(src_text, post_funcs):
    """Remove post_funcs' entries from the serve's list_functions payload.

    Each entry is a line like
        pos += snprintf(resp + pos, resp_size - pos, ",\\"TA_PVO\\"");
    The escaped-quote token \\"TA_<NAME>\\" appears ONLY in list_functions (the
    dispatch chain uses the bare "TA_<NAME>"), so removing the whole line is
    safe. Every non-first entry carries its own leading comma, so dropping a
    middle/last entry keeps the JSON array valid. (A post-version function is a
    new ADDITION, so it is never the alphabetically-first entry — currently
    ACCBANDS — whose line has no leading comma; dropping that one would leave the
    next entry's stray comma. Asserted below rather than silently mis-emitted.)"""
    for name in post_funcs:
        assert name > "ACCBANDS", (
            "filter_list_functions: %s sorts before the first list_functions "
            "entry; add leading-comma repair before removing it." % name)
        pattern = r'[^\n]*\\"TA_' + re.escape(name) + r'\\"[^\n]*\n'
        src_text, n = re.subn(pattern, '', src_text)
        assert n == 1, "filter_list_functions: expected 1 %s entry, removed %d" % (name, n)
    return src_text


def stub_definitions(post_funcs, header_path):
    """Portable C stub definitions for the indicator symbols each post-version
    function contributes and the current transport references: TA_<NAME>,
    TA_S_<NAME>, TA_<NAME>_Lookback (whichever ta_func.h declares).

    Each stub reuses the EXACT prototype from ta_func.h with a
    `{ return TA_BAD_PARAM; }` body, so it is a valid definition in the serve TU
    (which already `#include`s ta_func.h) — no cross-TU type mismatch and no
    linker-specific flag (unlike -Wl,--defsym, which Apple's ld64 does not
    implement). TA_BAD_PARAM is a small int, fine for the int-returning
    *_Lookback too. Never reached at runtime: the subset gate skips these
    functions before dispatch."""
    if not post_funcs:
        return ""
    with open(header_path) as f:
        header = f.read()
    defs = []
    for name in post_funcs:
        for sym in ("TA_%s" % name, "TA_S_%s" % name,
                    "TA_%s_Lookback" % name, "TA_S_%s_Lookback" % name):
            # `<sym>\s*\(` (paren-anchored) matches only <sym>'s own declaration,
            # never a longer name that has <sym> as a prefix (e.g. TA_PVO vs
            # TA_PVO_Lookback). A symbol ta_func.h does not declare is not
            # referenced by the transport, so no stub is needed for it.
            m = re.search(r'(TA_LIB_API\s+[\w ]+?' + re.escape(sym) +
                          r'\s*\([^;]*\))\s*;', header)
            if m:
                defs.append(m.group(1).strip() + " { return TA_BAD_PARAM; }")
    if not defs:
        return ""
    return ("\n/* [serve-of-another-version] portable stub definitions for symbols\n"
            " * absent from the frozen library; never dispatched (the subset gate\n"
            " * skips these functions). Exact ta_func.h signatures => valid C, no\n"
            " * linker flag, no cross-TU type mismatch. */\n" + "\n".join(defs) + "\n")


def header_path(include_dirs):
    """Locate ta_func.h among a serve build's -I directories."""
    for d in include_dirs:
        cand = os.path.join(d, "ta_func.h")
        if os.path.exists(cand):
            return cand
    return None
