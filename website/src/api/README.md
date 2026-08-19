---
title: C/C++ Core API
description: "Calling TA-Lib from C/C++: initialization, the batch calling pattern, sizing outputs with the lookback, return codes, the abstraction layer and thread safety."
toc: false
---

<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 How to add TA-Lib to your app</a></p>

<p><a href="#ta_func">3.0 Calling into TA-Lib</a></p>

<blockquote>
<p><a href="#init">3.1 Initialize and Shutdown</a><br>
<a href="#direct_call">3.2 Batch Processing</a><br>
<a href="#output_size">3.3 Output Size and Lookback</a><br>
<a href="#retcode">3.4 Return Codes</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced Features</a></p>
<blockquote>
<p><a href="#abstract">4.1 Abstraction Layer</a><br>
<a href="#numerical_stability">4.2 Numerical Stability</a><br>
<a href="#candle_settings">4.3 Candlestick Settings</a><br>
<a href="#input_type">4.4 Input Type: float vs. double</a><br>
<a href="#multithreading">4.5 High-performance Multi-threading</a></p>
</blockquote>

## 1.0 Introduction {#intro}

<p>The <b>Core API</b> provides:</p>
<ul>
  <li>The lifecycle of the library (<a href="#init">TA_Initialize / TA_Shutdown</a>).</li>
  <li>The global settings (e.g. <a href="/api/unstable-period/">TA_SetUnstablePeriod</a>, <a href="/api/candle-settings/">TA_SetCandleSettings</a>).</li>
  <li>Every <a href="#ta_func">TA function</a>, each processing a whole array of data at once.</li>
  <li>An optional <a href="#abstract">abstraction layer</a> for calling those functions dynamically.</li>
</ul>
<p>To process a live feed one bar at a time instead, see the companion <a href="/api/stream/">C/C++ Streaming API</a>.</p>
<p>You must first <a href="/install/c/">install the C/C++ library</a>, which will provide all the shared/static libraries and headers needed to compile and link your program.</p>

## 2.0 How to add TA-Lib to your app {#build}

<p>In your source code, add <b>#include &quot;ta_libc.h&quot;</b> and link to the library named "ta-lib".</p>

You may need to add TA-Lib to the compiler's and linker's search paths. For example, with gcc:

```sh
-I/usr/local/include/ta-lib -L/usr/local/lib -lta-lib
```

The paths depend on the method used to install. Typical locations for headers (`-I`) are:

- `/usr/local/include/ta-lib`
- `/usr/include/ta-lib`
- `/opt/include/ta-lib`

Typical locations for the libraries (`-L`) are:

- `/usr/lib`
- `/usr/lib64`
- `/usr/local/lib`
- `/usr/local/lib64`
- `/opt/lib`
- `/opt/local/lib`

