/* TA-LIB Copyright (c) 1999-2000, Mario Fortier
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
 *  110199 MF   First version.
 *
 */

/* Description:
 *    This is the entry points of the data source driver for the ASCII format.
 *
 *    It provides ALL the functions needed by the "TA_DataSourceDriver"
 *    structure (see ta_source.h).
 */

/**** Headers ****/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "ta_source.h"
#include "ta_ascii.h"
#include "ta_common.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_list.h"
#include "ta_data.h"
#include "ta_system.h"
#include "ta_ascii_handle.h"
#include "ta_fileindex.h"
#include "ta_readop.h"
#include "ta_global.h"

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
TA_RetCode TA_ASCII_InitializeSourceDriver( TA_Libc *libHandle )
{
   TA_PROLOG;
   TA_TRACE_BEGIN( libHandle, TA_ASCII_InitializeSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_ShutdownSourceDriver( TA_Libc *libHandle )
{
   TA_PROLOG;
   TA_TRACE_BEGIN( libHandle, TA_ASCII_ShutdownSourceDriver );

    /* Nothing to do for the time being. */
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_GetParameters( TA_Libc *libHandle, TA_DataSourceParameters *param )
{
   TA_PROLOG;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_GetParameters );

   memset( param, 0, sizeof( TA_DataSourceParameters ) );

   /* For the time being, the ASCII driver is a read-only source. */
   param->supportUpdateIndex  = 0;
   param->supportUpdateSymbol = 0;

    /* No support for realtime feedback with local ASCII database. */
   param->supportCallback = 0;

   /* Assume it is a fast local data base. */
   param->slowAccess = 0;

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_ASCII_OpenSource( TA_Libc *libHandle,
                                const TA_AddDataSourceParamPriv *param,
                                TA_DataSourceHandle **handle )
{
   TA_PROLOG;
   TA_DataSourceHandle *tmpHandle;
   TA_PrivateAsciiHandle *privData;
   TA_RetCode retCode;
   TA_StringCache *stringCache;

   *handle = NULL;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_OpenSource );

   stringCache = TA_GetGlobalStringCache( libHandle );

   /* Verify that the requested functionality is supported or not. */
   if( (param->flags & TA_ENABLE_UPDATE_INDEX) ||
       (param->flags & TA_ENABLE_UPDATE_SYMBOL) )
   {
      TA_TRACE_RETURN( TA_NOT_SUPPORTED );
   }

   /* 'info' and 'location' are mandatory. */
   if( (!param->info) || (!param->location) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   /* Allocate and initialize the handle. This function will also allocate the
    * private handle (opaque data).
    */
   tmpHandle = TA_ASCII_DataSourceHandleAlloc(libHandle,param);

   if( tmpHandle == NULL )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   privData = (TA_PrivateAsciiHandle *)(tmpHandle->opaqueData);

   /* Keep a pointer on the TA_AddDataSourcePriv parameters.
    * This pointer (and its content) is guaranteed to be good
    * until CloseSource gets called.
    */
   privData->param = param;

   /* Now build the TA_FileIndex. */
   retCode = TA_ASCII_BuildFileIndex( tmpHandle );

   if( retCode != TA_SUCCESS )
   {
      TA_ASCII_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( retCode );
   }

   /* Set the total number of distinct category. */
   tmpHandle->nbCategory = TA_FileIndexNbCategory( privData->theFileIndex );

   /* Build the array of operation to perform for reading the
    * ASCII file.
    */
   retCode = TA_ASCII_BuildReadOpInfo( tmpHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_ASCII_DataSourceHandleFree( tmpHandle );
      TA_TRACE_RETURN( retCode );
   }

   *handle = tmpHandle;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_CloseSource( TA_Libc *libHandle,
                                 TA_DataSourceHandle *handle )
{
   TA_PROLOG;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_CloseSource );

   /* Free all ressource used by this handle. */
   if( handle )
      TA_ASCII_DataSourceHandleFree( handle );

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_GetFirstCategoryHandle( TA_Libc *libHandle,
                                            TA_DataSourceHandle *handle,
                                            TA_CategoryHandle   *categoryHandle )
{
   TA_PROLOG;
   TA_PrivateAsciiHandle *privData;
   TA_FileIndex     *fileIndex;
   TA_String        *string;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_GetFirstCategoryHandle );

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(49) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(50) );
   }

   string = TA_FileIndexFirstCategory( fileIndex );

   if( !string )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(51) ); /* At least one category must exist. */
   }

   /* Set the categoryHandle. */
   categoryHandle->string = string;
   categoryHandle->nbSymbol = TA_FileIndexNbSymbol( fileIndex );
   categoryHandle->opaqueData = categoryHandle;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_GetNextCategoryHandle( TA_Libc *libHandle,
                                           TA_DataSourceHandle *handle,
                                           TA_CategoryHandle   *categoryHandle,
                                           unsigned int index )
{
   TA_PROLOG;

   TA_PrivateAsciiHandle *privData;
   TA_FileIndex     *fileIndex;
   TA_String        *string;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_GetNextCategoryHandle );

   (void)index; /* Get ride of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(52) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(53) );
   }

   /* Get the next category from the fileIdnex. */
   string = TA_FileIndexNextCategory( fileIndex );

   if( !string )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Set the categoryHandle. */
   categoryHandle->string = string;
   categoryHandle->nbSymbol = TA_FileIndexNbSymbol( fileIndex );
   categoryHandle->opaqueData = NULL; /* Not needed... */

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_GetFirstSymbolHandle( TA_Libc *libHandle,
                                          TA_DataSourceHandle *handle,
                                          TA_CategoryHandle   *categoryHandle,
                                          TA_SymbolHandle     *symbolHandle )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_PrivateAsciiHandle *privData;
   TA_FileIndex *fileIndex;
   TA_FileInfo *sourceInfo;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_GetFirstSymbolHandle );

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(54) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex || !categoryHandle->string )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(55) );
   }

   /* Make sure the current category is the one requested. */
   retCode = TA_FileIndexSelectCategory( fileIndex, categoryHandle->string );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(56) );
   }

   /* Get the first symbol in this category. */
   sourceInfo = TA_FileIndexFirstSymbol( fileIndex );

   if( !sourceInfo )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Parano sanity check: the string of the requested categoryHandle should
    * correspond to the string of this sourceInfo.
    */
   if( strcmp( TA_StringToChar( TA_FileInfoCategory( libHandle, sourceInfo ) ),
               TA_StringToChar( categoryHandle->string ) ) != 0 )
   {
      TA_FATAL( libHandle, NULL, 0, 0 );
   }

   /* Set the symbolHandle. */
   symbolHandle->string = TA_FileInfoSymbol( libHandle, sourceInfo );
   symbolHandle->opaqueData = sourceInfo;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_GetNextSymbolHandle( TA_Libc *libHandle,
                                         TA_DataSourceHandle *handle,
                                         TA_CategoryHandle   *categoryHandle,
                                         TA_SymbolHandle     *symbolHandle,
                                         unsigned int index )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_PrivateAsciiHandle *privData;
   TA_FileIndex *fileIndex;
   TA_FileInfo *sourceInfo;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_GetNextSymbolHandle );

   (void)index; /* Get ride of compiler warnings. */

   if( (handle == NULL) || (categoryHandle == NULL) || (symbolHandle == NULL) )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   privData = (TA_PrivateAsciiHandle *)(handle->opaqueData);

   if( !privData )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(57) );
   }

   fileIndex = privData->theFileIndex;

   if( !fileIndex || !categoryHandle->string )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(58) );
   }

   /* Make sure the current category is the one requested. */
   retCode = TA_FileIndexSelectCategory( fileIndex, categoryHandle->string );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(59) );
   }

   /* Get the next symbol in this category. */
   sourceInfo = TA_FileIndexNextSymbol( fileIndex );

   if( !sourceInfo )
   {
      TA_TRACE_RETURN( TA_END_OF_INDEX );
   }

   /* Parano sanity check: the string of the requested categoryHandle should
    * correspond to the string of this sourceInfo.
    */
   if( strcmp( TA_StringToChar( TA_FileInfoCategory( libHandle, sourceInfo ) ),
               TA_StringToChar( categoryHandle->string ) ) != 0 )
   {
      TA_FATAL( libHandle, NULL, 0, 0 );
   }

   /* Set the symbolHandle. */
   symbolHandle->string = TA_FileInfoSymbol( libHandle, sourceInfo );
   symbolHandle->opaqueData = sourceInfo;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_GetHistoryData( TA_Libc *libHandle,
                                    TA_DataSourceHandle *handle,
                                    TA_CategoryHandle   *categoryHandle,
                                    TA_SymbolHandle     *symbolHandle,
                                    TA_Period            period,
                                    const TA_Timestamp  *start,
                                    const TA_Timestamp  *end,
                                    TA_Field             fieldToAlloc,
                                    TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG;
   const char *path;
   TA_FileInfo *sourceInfo;
   TA_FileHandle *fileHandle;
   TA_RetCode retCode;
   TA_PrivateAsciiHandle *privateHandle;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_GetHistoryData );

   TA_ASSERT( libHandle, handle != NULL );

   privateHandle = (TA_PrivateAsciiHandle *)handle->opaqueData;
   TA_ASSERT( libHandle, privateHandle != NULL );

   TA_ASSERT( libHandle, paramForAddData != NULL );
   TA_ASSERT( libHandle, categoryHandle != NULL );
   TA_ASSERT( libHandle, symbolHandle != NULL );

   sourceInfo = (TA_FileInfo *)symbolHandle->opaqueData;

   TA_ASSERT( libHandle, sourceInfo != NULL );

   /* If the requested period is too precise for the
    * period that can be provided by this data source,
    * simply return without error.
    * Since no data has been added, the TA-LIB will ignore
    * this data source.
    */
   if( period < privateHandle->readOpInfo->period )
   {
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* If all fields are requested, provides all the fields
    * that were specified when this data source was added.
    */
   if( fieldToAlloc == TA_ALL )
      fieldToAlloc = privateHandle->readOpInfo->fieldProvided;

   /* Get the path of the file. */
   path = TA_FileInfoPath( libHandle, sourceInfo );

   if( !path )
   {
      TA_FATAL( libHandle, "Building path failed", 0, 0 );
   }

   /* Open the file for sequential read only (optimized read) */
   retCode = TA_FileSeqOpen( libHandle, path, &fileHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Optimize the readOp according to the requested field. */
   retCode = TA_ReadOp_Optimize( libHandle,
                                 privateHandle->readOpInfo,
                                 privateHandle->readOpInfo->period, fieldToAlloc );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Read all the price bar. */
   retCode = TA_ReadOp_Do( libHandle, fileHandle,                           
                           privateHandle->readOpInfo,
                           privateHandle->readOpInfo->period,
                           start, end, 200,
                           fieldToAlloc, paramForAddData, NULL );

   if( retCode != TA_SUCCESS )
   {
      TA_FileSeqClose( libHandle, fileHandle );
      TA_TRACE_RETURN( retCode );
   }

   /* Read completed... clean-up and return. */
   retCode = TA_FileSeqClose( libHandle, fileHandle );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
/* None */

