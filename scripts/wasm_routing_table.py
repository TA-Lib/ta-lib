#!/usr/bin/env python3
"""Turns wasm_routing_bench.mjs results into ta_codegen/input/wasm_routing.yaml.

Usage: python3 scripts/wasm_routing_table.py <host1.jsonl> <host2.jsonl> [...]
           [--margin=0.10] [--min-hosts=3] [--max-iqr=PERCENT]
       python3 scripts/wasm_routing_table.py --self-test

One file per timing host; a file may hold several sweeps, and each (function,
bars, arm) then takes the median over the sweeps of its median and of its IQR.
Prints the YAML on stdout and the report on stderr; prints no YAML, and exits 1,
when it refuses.

A function routes from the smallest measured bar count B such that at B and at
every larger measured count, on every host, WASM's upper quartile is at most
(1 - margin) times TypeScript's lower quartile. Quartiles, because then a row
routes only when the two arms' middle halves stay apart by the margin, so noise
raises a threshold or leaves a function out instead of flipping a decision on a
rerun; the margin, because a host the table was not timed on differs by more
than the noise, and a function routed at parity buys nothing and risks a loss.

--max-iqr is an optional hard ceiling, off by default: any row whose arm has an
IQR above that percent of its median refuses the whole table.
"""

import argparse
import json
import os
import statistics
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROUTING_FILE = os.path.join(ROOT, "ta_codegen", "input", "wasm_routing.yaml")
EPS = 1e-12


def load(path):
    """{'name', 'funcs': {F: None (no export) | {bars: {arm: [median...], arm+'_iqr': [...]}}}, 'fails': [...]}."""
    host = {"name": os.path.splitext(os.path.basename(path))[0], "path": path, "funcs": {}, "fails": []}
    env = path + ".env"
    if os.path.exists(env):
        with open(env) as f:
            for line in f:
                if line.startswith("cpu="):
                    host["cpu"] = line[4:].strip()
    with open(path) as f:
        for n, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            try:
                row = json.loads(line)
            except ValueError:
                host["fails"].append("%s line %d: not JSON" % (path, n))
                continue
            if "fail" in row:
                host["fails"].append(row["fail"])
            elif "error" in row:
                host["fails"].append("%s at %s bars: %s" % (row.get("func"), row.get("bars"), row["error"]))
            elif row.get("wasm") is False:
                host["funcs"].setdefault(row["func"], None)
            elif "skip" in row:
                continue
            else:
                sizes = host["funcs"].setdefault(row["func"], {})
                cell = sizes.setdefault(row["bars"], {"js": [], "wasm": [], "js_iqr": [], "wasm_iqr": []})
                for arm in ("js", "wasm"):
                    cell[arm].append(row[arm]["median"])
                    cell[arm + "_iqr"].append(row[arm]["iqr"])
    return host


def med(cell, arm):
    return statistics.median(cell[arm])


def quartiles(cell, arm):
    """(lower, upper) quartile of an arm's samples.

    The bench line carries the median and the IQR as a percent of it, not the
    quartiles: this takes them as median * (1 -/+ IQR/2), which is exact only for
    a sample symmetric about its median.
    """
    m = med(cell, arm)
    half = statistics.median(cell[arm + "_iqr"]) / 200
    return m * (1 - half), m * (1 + half)


def routes(cell, margin):
    return quartiles(cell, "wasm")[1] <= (1 - margin) * quartiles(cell, "js")[0] * (1 + EPS)


def routes_on_medians(cell, margin):
    return med(cell, "wasm") <= (1 - margin) * med(cell, "js") * (1 + EPS)


def faster(js, wasm):
    gain = 100 * (1 - wasm / js)
    return "%.0f%% %s" % (abs(gain), "faster" if gain >= 0 else "slower")


def threshold(func, hosts, margin, test=routes):
    """The smallest B routed for `func` over `hosts`, or None."""
    sizes = sorted(hosts[0]["funcs"][func])
    best = None
    for s in reversed(sizes):
        if all(test(h["funcs"][func][s], margin) for h in hosts):
            best = s
        else:
            break
    return best


def quartile_ratio(cell):
    """WASM's upper quartile over TypeScript's lower one: the rule routes at <= 1 - margin."""
    return quartiles(cell, "wasm")[1] / quartiles(cell, "js")[0]


