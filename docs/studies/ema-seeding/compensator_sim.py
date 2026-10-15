"""Synthetic check of the zero-start compensator against rules S and T: which seed best estimates the EMA of the full history, given only bars from the start on?

Truth: the EMA run over 40n bars of history before the start (unseen weight (1-a)^(40n) < 1e-30).
Each estimator sees bars from the start only. Error is reported relative to the truth's scale:
  prices (GBM, random walk, trend): |err| / price, in basis points.
  stationary noise (mean 0, sd 1): |err| in units of the noise sd.
RMS over trials, at offsets j from the start; TA-Lib has no output before j = n-1.
"""
import math, random, sys


def comp(x, n):
    """Zero start, output divided by 1-(1-a)^t until that is below 1e-10 (QuanTAlib Ema.Update)."""
    a = 2.0 / (n + 1); d = 1.0 - a
    ema, E, out, done = 0.0, 1.0, [], False
    for v in x:
        ema = ema * d + a * v
        if not done:
            E *= d
            if E <= 1e-10:
                done = True; out.append(ema)
            else:
                out.append(ema / (1.0 - E))
        else:
            out.append(ema)
    return out


def first(x, n):
    """Rule T: seed with the first value."""
    a = 2.0 / (n + 1); d = 1.0 - a
    ema = x[0]; out = [ema]
    for v in x[1:]:
        ema = ema * d + a * v
        out.append(ema)
    return out


def talib(x, n):
    """Rule S (TA_EMA): SMA of the first n bars at index n-1, then the recursion. None before n-1."""
    k = 2.0 / (n + 1)
    out = [None] * (n - 1)
    prev = sum(x[:n]) / n
    out.append(prev)
    for v in x[n:]:
        prev = (v - prev) * k + prev
        out.append(prev)
    return out

def series(kind, L, rng):
    if kind == "gbm":
        x = [100.0]
        for _ in range(L - 1): x.append(x[-1] * math.exp(rng.gauss(0, 0.01)))
    elif kind == "randwalk":
        x = [100.0]
        for _ in range(L - 1): x.append(x[-1] + rng.gauss(0, 1.0))
    elif kind == "trend":
        x = [100.0 + 0.1 * i + rng.gauss(0, 1.0) for i in range(L)]
    elif kind == "noise":
        x = [rng.gauss(0, 1.0) for _ in range(L)]
    elif kind == "ar1":  # mean-reverting around 0, phi 0.95, unit stationary sd
        phi = 0.95; s = math.sqrt(1 - phi * phi); v = rng.gauss(0, 1); x = []
        for _ in range(L): v = phi * v + rng.gauss(0, s); x.append(v)
    return x


def run(kind, n, trials, rng):
    H = 40 * n
    offs = sorted({0, n // 2, n - 1, 2 * n - 1, 3 * n - 1, 5 * n - 1})
    acc = {m: {j: 0.0 for j in offs} for m in ("comp", "first", "talib")}
    for _ in range(trials):
        x = series(kind, H + 5 * n, rng)
        truth = first(x, n)[H:]
        seen = x[H:]
        est = {"comp": comp(seen, n), "first": first(seen, n), "talib": talib(seen, n)}
        for j in offs:
            scale = abs(x[H + j]) if kind in ("gbm", "trend") else 1.0
            for m, e in est.items():
                if e[j] is not None:
                    acc[m][j] += ((e[j] - truth[j]) / scale) ** 2
    unit = 1e4 if kind in ("gbm", "trend") else 1.0
    rows = []
    for j in offs:
        r = {m: (math.sqrt(acc[m][j] / trials) * unit if (m != "talib" or j >= n - 1) else None) for m in acc}
        rows.append((j, r))
    return rows


if __name__ == "__main__":
    trials = int(sys.argv[1]) if len(sys.argv) > 1 else 2000
    rng = random.Random(12345)
    for kind in ("gbm", "randwalk", "trend", "noise", "ar1"):
        unit = "bp of price" if kind in ("gbm", "trend") else "step or noise sd"
        for n in (10, 30):
            print(f"\n{kind} n={n}  RMS error vs full-history EMA ({unit}), {trials} trials")
            print(f"  {'j':>4} {'compensator':>12} {'first-value':>12} {'TA-Lib SMA':>12}  best")
            for j, r in run(kind, n, trials, rng):
                f = lambda v: f"{v:12.4f}" if v is not None else f"{'-':>12}"
                best = min((v, m) for m, v in r.items() if v is not None)[1]
                print(f"  {j:>4} {f(r['comp'])} {f(r['first'])} {f(r['talib'])}  {best}")
