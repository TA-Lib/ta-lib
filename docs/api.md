<body>

<div id="content">
<h1>C/C++ API Documentation</h1>
<p><a href="#Introduction">1.0 Introduction</a></p>

<p><a href="#How to start using TA-LIB">2.0 How
to build and link to TA-Lib</a></p>

<p><a href="#Technical Analysis Functions">3.0 Technical Analysis Functions</a></p>

<blockquote>
<p><a href="#Direct call to a TA Function">3.1 Direct call to a TA Function</a><br>
<a href="#Output Size">3.2 Output Size</a><br>
</blockquote>

<p><a href="#Advanced">4.0 Advanced Features</a></p>
<blockquote>
<a href="#Abstraction">4.1 Abstraction layer</a><br>
<a href="#Unstable Period">4.2 Unstable Period</a><br>
<a href="#Input Type">4.3 Input Type: float vs. double</a><br>
<a href="#Multithreading">4.4 High-performance Multi-threading</a>
</blockquote>

</div>

<div id="contentLarge">
<h2><a name="Introduction">1.0 Introduction</a></h2>

<p>All functions available to the end-user are documented here.</p>

<h2><a name="How to start using TA-LIB">2.0 How to build and link to TA-Lib</a></h2>

<p>To use the library in C/C++ project, you just need to #include
&quot;ta_libc.h&quot; and link to the static library corresponding to your type
of application.</p>

<p>All the required header file are in
<a href="http://cvs.sourceforge.net/viewcvs.py/ta-lib/ta-lib/c/include/">ta-lib/c/include</a>. Header files in other directories
should never be included by your application directly.</p>

<h3><a name="Windows instruction">2.1 Windows - MSVC and Visual Studio</a></h3>

<p>Here is the list of variant of the static library currently supported:</p>
	<p>&nbsp;</p>
<table class="simple">
  <tr>
    <th>Static Library Name</th>
    <th>Use runtime DLL? </th>
    <th>Multithreaded?</th>
    <th>Debug info?</th>
  </tr>
  <tr>
    <td>ta_libc_csr.lib</td>
    <td>-</td>
    <td>-</td>
    <td>-</td>
  </tr>
  <tr>
    <td>ta_libc_csd.lib</td>
    <td>-</td>
    <td>-</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>ta_libc_cmr.lib</td>
    <td>-</td>
    <td>Yes</td>
    <td>-</td>
  </tr>
  <tr>
    <td>ta_libc_cmd.lib</td>
    <td>-&nbsp;</td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
  <tr>
    <td>ta_libc_cdr.lib</td>
    <td>Yes</td>
    <td>Yes</td>
    <td>-</td>
  </tr>
  <tr>
    <td>ta_libc_cdd.lib</td>
    <td>Yes</td>
    <td>Yes</td>
    <td>Yes</td>
  </tr>
</table>
    <p>&nbsp;<p>Pre-compiled
	version of these libraries is part of the MSVC package. If you wish to
	re-build yourself the static libraries, makefiles can be found in <b>ta-lib/make/<i>&lt;ENV&gt;</i>/win32/msvc.
	</b>These MSVC makefiles also works with Visual Studio 2005.<b><br>
	<br>
	</b>The <i>&lt;ENV&gt;</i><b> </b>is a 3 letter sub-directories (cmd, cmr, csd, csr, cdd
	and cdr)
	allowing to select the &quot;standard library runtime&quot; setting for your
	application).<br>
<b><br>
</b>Just type &quot;nmake&quot; or &quot;nmake /A&quot; to build all targets.
The generated targets will be found in <b>ta-lib/c/lib</b> and <b>ta-lib/c/bin</b>.

<p>To rebuild from scratch do &quot;nmake clean&quot; and then &quot;nmake&quot; again.</p>
	<ul>
		<li>
  <p style="margin-right: 100" align="justify">The application without debug info are the speed optimized version. They cannot be trace for debugging though.<br>
&nbsp;</li>
		<li>
  <p style="margin-right: 100" align="justify">Visual Studio 2005 project files can be found in <b>
  <font size="2">ta-lib/c/ide/vs2005<br>
