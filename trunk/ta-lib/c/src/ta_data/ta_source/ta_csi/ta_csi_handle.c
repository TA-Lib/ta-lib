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
#include "ta_csi_handle.h"
#include "ta_global.h"
#include "ta_string.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_PrivateCSIHandle *allocPrivateHandle( void );
static TA_RetCode freePrivateHandle( TA_PrivateCSIHandle *privateHandle );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

TA_DataSourceHandle *TA_CSI_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param )
{
   TA_DataSourceHandle *handle;
   TA_PrivateCSIHandle *privateHandle;

   TA_ASSERT_RET( param != NULL, (TA_DataSourceHandle *)NULL );
      
   handle = (TA_DataSourceHandle *)TA_Malloc(sizeof( TA_DataSourceHandle ));
   if( !handle )
      return (TA_DataSourceHandle *)NULL;

   memset( handle, 0, sizeof(handle) );

   /* Allocate the opaque data. */
   handle->opaqueData = allocPrivateHandle();
   if( !handle->opaqueData )
   {
      TA_CSI_DataSourceHandleFree( handle );
      return (TA_DataSourceHandle *)NULL;
   }

   /* Keep a pointer on the TA_AddDataSourcePriv parameters.
    * This pointer (and its content) is guaranteed to be good
    * until CloseSource gets called.
    */
   privateHandle = (TA_PrivateCSIHandle *)handle->opaqueData;   
   privateHandle->param = param;

   return handle;
}

TA_RetCode TA_CSI_DataSourceHandleFree( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_PrivateCSIHandle *privateHandle;

   TA_TRACE_BEGIN( TA_CSI_DataSourceHandleFree );
   TA_ASSERT( handle != NULL );

   privateHandle = (TA_PrivateCSIHandle *)handle->opaqueData;

   if( freePrivateHandle( (TA_PrivateCSIHandle *)handle->opaqueData ) != TA_SUCCESS )
   {
      TA_FATAL(  NULL, handle, 0 );
   }

   TA_Free( handle );

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_CSI_BuildIndex( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_PrivateCSIHandle *privateHandle;
   TA_String *stringTmp;   
   TA_String **indexString;
   TA_StringCache *stringCache;
   const char *directory;

   struct MasterListRecordType *outMasterList;
   int outNRec;
   TA_RetCode retCode;
   int i;

   TA_TRACE_BEGIN( TA_CSI_BuildIndex );

   TA_ASSERT( handle != NULL );
   privateHandle = (TA_PrivateCSIHandle *)handle->opaqueData;

   TA_ASSERT( privateHandle != NULL );
   TA_ASSERT( privateHandle->param != NULL );
   TA_ASSERT( privateHandle->param->category != NULL );
   TA_ASSERT( privateHandle->param->location != NULL );

   /* De-allocate potentialy already existing file index. */
   if( privateHandle->index != NULL )
   {
      // An error for now, will be implemented correctly later (as needed).
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Get the directory, if none provided, just use current one. */
   directory = ".";
   stringTmp = privateHandle->param->location;
   if( stringTmp )
   {
      directory = TA_StringToChar(stringTmp);
      if( directory[0] == '\0' )
         directory = ".";
   }
  
   /* Allocate new file index. */   
   outMasterList = NULL;
   outNRec = 0;
   switch( privateHandle->param->id )
   {
   case TA_CSI:  
      retCode = ReadCSIMaster(directory, &outMasterList, &outNRec);
      break;
   case TA_CSIM:  
      retCode = ReadCSIMMaster(directory, &outMasterList, &outNRec);
      break;
   default:
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(165) );
   }

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( outMasterList != NULL );

   /* Build an index of TA_String */
   if( outNRec > 0 )
   {
      indexString = (TA_String **) TA_Malloc( sizeof( TA_String * ) * outNRec );;
      
      if( !indexString )
      {
         TA_Free( outMasterList );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      stringCache = TA_GetGlobalStringCache();
      for ( i=0; i < outNRec; i++ )
      {
         stringTmp = TA_StringAlloc_ULong( stringCache, outMasterList[i].csinum );
         if( !stringTmp )
         {
            while( --i >= 0 )
            {
               TA_StringFree( stringCache, indexString[i] );
            }
            TA_Free( outMasterList );
            TA_Free( (void *)indexString );
            TA_TRACE_RETURN( TA_ALLOC_ERR );
         }
         indexString[i] = stringTmp;
      }
      privateHandle->indexString = indexString;
   }

   /* Success */
   privateHandle->index     = outMasterList;
   privateHandle->indexSize = outNRec;

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/

static TA_PrivateCSIHandle *allocPrivateHandle( void  )
{
   TA_PrivateCSIHandle *privateHandle;
   TA_String *stringTmp;

   privateHandle = (TA_PrivateCSIHandle *)TA_Malloc( sizeof( TA_PrivateCSIHandle ) );
   if( !privateHandle )
      return NULL;

   memset( privateHandle, 0, sizeof( TA_PrivateCSIHandle ) );

   stringTmp = TA_StringAlloc(TA_GetGlobalStringCache(),"CSI_ID");
   if( !stringTmp )
   {
      TA_Free( privateHandle );
      return (TA_PrivateCSIHandle *)NULL;
   }
   
   privateHandle->category = stringTmp;

   return privateHandle;
}

static TA_RetCode freePrivateHandle( TA_PrivateCSIHandle *privateHandle )
{
   int i;
   TA_StringCache *stringCache;

   if( privateHandle )
   {
      FREE_IF_NOT_NULL( privateHandle->index );
      stringCache = TA_GetGlobalStringCache();
      TA_StringFree( stringCache, privateHandle->category );
      if( privateHandle->indexString )
      {
         for( i=0; i < privateHandle->indexSize; i++ )
            TA_StringFree( stringCache, privateHandle->indexString[i] );

         TA_Free( (void *)privateHandle->indexString);
      }
      TA_Free( privateHandle );
   }

   return TA_SUCCESS;
}
