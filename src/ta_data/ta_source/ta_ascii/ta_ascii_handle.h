#ifndef TA_ASCII_HANDLE_H
#define TA_ASCII_HANDLE_H

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

#ifndef TA_READOP_H
   #include "ta_readop.h"
#endif

#ifndef TA_DATA_UDB_H
   #include "ta_data_udb.h"
#endif

typedef struct
{
   TA_Libc *libHandle;
   
   /* Keep a ptr on the user TA_AddDataSource parameters. */
   const TA_AddDataSourceParamPriv *param;

   /* The TA_FileIndex represent all the categories and symbols
    * extracted from the provided 'sourceLocation' and 'categoryString'.
    */
   TA_FileIndex *theFileIndex;

   /* Contains all the info for interpreting the ASCII file. */
   TA_ReadOpInfo *readOpInfo;

} TA_PrivateAsciiHandle;

/* Alloc/Free for the TA_DataSourceHandle.
 *
 * Takes care also to alloc/initialize/free the TA_PrivateAsciiHandle
 * which is the 'opaque' part of the TA_DataSourceHandle.
 */
TA_DataSourceHandle *TA_ASCII_DataSourceHandleAlloc( TA_Libc *libHandle,
                                                     const TA_AddDataSourceParamPriv *param );

TA_RetCode TA_ASCII_DataSourceHandleFree( TA_DataSourceHandle *handle );

/* Build (or re-build) the TA_FileIndex for this handle. */
TA_RetCode TA_ASCII_BuildFileIndex( TA_DataSourceHandle *handle );

/* Build the list of "read operations" to perform for this ASCII file. */
TA_RetCode TA_ASCII_BuildReadOpInfo( TA_DataSourceHandle *handle );

#endif
