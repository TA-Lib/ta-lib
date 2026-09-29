#!/usr/bin/env python3
"""Seed study for TA_VIDYA (#474), on the harness of ../mcgd-seeding.

Question: which seed, computed from bars s..s+m-1 only, gives the first
published value (bar s+m) closest to the value an implementation with unlimited
history prints at the same bar?

VIDYA's step needs a per-bar gain k_t = alpha*|CMOU_t|/100 besides the price,
so the step here takes the bar's gain, and study_one is this file's own.  The
corpora, the Rule record, the SMA, the statistics and the block count are the
MCGD study's, imported.

Run:  python3 seed_study.py      (writes results.txt; numpy only, no network)
"""

import importlib.util
import math
import os
import sys
import time

import numpy as np

sys.dont_write_bytecode = True    # the imported MCGD study is not ours to add a cache to
HERE = os.path.dirname(os.path.abspath(__file__))


def _load(name, rel):
    spec = importlib.util.spec_from_file_location(name, os.path.join(HERE, rel))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


H = _load("mcgd_seed_study", os.path.join("..", "mcgd-seeding", "seed_study.py"))
Rule, seq_mean, stats, blocks = H.Rule, H.seq_mean, H.stats, H.blocks
MIN_HISTORY, REF_AGREE, THRESHOLDS, CONV_HORIZON = \
    H.MIN_HISTORY, H.REF_AGREE, H.THRESHOLDS, H.CONV_HORIZON
LAG_HORIZON = 1000
TAIL_HORIZON = 4000
TAIL_TOL = 1e-14

OUT = []


def say(*a):
    line = " ".join(str(x) for x in a)
    OUT.append(line)
    print(line)


# --------------------------------------------------------------------------
# VIDYA
# --------------------------------------------------------------------------

class P:
    def __init__(self, n, m):
        self.n, self.m = n, m
        self.alpha = 2.0 / float(n + 1)

    def __str__(self):
        return "(%d,%d)" % (self.n, self.m)


def gains(x, p):
    """k_t for every bar, 0 before bar m: cmou.c's running sums, nullRun reset included."""
    m, alpha = p.m, p.alpha
    x = [float(v) for v in x]
    k = np.zeros(len(x))
    up = down = 0.0
    null_run = 0
    for t in range(1, len(x)):
        if t > m:
            d = x[t - m] - x[t - m - 1]
            if d > 0.0:
                up -= d
            elif d < 0.0:
                down += d
        d = x[t] - x[t - 1]
        if d > 0.0:
            up += d
        elif d < 0.0:
            down -= d
        null_run = null_run + 1 if d == 0.0 else 0
        if t > m and null_run >= m:
            null_run = m
            up = down = 0.0
        if t >= m:
            s = up + down
            k[t] = alpha * (math.fabs((100.0 * (up - down)) / s) / 100.0) if s > 0.0 else 0.0
    return k


def step(y, x, k):
    return ((x - y) * k) + y


def run_from(x, k, bar, y0):
    """The recursion over the whole series from value y0 held at `bar`."""
    out = np.full(len(x), np.nan)
    y = float(y0)
    out[bar] = y
    for i in range(bar + 1, len(x)):
        y = ((x[i] - y) * k[i]) + y
        out[i] = y
    return out


# Seeds: every eligible rule returns the line's value at bar s+m-1 from x[s..s+m-1].

def seed_ema(x, s, p):
    y = x[s].astype(float)
    for j in range(1, p.m):
        y = step(y, x[s + j], p.alpha)
    return y


def seed_adaptive(x, s, p, h=1):
    """SMA of x[s..s+h-1] (h = 1: x[s]), then VIDYA's own step over x[s+h..s+m-1],
    its CMO over the changes seen since s."""
    y = seq_mean(x, s, h)
    up = np.zeros(len(s))
    down = np.zeros(len(s))
    for j in range(1, p.m):
        d = x[s + j] - x[s + j - 1]
        up += np.where(d > 0, d, 0.0)
        down += np.where(d < 0, -d, 0.0)
        if j < h:
            continue
        tot = up + down
        with np.errstate(all="ignore"):
            k = np.where(tot > 0, p.alpha * (np.abs((100.0 * (up - down)) / tot) / 100.0), 0.0)
        y = step(y, x[s + j], k)
    return y


