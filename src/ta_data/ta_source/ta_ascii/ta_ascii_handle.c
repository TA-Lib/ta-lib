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
 *    Allows to allocate/de-allocate TA_DataSourceHandle structure.
 */

/**** Headers ****/
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_ascii_handle.h"
#include "ta_fileindex.h"
#include "ta_token.h"
#include "ta_global.h"
#include "ta_readop.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_PrivateAsciiHandle *allocPrivateHandle( TA_Libc *libHandle );
static TA_RetCode freePrivateHandle( TA_Libc *libHandle, TA_PrivateAsciiHandle *privateHandle );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

TA_DataSourceHandle *TA_ASCII_DataSourceHandleAlloc( TA_Libc *libHandle,
                                                     const TA_AddDataSourceParamPriv *param )
{
   TA_DataSourceHandle *handle;
   TA_PrivateAsciiHandle *privateHandle;

   TA_ASSERT_RET( libHandle, libHandle != NULL, (TA_DataSourceHandle *)NULL );
   TA_ASSERT_RET( libHandle, param != NULL, (TA_DataSourceHandle *)NULL );
      
   handle = (TA_DataSourceHandle *)TA_Malloc(libHandle,sizeof( TA_DataSourceHandle ));
   if( !handle )
      return (TA_DataSourceHandle *)NULL;

   /* Initialized fields. */
   handle->nbCategory = 0;

   /* Allocate the opaque data. */
   handle->opaqueData = allocPrivateHandle(libHandle);
   if( !handle->opaqueData )
   {
      TA_ASCII_DataSourceHandleFree( handle );
      return (TA_DataSourceHandle *)NULL;
   }

   privateHandle = (TA_PrivateAsciiHandle *)handle->opaqueData;
   privateHandle->param = param;

   return handle;
}

TA_RetCode TA_ASCII_DataSourceHandleFree( TA_DataSourceHandle *handle )
{
   TA_PROLOG;
   TA_PrivateAsciiHandle *privateHandle;
   TA_Libc *libHandle;

   if( !handle )
      return TA_UNKNOWN_ERR;

   privateHandle = (TA_PrivateAsciiHandle *)handle->opaqueData;
   libHandle = privateHandle->libHandle;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_DataSourceHandleFree );

   if( handle )
   {
      if( freePrivateHandle( libHandle, (TA_PrivateAsciiHandle *)handle->opaqueData ) != TA_SUCCESS )
      {
         TA_FATAL( libHandle, NULL, handle, 0 );
      }

      TA_Free( libHandle, handle );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_BuildFileIndex( TA_DataSourceHandle *handle )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_FileIndex *newIndex;
   TA_PrivateAsciiHandle *privateHandle;
   TA_Libc *libHandle;

   if( !handle )
      return TA_UNKNOWN_ERR;

   privateHandle = (TA_PrivateAsciiHandle *)handle->opaqueData;
   libHandle = privateHandle->libHandle;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_BuildFileIndex );

   TA_ASSERT( libHandle, privateHandle != NULL );
   TA_ASSERT( libHandle, privateHandle->param != NULL );
   TA_ASSERT( libHandle, privateHandle->param->category != NULL );
   TA_ASSERT( libHandle, privateHandle->param->location != NULL );

   /* De-allocate potentialy already existing file index. */
   if( privateHandle->theFileIndex != NULL )
   {
      retCode = TA_FileIndexFree( privateHandle->theFileIndex );
      privateHandle->theFileIndex = NULL;
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
   }

   /* Allocate new file index. */
   retCode = TA_FileIndexAlloc( libHandle,
                                privateHandle->param->location,
                                privateHandle->param->category,
                                privateHandle->param->country,
                                privateHandle->param->exchange,
                                privateHandle->param->type,
                                &newIndex );

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( libHandle, newIndex != NULL );

   privateHandle->theFileIndex = newIndex;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ASCII_BuildReadOpInfo( TA_DataSourceHandle *handle )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_PrivateAsciiHandle *privateHandle;
   TA_Libc *libHandle;

   if( !handle )
      return TA_UNKNOWN_ERR;

   privateHandle = (TA_PrivateAsciiHandle *)handle->opaqueData;
   libHandle = privateHandle->libHandle;

   TA_TRACE_BEGIN( libHandle, TA_ASCII_BuildFileIndex );

   TA_ASSERT( libHandle, privateHandle != NULL );
   TA_ASSERT( libHandle, privateHandle->param != NULL );
   TA_ASSERT( libHandle, privateHandle->param->category != NULL );
   TA_ASSERT( libHandle, privateHandle->param->location != NULL );

   
   retCode = TA_ReadOpInfoAlloc( libHandle,
                                 TA_StringToChar(privateHandle->param->info),
                                 &privateHandle->readOpInfo );

   TA_TRACE_RETURN( retCode );
}

/**** Local functions definitions.     ****/

static TA_PrivateAsciiHandle *allocPrivateHandle( TA_Libc *libHandle )
{
   TA_PrivateAsciiHandle *privateHandle;

   privateHandle = (TA_PrivateAsciiHandle *)TA_Malloc( libHandle, sizeof( TA_PrivateAsciiHandle ) );
   if( !privateHandle )
      return NULL;

   memset( privateHandle, 0, sizeof( TA_PrivateAsciiHandle ) );

   privateHandle->libHandle = libHandle;

   return privateHandle;
}

static TA_RetCode freePrivateHandle( TA_Libc *libHandle, TA_PrivateAsciiHandle *privateHandle )
{
   if( privateHandle )
   {
      if( privateHandle->theFileIndex )
         TA_FileIndexFree( privateHandle->theFileIndex );

      if( privateHandle->readOpInfo )
         TA_ReadOpInfoFree( privateHandle->readOpInfo );

      TA_Free( libHandle, privateHandle );
   }

   return TA_SUCCESS;
}
