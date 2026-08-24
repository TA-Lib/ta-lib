#!/usr/bin/env python3
"""Bake the numerical-reference goldens for ta_regtest (issue #251).

Reads the datasets out of src/tools/ta_regtest/ta_test_reference.c, computes
what each shipped function must return on them, and writes

    src/tools/ta_regtest/ta_test_reference_golden.h
    src/tools/ta_regtest/ta_test_reference_golden.c

WHY BAKE INSTEAD OF COMPUTING IN THE TEST BINARY

Two reasons, and the second is the stronger one.

1. A runtime oracle shares the test binary's fate. If it is wrong the
   comparison is co-wrong -- the trap from #228, where a bit-exact differential
   stayed green because both arms had moved together. The values below are
   computed in EXACT RATIONAL ARITHMETIC, in a different language, by a
   different person's library: `fractions.Fraction` over the exact binary
   values of the inputs, so the sums and products carry no error at all. Only
   the square roots leave that arithmetic -- the sigma tables, the correlations
   and NORRIS_R -- and they take one correctly-rounded 50-digit `decimal` root
   of an exactly-known rational, not a chain of roundings. There is no shared
   code with TA-Lib to be co-wrong with.

2. `long double` is not portable: 64 mantissa bits on x86 Linux, 53 on MSVC,
   113 on AArch64. Every pin measured on Linux was therefore weaker on Windows.
   A constant is a constant on every ABI.

The values are printed with repr(), i.e. Python's shortest round-trip form, so
every literal parses back to the exact double this script computed.

The corresponding runtime oracles (ta_test_reference.c) are NOT deleted -- the
LCG-driven sweeps generate far more windows than it would be sensible to bake,
and those keep an oracle. What changed there is that it accumulates in
compensated double-double rather than `long double`, so it too is
ABI-independent; and test_func_reference() in
src/tools/ta_regtest/ta_test_func/test_reference.c -- run it with
`ta_regtest --function=REFERENCE` -- asserts the oracle reproduces every golden
below, which is what keeps the two honest.

Usage:
    scripts/gen_test_reference.py            regenerate the two files
    scripts/gen_test_reference.py --check    fail if regenerating would change them
"""

import argparse
import decimal
import os
import re
import sys
from fractions import Fraction

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, "src", "tools", "ta_regtest", "ta_test_reference.c")
OUT_H = os.path.join(REPO, "src", "tools", "ta_regtest", "ta_test_reference_golden.h")
OUT_C = os.path.join(REPO, "src", "tools", "ta_regtest", "ta_test_reference_golden.c")

BEGIN = "/* --- BEGIN GENERATOR-PARSED DATASETS --- */"
END = "/* --- END GENERATOR-PARSED DATASETS --- */"

decimal.getcontext().prec = 50


# ---------------------------------------------------------------------------
# Parsing the datasets out of the C file. Deliberately narrow: one
# `const double NAME[ANYTHING] = { ... };` per dataset, plain decimal literals
# only. Anything else is an error rather than a guess.
# ---------------------------------------------------------------------------

DEF_RE = re.compile(
    r"const\s+double\s+(\w+)\s*\[[^\]]*\]\s*=\s*\{(.*?)\}\s*;", re.S)
NUM_RE = re.compile(r"^[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?$")


def load_datasets():
    text = open(SRC, encoding="utf-8").read()
    try:
        block = text.split(BEGIN, 1)[1].split(END, 1)[0]
    except IndexError:
        sys.exit("gen_test_reference: dataset markers not found in %s" % SRC)
    # Strip comments so a commented-out value can never be read as data.
    block = re.sub(r"/\*.*?\*/", " ", block, flags=re.S)
    out = {}
    for name, body in DEF_RE.findall(block):
        vals = []
        for tok in body.split(","):
            tok = tok.strip()
            if not tok:
                continue
            if not NUM_RE.match(tok):
                sys.exit("gen_test_reference: %s: unparsable literal %r" % (name, tok))
            vals.append(float(tok))
        out[name] = vals
    if not out:
        sys.exit("gen_test_reference: no datasets parsed from %s" % SRC)
    return out


# ---------------------------------------------------------------------------
# Exact-rational statistics. Every input is a double, so Fraction(v) is its
# exact value and every sum/product below is exact.
# ---------------------------------------------------------------------------

