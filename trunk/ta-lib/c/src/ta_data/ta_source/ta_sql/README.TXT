===============================================================================
SQL Source Driver for TA-Lib
===============================================================================

See the online documentation about how to use the SQL data sources:
     http://ta-lib.org/d_source/d_sql.html

Interface using the "MySQL++ Connector" and the unixodbc are distributed
seperatly at http://ta-lib.org


Quick Start
===========
On win32, ODBC support is always part of TA-Lib.

On Linux, ODBC support is possible only when you install seperatly
the "ta-unixodbc" library. Read documentation included in that package.

On win32, direct support for "MySQL++ Connector" is possible when you
install seperatly the "ta-mysql" library. Read documentation included
in the ta-mysql package. Note that it is possible to access MySQL
through ODBC, and the additional "ta-mysql" package is needed stricly
if you wish to bypass ODBC.


Design
======
TA-Lib have the notion of "Data Source Driver" that are statically link.
The SQL driver have an additional level of drivers called "minidriver". 
These can be added dynamically at runtime. This is how the MySQL++ Connector
and unixodbc gets  optionally added to TA-Lib by independent libraries.

=====================
Win32 ODBC Test Setup
=====================
Windows 2000, ODBC lib version 3.52, Visual C++ 6.0:
    MySQL ODBC 3.51                     version 3.51.06.00
    Microsoft Access Driver (*.mdb)     version 4.00.6200.00

This is not a requirement list, this library is expected to work
with other setups.
