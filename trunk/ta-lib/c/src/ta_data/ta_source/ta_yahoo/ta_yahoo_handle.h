#ifndef TA_YAHOO_HANDLE_H
#define TA_YAHOO_HANDLE_H

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

#ifndef TA_YAHOO_IDX_H
   #include "ta_yahoo_idx.h"
#endif

#ifndef TA_READOP_H
   #include "ta_readop.h"
#endif

typedef struct
{   
   /* Keep a local copy of the initial parameters. */
   const TA_AddDataSourceParamPriv *param;

   /* Represent all the categories and symbols available. */
   TA_YahooIdx *index;

   /* The read operations for interpreting the Yahoo! CSV format. */
   TA_ReadOpInfo *readOp6Fields;
   TA_ReadOpInfo *readOp5Fields;
   TA_ReadOpInfo *readOp2Fields;
} TA_PrivateYahooHandle;

/* Alloc/Free for TA_DataSourceHandle.
 * Takes care also to alloc/initialize/free the TA_PrivateHandle which
 * become the 'opaque' part of the TA_DataSourceHandle.
 */
TA_DataSourceHandle *TA_YAHOO_DataSourceHandleAlloc( void );
TA_RetCode TA_YAHOO_DataSourceHandleFree( TA_DataSourceHandle *handle );

#endif
