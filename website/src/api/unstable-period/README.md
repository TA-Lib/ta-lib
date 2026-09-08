---
title: Unstable Period
description: "How many warm-up bars TA-Lib discards from recursive indicators such as EMA, RSI and ADX before reporting their output, and how to set it in C, Rust, Java and C#."
toc: false
---

**TL;DR:** Some indicators need a warm-up before their output settles. TA-Lib can discard those bars for you, so unstable values never reach your application.

## Why it exists

Some indicators have "memory". Each output depends on the previous one, seeded from the start of the data. An Exponential Moving Average is the classic example: the seed's effect is large at first, and decays with each bar until the result is **stable**.

<figure class="uf-fig">
<svg class="uf" viewBox="0 0 760 344" role="img" aria-labelledby="uf-cap" xmlns="http://www.w3.org/2000/svg">
<rect class="uf-wash" x="54" y="16" width="410.5" height="246"/>
<line class="uf-grid" x1="54" x2="736" y1="252.4" y2="252.4"/>
<text class="uf-tick uf-tick-y" x="45" y="255.9">145</text>
<line class="uf-grid" x1="54" x2="736" y1="168.6" y2="168.6"/>
<text class="uf-tick uf-tick-y" x="45" y="172.1">150</text>
<line class="uf-grid" x1="54" x2="736" y1="84.9" y2="84.9"/>
<text class="uf-tick uf-tick-y" x="45" y="88.4">155</text>
<mask id="uf-fade"><linearGradient id="uf-fadeg" x1="0" y1="16" x2="0" y2="262" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#000"/><stop offset="0.16" stop-color="#fff"/><stop offset="0.84" stop-color="#fff"/><stop offset="1" stop-color="#000"/></linearGradient><rect x="54" y="16" width="682" height="246" fill="url(#uf-fadeg)"/></mask>
<g mask="url(#uf-fade)">
<path class="uf-ctx" d="M54,185.4V227.2M60.3,151.9V185.4M66.6,153.9V179M72.9,120.4V177M79.3,122.6V143.5M85.6,110V139.3M91.9,91.1V135.1M98.2,107.8V164.4M104.5,126.8V204.1M110.8,218.9V271.1M117.1,231.4V275.3M123.5,250.2V298.4M129.8,225.1V264.9M136.1,229.3V264.9M142.4,248.2V273.3M148.7,241.8V258.6M155,199.9V246M161.4,183.2V218.9M167.7,162.3V189.6M174,143.5V172.8M180.3,164.4V199.9M186.6,160.2V191.6M192.9,110V168.6M199.2,70.1V110M205.6,70.1V93.3M211.9,47.2V107.8M218.2,43V72.3M224.5,5.3V63.9M230.8,-30.3V30.5M237.1,-22V26.3M243.4,-5.2V26.3M249.8,26.3V76.5M256.1,80.7V132.9M262.4,82.7V130.9M268.7,86.9V130.9M275,86.9V126.8M281.3,74.3V99.5M287.6,80.7V114.2M294,107.8V143.5M300.3,124.6V183.2M306.6,128.8V181.2M312.9,143.5V199.9M319.2,124.6V195.8M325.5,135.1V172.8M331.9,164.4V212.5M338.2,166.4V199.9M344.5,120.4V185.4M350.8,149.7V202.1M357.1,135.1V183.2M363.4,116.2V174.8M369.7,151.9V199.9M376.1,204.1V223.1M382.4,189.6V214.7M388.7,168.6V227.2M395,153.9V193.7M401.3,122.6V170.6M407.6,128.8V164.4M413.9,110V156.1M420.3,151.9V177M426.6,143.5V202.1M432.9,153.9V206.3M439.2,124.6V187.4M445.5,132.9V185.4M451.8,193.7V231.4M458.1,214.7V260.7M464.5,183.2V248.2M470.8,195.8V227.2M477.1,116.2V183.2M483.4,126.8V166.4M489.7,55.6V168.6M496,30.5V93.3M502.4,15.7V45M508.7,24.1V59.8M515,1.1V38.8M521.3,-15.6V15.7M527.6,15.7V49.2M533.9,-5.2V49.2M540.2,5.3V68.1M546.6,36.6V82.7M552.9,70.1V110M559.2,47.2V105.8M565.5,30.5V68.1M571.8,36.6V78.5M578.1,47.2V151.9M584.4,68.1V101.6M590.8,84.9V132.9M597.1,76.5V139.3M603.4,91.1V141.3M609.7,110V179M616,149.7V191.6M622.3,107.8V162.3M628.6,139.3V185.4M635,147.7V197.9M641.3,160.2V204.1M647.6,202.1V266.9M653.9,168.6V285.9M660.2,162.3V193.7M666.5,168.6V197.9M672.9,63.9V168.6M679.2,59.8V124.6M685.5,122.6V181.2M691.8,170.6V189.6M698.1,174.8V204.1M704.4,151.9V202.1M710.7,162.3V214.7M717.1,68.1V218.9M723.4,59.8V95.3M729.7,86.9V116.2M736,95.3V128.8"/>
</g>
<line class="uf-axis" x1="54" x2="736" y1="262" y2="262"/>
<line class="uf-axis" x1="54" x2="54" y1="262" y2="267"/>
<text class="uf-tick" x="54" y="284" text-anchor="middle">0</text>
<line class="uf-axis" x1="180.3" x2="180.3" y1="262" y2="267"/>
<text class="uf-tick" x="180.3" y="284" text-anchor="middle">20</text>
<line class="uf-axis" x1="306.6" x2="306.6" y1="262" y2="267"/>
<text class="uf-tick" x="306.6" y="284" text-anchor="middle">40</text>
<line class="uf-axis" x1="432.9" x2="432.9" y1="262" y2="267"/>
<text class="uf-tick" x="432.9" y="284" text-anchor="middle">60</text>
<line class="uf-axis" x1="559.2" x2="559.2" y1="262" y2="267"/>
<text class="uf-tick" x="559.2" y="284" text-anchor="middle">80</text>
<line class="uf-axis" x1="685.5" x2="685.5" y1="262" y2="267"/>
<text class="uf-tick" x="685.5" y="284" text-anchor="middle">100</text>
<text class="uf-tick" x="736" y="284" text-anchor="end">bar</text>
<line class="uf-boundary" x1="464.5" x2="464.5" y1="16" y2="335"/>
<polyline class="uf-ema uf-ema1" points="174,197.6 180.3,195.2 186.6,194.3 192.9,189.8 199.2,181 205.6,170.9 211.9,164.1 218.2,153.5 224.5,145 230.8,132.3 237.1,117.6 243.4,107.3 249.8,101 256.1,99 262.4,100.9 268.7,99.6 275,101.9 281.3,100.9 287.6,99.8 294,102.1 300.3,104.7 306.6,111.2 312.9,116.2 319.2,122.4 325.5,123.6 331.9,128.7 338.2,135.3 344.5,139.1 350.8,140.3 357.1,144.4 363.4,144.3 369.7,148 376.1,153.7 382.4,158.3 388.7,161.1 395,163.4 401.3,163.7 407.6,161.6 413.9,158.5 420.3,158 426.6,158.4 432.9,158.6 439.2,161.1 445.5,158.7 451.8,162 458.1,169 464.5,174.2 470.8,176.2 477.1,175.9 483.4,173.2 489.7,171.4 496,161.7 502.4,149.2 508.7,139.1 515,127.8 521.3,116.1 527.6,106.5 533.9,100.5 540.2,92.8 546.6,90.1 552.9,88.6 559.2,88.2 565.5,85.1 571.8,81.5 578.1,80.6 584.4,81 590.8,81.6 597.1,83.5 603.4,84.2 609.7,89.9 616,98.2 622.3,103.3 628.6,106.7 635,113.2 641.3,118.5 647.6,126.4 653.9,139.6 660.2,143.2 666.5,146.6 672.9,147.3 679.2,139.8 685.5,138.3 691.8,142.4 698.1,146.5 704.4,151 710.7,152.1 717.1,155.4 723.4,147.7 729.7,142.7 736,138.2"/>
<polyline class="uf-ema uf-ema2" points="237.1,162.5 243.4,147.9 249.8,137.7 256.1,132.3 262.4,131 268.7,126.8 275,126.6 281.3,123.2 287.6,119.9 294,120.4 300.3,121.2 306.6,126.1 312.9,129.7 319.2,134.6 325.5,134.7 331.9,138.7 338.2,144.4 344.5,147.3 350.8,147.7 357.1,151.1 363.4,150.4 369.7,153.5 376.1,158.7 382.4,162.8 388.7,165.2 395,167.1 401.3,167 407.6,164.6 413.9,161.2 420.3,160.5 426.6,160.7 432.9,160.6 439.2,163 445.5,160.3 451.8,163.5 458.1,170.4 464.5,175.4 470.8,177.3 477.1,176.9 483.4,174.1 489.7,172.2 496,162.5 502.4,149.9 508.7,139.7 515,128.3 521.3,116.6 527.6,107 533.9,100.9 540.2,93.2 546.6,90.4 552.9,88.9 559.2,88.5 565.5,85.4 571.8,81.7 578.1,80.8 584.4,81.2 590.8,81.8 597.1,83.6 603.4,84.4 609.7,90 616,98.3 622.3,103.4 628.6,106.8 635,113.3 641.3,118.5 647.6,126.5 653.9,139.7 660.2,143.2 666.5,146.6 672.9,147.3 679.2,139.8 685.5,138.3 691.8,142.4 698.1,146.5 704.4,151 710.7,152.1 717.1,155.4 723.4,147.7 729.7,142.7 736,138.2"/>
<polyline class="uf-ema uf-ema3" points="300.3,88.9 306.6,96.9 312.9,103.3 319.2,110.8 325.5,113.1 331.9,119.2 338.2,126.7 344.5,131.3 350.8,133.2 357.1,138 363.4,138.5 369.7,142.8 376.1,149 382.4,154.1 388.7,157.2 395,159.9 401.3,160.5 407.6,158.7 413.9,155.9 420.3,155.7 426.6,156.3 432.9,156.7 439.2,159.4 445.5,157.1 451.8,160.6 458.1,167.7 464.5,173 470.8,175.2 477.1,174.9 483.4,172.3 489.7,170.6 496,161 502.4,148.6 508.7,138.5 515,127.2 521.3,115.6 527.6,106.1 533.9,100.1 540.2,92.5 546.6,89.7 552.9,88.3 559.2,88 565.5,84.9 571.8,81.3 578.1,80.4 584.4,80.9 590.8,81.4 597.1,83.4 603.4,84.1 609.7,89.7 616,98.1 622.3,103.2 628.6,106.6 635,113.1 641.3,118.4 647.6,126.4 653.9,139.6 660.2,143.1 666.5,146.6 672.9,147.3 679.2,139.7 685.5,138.3 691.8,142.4 698.1,146.5 704.4,151 710.7,152 717.1,155.4 723.4,147.7 729.7,142.7 736,138.2"/>
<circle class="uf-dot uf-dot1" cx="174" cy="197.6" r="4.5"/>
<circle class="uf-dot uf-dot2" cx="237.1" cy="162.5" r="4.5"/>
<circle class="uf-dot uf-dot3" cx="300.3" cy="88.9" r="4.5"/>
<rect class="uf-legend-bg" x="54" y="16" width="150" height="72" rx="5"/>
<line class="uf-ema uf-ema1" x1="66" x2="88" y1="36" y2="36"/>
<text class="uf-legend" x="96" y="40">fed from bar 0</text>
<line class="uf-ema uf-ema2" x1="66" x2="88" y1="57" y2="57"/>
<text class="uf-legend" x="96" y="61">fed from bar 10</text>
<line class="uf-ema uf-ema3" x1="66" x2="88" y1="78" y2="78"/>
<text class="uf-legend" x="96" y="82">fed from bar 20</text>
<defs><marker id="uf-ah" viewBox="0 0 10 10" refX="9.5" refY="5" markerWidth="7" markerHeight="7" orient="auto"><path d="M0,0.7 L10,5 L0,9.3 z"/></marker></defs>
<line class="uf-arrow" x1="455.5" x2="56" y1="326" y2="326" marker-end="url(#uf-ah)"/>
<line class="uf-arrow" x1="473.5" x2="734" y1="326" y2="326" marker-end="url(#uf-ah)"/>
<text class="uf-zone" x="259.2" y="315" text-anchor="middle">unstable</text>
<text class="uf-zone" x="600.2" y="315" text-anchor="middle">stable</text>
</svg>
<figcaption id="uf-cap">Three 20-bar EMAs of the same prices, each fed from a different starting bar; from bar 65 on they stabilize within an acceptable error margin.</figcaption>
</figure>

