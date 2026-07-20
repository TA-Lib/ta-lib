---
title: C/C++ Core API
toc: false
---

# C/C++ Core API Documentation #
<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 How to add TA-Lib to your app</a></p>

<p><a href="#ta_func">3.0 Calling into TA-Lib</a></p>

<blockquote>
<p><a href="#init">3.1 Initialize and Shutdown</a><br>
<a href="#direct_call">3.2 Batch Processing</a><br>
<a href="#output_size">3.3 Output Size</a><br></p>
</blockquote>

<p><a href="#advanced">4.0 Advanced Features</a></p>
<blockquote>
<p><a href="#abstract">4.1 Abstraction layer</a><br>
<a href="#unstable_period">4.2 Unstable Period</a><br>
<a href="#input_type">4.3 Input Type: float vs. double</a><br>
<a href="#multithreading">4.4 High-performance Multi-threading</a></p>
</blockquote>

<h2 id="intro">1.0 Introduction</h2>

<p>The <b>Core API</b> provides:</p>
<ul>
  <li>Lifecycle of the library (<a href="#init">TA_Initialize / TA_Shutdown</a>).</li>
  <li>Setting global variables (e.g. <a href="/api/unstable-period/">TA_SetUnstablePeriod</a>, <a href="/api/candle-settings/">TA_SetCandleSettings</a>).</li>
  <li>Each <a href="#ta_func">TA function</a> for processing a whole array of data at once.</li>
  <li>An <a href="#abstract">abstraction layer</a> for calling these functions dynamically.</li>
</ul>
<p>To process a live feed one bar at a time instead, see the companion <a href="/api/stream/">C/C++ Streaming API</a>.</p>
<p>You must first <a href="/install/">install TA-Lib</a>, which will provide all the shared/static libraries and headers needed to compile with your program.</p>

<h2 id="build">2.0 How to add TA-Lib to your app</h2>

In your source code, add <b>#include &quot;ta_libc.h&quot;</b> and link to the library named "ta-lib".

You may need to add TA-Lib to your compiler's search path. For example, with gcc, you can use the following options:

```sh
-I/usr/local/include/ta-lib -lta-lib
```

The paths depend on the method used to install. Typical locations for headers are:

    - /usr/local/include/ta-lib
    - /usr/include/ta-lib
    - /opt/include/ta-lib

Typical locations for the libraries are:

    - /usr/lib
    - /usr/lib64
    - /usr/local/lib
    - /usr/local/lib64
    - /opt/lib
    - /opt/local/lib

