# SAR

## Summary

Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level that accelerates toward price via an acceleration factor. Signals trend direction and trailing exit points. SAR below price = uptrend (long); SAR above price = downtrend (short). Price crossing SAR flips direction.

## Formula

SAR_next = SAR + af * (EP - SAR)
EP = extreme point (highest high in long / lowest low in short); af starts at Acceleration, += Acceleration each new EP, capped at Maximum.
On penetration: reverse, SAR := prior EP, reset af = Acceleration. SAR clamped each bar so it does not penetrate the prior/current bar's range.

## Notes

- Wilder left a few details open, so SAR values can differ between charting tools, mostly near the start of a series and on reversal bars. TA-Lib's choices:
  - The first direction comes from Wilder's own Directional Movement on the first two bars (as in MINUS_DM): short if the low fell by more than the high rose, long otherwise.
  - A bar that touches the SAR exactly counts as a reversal.
  - Wilder's clamping rule is applied before checking for a reversal, so price must reach the clamped stop to trigger one.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Parabolic SAR stop/reverse level per bar

## Parameters

- `optInAcceleration` — Step added to the acceleration factor on each new extreme point
- `optInMaximum` — Ceiling on the acceleration factor

## Aliases

Parabolic SAR, PSAR, Stop and Reverse

## See Also

SAREXT · MINUS_DM · PLUS_DM

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
