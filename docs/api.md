---
hide:
    - toc
---

# C/C++ API Documentation #
<p><a href="#intro">1.0 Introduction</a></p>

<p><a href="#build">2.0 How
to build and link to TA-Lib</a></p>

<p><a href="#ta_func">3.0 Technical Analysis Functions</a></p>

<blockquote>
<p><a href="#direct_call">3.1 Direct call to a TA Function</a><br>
<a href="#Output output_size">3.2 Output Size</a><br>
</blockquote>

<p><a href="#Advanced">4.0 Advanced Features</a></p>
<blockquote>
<a href="#Abstraction">4.1 Abstraction layer</a><br>
<a href="#Unstable Period">4.2 Unstable Period</a><br>
<a href="#Input Type">4.3 Input Type: float vs. double</a><br>
<a href="#Multithreading">4.4 High-performance Multi-threading</a>
</blockquote>

<h2><a name="intro">1.0 Introduction</a></h2>

<p>All public functions of the libraries are documented here.</p>
<p>You must first <a href="http://ta-lib.org/install">install TA-Lib</a>, which will provide all the shared/static libraries and headers needed to compile with your program.</p>

<h2><a name="build">2.0 How to build and link to TA-Lib</a></h2>

In your source code, add <b>#include &quot;ta_libc.h&quot;</b> and link to the library named "ta-lib".<br/><br/>
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


<h2><a name="ta_func">3.0 Technical Analysis Functions</a></h2>

Make sure <a href="ta_initialize.html">TA_Initialize</a> was called once (and only once) prior to any other API functions.<br/>
<br/>
All TA functions can be <a href="#direct_call">directly called</a>.<br/>
<br/>
Alternatively, an app can call all TA functions using the <a href="#abstract">abstraction layer</a>. This is useful to make dynamic call without knowing at priori the function name or parameters. Example of use is to automatically integrate new functions after upgrading TA-Lib, or for "mutating" calls while strategy searching (e.g. genetic/neural network algo).<br/>

<h3><a name="direct_call">3.1 Direct call to a TA Function</a></h3>
Direct calls could be done through the API defined in
<a href="https://github.com/TA-Lib/ta-lib/blob/main/include/ta_func.h">include/ta_func.h</a><br/>
<br/>
All functions are simple "array processing" functions. You provides the inputs with an array, and the the output is written in a caller provided output array.<br/>
<br/>
The number of data written will NEVER exceed the number of elements requested to be calculated (with the startIdx and endIdx explained below).<br/>

