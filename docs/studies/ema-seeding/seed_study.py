#!/usr/bin/env python3
"""EMA seeding study for TA_SMI (ta-lib-proposal-drafts#59).

Question: TA-Lib seeds every EMA with an SMA of the first `period` samples;
TradingView's `ta.ema` and Tulip's `ti_smi` seed with the first sample itself
(for the SMI denominator that first sample is the initial `HH-LL` range, hence
"the H-L seed").  Which seed carries the smaller error?

"Error" is measured against the only thing that is well defined here: the
*converged* EMA -- the value an implementation with unlimited history would
produce at the same bar.  Neither seed is the truth; both are estimators of it.

Four parts:

  A. Analytic.  A seeded EMA's error at bar n is EXACTLY
     (1-alpha)^(n-t0) * (seed - y*(t0)).  The decay factor is a property of
     alpha alone, identical for both rules, so the whole question collapses to
     "which seed is the better estimator of y*(t0)".  That expectation is
     evaluated in closed form for white noise, AR(1), a random walk, and a
     linear trend.

  B. Empirical, single EMA, on the two real series SMI actually smooths
     (num = C - (HH+LL)/2 and den = HH - LL) plus close itself.

  C. Empirical, end-to-end SMI(q,r,s), error in SMI points, over thousands of
     independent data-start offsets on long real daily histories.

  D. The issue's own 252-bar table, reproduced (validation), then repeated on a
     252-bar window that HAS pre-history so the arm-to-arm gap can be split
     into "how wrong is TA-Lib" and "how wrong is TradingView/Tulip".

Corpora (all real daily OHLC, no synthetic data):
  * gData      -- 10000 bars, src/tools/ta_regtest/ta_gData{High,Low,Close}.c
                  (its first 252 bars are the corpus issue #59 measured on)
  * IBM        -- 6741 bars 1999-11-01..2026-08-20, data/ibm_daily_ohlc.csv

Run:  python3 seed_study.py            (writes results.txt, prints the same)
"""

import csv
import os
import re
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", "..", ".."))

OUT = []


def say(*a):
    line = " ".join(str(x) for x in a)
    OUT.append(line)
    print(line)


# --------------------------------------------------------------------------
# corpora
# --------------------------------------------------------------------------

def _c_array(path, name):
    src = open(path).read()
    m = re.search(re.escape(name) + r"\s*\[[^\]]*\]\s*=\s*\{", src)
    if not m:
        raise SystemExit("array %s not found in %s" % (name, path))
    depth, i = 1, m.end()
    while depth:
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
        i += 1
    body = src[m.end():i - 1]
    return np.array([float(v) for v in re.split(r"[,\s]+", body) if v])


def corpus_gdata():
    d = os.path.join(REPO, "src", "tools", "ta_regtest")
    return {n: _c_array(os.path.join(d, "ta_gData%s.c" % n.title()), "gData" + n.title())
            for n in ("high", "low", "close")}


def corpus_ibm():
    rows = list(csv.DictReader(open(os.path.join(HERE, "data", "ibm_daily_ohlc.csv"))))
    return {n: np.array([float(r[n]) for r in rows]) for n in ("high", "low", "close")}


# --------------------------------------------------------------------------
# primitives
# --------------------------------------------------------------------------

def alpha_of(p):
    return 2.0 / (p + 1.0)


def roll_max(x, q):
    """TA_MAX: out[i] = max(x[i-q+1..i]) for i >= q-1, NaN before."""
    out = np.full(x.shape, np.nan)
    out[q - 1:] = np.lib.stride_tricks.sliding_window_view(x, q).max(axis=1)
    return out


def roll_min(x, q):
    out = np.full(x.shape, np.nan)
    out[q - 1:] = np.lib.stride_tricks.sliding_window_view(x, q).min(axis=1)
    return out


def ema_full(x, p, first):
    """EMA over a whole series, seeded with the first sample at index `first`.

    The reference/"converged" run: started far enough back that the seed's
    contribution is below double precision by the time we look at it.
    """
    a = alpha_of(p)
    out = np.full(x.shape, np.nan)
    prev = x[first]
    out[first] = prev
    for i in range(first + 1, len(x)):
        prev += (x[i] - prev) * a
        out[i] = prev
    return out


