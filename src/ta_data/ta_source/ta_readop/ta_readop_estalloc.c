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
 *  120100 MF   First version.
 *
 */

/* Description:
 *    Provides function to estimate the minimum memory allocation space
 *    needed for a given ASCII file.
 *    Consider multiple factor, including the time range requested
 *    by the user.
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
/* None */

/**** Global functions definitions.   ****/

TA_RetCode TA_EstimateAllocInit( const TA_Timestamp *start,
                                 const TA_Timestamp *end,
                                 TA_Period period,
                                 unsigned int minimumSize,
                                 unsigned int maximumSize,
                                 TA_EstimateInfo *estimationInfo,
                                 unsigned int *nbElementToAllocate )
{
   unsigned int nbElement;

   #if 0
      /* Force extrems value for testing. */
      *nbElementToAllocate = 1;
      return TA_SUCCESS;
   #endif

   nbElement = minimumSize; /* Default */

   if( start && end )
   {
      switch( period )
      {
      case TA_DAILY:
         TA_TimestampDeltaDay( start, end, &nbElement );
         break;
      case TA_WEEKLY:
         TA_TimestampDeltaWeek( start, end, &nbElement );
         break;
      case TA_MONTHLY:
         TA_TimestampDeltaMonth( start, end, &nbElement );
         break;
      case TA_QUARTERLY:
         TA_TimestampDeltaQuarter( start, end, &nbElement );
         break;
      case TA_YEARLY:
         TA_TimestampDeltaYear( start, end, &nbElement );
         break;
      default:
         return TA_BAD_PARAM;
      }
      nbElement += 2;
   }

   /* Make the estimation fits within the max/min provided. */
   estimationInfo->maximumSize = maximumSize;
   estimationInfo->minimumSize = minimumSize;
   if( nbElement > maximumSize )
      nbElement = maximumSize;

   if( nbElement < minimumSize )
      nbElement = minimumSize;

   *nbElementToAllocate = nbElement;

   return TA_SUCCESS;
}

TA_RetCode TA_EstimateAllocNext( TA_EstimateInfo *estimationInfo,
                                 unsigned int *nbElementToAllocate )
{
   /* In the case that the initial estimation was not sufficient,
    * a re-estimation is requested for the remainder of the file.
    */
   (void)estimationInfo;

   #if 0
      /* Force extrems value for testing. */
      *nbElementToAllocate = 11;
      return TA_SUCCESS;
   #endif

   /* Let's keep it simple for the time being. Just add 
    * 200 price bar.
    */
   *nbElementToAllocate = 200;

   return TA_SUCCESS;
}



/**** Local functions definitions.     ****/
/* None */