For [homebrew](https://formulae.brew.sh/formula/ta-lib), use <b>brew --prefix ta-lib</b> to find the paths.

For Windows, look into <b>C:\Program Files\TA-Lib</b> for 64-bit and <b>C:\Program Files (x86)\TA-Lib</b> for 32-bit.

## 3.0 Calling into TA-Lib {#ta_func}

<p>All of TA-Lib's public functions are declared in <a href="https://github.com/TA-Lib/ta-lib/blob/main/include">the include/*.h headers</a>.</p>

### 3.1 Initialize and Shutdown {#init}

<pre>TA_RetCode TA_Initialize( void );
TA_RetCode TA_Shutdown( void );</pre>
<p><b>TA_Initialize</b> must be called once (and only once), from a single thread, prior to any other API function. After it returns TA_SUCCESS, you can start processing your data in three ways: <a href="#direct_call">batch processing</a>, the <a href="/api/stream/">streaming API</a> or through the <a href="#abstract">abstraction layer</a>.</p>
<p><b>TA_Shutdown</b> releases the resources acquired by TA_Initialize. Call it single-threaded, typically from the last remaining thread just before your application exits.</p>

### 3.2 Batch Processing {#direct_call}

<p>Every function follows the same simple pattern: it reads its inputs from arrays you pass in and writes its results to buffers you allocate.</p>
<p>A function never writes more elements than you request, so the buffers only need to cover the startIdx-to-endIdx range.</p>
<p>As an example, let's walk through TA_MA, a function to calculate a moving average.</p>
<pre>TA_RetCode TA_MA( <span class="ta-arg-range">int          startIdx,</span>
                  <span class="ta-arg-range">int          endIdx,</span>
                  <span class="ta-arg-in">const double inReal[],</span>
                  <span class="ta-arg-opt">int          optInTimePeriod,</span>
                  <span class="ta-arg-opt">int          optInMAType,</span>
                  <span class="ta-arg-out">int         *outBegIdx,</span>
                  <span class="ta-arg-out">int         *outNBElement,</span>
                  <span class="ta-arg-out">double       outReal[]</span>   )
</pre>

<p>All TA functions use the same calling pattern, divided into four groups:</p>
<ul>
<li>
<span class="ta-arg-range">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input arrays.</span></li>
<li>
<span class="ta-arg-in">One or more input arrays are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with &quot;in&quot;.</span>
</li>
<li><span class="ta-arg-opt">Zero or more optional inputs are then specified. In this example there are two optional inputs. These parameters give finer control specific to each function. If you do not care about a particular optIn, just specify TA_INTEGER_DEFAULT or TA_REAL_DEFAULT (depending on the type). For a moving-average type, use TA_MAType_DEFAULT.</span>
</li>
<li>
<span class="ta-arg-out">One or more output arrays come last. In this example there is only one output (outReal). The parameters outBegIdx and outNBElement always come just before the output arrays.</span>
</li>
</ul>
<p>This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.
</p>
<p>For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:</p>

<pre>TA_Real    closePrice[400];
TA_Real    out[400];
TA_Integer outBeg;
TA_Integer outNBElement;

/* ... initialize your closing price here... */

retCode = TA_MA( <span class="ta-arg-range">0</span>, <span class="ta-arg-range">399</span>,
                 <span class="ta-arg-in">&amp;closePrice[0]</span>,
                 <span class="ta-arg-opt">30</span>, <span class="ta-arg-opt">TA_MAType_SMA</span>,
                 <span class="ta-arg-out">&amp;outBeg</span>, <span class="ta-arg-out">&amp;outNBElement</span>, <span class="ta-arg-out">&amp;out[0]</span> );

/* The output is displayed here */
for( i=0; i &lt; outNBElement; i++ )
   printf( &quot;Day %d = %f\n&quot;, outBeg+i, out[i] );
</pre>

<p>After the call, it is important to check the values returned in outBeg and outNBElement. Even though we requested the whole range (0 to 399), a 30-day average is not defined until the 30th day. Consequently, outBeg will be 29 (zero-based) and
outNBElement will be 400-29 = 371. In other words, only the first 371 elements of out[] are written, and they correspond to input elements 29 through 399.</p>
<p>As another example, if you had requested only the range 125 to 225, outBeg
would be 125 and outNBElement would be 101 (endIdx is inclusive: 225-125+1).
The 30-day minimum is not a problem here, because the 125 closing prices before
the requested range provide the needed history. As you may have guessed, only
the first 101 elements of out[] are written; the rest is left untouched.</p>
<p>Here is another example. This time we calculate a 14-bar exponential moving average
for a single price bar (say, the last of 300 bars):</p>

<pre>TA_Real    closePrice[300];
TA_Real    out;
TA_Integer outBeg;
TA_Integer outNBElement;

/* ... initialize your closing price here... */

retCode = TA_MA( <span class="ta-arg-range">299</span>, <span class="ta-arg-range">299</span>,
                 <span class="ta-arg-in">&amp;closePrice[0]</span>,
                 <span class="ta-arg-opt">14</span>, <span class="ta-arg-opt">TA_MAType_EMA</span>,
                 <span class="ta-arg-out">&amp;outBeg</span>, <span class="ta-arg-out">&amp;outNBElement</span>, <span class="ta-arg-out">&amp;out</span> );</pre>

<p>In this example, outBeg will be 299, outNBElement will be 1, and only one value is written into out.</p>
<p>If you do not provide enough data to calculate even one value, outNBElement will be 0 and outBeg should be ignored.</p>
<p>If the input and output of a TA function are of the same type, the caller can reuse the input buffer to store <b>one of the outputs</b>. The following example works:</p>
<pre>#define BUFFER_SIZE 100
TA_Real buffer[BUFFER_SIZE];
...
retCode = TA_MA( <span class="ta-arg-range">0</span>, <span class="ta-arg-range">BUFFER_SIZE-1</span>,
                 <span class="ta-arg-in">&amp;buffer[0]</span>,
                 <span class="ta-arg-opt">30</span>, <span class="ta-arg-opt">TA_MAType_SMA</span>,
                 <span class="ta-arg-out">&amp;outBeg</span>, <span class="ta-arg-out">&amp;outNBElement</span>, <span class="ta-arg-out">&amp;buffer[0]</span> );</pre>
<p>Of course, the input is overwritten, but this avoids allocating a temporary buffer. All TA functions support this.</p>

### 3.3 Output Size and Lookback {#output_size}

<p>
It is important that the output array is large enough. Here are three ways to determine the allocation size; all of them work for every TA function:</p>

| Method           | Description                                                                                                                                                                                                 |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Input Matching   | allocationSize = endIdx + 1; <br> **Pros**: Easy to understand and implement. <br> **Cons**: Memory allocation unnecessarily large when requesting a small range.                                              |
| Range Matching   | allocationSize = endIdx - startIdx + 1; <br> **Pros**: Easy to implement. <br> **Cons**: Allocation slightly larger than needed. Example: with startIdx = 0, a 30-period SMA wastes 29 elements because of the lookback. |
| Exact Allocation | lookback = TA_XXXX_Lookback( ... ) ; <br> temp = max( lookback, startIdx ); <br> if( temp > endIdx ) <br> &nbsp;&nbsp; allocationSize = 0; // No output <br> else <br> &nbsp;&nbsp; allocationSize = endIdx - temp + 1; <br> **Pros**: Allocates exactly what is needed. <br> **Cons**: Slightly more complex. |

<p>Each TA function has a matching TA_XXXX_Lookback function. Example: for TA_SMA,
it is TA_SMA_Lookback.</p>
<p>The lookback is the number of input elements consumed before the first output can be calculated. Example: a simple moving average (SMA) of period 10 has a lookback of 9.</p>

### 3.4 Return Codes {#retcode}

<p>Every TA function returns a <b>TA_RetCode</b>. <b>TA_SUCCESS</b> (zero) means the call completed and wrote its outputs; on anything else, treat outBegIdx and outNBElement as undefined and the output buffers as untouched.</p>
<p>The codes a caller normally encounters:</p>

| Code | Meaning |
|------|---------|
| `TA_SUCCESS` | No error. |
| `TA_LIB_NOT_INITIALIZE` | [TA_Initialize](#init) was not called, or did not succeed. |
| `TA_BAD_PARAM` | A parameter is out of range, or a required pointer is NULL. |
| `TA_ALLOC_ERR` | Allocation failed, most likely out of memory. |
| `TA_OUT_OF_RANGE_START_INDEX` | startIdx is negative or above [TA_MAX_INDEX](#index_range). |
| `TA_OUT_OF_RANGE_END_INDEX` | endIdx is negative, above [TA_MAX_INDEX](#index_range), or below startIdx. |

<p>The full list is the TA_RetCode enumeration in <a href="https://github.com/TA-Lib/ta-lib/blob/main/include/ta_defs.h">ta_defs.h</a>. Rather than mapping the codes yourself, <b>TA_SetRetCodeInfo</b> turns any of them - including one this version of the library does not know - into a printable name and description:</p>

```c
TA_RetCodeInfo info;

if( retCode != TA_SUCCESS )
{
   TA_SetRetCodeInfo( retCode, &info );
   printf( "Error %d(%s): %s\n", retCode, info.enumStr, info.infoStr );
}
```

<p>which prints, for example:</p>

```
Error 1(TA_LIB_NOT_INITIALIZE): TA_Initialize was not successfully called
```

<p>The <a href="#output_size">TA_XXXX_Lookback</a> functions are the exception to the pattern: they return an int rather than a TA_RetCode, and answer <b>-1</b> when a parameter is out of range. Check for that before using the value as an allocation size.</p>

### 3.5 Index Range {#index_range}

<p><b>TA_MAX_INDEX</b> is the largest value startIdx or endIdx may take: <b>100,000,000</b>. A call outside the range is rejected rather than computed:</p>

| Condition | Return code |
|-----------|-------------|
| `startIdx < 0` or `startIdx > TA_MAX_INDEX` | `TA_OUT_OF_RANGE_START_INDEX` |
| `endIdx < 0`, `endIdx > TA_MAX_INDEX`, or `endIdx < startIdx` | `TA_OUT_OF_RANGE_END_INDEX` |

<p>The limit is the same number in every language binding — <code>TA_MAX_INDEX</code> in C, <code>Core::MAX_INDEX</code> in Rust, <code>Core.MAX_INDEX</code> in Java and C# — so a call is accepted or rejected identically whichever you use. It is a constant rather than a buffer length, so the check costs two comparisons and is done before anything is read.</p>

<p>For context on the size: 100 million one-minute bars is about 190 years of 24/7 data, or a century of a regular equity session. Series that long are usually tick data, where the <a href="/api/stream/">streaming API</a> is the better tool anyway.</p>

<p><b>This bounds the API domain and nothing else.</b> In particular it is not a promise about accuracy. A handful of functions accumulate rounding error as the series grows, and the worst of them have lost several digits well before this limit — WMA, HMA, CORREL and the LINEARREG family are the ones to know about. No single index cap can express that, because the error depends on the data and on the period, not on the index alone. It is also unrelated to the <a href="#numerical_stability">numerical-stability categories</a>, which answer a different question: whether a value converges as history grows, not how rounding accumulates within it.</p>

<p>Raising this limit later would only admit calls that are rejected today, so it is safe to treat 100,000,000 as a floor rather than a fixed contract.</p>

## 4.0 Advanced Features {#advanced}

### 4.1 Abstraction Layer {#abstract}

<p>Instead of hard-coding calls to specific TA functions, an app can drive them all dynamically through the interface in <a href="https://github.com/TA-Lib/ta-lib/blob/main/include/ta_abstract.h">ta_abstract.h</a> — looking functions up by name at runtime. For any function it reports the inputs it takes, its optional parameters with their valid ranges, and the outputs it produces — so you can call a function whose signature was unknown at compile time.</p>
<p>This is what you want when the function or its parameters are not fixed in your code. Typical uses:</p>
<ul>
  <li>Generating glue code or wrappers for higher-level languages.</li>
  <li>Automatically picking up new functions after a TA-Lib upgrade, with no code change.</li>
  <li>"Mutating" the function and its parameters while searching for strategies (e.g. a genetic or neural-network algorithm).</li>
  <li>Populating a charting app: the indicator menu and each settings dialog come straight from the metadata.</li>
</ul>
<p>If you only need a handful of specific functions, calling them directly — with <a href="#direct_call">batch processing</a> or the <a href="/api/stream/">streaming API</a> — is simpler.</p>

### 4.2 Numerical Stability {#numerical_stability}

<a id="unstable_period"></a>
<p>Take one bar and compute an indicator for it twice: once with a year of history before it, once with a decade. Do you get the same value? For many functions, always — they read a fixed number of bars and ignore everything older. Others are recursive, so their earliest values depend on how much history precedes them, converging as more bars are supplied — the Exponential Moving Average is the classic example. A few accumulate from the very first bar and never converge at all.</p>
<p>Each function's documentation specifies which of the four <a href="/functions/stability.html">numerical-stability categories</a> applies to it.</p>
<p>This is about convergence, not rounding. A function can be perfectly convergent and still accumulate floating-point error over a very long series — a separate axis, noted under <a href="#index_range">Index Range</a>.</p>
<p>See the <a href="/api/unstable-period/">Unstable Period</a> page for how to configure this.</p>

### 4.3 Candlestick Settings {#candle_settings}

<p>The candlestick pattern functions (<b>TA_CDL*</b>) judge each candle against tunable thresholds — is its body "long", its shadow "short", two candles "near". These thresholds are global settings: change them once, from a single thread, before any concurrent calls (see <a href="#multithreading">multi-threading</a>).</p>
<p>See the <a href="/api/candle-settings/">Candlestick Settings</a> page for the API, the setting types and their defaults.</p>

### 4.4 Input Type: float vs. double {#input_type}

<p>Each TA function has two implementations: one accepts input arrays of double, the other of float. The float version carries the &quot;TA_S_&quot; prefix, e.g. TA_S_MA is the float equivalent of TA_MA.</p>
<pre>TA_RetCode TA_MA( int          startIdx,
                  int          endIdx,
                  <b>const double inReal[]</b>,
                  int          optInTimePeriod,
                  TA_MAType    optInMAType,
                  int         *outBegIdx,
                  int         *outNBElement,
                  double       outReal[] );
</pre>
<pre>TA_RetCode TA_S_MA( int          startIdx,
                    int          endIdx,
                    <b>const float  inReal[]</b>,
                    int          optInTimePeriod,
                    TA_MAType    optInMAType,
                    int         *outBegIdx,
                    int         *outNBElement,
                    double       outReal[] );
</pre>

<p>Internally, both versions do all calculations in double &mdash; each float element is converted to double when read. Consequently, both functions produce the same output, bit-for-bit.</p>
<p>Some apps already hold their price data as float. The TA_S_XXXX functions consume such arrays directly (no conversion copy needed) while keeping every intermediate calculation in double.
</p>

### 4.5 High-performance Multi-threading {#multithreading}

<p>TA-Lib is multi-thread safe where it matters most for performance: calling the TA functions themselves (TA_SMA, TA_RSI, ...).</p>

<p>One important caveat: the "global settings" must first be initialized from a single thread. That includes calls to:</p>
<ul>
  <li><a href="#init">TA_Initialize</a></li>
  <li><a href="/api/unstable-period/">TA_SetUnstablePeriod</a></li>
  <li><a href="/api/candle-settings/">TA_SetCandleSettings, TA_RestoreCandleDefaultSettings</a></li>
</ul>

<p>Once these initial calls are done, the application can call the rest of the API from multiple threads (including the ta_abstract.h interface).</p>

<p>The exception at the other end is <a href="#init">TA_Shutdown</a>, which is single-threaded as well.</p>

<p>Note: TA-Lib assumes it is linked against a thread-safe malloc/free runtime, which is the default on all modern platforms (Linux, Windows, Mac). In other words, any toolchain supporting C11 or newer is safe.</p>
