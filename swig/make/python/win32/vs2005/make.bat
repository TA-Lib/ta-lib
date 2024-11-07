call "C:\Program Files\Microsoft Visual Studio 8\Common7\Tools\vsvars32.bat"
set PATH=c:\python25;c:\swigwin-1.3.33;%PATH%
set INCLUDE=C:\Program Files\Microsoft SDKs\Windows\v6.0\Include;%INCLUDE%
set LIB=C:\Program Files\Microsoft SDKs\Windows\v6.0\Lib;%LIB%
nmake %1 %2 %3 %4 %5 %6 %7 %8 %9
