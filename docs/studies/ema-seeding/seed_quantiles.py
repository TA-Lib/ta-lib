#!/usr/bin/env python3
"""Section 13: the spread of a single EMA's first-published-value error, per seed.

For every data-start offset t0, both seeds publish first at bar t0+p-1:
  SMA_SEED        seed mean(x[t0..t0+p-1]), summed in order as TA_EMA does
  FIRST_BAR_SEED  seed x[t0], then p-1 EMA steps
Error = that value minus the EMA run from bar 0 of the series, as a percentage of
the series' population sd (section 5's normalisation). Offsets run from
max(400, the bar where the bar-0 run's weight on its own first sample is below
1e-9) up to n-p-3, so at p <= 30 the range is section 5's own and its rms
columns reproduce. Percentiles of |error| interpolate linearly between order
statistics. Synthetic kinds pool 10 series of 10000 bars, each normalised by its
own sd.

Section 14 appends DEMA, TEMA, TRIX and MACD(12,26,9) on gData, IBM and the random
walk; its header in quantiles.txt states the method.

Pure standard library. Run:  python3 seed_quantiles.py   (writes quantiles.txt)
"""

import csv
import functools
import math
import os
import random
import re

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.environ.get("TA_REPO") or os.path.abspath(os.path.join(HERE, "..", "..", ".."))

PERIODS = (5, 10, 14, 20, 26, 30, 50, 100, 200)
QUANTS = (0.10, 0.25, 0.50, 0.75, 0.90)
SERIES, BARS = 10, 10000

# Section 5's rms at its first published bar: (e_S first out, e_T @common).
SECTION5 = {5: (1.401, 0.849), 26: (3.092, 1.652), 30: (3.293, 1.756)}

OUT = []


def say(line=""):
    OUT.append(line)
    print(line)


def gdata_close():
    src = open(os.path.join(REPO, "src", "tools", "ta_regtest", "ta_gDataClose.c")).read()
    body = re.search(r"gDataClose\s*\[[^\]]*\]\s*=\s*\{([^}]*)\}", src).group(1)
    return [float(v) for v in re.split(r"[,\s]+", body) if v]


def ibm_close():
    with open(os.path.join(HERE, "data", "ibm_daily_ohlc.csv")) as f:
        return [float(r["close"]) for r in csv.DictReader(f)]


def random_walk(seed):
    rng, x, v = random.Random(seed), [], 100.0
    for _ in range(BARS):
        x.append(v)
        v *= math.exp(rng.gauss(0.0, 0.01))
    return x


def ar1(seed, phi=0.9, level=100.0, sd=1.0):
    rng = random.Random(seed)
    v = level + rng.gauss(0.0, sd / math.sqrt(1.0 - phi * phi))
    x = []
    for _ in range(BARS):
        x.append(v)
        v = level + phi * (v - level) + rng.gauss(0.0, sd)
    return x


def white_noise(seed, level=100.0, sd=1.0):
    rng = random.Random(seed)
    return [level + rng.gauss(0.0, sd) for _ in range(BARS)]


def pop_sd(x):
    m = sum(x) / len(x)
    return math.sqrt(sum((v - m) ** 2 for v in x) / len(x))


def first_offset(p):
    a = 2.0 / (p + 1.0)
    return max(400, math.ceil(math.log(1e-9) / math.log(1.0 - a)))


def errors(x, p):
    """|error| in % of sd for both seeds, one entry per start offset."""
    a = 2.0 / (p + 1.0)
    sd = pop_sd(x)
    truth, prev = [x[0]], x[0]
    for v in x[1:]:
        prev += (v - prev) * a
        truth.append(prev)
    eS, eF = [], []
    for t0 in range(first_offset(p), len(x) - p - 2):
        s = x[t0]
        f = x[t0]
        for i in range(t0 + 1, t0 + p):
            v = x[i]
            s += v
            f += (v - f) * a
        s /= p
        y = truth[t0 + p - 1]
        eS.append(abs(s - y) / sd * 100.0)
        eF.append(abs(f - y) / sd * 100.0)
    return eS, eF


