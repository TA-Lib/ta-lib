#ifndef TA_CSI_HANDLE_H
#define TA_CSI_HANDLE_H

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

#ifndef TA_CSI_FILES_H
   #include "ta_csi_files.h"
#endif


typedef struct
{  
   /* Keep a ptr on the user TA_AddDataSource parameters. */
   const TA_AddDataSourceParamPriv *param;

   /* Index extracted from the files. */
   struct MasterListRecordType *index;
   int indexSize;

   /* A TA_String for each MasterListRecordType */
   TA_String **indexString;

   /* A TA_String for the CSI_ID_CATEGORY_NAME */
   TA_String *category;

} TA_PrivateCSIHandle;

#define CSI_ID_CATEGORY_NAME "CSI_ID"

/* Alloc/Free for the TA_DataSourceHandle.
 *
 * Takes care also to alloc/initialize/free the TA_PrivateCSIHandle
 * which is the 'opaque' part of the TA_DataSourceHandle.
 */
TA_DataSourceHandle *TA_CSI_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param );
TA_RetCode TA_CSI_DataSourceHandleFree( TA_DataSourceHandle *handle );

/* Build (or re-build) the index for this handle. */
TA_RetCode TA_CSI_BuildIndex( TA_DataSourceHandle *handle );

#endif
