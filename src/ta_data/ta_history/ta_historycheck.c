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
 *  112400 MF   First version.
 *
 */

/* Description:
 *     Implement an "independant" mechanism to validate a TA_History.
 */

/**** Headers ****/
#include "ta_history.h"
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
TA_RetCode TA_HistoryCheckInternal(
                                    TA_Period           expectedPeriod,
                                    const TA_Timestamp *expectedStart,
                                    const TA_Timestamp *expectedEnd,
                                    TA_Field            fieldToCheck,
                                    const TA_History   *history,
                                    unsigned int       *faultyIndex,
                                    unsigned int       *faultyField )
{
   TA_PROLOG
   unsigned int allFieldNull;

   (void)faultyField;
   (void)faultyIndex;
   (void)fieldToCheck;
   (void)expectedEnd;
   (void)expectedStart;

   TA_TRACE_BEGIN(  TA_HistoryCheckInternal );

   TA_ASSERT( history != NULL );

   /* Period shall be always set in the history. */
   if( history->period != expectedPeriod )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(45) );
   }

   /* Verify that an empty history is really empty. */
   if( (history->open  == NULL) &&
       (history->high  == NULL) &&
       (history->low   == NULL) &&
       (history->close == NULL) &&
       (history->volume == NULL) &&
       (history->openInterest == NULL) &&
       (history->timestamp == NULL) )
   {
      allFieldNull = 1;
      if( history->nbBars != 0 )
      {
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(46) );
      }
   }
   else
      allFieldNull = 0;

   if( (history->nbBars == 0) && !allFieldNull )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(47) );
   }

   /* !!! Some more runtime verification could be added here... */

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
/* None */