def decide(hosts, margin, min_hosts, max_iqr=None):
    """(table or None, report lines, refusals)."""
    refusals = []
    report = []
    if len(hosts) < min_hosts:
        refusals.append("%d host file(s) given; the table needs %d" % (len(hosts), min_hosts))
    for h in hosts:
        for f in h["fails"]:
            refusals.append("%s: failed: %s" % (h["name"], f))
    names = sorted(set().union(*(h["funcs"] for h in hosts))) if hosts else []
    for h in hosts:
        for f in names:
            if f not in h["funcs"]:
                refusals.append("%s: lacks %s, which another host has" % (h["name"], f))
    noisy = []
    timed = []
    for f in names:
        present = [h for h in hosts if f in h["funcs"]]
        kinds = {h["funcs"][f] is None for h in present}
        if len(kinds) > 1:
            refusals.append("%s: a WASM export on some hosts and not on others" % f)
            continue
        if True in kinds:
            continue
        size_sets = {tuple(sorted(h["funcs"][f])) for h in present}
        if len(size_sets) > 1:
            refusals.append("%s: measured at different bar counts on different hosts" % f)
            continue
        timed.append(f)
        if max_iqr is None:
            continue
        for h in present:
            for s, cell in sorted(h["funcs"][f].items()):
                for arm in ("js", "wasm"):
                    iqr = statistics.median(cell[arm + "_iqr"])
                    if iqr > max_iqr:
                        noisy.append("%s: %s at %d bars: %s IQR %.1f%% of its median (ceiling %.1f%%)" % (h["name"], f, s, arm, iqr, max_iqr))
    if noisy:
        refusals.append("%d row(s) above the --max-iqr ceiling:" % len(noisy))
        refusals.extend("  " + n for n in noisy)
    if refusals:
        return None, report, refusals

    table = {}
    left_out = []
    differs = []
    for f in timed:
        b = threshold(f, hosts, margin)
        per_host = {h["name"]: threshold(f, [h], margin) for h in hosts}
        if None in per_host.values() and len(set(per_host.values())) > 1:
            differs.append((f, per_host))
        if b is not None:
            table[f] = b
            continue
        best = None
        for s in sorted(hosts[0]["funcs"][f]):
            worst = max(((quartile_ratio(h["funcs"][f][s]), h) for h in hosts), key=lambda t: t[0])
            if best is None or worst[0] < best[0]:
                best = (worst[0], s, worst[1])
        left_out.append((f, best))

    noise = {}
    for h in hosts:
        rows = sum(1 for f in timed for c in h["funcs"][f].values() if routes_on_medians(c, margin) and not routes(c, margin))
        moved = sum(1 for f in timed if threshold(f, [h], margin, routes_on_medians) != threshold(f, [h], margin))
        noise[h["name"]] = (rows, moved)

    report.append("hosts: " + ", ".join("%s (%s)" % (h["name"], h.get("cpu", "cpu unknown")) for h in hosts))
    report.append("margin %.0f%%; a row routes when WASM's upper quartile <= (1 - margin) x TS's lower quartile;"
                  " q-ratio is that quotient, faster/slower is 1 - WASM/TS median time, both on the host that gains least"
                  % (100 * margin))
    report.append("routed (%d):" % len(table))
    for f, b in sorted(table.items()):
        cells = [h["funcs"][f][b] for h in hosts]
        worst = max(cells, key=quartile_ratio)
        report.append("  %-20s from %6d bars, q-ratio %.3f, WASM %s there (TS %.0f ns, WASM %.0f ns)"
                      % (f, b, quartile_ratio(worst), faster(med(worst, "js"), med(worst, "wasm")), med(worst, "js"), med(worst, "wasm")))
    report.append("left out (%d), best size by the worst host's q-ratio:" % len(left_out))
    for f, (ratio, s, h) in left_out:
        cell = h["funcs"][f][s]
        report.append("  %-20s q-ratio %.3f at %d bars on %s, WASM %s on medians (TS %.0f ns, WASM %.0f ns)"
                      % (f, ratio, s, h["name"], faster(med(cell, "js"), med(cell, "wasm")), med(cell, "js"), med(cell, "wasm")))
    report.append("decision differs between hosts (%d):" % len(differs))
    for f, per_host in differs:
        report.append("  %-20s %s" % (f, ", ".join("%s: %s" % (n, "from %d" % b if b is not None else "not routed")
                                                    for n, b in per_host.items())))
    report.append("decided by the noise term, per host (rows medians alone would route and quartiles do not;"
                  " functions whose threshold on that host alone it raises or removes):")
    for name, (rows, moved) in noise.items():
        report.append("  %-20s %d row(s), %d function(s)" % (name, rows, moved))
    return table, report, refusals


