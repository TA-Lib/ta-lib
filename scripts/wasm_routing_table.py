#!/usr/bin/env python3
"""Turns wasm_routing_bench.mjs results into ta_codegen/input/wasm_routing.yaml.

Usage: python3 scripts/wasm_routing_table.py <host1.jsonl> <host2.jsonl> [...]
           [--margin=0.10] [--flat=0.05] [--min-hosts=3] [--max-iqr=PERCENT]
           [--slack=1] [--fresh] [--write]
       python3 scripts/wasm_routing_table.py --self-test

One file per timing host; a file may hold several sweeps, and each (function,
bars, arm) then takes the median over the sweeps of its median and of its IQR.
A row timed across more than MAX_MAJOR_GCS major GCs (`majorGcs`, 0 when
absent) is dropped, and the report counts the drops per sweep; a cell whose
every row is dropped is missing, as an unmeasured one is. Only --form=options
sweeps set the table: a file holding rows of another form refuses.
Prints the YAML on stdout, or with --write writes it to
ta_codegen/input/wasm_routing.yaml, and the report on stderr; writes no YAML,
and exits 1, when it refuses. The table is written with --write, not by
redirecting stdout into that file, which the shell empties before it is read.

A host routes a measured bar count when WASM's upper quartile is at most
(1 - margin) times TypeScript's lower quartile on it. A count wins when every
host routes it, is a loss when on some host WASM's median exceeds (1 + flat)
times TypeScript's, and is otherwise mixed when some host routes it, else even.
The entry is the interval of consecutive counts without a loss that starts at
a win, holds more wins than even counts, and ends at the largest count or at a
win with at least 2 wins in it; of these, the one with the most wins, then the
larger first count, then the larger last count. It is `F: B`, B its first
count, when it ends at the largest count, else `F: [LO, HI]`; none, no entry.
Quartiles, because then a row routes only when the two arms' middle halves stay
apart by the margin; the margin, because a host the table was not timed on
differs by more than the noise, and a function routed at parity buys nothing
and risks a loss.

The table in the tree is the previous one, unless --fresh. A function keeps
its previous entry P over the rule's entry E when P's first count, and a
range's last, is a measured count, no count inside P is a loss, wins outnumber
even counts inside P, and E has P's form (both open or both ranges) with at
most --slack more wins and at most --slack fewer even counts than P.
The ends of an entry move between sweeps of unchanged code, and following them
would change the table for at most --slack winning counts. A function no host
measures gets no entry, previous or not.

--max-iqr is an optional hard ceiling, off by default: any row whose arm has an
IQR above that percent of its median refuses the whole table.
"""

import argparse
import contextlib
import io
import json
import os
import re
import statistics
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROUTING_FILE = os.path.join(ROOT, "ta_codegen", "input", "wasm_routing.yaml")
EPS = 1e-12
SLACK = 1
MAX_MAJOR_GCS = 2


def load(path):
    """{'name', 'funcs': {F: None (no export) | {bars: {arm: [median...], arm+'_iqr': [...]}}}, 'fails': [...]}."""
    host = {"name": os.path.splitext(os.path.basename(path))[0], "path": path, "funcs": {}, "fails": [], "forms": set(),
            "dropped": {}}
    sweeps = {}
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
            form = row.get("form", "options")
            if form != "options":
                host["forms"].add(form)
            elif "fail" in row:
                host["fails"].append(row["fail"])
            elif "error" in row:
                host["fails"].append("%s at %s bars: %s" % (row.get("func"), row.get("bars"), row["error"]))
            elif row.get("wasm") is False:
                host["funcs"].setdefault(row["func"], None)
            elif "skip" in row:
                continue
            else:
                sweep = sweeps[row["func"], row["bars"]] = sweeps.get((row["func"], row["bars"]), 0) + 1
                if row.get("majorGcs", 0) > MAX_MAJOR_GCS:
                    host["dropped"][sweep] = host["dropped"].get(sweep, 0) + 1
                    continue
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


def is_flat(cell, flat):
    return med(cell, "wasm") <= (1 + flat) * med(cell, "js") * (1 + EPS)


