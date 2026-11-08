#!/usr/bin/env python3
"""Synthetic daily OHLCV series for the candlestick function-page charts.

Deterministic (fixed seed, python3 stdlib only) and public domain by
construction. Prices are drawn in integer cents, so every value has at most
two decimals and equalities (doji, repeated closes/lows) are exact.

Two layers:
  * bar mixture: each regime draws a style (volatility, trend, gap and
    shadow habits); each bar draws its open, body and shadows from it.
  * episodes: short market behaviours with random sizes that a bar-by-bar
    mixture almost never strings together (a pullback inside a trend bar, a
    gap that reverses, a run of trend days, a retest of a close). They were
    shaped while reading the TA-Lib pattern definitions, so the multi-bar
    patterns occur, but no bar is checked against a pattern.

Usage: gen_synth.py [out.csv] [nbars] [seed]
(the committed file: gen_synth.py src/tools/ta_refdata/corpus/synthetic.csv)
"""
import datetime
import math
import random
import sys

# Of seeds 1..330, the one whose shortest prefix gives every CDL function at least
# two flagged bars in some 252-bar window; NBARS is that prefix.
SEED = 186
NBARS = 1020

# Per-bar probabilities of a style:
#   vol    daily volatility range (fraction of price)    trend  white-body probability range
#   pers   body repeats the previous colour              gap    open beyond the prior real body
#   fgap   a gap also clears the prior high/low          cont   gap direction follows the prior bar
#   rev    gap direction against the prior bar (else the trend)
#   fade   a gapped bar closes back toward the prior bar gogo   a gap that holds has no shadows
#   tail   rejection wick the day after a trend bar
#   doji/short/long  body-size mixture (rest: medium)    dpers  doji follows doji
#   maru   no shadows (long bodies: maru_long)           ext    long body closes at its extreme
#   snap   close lands exactly on a recent close/low/high
#   ep     an episode starts (weights in EPISODES)
STYLES = {
    "calm": dict(w=3, vol=(0.006, 0.010), trend=(0.45, 0.55), pers=0.50, gap=0.12, fgap=0.25, cont=0.45, rev=0.2,
                 fade=0.5, doji=0.10, dpers=0.25, short=0.45, long=0.10, maru=0.02, maru_long=0.07,
                 ext=0.3, snap=0.10, ep=0.14, gogo=0.07, tail=0.2),
    "trend": dict(w=3, vol=(0.010, 0.016), trend=(0.25, 0.75), pers=0.62, gap=0.20, fgap=0.35, cont=0.45, rev=0.2,
                  fade=0.4, doji=0.06, dpers=0.2, short=0.30, long=0.22, maru=0.03, maru_long=0.17,
                  ext=0.5, snap=0.08, ep=0.29, gogo=0.15, tail=0.25),
    "volatile": dict(w=2, vol=(0.016, 0.026), trend=(0.40, 0.60), pers=0.45, gap=0.28, fgap=0.45, cont=0.45, rev=0.2,
                     fade=0.55, doji=0.07, dpers=0.2, short=0.30, long=0.25, maru=0.02, maru_long=0.12,
                     ext=0.4, snap=0.10, ep=0.29, gogo=0.12, tail=0.35),
    "gappy": dict(w=2, vol=(0.011, 0.019), trend=(0.35, 0.65), pers=0.55, gap=0.30, fgap=0.50, cont=0.45, rev=0.2,
                  fade=0.5, doji=0.08, dpers=0.3, short=0.28, long=0.28, maru=0.06, maru_long=0.23,
                  ext=0.5, snap=0.10, ep=0.29, gogo=0.23, tail=0.25),
    "quiet": dict(w=1, vol=(0.005, 0.008), trend=(0.45, 0.55), pers=0.45, gap=0.25, fgap=0.40, cont=0.45, rev=0.2,
                  fade=0.5, doji=0.30, dpers=0.45, short=0.45, long=0.08, maru=0.03, maru_long=0.10,
                  ext=0.3, snap=0.12, ep=0.11, gogo=0.10, tail=0.2),
}
REGIME_LEN = (25, 70)
# Episodes whose shape reads one way: probability of an up-move first.
SIDE = {"crows": 0.85, "failed": 0.85, "pullback_gap": 0.8,
        "dwindle": 0.15, "river": 0.15, "conceal": 0.15, "ladder": 0.15}
