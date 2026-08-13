# WAD

## Summary

Williams' Accumulation/Distribution: a cumulative line that measures each bar's close against the *true range* extreme — the previous close, whenever it lies outside the current bar — rather than against the bar's own high and low. A close above the previous one accumulates the distance up from the true low; a close below it distributes the distance down from the true high; an unchanged close contributes nothing.

**It consumes no volume.** Larry Williams' original multiplies each move by that bar's volume; Steven Achelis published the modification that drops the multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), and the industry kept Williams' name on that no-volume form. That industry-wide decision is enough for TA-Lib to ship the same form under the same name.

## Formula

TRH_t = max(close_{t-1}, high_t); TRL_t = min(close_{t-1}, low_t); AD_t = close_t - TRL_t if close_t > close_{t-1}, close_t - TRH_t if close_t < close_{t-1}, otherwise 0; WAD_t = WAD_{t-1} + AD_t

The first bar of the requested range has no previous close, so it contributes 0 and the line starts there — the same convention as AD, OBV, NVI and PVI. The accumulator restarts wherever the caller starts, so a different `startIdx` shifts the whole line by a constant.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Cumulative accumulation/distribution

## Implementation

TA-Lib Definition: [`wad.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wad/wad.c) · [`wad.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wad/wad.yaml)

| Native | File |
|--------|------|
| C | [`ta_WAD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WAD.c) |
| Rust | [`wad.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wad.rs) |
| Java | [`Core_WAD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WAD.java) |
| C# | [`Core_WAD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WAD.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

AD · ADOSC · NVI · OBV · PVI

## References

- Larry Williams is the originator; Steven Achelis, *Technical Analysis from A to Z*, 2nd edition, page 368 publishes the no-volume form this ships, with the worked 12-bar example pinned in the test suite.
- IncredibleCharts, *Williams Accumulation Distribution* — Williams' volume-weighted original, `AD = Price Move × Volume` over the same true-range price move.
- IncredibleCharts, *Williams Accumulate Distribute* — the Achelis form under its disambiguating name, "not a volume indicator despite the name".
