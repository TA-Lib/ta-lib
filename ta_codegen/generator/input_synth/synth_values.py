#!/usr/bin/env python3
"""Golden-value leg of the synthetic gate — the SYNTH family's only oracle.

The two legs `synth_gate.py` already runs are both COMPARATIVE: `--codegen`
diffs each fixture's streaming tier against its own batch tier, and
`--xlang-hash` diffs Rust/Java/C# against the in-process C. Neither can see a
fixture that is wrong the SAME way everywhere — a generator defect that lands
identically in all four backends, or an input `.c` that never computed what its
author meant. Both legs stay green while the whole family computes nonsense
together.

This leg closes that: every value below was derived by hand from the fixture's
`synth<n>.c`, independently of the generator and of all four backends, and is
compared against what each server actually returns. It is the SYNTH counterpart
of `test_<name>.c`'s hardcoded expected values for a real indicator, and of the
book-sourced golden vectors the private oracle servers replay — a small,
hand-picked set of specific inputs, not a sweep. The sweep is the other two
legs' job.

Deliberately NOT in `src/tools/ta_regtest/ta_test_func/`: a `TA_SYNTH<n>` symbol
exists only inside the throwaway worktree `synth_gate.py` builds, so a test file
registered in `CMakeLists.txt` / `Makefile.am` would break every ordinary build.
It lives here, beside the fixtures whose values it pins, for the same reason the
`.md` does.

All four servers are checked, not just C. `--xlang-hash` already proves the
other three match C bit-for-bit, so C alone would be enough IF that gate's own
comparison were beyond doubt for every shape — checking all four costs three
more subprocess calls and removes the "unless the comparator is blind here"
clause entirely.

Comparison is EXACT, reals included: every fixture is built from +, -, *, / and
comparisons, all correctly rounded by IEEE-754, so identical operation sequences
must give identical bits in all four languages. That is the same property
`--xlang-hash` asserts; a tolerance here would only hide a real divergence.

Run:  python3 synth_values.py <bin-dir>     (invoked by synth_gate.py)
Exit code 0 on success, 1 on any mismatch.
"""

import json
import os
import struct
import subprocess
import sys

OK = "\033[32mok\033[0m" if sys.stdout.isatty() else "ok"
FAIL = "\033[31mFAIL\033[0m" if sys.stdout.isatty() else "FAIL"

# --------------------------------------------------------------------------- #
# Inputs. One `inReal` series for the real-input fixtures and one OHLC bundle
# for the price-input ones. Both deliberately carry a negative bar, an exact
# zero, a value past the fixtures' 1e6 clamp and a fractional value, so the
# guards every fixture opens with are actually taken rather than merely present.
# --------------------------------------------------------------------------- #
IN_REAL = [100.0, 250.5, -30.0, 2000000.0, 0.0, 50.25, -0.001, 5.5,
           300.0, 700.0, 12.0, 88.0, 1500.0, 45.0, 999.9, 220.0]

OPEN  = [10.0, 11.5, 20.0, 5.0, 100.0, 50.0, 30.0, 8.0, 60.0, 40.0, 25.0, 70.0, 15.0, 33.0, 90.0]
HIGH  = [12.0, 11.6, 25.0, 5.1, 110.0, 52.0, 30.5, 9.0, 65.0, 41.0, 27.0, 72.0, 16.0, 34.0, 95.0]
LOW   = [ 9.0, 11.4, 15.0, 4.9,  90.0, 48.0, 29.0, 7.5, 58.0, 39.5, 24.0, 69.0, 14.0, 32.0, 85.0]
CLOSE = [11.5, 11.55, 22.0, 5.05, 95.0, 51.0, 29.2, 8.8, 63.0, 40.2, 26.5, 71.5, 15.8, 33.5, 92.0]

PERIOD = 4   # optInTimePeriod for the fixtures that take one

