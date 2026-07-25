---
title: Numerical Stability
description: "What it means for an indicator to be start-independent, to carry an initial unstable period, to depend on the MA type selected, or to be path-dependent."
---

# Numerical Stability

The documentation specifies which of the four categories below applies to each function. They answer a single practical question: **does the value at a given bar depend on how much history you passed in?**

## If Start-Independent, then... {#start-independent}

The value at a bar does not depend on where your data starts. Feed the function a year or a decade and the value it reports for a given bar is identical. These functions read a bounded window — a fixed number of bars — and ignore everything older.

## If Initial Unstable Period, then... {#initial-unstable-period}

Early values depend on how much history precedes them, and converge as more bars are supplied. These functions are defined recursively: each value folds in the previous one, so the series never entirely forgets where it began — though the influence decays until it is lost in floating-point rounding.

How many leading values are discarded is tunable with `TA_SetUnstablePeriod`. Some functions own their setting; others inherit one from a function they compute internally — DEMA has no unstable period of its own, but is built from EMA and responds to EMA's. Each function page names the setting it responds to.

See [Unstable Period](/api/unstable-period/) for what to do about it: when to ignore it, when to supply extra history, and how to have TA-Lib drop the unstable values for you.

## If Depends on MA Type, then... {#depends-on-ma-type}

Some functions take an `optInMAType` parameter selecting how their moving average is computed. That choice decides which of the properties above applies: a recursive MA type gives the function an initial unstable period, a windowed one leaves it start-independent.

| MA Type | Value | Numerical Stability | Why |
| :-- | --: | :-- | :-- |
| [SMA](/functions/sma) | 0 | Start-Independent | A windowed average: it reads a fixed number of bars and forgets everything older. |
| [EMA](/functions/ema) | 1 | Initial Unstable Period | Recursive: each value folds in the previous one. Tunable via EMA's own unstable period. |
| [WMA](/functions/wma) | 2 | Start-Independent | A windowed average: it reads a fixed number of bars and forgets everything older. |
| [DEMA](/functions/dema) | 3 | Initial Unstable Period | Built from EMA, and inherits its unstable period. |
| [TEMA](/functions/tema) | 4 | Initial Unstable Period | Built from EMA, and inherits its unstable period. |
| [TRIMA](/functions/trima) | 5 | Start-Independent | A windowed average: it reads a fixed number of bars and forgets everything older. |
| [KAMA](/functions/kama) | 6 | Initial Unstable Period | Recursive: each value folds in the previous one. Tunable via KAMA's own unstable period. |
| [MAMA](/functions/mama) | 7 | Initial Unstable Period | Recursive: each value folds in the previous one. Tunable via MAMA's own unstable period. |
| [T3](/functions/t3) | 8 | Initial Unstable Period | Recursive: each value folds in the previous one. Tunable via T3's own unstable period. |
| [HMA](/functions/hma) | 9 | Start-Independent | A windowed average: it reads a fixed number of bars and forgets everything older. |
| `DISABLED` | 10 | Start-Independent | Not a moving average: the input is copied through unchanged. |

## If Path-Dependent, then... {#path-dependent}

The value is built up from the first bar — a running accumulation or a state machine that tracks the path prices took — so it depends on where your data begins and never converges. Unlike an unstable period, there is no warm-up you can discard: the difference persists for the whole series.

Do not compare these values across differently-sized windows, and expect a backtest starting at a different date to produce different numbers.
