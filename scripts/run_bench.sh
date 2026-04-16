#!/bin/bash
set -euo pipefail

# Run Accelerate benchmark, store results in SQLite, print comparison.
#
# Usage: scripts/run_bench.sh [runs_per_function]
#   Default: 1000 runs per function per size

RUNS="${1:-1000}"
DB="bench_results.db"
BENCH_ACCEL="bin/ta_bench_accel"
BENCH_SCALAR="bin/ta_bench_scalar"

for bin in "$BENCH_ACCEL" "$BENCH_SCALAR"; do
  if [ ! -x "$bin" ]; then
    echo "Missing $bin — build both binaries first:"
    echo "  cmake -DTA_USE_ACCELERATE=ON  . && cmake --build . --clean-first --target ta_bench && cp bin/ta_bench bin/ta_bench_accel"
    echo "  cmake -DTA_USE_ACCELERATE=OFF . && cmake --build . --clean-first --target ta_bench && cp bin/ta_bench bin/ta_bench_scalar"
    exit 1
  fi
done

rm -f "$DB"

sqlite3 "$DB" <<'SQL'
CREATE TABLE samples (
  accel   TEXT NOT NULL,
  func    TEXT NOT NULL,
  size    INTEGER NOT NULL,
  run     INTEGER NOT NULL,
  us      REAL NOT NULL
);
SQL

echo "Running scalar benchmark ($RUNS runs)..."
"$BENCH_SCALAR" "$RUNS" | tail -n +2 | sqlite3 "$DB" ".mode csv" ".import /dev/stdin samples"

echo "Running Accelerate benchmark ($RUNS runs)..."
"$BENCH_ACCEL" "$RUNS" | tail -n +2 | sqlite3 "$DB" ".mode csv" ".import /dev/stdin samples"

echo ""
echo "Results stored in $DB ($(sqlite3 "$DB" "SELECT count(*) FROM samples") samples)"
echo ""

sqlite3 -header -column "$DB" <<'SQL'
-- Summary: median, mean, stddev, and speedup per function/size
WITH stats AS (
  SELECT
    func,
    size,
    accel,
    count(*) as n,
    avg(us) as mean_us,
    -- stddev via sqrt(avg(x^2) - avg(x)^2)
    sqrt(avg(us*us) - avg(us)*avg(us)) as stddev_us
  FROM samples
  GROUP BY func, size, accel
),
pivoted AS (
  SELECT
    s_off.func,
    s_off.size,
    s_off.mean_us   AS scalar_us,
    s_off.stddev_us  AS scalar_sd,
    s_on.mean_us    AS accel_us,
    s_on.stddev_us   AS accel_sd,
    s_off.mean_us / s_on.mean_us AS speedup
  FROM stats s_off
  JOIN stats s_on
    ON s_off.func = s_on.func
   AND s_off.size = s_on.size
  WHERE s_off.accel = 'OFF'
    AND s_on.accel = 'ON'
)
SELECT
  printf('%-8s', func) AS "Function",
  printf('%6d', size) AS "Size",
  printf('%9.2f', scalar_us) AS "Scalar µs",
  printf('%9.2f', accel_us) AS "Accel µs",
  printf('%5.1fx', speedup) AS "Speedup",
  printf('±%.2f / ±%.2f', scalar_sd, accel_sd) AS "StdDev (S/A)"
FROM pivoted
ORDER BY func, size;
SQL