HABIT = 0.5  # a regime's first episode kind comes back this often
EPISODES = {"pullback_gap": 4, "pullback_inside": 2, "trap": 4, "kicker": 2, "star": 4, "crows": 3,
            "failed": 3, "breakaway": 4, "gaptest": 2, "dwindle": 2, "river": 2, "retest": 2, "coil": 2,
            "run": 2, "strike": 2, "conceal": 2, "ladder": 2}


def weekdays(start, n):
    d = start
    out = []
    while len(out) < n:
        if d.weekday() < 5:
            out.append(d)
        d += datetime.timedelta(days=1)
    return out


def pick(rng, table):
    r = rng.random() * sum(v if isinstance(v, (int, float)) else v["w"] for v in table.values())
    for k, v in table.items():
        r -= v if isinstance(v, (int, float)) else v["w"]
        if r < 0:
            return k
    return k


def episode(rng, kind, up):
    """Bar specs for one episode; `up` is the episode's trend direction."""
    g = 1 if up else -1
    if kind in ("pullback_gap", "pullback_inside"):
        # Long trend bar, 2-4 short counter bars stepping back into its body (the first
        # may gap past it), then a trend bar that resumes past them all.
        first_gap = kind == "pullback_gap"
        n = rng.choice((2, 3, 3, 3, 4))
        specs = [dict(kind="long", white=up, maru=rng.random() < 0.2)]
        depth = sorted(rng.uniform(0.05, 0.45 if first_gap else 0.95) for _ in range(n))
        for j in range(n):
            if j == 0 and first_gap:
                specs.append(dict(kind="short", white=not up, open="gap_body", gapdir=g, hold=True))
            else:
                specs.append(dict(kind="short", white=not up, level=depth[j], inside=True))
        specs.append(dict(kind="long", white=up, open="beyond_close", dir=g,
                          maru=rng.random() < 0.2, close_past="all"))
        return specs
    first = dict(kind=rng.choice(("long", "long", "mid")), white=up, maru=rng.random() < 0.15)
    if kind == "trap":
        # A big gap further along the move that reverses back to, or into, the trend bar.
        return [first, dict(kind="long", white=not up, open=rng.choice(("gap_full", "gap_body")),
                            gapdir=g, bigstep=True,
                            trap=rng.choice(("close", "edge", "neck", "shallow", "deep", "deep")))]
    if kind == "kicker":
        first.update(kind="long", maru=rng.random() < 0.75)
        return [first, dict(kind="long", white=not up, open="gap_full", gapdir=-g,
                            maru=rng.random() < 0.75)]
    if kind in ("star", "crows", "failed"):
        # A gap along the move to a star; the next day gaps back (star), opens inside
        # the star's body (crows), or opens past it and fails.
        star = dict(kind=rng.choice(("doji", "doji", "doji", "short")), white=rng.random() < 0.5,
                    open=rng.choice(("gap_full", "gap_full", "gap_full", "gap_body")), gapdir=g, hold=True)
        if kind != "star":
            star.update(kind="short", white=not up)
        om, into = {"star": (rng.choice(("gap_full", "gap_full", "gap_full", "gap_body")), "deep"),
                    "crows": ("in_body", "deep"), "failed": ("over", "shallow")}[kind]
        return [first, star, dict(kind=rng.choice(("long", "mid")), white=not up, open=om,
                                  gapdir=-g, into=into)]
    if kind == "gaptest":
        # A gap along the move, then a day back into the gap that leaves it open.
        return [first, dict(kind=rng.choice(("short", "mid", "mid")), white=up, open="gap_body", gapdir=g),
                dict(kind=rng.choice(("short", "mid")), white=not up, open="in_body", into="gap")]
    if kind == "breakaway":
        # A gap along the move, 1-2 more pushes, then a day back into the gap.
        pushes = rng.choice((1, 2, 2))
        specs = [first, dict(kind=rng.choice(("short", "mid")), white=up, open="gap_body", gapdir=g)]
        for j in range(pushes):
            specs.append(dict(kind=rng.choice(("short", "mid")),
                              white=rng.random() < 0.5 if j < pushes - 1 else up,
                              open="drift", drift=0.4 * g, push=g))
        specs.append(dict(kind=rng.choice(("long", "mid")), white=not up,
                          open=rng.choice(("gap_body", "near", "near")), gapdir=-g,
                          into="gap" if rng.random() < 0.7 else "deep"))
        return specs
    if kind in ("dwindle", "river"):
        # Trend days, each smaller, opening back inside the prior body, the rejection
        # wick shrinking (dwindle) or one last probe past the low (river).
        river = kind == "river"
        specs = [dict(kind="long", white=up, open="near", wick_mult=rng.uniform(1.1, 2.0))]
        specs.append(dict(kind="short" if river else rng.choice(("mid", "short")), white=up,
                          open="in_body", wick_to="past" if river else "between", smaller=True))
        if river:
            specs.append(dict(kind="short", white=not up, open="near"))
        else:
            specs.append(dict(kind="short", white=up, open="in_body", maru=True, smaller=True))
        return specs
    if kind == "retest":
        # A day against the trend, a day back that stays clear of its close, and a day
        # that returns exactly to that close.
        return [dict(kind=rng.choice(("mid", "long")), white=not up),
                dict(kind=rng.choice(("short", "mid")), white=up, open="clear"),
                dict(kind=rng.choice(("short", "mid")), white=not up, open="near", retest=True)]
    if kind == "coil":
        # 2-3 inside days, then a breakout either way.
        specs = [dict(kind=rng.choice(("long", "mid")))]
        for _ in range(rng.choice((2, 2, 3))):
            specs.append(dict(kind=rng.choice(("short", "short", "mid", "doji")), open="in_range",
                              white=rng.random() < 0.5, inner=True))
        specs.append(dict(kind=rng.choice(("mid", "long")), white=up, open="near", push=g))
        return specs
    # Runs: 3-4 trend days opening inside the prior body or at its close, closing near
    # their extreme, after a day the other way; they may end in a strike back (strike),
    # a gap and an outside day (conceal), or a reversal gap (ladder).
    n = 4 if kind == "ladder" else rng.choice((3, 4))
    om = rng.choice(("in_body", "in_body", "at_close"))
    specs = [dict(kind=rng.choice(("mid", "short")), white=not up)]
    specs += [dict(kind=rng.choice(("long", "mid", "mid")), white=up, open=om if j else "near",
                   ext=True, maru=kind == "conceal" or rng.random() < 0.1) for j in range(n)]
    if kind == "strike":
        specs.append(dict(kind="long", white=not up, open="beyond_close", dir=g, strike=True))
    elif kind == "conceal":
        specs[-1].update(open="gap_body", gapdir=g, maru=False, ext=False, wick=True)
        specs.append(dict(kind="long", white=up, open="near", outside=True))
    elif kind == "ladder":
        specs[-1].update(wick=True, ext=False, maru=False)
        specs.append(dict(kind="long", white=not up, open="gap_body", gapdir=-g, close_past=1))
    return specs