def quantile(sorted_v, q):
    pos = q * (len(sorted_v) - 1)
    i = int(pos)
    j = min(i + 1, len(sorted_v) - 1)
    return sorted_v[i] + (sorted_v[j] - sorted_v[i]) * (pos - i)


def summary(e):
    v = sorted(e)
    return [quantile(v, q) for q in QUANTS] + [math.sqrt(sum(d * d for d in v) / len(v)), len(v)]


def block(name, series):
    say("-- %s" % name)
    say("     p  seed           |    p10      p25      p50      p75      p90  |    rms   | offsets")
    rows = {}
    for p in PERIODS:
        eS, eF = [], []
        for x in series:
            s, f = errors(x, p)
            eS += s
            eF += f
        for seed, e in (("SMA_SEED", eS), ("FIRST_BAR_SEED", eF)):
            r = summary(e)
            rows[(p, seed)] = r
            say("   %3d  %-14s | %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  | %7.4f  | %d" % ((p, seed) + tuple(r)))
    say()
    return rows


# ---------------------------------------------------------------------------
# Section 14: DEMA, TEMA, TRIX and MACD(12,26,9)
# ---------------------------------------------------------------------------

CHAINED = (("DEMA", 2), ("TEMA", 3), ("TRIX", 3))
MACD_P = (12, 26, 9)
MACD_OUTS = ("line", "signal", "hist")
SECTION11_P = (5, 12, 26, 30)
SECTION11_LO = 500
# Section 11's printed rms S / rms N at p = 5, 12, 26, 30: (low, high), or MACD's three outputs.
SECTION11 = {
    "gData": {"DEMA": (1.303, 1.569), "TEMA": (1.213, 1.275), "TRIX": (1.368, 1.600),
              "MACD": (1.961, 2.088, 2.155)},
    "IBM": {"DEMA": (1.338, 1.592), "TEMA": (1.254, 1.431), "TRIX": (0.996, 1.126),
            "MACD": (1.945, 2.094, 2.268)},
}


def ema_factors(p):
    beta = (p - 1) / (p + 1)
    k = 1.0 - beta
    if beta < 0.5:
        beta = 1.0 - k
    return k, beta


def seq_mean(v):
    s = 0.0
    for e in v:
        s += e
    return s / len(v)


def run(feed, p, start, seed):
    """One stage seeded with `seed` on bar `start`, stepped as the library steps it."""
    k, beta = ema_factors(p)
    out = [None] * len(feed)
    prev = out[start] = seed
    for i in range(start + 1, len(feed)):
        prev = k * feed[i] + beta * prev
        out[i] = prev
    return out


def converged_stages(x, p, n):
    stages, feed = [], x
    for _ in range(n):
        feed = run(feed, p, 0, feed[0])
        stages.append(feed)
    return stages


def seeded_stages(x, p, n, first_bar):
    """Stages 1..n of a call whose array is x. Stage j's SMA_SEED window starts on bar
    (j-1)(p-1); FIRST_BAR_SEED seeds the stage there with its feeder's value."""
    stages, feed = [], x
    for j in range(n):
        w0 = j * (p - 1)
        if first_bar:
            feed = run(feed, p, w0, feed[w0])
        else:
            feed = run(feed, p, w0 + p - 1, seq_mean(feed[w0:w0 + p]))
        stages.append(feed)
    return stages


def lookback(name, p):
    return {"DEMA": 2 * (p - 1), "TEMA": 3 * (p - 1), "TRIX": 3 * (p - 1) + 1}[name]


def chained_value(name, st, b):
    if name == "DEMA":
        return (2.0 * st[0][b]) - st[1][b]
    if name == "TEMA":
        return st[2][b] + ((3.0 * st[0][b]) - (3.0 * st[1][b]))
    return ((st[2][b] / st[2][b - 1]) - 1.0) * 100.0


