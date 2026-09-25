# OBV

## Summary

On Balance Volume: a running cumulative total of volume, added on up-price bars and subtracted on down-price bars. Relates volume flow to price direction.

## Formula

OBV[i] = OBV[i-1] + (inReal[i] > inReal[i-1] ? V[i] : inReal[i] < inReal[i-1] ? -V[i] : 0); seed OBV[startIdx] = V[startIdx]

## Inputs

- `inReal` — Price series, typically close
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative on-balance volume

## Aliases

On Balance Volume

## References

- Joseph Ensign Granville, B. Granville, *Granville's New Strategy of Daily Stock Market Timing for Maximum Profit*, Simon & Schuster (ISBN 0133634329)
