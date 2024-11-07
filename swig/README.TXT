SWIG Wrapper for TA-Lib
=======================
 vi:set tw=77:

This directory tree contains files needed to build a SWIG wrapper to 
TA-Lib. SWIG is a software development tool for building scripting language
interfaces to C and C++ programs. It supports a number of target languages:
including Guile, Java, Mzscheme, Ocaml, Perl, Pike, PHP, Python, Ruby, and
Tcl.  More information can be found at http://www.swig.org

I had two goals while making this wrapper:
1. To stay as close as possible to the original TA-LIB C interface
2. To make the interface as much convenient (and "Perlish") and robust 
   as possible.
Those two objectives were often not compatible, what results in many elements
being wrapped in two or more ways; choose your own perferences.  In a few
cases, those objectives were conflicting; then the Perl way took precedence
over being literally identical to the TA-LIB C way.

Although SWIG is capable of generating wrappers to many languages, makefiles
in this directory tree support Perl and Python only.  Taking these as examples, one can
extend it to other laungages, if needed.

First Thing
===========
If you are a Win32 user, and you are using Perl 5.8.0, you will find in
the TA-Lib MSVC package the pre-compiled binaries. You can then directly 
jump to the section "Using TA-Lib in Perl".

Others will need to build their own binaries. Follow the step starting 
with the "Installation" section.

SWIG Installation
==================
To build the wrapper, you need to have SWIG installed at your system. Get it
from http://www.swig.org.  The swig executable must be present on your search
path. Verify it at command prompt:
> swig -version

This project is tested with SWIG version 1.3.21. It should work also with
higher versions within the 1.3.x familly.  If you have a lower SWIG version, 
better upgrade.

Python Installation
===================
The interface was tested with Pyton v2.3.5, v24 adn v2.5
Will likely work with newer version.

You must have the Python source code installed, else you
will get compilation error on a missing "Python.h" file.

Perl Installation
=================
Obviously, you need also perl installed at your system (Perl5).  The project
is tested with perl v5.6.1 and v5.8.4.  It should work with newer perl
releases as well.

Further, you need a C/C++ compiler, the same as used to compile the perl
executable you use.  At the end, we will be building perl modules.  If you
use ActivePerl on Windows (from ActiveState), you most probably need Visual
C++ version 6.0.  If you use Linux, you probably need gcc, etc.  To find it
out, try:
> perl -V

Building the Perl module
========================
Build can be done using MSVC with either the nmake utility or using the IDE.

IDE - Microsoft Developer Studio
--------------------------------
First, make sure that the proper ta-lib library is already build in
ta-lib/c/lib.  The wrapper links only with Release Multithreaded DLL variant
(because of necessary compatibility with Perl build), so you need
ta_libc_cdr.lib

Open workspace swig\ide\msvc\perl\perl.dsw and hit Build All. You can 
alternatively use "nmake" in that same directory.

make - Linux/gcc
----------------
First, make sure that the proper ta-lib library is already build in
ta-lib/c/lib.  The wrapper links only with Release Multithreaded Static
variant so you need have libta_libc_cmr.a.

cd swig/make/perl/linux/g++
make

Using ta-lib in Perl
====================
The generated perl modules are placed in ta-lib/swig/lib/perl, so make sure
that this directory is placed in your Perl lib search path.  Alternatively,
copy the modules to the appropriate places in your system, where local/site
modules belong.  Another alternative is to use "use lib" pragma in your perl
scripts.

The module name is Finance::TA.  The C identifier names are retained literally, 
so you can use the C API spec.  In case of doubt, look at examples in
ta-lib/swig/src/demo/perl and - even better - at the test scripts in 
ta-lib/swig/src/tools/test_perl.  The test scripts tend to be complete so there
is an example for every function, constant, or type.

The module covers all TA-LIB functions, a few exceptions exist.

In general, the following usage rules apply:
 * structs are turned to Perl classes, so can be created using 'new'
 * access to struct fields are provided through tied variables
    e.g.
        print $history->{period};
 * Ouput parameters are returned as elements in the return list
    e.g.  
        $ts = new TA_Timestamp;
        TA_SetDate($ts, 2004, 2, 29);
        @ret = TA_GetDate($ts);
        # @ret == ($TA_SUCCESS, 2004, 2, 29);
 * Null pointers are represented by undef

There are some exceptions and there are easier ways of doing things:

* There is no need to call &TA_Initialize.  The module initializes itself on
'use' with default parameters.  Call &TA_Initialize if you need to provide
your own init params.

* There is no need to call &TA_Shutdown, ever.  The module shuts itself down
on 'END'. Calling &TA_Initialize on already initialized library
automatically invokes &TA_Shutdown first.  If you call &TA_Shutdown, make
sure that there are no defined "TA" variables around.  Some objects make
calls to TA-LIB in their destructors.

* Instead of doing
    $rci = new TA_RetCodeInfo;
    TA_SetRetCodeInfo($retCode, $rci);
you may prefer:
    $rci = new TA_RetCodeInfo($retCode);

* As a bonus to Perl programmers, the constructor of TA_Timestamp allows for
initializing the date and time:
    $ts = new TA_Timestamp(2004, 2, 29, 9, 11, 59);
    $ts = new TA_Timestamp(2004, 2, 29);  # time set to zero
But There's More Than One Way To Do It.
    $ts = new TA_Timestamp("2004-02-29 09:11:59");
    $ts = new TA_Timestamp("2004-02-29");
To be parsed, all numbers have to be two-digit (except the year).  Superfluous
characters are ignored. Dash separators for date, colons for time.