def classify(func, hosts, margin, flat, test=routes):
    """(sizes, win, loss, even): `func`'s measured bar counts, ascending, and a flag per count for each class."""
    sizes = sorted(hosts[0]["funcs"][func])
    cells = [[h["funcs"][func][s] for h in hosts] for s in sizes]
    win = [all(test(c, margin) for c in row) for row in cells]
    loss = [not all(is_flat(c, flat) for c in row) for row in cells]
    even = [not w and not l and not any(test(c, margin) for c in row) for w, l, row in zip(win, loss, cells)]
    return sizes, win, loss, even


def entry(func, hosts, margin, flat, test=routes):
    """`func`'s table entry over `hosts`: B (from B on), (LO, HI) inclusive, or None."""
    return rule_entry(*classify(func, hosts, margin, flat, test))


def rule_entry(sizes, win, loss, even):
    top = len(sizes) - 1
    best = None
    for lo in range(len(sizes)):
        if not win[lo]:
            continue
        wins = evens = 0
        for hi in range(lo, len(sizes)):
            if loss[hi]:
                break
            wins += win[hi]
            evens += even[hi]
            if wins > evens and (hi == top or (win[hi] and wins >= 2)):
                best = max(best or (0,), (wins, lo, hi))
    if best is None:
        return None
    _, lo, hi = best
    return sizes[lo] if hi == top else (sizes[lo], sizes[hi])


def inside(e, sizes):
    """Indices of the measured counts entry `e` covers; an open entry runs through the largest."""
    hi = sizes[-1] if isinstance(e, int) else e[1]
    return [i for i, s in enumerate(sizes) if first(e) <= s <= hi]


def keeps(prev, rule, classes, slack):
    """Whether the previous entry `prev` stands in for the rule's entry `rule`."""
    sizes, win, loss, even = classes
    if first(prev) not in sizes or (not isinstance(prev, int) and prev[1] not in sizes):
        return False
    span = inside(prev, sizes)
    if any(loss[i] for i in span):
        return False
    wins = sum(win[i] for i in span)
    evens = sum(even[i] for i in span)
    if wins <= evens or rule is None or isinstance(rule, int) != isinstance(prev, int):
        return False
    rule_span = inside(rule, sizes)
    if sum(win[i] for i in rule_span) - wins > slack:
        return False
    return evens - sum(even[i] for i in rule_span) <= slack


def spell(e):
    if e is None:
        return "not routed"
    return "from %d" % e if isinstance(e, int) else "%d to %d" % e


def first(e):
    return e if isinstance(e, int) else e[0]


def quartile_ratio(cell):
    """WASM's upper quartile over TypeScript's lower one: the rule routes at <= 1 - margin."""
    return quartiles(cell, "wasm")[1] / quartiles(cell, "js")[0]


