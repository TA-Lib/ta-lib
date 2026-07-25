---
title: RSI
description: "Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the ratio of average gains to average losses over the period. Used to gauge overbought/oversold conditions. >70 overbought, <30 oversold."
---

# RSI

## Summary

Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the ratio of average gains to average losses over the period. Used to gauge overbought/oversold conditions. >70 overbought, <30 oversold.

## Formula

$$
\begin{aligned}
U_t &= \max(X_t - X_{t-1},\ 0)
   &  D_t &= \max(X_{t-1} - X_t,\ 0) \\[4pt]
\overline{U}_t &= \begin{cases}
    \operatorname{SMA}(U, n)_t                 & \text{if } t = n \\[4pt]
    \dfrac{(n-1)\,\overline{U}_{t-1} + U_t}{n} & \text{if } t > n
  \end{cases}
   &  \overline{D}_t &= \begin{cases}
    \operatorname{SMA}(D, n)_t                 & \text{if } t = n \\[4pt]
    \dfrac{(n-1)\,\overline{D}_{t-1} + D_t}{n} & \text{if } t > n
  \end{cases} \\[4pt]
\mathrm{RS}_t &= \frac{\overline{U}_t}{\overline{D}_t}
   &  \mathrm{RSI}_t &= 100 - \frac{100}{1 + \mathrm{RS}_t}
\end{aligned}
$$

where $X$ is the input series and $n$ the period.

## Inputs

- `inReal` — Price series (typically close)

## Outputs

- `outReal` — RSI value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback for the gain/loss averaging |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`rsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rsi/rsi.c) · [`rsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rsi/rsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_RSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RSI.c) |
| Rust | [`rsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rsi.rs) |
| Java | [`Core_RSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_RSI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

relative strength index

## See Also

[CMO](/functions/cmo) · [STOCHRSI](/functions/stochrsi)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
