#ifndef TA_MYSQL_HANDLE_H
#define TA_MYSQL_HANDLE_H

extern "C" {

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_LIST_H
   #include "ta_list.h"
#endif

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

#ifndef TA_LIST_H
   #include "ta_list.h"
#endif

#ifndef TA_DATA_UDB_H
   #include "ta_data_udb.h"
#endif
}

#if defined(_MSC_VER)
/* disable some C++ warnings in MySQL++ for Microsoft compiler */
#pragma warning( disable: 4800 4355 )
#endif
#include <mysql++.h>


#define TA_MYSQL_CATEGORY_COLUMN_NAME         "category"
#define TA_MYSQL_SYMBOL_COLUMN_NAME           "symbol"
#define TA_MYSQL_DATE_COLUMN_NAME             "date"
#define TA_MYSQL_TIME_COLUMN_NAME             "time"
#define TA_MYSQL_OPEN_COLUMN_NAME             "open"
#define TA_MYSQL_HIGH_COLUMN_NAME             "high"
#define TA_MYSQL_LOW_COLUMN_NAME              "low"
#define TA_MYSQL_CLOSE_COLUMN_NAME            "close"
#define TA_MYSQL_VOLUME_COLUMN_NAME           "volume"
#define TA_MYSQL_OI_COLUMN_NAME               "oi"

#define TA_MYSQL_CATEGORY_PLACEHOLDER         "$c"
#define TA_MYSQL_SYMBOL_PLACEHOLDER           "$s"
#define TA_MYSQL_START_DATE_PLACEHOLDER       "$<"
#define TA_MYSQL_END_DATE_PLACEHOLDER         "$>"

/* placeholders not supported yet
#define TA_MYSQL_COUNTRY_PLACEHOLDER          "$z"
#define TA_MYSQL_EXCHANGE_PLACEHOLDER         "$x"
#define TA_MYSQL_TYPE_PLACEHOLDER             "$t"
*/


typedef struct
{
   /* the name of the category */
   TA_String *category;

   /* List of symbols in this category.
    * Elements on this list are of TA_String type.
    */
   TA_List *theSymbols;

} TA_MySQLCategoryNode;

typedef struct
{  
   /* Keep a ptr on the user TA_AddDataSource parameters. */
   const TA_AddDataSourceParamPriv *param;

   /* theSymbolsIndex represent all the categories and symbols
    * extracted from the provided data source parameters.
    * Elements on this list are of TA_MySQLCategoryNode type.
    */
   TA_List *theCategoryIndex;

   /* Contains MySQL Connection object */
   Connection *con;

   /* database name (from location) */
   TA_String *database;

} TA_PrivateMySQLHandle;

/* Alloc/Free for the TA_DataSourceHandle.
 *
 * Takes care also to alloc/initialize/free the TA_PrivateMySQLHandle
 * which is the 'opaque' part of the TA_DataSourceHandle.
 */
TA_DataSourceHandle *TA_MYSQL_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param );

TA_RetCode TA_MYSQL_DataSourceHandleFree( TA_DataSourceHandle *handle );

/* Build (or re-build) the categories and symbols index for this handle. */
TA_RetCode TA_MYSQL_BuildSymbolsIndex( TA_DataSourceHandle *handle );

/* Copy templateStr to resultStr replacing holderStr by valueStr */
char * TA_MYSQL_ExpandPlaceholders( const char *templateStr, const char *holderStr, const char *valueStr );

#endif