def frac_sqrt(f):
    """Nearest double to sqrt(f) for a non-negative Fraction f."""
    if f <= 0:
        return 0.0
    d = decimal.Decimal(f.numerator) / decimal.Decimal(f.denominator)
    return float(d.sqrt())


def moments(x, y, s, n):
    """Exact (Sxx, Syy, Sxy) of the mean-centred window, times n^2."""
    fx = [Fraction(v) for v in x[s:s + n]]
    fy = [Fraction(v) for v in y[s:s + n]]
    sx = sum(fx)
    sy = sum(fy)
    dx = [n * v - sx for v in fx]
    dy = [n * v - sy for v in fy]
    sxx = sum(a * a for a in dx)
    syy = sum(b * b for b in dy)
    sxy = sum(a * b for a, b in zip(dx, dy))
    return sx, sy, sxx, syy, sxy


def var_window(x, s, n):
    """Population variance, exactly, then rounded once to double."""
    if all(v == x[s] for v in x[s:s + n]):
        return 0.0
    fx = [Fraction(v) for v in x[s:s + n]]
    sx = sum(fx)
    acc = sum((n * v - sx) ** 2 for v in fx)
    return float(acc / (n ** 3))


def sigma_window(x, s, n):
    if all(v == x[s] for v in x[s:s + n]):
        return 0.0
    fx = [Fraction(v) for v in x[s:s + n]]
    sx = sum(fx)
    acc = sum((n * v - sx) ** 2 for v in fx)
    return frac_sqrt(acc / (n ** 3))


def corr_window(x, y, s, n):
    """Pearson r. r^2 is an exact rational, so this is one correctly-rounded
    square root of an exact value -- not a chain of roundings."""
    if all(v == x[s] for v in x[s:s + n]):
        return 0.0
    if all(v == y[s] for v in y[s:s + n]):
        return 0.0
    _, _, sxx, syy, sxy = moments(x, y, s, n)
    if sxx <= 0 or syy <= 0:
        return 0.0
    r2 = (sxy * sxy) / (sxx * syy)
    r = frac_sqrt(r2)
    if sxy < 0:
        r = -r
    return max(-1.0, min(1.0, r))


def linreg_window(y, s, n):
    """Least squares against the bar index 0..n-1, 0 at the oldest bar --
    the line the TA_LINEARREG family fits. Returns (slope, intercept, fit at
    x=n-1, forecast at x=n)."""
    fy = [Fraction(v) for v in y[s:s + n]]
    sy = sum(fy)
    sumx = Fraction(n * (n - 1), 2)
    dx = [Fraction(n * j) - sumx for j in range(n)]
    dy = [n * v - sy for v in fy]
    sxx = sum(a * a for a in dx)
    sxy = sum(a * b for a, b in zip(dx, dy))
    slope = sxy / sxx
    ybar = sy / n
    half = slope * Fraction(n - 1, 2)
    return (float(slope), float(ybar - half), float(ybar + half),
            float(ybar + slope * Fraction(n + 1, 2)))


def shifted(v, off):
    return [a + off for a in v]


# ---------------------------------------------------------------------------
# The golden battery.
# ---------------------------------------------------------------------------

LADDER_PERIODS = (2, 5, 14, 30)


