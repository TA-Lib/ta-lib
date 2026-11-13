#!/usr/bin/env python3
"""#548 probe: time TA-Lib through ta-lib-python the way a published comparison does.

    pybench.py one   --arm A --expect DIR --func F --bars N
    pybench.py suite --arm A --expect DIR

`one` is a fresh process for one function at one length. `suite` walks a list of
functions at rising lengths in ONE process, as a comparison script does, so the
allocator state each function meets is the one the earlier calls and numpy left.
Every result is dropped at once, as timeit does. Never lands.
"""

import argparse
import os
import platform
import resource
import sys
import time

import numpy as np
import talib

CALLS = {
    "SMA": lambda h, l, c: talib.SMA(c, timeperiod=30),
    "EMA": lambda h, l, c: talib.EMA(c, timeperiod=30),
    "RSI": lambda h, l, c: talib.RSI(c),
    "BBANDS": lambda h, l, c: talib.BBANDS(c),
    "BBANDS_EMA": lambda h, l, c: talib.BBANDS(c, matype=1),
    "MACD": lambda h, l, c: talib.MACD(c),
    "MACDEXT": lambda h, l, c: talib.MACDEXT(c),
    "MACDEXT_EMA": lambda h, l, c: talib.MACDEXT(c, fastmatype=1, slowmatype=1, signalmatype=1),
    "STOCH": lambda h, l, c: talib.STOCH(h, l, c),
    "STOCHF": lambda h, l, c: talib.STOCHF(h, l, c),
    "STOCHRSI": lambda h, l, c: talib.STOCHRSI(c),
    "ADX": lambda h, l, c: talib.ADX(h, l, c),
    "ATR": lambda h, l, c: talib.ATR(h, l, c),
    "TEMA": lambda h, l, c: talib.TEMA(c),
    "TRIX": lambda h, l, c: talib.TRIX(c),
    "T3": lambda h, l, c: talib.T3(c),
    "KC": lambda h, l, c: talib.KC(h, l, c),
    "SQZMOM": lambda h, l, c: talib.SQZMOM(h, l, c),
    "KSTEXT": lambda h, l, c: talib.KSTEXT(c),
}

SUITE = ["SMA", "EMA", "RSI", "BBANDS", "BBANDS_EMA", "MACD", "MACDEXT", "MACDEXT_EMA",
         "STOCH", "STOCHF", "STOCHRSI", "ADX", "ATR", "TEMA", "TRIX", "T3"]


def loaded_libs():
    found = set()
    with open("/proc/self/maps") as f:
        for line in f:
            parts = line.split(None, 5)
            if len(parts) == 6 and "libta-lib" in parts[5]:
                found.add(os.path.realpath(parts[5].strip()))
    return sorted(found)


def make_inputs(n):
    rng = np.random.default_rng(548)
    c = rng.random(n)
    h = c + rng.random(n) * 0.01
    l = c - rng.random(n) * 0.01
    return h, l, c


def minflt():
    return resource.getrusage(resource.RUSAGE_SELF).ru_minflt


def measure(fn, warm_s, rep_s, cap, tol):
    t0 = time.perf_counter_ns()
    n = 0
    while True:
        fn()
        n += 1
        elapsed = time.perf_counter_ns() - t0
        if elapsed >= warm_s * 1e9 and n >= 3:
            break
    inner = max(3, int(rep_s * 1e9 / (elapsed / n)))
    reps = []
    while len(reps) < cap:
        f0 = minflt()
        t0 = time.perf_counter_ns()
        for _ in range(inner):
            fn()
        t1 = time.perf_counter_ns()
        f1 = minflt()
        reps.append(((t1 - t0) / inner, (f1 - f0) / inner))
        if len(reps) >= 2:
            s = sorted(r[0] for r in reps)
            if s[1] <= s[0] * (1.0 + tol):
                break
    reps.sort()
    second = reps[1][0] if len(reps) > 1 else reps[0][0]
    return reps[0][0], second, reps[0][1], len(reps), inner


def row(arm, mode, name, bars, inputs, warm_s, rep_s, cap, tol):
    call = CALLS[name]
    h, l, c = inputs
    try:
        call(h, l, c)
    except Exception as e:  # a wrapper without this function, or a rejected call
        print("skip arm=%s mode=%s func=%s bars=%d: %s: %s"
              % (arm, mode, name, bars, type(e).__name__, e), flush=True)
        return
    ns, second, faults, nreps, inner = measure(lambda: call(h, l, c), warm_s, rep_s, cap, tol)
    print("row arm=%s mode=%s func=%s bars=%d ns_min=%.0f ns_2nd=%.0f reps=%d/%d inner=%d "
          "minflt_per_call=%.2f" % (arm, mode, name, bars, ns, second, nreps, cap, inner, faults),
          flush=True)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("mode", choices=["one", "suite"])
    p.add_argument("--arm", required=True)
    p.add_argument("--expect", required=True, help="directory the loaded libta-lib must live in")
    p.add_argument("--func")
    p.add_argument("--bars", type=int)
    p.add_argument("--sizes", default="1000,10000,100000,1000000")
    p.add_argument("--cap", type=int, default=6)
    p.add_argument("--tol", type=float, default=0.01)
    a = p.parse_args()

    libs = loaded_libs()
    expect = os.path.realpath(a.expect)
    if len(libs) != 1 or os.path.dirname(libs[0]) != expect:
        sys.exit("WRONG LIBRARY: loaded %s, expected exactly one under %s" % (libs, expect))
    try:
        with open("/sys/kernel/mm/transparent_hugepage/enabled") as f:
            thp = f.read().strip()
    except OSError:
        thp = "?"
    print("hdr arm=%s lib=%s python=%s numpy=%s talib=%s ta=%s libc=%s thp=%s"
          % (a.arm, libs[0], platform.python_version(), np.__version__, talib.__version__,
             getattr(talib, "__ta_version__", "?"), "-".join(platform.libc_ver()), thp),
          flush=True)

    if a.mode == "one":
        row(a.arm, "one", a.func, a.bars, make_inputs(a.bars), 0.3, 0.25, a.cap, a.tol)
    else:
        for n in (int(s) for s in a.sizes.split(",")):
            inputs = make_inputs(n)
            for name in SUITE:
                row(a.arm, "suite", name, n, inputs, 0.1, 0.1, a.cap, a.tol)
            del inputs


if __name__ == "__main__":
    main()