For [homebrew](https://formulae.brew.sh/formula/ta-lib), use <b>brew --prefix ta-lib</b> to find the paths.

For windows, look into <b>C:\Program Files\TA-Lib</b> for 64-bits and <b>C:\Program Files (x86)\TA-Lib</b> for 32-bits.


<h2 id="ta_func">3.0 Calling into TA-Lib</h2>

<p>All of TA-Lib's public functions are declared in <a href="https://github.com/TA-Lib/ta-lib/blob/main/include">the include/*.h headers</a>.</p>

<h3 id="init">3.1 Initialize and Shutdown</h3>
<pre>TA_RetCode TA_Initialize( void );
TA_RetCode TA_Shutdown( void );</pre>
<p><b>TA_Initialize</b> must be called once (and only once), from a single thread, prior to any other API function. After it returns TA_SUCCESS, you can start processing your data in 3 different ways: <a href="#direct_call">batch processing</a>, the <a href="/api/stream/">streaming API</a> or through the <a href="#abstract">abstraction layer.</a></p>
<p><b>TA_Shutdown</b> releases the resources acquired by TA_Initialize. Call it single-threaded, typically from the last remaining thread just before your application exits.</p>

<h3 id="direct_call">3.2 Batch Processing</h3>
Every function follows the same simple pattern: it reads its inputs from arrays you pass in and writes its results to buffers you allocate.<br/>
<br/>
A function never writes more elements than you request, so the buffers only need to cover the startIdx-to-endIdx range.<br/>

As an example, let's walk through TA_MA, a function to calculate a moving average.
<pre>TA_RetCode TA_MA(&nbsp;<span style="background-color: #66FFFF; color: #000">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; startIdx,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #66FFFF; color: #000">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; endIdx,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #00FF00; color: #000">const double&nbsp;inReal[],</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #C0C0C0; color: #000">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;optInTimePeriod,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #C0C0C0; color: #000">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;optInMAType,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #FFFF00; color: #000">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*outBegIdx,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #FFFF00; color: #000">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp;*outNbElement,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #FFFF00; color: #000">double&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; outReal[]</span>&nbsp;<span lang="en-us">&nbsp;&nbsp;</span>)
</pre>

All TA functions use the same calling pattern, divided into 4 groups:
<ul>
<li>
<span style="background-color: #66FFFF; color: #000">The output will be calculated only for the range specified by startIdx and endIdx. These are zero-based indices into the input arrays.</span></li>
<li>
<span style="background-color: #00FF00; color: #000">One or more input arrays are then specified. Typically, these are the "price" data. In this example there is only one input. All input parameter names start with &quot;in&quot;.</span>
</li>
<li><span style="background-color: #C0C0C0; color: #000">Zero or more optional inputs are then specified. In this example there are two optional inputs. These parameters give finer control specific to each function. If you do not care about a particular optIn, just specify TA_INTEGER_DEFAULT or TA_REAL_DEFAULT (depending on the type).</span>
</li>
<li>
<span style="background-color: #FFFF00; color: #000">One or more output arrays come last. In this example there is only one output (outReal). The parameters outBegIdx and outNbElement always come just before the output arrays.</span>
</li>
</ul>
<p>This calling pattern takes some getting used to, but it lets your app spend time and memory only on the data it actually needs.
</p>
<p>For example, here is how to calculate a 30-day simple moving average (SMA) of daily closing prices:</p>
<div align="justify">
<pre>TA_Real    closePrice[400];
TA_Real    out[400];
TA_Integer outBeg;
TA_Integer outNbElement;</pre>
</div>
<div align="justify">
  <pre>/* ... initialize your closing price here... */</pre>
</div>
<div align="justify">
  <pre>retCode = TA_MA( <span style="background-color: #00FFFF; color: #000">0</span>, <span style="background-color: #00FFFF; color: #000">399</span>,
                 <span style="background-color: #00FF00; color: #000">&amp;closePrice[0]</span>,
                 <span style="background-color: #C0C0C0; color: #000">30</span>,<span style="background-color: #C0C0C0; color: #000">TA_MAType_SMA</span>,
                 <span style="background-color: #FFFF00; color: #000">&amp;outBeg</span>, <span style="background-color: #FFFF00; color: #000">&amp;outNbElement</span>, <span style="background-color: #FFFF00; color: #000">&amp;out[0]</span> );</pre>
</div>
<div align="justify">
  <pre>/* The output is displayed here */
for( i=0; i &lt; outNbElement; i++ )
   printf( &quot;Day %d = %f\n&quot;, outBeg+i, out[i] );
</pre>
</div>
<p>After the call, it is important to check the values returned in outBeg and outNbElement. Even though we requested the whole range (0 to 399), a 30-day average is not defined until the 30th day. Consequently, outBeg will be 29 (zero-based) and
outNbElement will be 400-29 = 371. In other words, only the first 371 elements of out[] are written, and they correspond to input elements 29 through 399.</p>
<p>As another example, if you had requested only the range 125 to 225, outBeg
would be 125 and outNbElement would be 101 (endIdx is inclusive: 225-125+1).
The 30-day minimum is not a problem here, because the 125 closing prices before
the requested range provide the needed history. As you may have guessed, only
the first 101 elements of out[] are written; the rest is left untouched.</p>
<p>Here is another example. This time we calculate a 14-bar exponential moving average
for a single price bar (say, the last of 300 bars):</p>
<div align="justify">
<pre>TA_Real    closePrice[300];
TA_Real    out;
TA_Integer outBeg;
TA_Integer outNbElement;</pre>
</div>
<div align="justify">
  <pre>/* ... initialize your closing price here... */</pre>
</div>
<div align="justify">
  <pre>retCode = TA_MA( <span style="background-color: #00FFFF; color: #000">299</span>, <span style="background-color: #66FFFF; color: #000">299</span>,
                 <span style="background-color: #00FF00; color: #000">&amp;closePrice[0]</span>,
                 <span style="background-color: #C0C0C0; color: #000" lang="en-us">14</span>, <span style="background-color: #C0C0C0; color: #000">TA_MAType_EMA</span>,
                 <span style="background-color: #FFFF00; color: #000">&amp;outBeg</span>, <span style="background-color: #FFFF00; color: #000">&amp;outNbElement</span>, <span style="background-color: #FFFF00; color: #000">&amp;out</span> );</pre>
</div>
<p>In this example, outBeg will be 299, outNbElement will be 1, and only one value is written into out.</p>
<p>If you do not provide enough data to calculate even one value, outNbElement will be 0 and outBeg should be ignored.</p>
<p>If the input and output of a TA function are of the same type, the caller can reuse the input buffer to store <u>one of the outputs</u>. The following example works:</p>
<div align="justify">
<pre>#define BUFFER_SIZE 100
TA_Real buffer[BUFFER_SIZE];
...
retCode = TA_MA( <span style="background-color: #00FFFF; color: #000" lang="en-us">0</span>, <span style="background-color: #00FFFF; color: #000" lang="en-us">BUFFER_SIZE-1</span>,
                 <span style="background-color: #00FF00; color: #000">&amp;buffer[0]</span>,
                 <span style="background-color: #C0C0C0; color: #000">30</span>, <span style="background-color: #C0C0C0; color: #000">TA_MAType_SMA</span>,
                 <span style="background-color: #FFFF00; color: #000">&amp;outBeg</span>, <span style="background-color: #FFFF00; color: #000">&amp;outNbElement</span>, <span style="background-color: #FFFF00; color: #000">&amp;buffer[0]</span> );</pre>
</div>
<p>Of course, the input is overwritten, but this avoids allocating a temporary buffer. All TA functions support this.</p>
<h3 id="output_size" align="justify">3.3 Output Size</h3>
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

<h2 id="advanced">4.0 Advanced Features</h2>

<h3 id="abstract">4.1 Abstraction Layer</h3>
<p>Instead of hard-coding calls to specific TA functions, an app can drive them all dynamically through the interface in <a href="https://github.com/TA-Lib/ta-lib/blob/main/include/ta_abstract.h">ta_abstract.h</a> — looking functions up by name at runtime. It reports which inputs, optional parameters (and their valid ranges), and outputs a function takes — so you can call a function whose signature was unknown at compile time.</p>
<p>This is what you want when the function or its parameters are not fixed in your code. Typical uses:</p>
<ul>
  <li>Generating glue code or wrappers for higher-level languages.</li>
  <li>Automatically picking up new functions after a TA-Lib upgrade, with no code change.</li>
  <li>"Mutating" the function and its parameters while searching for strategies (e.g. a genetic or neural-network algorithm).</li>
  <li>Populating a charting app: the indicator menu and each settings dialog come straight from the metadata.</li>
</ul>
<p>If you only need a handful of specific functions, calling them directly — with <a href="#direct_call">batch processing</a> or the <a href="/api/stream/">streaming API</a> — is simpler.</p>

<h3 id="unstable_period">4.2 Unstable Period</h3>
<p>For some recursive TA functions, such as the Exponential Moving Average, the first outputs depend on where the input data begins — an effect that decays until the output becomes stable. The unstable period setting
(<a href="/api/unstable-period/#api">TA_SetUnstablePeriod</a>,
<a href="/api/unstable-period/#api">TA_GetUnstablePeriod</a>) controls how many of those early values are discarded. See the <a href="/api/unstable-period/">Unstable Period</a> page for details and the list of affected functions.</p>
<h3 id="input_type">4.3 Input Type: float vs. double</h3>
<p>Each TA function has two implementations: one accepts input arrays of double, the other of float. The float version carries the &quot;TA_S_&quot; prefix, e.g. TA_S_MA is the float equivalent of TA_MA.</p>
<pre>TA_RetCode TA_MA( int          startIdx,
                  int          endIdx,
                  <b>const double inReal[]</b>,
                  int          optInTimePeriod,
                  TA_MAType    optInMAType,
                  int         *outBegIdx,
                  int         *outNbElement,
                  double       outReal[] );
</pre>
<pre>TA_RetCode TA_S_MA( int          startIdx,
                    int          endIdx,
                    <b>const float  inReal[]</b>,
                    int          optInTimePeriod,
                    TA_MAType    optInMAType,
                    int         *outBegIdx,
                    int         *outNbElement,
                    double       outReal[] );
</pre>

<p>Internally, both versions do all calculations in double &mdash; each float element is converted to double when read. Consequently, both functions produce the same output, bit-for-bit.</p>
<p>Some apps already hold their price data as float. The TA_S_XXXX functions consume such arrays directly (no conversion copy needed) while keeping every intermediate calculation in double.
</p>

<h3 id="multithreading">4.4 High-performance multi-threading</h3>

<p>TA-Lib is multi-thread safe where it matters most for performance: calling the TA functions themselves (TA_SMA, TA_RSI, ...).</p>

<p>One important caveat: the "global settings" must first be initialized from a single thread. That includes calls to:</p>
<ul>
  <li><a href="#init">TA_Initialize</a></li>
  <li><a href="/api/unstable-period/">TA_SetUnstablePeriod</a></li>
  <li><a href="/api/candle-settings/">TA_SetCandleSettings, TA_RestoreCandleDefaultSettings</a></li>
</ul>

<p>Once these initial calls are done, the application can call the rest of the API from multiple threads (including the ta_abstract.h interface).</p>

<p>The exception is <a href="#init">TA_Shutdown()</a>, which must be called single-threaded (typically from the last remaining thread just before your application exits).</p>

<p>Note: TA-Lib assumes it is linked against a thread-safe malloc/free runtime, which is the default on all modern platforms (Linux, Windows, Mac). In other words, any toolchain supporting C11 or newer is safe.</p>