def build(ds):
    """Returns (list of (name, values, comment), list of (name, define-value,
    comment), list of int-table (name, values, comment))."""
    arrays = []
    defines = []
    ints = []

    def add(name, vals, comment):
        arrays.append((name, vals, comment))

    # --- CORREL over the pandas rolling-corr arrays (GH#65739) --------------
    ox, oy = ds["ta_test_ref_pd_outlier_x"], ds["ta_test_ref_pd_outlier_y"]
    n = len(ox)
    add("ta_test_ref_golden_corr_outlier_off0",
        [corr_window(ox, oy, s, 9) for s in range(n - 8)],
        "pandas GH#65739 outlier_exit, period 9, no offset on y")
    oy13 = shifted(oy, 1.0e13)
    add("ta_test_ref_golden_corr_outlier_off1e13",
        [corr_window(ox, oy13, s, 9) for s in range(n - 8)],
        "the same, y carrying a 1e13 shared offset")

    sx, sy = ds["ta_test_ref_pd_shared_x"], ds["ta_test_ref_pd_shared_y"]
    for off, tag in ((1.0e10, "1e10"), (1.0e14, "1e14")):
        a, b = shifted(sx, off), shifted(sy, off)
        add("ta_test_ref_golden_corr_shared_%s" % tag,
            [corr_window(a, b, s, 5) for s in range(len(sx) - 4)],
            "pandas GH#65739 shared_offset %s, period 5" % tag)

    nx, ny = ds["ta_test_ref_pd_nonan_x"], ds["ta_test_ref_pd_nonan_y"]
    add("ta_test_ref_golden_corr_nonan",
        [corr_window(nx, ny, s, 3) for s in range(len(nx) - 2)],
        "pandas GH#65739 outlier_exit_no_nan, period 3")

    ex, ey = ds["ta_test_ref_pd_extreme_x"], ds["ta_test_ref_pd_extreme_y"]
    add("ta_test_ref_golden_corr_extreme",
        [corr_window(ex, ey, s, 5) for s in range(len(ex) - 4)],
        "pandas GH#65739 extreme_range rescaled to TA_REAL_MAX, period 5")

    # --- VAR over the pandas rolling-var arrays -----------------------------
    v47 = ds["ta_test_ref_pd_var47721"]
    add("ta_test_ref_golden_var47721",
        [var_window(v47, s, 6) for s in range(len(v47) - 5)],
        "pandas GH#47721 (a 1e10 value transiting a window of 6), period 6")
    v52 = ds["ta_test_ref_pd_var52407"]
    add("ta_test_ref_golden_var52407",
        [var_window(v52, s, 3) for s in range(len(v52) - 2)],
        "pandas GH#52407 (mixed tiny magnitudes -> negative variance), period 3")

    # --- LINEARREG over Wilkinson's nasty.dat, period 9 ---------------------
    wnames = ["x", "round", "big", "little", "huge", "tiny", "zero"]
    wseries = [ds["ta_test_ref_wilkinson_" + w] for w in wnames]
    quads = [linreg_window(w, 0, 9) for w in wseries]
    for i, key in enumerate(("slope", "intercept", "fit", "tsf")):
        add("ta_test_ref_golden_wilkinson_" + key, [q[i] for q in quads],
            "Wilkinson nasty.dat regressed on the bar index, period 9, "
            "series order X ROUND BIG LITTLE HUGE TINY ZERO")

    # --- LINEARREG + sigma over the sliding-sum ladder ----------------------
    lad = ds["ta_test_ref_ladder"]
    ints.append(("ta_test_ref_golden_ladder_periods", list(LADDER_PERIODS),
                 "the periods the ladder tables below are indexed by"))
    ints.append(("ta_test_ref_golden_ladder_counts",
                 [len(lad) - p + 1 for p in LADDER_PERIODS],
                 "windows per period"))
    for p in LADDER_PERIODS:
        wins = [linreg_window(lad, s, p) for s in range(len(lad) - p + 1)]
        for i, key in enumerate(("slope", "intercept", "fit", "tsf")):
            add("ta_test_ref_golden_ladder_p%d_%s" % (p, key),
                [w[i] for w in wins],
                "sliding-sum ladder, period %d" % p)
        add("ta_test_ref_golden_ladder_p%d_sigma" % p,
            [sigma_window(lad, s, p) for s in range(len(lad) - p + 1)],
            "sliding-sum ladder population sigma, period %d" % p)

    # --- NIST StRD Norris, exact for the TRANSCRIBED DOUBLES ----------------
    # NIST certifies the fit of the exact decimal data. The doubles this
    # library is handed are those decimals rounded, so the exact answer for
    # what it actually sees differs from the certificate in the last digits.
    # Both are recorded: the certificate is the external truth, these are what
    # a perfect implementation would return on the stored input.
    x, y = ds["ta_test_ref_norris_x"], ds["ta_test_ref_norris_y"]
    n = len(x)
    _, _, sxx, syy, sxy = moments(x, y, 0, n)
    b1 = sxy / sxx
    b0 = sum(Fraction(v) for v in y) / n - b1 * (sum(Fraction(v) for v in x) / n)
    r2 = (sxy * sxy) / (sxx * syy)
    defines.append(("TA_TEST_REF_GOLDEN_NORRIS_B1", float(b1),
                    "exact OLS slope of the transcribed doubles "
                    "(NIST certifies 1.00211681802045 for the exact decimals)"))
    defines.append(("TA_TEST_REF_GOLDEN_NORRIS_B0", float(b0),
                    "exact OLS intercept (NIST certifies -0.262323073774029)"))
    defines.append(("TA_TEST_REF_GOLDEN_NORRIS_R", frac_sqrt(r2),
                    "exact Pearson r (NIST certifies R2 0.999993745883712, "
                    "i.e. r 0.99999687293696671)"))

    return arrays, defines, ints