def decide(hosts, margin, flat, min_hosts, max_iqr=None, prev=None, slack=SLACK):
    """(table or None, report lines, refusals); `prev` is the previous table, {F: B | (LO, HI)}."""
    refusals = []
    report = ["rows dropped for more than %d major GCs while timed: " % MAX_MAJOR_GCS
              + ("; ".join("%s: %d%s" % (h["name"], sum(h["dropped"].values()),
                                         " (%s)" % ", ".join("sweep %d: %d" % kv for kv in sorted(h["dropped"].items()))
                                         if h["dropped"] else "") for h in hosts) or "none")]
    if len(hosts) < min_hosts:
        refusals.append("%d host file(s) given; the table needs %d" % (len(hosts), min_hosts))
    for h in hosts:
        for f in h["fails"]:
            refusals.append("%s: failed: %s" % (h["name"], f))
        for form in sorted(h["forms"]):
            refusals.append("%s: holds --form=%s rows; only --form=options sweeps set the table" % (h["name"], form))
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
    kept = []
    for f in timed:
        classes = classify(f, hosts, margin, flat)
        b = rule_entry(*classes)
        p = (prev or {}).get(f)
        if p is not None and p != b and keeps(p, b, classes, slack):
            kept.append((f, p, b))
            b = p
        per_host = {h["name"]: entry(f, [h], margin, flat) for h in hosts}
        if any(e != b for e in per_host.values()):
            differs.append((f, b, per_host))
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
        moved = sum(1 for f in timed if entry(f, [h], margin, flat, routes_on_medians) != entry(f, [h], margin, flat))
        noise[h["name"]] = (rows, moved)

    report.append("hosts: " + ", ".join("%s (%s)" % (h["name"], h.get("cpu", "cpu unknown")) for h in hosts))
    report.append("margin %.0f%%, flat %.0f%%; a row routes when WASM's upper quartile <= (1 - margin) x TS's lower quartile;"
                  " q-ratio is that quotient, faster/slower is 1 - WASM/TS median time, both on the host that gains least"
                  % (100 * margin, 100 * flat))
    ranged = sum(1 for b in table.values() if not isinstance(b, int))
    report.append("routed (%d, %d of them ranged); q-ratio and times at the smallest routed size:" % (len(table), ranged))
    for f, b in sorted(table.items()):
        cells = [h["funcs"][f][first(b)] for h in hosts]
        worst = max(cells, key=quartile_ratio)
        span = "from %6d bars" % b if isinstance(b, int) else "from %6d to %d bars (ranged)" % b
        report.append("  %-20s %s, q-ratio %.3f, WASM %s there (TS %.0f ns, WASM %.0f ns)"
                      % (f, span, quartile_ratio(worst), faster(med(worst, "js"), med(worst, "wasm")), med(worst, "js"), med(worst, "wasm")))
    report.append("kept from the previous table (%d), with what the rule alone gives:" % len(kept))
    for f, p, b in kept:
        report.append("  %-20s %s (rule alone: %s)" % (f, spell(p), spell(b)))
    report.append("left out (%d), best size by the worst host's q-ratio:" % len(left_out))
    for f, (ratio, s, h) in left_out:
        cell = h["funcs"][f][s]
        report.append("  %-20s q-ratio %.3f at %d bars on %s, WASM %s on medians (TS %.0f ns, WASM %.0f ns)"
                      % (f, ratio, s, h["name"], faster(med(cell, "js"), med(cell, "wasm")), med(cell, "js"), med(cell, "wasm")))
    report.append("host entries that differ from the table's (%d):" % len(differs))
    for f, b, per_host in differs:
        report.append("  %-20s table: %s; %s" % (f, spell(b), ", ".join("%s: %s" % (n, spell(e)) for n, e in per_host.items())))
    report.append("decided by the noise term, per host (rows medians alone would route and quartiles do not;"
                  " functions whose entry on that host alone it changes or removes):")
    for name, (rows, moved) in noise.items():
        report.append("  %-20s %d row(s), %d function(s)" % (name, rows, moved))
    return table, report, refusals


def header(path):
    lines = []
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                if not line.startswith("#"):
                    break
                lines.append(line)
    return "".join(lines)


def previous(path, fresh):
    """The `batch:` map of the table at `path` as {F: B | (LO, HI)}; {} with --fresh or no file."""
    table = {}
    if not os.path.exists(path):
        return table
    in_batch = found = False
    with open(path) as f:
        for n, line in enumerate(f, 1):
            text = line.split("#")[0].rstrip()
            if not text:
                continue
            if not line[0].isspace():
                in_batch = text == "batch:"
                if text.startswith("batch:"):
                    if not re.fullmatch(r"batch:(\s*\{\s*\})?", text):
                        raise ValueError("%s line %d: the batch map is neither a block map nor {}" % (path, n))
                    found = True
                continue
            if not in_batch:
                continue
            m = re.fullmatch(r"\s+(\w+):\s*(?:(\d+)|\[\s*(\d+)\s*,\s*(\d+)\s*\])", text)
            if not m:
                raise ValueError("%s line %d: not `F: N` or `F: [LO, HI]`" % (path, n))
            table[m[1]] = int(m[2]) if m[2] else (int(m[3]), int(m[4]))
    if not found:
        raise ValueError("%s has no batch map; redirecting the output into it empties it first: use --write" % path)
    return {} if fresh else table


def emit_yaml(table, head):
    if not table:
        return head + "batch: {}\n"
    return head + "batch:\n" + "".join(("  %s: %d\n" % (f, b) if isinstance(b, int) else "  %s: [%d, %d]\n" % ((f,) + b))
                                        for f, b in sorted(table.items()))


# --- self-test -------------------------------------------------------------

def _row(func, bars, js, wasm, iqr=0.0, js_iqr=None, **extra):
    return dict({"func": func, "bars": bars, "js": {"median": js, "iqr": iqr if js_iqr is None else js_iqr},
                 "wasm": {"median": wasm, "iqr": iqr}}, **extra)


def _curve(func, ratios, sizes=(100, 1000, 10000), iqr=0.0, **extra):
    return [_row(func, s, 100.0, 100.0 * r, iqr, **extra) for s, r in zip(sizes, ratios)]


