# OBV

## Summary

On Balance Volume: a running cumulative total of volume, added on up-price bars and subtracted on down-price bars. Relates volume flow to price direction.

## Formula

OBV[i] = OBV[i-1] + (inReal[i] > inReal[i-1] ? V[i] : inReal[i] < inReal[i-1] ? -V[i] : 0); seed OBV[startIdx] = V[startIdx]

## Inputs

- `inReal` — Price series compared bar-over-bar (typically close)
- `inPriceV` — Volume added/subtracted each bar

## Outputs

- `outReal` — Cumulative on-balance volume

## Implementation

TA-Lib Definition: [`obv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/obv/obv.c) · [`obv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/obv/obv.yaml)

| Native | File |
|--------|------|
| C | [`ta_OBV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_OBV.c) |
| Rust | [`obv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/obv.rs) |
| Java | [`Core_OBV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_OBV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

On Balance Volume

## References

- Joseph Ensign Granville, B. Granville, *Granville's New Strategy of Daily Stock Market Timing for Maximum Profit*, Simon & Schuster (ISBN 0133634329)