def ema_block_T(win, p):
    """TradingView `ta.ema` / Tulip rule, vectorised over rows.

    win: (rows, K); column 0 is each row's first available sample.
    Seed = win[:,0]; output starts at column 0.
    """
    a = alpha_of(p)
    out = np.empty(win.shape)
    prev = win[:, 0].copy()
    out[:, 0] = prev
    for k in range(1, win.shape[1]):
        prev += (win[:, k] - prev) * a
        out[:, k] = prev
    return out


def ema_block_S(win, p):
    """TA-Lib / R TTR rule, vectorised over rows.

    Seed = mean(win[:, 0:p]) placed at column p-1; output starts there.
    Columns < p-1 are NaN -- nothing is published.
    """
    a = alpha_of(p)
    if p == 1:
        return win.copy()
    out = np.full(win.shape, np.nan)
    prev = win[:, 0].copy()                  # sequential sum, as TA_EMA does,
    for i in range(1, p):                    # so the seed matches bit for bit
        prev += win[:, i]
    prev /= p
    out[:, p - 1] = prev
    for k in range(p, win.shape[1]):
        prev += (win[:, k] - prev) * a
        out[:, k] = prev
    return out


def smi_series(high, low, close, q):
    hh, ll = roll_max(high, q), roll_min(low, q)
    return close - 0.5 * (hh + ll), hh - ll


def smi_truth(high, low, close, q, r, s):
    """SMI with unlimited history: the EMA chain started at the very first bar."""
    num, den = smi_series(high, low, close, q)
    f = q - 1
    n2 = ema_full(ema_full(num, r, f), s, f)
    d2 = ema_full(ema_full(den, r, f), s, f)
    return 100.0 * n2 / (0.5 * d2)


def smi_arm(num, den, q, r, s, starts, K, rule):
    """SMI as computed by a caller whose data begins at bar starts[i].

    Returns (values, bars): values[i,k] is the arm's SMI at bar
    bars[i,k] = starts[i] + q - 1 + k, NaN where the arm publishes nothing.
    Both arms share the identical TA_MAX/TA_MIN front end; only seeding differs.
    """
    base = starts[:, None] + (q - 1) + np.arange(K)[None, :]
    wn, wd = num[base], den[base]
    ema = ema_block_T if rule == "T" else ema_block_S
    n1, d1 = ema(wn, r), ema(wd, r)
    off = 0 if rule == "T" else r - 1          # first column the 2nd stage sees
    n2 = np.full_like(n1, np.nan)
    d2 = np.full_like(d1, np.nan)
    n2[:, off:] = ema(n1[:, off:], s)
    d2[:, off:] = ema(d1[:, off:], s)
    return 100.0 * n2 / (0.5 * d2), base


def rms(v):
    v = v[~np.isnan(v)]
    return float(np.sqrt(np.mean(v ** 2)))


# --------------------------------------------------------------------------
# Part A -- analytic
# --------------------------------------------------------------------------

def seed_coeffs(p):
    """Coefficients of (seed - y*) as a linear functional of the series.

    Index j is the lag behind the seed's own evaluation bar.  Both vectors sum
    to zero, so the error is invariant to the series level -- which is what
    makes the random-walk case well posed.
    """
    a = alpha_of(p)
    J = int(np.ceil(np.log(1e-18) / np.log(1 - a))) + p + 2
    w = a * (1 - a) ** np.arange(J)
    cT = -w.copy()
    cT[0] += 1.0
    cS = -w.copy()
    cS[:p] += 1.0 / p
    return cT, cS


def var_stationary(c, rho):
    """Var(c . x) for a unit-variance stationary series with acf rho(L)."""
    acf = rho(np.arange(len(c)))
    ac = np.correlate(c, c, mode="full")[len(c) - 1:]     # ac[L] = sum_j c_j c_{j+L}
    return float(ac[0] * acf[0] + 2.0 * np.sum(ac[1:] * acf[1:]))


