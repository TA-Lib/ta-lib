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
 *  112400 MF   First version.
 *  120104 MF   Now check start/end boundary.
 *  080804 MF   Add a lot of price bar validation.
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
TA_RetCode TA_HistoryCheck( TA_Period           expectedPeriod,
                            const TA_Timestamp *expectedStart,
                            const TA_Timestamp *expectedEnd,
                            TA_Field            fieldToCheck,
                            const TA_History   *history,
                            unsigned int       *faultyIndex,
                            unsigned int       *faultyField )
{
   TA_PROLOG
   unsigned int i, defaultLoop;
   TA_Timestamp *prevTimestamp;
   TA_Timestamp *tempTimestamp;
   TA_Real tempRealHigh, tempRealLow, tempRealOpen, tempRealClose;

   (void)faultyField;
   (void)faultyIndex;
   (void)fieldToCheck;

   TA_TRACE_BEGIN( TA_HistoryCheck );

   TA_ASSERT( history != NULL );

   /***************************************************************
    * First section detects issues that would indicate a flaw in  *
    * the TA-Lib logic. For these, report a fatal internal error. *
    ***************************************************************/

   /* Period shall be always set in the history. */
   if( history->period != expectedPeriod )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(45) );
   }

   if( (history->open  == NULL) &&
       (history->high  == NULL) &&
       (history->low   == NULL) &&
       (history->close == NULL) &&
       (history->volume == NULL) &&
       (history->openInterest == NULL) &&
       (fieldToCheck != TA_TIMESTAMP) )
   {
      /* An empty history should have none of these array allocated. */
      if( history->nbBars != 0 )
      {
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(46) );
      }      
      /* Nothing else to check for an empty history. */
      TA_TRACE_RETURN( TA_SUCCESS );
   }
   else if( history->nbBars == 0 )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(47) );
   }
   else if( history->nbBars > 0 )
   {
      if( history->timestamp == NULL )
      {
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(168) );
      }

      /* Verify that the data is not older than "start". */
      if( expectedStart )
      {
         if( expectedPeriod < TA_DAILY )
         {
            if( TA_TimestampLess( &history->timestamp[0], expectedStart ) )
               TA_TRACE_RETURN( TA_INTERNAL_ERROR(142) );
         }
         else
         {
            if( TA_TimestampDateLess( &history->timestamp[0], expectedStart ) )
               TA_TRACE_RETURN( TA_INTERNAL_ERROR(144) );
         }
      }

      /* Verify that the data is not more recent than "end". */
      if( expectedEnd ) 
      {
         if( expectedPeriod < TA_DAILY )
         {
            if( TA_TimestampGreater( &history->timestamp[history->nbBars-1], expectedEnd) )
               TA_TRACE_RETURN( TA_INTERNAL_ERROR(143) );
         }
         else
         {
            if( TA_TimestampDateGreater( &history->timestamp[history->nbBars-1], expectedEnd) )
               TA_TRACE_RETURN( TA_INTERNAL_ERROR(145) );
         }
      }
   }

   if( fieldToCheck != TA_ALL )
   {
      #define CHECK_EXPECTED_FIELD(lc,uc,rc) { \
         if( ((history->lc) && !(fieldToCheck&TA_##uc)) || (!(history->lc) && (fieldToCheck&TA_##uc)) ) \
         { \
            TA_TRACE_RETURN( TA_INTERNAL_ERROR(rc) ); \
         } \
      }
      CHECK_EXPECTED_FIELD( open,         OPEN,         169 );
      CHECK_EXPECTED_FIELD( high,         HIGH,         170 );
      CHECK_EXPECTED_FIELD( low ,         LOW,          171 );
      CHECK_EXPECTED_FIELD( close,        CLOSE,        172 );
      CHECK_EXPECTED_FIELD( volume,       VOLUME,       173 );
      CHECK_EXPECTED_FIELD( openInterest, OPENINTEREST, 174 );
      #undef CHECK_EXPECTED_FIELD
   }

   /***************************************************************
    * Second section detects issues related to the data returned  *
    * by the data source. These are reported as normal errors to  *
    * the caller.                                                 *
    ***************************************************************/

   if( history->nbBars != 0 )
   {
     /* Optimized loop depending which fields are available
      * 
      *               Loop #
      *            1  2  3  4  5
      * Open       *  *  *         
      * High       *  *  *         
      * Low        *  *  *         
      * Close      *  *  *  *  *   
      * Volume     *  *     *   
      * OI         *
      *
      * These are the expected most common field combinations, all 
      * other combinations are handled by the "defaultLoop"
      */
      defaultLoop = 1;

      #define CHECK_ASCENDING_TIMESTAMP { \
         tempTimestamp = &history->timestamp[i]; \
         if(TA_TimestampLess( tempTimestamp, prevTimestamp) ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_TIMESTAMP_ORDER ); \
         prevTimestamp = tempTimestamp; \
      }

      #define CHECK_VOLUME { \
         if( history->volume[i] < 0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_VOLUME_IS_NEGATIVE ); \
      }

      #define CHECK_OPEN_INTEREST { \
         if( history->openInterest[i] < 0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_OI_IS_NEGATIVE ); \
      }

      #define CHECK_CLOSE_ONLY { \
         if( history->close[i] < 0.0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_CLOSE_IS_NEGATIVE ); \
      }

      #define CHECK_OPEN_HIGH_LOW_CLOSE { \
         tempRealHigh  = history->high[i]; \
         tempRealLow   = history->low[i]; \
         if( tempRealHigh < 0.0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_HIGH_IS_NEGATIVE ); \
         if( tempRealLow < 0.0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_LOW_IS_NEGATIVE ); \
         if( tempRealHigh < tempRealLow ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_LOW_EXCEED_HIGH ); \
         tempRealOpen  = history->open[i]; \
         if( tempRealOpen < 0.0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_OPEN_IS_NEGATIVE ); \
         if( tempRealOpen > tempRealHigh ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_OPEN_EXCEED_HIGH ); \
         if( tempRealOpen < tempRealLow ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_OPEN_BELOW_LOW ); \
         tempRealClose = history->close[i]; \
         if( tempRealClose < 0.0 ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_CLOSE_IS_NEGATIVE ); \
         if( tempRealClose > tempRealHigh ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_CLOSE_EXCEED_HIGH ); \
         if( tempRealClose < tempRealLow ) \
            TA_TRACE_RETURN( TA_DATA_ERROR_CLOSE_BELOW_LOW ); \
      }

      if( history->close )
      {
         if( history->open && history->high && history->low )
         {
            if( history->volume )
            {
               if( history->openInterest )
               {
                  /* Loop #1 - Open,High,Low,Close,Volume,OpenInterest */
                  defaultLoop = 0;
                  prevTimestamp = &history->timestamp[0];
                  for( i=1; i < history->nbBars; i++ )
                  {
                     CHECK_ASCENDING_TIMESTAMP;
                     CHECK_VOLUME;
                     CHECK_OPEN_INTEREST;
                     CHECK_OPEN_HIGH_LOW_CLOSE;
                  }
               }
               else
               {
                  /* Loop #2 - Open,High,Low,Close,Volume */
                  defaultLoop = 0;
                  prevTimestamp = &history->timestamp[0];
                  for( i=1; i < history->nbBars; i++ )
                  {
                     CHECK_ASCENDING_TIMESTAMP;
                     CHECK_VOLUME;
                     CHECK_OPEN_HIGH_LOW_CLOSE;
                  }
               }
            }
            else if( !history->openInterest )
            {
               /* Loop #3 - Open,High,Low,Close */
               defaultLoop = 0;
               prevTimestamp = &history->timestamp[0];
               for( i=1; i < history->nbBars; i++ )
               {
                  CHECK_ASCENDING_TIMESTAMP;
                  CHECK_OPEN_HIGH_LOW_CLOSE;
               }
            }         
         }
         else if( (history->volume) && !(history->openInterest) )
         {
            /* Loop #4 - Close,Volume */
            defaultLoop = 0;
            prevTimestamp = &history->timestamp[0];
            for( i=1; i < history->nbBars; i++ )
            {
               CHECK_ASCENDING_TIMESTAMP;
               CHECK_CLOSE_ONLY;
               CHECK_VOLUME;
            }
         }
         else if( !(history->volume) && !(history->openInterest) )
         {
            /* Loop #5 - Close */
            defaultLoop = 0;
            prevTimestamp = &history->timestamp[0];
            for( i=1; i < history->nbBars; i++ )
            {
               CHECK_ASCENDING_TIMESTAMP;
               CHECK_CLOSE_ONLY;
            }
         }
      }

      if( defaultLoop )
      {
         prevTimestamp = &history->timestamp[0];
         for( i=1; i < history->nbBars; i++ )
         {
            CHECK_ASCENDING_TIMESTAMP;
            if( history->volume ) CHECK_VOLUME;
            if( history->openInterest ) CHECK_OPEN_INTEREST;
            if( history->close ) CHECK_CLOSE_ONLY;
            /* !!! More test can be added here */
         }
      }      
   }

   /* All fine! */
   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
/* None */