&nbsp;</font></b></li>
		<li>
  <p style="margin-right: 100" align="justify">If you observe
  link errors on Windows, verify that the &quot;Use runtime library&quot; setting in the
	C/C++ Code generation tab is the same as your choice of static libraries.<br>
&nbsp;</li>
		<li>
		<p style="margin-right: 100" align="justify">Link errors
		will show up if your application does not links with wininet and
  odbc32. These are provided with MSVC and Borland and should be found on your
  system.</li>
</ul>

<h3 align="justify"><a name="Windows instruction">2.2 Windows/Free C++
Borland Compiler</a></h3>

	<ul>
		<li>Same as for Microsoft Visual C++, except: Makefiles are in <b>ta-lib/c/make/<i>&lt;ENV&gt;</i>/win32/borland </b>
	and the cdd and cdr static library are not available.<b><br>&nbsp;</b></li>
		<li>Execute the Borland &quot;make&quot; instead of the Microsoft &quot;nmake&quot;.<br>
&nbsp;</li>
		<li>To build from scratch, do &quot;make clean&quot;. This is needed particularly if you are switching between Borland and MSVC compiler because the object file formats are different (COFF=MSFT, OMF=Borland)</li>
</ul>

<h3><a name="Unix instruction">2.3 Other Platforms</a></h3>

<h4><a name="Unix instruction1">2.3.1 Linux Static Libraries</a></h4>

<p>The Git repository
and the Win32 packages contains multiple makefiles generated for multiple
platforms, including Linux. The makefiles are
found in <b>ta-lib/make/<i>&lt;ENV&gt;</i>/linux/g++.</b></p>

<p>The <i>&lt;ENV&gt;</i><b> </b>is a 3 letter sub-directories (cmd,cmr,csd,csr) allowing to select an environment of development applicable to your
application (see section 2.1). The cdd and cdr type does not apply to Linux.</p>

<p>Just type &quot;make clean&quot; and &quot;make&quot; to build all targets.
The generated target will be found in <b>ta-lib/c/lib</b> and <b>ta-lib/c/bin</b>.</p>
<p>You will need to
link to 3 static libraries: ta-abstract, ta-func and ta-common</p>

<h4><a name="Unix instruction0">2.3.2 All Unix Flavors Shared Libraries</a></h4>

<p>Download the source code tar.gz package and perform the following as root:</p>
```sh
$ cd ta-lib
$ ./configure --prefix=/usr
$ make
$ sudo make install
```

<p>TA-Lib is installed
as a shared library called &quot;libta-lib&quot; (name will vary depending of your
platform). With gcc you link using the switch &quot;-lta-lib&quot;.</p>

<h3 align="justify"><a name="Unix instruction">2.4
Regression Testing (All Platform)</a></h3>

<p>When building the
complete source tree, an application called &quot;ta_regtest&quot; is created in the ta-lib/c/bin
directory. This is a suite of tests to validate that the library you did compiled is behaving as expected
within your environment.</p>