def header():
    lines = []
    if os.path.exists(ROUTING_FILE):
        with open(ROUTING_FILE) as f:
            for line in f:
                if not line.startswith("#"):
                    break
                lines.append(line)
    return "".join(lines)


def emit_yaml(table, head):
    if not table:
        return head + "batch: {}\n"
    return head + "batch:\n" + "".join("  %s: %d\n" % (f, b) for f, b in sorted(table.items()))


# --- self-test -------------------------------------------------------------

def _row(func, bars, js, wasm, iqr=0.0, js_iqr=None):
    return {"func": func, "bars": bars, "js": {"median": js, "iqr": iqr if js_iqr is None else js_iqr},
            "wasm": {"median": wasm, "iqr": iqr}}


def _curve(func, ratios, sizes=(100, 1000, 10000), iqr=0.0):
    return [_row(func, s, 100.0, 100.0 * r, iqr) for s, r in zip(sizes, ratios)]


def _run(case, host_rows, expect_yaml, expect_refusal=None, min_hosts=3, margin=0.10, max_iqr=None, expect_report=()):
    with tempfile.TemporaryDirectory() as d:
        hosts = []
        for i, rows in enumerate(host_rows):
            p = os.path.join(d, "host%d.jsonl" % (i + 1))
            with open(p, "w") as f:
                f.write("".join(json.dumps(r) + "\n" for r in rows))
            hosts.append(load(p))
        table, report, refusals = decide(hosts, margin, min_hosts, max_iqr)
    got = None if table is None else emit_yaml(table, "")
    ok = (got == expect_yaml and (expect_refusal is None or any(expect_refusal in r for r in refusals))
          and all(any(e in line for line in report) for e in expect_report))
    print("%-4s %s" % ("ok" if ok else "FAIL", case))
    if not ok:
        print("     yaml %r, refusals %r, report %r" % (got, refusals, report))
    return ok