This is inherent to the algorithms, not something specific to TA-Lib: every implementation has to seed the recursion somewhere.

## What to do

There are three distinct approaches, from the most common to the most rigorous:

1. **Ignore the problem.** What most charting sites do, and usually fine: the latest bar has plenty of history behind it. Nothing warns you when it doesn't; a short series, or a back-test on the earliest bars, quietly uses bad values.

2. **Scrub it yourself.** TA-Lib stays at its default (unstable period `0`) and returns everything it can compute; your code decides how many leading outputs to drop.

3. **Let TA-Lib do it.** Set an unstable period and the function stops emitting that many leading values, the ones the seed still distorts.

## API

::: code-tabs#lang

@tab C

```c
TA_RetCode   TA_SetUnstablePeriod( TA_FuncUnstId id, unsigned int unstablePeriod );
unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id );

/* Strip 30 extra bars from every EMA-based calculation: */
TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 30 );

/* Apply the same unstable period to ALL affected functions at once: */
TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 30 );
```

@tab Rust

```rust
use ta_lib::{Core, FuncUnstId};

// Strip 30 extra bars from every EMA-based calculation:
let core = Core::builder()
    .unstable_period(FuncUnstId::EMA, 30)
    .build()?;

// Apply the same unstable period to ALL affected functions at once:
let core = Core::builder()
    .unstable_period(FuncUnstId::ALL, 30)
    .build()?;

let n = core.get_unstable_period(FuncUnstId::EMA)?;   // read it back
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.FuncUnstId;

// Strip 30 extra bars from every EMA-based calculation:
Core core = Core.builder()
    .unstablePeriod(FuncUnstId.EMA, 30)
    .build();

// Apply the same unstable period to ALL affected functions at once:
Core all = Core.builder()
    .unstablePeriod(FuncUnstId.ALL, 30)
    .build();

int n = core.unstablePeriod(FuncUnstId.EMA);   // read it back
```