def macd_stages(x, first_bar):
    """(line, signal, hist) of MACD(12,26,9) over a call whose array is x."""
    f, s, g = MACD_P
    if first_bar:
        fast, slow = run(x, f, 0, x[0]), run(x, s, 0, x[0])
    else:
        fast = run(x, f, s - 1, seq_mean(x[s - f:s]))
        slow = run(x, s, s - 1, seq_mean(x[0:s]))
    line = [None if a is None or b is None else a - b for a, b in zip(fast, slow)]
    if first_bar:
        sig = run(line, g, s - 1, line[s - 1])
    else:
        sig = run(line, g, s + g - 2, seq_mean(line[s - 1:s + g - 1]))
    hist = [None if a is None or b is None else a - b for a, b in zip(line, sig)]
    return line, sig, hist


def macd_converged(x):
    f, s, g = MACD_P
    fast, slow = run(x, f, 0, x[0]), run(x, s, 0, x[0])
    line = [a - b for a, b in zip(fast, slow)]
    sig = run(line, g, 0, line[0])
    return line, sig, [a - b for a, b in zip(line, sig)], (fast, slow, sig)


def settle_bar(stages):
    """First bar from which every stage's weight on its bar-0 seed stays below 1e-9."""
    last = 0
    for st in stages:
        for t, w in enumerate(st):
            if abs(w) >= 1e-9:
                last = max(last, t)
    return last + 1


@functools.lru_cache(maxsize=None)
def chained_first_offset(n_stages, p, nb):
    imp = [1.0] + [0.0] * (nb - 1)
    return max(400, settle_bar(converged_stages(imp, p, n_stages)))


def macd_first_offset(nb):
    return max(400, settle_bar(macd_converged([1.0] + [0.0] * (nb - 1))[3]))


def chained_errors(x, p, offsets_of, sd_of):
    """|error| per function and seed at each function's first published bar, % of
    sd_of(name, converged output); offsets_of(name) gives the start offsets."""
    full = converged_stages(x, p, 3)
    nb = len(x)
    truth, rng, scale = {}, {}, {}
    for name, _ in CHAINED:
        truth[name] = [None] + [chained_value(name, full, t) for t in range(1, nb)]
        rng[name] = offsets_of(name)
        scale[name] = 100.0 / sd_of(name, truth[name])
    errs = {(name, seed): [] for name, _ in CHAINED for seed in ("SMA_SEED", "FIRST_BAR_SEED")}
    lo = min(r.start for r in rng.values())
    hi = max(r.stop for r in rng.values())
    for t0 in range(lo, hi):
        here = [(name, n) for name, n in CHAINED if t0 in rng[name]]
        if not here:
            continue
        win = x[t0:t0 + max(lookback(name, p) for name, _ in here) + 1]
        for seed, fb in (("SMA_SEED", False), ("FIRST_BAR_SEED", True)):
            st = seeded_stages(win, p, max(n for _, n in here), fb)
            for name, _ in here:
                b = lookback(name, p)
                errs[(name, seed)].append(abs(chained_value(name, st, b) - truth[name][t0 + b]) * scale[name])
    return errs


def macd_errors(x, offsets, sd_line):
    line, sig, hist, _ = macd_converged(x)
    truth = (line, sig, hist)
    scale = 100.0 / sd_line(line)
    L = MACD_P[1] + MACD_P[2] - 2
    errs = {(o, seed): [] for o in MACD_OUTS for seed in ("SMA_SEED", "FIRST_BAR_SEED")}
    for t0 in offsets:
        win = x[t0:t0 + L + 1]
        for seed, fb in (("SMA_SEED", False), ("FIRST_BAR_SEED", True)):
            out = macd_stages(win, fb)
            for oi, o in enumerate(MACD_OUTS):
                errs[(o, seed)].append(abs(out[oi][L] - truth[oi][t0 + L]) * scale)
    return errs


def row(label, seed, r, extra=""):
    say("   %-6s %-14s | %7.4f  %7.4f  %7.4f  %7.4f  %7.4f  | %7.4f  | %d%s" % ((label, seed) + tuple(r) + (extra,)))