def generate(nbars, seed=SEED):
    rng = random.Random(seed)
    bars = []
    price = 4817  # cents
    left = 0
    st = None
    white_p = 0.5
    vol = 0.015
    last_white = True
    last_doji = False
    queue = []
    anchor = None  # first bar of the running episode
    while len(bars) < nbars:
        if left <= 0:
            st = STYLES[pick(rng, STYLES)]
            left = rng.randint(*REGIME_LEN)
            vol = rng.uniform(*st["vol"])
            white_p = rng.uniform(*st["trend"])
            hab = {}
        left -= 1
        # Keep the walk inside roughly 20..200: lean toward 60, harder past 30 and 140.
        lp = math.log(price / 100.0)
        lean = -0.15 * (lp - math.log(60)) / math.log(2.0)
        if lp > math.log(140):
            lean -= 0.3 * (lp - math.log(140)) / math.log(1.3)
        elif lp < math.log(30):
            lean += 0.3 * (math.log(30) - lp) / math.log(1.3)
        wp = min(0.9, max(0.1, white_p + lean))

        if not queue and bars and rng.random() < st["ep"]:
            if "kind" not in hab:
                hab["kind"] = pick(rng, EPISODES)
            kind = hab["kind"] if rng.random() < HABIT else pick(rng, EPISODES)
            side = SIDE.get(kind)
            side = wp if side is None else min(0.9, max(0.1, side + 2 * lean))
            queue = episode(rng, kind, rng.random() < side)
            anchor = len(bars)
        spec = queue.pop(0) if queue else {}

        unit = max(4, int(round(price * vol)))  # typical day's body, cents
        pc = price
        prev = bars[-1] if bars else None
        po, ph, pl, pcl = prev[:4] if prev else (pc, pc, pc, pc)
        ptop, pbot = max(po, pcl), min(po, pcl)
        # ---- body size
        kind = spec.get("kind")
        if kind is None:
            s = rng.random()
            pd = st["dpers"] if last_doji else st["doji"]
            kind = "mid"
            if s < pd:
                kind = "doji"
            elif s < pd + st["short"]:
                kind = "short"
            elif s < pd + st["short"] + st["long"]:
                kind = "long"
        if kind == "doji":
            b = rng.choice((0, 0, 1))
        elif kind == "short":
            b = int(round(unit * rng.uniform(0.08, 0.6)))
        elif kind == "long":
            b = int(round(unit * rng.uniform(1.4, 3.2)))
        else:
            b = int(round(unit * rng.uniform(0.5, 1.4)))
        if spec.get("smaller") and prev is not None:
            b = max(0, min(b, abs(pcl - po) - 1))
        # ---- open
        gapped = 0
        om = spec.get("open")
        if om is None and prev is not None and rng.random() < st["gap"]:
            om = "gap_full" if rng.random() < st["fgap"] else "gap_body"
        step = max(1, int(round(unit * (rng.uniform(0.8, 2.2) if spec.get("bigstep") else rng.uniform(0.05, 0.9)))))
        if prev is None:
            o = pc
        elif om in ("gap_full", "gap_body"):
            if "gapdir" in spec:
                up = spec["gapdir"] > 0
            else:
                r = rng.random()
                if r < st["cont"]:
                    up = pcl >= po
                elif r < st["cont"] + st["rev"]:
                    up = pcl < po
                else:
                    up = rng.random() < wp
            if om == "gap_full":
                edge = ph if up else pl
            else:
                edge = ptop if up else pbot
            o = edge + step if up else edge - step
            gapped = 1 if up else -1
        elif om == "in_body":
            o = pbot + int(round((ptop - pbot) * rng.uniform(0.1, 0.9)))
            if ptop - pbot >= 2:
                o = min(ptop - 1, max(pbot + 1, o))
        elif om == "clear":
            # Opens and trades entirely past the prior close, toward this bar's colour.
            o = pcl + (1 if spec["white"] else -1) * max(2, int(round(unit * rng.uniform(0.1, 0.6))))
        elif om == "in_range":
            o = pl + 1 + int(round(max(0, ph - pl - 2) * rng.uniform(0.15, 0.85)))
        elif om == "at_close":
            o = pc + rng.choice((-1, 0, 0, 1))
        elif om == "drift":
            o = pc + int(round(spec["drift"] * unit * rng.uniform(0.0, 1.0)))
        elif om == "beyond_close":
            o = pc + spec["dir"] * max(1, int(round(unit * rng.uniform(0.02, 0.4))))
        elif om == "over":
            # Opens past the prior body, against this bar's colour (a failed push).
            o = (ptop + step // 2 + 1) if not spec["white"] else (pbot - step // 2 - 1)
        elif om is None and rng.random() < 0.08:
            o = pc
        else:
            o = pc + int(round(rng.gauss(0, 0.15) * unit))
        o = max(100, o)
        # ---- colour / close
        if "into" in spec and anchor is not None:
            a = bars[anchor]
            if spec["into"] == "gap":
                x2 = bars[anchor + 1][0]
            elif spec["into"] == "shallow":
                x2 = pbot if a[3] >= a[0] else ptop
            else:
                x2 = a[0]
            f = rng.uniform(0.3, 1.1) if spec["into"] == "deep" else rng.uniform(0.1, 0.9)
            c = a[3] + int(round((x2 - a[3]) * f))
            if (c > o) != spec["white"] or c == o:
                c = o + b if spec["white"] else o - b
        elif "trap" in spec and prev is not None:
            t, pb = spec["trap"], abs(pcl - po)
            sgn = 1 if po > pcl else -1  # from the prior close back into its body
            if t == "close":
                c = pcl
            elif t == "edge":
                c = pl if spec["white"] else ph
            elif t == "neck":
                c = pcl + sgn * rng.randint(1, max(1, pb // 20))
            elif t == "shallow":
                c = pcl + sgn * int(round(pb * rng.uniform(0.1, 0.48)))
            else:
                c = pcl + sgn * int(round(pb * rng.uniform(0.52, 0.95)))
            if (c > o) != spec["white"] or c == o:
                c = o + b if spec["white"] else o - b
        elif "level" in spec and anchor is not None:
            a = bars[anchor]
            c = a[3] + int(round((a[0] - a[3]) * spec["level"]))
            o = c + b if not spec["white"] else c - b
            if spec["white"] == (a[3] > a[0]):
                o = c - b if spec["white"] else c + b
        elif spec.get("strike") and anchor is not None:
            a = bars[anchor]
            c = a[0] + (1 if spec["white"] else -1) * max(1, int(round(unit * rng.uniform(0.05, 0.6))))
            if (c > o) != spec["white"] or c == o:
                c = o + b if spec["white"] else o - b
        elif "white" in spec:
            c = o + b if spec["white"] else o - b
        elif gapped and kind != "doji" and rng.random() < st["fade"]:
            # Gap fade: back to the prior close, onto a prior extreme, or across its range.
            q = rng.random()
            if q < 0.35:
                c = pcl
            elif q < 0.5:
                c = pl if gapped < 0 else ph
            else:
                c = pl + int(round(max(2, ph - pl) * rng.uniform(-0.15, 1.15)))
            if (gapped > 0 and c >= o) or (gapped < 0 and c <= o):
                c = o - b if gapped > 0 else o + b
        else:
            if gapped and kind != "doji":
                white = gapped > 0
            elif rng.random() < st["pers"]:
                white = last_white
            else:
                white = rng.random() < wp
            c = o + b if white else o - b
        if spec.get("retest") and len(bars) >= 2:
            t = bars[-2][3]
            if t != o and (t > o) == spec["white"]:
                c = t
        if spec.get("hold"):
            # A star keeps its body beyond the prior body.
            c = max(c, ptop + 1) if gapped > 0 else min(c, pbot - 1)
        if spec.get("inside") and anchor is not None:
            a = bars[anchor]
            c = min(a[1] - 1, max(a[2] + 1, c))
        if spec.get("close_past") and anchor is not None:
            back = bars[anchor:] if spec["close_past"] == "all" else bars[-spec["close_past"]:]
            hi = max(x[1] for x in back)
            lo = min(x[2] for x in back)
            if spec["white"]:
                c = max(c, hi + max(1, int(round(unit * rng.uniform(0.05, 0.5)))))
            else:
                c = min(c, lo - max(1, int(round(unit * rng.uniform(0.05, 0.5)))))
        if spec.get("inner") and prev is not None and ph - pl >= 3:
            c = min(ph - 1, max(pl + 1, c))
            if c == o:
                c = o + (1 if o < ph - 1 else -1)
        if spec.get("outside"):
            c = pl - step if not spec["white"] else ph + step
        # Exact landings on a recent level (matching low, stick sandwich, on-neck ...)
        if not spec and prev is not None and kind != "doji" and rng.random() < st["snap"]:
            refs = bars[-3:]
            cands = [r[3] for r in refs] * 2 + [r[2] for r in refs[-2:]] + [refs[-1][1]]
            cands = [t for t in cands if t != o and (t > o) == (c > o)]
            if cands:
                c = rng.choice(cands)
        c = max(100, c)
        if c != o:
            last_white = c > o
        last_doji = kind == "doji"
        # ---- shadows
        top, bot = max(o, c), min(o, c)
        body = top - bot
        pm = st["maru_long"] if kind == "long" else st["maru"]
        if gapped and not spec and (c > o) == (gapped > 0):
            pm = max(pm, st["gogo"])
        if spec.get("maru") or (not spec and rng.random() < pm):
            us = rng.choice((0, 0, 1))
            ls = rng.choice((0, 0, 1))
        else:
            us = int(round(abs(rng.gauss(0, 0.5)) * unit))
            ls = int(round(abs(rng.gauss(0, 0.5)) * unit))
            t = rng.random()
            if prev is not None and not spec and abs(pcl - po) > 1.3 * unit and rng.random() < st["tail"]:
                # Rejection wick the day after a trend bar.
                wick = int(round(max(body, unit * 0.3) * rng.uniform(1.0, 2.5)))
                if pcl < po:
                    ls = wick
                else:
                    us = wick
            elif t < 0.10:
                us = int(round(max(body, unit * 0.3) * rng.uniform(1.2, 3.0)))
            elif t < 0.20:
                ls = int(round(max(body, unit * 0.3) * rng.uniform(1.2, 3.0)))
            elif t < 0.25:
                us = int(round(unit * rng.uniform(0.8, 2.0)))
                ls = int(round(unit * rng.uniform(0.8, 2.0)))
            if spec.get("ext") or (kind == "long" and not spec and rng.random() < st["ext"]):
                if c > o:
                    us = rng.choice((0, 1))
                else:
                    ls = rng.choice((0, 1))
            if spec.get("wick"):
                # Wick back into the prior body.
                if c > o:
                    ls = max(ls, bot - pcl + rng.randint(1, max(1, unit // 3)))
                else:
                    us = max(us, pcl - top + rng.randint(1, max(1, unit // 3)))
        if "wick_mult" in spec:
            w = int(round(max(body, 1) * spec["wick_mult"]))
            if c < o:
                ls, us = w, min(us, unit // 4)
            else:
                us, ls = w, min(ls, unit // 4)
        if "wick_to" in spec and prev is not None:
            if c < o:
                lvl = (pl + int(round((min(pcl, bot) - pl) * rng.uniform(0.1, 0.9)))
                       if spec["wick_to"] == "between" else pl - rng.randint(1, max(1, unit // 2)))
                ls = max(0, bot - lvl)
            else:
                lvl = (ph - int(round((ph - max(pcl, top)) * rng.uniform(0.1, 0.9)))
                       if spec["wick_to"] == "between" else ph + rng.randint(1, max(1, unit // 2)))
                us = max(0, lvl - top)
        if gapped and om == "gap_full" and (spec or rng.random() < 0.6):
            # The window stays open on the gap day.
            if gapped > 0:
                ls = min(ls, max(0, bot - ph - 1))
            else:
                us = min(us, max(0, pl - 1 - top))
        if om == "clear" and prev is not None:
            if c > o:
                ls = min(ls, max(0, bot - pcl - 1))
            else:
                us = min(us, max(0, pcl - 1 - top))
        if spec.get("inner") and prev is not None:
            us = min(us, max(0, ph - 1 - top))
            ls = min(ls, max(0, bot - pl - 1))
        if spec.get("inside") and anchor is not None:
            a = bars[anchor]
            us = min(us, max(0, a[1] - 1 - top))
            ls = min(ls, max(0, bot - a[2] - 1))
        h = top + us
        l = bot - ls
        if spec.get("push") and prev is not None:
            # Higher highs and higher lows (or lower and lower) than the prior bar.
            if spec["push"] > 0:
                h = max(h, ph + rng.randint(1, max(1, unit // 3)))
                l = max(l, pl + 1) if pl + 1 <= bot else l
            else:
                l = min(l, pl - rng.randint(1, max(1, unit // 3)))
                h = min(h, ph - 1) if ph - 1 >= top else h
        if spec.get("outside") and prev is not None:
            h = max(h, ph + rng.randint(1, max(1, unit // 3)))
            l = min(l, pl - rng.randint(1, max(1, unit // 3)))
        l = max(50, l)
        # ---- volume: heavier on wide days
        rng_frac = (h - l) / max(1.0, unit * 2.0)
        v = int(1_200_000 * math.exp(rng.gauss(0, 0.35)) * (0.6 + 0.6 * rng_frac))
        bars.append((o, h, l, c, max(1000, v)))
        price = c
    return bars


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "synthetic.csv"
    n = int(sys.argv[2]) if len(sys.argv) > 2 else NBARS
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else SEED
    bars = generate(n, seed)
    dates = weekdays(datetime.date(2001, 1, 2), n)
    with open(out, "w") as f:
        f.write("# label: Synthetic series, daily\n")
        f.write("# synthetic: src/tools/ta_refdata/gen_synth.py, seed %d, %d bars\n" % (seed, n))
        f.write("date,open,high,low,close,volume\n")
        for d, (o, h, l, c, v) in zip(dates, bars):
            assert 0 < l <= min(o, c) and h >= max(o, c), (d, o, h, l, c)
            f.write("%s,%.2f,%.2f,%.2f,%.2f,%d\n" % (d.isoformat(), o / 100, h / 100, l / 100, c / 100, v))


if __name__ == "__main__":
    main()