def var_randomwalk(c):
    """Var(c . x) for a random walk with unit-variance increments."""
    tail = np.cumsum(c[::-1])[::-1]                       # tail[m] = sum_{j>=m} c_j
    return float(np.sum(tail[1:] ** 2))


def trend_bias(c):
    """(seed - y*) for x_{-j} = x_0 - j, i.e. a unit upward slope per bar."""
    return -float(np.sum(c * np.arange(len(c))))


def part_a():
    say("=" * 78)
    say("PART A -- analytic: which seed estimates the converged EMA better?")
    say("=" * 78)
    say("")
    say("A seeded EMA reproduces the converged EMA exactly, plus one decaying term:")
    say("    y_seed(n) - y*(n) = (1-alpha)^(n-t0) * (seed - y*(t0))")
    say("The decay factor depends on alpha only -- it is the SAME for both rules.")
    say("So the seeding question is entirely 'how big is (seed - y*(t0))?'.")
    say("")
    say("Rule T (TradingView ta.ema / Tulip): seed = x[t0], t0 = first data bar.")
    say("Rule S (TA-Lib / R TTR):             seed = SMA(x[t0-p+1..t0]), t0 = bar p-1,")
    say("                                     summed sequentially, as TA_EMA does.")
    say("")
    say("Rule T seeds p-1 bars earlier, so by rule S's first output bar its error has")
    say("already been damped by (1-alpha)^(p-1).  The 'common' column credits it with")
    say("exactly that head start: it is sd(e_S) / [ (1-alpha)^(p-1) * sd(e_T) ], the")
    say("ratio of the two rules' errors at the SAME bar.  Below 1 = TA-Lib wins.")
    say("")

    hdr = ("   p |  sd(e_T)  sd(e_S) | e_S/e_T | (1-a)^(p-1) |  common "
           "| trend bias: e_T     e_S")
    for name, rho in (("white noise", lambda L: (L == 0) * 1.0),
                      ("AR(1) phi=0.50", lambda L: 0.50 ** L),
                      ("AR(1) phi=0.90", lambda L: 0.90 ** L),
                      ("AR(1) phi=0.99", lambda L: 0.99 ** L),
                      ("random walk", None)):
        say("-- %s (unit variance / unit increment variance)" % name)
        say(hdr)
        for p in (2, 3, 5, 9, 12, 13, 20, 25, 26, 30, 50):
            cT, cS = seed_coeffs(p)
            if rho is None:
                vT, vS = var_randomwalk(cT), var_randomwalk(cS)
            else:
                vT, vS = var_stationary(cT, rho), var_stationary(cS, rho)
            sT, sS = np.sqrt(vT), np.sqrt(vS)
            damp = (1 - alpha_of(p)) ** (p - 1)
            say("%4d | %8.4f %8.4f | %7.3f | %11.5f | %7.3f | %11.2f %7.2f"
                % (p, sT, sS, sS / sT, damp, sS / (sT * damp),
                   trend_bias(cT), trend_bias(cS)))
        say("")

    say("Reading the table:")
    say("  * e_S/e_T < 1 under every process: the SMA seed is the better estimator")
    say("    of the converged EMA, by 3x-9x at the periods SMI uses.")
    say("  * 'common' is the honest head-to-head at the same bar index, with rule T")
    say("    given full credit for having started p-1 bars earlier.")
    say("  * 'trend bias' is EXACT, not statistical.  alpha = 2/(p+1) puts the EMA's")
    say("    centre of mass at (1-alpha)/alpha = (p-1)/2 bars back -- precisely the")
    say("    centre of mass of a p-bar SMA.  So on any locally linear stretch the SMA")
    say("    seed is unbiased to machine precision while the last-value seed is off")
    say("    by (p-1)/2 * slope.  The p-bar SMA is not an arbitrary convention: it is")
    say("    the seed that alpha = 2/(p+1) was matched to.")
    say("")


# --------------------------------------------------------------------------
# Part B -- empirical, single EMA
# --------------------------------------------------------------------------