def hybrid(h):
    return Rule("H%d" % h, lambda p: p.m - 1, lambda p: p.m,
                lambda x, s, p, h=h: seed_adaptive(x, s, p, h), "")


RULES = [
    Rule("R", lambda p: p.m - 1, lambda p: p.m, lambda x, s, p: x[s + p.m - 1].astype(float),
         "x[s+m-1] (the proposal; KAMA's and LEAN's convention)"),
    Rule("S", lambda p: p.m - 1, lambda p: p.m, lambda x, s, p: seq_mean(x, s, p.m),
         "SMA of x[s..s+m-1] (pandas-ta-classic's SMA(x, L) at n = m = L)"),
    Rule("Sn", lambda p: p.m - 1, lambda p: p.m,
         lambda x, s, p: seq_mean(x, s + p.m - min(p.n, p.m), min(p.n, p.m)),
         "SMA of the last min(n, m) bars, x[s+m-min(n,m)..s+m-1]"),
    Rule("E", lambda p: p.m - 1, lambda p: p.m, seed_ema,
         "x[s], then a plain alpha-EMA step (V = 1) over x[s+1..s+m-1]"),
    Rule("X0", lambda p: p.m - 1, lambda p: p.m, lambda x, s, p: x[s].astype(float),
         "x[s], held (k = 0) until the first CMOU at s+m (trading-signals 8.3.0, model)"),
    Rule("A", lambda p: p.m - 1, lambda p: p.m, seed_adaptive,
         "x[s], then VIDYA's step with the CMO of the j <= m-1 changes seen so far"),
    Rule("W", lambda p: p.m - 1, lambda p: p.m + p.n, lambda x, s, p: x[s + p.m - 1].astype(float),
         "R published n bars later, lookback m+n (information only, not eligible)"),
]
ELIGIBLE = ["R", "S", "Sn", "E", "X0", "A"]


# --------------------------------------------------------------------------
# harness (MCGD's study_one with a per-bar gain)
# --------------------------------------------------------------------------

def reference(x, k, p):
    """y* = the recursion from bar 0 (x[0] held until the first CMOU).  Trusted from
    MIN_HISTORY on where two other runs agree to REF_AGREE: x[100] held at bar 100, and
    the SMA of bars 0..m-1 at bar m-1."""
    ref = run_from(x, k, 0, x[0])
    alts = [run_from(x, k, 100, x[100]), run_from(x, k, p.m - 1, x[:p.m].sum() / p.m)]
    ok = np.zeros(len(x), bool)
    ok[MIN_HISTORY:] = np.logical_and.reduce(
        [np.abs(a - ref)[MIN_HISTORY:] <= REF_AGREE * np.abs(ref)[MIN_HISTORY:] for a in alts])
    return ref, ok