* Some other TA_Timestamp goodies: synthesized attributes (data members):
    $y = $ts->{year};
    $m = $ts->{month};
    $s = $ts->{day};
    $ts->{hours} = 12;
    $ts->{minutes} = 30;
    $ts->{seconds} = 59;

* Do not call &TA_HistoryAlloc and &TA_HistoryFree directly.  Simply use
&TA_History::new.  The parameters to &new are the same as to
TA_HistoryAlloc(). 
E.g.:
    my $haParam = new TA_HistoryAllocParam;  # my $haParam = {}; works too
    $haParam->{category} = "US.NASDAQ.STOCKS";
    $haParam->{symbol} = "LNUX";
    $haParam->{period} = $TA_DAILY;
    $haParam->{field} = $TA_ALL;
    my $hist = new TA_History($udBase, $haParam);
    if ($hist->{retCode} == $TA_SUCCESS) {
        ...
    }
Alternatively:
    my $hist = $udBase->History($haParam);
    if ($hist->{retCode} == $TA_SUCCESS) {
        ...
    }
Alternatively:
    my $hist = $udBase->History({
            category => "US.NASDAQ.STOCKS", 
            symbol => "LNUX", 
            period => $TA_DAILY,
            field => $TA_ALL,
        });

* Note that TA_History objects are read-only.

* The TA_History member arrays are made available as references to Perl
arrays.  Examples:
    print $hist->{close}[0], "\n";
    $close_series = $hist->{close};
    print $$close_series[0], "\n";
    print "@$close_series\n";

* Note that each access to a tied member variable is going through the
thunk code from Perl to C and back, so for the sake of performance, 
consider copying the history arrays to Perl arrays.
Eg.
    # slow
    for ($i = 0;  $i < $history->{nbBars};  $i++) {
        print $history->{close}[$i], "\n";
    }
    # fast
    @close = @{ $history->{close} };
    for $close (@close) {
        print $close, "\n";
    }

* If you do this for the timestamp array, note that the Perl array contains
only the references to C Timestamp structs, not the object themselves.
The C structs fall under the authority of C hence are not reference-counted.
This means that if you undef the history, the references to timestamps become
garbage.

* Insead of doing:
    $retCode = TA_UDBaseAlloc(\$udBase);
    TA_UDBaseFree($udBase)
you may strongly prefer:
    $udBase = new TA_UDBase; 
In the former case, you have to call &TA_UDBaseFree, otherwise a memory leak
will occur.  Also, after the call to &TA_UDBaseFree, you must not use the
variable anywhere, otherwise a segmentation fault may occur.  In such case it
is safer to undef the variable directly after the call to &TA_UDBaseFree.
In the latter case, the database will be freed automatically when $udBase
goes out of scope or is redefined. If the allocation fails, $udBase will be
undef.  There is no way of getting the ret code then.  But hey, what can go
wrong with TA_UDBaseAlloc()?  And you are freed from the hurdle of keeping
track of allocation and deallocation.  This is the True Perl Way.

* The same as above applies to TA_FuncHandle, TA_FuncInfo, except that there
is no need (and no possibility) to deallocate the handles.

* Instead of TA_CategoryTableAlloc/TA_CategoryTableFree use just
&TA_CategoryTable.  This function returns a Perl list of strings, so there is
no need do deallocate it explicitly.  Note that the first list element is the
return code, as usual.  The same applies to TA_SymbolTableAlloc/Free,
TA_GoupTableAlloc/Free, TA_FuncTableAlloc/Free.

* Alternatively, you may want to use the object interface of TA_UDBase:
    @categories = $udBase->CategoryTable();
    @symbols = $udBase->SymbolTable("ZZ.OTHER.OTHER");
    @symbols = $udBase->SymbolTable(); # defaults to $TA_DEFAULT_CATEGORY
In such case, you don't have to bother about the error code; you get undef
value in case of any error.

* C functions in ta_func have three groups of parameters, input, optional
input and output.  The input parameters are startIdx, endIdx, and one or more
arrays with numbers, These are also expected parameters of the Perl
functions, with a note that in place of arrays, references to arrays are
expected.

The optional parameters can be set to $TA_INTEGER_DEFAULT/$TA_REAL_DEFAULT or
undef, or ommitted altogether.

The output parameters are retured in the return array.  The returned array
has the following structure: return code of the function, begin index, and
one or more references to output arrays.  The parameter outNbElement used in
the C functions is not passed to Perl as it can be easilly obtained by
evaluating an array in a scalar context.

Example (default optInTimePeriod):
    ($retCode, $begIdx, $outReal) = TA_MAX(0, $#inReal, \@inReal);

More examples in swig/src/tools/test_perl/ta_func.t

* Several complex TA-LIB functions accept a pointer to a struct containing
the parameters for the function invocation.  This is to avoid long argument
lists in function signatures.  Using this in Perl is rather inconvenient,
meaning that a specific object has to be created, and each of it's fields has
to be initialised explicitly through tied variables.  A simpler way exists
if you use the object-oriented interface of the wrapper.  Instead of passing
a reference to a specific object type as required by TA-LIB, you may pass
a reference to a (possibly unnamed) plain hash.  
This works for the following functions: TA_History (see example above), 
AddDataSource, TradeLogAdd.


Good luck!
Pawel Konieczny, konieczp-at-users.sourceforge.net 


Open issues/To do
=================

* How to write log files to stdout
* How to define a custom TA fatal handler in Perl
* Get parameters on "use"-line for &TA_Initialize

