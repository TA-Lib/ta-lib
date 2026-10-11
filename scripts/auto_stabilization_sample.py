#!/usr/bin/env python3
"""Nightly sampled check of the Auto-Stabilization counts (#540).

Runs a reduced census (docs/studies/auto-stabilization/auto_stabilization_census.c)
against this tree's static library and holds each row to the committed table
beside it, census_table.tsv:

  - the lookback and the two counts, which a trial's start and length are
    drawn from: a row where one moved is calibrated again, not compared;
  - the share of starts whose need passes the count, at each level;
  - the largest need of the sample, and that of the trial that set the
    largest need of each row in the full runs the table was made from;
  - the starts that never converge, and the trials that count at all.

The sample is the first trials of one seed, so on one machine it is the same
number every night: a ceiling's margin is for another machine and for a change
that moves a few trials, not for sampling noise.

    scripts/auto_stabilization_sample.py                 the check
    scripts/auto_stabilization_sample.py --calibrate DIR [DIR...]
        print a new table. Each DIR holds a full census of the table's two
        seeds, one file per function, as DIR/s<seed>/<FUNC>.tsv (20000 trials,
        CENSUS_CSV set), from one machine each. Needs, worst trials and the
        spread between machines come from them, the shares from a sample run
        here.

A row the census prints and the table lacks fails, and so does the converse:
a function that gains an unstable id brings its rows. The census drops a row
with no live trial, e.g. when a count outgrows its series, so a missing row is
also how that shows.
"""
import argparse
import concurrent.futures
import math
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
STUDY = os.path.join(ROOT, "docs", "studies", "auto-stabilization")
TABLE = os.path.join(STUDY, "census_table.tsv")
CSV = os.path.join(ROOT, "docs", "studies", "ema-seeding", "data", "ibm_daily_ohlc.csv")
LIB = os.path.join(ROOT, "cmake-build", "libta-lib.a")

SAMPLE_SEED = 1
SAMPLE_TRIALS = 2000
CALIBRATION_SEEDS = (1, 2)

# What a ceiling adds to the sampled share, in points: MARGIN_SPREAD times the
# row's largest difference between two calibration machines, at least
# MARGIN_FLOOR. A need may pass the largest one calibrated by NEED_SLACK.
MARGIN_FLOOR = 0.25
MARGIN_SPREAD = 3.0
NEED_SLACK = 0.02
LIVE_SLACK = 0.05

COLS = ("func cfg kind lookback live auto4 p50_10 p99_10 max_10 over10 over7 "
        "auto8 p50_19 p99_19 max_19 over19 over16 never worstT10 worstT19").split()
TABLE_COLS = ("func cfg kind lookback auto4 auto8 live_min share10_max share19_max "
              "need10_max need19_max replay10_max replay19_max never_max worst10 worst19").split()
TABLE_INTS = ("lookback auto4 auto8 live_min need10_max need19_max "
              "replay10_max replay19_max never_max").split()


def parse(text, where):
    """Census rows keyed by (func, cfg, kind). The header line is required."""
    lines = text.splitlines()
    if not lines or lines[0].split("\t") != COLS:
        raise SystemExit(f"FAIL: {where}: no census header line")
    rows = {}
    for line in lines[1:]:
        p = line.split("\t")
        if len(p) != len(COLS):
            raise SystemExit(f"FAIL: {where}: a row of {len(p)} columns, expected {len(COLS)}")
        r = dict(zip(COLS, p))
        for c in COLS[3:]:
            r[c] = int(r[c])
        rows[(r["func"], r["cfg"], r["kind"])] = r
    return rows


def build(workdir):
    if not os.path.exists(LIB):
        subprocess.run([sys.executable, os.path.join(ROOT, "scripts", "build.py")], check=True)
    exe = os.path.join(workdir, "census")
    subprocess.run([os.environ.get("CC", "cc"), "-O2", "-I" + os.path.join(ROOT, "include"),
                    os.path.join(STUDY, "auto_stabilization_census.c"), LIB, "-lm", "-o", exe],
                   check=True)
    return exe


