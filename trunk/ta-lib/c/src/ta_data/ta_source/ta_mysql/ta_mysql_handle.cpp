/* TA-LIB MySQL Data Source Driver Copyright (c) 2003, Pawel A. Konieczny
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
 *  PK       Pawel Konieczny
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  101703 PK   First version.
 *
 */

/* Description:
 *    Allows to allocate/de-allocate TA_DataSourceHandle structure.
 */

/**** Headers ****/
extern "C" {
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_fileindex.h"
#include "ta_token.h"
#include "ta_global.h"
}
#include "ta_mysql_handle.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_PrivateMySQLHandle *allocPrivateHandle( void );
static TA_RetCode freePrivateHandle( TA_PrivateMySQLHandle *privateHandle );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

TA_DataSourceHandle *TA_MYSQL_DataSourceHandleAlloc( const TA_AddDataSourceParamPriv *param )
{
   TA_DataSourceHandle *handle;
   TA_PrivateMySQLHandle *privateHandle;

   TA_ASSERT_RET( param != NULL, (TA_DataSourceHandle *)NULL );
      
   handle = (TA_DataSourceHandle *)TA_Malloc(sizeof( TA_DataSourceHandle ));
   if( !handle )
      return (TA_DataSourceHandle *)NULL;

   /* Initialized fields. */
   handle->nbCategory = 0;

   /* Allocate the opaque data. */
   handle->opaqueData = allocPrivateHandle();
   if( !handle->opaqueData )
   {
      TA_MYSQL_DataSourceHandleFree( handle );
      return (TA_DataSourceHandle *)NULL;
   }

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;
   privateHandle->param = param;

   return handle;
}

TA_RetCode TA_MYSQL_DataSourceHandleFree( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_PrivateMySQLHandle *privateHandle;

   if( !handle )
      return (TA_RetCode)TA_INTERNAL_ERROR(60);

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;

   TA_TRACE_BEGIN(  TA_MYSQL_DataSourceHandleFree );

   if( handle )
   {
      if( freePrivateHandle( (TA_PrivateMySQLHandle *)handle->opaqueData ) != TA_SUCCESS )
      {
         TA_FATAL(  NULL, handle, 0 );
      }

      TA_Free( handle );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_MYSQL_BuildFileIndex( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_FileIndex *newIndex;
   TA_PrivateMySQLHandle *privateHandle;

   if( !handle )
      return (TA_RetCode)TA_INTERNAL_ERROR(61);

   privateHandle = (TA_PrivateMySQLHandle *)handle->opaqueData;

   TA_TRACE_BEGIN(  TA_MYSQL_BuildFileIndex );

   TA_ASSERT( privateHandle != NULL );
   TA_ASSERT( privateHandle->param != NULL );
   TA_ASSERT( privateHandle->param->category != NULL );
   TA_ASSERT( privateHandle->param->location != NULL );

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
   retCode = TA_FileIndexAlloc( privateHandle->param->location,
                                privateHandle->param->category,
                                privateHandle->param->country,
                                privateHandle->param->exchange,
                                privateHandle->param->type,
                                &newIndex );

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( newIndex != NULL );

   privateHandle->theFileIndex = newIndex;

   TA_TRACE_RETURN( TA_SUCCESS );
}


/**** Local functions definitions.     ****/

static TA_PrivateMySQLHandle *allocPrivateHandle( void  )
{
   TA_PrivateMySQLHandle *privateHandle;

   privateHandle = (TA_PrivateMySQLHandle *)TA_Malloc( sizeof( TA_PrivateMySQLHandle ) );
   if( !privateHandle )
      return NULL;

   memset( privateHandle, 0, sizeof( TA_PrivateMySQLHandle ) );

   return privateHandle;
}

static TA_RetCode freePrivateHandle( TA_PrivateMySQLHandle *privateHandle )
{
   if( privateHandle )
   {
      if( privateHandle->theFileIndex )
         TA_FileIndexFree( privateHandle->theFileIndex );

      if( privateHandle->con )
         delete privateHandle->con;

      TA_Free(  privateHandle );
   }

   return TA_SUCCESS;
}