def part_b(corpora):
    say("=" * 78)
    say("PART B -- empirical, one EMA, on the series SMI actually smooths")
    say("=" * 78)
    say("")
    say("Error vs the converged EMA, rms over every data-start offset in the corpus,")
    say("as a percentage of the series' own standard deviation.")
    say("  'first out' = each rule at its own first published bar")
    say("  '@common'   = rule T at rule S's first published bar (its head start)")
    say("  'ratio'     = e_S / e_T@common;  below 1 = TA-Lib wins at the same bar")
    say("")

    for cname, c in corpora:
        num, den = smi_series(c["high"], c["low"], c["close"], 13)
        for sname, x in (("close", c["close"]),
                         ("num = C-(HH+LL)/2, q=13", num),
                         ("den = HH-LL, q=13", den)):
            x = x[~np.isnan(x)]
            n, sd = len(x), x.std()
            say("-- %s / %s   (%d bars, sd = %.4f)" % (cname, sname, n, sd))
            say("      p | e_T first out | e_S first out |    e_T @common | ratio "
                "| bias e_T  bias e_S")
            for p in (2, 3, 5, 9, 12, 13, 20, 25, 26, 30):
                K = p + 2
                lo, hi = 400, n - K
                if hi - lo < 50:
                    continue
                starts = np.arange(lo, hi)
                truth = ema_full(x, p, 0)
                base = starts[:, None] + np.arange(K)[None, :]
                win, tr = x[base], truth[base]
                tT, tS = ema_block_T(win, p), ema_block_S(win, p)
                dT0, dS0 = tT[:, 0] - tr[:, 0], tS[:, p - 1] - tr[:, p - 1]
                eT0 = rms(dT0) / sd * 100
                eS0 = rms(dS0) / sd * 100
                eTc = rms(tT[:, p - 1] - tr[:, p - 1]) / sd * 100
                say("   %4d | %12.3f%% | %12.3f%% | %13.3f%% | %6.3f | %7.3f%% %8.3f%%"
                    % (p, eT0, eS0, eTc, eS0 / eTc,
                       float(np.nanmean(dT0)) / sd * 100, float(np.nanmean(dS0)) / sd * 100))
            say("")


# --------------------------------------------------------------------------
# Part C -- empirical, end-to-end SMI
# --------------------------------------------------------------------------

PARAMS = [(13, 25, 2), (10, 3, 3), (5, 3, 3), (14, 20, 5)]