<p style="margin-right: 100" align="justify">Whenever you
re-compile the TA-Lib libraries, it is suggested to re-run ta_regtest. An internet connection is required
since web data fetching is one feature being tested.</p>
<h2 style="margin-right: 150" align="justify"><a name="Technical Analysis Functions">
3.0 Technical Analysis Functions</a></h2>
<p style="margin-right: 100" align="justify"><font size="2">Make sure <a href="ta_initialize.html">TA_Initialize</a>
was called once (and only once) prior to any other API functions.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">The individual TA function can be directly called. User who would like to integrate
the TA functions without prior knowledge of their parameters, should consider
the
<a href="d_api.html#Unstable Period">abstraction
layer</a> interface.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">The source code of all the TA functions is in
<a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/ta-lib/ta-lib/c/src/ta_func/">ta-lib/c/src/ta_func</a>.</font></p>
<h3 align="justify"><a name="Direct call to a TA Function">3.1 Direct
call to a TA Function</a></h3>
<p style="margin-right: 100" align="justify"><font size="2">Direct call could be
done through the interface defined in
<a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/ta-lib/ta-lib/c/include/ta_func.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">
ta-lib/c/include/ta_func.h</a></font></p>
<p style="margin-right: 100" align="justify"><font size="2">All the TA functions are simple mathematical function. You provides the inputs with an array, and the function simply store the output in a
caller provided output array. The TA functions are NOT allocating
any space for the caller.  The
number of data in the output  will NEVER exceed the number of elements requested to be calculated (with
the startIdx and endIdx explained below).</font></p>
<p align="justify" style="margin-right: 100"><font size="2">Here is an example:</font></p>
<p align="justify" style="margin-right: 100"><font size="2">We will dissect the TA_MA function allowing to calculate a
simple moving average.</font></p>
<p align="justify"><font face="Courier New" size="2">TA_RetCode TA_MA(
<span style="background-color: #66FFFF">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; startIdx,</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #66FFFF">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; endIdx,</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #00FF00">const double&nbsp;inReal[],</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #C0C0C0">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
optInTimePeriod,</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #C0C0C0">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
optInMAType,</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #FFFF00">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
*outBegIdx,</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #FFFF00">int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; &nbsp;*outNbElement,</span><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<span style="background-color: #FFFF00">double&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; outReal[],</span><br>
&nbsp;<span lang="en-us">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
</span>)
</font></p>
<p style="margin-right: 100" align="justify"><font size="2">At first it appears that there is a lot of parameters, but do not be
discourage, all functions are consistent and share the same parameter structure. The parameters are provided in
4 sections:</font></p>
	<ul>
		<li>
  <p style="margin-right: 150" align="justify"><span style="background-color: #66FFFF"><font size="2">The output will be calculated only
    for the range specified by startIdx to endIdx.</font></span></li>
		<li>
  <p style="margin-right: 150" align="justify"><span style="background-color: #00FF00"><font size="2">
  One or more data inputs are then specified. In that example there is only one
  input. All inputs parameter
    name starts with &quot;in&quot;. </font> </span></li>
		<li>
  <p style="margin-right: 150" align="justify"><span style="background-color: #C0C0C0"><font size="2">
  zero or more optional inputs are specified
    here. In that example
    there is 2 optional inputs. These parameters allows to fine tune the function. If you do not care about a particular optIn
  just  specify TA_INTEGER_DEFAULT or TA_REAL_DEFAULT (depending of the type).</font></span></li>
		<li>
  <p style="margin-right: 150" align="justify"><span style="background-color: #FFFF00"><font size="2">
	One or more output are finally specified. In that example there is only one
	output which is outReal (the parameters outBegIdx and
  outNbElement are always specified once before the list of outputs).</font></span></li>
</ul>
<p style="margin-right: 100" align="justify"><font size="2">This structure of parameters gives a lot of flexibility to make the function
calculate ONLY the portion of required data. It is slightly complex, but it
allows demanding user to manage efficiently the memory and the CPU processing.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">Lets say you wish to calculate a 30 day moving average using closing prices.
The function call could look as follow:</font></p>
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
<p style="margin-right: 100" align="justify"><font size="2">One important aspect
of the output are the outBeg and outNbElement. Even if it was requested to
calculate for the whole range (from 0 to 399), the moving average is not valid
until the 30th day. Consequently, the outBeg will be 29 (zero base)&nbsp; and
the outNbElement will be 400-29 = 371. Meaning only
the first 371 elements of out are valid, and these could be calculated only
starting at the 30th element of the input.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">As an alternative example, if you would have requested to calculate only in
the &quot;125 to 225&quot; range (with startIdx and endIdx), the outBeg will be
125 and outNbElement will be 100. (the &quot;30&quot; minimum required is not an
issue because we dispose of 125 closing price before the start of the requested
range...). As you may have already understand, the &quot;out&quot; array will be
written only for its first 100 elements. The rest will be left untouched.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">Here is another example. In that case we want to calculate a 14 bars exponential moving average
only for 1 price bar in particular (say the last day of 300 price bar):&nbsp;</font></p>
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
<p style="margin-right: 100" align="justify"><font size="2">In that example: outBeg will be 299,&nbsp; outNbElement will be 1, and only
one value gets written into out.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">In the case that you do not provide enough data to even being able to
calculate at least one value, outNbElement will be 0 and outBeg shall&nbsp; be
ignored.</font></p>
<p style="margin-right: 100" align="justify"><font size="2">If the input and
output of a TA function are of the same type, the caller can re-use the input
buffer for storing <u>one of the output</u> of the TA function. The following example will
work:</font></p>
<div align="justify">
<pre>#define BUFFER_SIZE 100
TA_Real buffer[BUFFER_SIZE];
...
retCode = TA_MA( <span style="background-color: #00FFFF" lang="en-us">0</span>, <span style="background-color: #00FFFF" lang="en-us">BUFFER_SIZE-1</span>,
                 <span style="background-color: #00FF00">&amp;buffer[0]</span>,
                 <span style="background-color: #C0C0C0">30</span>, <span style="background-color: #C0C0C0">TA_MAType_SMA</span>,
                 <span style="background-color: #FFFF00">&amp;outBeg</span>, <span style="background-color: #FFFF00">&amp;outNbElement</span>, <span style="background-color: #FFFF00">&amp;buffer[0]</span> );</pre>
