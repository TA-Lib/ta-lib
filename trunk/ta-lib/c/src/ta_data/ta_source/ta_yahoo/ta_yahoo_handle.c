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
#include "ta_yahoo_handle.h"
#include "ta_yahoo_idx.h"
#include "ta_fileindex.h"
#include "ta_token.h"
#include "ta_yahoo_priv.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_PrivateYahooHandle *allocPrivateHandle( void );
static TA_RetCode freePrivateHandle( TA_PrivateYahooHandle *privateHandle );


/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

TA_DataSourceHandle *TA_YAHOO_DataSourceHandleAlloc( void )
{
   TA_DataSourceHandle *handle;
   TA_PrivateYahooHandle *privateHandle;

   handle = (TA_DataSourceHandle *)TA_Malloc(sizeof( TA_DataSourceHandle ));
   if( !handle )
      return NULL;

   memset( handle, 0, sizeof( TA_DataSourceHandle ) );

   /* Allocate the opaque data. */
   privateHandle = allocPrivateHandle();
   handle->opaqueData = privateHandle;
   if( !handle->opaqueData )
   {
      TA_Free(  handle );
      return NULL;
   }
    
   handle->nbCategory = 0;
   
   return handle;
}

TA_RetCode TA_YAHOO_DataSourceHandleFree( TA_DataSourceHandle *handle )
{
   TA_PROLOG
   TA_PrivateYahooHandle *privateHandle;

   privateHandle = (TA_PrivateYahooHandle *)handle->opaqueData;

   TA_TRACE_BEGIN( TA_YAHOO_DataSourceHandleFree );

   if( handle )
   {
      if( freePrivateHandle( (TA_PrivateYahooHandle *)handle->opaqueData ) != TA_SUCCESS )
      {
         TA_FATAL( NULL, handle, 0 );
      }

      TA_Free( handle );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
static TA_PrivateYahooHandle *allocPrivateHandle( void )
{
   TA_PrivateYahooHandle *privateHandle;
   TA_RetCode retCode;

   privateHandle = (TA_PrivateYahooHandle *)TA_Malloc( sizeof( TA_PrivateYahooHandle ) );
   if( !privateHandle )
      return NULL;

   memset( privateHandle, 0, sizeof( TA_PrivateYahooHandle ) );

   /* freePrivateHandle can be safely called from this point. */

   retCode = TA_ReadOpInfoAlloc( "[D][MMM][YY][O][H][L][C][V]",                               
                                 &privateHandle->readOp6Fields );
   if( retCode != TA_SUCCESS )
   {
      freePrivateHandle( privateHandle );
      return NULL;
   }

   retCode = TA_ReadOpInfoAlloc( "[D][MMM][YY][C]",
                                 &privateHandle->readOp2Fields );
   if( retCode != TA_SUCCESS )
   {
      freePrivateHandle( privateHandle );
      return NULL;
   }

   retCode = TA_ReadOpInfoAlloc( "[D][MMM][YY][O][H][L][C]",
                                 &privateHandle->readOp5Fields );
   if( retCode != TA_SUCCESS )
   {
      freePrivateHandle( privateHandle );
      return NULL;
   }

   return privateHandle;
}

static TA_RetCode freePrivateHandle( TA_PrivateYahooHandle *privateHandle )
{
   if( privateHandle )
   {
      if( privateHandle->index )
         TA_YahooIdxFree( privateHandle->index );

      if( privateHandle->readOp6Fields )
         TA_ReadOpInfoFree( privateHandle->readOp6Fields );

      if( privateHandle->readOp5Fields )
         TA_ReadOpInfoFree( privateHandle->readOp5Fields );

      if( privateHandle->readOp2Fields )
         TA_ReadOpInfoFree( privateHandle->readOp2Fields );

      TA_Free(  privateHandle );
   }

   return TA_SUCCESS;
}
