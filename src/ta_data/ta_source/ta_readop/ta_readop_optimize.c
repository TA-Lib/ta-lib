/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
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
 *  112400 MF   First version.
 *
 */

/* Description:
 *    Adapt readOp for faster execution. Mainly, indicates which operation can
 *    be skipped and determine the last operation.
 *    Optimization is becoming significant when the number of requested fields
 *    is smaller than the number of fields provided by the file. In these
 *    scenario, there is less "atof" and/or "atoi" conversion needed since the
 *    numeric string can simply be skip.
 *    Determining the last needed operations allow to skip the remaining of
 *    a line if we got already all the requested information.
 *
 *    The "optimization" is achieve by setting flags (bit) in the elements
 *    of the arrayReadOp. These bits indicates which fields can be skip and
 *    when to stop.
 */

/**** Headers ****/
#include "ta_readop.h"
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
TA_RetCode TA_ReadOp_Optimize( TA_ReadOpInfo *readOpInfo,
                               TA_Period      period,
                               TA_Field       fieldToAlloc )
{
   TA_PROLOG
   TA_ReadOp *lastOp;
   unsigned int tmpIdx;
   unsigned int i, nbReadOp;
   unsigned int flagSet;
   TA_Field field;
   TA_ReadOp *readOp;
    
   TA_TRACE_BEGIN(  TA_OptimizeReadOp );

   TA_ASSERT( fieldToAlloc != 0 );

   nbReadOp = readOpInfo->nbReadOp;
   readOp = readOpInfo->arrayReadOp;

   /* Skip all fields not requested. */
   for( i=0; i < nbReadOp; i++ )
   {
      if( TA_IS_PERMANENT_SKIP_SET(readOp[i]) )
      {
         TA_SET_SKIP_FLAG(readOp[i]);
      }
      else
      {
         TA_CLR_SKIP_FLAG(readOp[i]);
         field = TA_ReadOpToField( readOp[i] );

         if( field == TA_TIMESTAMP )
         {
            tmpIdx = TA_GET_IDX(readOp[i]);
            switch( period )
            {
            case TA_YEARLY:
               /* No need for anything shorter than year in the timestamp. */
               if( tmpIdx <= TA_MONTH_IDX )
               {
                  TA_SET_SKIP_FLAG(readOp[i]);
                  TA_SET_NB_NUMERIC(readOp[i],1);
               }
               break;

            case TA_MONTHLY:
               /* No need for anything shorter than days in the timestamp. */
               if( tmpIdx <= TA_DAY_IDX )
               {
                  TA_SET_SKIP_FLAG(readOp[i]);
                  TA_SET_NB_NUMERIC(readOp[i],1);
               }
               break;

            case TA_DAILY:
               /* No need for hour,min,sec in the timestamp. */
               if( tmpIdx <= TA_HOUR_IDX )
               {
                  TA_SET_SKIP_FLAG(readOp[i]);
                  TA_SET_NB_NUMERIC(readOp[i],1);
               }
               break;
            default:
               /* Do nothing. */
               break;
            }
         }
         else if( !(field&fieldToAlloc) )
         {
            /* This field is not requested for being allocated. */
            TA_SET_SKIP_FLAG(readOp[i]);
            TA_SET_NB_NUMERIC(readOp[i],1);
         }
      }
   }

   TA_ASSERT( TA_IS_LAST_SET(readOp[nbReadOp-1]) );

   /* Determine the last needed field. Mark it with the STOP flag.
    * Make sure all previous get their STOP flag clear.
    */
   flagSet = 0;
   for( i=nbReadOp; i > 0; i-- )
   {
      lastOp = &readOp[i-1];
      if( flagSet )
      {
         TA_CLR_READ_STOP_FLAG(*lastOp);
      }
      else
      {
         TA_SET_READ_STOP_FLAG(*lastOp);

         if( !TA_IS_SKIP_SET(*lastOp) )
           flagSet = 1;
      }
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
/* None. */



