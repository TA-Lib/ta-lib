===============================================================================
MySQL Source Driver for TA-Lib
===============================================================================
vim: textwidth=77 fileencoding=utf-8

This file describes how TA_MYSQL source data driver interprets 
the TA_AddDataSourceParam struct:

TA_AddDataSourceParam.id: must be equal to TA_MYSQL

TA_AddDataSourceParam.flags: None currently supported (TA_NO_FLAGS)
Later, TA_REPLACE_ZERO_PRICE_BAR will be supported, plus some additional:
TA_PAD_PRICE_BAR: in case some price bars are missing, generate dummy price
bars equal to previous valid close (i.e. similar to TA_REPLACE_ZERO_PRICE_BAR,
but assume non-existing in-between to be zero).
TA_INTERPOLATE_PRICE_BAR: in case some price bars are missing, interpolate
values between previous and next valid close.

TA_AddDataSourceParam.location: URL descibing the access to the MySQL database, 
in the form:
"mysql://myserver.mydomain.org:port/database"
The scheme is fixed to "mysql". The server name is just a name 
(like "localhost") or FQDN or IP address.  No username/password supported here. 
The port is optional and defaults to 3306 (MySQL default port).
The path has one segment only and is interpretted as the database name
(and a default stock symbol in case other fields do not specify how to obtain
stock symbols).

TA_AddDataSourceParam.info identifies the SQL query to execute to get
the records.  The columns may have to be renamed to some standard names
to be able to interpret the results.  
Recognized columns are: date, time, open, high, low, close, volume, oi 
Example: 
"SELECT date, time, quote AS close FROM quotes_intraday_AEX WHERE date = 20031014" 
Fields date and time (if present) will be interpreted as ISO formats (SQL default).

TA_AddDataSourceParam.username and
TA_AddDataSourceParam.password are used to log in to the server. 
May be NULL if not needed. In that case, username defaults to user login name,
password defaults to an empty password.

TA_AddDataSourceParam.category contains the default category string or 
an SQL query returning all categories within the database.  This can 
be done either by returning a recordset with column "category" or 
with columns "country", "exchange", and "type". Optionally, the query 
may return column "symbol" identifying all symbols within a given category.

TA_AddDataSourceParam.country
TA_AddDataSourceParam.exchange
TA_AddDataSourceParam.type: used only when TA_AddDataSourceParam.category is
NULL.  Those fields contain strings making up the default category name or
SQL queries returning all supported category components. Queries return data
in columns "country", "exchange", and "type" respectively.
NOTE: Currently handling of country, exchange, and type fields is not
implemented.  Use category field instead.

TA_AddDataSourceParam.symbol: contains an SQL query returning
all symbols in the database.  This field is used only
when TA_AddDataSourceParam.category query does not return a
column named "symbol" If this field is NULL, (and the query in
TA_AddDataSourceParam.category field did not return column "symbol"),
the database name is used as the symbol.  If this field does not contain an
SQL query, it is taken literally as the symbol name.

PLACEHOLDERS
------------
Usually, categories and symbols are not orthogonal (i.e. the list of symbols
depends on the category).  To facilitate this in queries, a number of
placeholders are recognized and expanded with proper values:
$c      expanded with the category string
$z      expanded with the country string
$x      expanded with the exchange string
$t      expanded with the type string
$s      expanded with the symbol string

Applicability of placeholders is as follows:
TA_AddDataSourceParam.category:     none expanded
TA_AddDataSourceParam.country:      $c expanded
TA_AddDataSourceParam.exchange:     $c $z expanded
TA_AddDataSourceParam.type:         $c $z $x expanded
TA_AddDataSourceParam.symbol:       $c $z $x $t expanded
TA_AddDataSourceParam.info:         $c $z $x $t $s expanded

Example:
TA_AddDataSourceParam.exchange = "SELECT exchange FROM exchange_data WHERE country = '$z'";

Timestamp placeholders:
To optimize the execution of the data query, the TA_AddDataSourceParam.info 
parameter is subject of expansion of the following placeholders:
$<      expanded with the begin date
$>      expanded with the end date
(mnemonic: think of $< and $> as opening and closing angle brackets defining
a timespan).

Example:
TA_AddDataSourceParam.info = "SELECT * FROM stock_data WHERE '$<' <= date AND '$'> >= date ORDER BY date"
TA_AddDataSourceParam.info = "SELECT * FROM stock_data WHERE date BETWEEN '$<' AND '$>' ORDER BY date"

IMPORTANT NOTES
---------------
The order of the data is important! TA-Lib cannot process unordered 
historical data.  Data should be ordered by date, time (or just date if time
is not available).  All rows should be unique on (date, time) key (or just
date if time is not available).

========================================================================
Compilation Notes
========================================================================

Currently, this driver is tested only with mysql++ version 1.7.1 for Visual C++

Compilation of this driver is DISABLED by default.  In order to get this
driver compiled in your ta-lib build, you need to do the following:
1. Download libMySQL++ from http://www.mysql.com/get/Downloads/mysql++/mysql++-1.7.1-1-win32-vc++.zip/from/pick
2. Unpack it to a peer directory to ta-lib, e.g. if ta-lib is in
    C:\src\ta-lib
then mysql++ should be in 
    C:\src\mysql++
3. Copy C:\src\mysql++\mysql\lib\libmySQL.dll to Windows directory.
4. Compile mysql++ in the same model as you use for ta-mysql (Debug/Release,
Single/Multi-thread, etc.)
5. Define preprocessor symbol TA_SQL in the top-level makefile of ta-lib (or
Visual C++ project settings).
6. Add C:\src\mysql++\include to include search path for ta-lib
7. Add C:\src\lib\mysql++.lib to link libs.
8. Rebuild ta-lib.
Note that the resulting ta_libc_???.lib will be GPL, not BSD, because mysql++
is GPL.


========================================================================
To Do:

Increase TA_CATEGORY_MAX_LENGTH to 200 (to accomodate longer SQL queries).
Implement handling of country, exchange, and type (on hold, because not
really needed).
Replace MySQL++ by libMYSQL and switch to plain C (on hold because of
incompatible MySQL license).
Expand time placeholders: $[ and $]
Validate timestamps in history procedure (to avoid buffer overflows during
date conversion).
Replace MySQL by ODBC

To do for Mario:
Implement TA_FatalReportToBuffer().
Implement automatic conversion from PERIOD_1SEC to PERIOD_1MIN and higher.
Complete strings for error codes.
Allow for null-drivers (drivers with function pointers set to null), 
to simplify development (actually, I have done it already, but regressionn
testing must be performed).

Questions for Mario:
Are driver functions protected for multithread access, or do they have to build a protection on their own?
What error codes to use for specific cases in my driver.

TA_XXX_GetNextCategoryHandle has an index parameter. In 
TA_MYSQL_GetNextCategoryHandle I use TA_List which allows for iterating
through the list but not random element access.  Can I ignore the index and
assume always seqiential access?  Can I use the internal list iterator and
assume only one thread will be accessing it?