# --------------------------------------------------------------------------- #
# The golden table.
#
# `beg` is outBegIdx; `outs` maps each output's DECLARED name (the .yaml order,
# which is also the order the JSON keys are ranked in) to its expected values.
# Every number was computed by hand from the fixture's own `synth<n>.c` — read
# the C, not the generated output — so this table shares no code with anything
# it checks.
# --------------------------------------------------------------------------- #
GOLDEN = {
    "TA_SYNTH1": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=3,
        outs={"outInteger": [788, 167, 85, 700, 473, 958, 23, 217, 9, 700, 141, 697, 824]}),

    "TA_SYNTH2": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=0,
        outs={"outInteger": [24976, 25578, 9352, 8192, 8192, 24777, 8192, 20502,
                             25776, 27376, 24624, 24928, 26480, 24756, 28575, 25456]}),

    "TA_SYNTH3": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=0,
        outs={"outInteger": [12779628, 16187650, 16187400, 16187400, 16187400, 16187450,
                             16187400, 12779533, 12779828, 12780228, 12779540, 16187488,
                             12781028, 12779573, 12649455, 12779748]}),

    # SYNTH13's four legs all recompute the same SMA, so the value is 4x it.
    # Derived from IN_REAL here rather than read back from the library: a golden
    # row transcribed from the implementation it checks proves only that the
    # implementation did not change. The running-window accumulation order is
    # TA_SMA's own -- a naive sum over each slice differs in the last bits.
    "TA_SYNTH13": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=3,
        outs={"outReal": [2000320.5, 2000220.5, 2000020.25, 2000050.249,
                          55.74900000006892, 355.7490000000689, 1005.4990000000689,
                          1017.5000000000689, 1100.000000000069, 2300.000000000069,
                          1645.0000000000691, 2632.900000000069, 2764.900000000069]}),

    "TA_SYNTH4": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=3,
        outs={"outInteger": [357, 214, 289, 173, 121, 9, 811, 730, 310, 480, 432, 182, 813]}),

    # SYNTH5 and SYNTH6 are the same window walked two ways (PRAGMA TA_ALT), so
    # they must agree value-for-value as well as each matching this table.
    "TA_SYNTH5": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=3,
        outs={"outInteger": [100, 250, 50, 0, 5, 350, 700, 17, 388, 152, 57, 63, 696]}),

    "TA_SYNTH6": dict(
        params={"inReal": IN_REAL, "optInTimePeriod": PERIOD}, beg=3,
        outs={"outInteger": [100, 250, 50, 0, 5, 350, 700, 17, 388, 152, 57, 63, 696]}),

    "TA_SYNTH7": dict(
        params={"inOpen": OPEN, "inHigh": HIGH, "inLow": LOW, "inClose": CLOSE}, beg=0,
        outs={
            "outRealBodyRange": [1.5, 0.05000000000000071, 2.0, 0.04999999999999982, 5.0,
                                 1.0, 0.8000000000000007, 0.8000000000000007, 3.0,
                                 0.20000000000000284, 1.5, 1.5, 0.8000000000000007, 0.5, 2.0],
            "outHighLowRange": [3.0, 0.1999999999999993, 10.0, 0.1999999999999993, 20.0,
                                4.0, 1.5, 1.5, 7.0, 1.5, 3.0, 3.0, 2.0, 2.0, 10.0],
            "outShadowsRange": [1.5, 0.14999999999999858, 8.0, 0.14999999999999947, 15.0,
                                3.0, 0.6999999999999993, 0.6999999999999993, 4.0,
                                1.2999999999999972, 1.5, 1.5, 1.1999999999999993, 1.5, 8.0],
        }),

    "TA_SYNTH8": dict(
        params={"inOpen": OPEN, "inHigh": HIGH, "inLow": LOW, "inClose": CLOSE}, beg=10,
        outs={
            "outAvgShadows": [1.7249999999999996, 1.7249999999999996, 1.7924999999999998,
                              1.4524999999999997, 1.5199999999999998],
            "outAvgCurrentBar": [3.0, 3.0, 1.6000000000000014, 1.0, 4.0],
        }),

    "TA_SYNTH9": dict(
        params={"inHigh": HIGH, "inLow": LOW, "inClose": CLOSE}, beg=0,
        outs={
            "outNegExp": [0.00250000000115, 0.002500000001155, 0.0025000000022,
                          0.002500000000505, 0.0025000000095000002, 0.0025000000051,
                          0.0025000000029200002, 0.00250000000088, 0.0025000000063,
                          0.00250000000402, 0.00250000000265, 0.0025000000071500003,
                          0.00250000000158, 0.00250000000335, 0.0025000000092],
            "outPosExp": [305.000003, 305.0000002, 305.00001, 305.0000002, 305.00002,
                          305.000004, 305.0000015, 305.0000015, 305.000007, 305.0000015,
                          305.000003, 305.000003, 305.000002, 305.000002, 305.00001],
            "outBigExp": [1.1499999999999999e-299, 1.1550000000000001e-299, 2.2e-299,
                          5.0499999999999996e-300, 9.499999999999999e-299, 5.1e-299,
                          2.92e-299, 8.8e-300, 6.3e-299, 4.02e-299, 2.6499999999999997e-299,
                          7.15e-299, 1.58e-299, 3.35e-299, 9.199999999999999e-299],
        }),

    "TA_SYNTH10": dict(
        params={"inReal": IN_REAL}, beg=0,
        outs={
            "outFirstOptional": [50.0, 125.25, -15.0, 1000000.0, 0.0, 25.125, -0.0005, 2.75,
                                 150.0, 350.0, 6.0, 44.0, 750.0, 22.5, 499.95, 110.0],
            "outRequired": IN_REAL,
            "outSecondOptional": [25.0, 62.625, -7.5, 500000.0, 0.0, 12.5625, -0.00025,
                                  1.375, 75.0, 175.0, 3.0, 22.0, 375.0, 11.25, 249.975, 55.0],
        }),

    "TA_SYNTH11": dict(
        params={"inReal": IN_REAL}, beg=0,
        outs={
            "outAbove": [1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            "outBelow": [0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            "outLarge": [0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0],
        }),

    # The mixed-type fixture. Its real outputs are ranked "outReal"/"outReal1"
    # in the JSON while the integer between them is "outInteger" — the per-type
    # ranking that a positional reader gets wrong at two of these three slots.
    "TA_SYNTH12": dict(
        params={"inReal": IN_REAL}, beg=0,
        outs={
            "outHalf": [50.0, 125.25, -15.0, 1000000.0, 0.0, 25.125, -0.0005, 2.75,
                        150.0, 350.0, 6.0, 44.0, 750.0, 22.5, 499.95, 110.0],
            "outSign": [1, 1, -1, 1, 0, 1, -1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
            "outQuarter": [25.0, 62.625, -7.5, 500000.0, 0.0, 12.5625, -0.00025, 1.375,
                           75.0, 175.0, 3.0, 22.0, 375.0, 11.25, 249.975, 55.0],
        }),

    # The composed tier's mixed-output case: SMA via a cross-call, read three
    # ways. It takes its OWN input series rather than IN_REAL because every
    # 4-bar average of IN_REAL is positive, which would leave outSide a column
    # of 1s and the integer output's other two arms unexercised. This series
    # crosses zero and lands on it exactly, so all three of -1/0/1 appear.
    "TA_SYNTH14": dict(
        params={"inReal": [100.0, -100.0, 50.0, -50.0, 0.0, -20.0, -30.0, 200.0,
                           -5.5, 5.5, -1.0, 1.0, 300.0, -300.0, 0.0, 12.0],
                "optInTimePeriod": PERIOD}, beg=3,
        outs={
            "outAvg": [0.0, -12.5, -2.5, -12.5, 18.75, 18.0625, 21.25, 24.875,
                       0.0, 38.1875, 0.0, 0.125, 1.5],
            "outSide": [0, -1, -1, -1, 1, 1, 1, 1, 0, 1, 0, 1, 1],
            "outTwice": [0.0, -50.0, -10.0, -50.0, 75.0, 72.25, 85.0, 99.5,
                         0.0, 152.75, 0.0, 0.5, 6.0],
        }),
}


def declared_outputs(fixture_dir):
    """(name, is_integer) per output, in declaration order, read from the
    fixture's own .yaml.

    Taken from the declaration rather than inferred from how the golden
    literals happen to be spelled: a real output whose every value is integral
    would be indistinguishable from an integer one under `isinstance(v, int)`,
    and the JSON key this picks depends on getting that right. Reading the
    .yaml also makes a type changed there without a matching golden update a
    failure here rather than a silently different key.

    The parse is deliberately narrow — the `outputs:` block of a SYNTH fixture
    is a flat list of `- name:` / `type:` pairs — and raises rather than
    guessing when it sees anything else.
    """
    path = os.path.join(fixture_dir, os.path.basename(fixture_dir) + ".yaml")
    outs, in_block, name = [], False, None
    for raw in open(path, encoding="utf-8"):
        line = raw.rstrip("\n")
        if line.startswith("outputs:"):
            in_block = True
            continue
        if in_block:
            if line and not line[0].isspace():
                break                      # next top-level key ends the block
            s = line.strip()
            if s.startswith("#") or not s:
                continue
            if s.startswith("- name:"):
                name = s.split(":", 1)[1].strip()
            elif s.startswith("type:") and name is not None:
                outs.append((name, s.split(":", 1)[1].strip() == "integer"))
                name = None
    if not outs:
        raise RuntimeError(f"no outputs parsed from {path}")
    return outs


def to_hex(vals):
    """Lossless hex-of-IEEE-bits, the #115 input transport."""
    return "".join(struct.pack(">d", float(v)).hex() for v in vals)


def from_hex(s):
    raw = bytes.fromhex(s.strip())
    return [struct.unpack(">d", raw[i:i + 8])[0] for i in range(0, len(raw), 8)]


def json_key(names, is_int, idx):
    """The response key for output `idx`: type name + rank among SAME-typed
    outputs, rank omitted at 0. Mirrors `output_json_key` (server_gen.rs) and
    `codegen_output_field` (test_codegen.c). Positional numbering is what this
    spelling exists to not be."""
    base = "outInteger" if is_int[idx] else "outReal"
    rank = sum(1 for k in range(idx) if is_int[k] == is_int[idx])
    return base if rank == 0 else f"{base}{rank}"


class Server:
    def __init__(self, name, argv, cwd):
        self.name = name
        self.p = subprocess.Popen(argv, cwd=cwd, stdin=subprocess.PIPE,
                                  stdout=subprocess.PIPE, stderr=sys.stderr,
                                  text=True, bufsize=1)

    def call(self, method, params):
        # Compact separators and the params nesting are both load-bearing: the C
        # server scans for the literal `"field":`, and the Rust server reads
        # req["params"] and panics on a flat request.
        req = {"method": method, "params": params}
        self.p.stdin.write(json.dumps(req, separators=(",", ":")) + "\n")
        self.p.stdin.flush()
        line = self.p.stdout.readline()
        if not line:
            raise RuntimeError(f"{self.name} server closed the pipe (see stderr)")
        return json.loads(line)

    def close(self):
        try:
            self.p.stdin.close()
        except Exception:
            pass
        try:
            self.p.wait(timeout=10)
        except Exception:
            self.p.kill()


def main():
    if len(sys.argv) != 2:
        print("usage: synth_values.py <bin-dir>")
        return 1
    bindir = os.path.abspath(sys.argv[1])
    here = os.path.dirname(os.path.abspath(__file__))

    # Coverage, checked against the fixtures on disk rather than against this
    # file's own table: summing the table proves only that everything IT lists
    # was compared, which is exactly the assurance a fixture added without a
    # golden row would still satisfy. Declared names and types are pinned to
    # each .yaml for the same reason.
    fixtures = sorted(d for d in os.listdir(here)
                      if d.startswith("synth") and d[5:].isdigit()
                      and os.path.isdir(os.path.join(here, d)))
    declared = {}
    for fx in fixtures:
        method = "TA_" + fx.upper()
        if method not in GOLDEN:
            print(f"synth_values: {FAIL} — fixture {fx} has no GOLDEN row; it would "
                  f"run in every other leg and never have its values checked")
            return 1
        outs = declared_outputs(os.path.join(here, fx))
        want = list(GOLDEN[method]["outs"].keys())
        if [n for n, _ in outs] != want:
            print(f"synth_values: {FAIL} — {method} GOLDEN outputs {want} do not match "
                  f"{fx}.yaml's {[n for n, _ in outs]} (name or order)")
            return 1
        declared[method] = outs
    for method in GOLDEN:
        if method[3:].lower() not in fixtures:
            print(f"synth_values: {FAIL} — GOLDEN has {method} with no fixture on disk")
            return 1

    servers = [
        ("c", ["./ta_codegen_serve_c"]),
        ("rust", ["./ta_codegen_serve_rust"]),
        ("java", ["java", "-cp", "ta_codegen_java", "TaCodegenServe"]),
        ("csharp", ["dotnet", "ta_codegen_csharp/TaCodegenServe.dll"]),
    ]

    fails = 0
    compared = 0          # counted at the element comparison itself, never before it
    print(f"synth_values: {len(GOLDEN)} fixture(s) x {len(servers)} server(s), "
          f"exact comparison")

    for lang, argv in servers:
        exe = os.path.join(bindir, argv[0] if not argv[0].startswith("./") else argv[0][2:])
        if argv[0].startswith("./") and not os.path.exists(exe):
            print(f"  [{FAIL}] {lang}: {exe} not found")
            fails += 1
            continue
        try:
            srv = Server(lang, argv, bindir)
        except Exception as e:  # noqa: BLE001
            print(f"  [{FAIL}] {lang}: cannot start ({e})")
            fails += 1
            continue

        try:
            for method, g in sorted(GOLDEN.items()):
                names = [n for n, _ in declared[method]]
                is_int = [t for _, t in declared[method]]

                params = {"startIdx": 0}
                nbar = None
                for k, v in g["params"].items():
                    if isinstance(v, list):
                        params[k] = to_hex(v)
                        nbar = len(v) if nbar is None else nbar
                    else:
                        params[k] = v
                params["endIdx"] = nbar - 1

                r = srv.call(method, params)

                if r.get("retCode") != 0:
                    print(f"  [{FAIL}] {lang} {method}: retCode {r.get('retCode')}")
                    fails += 1
                    continue
                if r.get("outBegIdx") != g["beg"]:
                    print(f"  [{FAIL}] {lang} {method}: outBegIdx {r.get('outBegIdx')} "
                          f"expected {g['beg']}")
                    fails += 1
                    continue
                nb = r.get("outNBElement")
                want_nb = len(g["outs"][names[0]])
                if nb != want_nb:
                    print(f"  [{FAIL}] {lang} {method}: outNBElement {nb} expected {want_nb}")
                    fails += 1
                    continue

                bad = False
                for idx, nm in enumerate(names):
                    key = json_key(names, is_int, idx)
                    if key not in r:
                        print(f"  [{FAIL}] {lang} {method}: response has no '{key}' "
                              f"(for output '{nm}'); keys={sorted(r.keys())}")
                        fails += 1
                        bad = True
                        break
                    got = r[key]
                    if not is_int[idx]:
                        # A real output is hex-of-IEEE-bits (#257/#258); a '['
                        # here means a stale server binary.
                        if not isinstance(got, str):
                            print(f"  [{FAIL}] {lang} {method}: '{key}' is not a hex "
                                  f"string — stale server?")
                            fails += 1
                            bad = True
                            break
                        got = from_hex(got)
                    want = g["outs"][nm]
                    if len(got) != len(want):
                        print(f"  [{FAIL}] {lang} {method}: '{key}' carried {len(got)} "
                              f"value(s), expected {len(want)}")
                        fails += 1
                        bad = True
                        break
                    for j, (a, b) in enumerate(zip(got, want)):
                        compared += 1
                        same = (a == b) if is_int[idx] else \
                               (struct.pack("<d", a) == struct.pack("<d", float(b)))
                        if not same:
                            print(f"  [{FAIL}] {lang} {method}: {key}[{j}] "
                                  f"got {a!r} expected {b!r}")
                            fails += 1
                            bad = True
                            break
                    if bad:
                        break
                if not bad:
                    print(f"  [{OK}] {lang} {method}")
        finally:
            srv.close()

    # Non-vacuity: the count is incremented AT the comparison, so a leg that
    # silently stopped comparing cannot leave this looking the same. That the
    # table covers every fixture is a separate check, above — this one can only
    # speak for what the table lists.
    want_elems = sum(len(v) for g in GOLDEN.values() for v in g["outs"].values()) * len(servers)
    print(f"\nsynth_values: compared {compared} element(s) across {len(fixtures)} "
          f"fixture(s) x {len(servers)} server(s)")
    if compared != want_elems and fails == 0:
        print(f"synth_values: VACUOUS — compared {compared}, expected {want_elems}")
        return 1
    if fails:
        print(f"synth_values: {FAIL} ({fails} failure(s))")
        return 1
    print(f"synth_values: all checks {OK}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