def run(exe, trials, seed, func=None, trial=None):
    env = dict(os.environ, CENSUS_CSV=CSV)
    for name in ("CENSUS_PERIOD", "CENSUS_SET", "CENSUS_NEEDS", "CENSUS_TRIAL"):
        env.pop(name, None)
    if trial is not None:
        env["CENSUS_TRIAL"] = str(trial)
    args = [exe, str(trials), str(seed)] + ([func] if func else [])
    done = subprocess.run(args, env=env, capture_output=True, text=True)
    if done.returncode != 0 or done.stderr:
        raise SystemExit(f"FAIL: {' '.join(args)}: exit {done.returncode}: {done.stderr.strip()}")
    return parse(done.stdout, " ".join(args))


def functions(exe):
    done = subprocess.run([exe, "list"], capture_output=True, text=True, check=True)
    return done.stdout.split()


def sample(exe, pool):
    rows = {}
    for part in pool.map(lambda f: run(exe, SAMPLE_TRIALS, SAMPLE_SEED, f), functions(exe)):
        rows.update(part)
    return rows


def share(row, col):
    return 100.0 * row[col] / row["live"]


def read_table():
    table, meta = {}, {}
    with open(TABLE) as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("#"):
                for word in line[1:].split():
                    if "=" in word:
                        k, v = word.split("=", 1)
                        meta[k] = v
                continue
            p = line.split("\t")
            if p == TABLE_COLS:
                continue
            if len(p) != len(TABLE_COLS):
                raise SystemExit(f"FAIL: {TABLE}: a row of {len(p)} columns")
            r = dict(zip(TABLE_COLS, p))
            for c in TABLE_INTS:
                r[c] = int(r[c])
            for c in ("share10_max", "share19_max"):
                r[c] = float(r[c])
            for c in ("worst10", "worst19"):
                seed, trial = r[c].split(":")
                r[c] = (int(seed), int(trial))
            table[(r["func"], r["cfg"], r["kind"])] = r
    if meta.get("seed") != str(SAMPLE_SEED) or meta.get("trials") != str(SAMPLE_TRIALS):
        raise SystemExit(f"FAIL: {TABLE} was made for seed={meta.get('seed')} trials={meta.get('trials')}, "
                         f"this script samples seed={SAMPLE_SEED} trials={SAMPLE_TRIALS}")
    return table


def check(exe, pool):
    table = read_table()
    got = sample(exe, pool)
    fails = []
    for key in sorted(set(table) - set(got)):
        fails.append(f"{key}: in the table, not in the census output (no live trial, or the row is gone)")
    for key in sorted(set(got) - set(table)):
        fails.append(f"{key}: in the census output, not in the table (--calibrate adds it)")
    nb = {"share": 0, "share_above_0": 0, "need": 0, "replay": 0}
    for key in sorted(set(got) & set(table)):
        r, t = got[key], table[key]
        moved = [c for c in ("lookback", "auto4", "auto8") if r[c] != t[c]]
        if moved:
            fails.append(f"{key}: {', '.join(f'{c} is {r[c]}, was {t[c]}' for c in moved)}: "
                         "the row's trials are no longer the table's, calibrate it again")
            continue
        if r["live"] < t["live_min"]:
            fails.append(f"{key}: {r['live']} live trials, at least {t['live_min']} expected")
            continue
        if r["never"] > t["never_max"]:
            fails.append(f"{key}: {r['never']} starts never converge, at most {t['never_max']}")
        for lvl, over, mx in (("10", "over10", "max_10"), ("19", "over19", "max_19")):
            s = share(r, over)
            nb["share"] += 1
            nb["share_above_0"] += s > 0
            if s > t[f"share{lvl}_max"]:
                fails.append(f"{key}: {s:.2f}% of starts need more than the count at e^-{lvl}, "
                             f"ceiling {t[f'share{lvl}_max']:.2f}%")
            nb["need"] += 1
            if r[mx] > t[f"need{lvl}_max"]:
                fails.append(f"{key}: a start needs {r[mx]} bars at e^-{lvl}, largest allowed {t[f'need{lvl}_max']}")

    replays = {}
    for key, t in table.items():
        for lvl in ("10", "19"):
            replays.setdefault((key[0],) + t[f"worst{lvl}"], []).append((key, lvl))
    jobs = sorted(replays)
    for job, rows in zip(jobs, pool.map(lambda j: run(exe, 1, j[1], j[0], j[2]), jobs)):
        for key, lvl in replays[job]:
            r, t = rows.get(key), table[key]
            if r is None or r["live"] != 1:
                fails.append(f"{key}: the replay of seed {job[1]} trial {job[2]} is not a live trial")
                continue
            nb["replay"] += 1
            if r[f"max_{lvl}"] > t[f"replay{lvl}_max"]:
                fails.append(f"{key}: seed {job[1]} trial {job[2]} needs {r[f'max_{lvl}']} bars at e^-{lvl}, "
                             f"largest allowed {t[f'replay{lvl}_max']}")

    if not fails and (nb["share"] != 2 * len(table) or nb["replay"] != 2 * len(table) or nb["share_above_0"] == 0):
        fails.append(f"vacuous: {nb} over {len(table)} rows")
    for line in fails:
        print("FAIL: " + line)
    if fails:
        return 1
    print(f"Auto-Stabilization sample: {len(table)} rows, {nb['share']} shares and {nb['replay']} replays held. OK.")
    return 0