</div>
<p style="margin-right: 100" align="justify"><font size="2">Of course, the
input is overwritten, but this capability
diminish needs for temporary memory allocation for certain application. You can
assume this capability is true for all TA functions.</font></p>
<h3 align="justify"><a name="Output Size">3.2 Output Size</a></h3>
<p align="justify" style="margin-right: 100"><span lang="en-us"><font size="2">
It is important that the output array is large enough. Depending of your needs,
you might find one of the following method useful to determine the output
allocation size. All these methods are consistent and works with all TA
functions:</font></span></p>
<table class="simple">
  <tr>
    <th>Method</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>Input Matching</td>
    <td>allocationSize = endIdx + 1;<p>
    Pros: Easy to understand and implement. <br>
    Cons: Memory allocation unnecessary large when specifying small range.</td>
  </tr>
  <tr>
    <td>Range Matching</td>
    <td>allocationSize = endIdx - startIdx + 1;<br>
    <br>
    Pros: Easy to implement. <br>
    Cons: Allocation slightly larger than needed. Example: for a 30 period SMA,
    you will get 29 elements wasted because of the lookback.</td>
  </tr>
  <tr>
    <td>Exact Allocation</td>
    <td>lookback = TA_XXXX_Lookback( ... ) ;<br>
    temp = max( lookback, startIdx );<br>
    if( temp &gt; endIdx )<br>
&nbsp;&nbsp; allocationSize = 0; // No output<br>
    else <br>
&nbsp;&nbsp; allocationSize = endIdx - temp + 1;<p>
    Pros: Optimal allocation algorithm.<br>
    Cons: Slightly more complex</td>
  </tr>
</table>
<p align="justify" style="margin-right: 100">&nbsp;</p>
		<p align="justify" style="margin-right: 100">A
function TA_XXXX_Lookback is provided for each TA function. Example: For TA_SMA,
there is a TA_SMA_Lookback.</p>
<p align="justify" style="margin-right: 100">The lookback function indicates how many inputs are consume before the first
output can be calculated. Example: A simple moving average (SMA) of period 10
will have a lookback of 9.</p>

<h2><a name="Advanced">4.0 Advanced Features</a></h2>

<h3><a name="Abstraction">4.1 Abstraction Layer</a></h3>
<p>All the TA Function can be called using the interface defined in
<a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/ta-lib/ta-lib/c/include/ta_abstract.h?rev=HEAD&content-type=text/vnd.viewcvs-markup">ta-lib/c/include/ta_abstract.h</a></p>
<p>The abstraction layer is particularly interesting for an application who wishes to support the complete list of TA functions without having
to re-write new code each time a new function is added to the TA-LIB. If you wish to simply integrate in your application a small number of specific
function, you may be better to simply call these directly (see previous section).<br>
<br>
 Example:<br>
