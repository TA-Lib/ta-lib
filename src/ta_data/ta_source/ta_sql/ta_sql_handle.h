#ifndef TA_SQL_HANDLE_H
#define TA_SQL_HANDLE_H

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

#ifndef TA_SQL_MINIDRIVER_H
   #include "ta_sql_minidriver.h"
#endif

#define TA_SQL_CATEGORY_COLUMN              "category"
#define TA_SQL_SYMBOL_COLUMN                "symbol"
#define TA_SQL_DATE_COLUMN                  "date"
#define TA_SQL_TIME_COLUMN                  "time"
#define TA_SQL_OPEN_COLUMN                  "open"
#define TA_SQL_HIGH_COLUMN                  "high"
#define TA_SQL_LOW_COLUMN                   "low"
#define TA_SQL_CLOSE_COLUMN                 "close"
#define TA_SQL_VOLUME_COLUMN                "volume"
#define TA_SQL_OI_COLUMN                    "oi"

#define TA_SQL_MAX_COLUMN_NAME_SIZE         10


#define TA_SQL_CATEGORY_PLACEHOLDER         "$c"
#define TA_SQL_SYMBOL_PLACEHOLDER           "$s"
#define TA_SQL_START_DATE_PLACEHOLDER       "$<"
#define TA_SQL_END_DATE_PLACEHOLDER         "$>"
#define TA_SQL_START_TIME_PLACEHOLDER       "$["
#define TA_SQL_END_TIME_PLACEHOLDER         "$]"

/* placeholders not supported yet
#define TA_SQL_COUNTRY_PLACEHOLDER          "$z"
#define TA_SQL_EXCHANGE_PLACEHOLDER         "$x"
#define TA_SQL_TYPE_PLACEHOLDER             "$t"
*/


typedef struct
{
   /* the name of the category */
   TA_String *category;

   /* List of symbols in this category.
    * Elements on this list are of TA_String type.
    */
   TA_List *theSymbols;

} TA_SQLCategoryNode;

typedef struct
{  
   /* Keep a ptr on the user TA_AddDataSource parameters. */
   const TA_AddDataSourceParamPriv *param;

   /* database name (from location) */
   TA_String *database;

   /* theSymbolsIndex represent all the categories and symbols
    * extracted from the provided data source parameters.
    * Elements on this list are of TA_SQLCategoryNode type.
    */
   TA_List *theCategoryIndex;

   /* ID of the minidriver requested for this connection */
   TA_SQL_MinidriverID minidriver;

   /* Contains database connection object 
    * It is opaque data used by connection minidrivers
    */
   void *connection;

} TA_PrivateSQLHandle;

/* Alloc/Free for the TA_DataSourceHandle.
 *
 * Takes care also to alloc/initialize/free the TA_PrivateSQLHandle
 * which is the 'opaque' part of the TA_DataSourceHandle.
 */
TA_DataSourceHandle *TA_SQL_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param );

TA_RetCode TA_SQL_DataSourceHandleFree( TA_DataSourceHandle *handle );

/* Build (or re-build) the categories and symbols index for this handle. */
TA_RetCode TA_SQL_BuildSymbolsIndex( TA_DataSourceHandle *handle );


#endif