def _run(case, host_rows, expect_yaml, expect_refusal=None, min_hosts=3, margin=0.10, max_iqr=None, expect_report=(),
         flat=0.05, prev=None, fresh=False, slack=None):
    with tempfile.TemporaryDirectory() as d:
        hosts = []
        for i, rows in enumerate(host_rows):
            p = os.path.join(d, "host%d.jsonl" % (i + 1))
            with open(p, "w") as f:
                f.write("".join(json.dumps(r) + "\n" for r in rows))
            hosts.append(load(p))
        prev_table = None
        if prev is not None:
            p = os.path.join(d, "wasm_routing.yaml")
            with open(p, "w") as f:
                f.write("# header\n" + prev)
            prev_table = previous(p, fresh)
        kw = {} if slack is None else {"slack": slack}
        table, report, refusals = decide(hosts, margin, flat, min_hosts, max_iqr, prev_table, **kw)
    got = None if table is None else emit_yaml(table, "")
    ok = (got == expect_yaml and (expect_refusal is None or any(expect_refusal in r for r in refusals))
          and all(any(e in line for line in report) for e in expect_report))
    print("%-4s %s" % ("ok" if ok else "FAIL", case))
    if not ok:
        print("     yaml %r, refusals %r, report %r" % (got, refusals, report))
    return ok


def _file_cases():
    with tempfile.TemporaryDirectory() as d:
        host = os.path.join(d, "host1.jsonl")
        with open(host, "w") as f:
            f.write("".join(json.dumps(r) + "\n" for r in _curve("A", [0.5] * 3)))
        routing = os.path.join(d, "wasm_routing.yaml")

        def run(text, *args):
            with open(routing, "w") as f:
                f.write(text)
            out, err = io.StringIO(), io.StringIO()
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                rc = main([host, "--min-hosts=1"] + list(args), routing)
            with open(routing) as f:
                return rc, out.getvalue(), err.getvalue(), f.read()

        def raises(text):
            with open(routing, "w") as f:
                f.write(text)
            try:
                previous(routing, False)
            except ValueError:
                return True
            return False

        rc, out, err, after = run("")
        empty = rc == 1 and out == "" and "use --write" in err and after == ""
        rc, out, err, after = run("", "--fresh")
        empty_fresh = rc == 1 and out == "" and "use --write" in err and after == ""
        holds = "# h\nbatch:\n  A: 1000\n"
        kept = run(holds)[:2] == (0, holds)
        fresh = run(holds, "--fresh")[:2] == (0, "# h\nbatch:\n  A: 100\n")
        strict = run(holds, "--slack=0")[:2] == (0, "# h\nbatch:\n  A: 100\n")
        rc, out, _, _ = run("# h\nbatch: {}\n")
        flow_empty = rc == 0 and out == "# h\nbatch:\n  A: 100\n"
        flow = raises("batch: {SMA: 100}\n")
        rc, out, _, after = run("# h1\n# h2\nbatch:\n  A: 500\n", "--write")
        written = rc == 0 and out == "" and after == "# h1\n# h2\nbatch:\n  A: 100\n"
        table = "# h\nbatch:\n  A: 500\n"
        rc, out, _, after = run(table, "--write", "--min-hosts=2")
        refused = rc == 1 and out == "" and after == table
    checks = [
        ("a table file that exists but is empty refuses and names --write", empty),
        ("a table file that exists but is empty refuses with --fresh too", empty_fresh),
        ("through main, an entry that holds is kept", kept),
        ("through main, --fresh replaces it", fresh),
        ("through main, --slack=0 replaces it", strict),
        ("`batch: {}` reads as an empty table", flow_empty),
        ("a flow-style batch map that is not empty refuses", flow),
        ("--write writes the header and the table to the file and nothing to stdout", written),
        ("a refusal with --write leaves the file as it was", refused),
    ]
    for case, ok in checks:
        print("%-4s %s" % ("ok" if ok else "FAIL", case))
    return [ok for _, ok in checks]


