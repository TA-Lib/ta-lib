---
title: EMA Variant Settings
description: "How the EMA family starts its recursion: a simple average over the period (the default) or the first element of the input array, with the same lookback either way. Who uses which seed, the measured difference, and how to set it in C, Rust, Java, C#, TypeScript and Go."
toc: false
---

**TL;DR:** An EMA needs a first value to start from. TA-Lib has always started from a simple average (SMA) over `period` bars, and still does by default; starting from the first element of the input array is now an option since v0.8.4 (Dec 2026).

## The two options

| Value | How the EMA starts | First value published | Note |
| --- | --- | --- | --- |
| `SMA_SEED` | From an SMA over `period` bars | Once `period` bars are available | De facto industry standard (and TA-Lib default). |
| `FIRST_BAR_SEED` | From the first element of the input array, then one EMA step per bar | Once `period` bars are available | Smaller error on the first values published: see [Measured difference](#measured-difference). |

## How to set it

::: code-tabs#lang

@tab C

```c
/* Once, after TA_Initialize: */
TA_SetEMAVariation( TA_EMAVariation_FIRST_BAR_SEED );

TA_EMAVariation v = TA_GetEMAVariation();   /* TA_EMAVariation_FIRST_BAR_SEED */

/* Multithreading: the setting is process-wide. Change it only while no TA
 * function is running and no stream is open. */
```

@tab Rust

```rust
use ta_lib::{Core, EMAVariation};

let core = Core::builder()
    .ema_variation(EMAVariation::FIRST_BAR_SEED)
    .build()?;

let v = core.ema_variation();   // EMAVariation::FIRST_BAR_SEED

// Multithreading: the setting belongs to this Core and is fixed once the Core
// is built, so the Core can be shared between threads. A stream reads it when
// it opens and keeps it for its life.
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.EMAVariation;

Core core = Core.builder()
    .emaVariation(EMAVariation.FIRST_BAR_SEED)
    .build();

EMAVariation v = core.emaVariation();   // EMAVariation.FIRST_BAR_SEED

// Multithreading: the setting belongs to this Core and is fixed once the Core
// is built, so the Core can be shared between threads. A stream reads it when
// it opens and keeps it for its life.
```

@tab C\#

```csharp
using TALib;

Core core = Core.Builder()
    .EmaVariation(EMAVariation.FIRST_BAR_SEED)
    .Build();

EMAVariation v = core.EmaVariation();   // EMAVariation.FIRST_BAR_SEED

// Multithreading: the setting belongs to this Core and is fixed once the Core
// is built, so the Core can be shared between threads. A stream reads it when
// it opens and keeps it for its life.
```

@tab TypeScript

```ts
import { CoreBuilder, EMAVariation } from 'ta-lib';

const core = new CoreBuilder()
  .emaVariation(EMAVariation.FIRST_BAR_SEED)
  .build();

const v = core.emaVariation();   // EMAVariation.FIRST_BAR_SEED

// Multithreading: the setting belongs to this Core and is fixed once the Core
// is built. Pass the Core as the `core` option of each call. A stream reads
// the setting when it opens and keeps it for its life.
```

@tab Go

```go
import talib "ta-lib.github.io/go"

core := talib.NewCore()
if err := core.SetEMAVariation(talib.EMAVariationFIRST_BAR_SEED); err != nil {
	log.Fatal(err)
}

v := core.EMAVariation() // talib.EMAVariationFIRST_BAR_SEED

// Multithreading: call a Core's setters only while no other goroutine uses
// it and no stream opened on it is in use.
```

:::

The rules for changing a setting are in the [specification](/spec/settings-threads/#idle-settings).

## Which one to pick

Mostly a matter of preference: pick the seed of the tool or application you want your values to match.

| Products | Seed | First value published | TA-Lib setting |
| --- | --- | --- | --- |
| TA-Lib (default), R `TTR::EMA`, TradingView `ta.ema` | SMA over `period` bars | Bar `period - 1` | `SMA_SEED` |
| MetaStock | First element | Bar `period - 1` | `FIRST_BAR_SEED` |
| Tulip Indicators, pandas `ewm(adjust=False)`, TradeStation `XAverage` | First element | Bar 0 | Not supported |

Two trade-offs beyond that:

- **Speed.** `SMA_SEED` reads only the bars just before the requested start, so a call that starts late in a long array, such as one for the last bar only, is faster. Under `FIRST_BAR_SEED` that call runs from the start of the array, and its cost grows with `startIdx` ([rT16](/spec/settings-threads/#rt16)). A call over the whole array reads the same bars under both.
- **Error on early bars.** `FIRST_BAR_SEED` has the smaller error on the first values published, so its output settles sooner: see [Measured difference](#measured-difference).

## Measured difference

How far the first value a single EMA publishes is from an EMA with unlimited history.

<figure class="uf-fig">
<svg class="uf" viewBox="0 0 760 338" role="img" aria-labelledby="ev-g-t" xmlns="http://www.w3.org/2000/svg">
<text id="ev-g-t" class="uf-zone" x="72" y="20">EMA on real daily prices (gData): lower error is better</text>
<line class="uf-med uf-med1" x1="72" x2="94" y1="42" y2="42"/>
<circle class="uf-dot uf-dot1" cx="83" cy="42" r="4"/>
<text class="uf-legend" x="102" y="46">SMA_SEED (default)</text>
<line class="uf-med uf-med2" x1="257" x2="279" y1="42" y2="42"/>
<rect class="uf-dot uf-dot2" x="264.2" y="38" width="8" height="8"/>
<text class="uf-legend" x="287" y="46">FIRST_BAR_SEED</text>
<line class="uf-whk uf-whk0" x1="416.8" x2="416.8" y1="35" y2="49"/>
<path class="uf-whk uf-whk0" d="M413.8,35h6M413.8,49h6"/>
<circle class="uf-key0" cx="416.8" cy="42" r="2.5"/>
<text class="uf-legend" x="426.8" y="46">median; whisker: 10th to 90th percentile</text>
<line class="uf-grid" x1="72" x2="740" y1="282" y2="282"/>
<text class="uf-tick uf-tick-y" x="64" y="286">0%</text>
<line class="uf-grid" x1="72" x2="740" y1="245.3" y2="245.3"/>
<text class="uf-tick uf-tick-y" x="64" y="249.3">2%</text>
<line class="uf-grid" x1="72" x2="740" y1="208.7" y2="208.7"/>
<text class="uf-tick uf-tick-y" x="64" y="212.7">4%</text>
<line class="uf-grid" x1="72" x2="740" y1="172" y2="172"/>
<text class="uf-tick uf-tick-y" x="64" y="176">6%</text>
<line class="uf-grid" x1="72" x2="740" y1="135.3" y2="135.3"/>
<text class="uf-tick uf-tick-y" x="64" y="139.3">8%</text>
<line class="uf-grid" x1="72" x2="740" y1="98.7" y2="98.7"/>
<text class="uf-tick uf-tick-y" x="64" y="102.7">10%</text>
<line class="uf-grid" x1="72" x2="740" y1="62" y2="62"/>
<text class="uf-tick uf-tick-y" x="64" y="66">12%</text>
<line class="uf-axis" x1="72" x2="72" y1="56" y2="290"/>
<line class="uf-axis" x1="72" x2="740" y1="290" y2="290"/>
<line class="uf-axis" x1="96" x2="96" y1="290" y2="294"/>
<text class="uf-tick" x="96" y="308" text-anchor="middle">5</text>
<line class="uf-axis" x1="212.5" x2="212.5" y1="290" y2="294"/>
<text class="uf-tick" x="212.5" y="308" text-anchor="middle">10</text>
<line class="uf-axis" x1="269.1" x2="269.1" y1="290" y2="294"/>
<text class="uf-tick" x="269.1" y="308" text-anchor="middle">14</text>
<line class="uf-axis" x1="329" x2="329" y1="290" y2="294"/>
<text class="uf-tick" x="329" y="308" text-anchor="middle">20</text>
<line class="uf-axis" x1="397.1" x2="397.1" y1="290" y2="294"/>
<text class="uf-tick" x="397.1" y="308" text-anchor="middle">30</text>
<line class="uf-axis" x1="483" x2="483" y1="290" y2="294"/>
<text class="uf-tick" x="483" y="308" text-anchor="middle">50</text>
<line class="uf-axis" x1="599.5" x2="599.5" y1="290" y2="294"/>
<text class="uf-tick" x="599.5" y="308" text-anchor="middle">100</text>
<line class="uf-axis" x1="716" x2="716" y1="290" y2="294"/>
<text class="uf-tick" x="716" y="308" text-anchor="middle">200</text>
<text class="uf-tick" x="406" y="330" text-anchor="middle">EMA period</text>
<text class="uf-tick" x="16" y="172" text-anchor="middle" transform="rotate(-90 16 172)">|error|, % of series sd</text>
<polyline class="uf-med uf-med1" points="91,276.1 207.5,273.6 264.1,272 324,269.6 392.1,266.1 478,261.2 594.5,250.8 711,233.4"/>
<polyline class="uf-med uf-med2" points="101,278.4 217.5,277 274.1,276.3 334,275.2 402.1,273.7 488,271.1 604.5,265.6 721,257.9"/>
<g><title>EMA(5), SMA_SEED: median 0.324%, 10th to 90th percentile 0.0578% to 1.18%</title><rect class="uf-hit" x="85" y="256.4" width="12" height="28.6"/><path class="uf-whk uf-whk1" d="M91,260.4V280.9M88,260.4h6M88,280.9h6"/><circle class="uf-dot uf-dot1" cx="91" cy="276.1" r="4"/></g>
<g><title>EMA(10), SMA_SEED: median 0.456%, 10th to 90th percentile 0.0796% to 1.67%</title><rect class="uf-hit" x="201.5" y="247.3" width="12" height="37.2"/><path class="uf-whk uf-whk1" d="M207.5,251.3V280.5M204.5,251.3h6M204.5,280.5h6"/><circle class="uf-dot uf-dot1" cx="207.5" cy="273.6" r="4"/></g>
<g><title>EMA(14), SMA_SEED: median 0.546%, 10th to 90th percentile 0.1% to 1.97%</title><rect class="uf-hit" x="258.1" y="241.9" width="12" height="42.3"/><path class="uf-whk uf-whk1" d="M264.1,245.9V280.2M261.1,245.9h6M261.1,280.2h6"/><circle class="uf-dot uf-dot1" cx="264.1" cy="272" r="4"/></g>
<g><title>EMA(20), SMA_SEED: median 0.679%, 10th to 90th percentile 0.124% to 2.38%</title><rect class="uf-hit" x="318" y="234.3" width="12" height="49.4"/><path class="uf-whk uf-whk1" d="M324,238.3V279.7M321,238.3h6M321,279.7h6"/><circle class="uf-dot uf-dot1" cx="324" cy="269.6" r="4"/></g>
<g><title>EMA(30), SMA_SEED: median 0.869%, 10th to 90th percentile 0.162% to 3.01%</title><rect class="uf-hit" x="386.1" y="222.7" width="12" height="60.3"/><path class="uf-whk uf-whk1" d="M392.1,226.7V279M389.1,226.7h6M389.1,279h6"/><circle class="uf-dot uf-dot1" cx="392.1" cy="266.1" r="4"/></g>
<g><title>EMA(50), SMA_SEED: median 1.14%, 10th to 90th percentile 0.2% to 4.29%</title><rect class="uf-hit" x="472" y="199.3" width="12" height="83"/><path class="uf-whk uf-whk1" d="M478,203.3V278.3M475,203.3h6M475,278.3h6"/><circle class="uf-dot uf-dot1" cx="478" cy="261.2" r="4"/></g>
<g><title>EMA(100), SMA_SEED: median 1.7%, 10th to 90th percentile 0.341% to 6.39%</title><rect class="uf-hit" x="588.5" y="160.8" width="12" height="118.9"/><path class="uf-whk uf-whk1" d="M594.5,164.8V275.7M591.5,164.8h6M591.5,275.7h6"/><circle class="uf-dot uf-dot1" cx="594.5" cy="250.8" r="4"/></g>
<g><title>EMA(200), SMA_SEED: median 2.65%, 10th to 90th percentile 0.393% to 11.9%</title><rect class="uf-hit" x="705" y="60.7" width="12" height="218.1"/><path class="uf-whk uf-whk1" d="M711,64.7V274.8M708,64.7h6M708,274.8h6"/><circle class="uf-dot uf-dot1" cx="711" cy="233.4" r="4"/></g>
<g><title>EMA(5), FIRST_BAR_SEED: median 0.199%, 10th to 90th percentile 0.0351% to 0.706%</title><rect class="uf-hit" x="95" y="265.1" width="12" height="20.3"/><path class="uf-whk uf-whk2" d="M101,269.1V281.4M98,269.1h6M98,281.4h6"/><rect class="uf-dot uf-dot2" x="97" y="274.4" width="8" height="8"/></g>
<g><title>EMA(10), FIRST_BAR_SEED: median 0.272%, 10th to 90th percentile 0.0475% to 0.934%</title><rect class="uf-hit" x="211.5" y="260.9" width="12" height="24.3"/><path class="uf-whk uf-whk2" d="M217.5,264.9V281.1M214.5,264.9h6M214.5,281.1h6"/><rect class="uf-dot uf-dot2" x="213.5" y="273" width="8" height="8"/></g>
<g><title>EMA(14), FIRST_BAR_SEED: median 0.31%, 10th to 90th percentile 0.0537% to 1.09%</title><rect class="uf-hit" x="268.1" y="258" width="12" height="27"/><path class="uf-whk uf-whk2" d="M274.1,262V281M271.1,262h6M271.1,281h6"/><rect class="uf-dot uf-dot2" x="270.1" y="272.3" width="8" height="8"/></g>
<g><title>EMA(20), FIRST_BAR_SEED: median 0.372%, 10th to 90th percentile 0.0642% to 1.31%</title><rect class="uf-hit" x="328" y="253.9" width="12" height="30.9"/><path class="uf-whk uf-whk2" d="M334,257.9V280.8M331,257.9h6M331,280.8h6"/><rect class="uf-dot uf-dot2" x="330" y="271.2" width="8" height="8"/></g>
<g><title>EMA(30), FIRST_BAR_SEED: median 0.454%, 10th to 90th percentile 0.0762% to 1.64%</title><rect class="uf-hit" x="396.1" y="247.9" width="12" height="36.7"/><path class="uf-whk uf-whk2" d="M402.1,251.9V280.6M399.1,251.9h6M399.1,280.6h6"/><rect class="uf-dot uf-dot2" x="398.1" y="269.7" width="8" height="8"/></g>
<g><title>EMA(50), FIRST_BAR_SEED: median 0.596%, 10th to 90th percentile 0.106% to 2.11%</title><rect class="uf-hit" x="482" y="239.3" width="12" height="44.8"/><path class="uf-whk uf-whk2" d="M488,243.3V280.1M485,243.3h6M485,280.1h6"/><rect class="uf-dot uf-dot2" x="484" y="267.1" width="8" height="8"/></g>
<g><title>EMA(100), FIRST_BAR_SEED: median 0.892%, 10th to 90th percentile 0.153% to 3.02%</title><rect class="uf-hit" x="598.5" y="222.6" width="12" height="60.6"/><path class="uf-whk uf-whk2" d="M604.5,226.6V279.2M601.5,226.6h6M601.5,279.2h6"/><rect class="uf-dot uf-dot2" x="600.5" y="261.6" width="8" height="8"/></g>
<g><title>EMA(200), FIRST_BAR_SEED: median 1.32%, 10th to 90th percentile 0.242% to 4.59%</title><rect class="uf-hit" x="715" y="193.8" width="12" height="87.8"/><path class="uf-whk uf-whk2" d="M721,197.8V277.6M718,197.8h6M718,277.6h6"/><rect class="uf-dot uf-dot2" x="717" y="253.9" width="8" height="8"/></g>
</svg>
</figure>

<figure class="uf-fig">
<svg class="uf" viewBox="0 0 760 338" role="img" aria-labelledby="ev-rw-t" xmlns="http://www.w3.org/2000/svg">
<text id="ev-rw-t" class="uf-zone" x="72" y="20">EMA on a random walk (synthetic): lower error is better</text>
<line class="uf-med uf-med1" x1="72" x2="94" y1="42" y2="42"/>
<circle class="uf-dot uf-dot1" cx="83" cy="42" r="4"/>
<text class="uf-legend" x="102" y="46">SMA_SEED (default)</text>
<line class="uf-med uf-med2" x1="257" x2="279" y1="42" y2="42"/>
<rect class="uf-dot uf-dot2" x="264.2" y="38" width="8" height="8"/>
<text class="uf-legend" x="287" y="46">FIRST_BAR_SEED</text>
<line class="uf-whk uf-whk0" x1="416.8" x2="416.8" y1="35" y2="49"/>
<path class="uf-whk uf-whk0" d="M413.8,35h6M413.8,49h6"/>
<circle class="uf-key0" cx="416.8" cy="42" r="2.5"/>
<text class="uf-legend" x="426.8" y="46">median; whisker: 10th to 90th percentile</text>
<line class="uf-grid" x1="72" x2="740" y1="282" y2="282"/>
<text class="uf-tick uf-tick-y" x="64" y="286">0%</text>
<line class="uf-grid" x1="72" x2="740" y1="238" y2="238"/>
<text class="uf-tick uf-tick-y" x="64" y="242">2%</text>
<line class="uf-grid" x1="72" x2="740" y1="194" y2="194"/>
<text class="uf-tick uf-tick-y" x="64" y="198">4%</text>
<line class="uf-grid" x1="72" x2="740" y1="150" y2="150"/>
<text class="uf-tick uf-tick-y" x="64" y="154">6%</text>
<line class="uf-grid" x1="72" x2="740" y1="106" y2="106"/>
<text class="uf-tick uf-tick-y" x="64" y="110">8%</text>
<line class="uf-grid" x1="72" x2="740" y1="62" y2="62"/>
<text class="uf-tick uf-tick-y" x="64" y="66">10%</text>
<line class="uf-axis" x1="72" x2="72" y1="56" y2="290"/>
<line class="uf-axis" x1="72" x2="740" y1="290" y2="290"/>
<line class="uf-axis" x1="96" x2="96" y1="290" y2="294"/>
<text class="uf-tick" x="96" y="308" text-anchor="middle">5</text>
<line class="uf-axis" x1="212.5" x2="212.5" y1="290" y2="294"/>
<text class="uf-tick" x="212.5" y="308" text-anchor="middle">10</text>
<line class="uf-axis" x1="269.1" x2="269.1" y1="290" y2="294"/>
<text class="uf-tick" x="269.1" y="308" text-anchor="middle">14</text>
<line class="uf-axis" x1="329" x2="329" y1="290" y2="294"/>
<text class="uf-tick" x="329" y="308" text-anchor="middle">20</text>
<line class="uf-axis" x1="397.1" x2="397.1" y1="290" y2="294"/>
<text class="uf-tick" x="397.1" y="308" text-anchor="middle">30</text>
<line class="uf-axis" x1="483" x2="483" y1="290" y2="294"/>
<text class="uf-tick" x="483" y="308" text-anchor="middle">50</text>
<line class="uf-axis" x1="599.5" x2="599.5" y1="290" y2="294"/>
<text class="uf-tick" x="599.5" y="308" text-anchor="middle">100</text>
<line class="uf-axis" x1="716" x2="716" y1="290" y2="294"/>
<text class="uf-tick" x="716" y="308" text-anchor="middle">200</text>
<text class="uf-tick" x="406" y="330" text-anchor="middle">EMA period</text>
<text class="uf-tick" x="16" y="172" text-anchor="middle" transform="rotate(-90 16 172)">|error|, % of series sd</text>
<polyline class="uf-med uf-med1" points="91,270.8 207.5,266.7 264.1,264 324,260.6 392.1,256.1 478,248.2 594.5,234.7 711,218.7"/>
<polyline class="uf-med uf-med2" points="101,275.2 217.5,273.1 274.1,271.7 334,269.9 402.1,267.4 488,263.5 604.5,255.9 721,245.8"/>
<g><title>EMA(5), SMA_SEED: median 0.511%, 10th to 90th percentile 0.0688% to 1.53%</title><rect class="uf-hit" x="85" y="244.4" width="12" height="40.1"/><path class="uf-whk uf-whk1" d="M91,248.4V280.5M88,248.4h6M88,280.5h6"/><circle class="uf-dot uf-dot1" cx="91" cy="270.8" r="4"/></g>
<g><title>EMA(10), SMA_SEED: median 0.694%, 10th to 90th percentile 0.0961% to 2.05%</title><rect class="uf-hit" x="201.5" y="232.8" width="12" height="51.1"/><path class="uf-whk uf-whk1" d="M207.5,236.8V279.9M204.5,236.8h6M204.5,279.9h6"/><circle class="uf-dot uf-dot1" cx="207.5" cy="266.7" r="4"/></g>
<g><title>EMA(14), SMA_SEED: median 0.817%, 10th to 90th percentile 0.111% to 2.41%</title><rect class="uf-hit" x="258.1" y="224.9" width="12" height="58.7"/><path class="uf-whk uf-whk1" d="M264.1,228.9V279.6M261.1,228.9h6M261.1,279.6h6"/><circle class="uf-dot uf-dot1" cx="264.1" cy="264" r="4"/></g>
<g><title>EMA(20), SMA_SEED: median 0.973%, 10th to 90th percentile 0.131% to 2.91%</title><rect class="uf-hit" x="318" y="213.9" width="12" height="69.2"/><path class="uf-whk uf-whk1" d="M324,217.9V279.1M321,217.9h6M321,279.1h6"/><circle class="uf-dot uf-dot1" cx="324" cy="260.6" r="4"/></g>
<g><title>EMA(30), SMA_SEED: median 1.18%, 10th to 90th percentile 0.161% to 3.6%</title><rect class="uf-hit" x="386.1" y="198.8" width="12" height="83.7"/><path class="uf-whk uf-whk1" d="M392.1,202.8V278.5M389.1,202.8h6M389.1,278.5h6"/><circle class="uf-dot uf-dot1" cx="392.1" cy="256.1" r="4"/></g>
<g><title>EMA(50), SMA_SEED: median 1.53%, 10th to 90th percentile 0.208% to 4.66%</title><rect class="uf-hit" x="472" y="175.4" width="12" height="106"/><path class="uf-whk uf-whk1" d="M478,179.4V277.4M475,179.4h6M475,277.4h6"/><circle class="uf-dot uf-dot1" cx="478" cy="248.2" r="4"/></g>
<g><title>EMA(100), SMA_SEED: median 2.15%, 10th to 90th percentile 0.284% to 6.5%</title><rect class="uf-hit" x="588.5" y="135" width="12" height="144.7"/><path class="uf-whk uf-whk1" d="M594.5,139V275.7M591.5,139h6M591.5,275.7h6"/><circle class="uf-dot uf-dot1" cx="594.5" cy="234.7" r="4"/></g>
<g><title>EMA(200), SMA_SEED: median 2.88%, 10th to 90th percentile 0.364% to 8.9%</title><rect class="uf-hit" x="705" y="82.3" width="12" height="195.7"/><path class="uf-whk uf-whk1" d="M711,86.3V274M708,86.3h6M708,274h6"/><circle class="uf-dot uf-dot1" cx="711" cy="218.7" r="4"/></g>
<g><title>EMA(5), FIRST_BAR_SEED: median 0.307%, 10th to 90th percentile 0.0421% to 0.926%</title><rect class="uf-hit" x="95" y="257.6" width="12" height="27.5"/><path class="uf-whk uf-whk2" d="M101,261.6V281.1M98,261.6h6M98,281.1h6"/><rect class="uf-dot uf-dot2" x="97" y="271.2" width="8" height="8"/></g>
<g><title>EMA(10), FIRST_BAR_SEED: median 0.405%, 10th to 90th percentile 0.0557% to 1.22%</title><rect class="uf-hit" x="211.5" y="251.1" width="12" height="33.6"/><path class="uf-whk uf-whk2" d="M217.5,255.1V280.8M214.5,255.1h6M214.5,280.8h6"/><rect class="uf-dot uf-dot2" x="213.5" y="269.1" width="8" height="8"/></g>
<g><title>EMA(14), FIRST_BAR_SEED: median 0.468%, 10th to 90th percentile 0.0639% to 1.41%</title><rect class="uf-hit" x="268.1" y="247" width="12" height="37.6"/><path class="uf-whk uf-whk2" d="M274.1,251V280.6M271.1,251h6M271.1,280.6h6"/><rect class="uf-dot uf-dot2" x="270.1" y="267.7" width="8" height="8"/></g>
<g><title>EMA(20), FIRST_BAR_SEED: median 0.55%, 10th to 90th percentile 0.0756% to 1.66%</title><rect class="uf-hit" x="328" y="241.6" width="12" height="42.8"/><path class="uf-whk uf-whk2" d="M334,245.6V280.3M331,245.6h6M331,280.3h6"/><rect class="uf-dot uf-dot2" x="330" y="265.9" width="8" height="8"/></g>
<g><title>EMA(30), FIRST_BAR_SEED: median 0.665%, 10th to 90th percentile 0.0904% to 1.98%</title><rect class="uf-hit" x="396.1" y="234.4" width="12" height="49.6"/><path class="uf-whk uf-whk2" d="M402.1,238.4V280M399.1,238.4h6M399.1,280h6"/><rect class="uf-dot uf-dot2" x="398.1" y="263.4" width="8" height="8"/></g>
<g><title>EMA(50), FIRST_BAR_SEED: median 0.842%, 10th to 90th percentile 0.116% to 2.5%</title><rect class="uf-hit" x="482" y="223" width="12" height="60.4"/><path class="uf-whk uf-whk2" d="M488,227V279.5M485,227h6M485,279.5h6"/><rect class="uf-dot uf-dot2" x="484" y="259.5" width="8" height="8"/></g>
<g><title>EMA(100), FIRST_BAR_SEED: median 1.19%, 10th to 90th percentile 0.161% to 3.43%</title><rect class="uf-hit" x="598.5" y="202.4" width="12" height="80"/><path class="uf-whk uf-whk2" d="M604.5,206.4V278.5M601.5,206.4h6M601.5,278.5h6"/><rect class="uf-dot uf-dot2" x="600.5" y="251.9" width="8" height="8"/></g>
<g><title>EMA(200), FIRST_BAR_SEED: median 1.64%, 10th to 90th percentile 0.218% to 4.72%</title><rect class="uf-hit" x="715" y="174.1" width="12" height="107.1"/><path class="uf-whk uf-whk2" d="M721,178.1V277.2M718,178.1h6M718,277.2h6"/><rect class="uf-dot uf-dot2" x="717" y="241.8" width="8" height="8"/></g>
</svg>
</figure>

The same measure for two functions that chain EMAs.

<figure class="uf-fig">
<svg class="uf" viewBox="0 0 760 338" role="img" aria-labelledby="ev-tema-t" xmlns="http://www.w3.org/2000/svg">
<text id="ev-tema-t" class="uf-zone" x="72" y="20">TEMA on real daily prices (gData): lower error is better</text>
<line class="uf-med uf-med1" x1="72" x2="94" y1="42" y2="42"/>
<circle class="uf-dot uf-dot1" cx="83" cy="42" r="4"/>
<text class="uf-legend" x="102" y="46">SMA_SEED (default)</text>
<line class="uf-med uf-med2" x1="257" x2="279" y1="42" y2="42"/>
<rect class="uf-dot uf-dot2" x="264.2" y="38" width="8" height="8"/>
<text class="uf-legend" x="287" y="46">FIRST_BAR_SEED</text>
<line class="uf-whk uf-whk0" x1="416.8" x2="416.8" y1="35" y2="49"/>
<path class="uf-whk uf-whk0" d="M413.8,35h6M413.8,49h6"/>
<circle class="uf-key0" cx="416.8" cy="42" r="2.5"/>
<text class="uf-legend" x="426.8" y="46">median; whisker: 10th to 90th percentile</text>
<line class="uf-grid" x1="72" x2="740" y1="282" y2="282"/>
<text class="uf-tick uf-tick-y" x="64" y="286">0%</text>
<line class="uf-grid" x1="72" x2="740" y1="238" y2="238"/>
<text class="uf-tick uf-tick-y" x="64" y="242">1%</text>
<line class="uf-grid" x1="72" x2="740" y1="194" y2="194"/>
<text class="uf-tick uf-tick-y" x="64" y="198">2%</text>
<line class="uf-grid" x1="72" x2="740" y1="150" y2="150"/>
<text class="uf-tick uf-tick-y" x="64" y="154">3%</text>
<line class="uf-grid" x1="72" x2="740" y1="106" y2="106"/>
<text class="uf-tick uf-tick-y" x="64" y="110">4%</text>
<line class="uf-grid" x1="72" x2="740" y1="62" y2="62"/>
<text class="uf-tick uf-tick-y" x="64" y="66">5%</text>
<line class="uf-axis" x1="72" x2="72" y1="56" y2="290"/>
<line class="uf-axis" x1="72" x2="740" y1="290" y2="290"/>
<line class="uf-axis" x1="96" x2="96" y1="290" y2="294"/>
<text class="uf-tick" x="96" y="308" text-anchor="middle">5</text>
<line class="uf-axis" x1="212.5" x2="212.5" y1="290" y2="294"/>
<text class="uf-tick" x="212.5" y="308" text-anchor="middle">10</text>
<line class="uf-axis" x1="269.1" x2="269.1" y1="290" y2="294"/>
<text class="uf-tick" x="269.1" y="308" text-anchor="middle">14</text>
<line class="uf-axis" x1="329" x2="329" y1="290" y2="294"/>
<text class="uf-tick" x="329" y="308" text-anchor="middle">20</text>
<line class="uf-axis" x1="397.1" x2="397.1" y1="290" y2="294"/>
<text class="uf-tick" x="397.1" y="308" text-anchor="middle">30</text>
<line class="uf-axis" x1="483" x2="483" y1="290" y2="294"/>
<text class="uf-tick" x="483" y="308" text-anchor="middle">50</text>
<line class="uf-axis" x1="599.5" x2="599.5" y1="290" y2="294"/>
<text class="uf-tick" x="599.5" y="308" text-anchor="middle">100</text>
<line class="uf-axis" x1="716" x2="716" y1="290" y2="294"/>
<text class="uf-tick" x="716" y="308" text-anchor="middle">200</text>
<text class="uf-tick" x="406" y="330" text-anchor="middle">TEMA period</text>
<text class="uf-tick" x="16" y="172" text-anchor="middle" transform="rotate(-90 16 172)">|error|, % of TEMA's sd</text>
<polyline class="uf-med uf-med1" points="91,276.5 207.5,273.7 264.1,272.1 324,269.9 392.1,267.1 478,261.4 594.5,250.9 711,240.4"/>
<polyline class="uf-med uf-med2" points="101,277.3 217.5,275 274.1,273.8 334,272.2 402.1,269.6 488,266.6 604.5,259.9 721,244"/>
<g><title>TEMA(5), SMA_SEED: median 0.125%, 10th to 90th percentile 0.0215% to 0.449%</title><rect class="uf-hit" x="85" y="258.2" width="12" height="26.8"/><path class="uf-whk uf-whk1" d="M91,262.2V281.1M88,262.2h6M88,281.1h6"/><circle class="uf-dot uf-dot1" cx="91" cy="276.5" r="4"/></g>
<g><title>TEMA(10), SMA_SEED: median 0.188%, 10th to 90th percentile 0.0319% to 0.656%</title><rect class="uf-hit" x="201.5" y="249.1" width="12" height="35.4"/><path class="uf-whk uf-whk1" d="M207.5,253.1V280.6M204.5,253.1h6M204.5,280.6h6"/><circle class="uf-dot uf-dot1" cx="207.5" cy="273.7" r="4"/></g>
<g><title>TEMA(14), SMA_SEED: median 0.226%, 10th to 90th percentile 0.0442% to 0.815%</title><rect class="uf-hit" x="258.1" y="242.1" width="12" height="41.9"/><path class="uf-whk uf-whk1" d="M264.1,246.1V280.1M261.1,246.1h6M261.1,280.1h6"/><circle class="uf-dot uf-dot1" cx="264.1" cy="272.1" r="4"/></g>
<g><title>TEMA(20), SMA_SEED: median 0.274%, 10th to 90th percentile 0.0537% to 1.01%</title><rect class="uf-hit" x="318" y="233.7" width="12" height="49.9"/><path class="uf-whk uf-whk1" d="M324,237.7V279.6M321,237.7h6M321,279.6h6"/><circle class="uf-dot uf-dot1" cx="324" cy="269.9" r="4"/></g>
<g><title>TEMA(30), SMA_SEED: median 0.338%, 10th to 90th percentile 0.0671% to 1.32%</title><rect class="uf-hit" x="386.1" y="219.9" width="12" height="63.1"/><path class="uf-whk uf-whk1" d="M392.1,223.9V279M389.1,223.9h6M389.1,279h6"/><circle class="uf-dot uf-dot1" cx="392.1" cy="267.1" r="4"/></g>
<g><title>TEMA(50), SMA_SEED: median 0.469%, 10th to 90th percentile 0.102% to 1.75%</title><rect class="uf-hit" x="472" y="201" width="12" height="80.5"/><path class="uf-whk uf-whk1" d="M478,205V277.5M475,205h6M475,277.5h6"/><circle class="uf-dot uf-dot1" cx="478" cy="261.4" r="4"/></g>
<g><title>TEMA(100), SMA_SEED: median 0.707%, 10th to 90th percentile 0.129% to 3.06%</title><rect class="uf-hit" x="588.5" y="143.2" width="12" height="137.2"/><path class="uf-whk uf-whk1" d="M594.5,147.2V276.3M591.5,147.2h6M591.5,276.3h6"/><circle class="uf-dot uf-dot1" cx="594.5" cy="250.9" r="4"/></g>
<g><title>TEMA(200), SMA_SEED: median 0.945%, 10th to 90th percentile 0.151% to 4.87%</title><rect class="uf-hit" x="705" y="63.8" width="12" height="215.6"/><path class="uf-whk uf-whk1" d="M711,67.8V275.4M708,67.8h6M708,275.4h6"/><circle class="uf-dot uf-dot1" cx="711" cy="240.4" r="4"/></g>
<g><title>TEMA(5), FIRST_BAR_SEED: median 0.107%, 10th to 90th percentile 0.0189% to 0.376%</title><rect class="uf-hit" x="95" y="261.5" width="12" height="23.7"/><path class="uf-whk uf-whk2" d="M101,265.5V281.2M98,265.5h6M98,281.2h6"/><rect class="uf-dot uf-dot2" x="97" y="273.3" width="8" height="8"/></g>
<g><title>TEMA(10), FIRST_BAR_SEED: median 0.158%, 10th to 90th percentile 0.0285% to 0.559%</title><rect class="uf-hit" x="211.5" y="253.4" width="12" height="31.3"/><path class="uf-whk uf-whk2" d="M217.5,257.4V280.7M214.5,257.4h6M214.5,280.7h6"/><rect class="uf-dot uf-dot2" x="213.5" y="271" width="8" height="8"/></g>
<g><title>TEMA(14), FIRST_BAR_SEED: median 0.187%, 10th to 90th percentile 0.0352% to 0.678%</title><rect class="uf-hit" x="268.1" y="248.2" width="12" height="36.3"/><path class="uf-whk uf-whk2" d="M274.1,252.2V280.5M271.1,252.2h6M271.1,280.5h6"/><rect class="uf-dot uf-dot2" x="270.1" y="269.8" width="8" height="8"/></g>
<g><title>TEMA(20), FIRST_BAR_SEED: median 0.222%, 10th to 90th percentile 0.0391% to 0.805%</title><rect class="uf-hit" x="328" y="242.6" width="12" height="41.7"/><path class="uf-whk uf-whk2" d="M334,246.6V280.3M331,246.6h6M331,280.3h6"/><rect class="uf-dot uf-dot2" x="330" y="268.2" width="8" height="8"/></g>
<g><title>TEMA(30), FIRST_BAR_SEED: median 0.282%, 10th to 90th percentile 0.0451% to 1.01%</title><rect class="uf-hit" x="396.1" y="233.4" width="12" height="50.6"/><path class="uf-whk uf-whk2" d="M402.1,237.4V280M399.1,237.4h6M399.1,280h6"/><rect class="uf-dot uf-dot2" x="398.1" y="265.6" width="8" height="8"/></g>
<g><title>TEMA(50), FIRST_BAR_SEED: median 0.35%, 10th to 90th percentile 0.0634% to 1.35%</title><rect class="uf-hit" x="482" y="218.4" width="12" height="64.8"/><path class="uf-whk uf-whk2" d="M488,222.4V279.2M485,222.4h6M485,279.2h6"/><rect class="uf-dot uf-dot2" x="484" y="262.6" width="8" height="8"/></g>
<g><title>TEMA(100), FIRST_BAR_SEED: median 0.503%, 10th to 90th percentile 0.107% to 2.01%</title><rect class="uf-hit" x="598.5" y="189.7" width="12" height="91.6"/><path class="uf-whk uf-whk2" d="M604.5,193.7V277.3M601.5,193.7h6M601.5,277.3h6"/><rect class="uf-dot uf-dot2" x="600.5" y="255.9" width="8" height="8"/></g>
<g><title>TEMA(200), FIRST_BAR_SEED: median 0.863%, 10th to 90th percentile 0.0982% to 2.75%</title><rect class="uf-hit" x="715" y="157.1" width="12" height="124.6"/><path class="uf-whk uf-whk2" d="M721,161.1V277.7M718,161.1h6M718,277.7h6"/><rect class="uf-dot uf-dot2" x="717" y="240" width="8" height="8"/></g>
</svg>
</figure>

<figure class="uf-fig">
<svg class="uf" viewBox="0 0 760 338" role="img" aria-labelledby="ev-macd-t" xmlns="http://www.w3.org/2000/svg">
<text id="ev-macd-t" class="uf-zone" x="72" y="20">MACD(12,26,9) on real daily prices (gData): lower error is better</text>
<circle class="uf-dot uf-dot1" cx="83" cy="42" r="4"/>
<text class="uf-legend" x="102" y="46">SMA_SEED (default)</text>
<rect class="uf-dot uf-dot2" x="264.2" y="38" width="8" height="8"/>
<text class="uf-legend" x="287" y="46">FIRST_BAR_SEED</text>
<line class="uf-whk uf-whk0" x1="416.8" x2="416.8" y1="35" y2="49"/>
<path class="uf-whk uf-whk0" d="M413.8,35h6M413.8,49h6"/>
<circle class="uf-key0" cx="416.8" cy="42" r="2.5"/>
<text class="uf-legend" x="426.8" y="46">median; whisker: 10th to 90th percentile</text>
<line class="uf-grid" x1="72" x2="740" y1="282" y2="282"/>
<text class="uf-tick uf-tick-y" x="64" y="286">0%</text>
<line class="uf-grid" x1="72" x2="740" y1="238" y2="238"/>
<text class="uf-tick uf-tick-y" x="64" y="242">10%</text>
<line class="uf-grid" x1="72" x2="740" y1="194" y2="194"/>
<text class="uf-tick uf-tick-y" x="64" y="198">20%</text>
<line class="uf-grid" x1="72" x2="740" y1="150" y2="150"/>
<text class="uf-tick uf-tick-y" x="64" y="154">30%</text>
<line class="uf-grid" x1="72" x2="740" y1="106" y2="106"/>
<text class="uf-tick uf-tick-y" x="64" y="110">40%</text>
<line class="uf-grid" x1="72" x2="740" y1="62" y2="62"/>
<text class="uf-tick uf-tick-y" x="64" y="66">50%</text>
<line class="uf-axis" x1="72" x2="72" y1="56" y2="290"/>
<line class="uf-axis" x1="72" x2="740" y1="290" y2="290"/>
<line class="uf-axis" x1="200" x2="200" y1="290" y2="294"/>
<text class="uf-tick" x="200" y="308" text-anchor="middle">line</text>
<line class="uf-axis" x1="406" x2="406" y1="290" y2="294"/>
<text class="uf-tick" x="406" y="308" text-anchor="middle">signal</text>
<line class="uf-axis" x1="612" x2="612" y1="290" y2="294"/>
<text class="uf-tick" x="612" y="308" text-anchor="middle">histogram</text>
<text class="uf-tick" x="406" y="330" text-anchor="middle">MACD(12,26,9) output</text>
<text class="uf-tick" x="16" y="172" text-anchor="middle" transform="rotate(-90 16 172)">|error|, % of the MACD line's sd</text>
<g><title>MACD(12,26,9) line, SMA_SEED: median 8.29%, 10th to 90th percentile 1.5% to 29.3%</title><rect class="uf-hit" x="185" y="149.3" width="12" height="130.1"/><path class="uf-whk uf-whk1" d="M191,153.3V275.4M188,153.3h6M188,275.4h6"/><circle class="uf-dot uf-dot1" cx="191" cy="245.5" r="4"/></g>
<g><title>MACD(12,26,9) signal, SMA_SEED: median 13%, 10th to 90th percentile 2.18% to 46.1%</title><rect class="uf-hit" x="391" y="75.3" width="12" height="201.1"/><path class="uf-whk uf-whk1" d="M397,79.3V272.4M394,79.3h6M394,272.4h6"/><circle class="uf-dot uf-dot1" cx="397" cy="224.8" r="4"/></g>
<g><title>MACD(12,26,9) histogram, SMA_SEED: median 5.06%, 10th to 90th percentile 0.841% to 17.8%</title><rect class="uf-hit" x="597" y="199.8" width="12" height="82.5"/><path class="uf-whk uf-whk1" d="M603,203.8V278.3M600,203.8h6M600,278.3h6"/><circle class="uf-dot uf-dot1" cx="603" cy="259.8" r="4"/></g>
<g><title>MACD(12,26,9) line, FIRST_BAR_SEED: median 4.32%, 10th to 90th percentile 0.748% to 15.5%</title><rect class="uf-hit" x="203" y="209.8" width="12" height="72.9"/><path class="uf-whk uf-whk2" d="M209,213.8V278.7M206,213.8h6M206,278.7h6"/><rect class="uf-dot uf-dot2" x="205" y="259" width="8" height="8"/></g>
<g><title>MACD(12,26,9) signal, FIRST_BAR_SEED: median 6.27%, 10th to 90th percentile 1.1% to 23%</title><rect class="uf-hit" x="409" y="176.9" width="12" height="104.2"/><path class="uf-whk uf-whk2" d="M415,180.9V277.2M412,180.9h6M412,277.2h6"/><rect class="uf-dot uf-dot2" x="411" y="250.4" width="8" height="8"/></g>
<g><title>MACD(12,26,9) histogram, FIRST_BAR_SEED: median 2.22%, 10th to 90th percentile 0.352% to 8.1%</title><rect class="uf-hit" x="615" y="242.4" width="12" height="42.1"/><path class="uf-whk uf-whk2" d="M621,246.4V280.5M618,246.4h6M618,280.5h6"/><rect class="uf-dot uf-dot2" x="617" y="268.2" width="8" height="8"/></g>
</svg>
</figure>

The full method and results: [EMA seeding study](https://github.com/TA-Lib/ta-lib/blob/main/docs/studies/ema-seeding/README.md).

## Why does the error range increase as the period increases?

- **Short period, small lag.** An EMA(5) follows the price closely, so the first element is already near where an EMA with unlimited history would be. The seed is a good guess, and the error of the first value published is small.
- **Long period, large lag.** An EMA(200) trails far behind the price, so the first element is a poor guess for where an EMA with unlimited history would be. The seed starts far off, and about 14% of that miss is still in the first value published.

## How to reduce the error?

Whichever seed is used, the first values an EMA publishes still carry part of the seed's error. The proper way to get numerically accurate values is to have TA-Lib leave those bars out: see [Unstable Period](/api/unstable-period/).

## Which functions follow the setting

[APO](/functions/apo), [BBANDS](/functions/bbands), [BBW](/functions/bbw), [CVI](/functions/cvi), [DEMA](/functions/dema), [DOSC](/functions/dosc), [EFI](/functions/efi), [EMA](/functions/ema), [ERI](/functions/eri), [KC](/functions/kc), [KDJ](/functions/kdj), [KSTEXT](/functions/kstext), [MA](/functions/ma), [MACD](/functions/macd), [MACDEXT](/functions/macdext), [MACDFIX](/functions/macdfix), [MACDV](/functions/macdv), [MAENV](/functions/maenv), [MASSI](/functions/massi), [MAVP](/functions/mavp), [PERCENTB](/functions/percentb), [PPO](/functions/ppo), [PSO](/functions/pso), [PVO](/functions/pvo), [SMI](/functions/smi), [STC](/functions/stc), [STOCH](/functions/stoch), [STOCHF](/functions/stochf), [STOCHRSI](/functions/stochrsi), [TEMA](/functions/tema), [TRIX](/functions/trix), [TSI](/functions/tsi), [WAVETREND](/functions/wavetrend), [ZLEMA](/functions/zlema).

A function that takes an MA type follows the setting only with an `EMA`, `DEMA`, `TEMA` or `ZLEMA` type.

## See also

- [Specification](/spec/settings-threads/#ema-variation): the rules of the setting
- Core API: [C/C++](/api/), [Rust](/api/rust/), [Java](/api/java/), [C#](/api/csharp/), [TypeScript](/api/typescript/), [Go](/api/go/)
- [Unstable Period](/api/unstable-period/)
- [Candlestick Settings](/api/candle-settings/)