# ---------------------------------------------------------------------------
# Emission.
# ---------------------------------------------------------------------------

BANNER = """/* GENERATED FILE -- DO NOT EDIT.
 *
 * Produced by scripts/gen_test_reference.py from the datasets in
 * ta_test_reference.c, in exact rational arithmetic. Issue #251.
 *
 * Re-generate with:   scripts/gen_test_reference.py
 * Verify in place:    scripts/gen_test_reference.py --check
 */
"""


def fmt(v):
    if v == 0.0:
        # Keep -0.0 distinguishable; repr() already does, this is only to make
        # the common case read as a C double rather than an int.
        return "-0.0" if str(v)[0] == "-" else "0.0"
    return repr(v)


def emit_h(arrays, defines, ints):
    out = [BANNER, "", "#ifndef TA_TEST_REFERENCE_GOLDEN_H",
           "#define TA_TEST_REFERENCE_GOLDEN_H", ""]
    for name, val, comment in defines:
        out.append("/* %s */" % comment)
        out.append("#define %s (%s)" % (name, fmt(val)))
        out.append("")
    for name, vals, comment in ints:
        out.append("/* %s */" % comment)
        out.append("#define %s_N %d" % (name.upper(), len(vals)))
        out.append("extern const int %s[%s_N];" % (name, name.upper()))
        out.append("")
    for name, vals, comment in arrays:
        out.append("/* %s */" % comment)
        out.append("#define %s_N %d" % (name.upper(), len(vals)))
        out.append("extern const double %s[%s_N];" % (name, name.upper()))
        out.append("")
    out.append("#endif /* TA_TEST_REFERENCE_GOLDEN_H */")
    return "\n".join(out) + "\n"


def emit_c(arrays, defines, ints):
    out = [BANNER, "", '#include "ta_test_reference_golden.h"', ""]
    for name, vals, comment in ints:
        out.append("/* %s */" % comment)
        out.append("const int %s[%s_N] = {" % (name, name.upper()))
        out.append("   " + ", ".join(str(v) for v in vals) + " };")
        out.append("")
    for name, vals, comment in arrays:
        out.append("/* %s */" % comment)
        out.append("const double %s[%s_N] = {" % (name, name.upper()))
        line = "  "
        for i, v in enumerate(vals):
            tok = " " + fmt(v) + ("," if i + 1 < len(vals) else " };")
            if len(line) + len(tok) > 78:
                out.append(line)
                line = "  "
            line += tok
        out.append(line)
        out.append("")
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="fail if regenerating would change the committed files")
    args = ap.parse_args()

    arrays, defines, ints = build(load_datasets())
    want = {OUT_H: emit_h(arrays, defines, ints),
            OUT_C: emit_c(arrays, defines, ints)}

    if args.check:
        bad = []
        for path, text in want.items():
            have = open(path, encoding="utf-8").read() if os.path.exists(path) else None
            if have != text:
                bad.append(path)
        if bad:
            print("gen_test_reference: STALE: " + ", ".join(bad))
            print("  run scripts/gen_test_reference.py")
            return 1
        print("gen_test_reference: goldens up to date (%d tables, %d values)"
              % (len(arrays), sum(len(v) for _, v, _ in arrays)))
        return 0

    for path, text in want.items():
        with open(path, "w", encoding="utf-8") as fh:
            fh.write(text)
        print("wrote %s" % os.path.relpath(path, REPO))
    print("%d tables, %d values" % (len(arrays), sum(len(v) for _, v, _ in arrays)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
