#!/usr/bin/env python3
"""Seed study for a first-order recursive indicator, run here for TA_MCGD (#471).

Question: which seed rule gives the first published value closest to the value
an implementation with unlimited history prints at the same bar?

The harness (`study`, `report`) knows nothing about MCGD: it takes a vectorised
`step` and a list of `Rule`s.  A later recursive function adds its own step,
rules and linearised kernel, and calls the same functions.

Run:  python3 seed_study.py [--oracle DIR]    (writes results.txt; numpy only)
  --oracle DIR  also compare rule S with an independent SMA-seeded implementation,
                dumped as <corpus>_N<N>.txt: a header line, then one value per line
"""

import argparse
import importlib.util
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))

OUT = []


def say(*a):
    line = " ".join(str(x) for x in a)
    OUT.append(line)
    print(line)


# --------------------------------------------------------------------------
# corpora
# --------------------------------------------------------------------------

def _load_ema_study():
    spec = importlib.util.spec_from_file_location(
        "ema_seed_study", os.path.join(HERE, "..", "ema-seeding", "seed_study.py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


SPLICE = 1.25   # a one-bar rise above this starts a new gData segment


def corpora():
    """{name: [series, ...]}.  gData close holds six one-bar rises of 26% to
    299% and no fall below -12%: price-level splices, which stall a recursion
    whose gain falls as (price/line)^-4.  The primary gData corpus is therefore
    the spliced segments, each its own series; the whole series is kept as a
    stress case."""
    ema = _load_ema_study()
    g = ema.corpus_gdata()["close"]
    ibm = ema.corpus_ibm()["close"]
    cut = [0] + (np.where(g[1:] / g[:-1] > SPLICE)[0] + 1).tolist() + [len(g)]
    segs = [g[a:b] for a, b in zip(cut, cut[1:])]
    return {"IBM": [ibm], "gData segments": segs, "gData whole (stress)": [g]}, g, ibm, cut


# --------------------------------------------------------------------------
# generic harness
# --------------------------------------------------------------------------

class Rule:
    """seed_bar(N): bar, relative to the data start s, the seed is attached to.
    publish(N): first published bar, relative to s.
    seed(x, starts, N): the seed for every start offset in `starts`."""

    def __init__(self, name, seed_bar, publish, seed, note):
        self.name, self.seed_bar, self.publish, self.seed, self.note = \
            name, seed_bar, publish, seed, note


def seq_mean(x, starts, width):
    acc = np.zeros(len(starts))
    for j in range(width):            # input order from 0.0, as TA_SMA sums
        acc += x[starts + j]
    return acc / width


def reference(x, step, N):
    """The converged line and the bars where it can be trusted: from MIN_HISTORY on,
    where three runs agree to REF_AGREE: first-sample seeds at bars 0 and 100, and
    an SMA seed over bars 0..N-1."""
    ref = full_run(x, step, N, 0)
    alt = [full_run(x, step, N, 100)]
    sma = np.full(len(x), np.nan)
    y = seq_mean(x, np.array([0]), N)
    sma[N - 1] = y[0]
    for i in range(N, len(x)):
        y = step(y, x[i:i + 1], N)
        sma[i] = y[0]
    alt.append(sma)
    ok = np.zeros(len(x), bool)
    ok[MIN_HISTORY:] = np.logical_and.reduce(
        [np.abs(a - ref)[MIN_HISTORY:] <= REF_AGREE * np.abs(ref)[MIN_HISTORY:] for a in alt])
    return ref, ok


def full_run(x, step, N, first):
    """The recursion over the whole series from a first-sample seed at `first`."""
    out = np.full(len(x), np.nan)
    y = np.array([x[first]])
    out[first] = y[0]
    for i in range(first + 1, len(x)):
        y = step(y, x[i:i + 1], N)
        out[i] = y[0]
    return out


MIN_HISTORY = 500     # the reference is used from this bar of a series on,
REF_AGREE = 1e-12     # where a run started 100 bars later agrees this closely
THRESHOLDS = (1e-3, 1e-6, 1e-12)
CONV_HORIZON = 1500   # bars followed after the first output for convergence
TAIL_HORIZON = 4000
LAG_HORIZON = 1000


def study_one(x, step, rules, N):
    n = len(x)
    common = max(r.publish(N) for r in rules)
    res = {"offsets": 0, "ref_excluded": 0, "common": common, "conv_n": 0, "rules": {}}
    starts = np.arange(MIN_HISTORY, n - common)
    if len(starts) == 0:
        return res
    ref, ok = reference(x, step, N)
    res["offsets"] = len(starts)
    res["ref_excluded"] = int((~ok[MIN_HISTORY:]).sum())
    # Convergence is measured, for every rule, only on offsets whose reference is
    # converged over the whole walk, so a NaN bar never reads as "below".
    bad = np.concatenate([[0], np.cumsum(~ok)])
    hi = starts + common + CONV_HORIZON
    conv_ok = hi <= n - 1
    conv_ok[conv_ok] = bad[hi[conv_ok] + 1] - bad[starts[conv_ok]] == 0
    res["conv_n"] = int(conv_ok.sum())
    for rule in rules:
        k0, P = rule.seed_bar(N), rule.publish(N)
        y = rule.seed(x, starts, N).astype(float)
        e_first = np.full(len(starts), np.nan)
        e_common = np.full(len(starts), np.nan)
        e_grow = np.full(len(starts), np.nan)
        last_above = {t: np.full(len(starts), P - 1) for t in THRESHOLDS}
        kmax = min(n - 1 - starts[0], P + CONV_HORIZON)
        for k in range(k0, kmax + 1):
            m = int(np.searchsorted(starts, n - k))      # offsets whose bar s+k exists
            bar = starts[:m] + k
            if k > k0:
                y[:m] = step(y[:m], x[bar], N)
            if k < P:
                continue
            e = np.where(ok[bar], (y[:m] - ref[bar]) / ref[bar], np.nan)
            if k == P:
                e_first[:m] = e
            if k == common:
                e_common[:m] = e
            if k == P + N:
                e_grow[:m] = e
            ae = np.abs(e)
            for t in THRESHOLDS:
                last_above[t][:m][ae > t] = k
        res["rules"][rule.name] = {
            "first": e_first, "common": e_common, "grow": e_grow,
            "conv": {t: (last_above[t] + 1 - P)[conv_ok] for t in THRESHOLDS},
            "censored": {t: int((conv_ok & (last_above[t] >= P + CONV_HORIZON)).sum())
                         for t in THRESHOLDS},
        }
    return res


def study(xs, step, rules, N):
    """study_one over several series, per-offset results concatenated."""
    parts = [study_one(x, step, rules, N) for x in xs]
    out = {"offsets": sum(p["offsets"] for p in parts),
           "ref_excluded": sum(p["ref_excluded"] for p in parts),
           "common": parts[0]["common"], "conv_n": sum(p["conv_n"] for p in parts), "rules": {}}
    for r in rules:
        ps = [p["rules"][r.name] for p in parts if r.name in p["rules"]]
        if not ps:
            continue
        cat = lambda key: np.concatenate([p[key] for p in ps])
        out["rules"][r.name] = {
            "first": cat("first"), "common": cat("common"), "grow": cat("grow"),
            "conv": {t: np.concatenate([p["conv"][t] for p in ps]) for t in THRESHOLDS},
            "censored": {t: sum(p["censored"][t] for p in ps) for t in THRESHOLDS},
        }
    # Rules are compared over the same offsets: those where every rule's first
    # output and the common bar have a converged reference.
    if out["rules"]:
        keep = np.logical_and.reduce([np.isfinite(r["first"]) & np.isfinite(r["common"])
                                      for r in out["rules"].values()])
        for r in out["rules"].values():
            for key in ("first", "common", "grow"):
                r[key] = np.where(keep, r[key], np.nan)
    return out


def stats(e):
    e = e[np.isfinite(e)]
    if len(e) == 0:
        return None
    a = np.abs(e)
    return (math.sqrt(float(np.mean(e * e))), float(np.mean(e)),
            float(np.percentile(a, 95)), float(a.max()), len(e))


def report(title, res, rules, N):
    say("")
    say("=== %s  N=%d: %d offsets; reference not converged on %d bars (excluded); common bar s+%d"
        % (title, N, res["offsets"], res["ref_excluded"], res["common"]))
    if not res["rules"] or stats(res["rules"][rules[0].name]["first"]) is None:
        say("  no offset has a converged reference")
        return
    say("  relative error (y - y*)/y*")
    say("  %-3s %-6s | %-9s %-10s %-9s %-9s %-6s | %-9s %-10s | %s"
        % ("", "first", "rms", "mean", "p95", "max", "n", "rms@cmn", "mean@cmn", "|e| grew over the next N bars"))
    for rule in rules:
        r = res["rules"][rule.name]
        f, c = stats(r["first"]), stats(r["common"])
        g = np.abs(r["grow"] / r["first"])
        g = g[np.isfinite(g)]
        say("  %-3s s+%-4d | %-9.3e %+-10.3e %-9.3e %-9.3e %-6d | %-9.3e %+-10.3e | %d of %d (max x%.2f)"
            % (rule.name, rule.publish(N), f[0], f[1], f[2], f[3], f[4], c[0], c[1],
               int((g > 1).sum()), len(g), float(g.max()) if len(g) else 0.0))
    if res["conv_n"] == 0:
        return
    say("  bars after the first output until |error| stays below, p50 / p95 over %d offsets"
        " (cN: N offsets still above after %d bars)" % (res["conv_n"], CONV_HORIZON))
    say("  %-3s | %s" % ("", " | ".join("%-18s" % ("< %g" % t) for t in THRESHOLDS)))
    for rule in rules:
        r = res["rules"][rule.name]
        cells = []
        for t in THRESHOLDS:
            v = r["conv"][t]
            cz = r["censored"][t]
            cells.append("%5d / %-5d %-6s" % (np.percentile(v, 50), np.percentile(v, 95),
                                               "c%d" % cz if cz else ""))
        say("  %-3s | %s" % (rule.name, " | ".join(cells)))


def blocks(res, a, b, nblocks=10):
    """How many of `nblocks` contiguous offset blocks rule a beats rule b in (rms at first output)."""
    ea, eb = res["rules"][a]["first"], res["rules"][b]["first"]
    keep = np.isfinite(ea) & np.isfinite(eb)
    ea, eb = ea[keep], eb[keep]
    wins = 0
    for ia, ib in zip(np.array_split(ea, nblocks), np.array_split(eb, nblocks)):
        wins += np.sqrt(np.mean(ia * ia)) < np.sqrt(np.mean(ib * ib))
    return wins, nblocks


# --------------------------------------------------------------------------
# MCGD
# --------------------------------------------------------------------------

PERIODS = (2, 3, 4, 5, 10, 14, 20, 30, 50, 200)


def mcgd_step(md, x, N):
    with np.errstate(all="ignore"):
        r = x / md
        nxt = md + (x - md) / (N * ((r * r) * (r * r)))
    return np.where(np.isfinite(nxt), nxt, md)


def ew_mean(x, starts, N):
    w = (1.0 - 1.0 / N) ** np.arange(N - 1, -1, -1)
    acc = np.zeros(len(starts))
    for j in range(N):
        acc += w[j] * x[starts + j]
    return acc / w.sum()


RULES = [
    Rule("S", lambda N: N - 1, lambda N: N - 1, lambda x, s, N: seq_mean(x, s, N),
         "SMA of x[s..s+N-1] at s+N-1 (#471's provisional rule)"),
    Rule("T", lambda N: 0, lambda N: 0, lambda x, s, N: x[s],
         "x[s] at s, published at once"),
    Rule("T'", lambda N: 0, lambda N: N - 1, lambda x, s, N: x[s],
         "x[s] at s, published from s+N-1"),
    Rule("L", lambda N: N - 1, lambda N: N - 1, lambda x, s, N: x[s + N - 1],
         "x[s+N-1] at s+N-1 (the zero-lag seed)"),
    Rule("E", lambda N: N - 1, lambda N: N - 1, ew_mean,
         "(1-1/N)^k-weighted mean of x[s..s+N-1] at s+N-1 (the RMA kernel, truncated)"),
    Rule("W", lambda N: 2 * N - 2, lambda N: 2 * N - 2, lambda x, s, N: seq_mean(x, s, 2 * N - 1),
         "SMA of x[s..s+2N-2] at s+2N-2 (centre of mass N-1 bars back, RMA's)"),
]


def hybrid(m):
    """SMA of the first m bars at s+m-1, then the recursion to s+N-1: m=1 is T', m=N is S."""
    return Rule("H%d" % m, lambda N, m=m: m - 1, lambda N: N - 1,
                lambda x, s, N, m=m: seq_mean(x, s, m), "")


def part_linear():
    say("")
    say("== A. Linearised: near price/line = 1 the step is RMA's, gain 1/N.  Each seed, and y*,")
    say("   is then a unit-sum linear filter of x; error sd per unit innovation sd, and the")
    say("   lag (bars) the rule leaves on a linear trend.  W is taken at its own first output.")
    K = 40000
    say("  %-5s | %s" % ("N", " | ".join("%-28s" % ("%s  wn / rw / trend lag" % k) for k in ("S", "T'", "L", "W"))))
    for N in PERIODS:
        a = 1.0 / N
        ystar = a * (1 - a) ** np.arange(K)
        ystar /= ystar.sum()
        ker = {k: np.zeros(K) for k in ("S", "T'", "L", "W")}
        ker["S"][:N] = 1.0 / N
        ker["T'"][:N - 1] = a * (1 - a) ** np.arange(N - 1)
        ker["T'"][N - 1] = (1 - a) ** (N - 1)
        ker["L"][0] = 1.0
        ker["W"][:2 * N - 1] = 1.0 / (2 * N - 1)
        cells = []
        for k in ("S", "T'", "L", "W"):
            d = ker[k] - ystar
            cells.append("%6.3f / %6.3f / %+7.2f       " % (
                math.sqrt(float((d * d).sum())),                       # white noise
                math.sqrt(float((np.cumsum(d)[:-1] ** 2).sum())),      # random walk
                -float((np.arange(K) * d).sum())))                     # trend
        say("  %-5d | %s" % (N, " | ".join(cells)))


def part_bias(ibm):
    say("")
    say("== C. Where the mean error comes from, IBM, bars 600 on (S's converged line as y*)")
    say("   A plain average of prices sits above the converged line by about 4*E[u^2],")
    say("   u = price/line - 1: the 4th power makes a bar below the line pull harder than")
    say("   one above it pushes.  Only a seed that runs through the recursion carries it.")
    say("  %-5s | %-14s %-14s %-12s" % ("N", "SMA_N vs y*", "SMA_2N-1 vs y*", "4*E[u^2]"))
    for N in PERIODS[:-1]:
        ref = rule_s_series(ibm, N)
        t = np.arange(600, len(ibm))
        sN = np.array([ibm[i - N + 1:i + 1].mean() for i in t])
        s2 = np.array([ibm[i - 2 * N + 2:i + 1].mean() for i in t])
        u = ibm[t] / ref[t] - 1
        say("  %-5d | %+-14.3e %+-14.3e %+-12.3e" % (
            N, np.mean(sN / ref[t] - 1), np.mean(s2 / ref[t] - 1), 4 * np.mean(u * u)))


def part_tail(corp):
    say("")
    say("== F. Where an S-seeded implementation meets T': bars after the first output")
    say("   until |T' - S| / |S| stays below 1e-14, p50 / p95 / max over every start offset")
    for name in ("IBM",):          # the gData segments are shorter than the horizon
        for N in PERIODS[:-1]:
            meds = []
            for x in corp[name]:
                starts = np.arange(0, len(x) - N - TAIL_HORIZON)
                if len(starts) == 0:
                    continue
                a = seq_mean(x, starts, N)
                b = x[starts].astype(float)
                for j in range(1, N):
                    b = mcgd_step(b, x[starts + j], N)
                last = np.zeros(len(starts), int)
                for k in range(0, TAIL_HORIZON):
                    if k:
                        a = mcgd_step(a, x[starts + N - 1 + k], N)
                        b = mcgd_step(b, x[starts + N - 1 + k], N)
                    last[np.abs(a - b) > 1e-14 * np.abs(a)] = k + 1
                meds.append(last)
            v = np.concatenate(meds)
            say("  %-15s N=%-4d %5d / %5d / %5d bars  (%d offsets)" % (
                name, N, np.percentile(v, 50), np.percentile(v, 95), v.max(), len(v)))
    c = np.array([100 + 10 * math.sin(i / 7) + 0.15 * i + 2 * math.sin(i / 2.3) for i in range(1200)])
    for N in (2, 10, 14, 50, 200):
        s = rule_s_series(c, N)
        t = full_run(c, mcgd_step, N, 0)
        d = np.abs(s - t) / np.abs(s)
        bad = np.where(d > 1e-14)[0]
        at = bad[-1] + 1 if len(bad) else N - 1
        say("  #471 1200-bar synthetic, N=%-4d from bar 0: T' within 1e-14 of S %s" % (
            N, "from bar %d" % at if at < len(c) else "on no bar (max rel %.1e at the last)" % d[-1]))


def part_lag(corp):
    say("")
    say("== G. T' against S at later bars: rms(T') / rms(S) at k bars after the first output,")
    say("   S's rms in brackets; over every offset with %d bars to follow" % LAG_HORIZON)
    for name in ("IBM", "gData segments"):
        for N in PERIODS[:-1]:
            sS, sT, cnt = np.zeros(LAG_HORIZON), np.zeros(LAG_HORIZON), np.zeros(LAG_HORIZON)
            for x in corp[name]:
                if len(x) <= MIN_HISTORY + N + LAG_HORIZON:
                    continue
                ref, okall = reference(x, mcgd_step, N)
                starts = np.arange(MIN_HISTORY, len(x) - N - LAG_HORIZON)
                yS = seq_mean(x, starts, N)
                yT = x[starts].astype(float)
                for j in range(1, N):
                    yT = mcgd_step(yT, x[starts + j], N)
                for k in range(LAG_HORIZON):
                    bar = starts + N - 1 + k
                    if k:
                        yS = mcgd_step(yS, x[bar], N)
                        yT = mcgd_step(yT, x[bar], N)
                    ok = okall[bar]
                    sS[k] += float(np.sum(np.where(ok, yS / ref[bar] - 1, 0) ** 2))
                    sT[k] += float(np.sum(np.where(ok, yT / ref[bar] - 1, 0) ** 2))
                    cnt[k] += int(ok.sum())
            rS, rT = np.sqrt(sS / np.maximum(cnt, 1)), np.sqrt(sT / np.maximum(cnt, 1))
            cross = np.where(rT > rS)[0]
            ks = [k for k in (0, N, 2 * N, 3 * N, 4 * N, 6 * N, 8 * N, 12 * N, 16 * N) if k < LAG_HORIZON]
            live = rS > 1e-12
            with np.errstate(all="ignore"):
                worst = int(np.argmax(np.where(live, rT / rS, 0)))
            say("  %-15s N=%-3d first k where T' is worse: %s; max ratio %.2f at k=%d (S rms %.0e)" % (
                name, N, "k=%d, S rms %.1e" % (cross[0], rS[cross[0]]) if len(cross) else "none",
                rT[worst] / rS[worst], worst, rS[worst]))
            say("      " + "  ".join("k=%d %.2f (%.0e)" % (k, rT[k] / rS[k], rS[k]) for k in ks))


def part_hybrid(corp):
    say("")
    say("== D. Hybrid seeds H(m): SMA of the first m bars, then the recursion to s+N-1")
    say("   (H1 = T', HN = S).  rms relative error at s+N-1.")
    for name in ("IBM", "gData segments"):
        for N in PERIODS[:-1]:
            ms = sorted(m for m in {1, 2, 3, max(1, N // 4), max(1, N // 2), max(1, 3 * N // 4), N} if m <= N)
            rs = [hybrid(m) for m in ms]
            res = study(corp[name], mcgd_step, rs, N)
            say("  %-15s N=%-4d %s" % (name, N, "  ".join(
                "H%d %.3e" % (m, stats(res["rules"]["H%d" % m]["first"])[0]) for m in ms)))


# --------------------------------------------------------------------------
# validation
# --------------------------------------------------------------------------

def rule_s_series(x, N):
    """Rule S from bar 0, i.e. what TA_MCGD(0, n-1) returns (NaN before N-1)."""
    out = np.full(len(x), np.nan)
    y = seq_mean(x, np.array([0]), N)
    out[N - 1] = y[0]
    for i in range(N, len(x)):
        y = mcgd_step(y, x[i:i + 1], N)
        out[i] = y[0]
    return out


def validate(oracle_dir, g, ibm):
    say("== 0. Validation of the harness's recursion and rule S")
    c = np.array([100 + 10 * math.sin(i / 7) + 0.15 * i + 2 * math.sin(i / 2.3) for i in range(1200)])
    gold = [(14, 13, 107.73907152422295), (14, 14, 107.93470414352933), (14, 15, 108.13782676673857),
            (14, 50, 103.29471927053798), (14, 100, 112.90934216832321), (14, 500, 175.75372293610602),
            (14, 999, 245.58749629044834), (14, 1199, 279.20106447901850), (10, 9, 106.88518208207886),
            (10, 1199, 281.27841736001125), (50, 49, 104.25789420798327), (50, 1199, 270.84466218343192)]
    worst = max(abs(rule_s_series(c, N)[bar] - v) / v for N, bar, v in gold)
    say("  #471's 12 goldens (60-digit, 1200-bar synthetic series): max rel %.2e (card tolerance 1e-14)"
        % worst)
    y = rule_s_series(np.array([10.0] * 7 + [14.0]), 7)[7]
    say("  primary worked example N=7, [10 x 7, 14]: %.17g; 170570/16807 = %.17g" % (y, 170570 / 16807))
    if not oracle_dir:
        say("  oracle: not compared (no --oracle)")
        return
    for name, x in (("gdata", g), ("ibm", ibm)):
        for N in PERIODS:
            path = os.path.join(oracle_dir, "%s_N%d.txt" % (name, N))
            if not os.path.exists(path):
                say("  oracle %-5s N=%-3d not dumped" % (name, N))
                continue
            with open(path) as f:
                head = f.readline().split()
                orc = np.array([float(v) for v in f.read().split()])
            y = rule_s_series(x, N)[N - 1:]
            d = np.abs(y - orc) / np.abs(orc)
            say("  oracle %-5s N=%-3d outBegIdx %-3s: %d values, first bit-equal %s, all bit-equal %d, max rel %.2e"
                % (name, N, head[1], len(orc), y[0] == orc[0], int((y == orc).sum()), float(d.max())))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--oracle")
    args = ap.parse_args()
    corp, g, ibm, cut = corpora()
    validate(args.oracle, g, ibm)
    say("")
    say("== Rules")
    for r in RULES:
        say("  %-3s %s" % (r.name, r.note))
    say("  gData splices (one-bar rise > %d%%) at bars %s; segments of %s bars"
        % (round((SPLICE - 1) * 100), cut[1:-1], [b - a for a, b in zip(cut, cut[1:])]))
    part_linear()
    say("")
    say("== B. Empirical: every start offset s, error vs the converged line y* at the same bar")
    summary = []
    for name, xs in corp.items():
        drift = float(np.mean(np.concatenate([np.diff(np.log(x)) for x in xs])))
        say("")
        say("##### %s: %d bars, mean log drift %.2e per bar" % (name, sum(len(x) for x in xs), drift))
        for N in PERIODS:
            res = study(xs, mcgd_step, RULES, N)
            report(name, res, RULES, N)
            if res["rules"] and stats(res["rules"]["S"]["first"]) is not None:
                w, nb = blocks(res, "T'", "S")
                rr = res["rules"]
                eS, eT = rr["S"]["first"], rr["T'"]["first"]
                both = np.isfinite(eS) & np.isfinite(eT)
                summary.append((name, N, stats(eS)[0], stats(eT)[0], stats(rr["W"]["first"])[0],
                                stats(rr["S"]["common"])[0], stats(rr["T'"]["common"])[0], w, nb,
                                float(np.mean(np.abs(eT[both]) < np.abs(eS[both])))))
    part_bias(ibm)
    part_hybrid(corp)
    part_tail(corp)
    part_lag(corp)
    say("")
    say("== E. Summary: rms relative error at the first output; T' vs S by offset block")
    say("  common = bar s+2N-2; offsets = share of single offsets where |e_T'| < |e_S|")
    say("  %-22s %-5s | %-9s %-9s %-9s | %-12s | %-9s %-9s | %-15s | %s" % (
        "corpus", "N", "S", "T'", "W", "T' vs S", "S@common", "T'@common", "T' better in", "offsets"))
    for name, N, s, t, w, sc, tc, wins, nb, share in summary:
        say("  %-22s %-5d | %.3e %.3e %.3e | %5.1f%% %-6s | %.3e %.3e | %2d of %d blocks | %.0f%%"
            % (name, N, s, t, w, abs(1 - t / s) * 100, "lower" if t < s else "higher",
               sc, tc, wins, nb, share * 100))
    with open(os.path.join(HERE, "results.txt"), "w") as f:
        f.write("\n".join(OUT) + "\n")


if __name__ == "__main__":
    sys.exit(main())