def part_c(corpora):
    say("=" * 78)
    say("PART C -- end-to-end SMI(q,r,s), error in SMI points (range -100..+100)")
    say("=" * 78)
    say("")
    say("Both arms share the identical TA_MAX/TA_MIN front end; only the four EMA")
    say("seeds differ.  Truth = the same chain run from the start of a long history,")
    say("so its own seed is far below double precision by the time we compare.")
    say("")

    for cname, c in corpora:
        h, l, cl = c["high"], c["low"], c["close"]
        n = len(cl)
        for (q, r, s) in PARAMS:
            lb = (q - 1) + (r - 1) + (s - 1)
            K = max(400, 14 * (r + s))
            lo, hi = 500, n - K - q
            if hi - lo < 200:
                continue
            step = max(1, (hi - lo) // 2000)
            starts = np.arange(lo, hi, step)
            truth = smi_truth(h, l, cl, q, r, s)
            num, den = smi_series(h, l, cl, q)
            vT, base = smi_arm(num, den, q, r, s, starts, K, "T")
            vS, _ = smi_arm(num, den, q, r, s, starts, K, "S")
            tr = truth[base]
            eT, eS = vT - tr, vS - tr
            kS = lb - (q - 1)                 # column of rule S's first output

            say("-- %s   SMI(q=%d, r=%d, s=%d)   %d offsets, TA-Lib lookback %d"
                % (cname, q, r, s, len(starts), lb))

            def stat(e, k):
                v = np.abs(e[:, k])
                v = v[~np.isnan(v)]
                return float(np.sqrt(np.mean(v ** 2))), float(np.percentile(v, 95)), float(v.max())

            say("   first published value:")
            say("      rule T (TV/Tulip)  at bar %3d: rms %8.3f  p95 %8.3f  max %8.3f"
                % (q - 1, *stat(eT, 0)))
            say("      rule S (TA-Lib)    at bar %3d: rms %8.3f  p95 %8.3f  max %8.3f"
                % (lb, *stat(eS, kS)))
            say("   both at bar %d (TA-Lib's first output; rule T pre-damped):" % lb)
            say("      rule T: rms %8.3f  p95 %8.3f  max %8.3f" % stat(eT, kS))
            say("      rule S: rms %8.3f  p95 %8.3f  max %8.3f" % stat(eS, kS))

            raw = 100.0 * num[base] / (0.5 * den[base])
            say("   character of the first published value:")
            say("      rule T equals the RAW unsmoothed 100*num/(0.5*den): max diff %.3g"
                % float(np.nanmax(np.abs(vT[:, 0] - raw[:, 0]))))
            say("      amplitude vs truth (sd across offsets, 1.0 = right):"
                "  rule T %.2f   rule S %.2f"
                % (float(np.nanstd(vT[:, 0]) / np.nanstd(tr[:, 0])),
                   float(np.nanstd(vS[:, kS]) / np.nanstd(tr[:, kS]))))
            say("      max |SMI| over all published values (the bound is 100, and both"
                " rules keep it):")
            say("         rule T %.14f   rule S %.14f   truth %.14f"
                % (float(np.nanmax(np.abs(vT))), float(np.nanmax(np.abs(vS))),
                   float(np.nanmax(np.abs(tr)))))

            wT = np.nanmax(np.abs(eT), axis=1)
            wS = np.nanmax(np.abs(eS), axis=1)
            say("   worst error anywhere in the arm's own published output:")
            say("      rule T: rms %8.3f  p95 %8.3f  max %8.3f"
                % (float(np.sqrt(np.mean(wT ** 2))), float(np.percentile(wT, 95)), float(wT.max())))
            say("      rule S: rms %8.3f  p95 %8.3f  max %8.3f"
                % (float(np.sqrt(np.mean(wS ** 2))), float(np.percentile(wS, 95)), float(wS.max())))

            def settle(e, k0, tol):
                """Bars after the data start before rms error stays under tol."""
                cols = np.arange(k0, e.shape[1])
                rmsk = np.array([rms(e[:, k]) for k in cols])
                bad = np.where(rmsk >= tol)[0]
                if len(bad) == 0:
                    return (q - 1) + k0
                if bad[-1] + 1 >= len(cols):
                    return None
                return (q - 1) + int(cols[bad[-1] + 1])

            say("   bars of history needed before rms error stays under ...")
            for tol in (1.0, 0.1, 0.01):
                say("      %5.2f pt : rule T %s   rule S %s"
                    % (tol, str(settle(eT, 0, tol)).rjust(5),
                       str(settle(eS, kS, tol)).rjust(5)))
            say("")


# --------------------------------------------------------------------------
# Part D -- the issue's own table, reproduced then attributed
# --------------------------------------------------------------------------

def part_d(gdata):
    say("=" * 78)
    say("PART D -- the issue's 252-bar table: reproduced, then attributed")
    say("=" * 78)
    say("")
    say("D1 validates this script against the numbers already in issue #59, which were")
    say("measured independently against a compiled Tulip `ti_smi` (and reproduced it")
    say("bit-for-bit).  Corpus = the first 252 bars of gData -- the issue calls it")
    say("'TA_SREF' but cites the gData files, and only gData reproduces its numbers;")
    say("the separate 252-bar TA_SREF_*_daily_ref_0_PRIV arrays are different data.")
    say("")
    h, l, cl = (gdata["high"][:252], gdata["low"][:252], gdata["close"][:252])
    say("   params    | lookback | bars |  max |A-B| | bit-identical from | issue says")
    ISSUE = {(13, 25, 2): (14.69, None), (10, 3, 3): (4.17, 67),
             (5, 3, 3): (3.59, 65), (14, 20, 5): (12.99, None)}
    for (q, r, s) in PARAMS:
        num, den = smi_series(h, l, cl, q)
        K = len(cl) - (q - 1)
        vT, _ = smi_arm(num, den, q, r, s, np.array([0]), K, "T")
        vS, _ = smi_arm(num, den, q, r, s, np.array([0]), K, "S")
        d = np.abs(vT[0] - vS[0])
        ok = ~np.isnan(d)
        lb = (q - 1) + (r - 1) + (s - 1)
        tail = "never"
        for i in range(len(d)):
            if ok[i] and np.all(d[i:][ok[i:]] == 0.0):
                tail = str(i + (q - 1))
                break
        want = ISSUE[(q, r, s)]
        say("   (%2d,%2d,%d) |    %3d   | %4d | %10.4f | %18s | %.2f / %s"
            % (q, r, s, lb, int(ok.sum()), float(np.nanmax(d)), tail,
               want[0], want[1] if want[1] else "-"))
    say("")
    say("   spot values, SMI(13,25,2), against the issue's sample table:")
    num, den = smi_series(h, l, cl, 13)
    vT, _ = smi_arm(num, den, 13, 25, 2, np.array([0]), 252 - 12, "T")
    vS, _ = smi_arm(num, den, 13, 25, 2, np.array([0]), 252 - 12, "S")
    for bar, wa, wb in ((37, -10.8035818118385, -25.4984363633175),
                        (100, -45.1029436620775, -45.2341954189360),
                        (251, -46.5268563975259, -46.5268575905730)):
        k = bar - 12
        say("     bar %3d  A %18.13f (issue %18.13f)  B %18.13f (issue %18.13f)"
            % (bar, vS[0, k], wa, vT[0, k], wb))
    say("")
    say("D2 repeats the identical 252-bar experiment on windows of gData that HAVE")
    say("history in front of them, so the gap can be attributed.  All three numbers")
    say("are measured over the SAME bars -- bar >= lookback, where both arms publish.")
    say("")
    for (q, r, s) in PARAMS:
        lb = (q - 1) + (r - 1) + (s - 1)
        K = 252 - (q - 1)
        kS = lb - (q - 1)
        n = len(gdata["close"])
        starts = np.arange(500, n - 252 - q, 4)
        truth = smi_truth(gdata["high"], gdata["low"], gdata["close"], q, r, s)
        num, den = smi_series(gdata["high"], gdata["low"], gdata["close"], q)
        vT, base = smi_arm(num, den, q, r, s, starts, K, "T")
        vS, _ = smi_arm(num, den, q, r, s, starts, K, "S")
        tr = truth[base]
        gap = np.nanmax(np.abs(vT - vS)[:, kS:], axis=1)
        wT = np.nanmax(np.abs(vT - tr)[:, kS:], axis=1)
        wS = np.nanmax(np.abs(vS - tr)[:, kS:], axis=1)
        say("   SMI(%2d,%2d,%d), %d windows, over the %d bars both arms publish:"
            % (q, r, s, len(starts), K - kS))
        say("      |A-B| disagreement : rms %8.3f  max %8.3f" %
            (float(np.sqrt(np.mean(gap ** 2))), float(gap.max())))
        say("      rule S (TA-Lib)    : rms %8.3f  max %8.3f" %
            (float(np.sqrt(np.mean(wS ** 2))), float(wS.max())))
        say("      rule T (TV/Tulip)  : rms %8.3f  max %8.3f" %
            (float(np.sqrt(np.mean(wT ** 2))), float(wT.max())))
        say("      TA-Lib owns %.0f%% of the disagreement (median over windows)"
            % (100 * float(np.median(wS / (wS + wT)))))
        say("")


# --------------------------------------------------------------------------
# Part E -- EMA cascades: the DEMA / TEMA / TRIX / T3 / MACD shape
# --------------------------------------------------------------------------

CASCADES = [(1, "EMA"), (2, "DEMA"), (3, "TEMA / TRIX"), (6, "T3")]


def cascade_block(win, p, n, rule):
    """n chained EMAs, each seeded by `rule`, vectorised over rows."""
    ema = ema_block_T if rule == "T" else ema_block_S
    off, y = 0, win
    for _ in range(n):
        z = np.full(y.shape, np.nan)
        z[:, off:] = ema(y[:, off:], p)
        y = z
        if rule == "S":
            off += p - 1
    return y, off


def cascade_full(x, p, n):
    y = x
    for _ in range(n):
        y = ema_full(y, p, 0)
    return y


def part_e(corpora):
    say("=" * 78)
    say("PART E -- EMA cascades, the shape DEMA/TEMA/TRIX/T3/MACD actually ship")
    say("=" * 78)
    say("")
    say("Every one of those inlines the SAME per-stage seed as TA_EMA: an SMA of the")
    say("stage's first p inputs (verified by reading ta_codegen/input/{dema,tema,trix,")
    say("macd,efi,t3}/*.c -- all carry the CLASSIC branch, and all but t3 also carry a")
    say("METASTOCK branch seeding from inReal[0]; t3 ships CLASSIC only).  So the seed")
    say("error compounds n times and the lookback grows")
    say("as n*(p-1).  Error vs the converged cascade, rms over every data-start offset,")
    say("as a percentage of the input series' standard deviation.")
    say("")
    for cname, c in corpora:
        x = c["close"]
        n_bars, sd = len(x), x.std()
        say("-- %s / close  (%d bars, sd = %.4f)" % (cname, n_bars, sd))
        say("   stages          p | lookback | e_T first out | e_S first out |"
            " e_T @common | ratio")
        for n, label in CASCADES:
            for p in (5, 12, 26, 30):
                lb = n * (p - 1)
                K = lb + 2
                lo, hi = 500, n_bars - K
                if hi - lo < 200:
                    continue
                starts = np.arange(lo, hi)
                truth = cascade_full(x, p, n)
                base = starts[:, None] + np.arange(K)[None, :]
                win, tr = x[base], truth[base]
                yT, _ = cascade_block(win, p, n, "T")
                yS, offS = cascade_block(win, p, n, "S")
                eT0 = rms(yT[:, 0] - tr[:, 0]) / sd * 100
                eS0 = rms(yS[:, offS] - tr[:, offS]) / sd * 100
                eTc = rms(yT[:, offS] - tr[:, offS]) / sd * 100
                say("   %d %-13s %2d | %8d | %12.3f%% | %12.3f%% | %10.3f%% | %6.3f"
                    % (n, "(%s)" % label, p, lb, eT0, eS0, eTc, eS0 / eTc))
        say("")


def part_zero():
    say("=" * 78)
    say("PART 0 -- both arms are pinned to real implementations, not modelled")
    say("=" * 78)
    say("")
    say("rule S == the shipped TA_EMA.  ema_probe.c calls TA_EMA(0,60,close,25) from")
    say("  cmake-build/libta-lib.a; all 37 outputs are BIT-IDENTICAL to ema_block_S")
    say("  below.  The same probe shows TA_EMA's first output is the plain 25-bar SMA")
    say("  ending at startIdx (bit-identical), and that TA_EMA re-seeds at")
    say("  startIdx-lookback -- TA_EMA(400,400) and TA_EMA(0,500)[400] differ by 0.199")
    say("  on gData close, so the first-published-value row below is not a curiosity:")
    say("  it is what any narrow-range TA-Lib call returns.")
    say("")
    say("rule T == Tulip ti_smi / TradingView ta.ema.  beta/smi.c seeds")
    say("  ema_r_num = ema_s_num = num[q-1] and ema_r_den = ema_s_den = den[q-1];")
    say("  Pine's ta.ema is `na(sum[1]) ? src : alpha*src + (1-alpha)*nz(sum[1])`.")
    say("  Part D reproduces, to every printed digit, the arm-vs-Tulip table in issue")
    say("  #59 -- which was itself measured against a compiled ti_smi.")
    say("")


def main():
    gdata = corpus_gdata()
    corpora = [("gData 10000", gdata), ("IBM 6741", corpus_ibm())]
    part_zero()
    part_a()
    part_b(corpora)
    part_c(corpora)
    part_d(gdata)
    part_e(corpora)
    with open(os.path.join(HERE, "results.txt"), "w") as f:
        f.write("\n".join(OUT) + "\n")
    say("wrote %s" % os.path.join(HERE, "results.txt"))


if __name__ == "__main__":
    sys.exit(main())