def self_test():
    three = lambda rows: [rows, rows, rows]
    noexp = {"func": "ACOS", "wasm": False}
    skip = {"func": "SMA", "bars": 30, "skip": "bars below lookback + 2"}
    quiet = _curve("K", [0.5] * 3)
    noisy_mid = [_row("K", 100, 100.0, 50.0), _row("K", 1000, 100.0, 50.0, iqr=80.0), _row("K", 10000, 100.0, 50.0)]
    cases = [
        ("median of medians over sweeps outvotes a bad first sweep",
         [_curve("H", [2.0, 2.0, 2.0]) + _curve("H", [0.5, 0.5, 0.5]) + _curve("H", [0.6, 0.6, 0.6]),
          _curve("H", [0.5, 0.5, 0.5]), _curve("H", [0.5, 0.5, 0.5])], "batch:\n  H: 100\n"),
        ("median IQR over sweeps outvotes a noisy first sweep",
         [_curve("P", [0.5] * 3, iqr=80.0) + _curve("P", [0.5] * 3) + _curve("P", [0.5] * 3), _curve("P", [0.5] * 3),
          _curve("P", [0.5] * 3)], "batch:\n  P: 100\n"),
        ("faster only above a size", three(_curve("A", [1.2, 0.8, 0.5])), "batch:\n  A: 1000\n"),
        ("faster small, slower larger, faster largest: only the tail counts",
         three(_curve("B", [0.5, 1.2, 0.8])), "batch:\n  B: 10000\n"),
        ("faster small, slower at the largest: left out", three(_curve("C", [0.5, 0.5, 1.2])), "batch: {}\n"),
        ("one host disagreeing raises the threshold",
         [_curve("D", [0.5, 0.5, 0.5]), _curve("D", [0.5, 0.5, 0.5]), _curve("D", [0.5, 0.95, 0.5])],
         "batch:\n  D: 10000\n"),
        ("one host never faster: left out",
         [_curve("E", [0.5, 0.5, 0.5]), _curve("E", [0.5, 0.5, 0.5]), _curve("E", [1.1, 1.1, 1.1])], "batch: {}\n"),
        ("margin exactly met routes; a hair short does not",
         three(_curve("F", [0.9, 0.9, 0.9]) + _curve("G", [0.9001, 0.9001, 0.9001])), "batch:\n  F: 100\n"),
        ("no-export and skip lines are ignored", three([noexp, skip] + _curve("A", [0.5, 0.5, 0.5])), "batch:\n  A: 100\n"),
        ("median of medians over sweeps outvotes one bad sweep",
         [_curve("H", [0.5, 0.5, 0.5]) + _curve("H", [2.0, 2.0, 2.0]) + _curve("H", [0.6, 0.6, 0.6]),
          _curve("H", [0.5, 0.5, 0.5]), _curve("H", [0.5, 0.5, 0.5])], "batch:\n  H: 100\n"),
        ("passes on medians, fails on quartiles: not routed", three(_curve("L", [0.8] * 3, iqr=20.0)), "batch: {}\n"),
        ("each quartile sits half the IQR from the median: 11% routes a 0.8 ratio, 12% does not",
         three(_curve("M", [0.8] * 3, iqr=11.0) + _curve("N", [0.8] * 3, iqr=12.0)), "batch:\n  M: 100\n"),
        ("TypeScript's noise alone blocks a row",
         three([_row("O", 100, 100.0, 80.0, iqr=0.0, js_iqr=24.0)]), "batch: {}\n"),
        ("a noisy row at a larger size pushes the threshold past it",
         [quiet, quiet, noisy_mid], "batch:\n  K: 10000\n"),
        ("a quiet host is unaffected by another host's noise",
         [quiet, quiet, noisy_mid], "batch:\n  K: 10000\n", None, 3, 0.10, None,
         ("host1                0 row(s), 0 function(s)", "host2                0 row(s), 0 function(s)",
          "host3                1 row(s), 1 function(s)")),
        ("median IQR over sweeps outvotes one noisy sweep",
         [_curve("P", [0.5] * 3) + _curve("P", [0.5] * 3, iqr=80.0) + _curve("P", [0.5] * 3), _curve("P", [0.5] * 3),
          _curve("P", [0.5] * 3)], "batch:\n  P: 100\n"),
        ("missing function refuses",
         [_curve("A", [0.5] * 3) + _curve("I", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 3) + _curve("I", [0.5] * 3)],
         None, "host2: lacks I"),
        ("fail line refuses", [_curve("A", [0.5] * 3) + [{"fail": "J"}], _curve("A", [0.5] * 3), _curve("A", [0.5] * 3)],
         None, "host1: failed: J"),
        ("error line refuses", [_curve("A", [0.5] * 3) + [{"func": "A", "bars": 5, "error": "boom"}], _curve("A", [0.5] * 3),
                                _curve("A", [0.5] * 3)], None, "host1: failed: A at 5 bars: boom"),
        ("an export on one host and not another refuses",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), [{"func": "A", "wasm": False}]], None, "A: a WASM export on some hosts"),
        ("too few hosts refuses", [_curve("A", [0.5] * 3)] * 2, None, "2 host file(s) given; the table needs 3"),
        ("without --max-iqr a noisy row only raises the threshold",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 2) + [_row("A", 10000, 100.0, 50.0, iqr=25.0)]],
         "batch:\n  A: 100\n"),
        ("the --max-iqr ceiling refuses and names the row",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 2) + [_row("A", 10000, 100.0, 50.0, iqr=25.0)]],
         None, "host3: A at 10000 bars: js IQR 25.0%", 3, 0.10, 10.0),
        ("different bar counts refuse",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 2)], None, "different bar counts"),
    ]
    results = [_run(*c) for c in cases]
    print("%d/%d passed" % (sum(results), len(results)))
    return 0 if all(results) else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("files", nargs="*")
    ap.add_argument("--margin", type=float, default=0.10)
    ap.add_argument("--min-hosts", type=int, default=3)
    ap.add_argument("--max-iqr", type=float,
                    help="percent; a hard ceiling, off by default: any row whose arm's IQR exceeds this percent of its"
                         " median refuses the whole table. Without it, noise only raises thresholds")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args()
    if args.self_test:
        return self_test()
    if not 0 <= args.margin < 1:
        ap.error("--margin is in [0, 1)")
    hosts = [load(p) for p in args.files]
    table, report, refusals = decide(hosts, args.margin, args.min_hosts, args.max_iqr)
    if refusals:
        print("refused; no table written:", file=sys.stderr)
        for r in refusals:
            print("  " + r, file=sys.stderr)
        return 1
    sys.stdout.write(emit_yaml(table, header()))
    print("\n".join(report), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
