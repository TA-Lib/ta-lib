# COPPOCK

## Summary

Coppock Curve: Edwin S. "Sedge" Coppock's long-term momentum oscillator (*Barron's*, originally published as the "Trendex Model"), computed as a weighted moving average of the **sum** of two rates of change.

Unbounded; positive turns from below zero are the signal the indicator was designed for (long-term buying opportunities on monthly index data).

## Formula

`COPPOCK = WMA(ROC(optInROC1Period) + ROC(optInROC2Period), optInWMAPeriod)`

Each ROC carries [`ROC`](/functions/roc)'s own zero guard — a zero price `optInROC*Period` bars back yields 0.0 for that term, never an infinity. The two ROCs are **summed**, not averaged: every published definition sums them. (Tulip's `copp` averages, so it reads at exactly half this amplitude — a clean 2.0x ratio against Tulip is Tulip's variant, not a defect.)

The formula is symmetric in the two ROC periods and the lookback keys off their max, so `optInROC1Period > optInROC2Period` is accepted rather than rejected.

The classic defaults are 11/14/10 on monthly data. Wikipedia's daily-scale variant (231/294-bar ROC, 210-bar WMA) is a parameter choice reachable through this API, not a competing formula.

## Inputs

- `inReal` — Source price/value series (canonically a monthly close)

## Outputs

- `outReal` — Coppock Curve value

## Parameters

- `optInWMAPeriod` — Smoothing period for the ROC sum
- `optInROC1Period` — Short rate-of-change period
- `optInROC2Period` — Long rate-of-change period

## Implementation

The WMA stage reproduces [`WMA`](/functions/wma)'s recurrence verbatim — the periodSum/periodSub carry, the double triangle divider and the periodic re-anchor — so the fused single pass is bit-identical to running `ROC + ROC` into `WMA`. First output at `max(optInROC1Period, optInROC2Period) + optInWMAPeriod - 1`; with the defaults, 23 bars of lookback. Not start-dependent: each output depends only on the finite trailing window.