def self_test():
    three = lambda rows: [rows, rows, rows]
    noexp = {"func": "ACOS", "wasm": False}
    skip = {"func": "SMA", "bars": 30, "skip": "bars below lookback + 2"}
    quiet = _curve("K", [0.5] * 3)
    sizes5 = (100, 1000, 10000, 100000, 1000000)
    sizes10 = tuple(2 ** i for i in range(10))
    noisy_mid = [_row("K", 100, 100.0, 50.0), _row("K", 1000, 100.0, 50.0, iqr=80.0), _row("K", 10000, 100.0, 50.0)]
    noisy_low = [_row("K", 100, 100.0, 50.0, iqr=80.0), _row("K", 1000, 100.0, 50.0), _row("K", 10000, 100.0, 50.0)]
    top = lambda name, r: [_curve(name, [1.2, 0.5, 0.5, r], sizes5[:4]), _curve(name, [1.2, 0.5, 0.5, 0.5], sizes5[:4]),
                           _curve(name, [1.2, 0.5, 0.5, 0.5], sizes5[:4])]
    z = lambda ratios: three(_curve("Z", ratios, sizes5))
    after = lambda entries, **kw: dict(prev="batch:\n  " + entries + "\n", **kw)
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
        ("faster small, slower at the largest: a closed range", three(_curve("C", [0.5, 0.5, 1.2])), "batch:\n  C: [100, 1000]\n"),
        ("faster at the middle sizes, parity at the largest: even, so open",
         three(_curve("Q", [1.2, 0.5, 0.5, 1.0], sizes5[:4])), "batch:\n  Q: 1000\n"),
        ("faster at the middle sizes, slower at the largest: a closed range",
         three(_curve("Q", [1.2, 0.5, 0.5, 1.2], sizes5[:4])), "batch:\n  Q: [1000, 10000]\n", None, 3, 0.10, None,
         ("Q                    from   1000 to 10000 bars (ranged)",)),
        ("4% slower at the largest on one host is not a loss: open", top("X", 1.04), "batch:\n  X: 1000\n"),
        ("one win then even to the largest: left out",
         three(_curve("Z", [0.89, 1.04, 1.04, 1.04, 1.04], sizes5)), "batch: {}\n"),
        ("two wins then three even: not open, so the range of the two",
         three(_curve("Z", [0.5, 0.5, 1.04, 1.04, 1.04], sizes5)), "batch:\n  Z: [100, 1000]\n"),
        ("three wins then two even: open",
         three(_curve("Z", [0.5, 0.5, 0.5, 1.04, 1.04], sizes5)), "batch:\n  Z: 100\n"),
        ("6% slower at the largest on one host is a loss: a range", top("X", 1.06), "batch:\n  X: [1000, 10000]\n"),
        ("hosts that route differently are reported, even when the table routes",
         [_curve("A", [0.5] * 3), _curve("A", [0.5, 0.5, 1.2]), _curve("A", [0.5] * 3)], "batch:\n  A: [100, 1000]\n",
         None, 3, 0.10, None, ("A                    table: 100 to 1000; host1: from 100, host2: 100 to 1000, host3: from 100",)),
        ("each host routes but some host loses at every size: left out and reported",
         [_curve("B", [0.5, 0.5, 1.2, 1.2], sizes5[:4]), _curve("B", [1.2, 1.2, 0.5, 0.5], sizes5[:4]),
          _curve("B", [0.5] * 4, sizes5[:4])], "batch: {}\n",
         None, 3, 0.10, None, ("B                    table: not routed; host1: 100 to 1000, host2: from 10000, host3: from 100",)),
        ("every host gives a range but some host loses at every size: left out and reported",
         [_curve("Y", [0.5, 0.5, 1.2, 1.2, 1.2], sizes5), _curve("Y", [1.2, 1.2, 0.5, 0.5, 1.2], sizes5),
          _curve("Y", [0.5, 0.5, 1.2, 1.2, 1.2], sizes5)], "batch: {}\n",
         None, 3, 0.10, None, ("Y                    table: not routed; host1: 100 to 1000, host2: 10000 to 100000, host3: 100 to 1000",)),
        ("one faster size below a slower one: left out", three(_curve("R", [1.2, 0.5, 1.2])), "batch: {}\n"),
        ("a range of 3 wins beats an open entry of 1 win",
         three(_curve("S", [0.5, 0.5, 0.5, 1.2, 0.5], sizes5)), "batch:\n  S: [100, 10000]\n"),
        ("a range of 8 wins beats an open entry of 1 win",
         three(_curve("S", [0.5] * 8 + [1.2, 0.5], sizes10)), "batch:\n  S: [1, 128]\n"),
        ("the most wins, not the highest interval",
         three(_curve("S", [0.5, 0.5, 0.5, 1.2, 0.5, 0.5, 1.2], sizes10[:7])), "batch:\n  S: [1, 4]\n"),
        ("two wins then two even to the largest: not open, so the range of the two",
         three(_curve("Z", [0.5, 0.5, 1.03, 1.03], sizes5[:4])), "batch:\n  Z: [100, 1000]\n"),
        ("three wins then two even to the largest: open",
         three(_curve("Z", [0.5, 0.5, 0.5, 1.03, 1.03], sizes5)), "batch:\n  Z: 100\n"),
        ("a mixed size counts for neither side: open from the first win",
         [_curve("M2", [0.5, 0.5, 0.5, 0.5, 1.03, 1.03, 1.03], sizes10[:7]),
          _curve("M2", [1.03, 1.03, 0.5, 0.5, 0.5, 0.5, 0.5], sizes10[:7]),
          _curve("M2", [1.03, 1.03, 0.5, 0.5, 0.5, 0.5, 0.5], sizes10[:7])], "batch:\n  M2: 4\n",
         None, 3, 0.10, None, ("M2                   table: from 4; host1: from 1, host2: from 4, host3: from 4",)),
        ("win win even loss: the range of the two wins",
         three(_curve("E", [0.5, 0.5, 1.03, 1.2], sizes5[:4])), "batch:\n  E: [100, 1000]\n"),
        ("win even even win loss: wins do not outnumber even sizes, left out",
         three(_curve("E", [0.5, 1.03, 1.03, 0.5, 1.2], sizes5)), "batch: {}\n"),
        ("even even win win: open from the first win",
         three(_curve("E", [1.03, 1.03, 0.5, 0.5], sizes5[:4])), "batch:\n  E: 10000\n"),
        ("win even loss win: open from the last win",
         three(_curve("E", [0.5, 1.03, 1.2, 0.5], sizes5[:4])), "batch:\n  E: 100000\n"),
        ("win even even win, the previous case with its loss made even: open from the last win",
         three(_curve("E", [0.5, 1.03, 1.03, 0.5], sizes5[:4])), "batch:\n  E: 100000\n"),
        ("an even size inside an open entry: open from the first win",
         three(_curve("E", [0.5, 0.5, 1.03, 0.5], sizes5[:4])), "batch:\n  E: 100\n"),
        ("open and closed from the same win with the same wins: open",
         three(_curve("E", [0.5, 0.5, 1.03])), "batch:\n  E: 100\n"),
        ("hosts whose own thresholds differ from the table's are reported",
         [_curve("W", [0.5, 0.5, 0.5]), _curve("W", [1.2, 0.5, 0.5]), _curve("W", [1.2, 0.5, 0.5])], "batch:\n  W: 1000\n",
         None, 3, 0.10, None, ("W                    table: from 1000; host1: from 100, host2: from 1000, host3: from 1000",
                               "host entries that differ from the table's (1):")),
        ("exactly 5% slower at the largest on one host is not a loss: open",
         top("X", 1.05), "batch:\n  X: 1000\n"),
        ("a hair over 5% slower at the largest on one host is a loss: a range",
         top("X", 1.0501), "batch:\n  X: [1000, 10000]\n"),
        ("two intervals of 2 wins tie: the one at larger sizes, open",
         three(_curve("T", [0.5, 0.5, 1.2, 0.5, 0.5], sizes5)), "batch:\n  T: 100000\n"),
        ("of two ranges of 2 wins, the one at larger sizes",
         three(_curve("U", [0.5, 0.5, 1.2, 0.5, 0.5, 1.2], sizes5 + (10000000,))), "batch:\n  U: [100000, 1000000]\n"),
        ("a host decision that is a range is reported as one",
         [_curve("V", [0.5, 0.5, 1.2]), _curve("V", [1.2, 1.2, 1.2]), _curve("V", [1.2, 1.2, 1.2])], "batch: {}\n",
         None, 3, 0.10, None, ("table: not routed; host1: 100 to 1000, host2: not routed",)),
        ("one host at parity in the middle: mixed, so the entry stays open from the smallest size",
         [_curve("D", [0.5, 0.5, 0.5]), _curve("D", [0.5, 0.5, 0.5]), _curve("D", [0.5, 0.95, 0.5])],
         "batch:\n  D: 100\n"),
        ("one host slower in the middle raises the threshold",
         [_curve("D2", [0.5, 0.5, 0.5]), _curve("D2", [0.5, 0.5, 0.5]), _curve("D2", [0.5, 1.2, 0.5])],
         "batch:\n  D2: 10000\n"),
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
        ("a noisy row at the smallest size raises the threshold past it",
         [quiet, quiet, noisy_low], "batch:\n  K: 1000\n"),
        ("a noisy row in the middle that wins on medians is mixed: the entry stays open",
         [quiet, quiet, noisy_mid], "batch:\n  K: 100\n"),
        ("a quiet host is unaffected by another host's noise",
         [quiet, quiet, noisy_low], "batch:\n  K: 1000\n", None, 3, 0.10, None,
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
        ("without --max-iqr a noisy row that still routes changes nothing",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 2) + [_row("A", 10000, 100.0, 50.0, iqr=25.0)]],
         "batch:\n  A: 100\n"),
        ("the --max-iqr ceiling refuses and names the row",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 2) + [_row("A", 10000, 100.0, 50.0, iqr=25.0)]],
         None, "host3: A at 10000 bars: js IQR 25.0%", 3, 0.10, 10.0),
        ("different bar counts refuse",
         [_curve("A", [0.5] * 3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 2)], None, "different bar counts"),
        ("kept: valid, and the rule's entry holds one more win", z([0.5] * 5), "batch:\n  Z: 1000\n",
         after("Z: 1000", expect_report=("kept from the previous table (1)",
                                         "Z                    from 1000 (rule alone: from 100)"))),
        ("replaced: the rule's entry holds two more wins", z([0.5] * 5), "batch:\n  Z: 100\n", after("Z: 10000")),
        ("replaced: a loss inside the previous entry", z([0.5, 1.2, 0.5, 0.5, 0.5]), "batch:\n  Z: 10000\n",
         after("Z: 100")),
        ("replaced: wins do not outnumber even counts inside the previous entry",
         three(_curve("Z", [0.5, 1.03, 1.03, 0.5, 0.5, 1.2], sizes5 + (10000000,))), "batch:\n  Z: [100, 1000000]\n",
         after("Z: [100, 100000]")),
        ("replaced: the previous entry's first count is not measured", z([0.5] * 5), "batch:\n  Z: 100\n",
         after("Z: 500")),
        ("replaced: the previous range's last count is not measured", z([0.5] * 4 + [1.2]),
         "batch:\n  Z: [100, 100000]\n", after("Z: [100, 50000]")),
        ("kept: a range", z([0.5] * 4 + [1.2]), "batch:\n  Z: [1000, 100000]\n", after("Z: [1000, 100000]")),
        ("dropped: the rule alone gives no entry, though the previous entry holds", three(_curve("R", [1.2, 0.5, 1.2])),
         "batch: {}\n", after("R: [1000, 1000]")),
        ("dropped: the previous entry fails and the rule alone gives no entry", three(_curve("R", [1.2, 0.5, 1.2])),
         "batch: {}\n", after("R: 1000")),
        ("--slack=0 replaces an entry the rule's beats by one win", z([0.5] * 5), "batch:\n  Z: 100\n",
         after("Z: 1000", slack=0)),
        ("--slack=0 keeps an entry with as many wins and even counts as the rule's",
         [_curve("Z", [0.5] * 5, sizes5), _curve("Z", [1.03] + [0.5] * 4, sizes5), _curve("Z", [1.03] + [0.5] * 4, sizes5)],
         "batch:\n  Z: 100\n", after("Z: 100", slack=0)),
        ("replaced: an open entry against a ranged one", three(_curve("Z", [0.5, 0.5, 1.2, 0.5], sizes5[:4])), "batch:\n  Z: [100, 1000]\n", after("Z: 100000")),
        ("replaced: a ranged entry against an open one", z([0.5] * 5), "batch:\n  Z: 100\n", after("Z: [100, 100000]")),
        ("kept: one more even count than the rule's entry", z([1.03] + [0.5] * 4), "batch:\n  Z: 100\n",
         after("Z: 100")),
        ("replaced: two more even counts than the rule's entry", z([1.03, 1.03, 0.5, 0.5, 0.5]), "batch:\n  Z: 10000\n",
         after("Z: 100")),
        ("--fresh ignores the previous table", z([0.5] * 5), "batch:\n  Z: 100\n", after("Z: 1000", fresh=True)),
        ("a previous entry equal to the rule's is not reported as kept", z([0.5] * 5), "batch:\n  Z: 100\n",
         after("Z: 100", expect_report=("kept from the previous table (0)",))),
        ("a previous entry for a function no host measures, or with no export, is dropped",
         three([noexp] + _curve("A", [0.5] * 3)), "batch:\n  A: 100\n", after("A: 100\n  ACOS: 100\n  GONE: 100")),
        ("a sweep timed across more than 2 major GCs is dropped, and the drops are counted per sweep",
         [_curve("H", [2.0] * 3, majorGcs=5) + _curve("H", [0.5] * 3), _curve("H", [0.5] * 3), _curve("H", [0.5] * 3)],
         "batch:\n  H: 100\n", None, 3, 0.10, None, ("host1: 3 (sweep 1: 3); host2: 0; host3: 0",)),
        ("a sweep timed across exactly 2 major GCs counts",
         [_curve("H", [2.0] * 3, majorGcs=2) + _curve("H", [0.5] * 3), _curve("H", [0.5] * 3), _curve("H", [0.5] * 3)],
         "batch: {}\n", None, 3, 0.10, None, ("host1: 0; host2: 0; host3: 0",)),
        ("a cell whose every row is dropped is missing: different bar counts refuse",
         [_curve("A", [0.5] * 2) + [_row("A", 10000, 100.0, 50.0, majorGcs=3)], _curve("A", [0.5] * 3), _curve("A", [0.5] * 3)],
         None, "different bar counts", 3, 0.10, None, ("host1: 1 (sweep 1: 1); host2: 0",)),
        ("a function whose every row is dropped on a host is missing there: refuses",
         [_curve("A", [0.5] * 3, majorGcs=3), _curve("A", [0.5] * 3), _curve("A", [0.5] * 3)], None, "host1: lacks A"),
        ("rows that name --form=options load", three(_curve("A", [0.5] * 3, form="options", majorGcs=0)), "batch:\n  A: 100\n"),
        ("a file holding --form=buffered rows refuses",
         [_curve("A", [0.5] * 3, form="buffered"), _curve("A", [0.5] * 3), _curve("A", [0.5] * 3)], None,
         "host1: holds --form=buffered rows"),
    ]
    results = [_run(*c[:-1], **c[-1]) if isinstance(c[-1], dict) else _run(*c) for c in cases] + _file_cases()
    print("%d/%d passed" % (sum(results), len(results)))
    return 0 if all(results) else 1


