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

#endif