def study_one(x, rules, p):
    n = len(x)
    common = 2 * p.m
    last = max([common] + [r.publish(p) for r in rules])
    res = {"offsets": 0, "ref_excluded": 0, "common": common, "conv_n": 0, "lag_n": 0, "rules": {}}
    starts = np.arange(MIN_HISTORY, n - last)
    if len(starts) == 0:
        return res
    k = gains(x, p)
    ref, ok = reference(x, k, p)
    res["offsets"] = len(starts)
    res["ref_excluded"] = int((~ok[MIN_HISTORY:]).sum())
    bad = np.concatenate([[0], np.cumsum(~ok)])
    hi = starts + last + CONV_HORIZON
    conv_ok = hi <= n - 1
    conv_ok[conv_ok] = bad[hi[conv_ok] + 1] - bad[starts[conv_ok]] == 0
    res["conv_n"] = int(conv_ok.sum())
    lag_ok = starts + p.m + LAG_HORIZON <= n - 1
    res["lag_n"] = int(lag_ok.sum())
    for rule in rules:
        k0, P0 = rule.seed_bar(p), rule.publish(p)
        y = rule.seed(x, starts, p).astype(float)
        e_first = np.full(len(starts), np.nan)
        e_common = np.full(len(starts), np.nan)
        lag_ss = np.zeros(LAG_HORIZON)
        lag_n = np.zeros(LAG_HORIZON)
        last_above = {t: np.full(len(starts), P0 - 1) for t in THRESHOLDS}
        kmax = min(n - 1 - starts[0], max(P0, common) + CONV_HORIZON)
        for kk in range(k0 + 1, kmax + 1):
            mm = int(np.searchsorted(starts, n - kk))
            bar = starts[:mm] + kk
            y[:mm] = step(y[:mm], x[bar], k[bar])
            if kk < min(P0, common):
                continue
            e = np.where(ok[bar], (y[:mm] - ref[bar]) / ref[bar], np.nan)
            if kk == P0:
                e_first[:mm] = e
            if kk == common:
                e_common[:mm] = e
            j = kk - p.m
            if 0 <= j < LAG_HORIZON:
                sel = lag_ok[:mm] & ok[bar]
                lag_ss[j] += float(np.sum(e[sel] ** 2))
                lag_n[j] += int(sel.sum())
            if kk >= P0:
                ae = np.abs(e)
                for t in THRESHOLDS:
                    last_above[t][:mm][ae > t] = kk
        res["rules"][rule.name] = {
            "first": e_first, "common": e_common, "lag_ss": lag_ss, "lag_n": lag_n,
            "conv": {t: (last_above[t] + 1 - P0)[conv_ok] for t in THRESHOLDS},
            "censored": {t: int((conv_ok & (last_above[t] >= P0 + CONV_HORIZON)).sum())
                         for t in THRESHOLDS},
        }
    return res


def study(xs, rules, p):
    parts = [study_one(x, rules, p) for x in xs]
    out = {"offsets": sum(q["offsets"] for q in parts),
           "ref_excluded": sum(q["ref_excluded"] for q in parts),
           "common": 2 * p.m, "conv_n": sum(q["conv_n"] for q in parts),
           "lag_n": sum(q["lag_n"] for q in parts), "rules": {}}
    for r in rules:
        ps = [q["rules"][r.name] for q in parts if r.name in q["rules"]]
        if not ps:
            continue
        out["rules"][r.name] = {
            "first": np.concatenate([q["first"] for q in ps]),
            "common": np.concatenate([q["common"] for q in ps]),
            "lag_ss": sum(q["lag_ss"] for q in ps), "lag_n": sum(q["lag_n"] for q in ps),
            "conv": {t: np.concatenate([q["conv"][t] for q in ps]) for t in THRESHOLDS},
            "censored": {t: sum(q["censored"][t] for q in ps) for t in THRESHOLDS},
        }
    if out["rules"]:
        keep = np.logical_and.reduce([np.isfinite(r["first"]) & np.isfinite(r["common"])
                                      for r in out["rules"].values()])
        out["compared"] = int(keep.sum())
        for r in out["rules"].values():
            for key in ("first", "common"):
                r[key] = np.where(keep, r[key], np.nan)
    return out


