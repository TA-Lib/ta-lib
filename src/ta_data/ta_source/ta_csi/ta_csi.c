/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  031904 MF   First version.
 *
 */

/* Description:
 *    This is the entry points of the data source driver for the CSI format.
 *
 *    It provides ALL the functions needed by the "TA_DataSourceDriver"
 *    structure (see ta_source.h).
 */

/**** Headers ****/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ta_source.h"
#include "ta_common.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_list.h"
#include "ta_data.h"
#include "ta_system.h"
#include "ta_csi_handle.h"
#include "ta_global.h"

#include "ta_csi.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_CSI_InitializeSourceDriver( void )
{
   TA_PROLOG
   TA_TRACE_BEGIN( TA_CSI_InitializeSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_ShutdownSourceDriver( void )
{
   TA_PROLOG
   TA_TRACE_BEGIN( TA_CSI_ShutdownSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_GetParameters( TA_DataSourceParameters *param )
{
   TA_PROLOG

   TA_TRACE_BEGIN( TA_CSI_GetParameters );

   memset( param, 0, sizeof( TA_DataSourceParameters ) );

   param->flags = TA_LOCATION_IS_PATH;

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_CSI_OpenSource( const TA_AddDataSourceParamPriv *param,
                              TA_DataSourceHandle **handle )
{
   TA_PROLOG
   TA_DataSourceHandle *tmpHandle;
   TA_PrivateCSIHandle *privData;
   TA_RetCode retCode;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( TA_CSI_OpenSource );

   TA_ASSERT( handle != NULL );
   TA_ASSERT( param != NULL );

   *handle = NULL;

   stringCache = TA_GetGlobalStringCache();

   /* Allocate and initialize the handle. This function will also allocate the
    * private handle (opaque data).
    */
   tmpHandle = TA_CSI_DataSourceHandleAlloc(param);

   if( tmpHandle == NULL )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   privData = (TA_PrivateCSIHandle *)(tmpHandle->opaqueData);

   privData->param = param;

   /* Now build the index. */  
   retCode = TA_CSI_BuildIndex( tmpHandle );

   if( retCode != TA_SUCCESS )
   {
      TA_CSI_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( retCode );
   }

   /* Set the total number of distinct category. */
   tmpHandle->nbCategory = 1;

   /* All fine, return result to caller. */
   *handle = tmpHandle;
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_CloseSource( TA_DataSourceHandle *handle )
{
   TA_PROLOG

   TA_TRACE_BEGIN( TA_CSI_CloseSource );

   /* Free all ressource used by this handle. */
   if( handle )
      TA_CSI_DataSourceHandleFree( handle );

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                          TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG
   TA_PrivateCSIHandle *privData;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( TA_CSI_GetFirstCategoryHandle );

   TA_ASSERT( handle != NULL );

   stringCache = TA_GetGlobalStringCache();

   privData = (TA_PrivateCSIHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(161) );
   }

   /* Set the categoryHandle. */
   categoryHandle->string = privData->category;
   categoryHandle->nbSymbol = privData->indexSize;
   categoryHandle->opaqueData = NULL;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_GetFirstSymbolHandle( TA_DataSourceHandle *handle,
                                        TA_CategoryHandle   *categoryHandle,
                                        TA_SymbolHandle     *symbolHandle )
{
   TA_PROLOG
   TA_PrivateCSIHandle *privData;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( TA_CSI_GetFirstSymbolHandle );

   TA_ASSERT( handle != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( symbolHandle != NULL );

   stringCache = TA_GetGlobalStringCache();
   privData = (TA_PrivateCSIHandle *)(handle->opaqueData);

   TA_ASSERT( privData != NULL );
   TA_ASSERT( privData->index != NULL );
   TA_ASSERT( privData->indexSize >= 1 );

   /* Set the symbolHandle. */
   symbolHandle->string = privData->indexString[0];
   symbolHandle->opaqueData = &privData->index[0];

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_GetNextSymbolHandle( TA_DataSourceHandle *handle,
                                       TA_CategoryHandle   *categoryHandle,
                                       TA_SymbolHandle     *symbolHandle,
                                       unsigned int index )
{
   TA_PROLOG
   TA_PrivateCSIHandle *privData;
   TA_StringCache *stringCache;

   TA_TRACE_BEGIN( TA_CSI_GetNextSymbolHandle );

   (void)index; /* Get ride of compiler warnings. */

   TA_ASSERT( handle != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( symbolHandle != NULL );

   stringCache = TA_GetGlobalStringCache();
   privData = (TA_PrivateCSIHandle *)(handle->opaqueData);

   TA_ASSERT( privData != NULL );
   TA_ASSERT( privData->index != NULL );
   TA_ASSERT( privData->indexString != NULL );
   TA_ASSERT( privData->indexString[index] != NULL );
   TA_ASSERT( privData->indexSize >= 1 );

   /* Set the symbolHandle. */
   symbolHandle->string = privData->indexString[index];
   symbolHandle->opaqueData = &privData->index[index];

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_GetHistoryData( TA_DataSourceHandle *handle,
                                  TA_CategoryHandle   *categoryHandle,
                                  TA_SymbolHandle     *symbolHandle,
                                  TA_Period            period,
                                  const TA_Timestamp  *start,
                                  const TA_Timestamp  *end,
                                  TA_Field             fieldToAlloc,
                                  TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_PrivateCSIHandle *privateHandle;
   MasterListRecord *record;

   SingleContractDataDayCore *dataList;
   int nbData;
   int i, index;
   int year,month,day,date,tmpInt;

   TA_Timestamp *timestamp;
   TA_Real *open, *high, *low, *close;
   TA_Integer *vol, *oi;
   
   TA_TRACE_BEGIN( TA_CSI_GetHistoryData );

   (void)start;
   (void)end;

   TA_ASSERT( handle != NULL );

   privateHandle = (TA_PrivateCSIHandle *)handle->opaqueData;
   TA_ASSERT( privateHandle != NULL );

   TA_ASSERT( paramForAddData != NULL );
   TA_ASSERT( categoryHandle != NULL );
   TA_ASSERT( symbolHandle != NULL );

   record = (MasterListRecord *)symbolHandle->opaqueData;

   TA_ASSERT( record != NULL );

   open = high = low = close = NULL;
   vol = oi = NULL;

   /* If the requested period is too precise for the
    * period that can be provided by this data source,
    * simply return without error.
    * Since no data has been added, the TA-LIB will ignore
    * this data source.
    */
   if( period < record->Period )
   {
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* Calculate the "offset" of the record in the main index. 
    * This is the "fileNumber" that CSI use.
    */
   index = ((const char *)record - (const char *)privateHandle->index)/sizeof(MasterListRecord);
   
   /* Get the data from the file. */
   dataList = NULL;
   switch( privateHandle->param->id )
   {
   case TA_CSI:
      retCode = ReadCSIData( TA_StringToChar(privateHandle->param->location), index, -1, &dataList, &nbData );
      break;
   case TA_CSIM:
      retCode = ReadCSIMData( TA_StringToChar(privateHandle->param->location), index, -1, &dataList, &nbData );
      break;
   default:
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(164) );
   }

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( dataList != NULL );

   /* Convert each field of data into arrays. */
   timestamp = (TA_Timestamp *)TA_Malloc(sizeof(TA_Timestamp)*nbData);
   if( !timestamp )
   {
      TA_Free( dataList );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   for( i=0; i < nbData; i++ )
   {
      TA_SetTime( 0, 0, 0, &timestamp[i] );
      date   = dataList[i].date;
      day    = date%100;
      tmpInt = date%10000;
      year   = (date - tmpInt)/10000;
      month  = (tmpInt - day)/100;
      TA_SetDate( year, month, day, &timestamp[i] );
   }

   #define PROCESS_ARRAY(type,uc,lc) \
   { \
      if( (fieldToAlloc == TA_ALL) || (fieldToAlloc & TA_##uc) ) \
      { \
         lc = (type *)TA_Malloc(sizeof(type)*nbData); \
         if( !lc ) \
         { \
            FREE_IF_NOT_NULL( timestamp ); \
            FREE_IF_NOT_NULL( open ); \
            FREE_IF_NOT_NULL( high ); \
            FREE_IF_NOT_NULL( low ); \
            FREE_IF_NOT_NULL( close ); \
            FREE_IF_NOT_NULL( vol ); \
            FREE_IF_NOT_NULL( oi ); \
            TA_Free( dataList ); \
            TA_TRACE_RETURN( TA_ALLOC_ERR ); \
         } \
         for( i=0; i < nbData; i++ ) \
            lc[i] = dataList[i].lc; \
      } \
   }

   PROCESS_ARRAY( TA_Real, OPEN,  open  );
   PROCESS_ARRAY( TA_Real, HIGH,  high  );
   PROCESS_ARRAY( TA_Real, LOW,   low   );
   PROCESS_ARRAY( TA_Real, CLOSE, close );

   PROCESS_ARRAY( TA_Integer, VOLUME, vol );
   PROCESS_ARRAY( TA_Integer, OPENINTEREST, oi );

   TA_Free( dataList );

   retCode = TA_HistoryAddData( paramForAddData, nbData,
                                record->Period, timestamp,
                                open, high, low, close,
                                vol, oi );

   TA_TRACE_RETURN( retCode );
}

/* The code for CSIM format is practically the same as for CSI.
 * The _CSI_ functions will handle the difference by looking
 * at the "id" TA_CSI vs TA_CSIM.
 */
TA_RetCode TA_CSIM_InitializeSourceDriver( void )
{
   TA_PROLOG
   TA_TRACE_BEGIN( TA_CSIM_InitializeSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSIM_ShutdownSourceDriver( void )
{
   TA_PROLOG
   TA_TRACE_BEGIN( TA_CSIM_ShutdownSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSIM_GetParameters( TA_DataSourceParameters *param )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_GetParameters );

   retCode = TA_CSI_GetParameters( param );
   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_CSIM_OpenSource( const TA_AddDataSourceParamPriv *param,
                              TA_DataSourceHandle **handle )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_OpenSource );

   retCode = TA_CSI_OpenSource( param, handle );
   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_CSIM_CloseSource( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_CloseSource );

   retCode = TA_CSI_CloseSource( handle );
   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_CSIM_GetFirstCategoryHandle( TA_DataSourceHandle *handle,
                                          TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_GetFirstCategoryHandle );

   retCode = TA_CSI_GetFirstCategoryHandle( handle, categoryHandle );
   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_CSIM_GetFirstSymbolHandle( TA_DataSourceHandle *handle,
                                        TA_CategoryHandle   *categoryHandle,
                                        TA_SymbolHandle     *symbolHandle )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_GetFirstSymbolHandle );

   retCode = TA_CSI_GetFirstSymbolHandle( handle, categoryHandle, symbolHandle );
   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_CSIM_GetNextSymbolHandle( TA_DataSourceHandle *handle,
                                       TA_CategoryHandle   *categoryHandle,
                                       TA_SymbolHandle     *symbolHandle,
                                       unsigned int index )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_GetNextSymbolHandle );

   retCode = TA_CSI_GetNextSymbolHandle( handle, categoryHandle, symbolHandle, index );
   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_CSIM_GetHistoryData( TA_DataSourceHandle *handle,
                                  TA_CategoryHandle   *categoryHandle,
                                  TA_SymbolHandle     *symbolHandle,
                                  TA_Period            period,
                                  const TA_Timestamp  *start,
                                  const TA_Timestamp  *end,
                                  TA_Field             fieldToAlloc,
                                  TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_TRACE_BEGIN( TA_CSIM_GetHistoryData );

   retCode = TA_CSI_GetHistoryData( handle, categoryHandle, symbolHandle,
                                    period, start, end, fieldToAlloc, paramForAddData );
   TA_TRACE_RETURN( retCode );                                 
}

/**** Local functions definitions.     ****/
/* None */