Lets say you are doing a charting software. When the user select a price bar, a side list offers blindly all the TA functions
that could be applied to a price bar. The user selects one of these, then a dialog open for allowing to adjust
the optional parameters (TA-LIB will tell your software which parameter are needed and the
valid value range for each). Once all the parameter are set, you can call blindly the
corresponding TA function. The returned information can then also blindly be drawn on the chart (some
output flags allows to get some hint on how the data shall be drawn). The same "abstract" logic apply to all the TA functions.
Some TA Functions works only on volume, or can work indifferently with any time series
data (the open, close, another indicator...) All the applicable functions to the currently selected/available
data can be determined through this "virtual" interface.<br>
<br>
The abstraction layer is a complex, but powerful  interface.</p>
<h3><a name="Unstable Period">4.2 Unstable Period</a></h3>
<p>Some TA functions provides different results depending of the &quot;starting point&quot; of the data being
involve. This is often referred as a function having memories. An example of such function is the Exponential Moving Average. It is
possible to control the unstable period (the amount of data to strip off) with
<a href="ta_setunstableperiod.html">TA_SetUnstablePeriod</a> and
<a href="ta_getunstableperiod.html">TA_GetUnstablePeriod</a>.</p>
<h3><a name="Input Type">4.3 Input Type: float vs. double</a></h3>
<p>For each technical analysis algorithm, there is one version of the function accepting the input as
array of float and another accepting array of double. The float version has a &quot;TA_S_&quot; suffix e.g.
for TA_MA there is an equivalent TA_S_MA function.</p>
<p><font face="Courier New"><font size="2">TA_RetCode TA_MA( int&nbsp;&nbsp;&nbsp; startIdx,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp; endIdx,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<b>const double&nbsp; inReal[],<br>
</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; optInTimePeriod,
<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
TA_MAType&nbsp;&nbsp;&nbsp;&nbsp; optInMAType,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; *outBegIdx,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; *outNbElement,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
double&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; outReal[] );<br>
<br>
TA_RetCode TA_S_MA( int&nbsp;&nbsp;&nbsp; startIdx,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp; endIdx,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<b>const float inReal[],</b><br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; optInTimePeriod,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
TA_MAType&nbsp;&nbsp; optInMAType,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; *outBegIdx,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
int&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; *outNbElement,<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
double&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; outReal[] );</font><br>
&nbsp;</font></p>
<p style="margin-right: 100" align="justify">Both version do all the
calculation using double e.g. when an element of a float array is accessed, it
is changed to double-precision. Consequently, both function will yield the same result.</p>
<p style="margin-right: 100" align="justify">It is typical that
users have their price bar data as float and maintain all their intermediate
calculation as double. Having direct support for both type in TA-Lib is more
memory efficient.&nbsp; With only one type, the user would be potentially forced
to duplicate their input data in a new array of a different type prior to a TA
function call. </p>
	<!-- #EndEditable -->
</div>
<h3><a name="Multithreading">4.4 High-performance multi-threading</a></h3>

<p>TA-Lib is multi-thread safe where it matters the most for performance: When calling any TA functions (e.g. TA_SMA, TA_RSI etc... )</p>

<p>One important caveat is the initialization of the "global setting" must first be all done from a single thread. That includes calls to:</p>
<ul>
  <li>TA_Initialize</li>
  <li>TA_SetUnstablePeriod, TA_SetCompatibility</li>
  <li>TA_SetCandleSettings, TA_RestoreCandleDefaultSettings</li>
</ul>

<p>After you are done with these initial calls, the application can start performing multi-thread calls with the rest of the API (including the ta_abstract.h API).</p>

<p>One exception to the rule is TA_Shutdown() which must be called single threaded (typically from the only thread remaining prior to exit your application).</p>

<p>Note: TA-Lib uses C11 malloc/free which is a thread safe default on all modern platforms (Linux,Windows,Mac). Just keep that in mind if you modify the Makefile to link with a C runtime library other than the default installed on your system.</p>

</body>