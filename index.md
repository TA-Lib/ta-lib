---
url: 'https://ta-lib.org/index.md'
description: >-
  Open-source technical analysis library: 200+ indicators (ADX, MACD, RSI),
  candlestick patterns, batch and streaming. Native C/C++, Java, C# and Rust,
  plus Python and R wrappers.
---
# TA-Lib

<div align="center" class="home-hero">

<img src="/assets/images/home.png" alt="TA-Lib">

### Technical analysis, battle-tested since 2001

</div>

TA-Lib implements the standard technical analysis algorithms used across the industry, and is often the reference other libraries test their results against.

<div class="home-cards">
<div class="home-card">
<h4>200+ indicators</h4>
<p>ADX, MACD, RSI, Stochastic, Bollinger Bands and more, plus candlestick pattern recognition. <a href="/functions/">See the complete list</a>.</p>
</div>
<div class="home-card">
<h4>Batch and streaming</h4>
<p>Compute a whole array, or update one bar at a time without recomputing the history. <a href="/api/stream/">Streaming API</a>.</p>
</div>
<div class="home-card">
<h4>Native in four languages</h4>
<p><a href="/api/">C/C++</a>, <a href="/api/java/">Java</a>, <a href="/api/csharp/">C#</a> and <a href="/api/rust/">Rust</a>. Wrappers for Python, R, and <a href="/install/#wrappers">more</a>.</p>
</div>
<div class="home-card">
<h4>Made for integration</h4>
<p>Glue-code friendly. Function metadata drives your UI, parameter range, automation and generated bindings. <a href="/api/abstract/">Abstract API</a>.</p>
</div>
</div>

A 30-bar simple moving average, over an array and then on a live feed:



::: tabs#lang

@tab C/C++

```c
#include <ta_libc.h>

TA_Initialize();

/* Batch: out[0] is the value at bar outBegIdx. */
int outBegIdx, outNBElement;
TA_SMA( 0, n - 1, close, 30, &outBegIdx, &outNBElement, out );

/* Streaming: open on history, then one call per bar. */
TA_SMA_Stream *s;
double sma;
TA_SMA_Open( &s, close, n, 30, &sma );
TA_SMA_Update( s, newClose, &sma );     /* a bar closed */
TA_SMA_Peek( s, formingClose, &sma );   /* bar still forming; state unchanged */
TA_SMA_Close( s );
```

[API](/api/) · [Streaming](/api/stream/)

@tab Rust

```rust
use ta_lib::Core;

let core = Core::new();

// Batch: out[0] is the value at bar range.beg_idx.
let mut out = vec![0.0; close.len()];
let range = core.sma(0, close.len() - 1, &close, 30, &mut out)?;

// Streaming: open on history, then one call per bar.
let (mut s, last) = core.sma_open(&close, 30)?;
let v = s.update(new_close)?;        // a bar closed
let p = s.peek(forming_close)?;      // bar still forming; state unchanged
```

[API](/api/rust/) · [Streaming](/api/rust/stream/)

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

Core core = Core.DEFAULT;

// Batch: out[0] is the value at bar r.begIdx().
double[] out = new double[close.length];
OutRange r = core.sma(0, close.length - 1, close, 30, out);

// Streaming: open on history, then one call per bar.
Core.SmaStream s = core.smaOpen(close, 30);
double v = s.update(newClose);       // a bar closed
double p = s.peek(formingClose);     // bar still forming; state unchanged
```

[API](/api/java/) · [Streaming](/api/java/stream/)

@tab C#

```csharp
using TALib;

var core = Core.Default;

// Batch: outReal[0] is the value at bar r.BegIdx.
var outReal = new double[close.Length];
OutRange r = core.Sma(0, close.Length - 1, close, 30, outReal);

// Streaming: open on history, then one call per bar.
Core.SmaStream s = core.SmaOpen(close, 30);
double v = s.Update(newClose);       // a bar closed
double p = s.Peek(formingClose);     // bar still forming; state unchanged
```

[API](/api/csharp/) · [Streaming](/api/csharp/stream/)

:::



Open-Source (BSD License). Can be freely integrated in your own open-source or commercial applications.