def chained_block(cname, series):
    """Every period, every chained function, pooled over `series`."""
    nb = len(series[0])
    res, firsts = {name: {} for name, _ in CHAINED}, {}
    for p in PERIODS:
        pooled = {}
        for x in series:
            def offsets_of(name, p=p, x=x):
                lo = chained_first_offset(dict(CHAINED)[name], p, len(x))
                return range(lo, len(x) - lookback(name, p) - 3)

            def sd_of(name, out):
                return pop_sd(out[offsets_of(name).start:])

            for k, v in chained_errors(x, p, offsets_of, sd_of).items():
                pooled.setdefault(k, []).extend(v)
        for name, n in CHAINED:
            firsts[(name, p)] = chained_first_offset(n, p, nb)
            for seed in ("SMA_SEED", "FIRST_BAR_SEED"):
                res[name][(p, seed)] = summary(pooled[(name, seed)])
    for name, n in CHAINED:
        say("-- %s, %s" % (name, cname))
        say("   p      seed           |    p10      p25      p50      p75      p90  |    rms   | offsets, from")
        for p in PERIODS:
            for seed in ("SMA_SEED", "FIRST_BAR_SEED"):
                row(str(p), seed, res[name][(p, seed)], ", from %d" % firsts[(name, p)])
        say()
    return res


def macd_block(cname, series):
    lo = macd_first_offset(len(series[0]))
    L = MACD_P[1] + MACD_P[2] - 2
    pooled = {}
    for x in series:
        for k, v in macd_errors(x, range(lo, len(x) - L - 3), lambda line: pop_sd(line[lo:])).items():
            pooled.setdefault(k, []).extend(v)
    say("-- MACD(12,26,9), %s, %% of the converged line's sd" % cname)
    say("   out    seed           |    p10      p25      p50      p75      p90  |    rms   | offsets, from")
    res = {}
    for o in MACD_OUTS:
        for seed in ("SMA_SEED", "FIRST_BAR_SEED"):
            res[(o, seed)] = summary(pooled[(o, seed)])
            row(o, seed, res[(o, seed)], ", from %d" % lo)
    say()
    return res


def section11_check(cname, key, x):
    """Section 11's offsets (500 to n-K-1, K = first bar + 3p + 2) and normalisation."""
    nb, lo = len(x), SECTION11_LO
    say("-- check: %s rms SMA_SEED / rms FIRST_BAR_SEED against section 11 (its offsets and sds)" % cname)
    for name, n in CHAINED:
        ratios = []
        for p in SECTION11_P:
            K = lookback(name, p) + 3 * p + 2
            e = chained_errors(x, p, lambda nm, K=K: range(lo, nb - K),
                               lambda nm, out: pop_sd(out[lo + 1:]))
            rS = summary(e[(name, "SMA_SEED")])[5]
            rN = summary(e[(name, "FIRST_BAR_SEED")])[5]
            ratios.append(rS / rN)
            say("   %-4s p %2d  rms S %7.3f%%  rms N %7.3f%%  S/N %.3f" % (name, p, rS, rN, rS / rN))
        say("   %-4s S/N %.3f to %.3f (section 11: %.3f to %.3f)"
            % ((name, min(ratios), max(ratios)) + SECTION11[key][name]))
    L = MACD_P[1] + MACD_P[2] - 2
    e = macd_errors(x, range(lo, nb - (L + 3 * MACD_P[1] + 2)), lambda line: pop_sd(line[lo:]))
    got = []
    for o in MACD_OUTS:
        rS, rN = summary(e[(o, "SMA_SEED")])[5], summary(e[(o, "FIRST_BAR_SEED")])[5]
        got.append(rS / rN)
        say("   MACD %-6s  rms S %7.3f%%  rms N %7.3f%%  S/N %.3f" % (o, rS, rN, rS / rN))
    say("   MACD S/N %.3f, %.3f, %.3f (section 11: %.3f, %.3f, %.3f)" % (tuple(got) + SECTION11[key]["MACD"]))
    say()


