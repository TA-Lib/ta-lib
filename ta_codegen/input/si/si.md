# SI

## Summary

Wilder's Swing Index: one number rating each bar against the bar before it, from both bars' open and close and the current bar's high and low, signed by the direction of the swing. A strong close above the prior close on a wide range reads high and positive, a strong down swing reads high and negative. It is scaled against the limit move, the largest one-bar move the market allows, and stays between -100 and +100 while every move is within that limit. J. Welles Wilder Jr. introduced it in 1978 and trades its running total, ASI.

## Formula

$$
\mathrm{SI}_t = 50 \cdot \frac{N_t}{R_t} \cdot \frac{K_t}{T}, \text{ or } 0 \text{ when } R_t = 0
$$

$$
N_t = (C_t - C_{t-1}) + \tfrac{1}{2}(C_t - O_t) + \tfrac{1}{4}(C_{t-1} - O_{t-1}) \qquad K_t = \max(a, b)
$$

$$
R_t = \begin{cases} a - \tfrac{1}{2}b + q & \text{if } a \ge b \text{ and } a \ge d \\ b - \tfrac{1}{2}a + q & \text{else if } b \ge d \\ d + q & \text{otherwise} \end{cases}
$$

where $a = |H_t - C_{t-1}|$, $b = |L_t - C_{t-1}|$, $d = |H_t - L_t|$, $q = \tfrac{1}{4}|C_{t-1} - O_{t-1}|$, and $T$ is `optInLimitMove` (Wilder writes it $L$).

## Notes

- Wilder rounds each value to a whole number when working by hand; the output is not rounded.
- A bar that moves more than the limit move can read beyond -100 or +100; the output is not clamped.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Swing index of the bar against the previous bar

## Parameters

- `optInLimitMove` — Limit move, the largest one-bar price move the index is scaled against, in price units

## See Also

ASI · WAD · TRANGE · BOP

## References

- J. Welles Wilder Jr., *New Concepts in Technical Trading Systems*, Trend Research, 1978, section VIII "The Swing Index", pp.87-106
