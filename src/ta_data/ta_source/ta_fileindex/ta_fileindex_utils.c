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
 *  102900 MF   First version.
 *
 */

/* Description:
 *    Provides private utility to the TA_FileIndex modules.
 */

/**** Headers ****/
#include <stddef.h>
#include "ta_fileindex_priv.h"
#include "ta_trace.h"

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
TA_RetCode TA_FileIndexMoveToDepth( TA_FileIndexPriv *data, unsigned int depth )
{
   while( depth != data->curTokenDepth )
   {
      if( depth > data->curTokenDepth )
         TA_FileIndexMoveToNextToken( data );
      else
         TA_FileIndexMoveToPrevToken( data );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_FileIndexMoveToNextToken( TA_FileIndexPriv *data )
{
   TA_PROLOG;
   TA_Libc *libHandle;

   if( !data )
      return TA_INTERNAL_ERROR(86);


   libHandle = data->libHandle;
   TA_TRACE_BEGIN( libHandle, TA_FileIndexMoveToNextToken );

   if( data->curToken == NULL )
   {
      /* First time this function get called for this 'data' */
      data->curTokenDepth = 0;
      data->curDirectionForward = 1;
      data->curToken = (TA_TokenInfo *)TA_ListAccessHead( data->listLocationToken );
      data->prevToken = NULL;
      data->nextToken = (TA_TokenInfo *)TA_ListAccessNext( data->listLocationToken );

      if( !data->curToken || !data->nextToken )
      {
         TA_FATAL( data->libHandle, NULL, data->curToken, data->nextToken );
      }
   }
   else if( data->nextToken == NULL )
   {
      /* Can't go further, simply return. */
      TA_TRACE_RETURN( TA_SUCCESS );
   }
   else
   {
      if( data->curDirectionForward == 0 )
      {
         if( data->prevToken != NULL )
            TA_ListAccessNext( data->listLocationToken );
         else
            TA_ListAccessHead( data->listLocationToken );

         TA_ListAccessNext( data->listLocationToken );
         data->curDirectionForward = 1;
      }

      data->prevToken = data->curToken;
      data->curToken = data->nextToken;
      data->nextToken = (TA_TokenInfo *)TA_ListAccessNext( data->listLocationToken );

      /* Parano test: Should never happen since the code assume that
       * the 'listLocationToken' is always terminated by TA_END.
       */
      if( !data->curToken )
      {
         TA_FATAL( data->libHandle, NULL, data->curToken->id, data->curTokenDepth );
      }
   }

   data->curTokenDepth++;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_FileIndexMoveToPrevToken( TA_FileIndexPriv *data )
{
   TA_PROLOG;
   TA_Libc *libHandle;
   TA_RetCode retCode;

   if( !data )
      return TA_INTERNAL_ERROR(87);

   libHandle = data->libHandle;
   TA_TRACE_BEGIN( libHandle, TA_FileIndexMoveToPrevToken );

   if( data->curToken == NULL )
   {
      /* Iterator variable not initialize. Simply position everything on the
       * first token.
       */
      retCode = TA_FileIndexMoveToNextToken( data );
      TA_TRACE_RETURN( retCode ); 
   }
   else if( data->prevToken == NULL )
   {
      /* Can't go further back. Simply return. */
      TA_TRACE_RETURN( TA_SUCCESS );
   }
   else
   {
      if( data->curDirectionForward == 1 )
      {
         if( data->nextToken != NULL )
            TA_ListAccessPrev( data->listLocationToken );
         else
            TA_ListAccessTail( data->listLocationToken );

         TA_ListAccessPrev( data->listLocationToken );
         data->curDirectionForward = 0;
      }

      data->nextToken = data->curToken;
      data->curToken = data->prevToken;
      data->prevToken = (TA_TokenInfo *)TA_ListAccessPrev( data->listLocationToken );

      /* Parano test: Should never happen since the code assume that
       * the 'listLocationToken' is always terminated by TA_END.
       */
      if( !data->curToken )
      {
         TA_FATAL( data->libHandle, NULL, data->curToken->id, data->curTokenDepth );
      }
   }
   data->curTokenDepth--;

   TA_TRACE_RETURN( TA_SUCCESS );
}

unsigned int TA_FileIndexIdentifyFileDepth( TA_FileIndexPriv *data )
{
   TA_List *listLocationToken;
   TA_TokenInfo *info;
   unsigned int latestPathPortionDepth;
   unsigned int currentDepth;

   /* Identify the last portion of the path where
    * the directory path is all resolved.
    *
    * It is imperative that the last token be TA_TOK_END.
    */
   listLocationToken = data->listLocationToken;

   info = TA_ListAccessHead( listLocationToken );
   latestPathPortionDepth = 1;
   currentDepth = 0;
   while( info )
   {
      currentDepth++;

      if( info->id == TA_TOK_END )
         return latestPathPortionDepth;
      if( info->id == TA_TOK_SEP )
         latestPathPortionDepth = currentDepth;

      info = TA_ListAccessNext( listLocationToken );
   }

   /* Should never get here. */
   TA_FATAL_RET( data->libHandle, NULL, currentDepth, latestPathPortionDepth, 0 );
}

/**** Local functions definitions.     ****/
/* None */