def main(argv=None, routing=ROUTING_FILE):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("files", nargs="*")
    ap.add_argument("--margin", type=float, default=0.10)
    ap.add_argument("--flat", type=float, default=0.05,
                    help="a size where no host has WASM slower by more than this fraction, by median, is not a loss")
    ap.add_argument("--min-hosts", type=int, default=3)
    ap.add_argument("--max-iqr", type=float,
                    help="percent; a hard ceiling, off by default: any row whose arm's IQR exceeds this percent of its"
                         " median refuses the whole table")
    ap.add_argument("--slack", type=int, default=SLACK,
                    help="a previous entry with measured ends, no loss and more wins than even counts stays only"
                         " while the rule's own entry has its form with at most this many more wins and at most this"
                         " many fewer even counts")
    ap.add_argument("--fresh", action="store_true",
                    help="ignore the table in the tree: every entry comes from the rule alone")
    ap.add_argument("--write", action="store_true",
                    help="write the table to ta_codegen/input/wasm_routing.yaml instead of stdout; redirecting stdout"
                         " into that file empties it before it is read")
    ap.add_argument("--self-test", action="store_true")
    args = ap.parse_args(argv)
    if args.self_test:
        return self_test()
    if not 0 <= args.margin < 1:
        ap.error("--margin is in [0, 1)")
    if not 0 <= args.flat < 1:
        ap.error("--flat is in [0, 1)")
    if args.slack < 0:
        ap.error("--slack is at least 0")
    try:
        prev = previous(routing, args.fresh)
    except ValueError as e:
        print("refused; no table written:\n  %s" % e, file=sys.stderr)
        return 1
    hosts = [load(p) for p in args.files]
    table, report, refusals = decide(hosts, args.margin, args.flat, args.min_hosts, args.max_iqr, prev, args.slack)
    if refusals:
        print("refused; no table written:", file=sys.stderr)
        for r in refusals:
            print("  " + r, file=sys.stderr)
        print("\n".join(report), file=sys.stderr)
        return 1
    text = emit_yaml(table, header(routing))
    if args.write:
        with open(routing, "w") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    print("\n".join(report), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
