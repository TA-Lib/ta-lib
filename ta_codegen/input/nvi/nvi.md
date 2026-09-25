# NVI

## Summary

Negative Volume Index: a running cumulative index that changes only on days when
volume falls versus the prior day, compounding that day's percentage price change.
The premise is that quiet, low-volume days reflect the actions of well-informed
"smart money", so NVI is read as a proxy for that cohort's positioning.

## Formula

NVI[startIdx] = 1000

For each subsequent bar i:

    NVI[i] = NVI[i-1] + ( inVolume[i] < inVolume[i-1]
                          ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * NVI[i-1]
                          : 0 )

The index carries forward unchanged on bars whose volume did not fall (and on the
degenerate case of a zero previous close, which would otherwise divide by zero).

## Notes

- The index compounds, so it has no upper bound. If a run of large rises ever pushes it past the largest representable number, the last representable value is carried forward instead of returning infinity. Real price series stay far away from that.

## Inputs

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative negative volume index (seeded at 1000)

## Aliases

Negative Volume Index

## References

- Norman G. Fosback, *Stock Market Logic*, The Institute for Econometric Research (ISBN 0917604482)