def ratio_block(name_rows):
    say("-- rms SMA_SEED / rms FIRST_BAR_SEED at the first published bar")
    say("   func   p    " + "  ".join("%-11s" % c for c, _ in name_rows))
    for name, _ in CHAINED:
        for p in PERIODS:
            say("   %-4s %4d    " % (name, p) + "  ".join(
                "%-11.3f" % (r[name][(p, "SMA_SEED")][5] / r[name][(p, "FIRST_BAR_SEED")][5]) for _, r in name_rows))
    for o in MACD_OUTS:
        say("   MACD %-6s  " % o + "  ".join(
            "%-11.3f" % (r["MACD"][(o, "SMA_SEED")][5] / r["MACD"][(o, "FIRST_BAR_SEED")][5]) for _, r in name_rows))
    say()


def chained_main(g, ibm):
    say("=" * 86)
    say("Section 14: DEMA, TEMA, TRIX and MACD(12,26,9), |error| of the first published value")
    say("against the function with every stage run from bar 0 of the series, at each start")
    say("offset t0. SMA_SEED: each stage seeded with the mean of its first p inputs. FIRST_BAR_SEED")
    say("(a call whose array starts at t0): the first stage, and both MACD legs, seeded with x[t0];")
    say("a later stage with its feeder's value on the bar where SMA_SEED's window for it starts.")
    say("Offsets run from max(400, the bar from which every stage's weight on its bar-0 seed")
    say("stays below 1e-9) to n - lookback - 4. DEMA, TEMA, TRIX as % of the converged output's")
    say("population sd, MACD as % of the converged line's, over the bars from the first offset on.")
    say()
    rows = []
    for cname, key, series in (("gData close", "gData", [g]), ("IBM close", "IBM", [ibm]),
                               ("random walk (geometric)", "rw",
                                [random_walk(101 + k) for k in range(SERIES)])):
        r = chained_block(cname, series)
        r["MACD"] = macd_block(cname, series)
        rows.append((key, r))
    ratio_block(rows)
    section11_check("gData", "gData", g)
    section11_check("IBM", "IBM", ibm)


def main():
    say("Single EMA, |error| of the first published value (bar t0+p-1) against the EMA")
    say("run from bar 0, % of the series' population sd, over every start offset t0 from")
    say("max(400, bar where (1-alpha)^t < 1e-9) to n-p-3. Percentiles: linear interpolation.")
    say("Synthetic: 10 series x 10000 bars pooled, each normalised by its own sd.")
    say("  random walk: geometric, start 100, daily log-return N(0, 0.01), seeds 101-110")
    say("  AR(1): phi 0.9 around level 100, innovation sd 1, stationary start, seeds 201-210")
    say("  white noise: level 100, sd 1, seeds 301-310")
    say()
    g = gdata_close()
    gd = block("gData close (%d bars, sd = %.4f)" % (len(g), pop_sd(g)), [g])
    ibm = ibm_close()
    block("IBM close (%d bars, sd = %.4f)" % (len(ibm), pop_sd(ibm)), [ibm])
    block("random walk (geometric)", [random_walk(101 + k) for k in range(SERIES)])
    block("AR(1) phi 0.9", [ar1(201 + k) for k in range(SERIES)])
    block("white noise (reference, not charted)", [white_noise(301 + k) for k in range(SERIES)])
    say("-- check: gData rms against section 5 (same offsets at these periods)")
    for p, (s5, f5) in sorted(SECTION5.items()):
        say("   p %3d  SMA_SEED %.3f%% (section 5 e_S first out %.3f%%)   "
            "FIRST_BAR_SEED %.3f%% (section 5 e_T @common %.3f%%)"
            % (p, gd[(p, "SMA_SEED")][5], s5, gd[(p, "FIRST_BAR_SEED")][5], f5))
    say()
    chained_main(g, ibm)
    with open(os.path.join(HERE, "quantiles.txt"), "w") as f:
        f.write("\n".join(OUT) + "\n")


if __name__ == "__main__":
    main()