Here is an example:
We will dissect the TA_MA function allowing to calculate a simple moving average.
<pre>TA_RetCode TA_MA(&nbsp;<span style="background-color: #66FFFF">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; startIdx,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #66FFFF">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; endIdx,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #00FF00">const double&nbsp;inReal[],</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #C0C0C0">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;optInTimePeriod,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #C0C0C0">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;optInMAType,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #FFFF00">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;*outBegIdx,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #FFFF00">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp;*outNbElement,</span>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<span style="background-color: #FFFF00">double&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; outReal[]</span>&nbsp;<span lang="en-us">&nbsp;&nbsp;</span>)
</pre>

All TA functions use the same calling pattern, divided in 4 groups:
	<ul>
		<li>
    <span style="background-color: #66FFFF">The output will be calculated only for the range specified by startIdx to endIdx. These are zero base index into the input arrays.</span></li>
		<li>
    <span style="background-color: #00FF00">One or more input arrays are then specified. Typically, these are going to be the "price" data. In that example there is only one input. All inputs parameter name starts with &quot;in&quot;.</span>
    </li>
		<li><span style="background-color: #C0C0C0">Zero or more optional inputs may need to be specified. In that example there are 2 optional inputs. These parameters allow more control specific to the function. If you do not care about a particular optIn just specify TA_INTEGER_DEFAULT or TA_REAL_DEFAULT (depending on the type).</span>
    </li>
		<li>
    <span style="background-color: #FFFF00">One or more output arrays are finally specified. In that example there is only one output (outReal). The params outBegIdx and outNbElement are always specified once before the output arrays.</span>
    </li>
</ul>
<p>This calling pattern provides control on calculating ONLY the portion of data needed for your app. It is slightly complex, but it enables speed/memory optimizations.
</p>
<p>Example calculating a 30 days simple moving average (SMA) of daily closing prices:</p>
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
  <pre>retCode = TA_MA( <span style="background-color: #00FFFF">0</span>, <span style="background-color: #00FFFF">399</span>,
                 <span style="background-color: #00FF00">&amp;closePrice[0]</span>,
                 <span style="background-color: #C0C0C0">30</span>,<span style="background-color: #C0C0C0">TA_MAType_SMA</span>,
                 <span style="background-color: #FFFF00">&amp;outBeg</span>, <span style="background-color: #FFFF00">&amp;outNbElement</span>, <span style="background-color: #FFFF00">&amp;out[0]</span> );</pre>
</div>
<div align="justify">
  <pre>/* The output is displayed here */
for( i=0; i &lt; outNbElement; i++ )
   printf( &quot;Day %d = %f\n&quot;, outBeg+i, out[i] );
</pre>
</div>
<p>After the call, it is important to check the value returned by outBeg and outNbElement. Even if it was requested to calculate for the whole range (from 0 to 399), the moving average is not valid until the 30th day. Consequently, outBeg will be 29 (zero base)&nbsp; and
outNbElement will be 400-29 = 371. In other words, only the first 371 elements of out[] are valid, and these could be calculated only
starting at the 30th element of the input array.</p>
<p>As an alternative example, if you would have requested to calculate only in
the &quot;125 to 225&quot; range (with startIdx and endIdx), the outBeg will be
125 and outNbElement will be 100. (the &quot;30&quot; minimum required is not an
issue because we dispose of 125 closing price before the start of the requested
range...). As you may have already understand, the &quot;out&quot; array will be
written only for its first 100 elements. The rest will be left untouched.</p>
<p>Here is another example. In that case we want to calculate a 14 bars exponential moving average
only for 1 price bar in particular (say the last day of 300 price bar):&nbsp;</p>
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
  <pre>retCode = TA_MA( <span style="background-color: #00FFFF">299</span>, <span style="background-color: #66FFFF">299</span>,
                 <span style="background-color: #00FF00">&amp;closePrice[0]</span>,
                 <span style="background-color: #C0C0C0" lang="en-us">14</span>, <span style="background-color: #C0C0C0">TA_MAType_EMA</span>,
                 <span style="background-color: #FFFF00">&amp;outBeg</span>, <span style="background-color: #FFFF00">&amp;outNbElement</span>, <span style="background-color: #FFFF00">&amp;out</span> );</pre>
</div>
<p>In that example: outBeg will be 299,&nbsp; outNbElement will be 1, and only one value gets written into out.</p>
<p>In the case that you do not provide enough data to even being able to calculate at least one value, outNbElement will be 0 and outBeg shall&nbsp; be ignored.</p>
<p>If the input and output of a TA function are of the same type, the caller can re-use the input buffer for storing <u>one of the output</u> of the TA function. The following example will work:</p>
<div align="justify">
<pre>#define BUFFER_SIZE 100
TA_Real buffer[BUFFER_SIZE];
...
retCode = TA_MA( <span style="background-color: #00FFFF" lang="en-us">0</span>, <span style="background-color: #00FFFF" lang="en-us">BUFFER_SIZE-1</span>,
                 <span style="background-color: #00FF00">&amp;buffer[0]</span>,
                 <span style="background-color: #C0C0C0">30</span>, <span style="background-color: #C0C0C0">TA_MAType_SMA</span>,
                 <span style="background-color: #FFFF00">&amp;outBeg</span>, <span style="background-color: #FFFF00">&amp;outNbElement</span>, <span style="background-color: #FFFF00">&amp;buffer[0]</span> );</pre>
</div>
<p>Of course, the input is overwritten, but this capability diminish needs for temporary memory buffers. This capability is true for all TA functions.</p>
<h3 align="justify"><a name="output_size">3.2 Output Size</a></h3>
<p>
It is important that the output array is large enough. Depending of your needs, you might find one of the following method useful to determine the output allocation size. All these methods works consistently for all TA functions:</p>
| Method           | Description                                                                                                                                                                                                 |
|------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Input Matching   | allocationSize = endIdx + 1; <br> **Pros**: Easy to understand and implement. <br> **Cons**: Memory allocation unnecessarily large when specifying small range.                                              |
| Range Matching   | allocationSize = endIdx - startIdx + 1; <br> <br> **Pros**: Easy to implement. <br> **Cons**: Allocation slightly larger than needed. Example: for a 30 period SMA, you will get 29 elements wasted because of the lookback. |
| Exact Allocation | lookback = TA_XXXX_Lookback( ... ) ; <br> temp = max( lookback, startIdx ); <br> if( temp > endIdx ) <br> &nbsp;&nbsp; allocationSize = 0; // No output <br> else <br> &nbsp;&nbsp; allocationSize = endIdx - temp + 1; <br> **Pros**: Optimal allocation algorithm. <br> **Cons**: Slightly more complex. |


<p>A function TA_XXXX_Lookback is provided for each TA function. Example: For TA_SMA,
there is a TA_SMA_Lookback.</p>
<p>The lookback indicates how many inputs are consume before the first output can be calculated. Example: A simple moving average (SMA) of period 10 will have a lookback of 9.</p>

<h2><a name="Advanced">4.0 Advanced Features</a></h2>

<h3><a name="abstract">4.1 Abstraction Layer</a></h3>
<p>All the TA Function can be called using the interface defined in
<a href="https://github.com/TA-Lib/ta-lib/blob/main/include/ta_abstract.h">ta_abstract.h</a></p>
<p>The abstraction layer is particularly useful for an application who wishes to support the complete list of TA functions without having to maintain new code each time a new function is added to TA-Lib. If you wish to simply integrate in your application a small number of specific functions, then you may be better to do simpler direct call (see previous section).<br/>
<br/>
Use Case 1:<br/>
The app is a charting software. When the user select a price bar, a side list offers blindly all the TA functions that could be applied to a price bar. The user selects one of these, then a dialog open for allowing to adjust the optional parameters (TA-Lib API will tell your app which parameters are needed and the
valid range for each). Once all parameters are set, you can dynamically call the corresponding TA function. The output can then be drawn on the chart using some hint (from TA-Lib) about data representation (overlap study, independent indicator with its own scale etc...).
<br/><br/>
The same "abstract" logic apply to all the TA functions. Some TA Functions works only on volume, or can work indifferently with any time series data (the open, close, another indicator...).<br/>
<br/>

Use Case 2:<br/>
Your app is searching for "best strategies" using your own backtesting platform. You might want to automate trying various mix of "volatility" and moving average crossings using a genetic algorith, You can make your app "mutate" and switch function and parameters using the abstract interface.<br/>
<br/>
Use Case 3:<br/>
You can generate any "glue code" or "wrapper" for a high-level language (e.g. Python, R, Java, C#) or for simply interfacing with your app. A lot of derived work are now "maintained" automatically using the abstract layer.<br/>

<h3><a name="Unstable Period">4.2 Unstable Period</a></h3>
<p>Some TA functions provides different results depending of the &quot;starting point&quot; of the data being
involve. This is often referred as a function having memories. An example of such function is the Exponential Moving Average. It is
possible to control the unstable period (the amount of data to strip off) with
<a href="ta_setunstableperiod.html">TA_SetUnstablePeriod</a> and
<a href="ta_getunstableperiod.html">TA_GetUnstablePeriod</a>.</p>
<h3><a name="Input Type">4.3 Input Type: float vs. double</a></h3>
<p>Each TA function have two implementation. One accepts input arrays of float and the other accepts double. The float version has a &quot;TA_S_&quot; suffix e.g. for TA_MA there is an equivalent TA_S_MA function.</p>
<pre>TA_RetCode TA_MA( int     startIdx,
                  int          endIdx,
                  <b>const double inReal[]</b>,
                  int          optInTimePeriod,
                  TA_MAType    optInMAType,
                  int         *outBegIdx,
                  int         *outNbElement,
                  double       outReal[] );
</pre>
<pre>TA_RetCode TA_S_MA( int     startIdx,
                    int          endIdx,
                    <b>const float inReal[]</b>,
                    int          optInTimePeriod,
                    TA_MAType    optInMAType,
                    int         *outBegIdx,
                    int         *outNbElement,
                    double       outReal[] );
</pre>

<p>Internally both version do all the calculation using double e.g. when an element of a float array is accessed, it is changed to double. Consequently, both function will produce the same output</p>
<p>Some apps have their price bar data already loaded as float. The TA_S_XXXX functions allows to digest these directly (no copy needed) while still maintaining all intermediate calculation as double.

<h3><a name="Multithreading">4.4 High-performance multi-threading</a></h3>

<p>TA-Lib is multi-thread safe where it matters the most for performance: When calling any TA functions (e.g. TA_SMA, TA_RSI etc... )</p>

<p>One important caveat is the initialization of the "global settings" must first be done from a single thread. That includes calls to:</p>
<ul>
  <li>TA_Initialize</li>
  <li>TA_SetUnstablePeriod, TA_SetCompatibility</li>
  <li>TA_SetCandleSettings, TA_RestoreCandleDefaultSettings</li>
</ul>

<p>After you are done with these initial calls, the application can start performing multi-thread calls with the rest of the API (including the ta_abstract.h API).</p>

<p>One exception to the rule is TA_Shutdown() which must be called single threaded (typically from the only thread remaining prior to exit your application).</p>

<p>Note: TA-Lib assumes it is link to a thread safe malloc/free runtime library, which is the default on all modern platforms (Linux,Windows,Mac). In other word, safe with any compiler supporting C11 or more recent.</p>
