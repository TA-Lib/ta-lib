---
title: ADOSC
description: "Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the Accumulation/Distribution line. Highlights momentum in accumulation/distribution volume flow. Positive/rising suggests accumulation; negative/falling suggests distribution."
---

# ADOSC

## Summary

Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the Accumulation/Distribution line. Highlights momentum in accumulation/distribution volume flow. Positive/rising suggests accumulation; negative/falling suggests distribution.

## Formula

ad += ((close-low)-(high-close))/(high-low) * volume   (only when high>low)
fastEMA = fastk*ad + (1-fastk)*fastEMA,  fastk = 2/(optInFastPeriod+1)
slowEMA = slowk*ad + (1-slowk)*slowEMA,  slowk = 2/(optInSlowPeriod+1)
ADOSC = fastEMA - slowEMA

## Inputs

- `inPriceHLCV` — High, low, close, and volume series

## Outputs

- `outReal` — Fast-EMA minus slow-EMA of the A/D line

## Parameters

- `optInFastPeriod` — Period of the fast A/D EMA
- `optInSlowPeriod` — Period of the slow A/D EMA

## Implementation

TA-Lib Definition: [`adosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adosc/adosc.c) · [`adosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adosc/adosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADOSC.c) |
| Rust | [`adosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/adosc.rs) |
| Java | [`Core_ADOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ADOSC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Chaikin A/D Oscillator, Chaikin Oscillator

## See Also

[AD](/functions/ad.md) · [EMA](/functions/ema.md)

## References

- Marc Chaikin