@tab C\#

```csharp
using TALib;

// Strip 30 extra bars from every EMA-based calculation:
Core core = Core.Builder()
    .UnstablePeriod(FuncUnstId.EMA, 30)
    .Build();

// Apply the same unstable period to ALL affected functions at once:
Core all = Core.Builder()
    .UnstablePeriod(FuncUnstId.ALL, 30)
    .Build();

int n = core.UnstablePeriod(FuncUnstId.EMA);   // read it back
```

:::

`id` selects which function to affect. The period sets how many warm-up bars that
function discards; the larger the value, the later the first output. The default,
`0`, discards nothing: you get every value the function can compute.

The setting follows the function wherever it runs: whether you call it directly, or
another indicator uses it internally. The EMA id therefore affects EMA itself and
every indicator built on one, such as MACD and DEMA.

## Functions with an unstable period

<!-- ta_codegen:begin unstable-func-list -->
`ADX`, `ATR`, `CMO`, `DX`, `EMA`, `HT_DCPERIOD`, `HT_DCPHASE`, `HT_PHASOR`, `HT_SINE`, `HT_TRENDLINE`, `HT_TRENDMODE`, `KAMA`, `MAMA`, `MINUS_DI`, `MINUS_DM`, `NATR`, `PLUS_DI`, `PLUS_DM`, `RSI`, `T3`, `RMA`, `HA`, `RVI`.
<!-- ta_codegen:end unstable-func-list -->

| Language | Id | Enum |
| --- | --- | --- |
| C | `TA_FUNC_UNST_<NAME>` | [ta_defs.h](https://github.com/TA-Lib/ta-lib/blob/main/include/ta_defs.h) |
| Rust | `FuncUnstId::<NAME>` | [types.rs](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/types.rs) |
| Java | `FuncUnstId.<NAME>` | [FuncUnstId.java](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/src/main/java/io/github/talib/FuncUnstId.java) |
| C# | `FuncUnstId.<NAME>` | [FuncUnstId.cs](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/FuncUnstId.cs) |

`<NAME>` may also be `ALL`, which targets every function above at once.

## See also

- [C/C++ Core API](/api/) / [Rust Core API](/api/rust/)
- [Candlestick Settings](/api/candle-settings/)
