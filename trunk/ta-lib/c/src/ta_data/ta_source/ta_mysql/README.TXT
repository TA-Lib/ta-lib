===============================================================================
MySQL Source Driver for TA-Lib
Copyright 2003 (C) Paweł A. Konieczny
===============================================================================

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
(and a default stock symbols in case other fields do not specify how to obtain
stock symbols).

TA_AddDataSourceParam.info identifies the SQL query to execute to get
the records.  The columns may have to be renamed to some standard names
to be able to interpret the results.  
Recognized columns are: date, time, open, high, low, close, volume, oi 
Example: 
"SELECT date, time, quote AS close FROM quotes_intraday_AEX WHERE date = 20031014" 
Fields dateand time (if present) will be interpreted as ISO formats (SQL default).

TA_AddDataSourceParam.username and
TA_AddDataSourceParam.password are used to log in to the server. 
May be NULL if not needed.

TA_AddDataSourceParam.category contains the default category string or an SQL query
returning all categories within the database.  This can be done either by
returning a recordset with column "category" or with columns "country",
"exchange", and "type". Optionally, the query may return column "symbol"
identifying all symbols within a given category.

TA_AddDataSourceParam.country
TA_AddDataSourceParam.exchange
TA_AddDataSourceParam.type: used only when TA_AddDataSourceParam.category is
NULL.  Those fields contain strings making up the default category name or
SQL queries returning all supported category components. Queries return data
in columns "country", "exchange", and "type" respectively.

TA_AddDataSourceParam.symbol: contains an SQL query returning all symbols in
the database.  Data is returned in the column named "symbol".  If this field
is NULL, the list of symbols is obtained from the query in
TA_AddDataSourceParam.category field, column "sumbol".  If such column is not
available, the database name is used as the symbol.

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
TA_AddDataSourceParam.exchange = "SELECT exchange FROM exchange_data WHERE category = '$z'";

========================================================================
To Do:

Replace MySQL++ by libMYSQL and switch to plain C.


