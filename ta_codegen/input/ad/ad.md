# AD

## Summary

Chaikin Accumulation/Distribution Line, a cumulative volume-flow indicator. Sums a volume-weighted money-flow multiplier per bar to gauge buying vs. selling pressure. Rising line = accumulation (buying pressure); falling = distribution.

## Formula

MFM = ((close-low) - (high-close)) / (high-low); AD_t = AD_{t-1} + MFM_t * volume_t (running sum, seeded at 0)

## Inputs

- `inPriceHLCV` — High, low, close, and volume series

## Outputs

- `outReal` — Cumulative A/D line value per bar

## Implementation

TA-Lib Definition: [`ad.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ad/ad.c) · [`ad.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ad/ad.yaml)

| Native | File |
|--------|------|
| C | [`ta_AD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AD.c) |
| Rust | [`ad.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ad.rs) |
| Java | [`Core_AD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_AD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Chaikin A/D Line, Accumulation/Distribution Line, Accumulation Distribution

## See Also

ADOSC · OBV