def load_full(directory):
    out = {}
    for seed in CALIBRATION_SEEDS:
        sub = os.path.join(directory, f"s{seed}")
        for name in sorted(os.listdir(sub)):
            if not name.endswith(".tsv"):
                continue
            with open(os.path.join(sub, name)) as f:
                text = f.read()
            if not text.startswith("func\t"):
                text = "\t".join(COLS) + "\n" + text
            for key, row in parse(text, os.path.join(sub, name)).items():
                out[(seed,) + key] = row
    return out


def calibrate(exe, pool, dirs):
    hosts = [load_full(d) for d in dirs]
    got = sample(exe, pool)
    keys = sorted(got)
    for d, h in zip(dirs, hosts):
        missing = [(s,) + k for s in CALIBRATION_SEEDS for k in keys if (s,) + k not in h]
        extra = [k for k in h if k[1:] not in got]
        if missing or extra:
            raise SystemExit(f"FAIL: {d}: {len(missing)} rows missing, {len(extra)} unknown, e.g. {(missing + extra)[0]}")
    print(f"# seed={SAMPLE_SEED} trials={SAMPLE_TRIALS} calibration={len(dirs)}")
    print("\t".join(TABLE_COLS))
    for key in keys:
        r = got[key]
        line = [key[0], key[1], key[2], str(r["lookback"]), str(r["auto4"]), str(r["auto8"]),
                str(int(r["live"] * (1 - LIVE_SLACK)))]
        for over in ("over10", "over19"):
            spread = max(max(share(h[(s,) + key], over) for h in hosts) - min(share(h[(s,) + key], over) for h in hosts)
                         for s in CALIBRATION_SEEDS)
            line.append(f"{share(r, over) + max(MARGIN_FLOOR, MARGIN_SPREAD * spread):.2f}")
        worst, replay = [], []
        for mx, wt in (("max_10", "worstT10"), ("max_19", "worstT19")):
            top = max(((h[(s,) + key][mx], s, h[(s,) + key][wt]) for h in hosts for s in CALIBRATION_SEEDS))
            line.append(str(int(math.ceil(r[mx] * (1 + NEED_SLACK)))))
            replay.append(str(int(math.ceil(top[0] * (1 + NEED_SLACK)))))
            worst.append(f"{top[1]}:{top[2]}")
        line += replay
        line.append(str(max(max(h[(s,) + key]["never"] for h in hosts for s in CALIBRATION_SEEDS), r["never"])))
        print("\t".join(line + worst))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--calibrate", nargs="+", metavar="DIR")
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 2)
    args = ap.parse_args()
    with tempfile.TemporaryDirectory() as workdir, \
            concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        exe = build(workdir)
        return calibrate(exe, pool, args.calibrate) if args.calibrate else check(exe, pool)


if __name__ == "__main__":
    sys.exit(main())