def report(title, res, rules, p):
    say("")
    say("=== %s  (n,m)=%s: %d offsets; reference not converged on %d bars (excluded); "
        "%d offsets compared; common bar s+%d"
        % (title, p, res["offsets"], res["ref_excluded"], res.get("compared", 0), res["common"]))
    if not res["rules"] or stats(res["rules"][rules[0].name]["first"]) is None:
        say("  no offset has a converged reference")
        return False
    say("  relative error (y - y*)/y*")
    say("  %-3s %-6s | %-9s %-10s %-9s %-9s | %-9s %-10s"
        % ("", "first", "rms", "mean", "p95", "max", "rms@cmn", "mean@cmn"))
    for rule in rules:
        r = res["rules"][rule.name]
        f, c = stats(r["first"]), stats(r["common"])
        say("  %-3s s+%-4d | %-9.3e %+-10.3e %-9.3e %-9.3e | %-9.3e %+-10.3e"
            % (rule.name, rule.publish(p), f[0], f[1], f[2], f[3], c[0], c[1]))
    if res["conv_n"] == 0:
        return True
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
    return True


# --------------------------------------------------------------------------
# grid and parts
# --------------------------------------------------------------------------

def grid():
    pairs = [(12, 9)]
    pairs += [(n, (3 * n + 2) // 4) for n in (2, 5, 10, 14, 20, 30, 50)]
    pairs += [(v, v) for v in (2, 9, 14, 30)]
    pairs += [(30, 9), (9, 30)]
    seen, out = set(), []
    for q in pairs:
        if q not in seen:
            seen.add(q)
            out.append(P(*q))
    return out


def synthetic():
    return np.array([100 + 10 * math.sin(i / 7) + 0.15 * i + 2 * math.sin(i / 2.3) for i in range(1200)])


def validate():
    say("== 0. Validation of the harness's gain and step: rule R from bar 0 at (12,9) against")
    say("   #474's 60-digit goldens (1200-bar synthetic series)")
    c = synthetic()
    p = P(12, 9)
    k = gains(c, p)
    y = run_from(c, k, p.m - 1, c[p.m - 1])
    gold = [(9, 109.6246782630551), (10, 109.61069647274289), (11, 109.61716494551899),
            (20, 110.01780316159493), (50, 109.13996673831133), (100, 120.18604396370417),
            (251, 132.94416445913191), (500, 181.7499950535472), (800, 223.84149738801602),
            (1000, 243.88694553681219), (1199, 284.69011624413429)]
    worst = 0.0
    for bar, v in gold:
        d = abs(y[bar] - v) / v
        worst = max(worst, d)
        say("  bar %-5d %.17g  golden %.17g  rel %.2e" % (bar, y[bar], v, d))
    say("  max rel over the %d goldens: %.2e" % (len(gold), worst))
    say("  #474 non-vacuity rows, relative move against R at (12,9):")
    s = run_from(c, k, p.m - 1, c[:p.m].sum() / p.m)
    x0 = run_from(c, k, 0, c[0])
    for name, o in (("SMA(9) seed", s), ("x[0] seed", x0)):
        say("    %-12s bars 9/10/11/100: %.1e / %.1e / %.1e / %.1e" % (
            (name,) + tuple(abs(o[b] - y[b]) / y[b] for b in (9, 10, 11, 100))))
    say("  #474 M5 (trading-signals and pandas-ta-classic measured against R at n = m = P):")
    say("  rules X0 and S against R here, max rel over bars >= P / >= 100 / >= 200")
    for v in (9, 12, 14):
        p = P(v, v)
        k = gains(c, p)
        r = run_from(c, k, v - 1, c[v - 1])
        cells = []
        for name, o in (("X0", run_from(c, k, 0, c[0])), ("S", run_from(c, k, v - 1, c[:v].sum() / v))):
            cells.append("%s %s" % (name, " / ".join(
                "%.1e" % float(np.max(np.abs(o[b:] - r[b:]) / r[b:])) for b in (v, 100, 200))))
        say("    P=%-3d %s" % (v, "; ".join(cells)))


def part_gain(corp, ps):
    say("")
    say("== C. Gain and memory.  mean |CMOU|/100 = mean k/alpha, and the memory: bars back until")
    say("   prod(1-k) < 1e-12, p50 / p95 / max over bars >= %d (EMA(n) alone: ln(1e-12)/ln(1-alpha))"
        % MIN_HISTORY)
    for name in ("IBM", "gData segments"):
        for p in ps:
            v, mem = [], []
            for x in corp[name]:
                if len(x) <= MIN_HISTORY:
                    continue
                k = gains(x, p)
                v.append(k[MIN_HISTORY:] / p.alpha)
                cl = np.concatenate([[0.0], np.cumsum(np.log1p(-k))])
                t = np.arange(MIN_HISTORY, len(x)) + 1
                # smallest b with cl[t] - cl[t-b] < ln(1e-12); cl is non-increasing
                target = cl[t] - math.log(1e-12)
                j = np.searchsorted(-cl, -target, side="left")
                b = np.where(j > 0, t - (j - 1), 10 ** 9)
                mem.append(b)
            v, mem = np.concatenate(v), np.concatenate(mem)
            fin = mem[mem < 10 ** 9]
            past = int((mem >= 10 ** 9).sum())
            say("  %-15s %-9s mean k/alpha %.3f | memory %s | %d of %d bars reach back past their series' bar 0 | EMA(n) %d" % (
                name, p, float(v.mean()),
                "%5d / %5d / %5d bars" % (np.percentile(fin, 50), np.percentile(fin, 95), fin.max())
                if len(fin) else "  none within the data   ",
                past, len(mem), math.ceil(math.log(1e-12) / math.log(1 - p.alpha))))


def part_hybrid(corp, ps):
    say("")
    say("== D. Hybrid seeds H(h): SMA of the first h bars, then VIDYA's step with the CMO of the")
    say("   changes since s, to s+m-1 (H1 = A, Hm = S).  rms relative error at s+m, over offsets")
    say("   with a converged reference at s+m and s+2m, so H1 and Hm can differ slightly from E's A and S.")
    for name in ("IBM", "gData segments"):
        for p in ps:
            hs = sorted(h for h in {1, 2, 3, p.m // 4, p.m // 2, 3 * p.m // 4, p.m} if 1 <= h <= p.m)
            res = study(corp[name], [hybrid(h) for h in hs], p)
            if stats(res["rules"]["H1"]["first"]) is None:
                continue
            say("  %-15s %-9s %s" % (name, p, "  ".join(
                "H%d %.3e" % (h, stats(res["rules"]["H%d" % h]["first"])[0]) for h in hs)))


def verdict_rule():
    return next(r for r in RULES if r.name == VERDICT)


def part_lag(results, ps):
    say("")
    say("== G. %s against R and S at later bars: rms(%s)/rms(other) at j bars after the first output" % (VERDICT, VERDICT))
    say("   (the other's rms in brackets), over offsets with %d bars to follow" % LAG_HORIZON)
    for name in ("IBM", "gData segments"):
        for p in ps:
            res = results[(name, str(p))]
            if not res["rules"] or res["rules"][VERDICT]["lag_n"].min() == 0:
                if res["rules"]:
                    say("  %-15s %-9s skipped: at some j no offset with %d bars to follow has a converged reference" % (name, p, LAG_HORIZON))
                continue
            rv = res["rules"][VERDICT]
            v = np.sqrt(rv["lag_ss"] / np.maximum(rv["lag_n"], 1))
            for other in ("R", "S"):
                if other == VERDICT:
                    continue
                ro = res["rules"][other]
                o = np.sqrt(ro["lag_ss"] / np.maximum(ro["lag_n"], 1))
                live = o > 1e-12
                cross = np.where((v > o) & live)[0]
                with np.errstate(all="ignore"):
                    worst = int(np.argmax(np.where(live, v / o, 0)))
                js = [j for j in (0, p.m, 2 * p.m, 4 * p.m, 8 * p.m, 16 * p.m, 32 * p.m) if j < LAG_HORIZON]
                say("  %-15s %-9s vs %-2s (%d to %d offsets) first j where %s is worse: %s; max ratio %.2f at j=%d (%s rms %.0e)" % (
                    name, p, other, rv["lag_n"].min(), rv["lag_n"].max(), VERDICT,
                    "j=%d, %s rms %.1e" % (cross[0], other, o[cross[0]]) if len(cross) else "none",
                    v[worst] / o[worst], worst, other, o[worst]))
                say("      " + "  ".join("j=%d %.2f (%.0e)" % (j, v[j] / o[j], o[j]) for j in js if o[j] > 0))


ORACLES = [("trading-signals (X0)", "X0"), ("pandas-ta-classic (S)", "S"), ("LEAN seed (R)", "R")]


def rule(name):
    return next(r for r in RULES if r.name == name)


def part_tail(corp):
    say("")
    say("== F. Where each oracle's seed meets %s, and R for comparison: bars after the first output" % VERDICT)
    say("   until |oracle - target| / |target| stays below %g.  Seeds only: every line runs on CMOU" % TAIL_TOL)
    say("   and TA-Lib's step (LEAN's Wilder CMO keeps it apart whatever the seed, #474 M2).")
    say("   n = m, the oracles' one period.  Cells: vs %s | vs R; 0 = identical from the first output." % VERDICT)
    ibm = corp["IBM"][0]
    for v in (2, 9, 12, 14, 30):
        p = P(v, v)
        k = gains(ibm, p)
        starts = np.arange(0, len(ibm) - p.m - TAIL_HORIZON)
        seeds = {r.name: r.seed(ibm, starts, p).astype(float) for r in RULES if r.name in (VERDICT, "X0", "S", "R")}
        cells = []
        for label, rn in ORACLES:
            parts = []
            for target in (VERDICT, "R"):
                if rn == target:
                    parts.append("0")
                    continue
                a, b = seeds[rn].copy(), seeds[target].copy()
                last = np.zeros(len(starts), int)
                for j in range(TAIL_HORIZON):
                    bar = starts + p.m + j
                    a = step(a, ibm[bar], k[bar])
                    b = step(b, ibm[bar], k[bar])
                    last[np.abs(a - b) > TAIL_TOL * np.abs(b)] = j + 1
                cz = int((last >= TAIL_HORIZON).sum())
                parts.append("%d / %d%s" % (np.percentile(last, 50), last.max(), " (c%d)" % cz if cz else ""))
            cells.append("%s %s" % (label, " | ".join(parts)))
        say("  IBM %-9s p50 / max over %d offsets: %s" % (p, len(starts), "; ".join(cells)))
    c = synthetic()
    s0 = np.array([0])
    for (n, m) in ((9, 9), (12, 12), (14, 14), (30, 30), (12, 9)):
        p = P(n, m)
        k = gains(c, p)
        runs = {rn: run_from(c, k, p.m - 1, rule(rn).seed(c, s0, p)[0]) for rn in (VERDICT, "X0", "S", "R")}
        cells = []
        for label, rn in ORACLES:
            parts = []
            for target in (VERDICT, "R"):
                if rn == target:
                    parts.append("identical")
                    continue
                d = np.abs(runs[rn] - runs[target]) / np.abs(runs[target])
                badb = np.where(d[p.m:] > TAIL_TOL)[0]
                at = p.m + badb[-1] + 1 if len(badb) else p.m
                parts.append("bar %d" % at if at < len(c) else "never (last %.1e)" % d[-1])
            cells.append("%s %s" % (label, " | ".join(parts)))
        say("  #474 synthetic %-9s first output bar %d, from: %s; %s vs R at bar %d / 100: %.1e / %.1e" % (
            p, p.m, "; ".join(cells), VERDICT, p.m,
            abs(runs[VERDICT][p.m] - runs["R"][p.m]) / runs["R"][p.m],
            abs(runs[VERDICT][100] - runs["R"][100]) / runs["R"][100]))


def part_tails_of_error(summary):
    say("")
    say("== I. p95 / max of |relative error| at the first output, R / S / %s" % VERDICT)
    for name, p, row, rowc, best, cmp, compared, excl, res in summary:
        cells = []
        for r in ("R", "S", VERDICT):
            st = stats(res["rules"][r]["first"])
            cells.append("%s %.3e / %.3e" % (r, st[2], st[3]))
        say("  %-22s %-9s %s" % (name, p, " | ".join(cells)))


VERDICT = "A"


def main():
    t0 = time.time()
    corp, g, ibm, cut = H.corpora()
    validate()
    say("")
    say("== Rules (seed value at bar s+m-1, first output s+m unless noted)")
    for r in RULES:
        say("  %-3s %s" % (r.name, r.note))
    say("  gData splices at bars %s; segments of %s bars"
        % (cut[1:-1], [b - a for a, b in zip(cut, cut[1:])]))
    ps = grid()
    say("  (n,m) grid: %s" % " ".join(str(p) for p in ps))
    part_gain(corp, ps)
    say("")
    say("== B. Empirical: every start offset s, error vs the converged line y* at the same bar")
    summary, results = [], {}
    for name, xs in corp.items():
        say("")
        say("##### %s: %d bars" % (name, sum(len(x) for x in xs)))
        for p in ps:
            res = study(xs, RULES, p)
            results[(name, str(p))] = res
            if not report(name, res, RULES, p):
                continue
            rr = res["rules"]
            row = {r: stats(rr[r]["first"])[0] for r in ELIGIBLE + ["W"]}
            rowc = {r: stats(rr[r]["common"])[0] for r in ELIGIBLE}
            best = min(ELIGIBLE, key=lambda r: row[r])
            ev = rr[VERDICT]["first"]
            cmp = {}
            for o in ("R", "S"):
                eo = rr[o]["first"]
                both = np.isfinite(ev) & np.isfinite(eo)
                cmp[o] = (blocks(res, VERDICT, o), float(np.mean(np.abs(ev[both]) < np.abs(eo[both]))))
            summary.append((name, p, row, rowc, best, cmp, res["compared"], res["ref_excluded"], res))
    part_hybrid(corp, ps)
    part_lag(results, ps)
    part_tail(corp)
    say("")
    say("== E. Summary: rms relative error at the first output s+m (W at s+m+n); best eligible rule;")
    say("   %s vs R and vs S: rms change, blocks of 10 where %s's rms is lower, share of single offsets" % (VERDICT, VERDICT))
    say("  %-22s %-9s | %s | %-4s | %-38s | %-38s | %s" % (
        "corpus", "(n,m)", " ".join("%-9s" % r for r in ELIGIBLE + ["W"]), "best",
        "%s vs R" % VERDICT, "%s vs S" % VERDICT, "rms@s+2m: " + " ".join(ELIGIBLE)))
    for name, p, row, rowc, best, cmp, compared, excl, res in summary:
        cells = []
        for o in ("R", "S"):
            (w, nb), share = cmp[o]
            t, s = row[VERDICT], row[o]
            cells.append("%5.1f%% %-6s %2d/%d blk, %3.0f%% offs" % (
                abs(1 - t / s) * 100, "lower" if t < s else "higher", w, nb, share * 100))
        say("  %-22s %-9s | %s | %-4s | %-38s | %-38s | %s" % (
            name, p, " ".join("%.3e" % row[r] for r in ELIGIBLE + ["W"]), best, cells[0], cells[1],
            " ".join("%.2e" % rowc[r] for r in ELIGIBLE)))
    say("")
    say("== H. Exclusions: offsets compared / offsets, bars >= %d with no converged reference" % MIN_HISTORY)
    for name, p, row, rowc, best, cmp, compared, excl, res in summary:
        say("  %-22s %-9s %6d / %-6d  %d bars" % (name, p, compared, res["offsets"], excl))
    part_tails_of_error(summary)
    with open(os.path.join(HERE, "results.txt"), "w") as f:
        f.write("\n".join(OUT) + "\n")
    print("runtime %.1f s" % (time.time() - t0))


if __name__ == "__main__":
    sys.exit(main())
