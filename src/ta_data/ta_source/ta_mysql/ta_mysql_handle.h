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

#ifndef TA_FILEINDEX_H
   #include "ta_fileindex.h"
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
   /* Keep a ptr on the user TA_AddDataSource parameters. */
   const TA_AddDataSourceParamPriv *param;

   /* The TA_FileIndex represent all the categories and symbols
    * extracted from the provided data source parameters.
    */
   TA_FileIndex *theFileIndex;

   /* Contains MySQL Connection object */
   Connection *con;

} TA_PrivateMySQLHandle;

/* Alloc/Free for the TA_DataSourceHandle.
 *
 * Takes care also to alloc/initialize/free the TA_PrivateMySQLHandle
 * which is the 'opaque' part of the TA_DataSourceHandle.
 */
TA_DataSourceHandle *TA_MYSQL_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param );

TA_RetCode TA_MYSQL_DataSourceHandleFree( TA_DataSourceHandle *handle );

/* Build (or re-build) the TA_FileIndex for this handle. */
TA_RetCode TA_MYSQL_BuildFileIndex( TA_DataSourceHandle *handle );

#endif
