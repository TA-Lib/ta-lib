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
 *  AK       Alexander Kugel
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *  052604 AK   Added intraday-to-intraday conversion
 *  082304 MF   Change default timestamp to begining of period.
 *              Now allow to transform in pre-allocated buffer.
 *  103104 MF   Add support for TA_ALLOW_INCOMPLETE_PRICE_BARS
 */

/* Description:
 *    Functions to consolidate price bars into a different period.
 */

/**** Headers ****/
#include <stdlib.h>
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_history_priv.h"


/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#ifndef max
   #define max(a,b) (a>b)?a:b
#endif

#ifndef min
   #define min(a,b) (a<b)?a:b
#endif

/**** Local functions declarations.    ****/
/* None */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/


TA_RetCode TA_PeriodNormalize( TA_BuilderSupport *builderSupport )
{
   /* This function will bring all the data to the same timeframe
    * prior to the merge procedure.
    */

   /* !!! Not implemented. For the time being all the data source are assumed
    * !!! to provide the data in the requested 'period' timeframe.
    */
   (void)builderSupport;

   return TA_SUCCESS;
}

/* Allocate data in a different timeframe from
 * an existing history.
 *
 * If the returned pointer are initialize with something
 * different than NULL, TA_Free must be called on
 * this pointer to free this data when not needed
 * anymore.
 */
TA_RetCode TA_PeriodTransform( TA_History *history,       /* The original history. */
                               TA_Period newPeriod,       /* The new desired period. */
                               TA_HistoryFlag flags, 
                               int doAllocateNew,         /* When true, following ptrs are used. */
                               TA_Integer *nbBars,        /* Return the number of price bar allocated. */
                               TA_Timestamp **timestamp,  /* Allocate new timestamp. */
                               TA_Real **open,            /* Allocate new open. */
                               TA_Real **high,            /* Allocate new high. */
                               TA_Real **low,             /* Allocate new low. */
                               TA_Real **close,           /* Allocate new close. */
                               TA_Integer **volume,       /* Allocate new volume. */
                               TA_Integer **openInterest  /* Allocate new openInterest. */ )
{
   TA_PROLOG

   TA_RetCode retCode;

   int transformToDailyAndMore;         /* Boolean */
   int transformToIntraday;             /* Boolean */
   int mustSkipFirstIncompletePriceBar; /* Boolean */

   TA_DayOfWeek dayOfTheWeek;

   /* Temporaries. */
   const TA_Timestamp *tempConstTimestamp;
   TA_Timestamp *tempTimestamp;
   TA_Timestamp tempLocalTimestamp;
   unsigned int tempInt;
   unsigned int day, month, year;

   /* Variable used to identify period crossing. */
   unsigned int currentDay, currentWeek, currentMonth, currentYear, currentQuarter;

   /* Pointer on the history being transformed. */
   TA_Timestamp *old_timestamp;    /* Old timestamp. */
   TA_Real      *old_open;         /* Old open. */
   TA_Real      *old_high;         /* Old high. */
   TA_Real      *old_low;          /* Old low. */
   TA_Real      *old_close;        /* Old close. */
   TA_Integer   *old_volume;       /* Old volume. */
   TA_Integer   *old_openInterest; /* Old openInterest. */
   TA_Integer    old_nbBars;

   /* Pointer on where the transformed history is written.
    *
    * Depending of the parameter doAllocateNew, these ptrs
    * are on new allocated data, or overlap on the memory block
    * in the history parameter.
    */
   TA_Timestamp *dest_timestamp;    /* New allocated timestamp. */
   TA_Real      *dest_open;         /* New allocated open. */
   TA_Real      *dest_high;         /* New allocated high. */
   TA_Real      *dest_low;          /* New allocated low. */
   TA_Real      *dest_close;        /* New allocated close. */
   TA_Integer   *dest_volume;       /* New allocated volume. */
   TA_Integer   *dest_openInterest; /* New allocated openInterest. */
   TA_Integer    dest_nbBars;

   TA_Timestamp  cur_timestamp;    /* Current new timestamp of new period. */
   TA_Real       cur_open;         /* Current new open of new period. */
   TA_Real       cur_high;         /* Current new high of new period. */
   TA_Real       cur_low;          /* Current new low of new period. */
   TA_Real       cur_close;        /* Current new close of new period. */
   unsigned long cur_volume;       /* Current new volume of new period. */
   unsigned long cur_openInterest; /* Current new openInterest of new period. */

   int oldPriceBar, newPriceBar; /* Array iterators. */
   unsigned int periodCompleted, errorOccured, newDay; /* Boolean */
   int nbDayAccumulated, firstIteration;

   TA_TRACE_BEGIN( TA_PeriodTransform );

   /* Validate some mandatory parameter. */
   TA_ASSERT( history != NULL );
   TA_ASSERT( newPeriod != 0 );

   /* It is assume that there is really a transform to do. */
   TA_ASSERT( history->period != newPeriod );
   TA_ASSERT( history->nbBars > 0 );

   /* Of course, timestamps from the source are needed. */
   TA_ASSERT( history->timestamp != NULL );

   /* Initialize all callers pointers to NULL.
    * These will be initialize only on success.
    * In the meantime, dest_XXXX pointers are
    * going to be used on the new allocated data.
    */
   #define SET_IF_NOT_NULL(x) {if(x) *x = NULL; }  
   SET_IF_NOT_NULL( timestamp    );
   SET_IF_NOT_NULL( open         );
   SET_IF_NOT_NULL( high         );
   SET_IF_NOT_NULL( low          );
   SET_IF_NOT_NULL( close        );
   SET_IF_NOT_NULL( volume       );
   SET_IF_NOT_NULL( openInterest );
   #undef SET_IF_NOT_NULL

   /* Validate the supported transformation. */

   /* First, we can transform only to longuer period.
    * (daily can become weekly, but not the reverse).
    */
   if( history->period > newPeriod )
   {
      TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
   }

   /* Eliminate all the transform that
    * are currently not supported. 
    * Identify also the major steps
    * needed to perform the transformation.
    */
   transformToDailyAndMore = 0;
   transformToIntraday     = 0;

   switch( history->period )
   {
   case TA_1MIN:
   case TA_5MINS:
   case TA_10MINS:
   case TA_15MINS:
   case TA_30MINS:
   case TA_1HOUR:
      switch( newPeriod )
      {
      case TA_DAILY:
      case TA_WEEKLY:
      case TA_MONTHLY:
      case TA_QUARTERLY:
      case TA_YEARLY:
         transformToDailyAndMore = 1;
         break;

      default:
         if(newPeriod <= TA_12HOURS)
            transformToIntraday = 1;
         else
            TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
         
      }
      break;

   case TA_DAILY:
      switch( newPeriod )
      {
      case TA_WEEKLY:
      case TA_MONTHLY:
      case TA_QUARTERLY:
      case TA_YEARLY:
         /* These are supported. */
         transformToDailyAndMore = 1;
         break; 
      default:
         TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
      }
      break;

   /* All other are not implemented yet. */
   default:      
      TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
   }

   old_timestamp    = &history->timestamp   [0];
   old_open         = &history->open        [0];
   old_high         = &history->high        [0];
   old_low          = &history->low         [0];
   old_close        = &history->close       [0];
   old_volume       = &history->volume      [0];
   old_openInterest = &history->openInterest[0];
   old_nbBars       = history->nbBars;

   /* Setup the destination for the new price bars. */
   if( !doAllocateNew )
   {
      /* Use the existing history to store new data. */      
      dest_timestamp    = old_timestamp;
      dest_open         = old_open;
      dest_high         = old_high;
      dest_low          = old_low;
      dest_close        = old_close;
      dest_volume       = old_volume;
      dest_openInterest = old_openInterest;
      dest_nbBars       = old_nbBars;
   }
   else
   {
      /* Calculate the number of required new price bar to allocate. 
       * Slight over-estimation is fine. the memory block are going
       * to be re-allocated at the end.
       */
      retCode = TA_UNKNOWN_ERR;
      if( transformToDailyAndMore )
      {
         switch( newPeriod )
         {
         case TA_WEEKLY:
            retCode = TA_TimestampDeltaWeek( &old_timestamp[0],
                                             &old_timestamp[old_nbBars-1],
                                             (unsigned int *)&dest_nbBars );
            break;
         case TA_MONTHLY:
            retCode = TA_TimestampDeltaMonth( &old_timestamp[0],
                                              &old_timestamp[old_nbBars-1],
                                              (unsigned int *)&dest_nbBars );
            break;
         case TA_QUARTERLY:
            retCode = TA_TimestampDeltaQuarter( &old_timestamp[0],
                                                &old_timestamp[old_nbBars-1],
                                                (unsigned int *)&dest_nbBars );
            break;
         case TA_YEARLY:
            retCode = TA_TimestampDeltaYear( &old_timestamp[0],
                                             &old_timestamp[old_nbBars-1],
                                             (unsigned int *)&dest_nbBars );
            break;
         case TA_DAILY:
            retCode = TA_TimestampDeltaDay( &old_timestamp[0],
                                            &old_timestamp[old_nbBars-1],
                                            (unsigned int *)&dest_nbBars );
            break;
         default:
            TA_TRACE_RETURN( TA_INTERNAL_ERROR(48) );
         }
      } 
      else if( transformToIntraday)
      {
          retCode = TA_TimestampDeltaIntraday(&old_timestamp[0],
                                              &old_timestamp[old_nbBars-1],
                                              (unsigned int *)&dest_nbBars, 
                                              history->period, newPeriod);
      }
   
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
      
      /* Do memory allocation of output.  */
   
      /* Slightly over-estimate to avoid limit cases */
      dest_nbBars += 2;
   
      /* Allocate the new data. */      
      errorOccured = 0;
      #define TRY_ALLOC( varName, varType ) \
      { \
         if( !errorOccured && varName && old_##varName ) \
         { \
            dest_##varName = TA_Malloc( dest_nbBars * sizeof( varType ) ); \
            if( !dest_##varName ) \
               errorOccured = 1; \
         } \
         else \
         { \
            dest_##varName = NULL; \
         } \
      }
      TRY_ALLOC( timestamp,    TA_Timestamp );
      TRY_ALLOC( open,         TA_Real      );
      TRY_ALLOC( high,         TA_Real      );
      TRY_ALLOC( low,          TA_Real      );
      TRY_ALLOC( close,        TA_Real      );
      TRY_ALLOC( volume,       TA_Integer   );
      TRY_ALLOC( openInterest, TA_Integer   );
      #undef TRY_ALLOC   

      if( errorOccured )
      {
         FREE_IF_NOT_NULL( dest_timestamp );
         FREE_IF_NOT_NULL( dest_open );
         FREE_IF_NOT_NULL( dest_high);
         FREE_IF_NOT_NULL( dest_low );
         FREE_IF_NOT_NULL( dest_close )
         FREE_IF_NOT_NULL( dest_volume );
         FREE_IF_NOT_NULL( dest_openInterest );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }
   }

   /* OK.. now proceed with the transformations. */
   newPriceBar      = 0;

   cur_timestamp.date = 0;    /* Current new timestamp of new period. */
   cur_timestamp.time = 0;    /* Current new timestamp of new period. */

   cur_open         = 0.0;     /* Current new open of new period. */
   cur_high         = 0.0;     /* Current new high of new period. */
   cur_low          = 0.0;     /* Current new low of new period. */
   cur_close        = 0.0;     /* Current new close of new period. */
   cur_volume       = 0;       /* Current new volume of new period. */
   cur_openInterest = 0;       /* Current new openInterest of new period. */

   if(transformToDailyAndMore)
   {
      /* Macro used to adjust the current value for the new period price bar. */
      #define SET_CUR_IF_NOT_NULL(var,todo) {if(dest_##var) { cur_##var=todo; }}
      
      /* Allows to detect crossing of the new period. */
      currentYear    = TA_GetYear ( &old_timestamp[0] );
      currentMonth   = TA_GetMonth( &old_timestamp[0] );
      currentDay     = TA_GetDay  ( &old_timestamp[0] );
      currentWeek    = TA_GetWeekOfTheYear   ( &old_timestamp[0] );
      currentQuarter = TA_GetQuarterOfTheYear( &old_timestamp[0] );

      /* If first old price bar is not the first weekday for the 
       * period, then consider this period incomplete.
       */
      mustSkipFirstIncompletePriceBar = 0;
      if( (newPeriod > TA_DAILY) && !(flags & TA_ALLOW_INCOMPLETE_PRICE_BARS) )
      {
         TA_TimestampCopy( &tempLocalTimestamp, &old_timestamp[0] );
         if( newPeriod == TA_WEEKLY )
            TA_BackToDayOfWeek( &tempLocalTimestamp, TA_MONDAY );
         else
         {
            switch( newPeriod )
            {
            case TA_MONTHLY:
               TA_BackToBeginOfMonth( &tempLocalTimestamp );
               break;
            case TA_QUARTERLY:
               TA_BackToBeginOfQuarter( &tempLocalTimestamp );
               break;
            case TA_YEARLY:
               TA_BackToBeginOfYear( &tempLocalTimestamp );
               break;         
            }
            dayOfTheWeek = TA_GetDayOfTheWeek(&tempLocalTimestamp);
            if( (dayOfTheWeek == TA_SUNDAY) || (dayOfTheWeek == TA_SATURDAY) )
               TA_NextWeekday( &tempLocalTimestamp );
         }

         if( !TA_TimestampDateEqual( &tempLocalTimestamp, &old_timestamp[0] ) )
            mustSkipFirstIncompletePriceBar = 1;
      }

      /* Iterate through the old price bar. */
      oldPriceBar = 0;

      /* Iterate through the new price bar. */
      newPriceBar = 0;

      while( oldPriceBar < old_nbBars)
      {
         /* Initialize cur_XXXXXX variables with the first bar in old timeframe. */
         SET_CUR_IF_NOT_NULL( timestamp,     old_timestamp   [oldPriceBar] );
         SET_CUR_IF_NOT_NULL( open,          old_open        [oldPriceBar] );
         SET_CUR_IF_NOT_NULL( high,          old_high        [oldPriceBar] );
         SET_CUR_IF_NOT_NULL( low,           old_low         [oldPriceBar] );
         SET_CUR_IF_NOT_NULL( close,         old_close       [oldPriceBar] );
         SET_CUR_IF_NOT_NULL( volume,        old_volume      [oldPriceBar] );
         SET_CUR_IF_NOT_NULL( openInterest,  old_openInterest[oldPriceBar] );

         /* Go through the bars and accumulate the info
          * until the end of the requested period is reach.
          */
         periodCompleted = 0;
         nbDayAccumulated = 1;
         firstIteration = 1;
         while( (oldPriceBar < old_nbBars) && !periodCompleted )
         {
            tempConstTimestamp = &old_timestamp[oldPriceBar];

            /* Check when being a new day */
            day   = TA_GetDay  ( tempConstTimestamp );
            month = TA_GetMonth( tempConstTimestamp );
            year  = TA_GetYear ( tempConstTimestamp );

            if( (currentDay   != day )  ||
                (currentMonth != month) ||
                (currentYear  != year) )
               newDay = 1;
            else
               newDay = 0;               

            /* Check if we reached an end of period. */
            switch( newPeriod )
            {
            case TA_DAILY:
               if( newDay )
                  periodCompleted = 1;
               break;
            case TA_WEEKLY:
               tempInt  = TA_GetWeekOfTheYear( tempConstTimestamp );

               /* Trap weeks on years boundary. */
               if( (currentWeek == 52) && (tempInt == 0) )
                  currentWeek = 0;
               else if( currentWeek != tempInt )
               {
                  periodCompleted = 1;
                  currentWeek = tempInt;
               }
               break;
            case TA_MONTHLY:
               if( (currentMonth != month) ||
                   (currentYear  != year) )
               {
                  periodCompleted = 1;
               }
               break;
            case TA_QUARTERLY:
               tempInt = TA_GetQuarterOfTheYear( tempConstTimestamp );
               if( (currentQuarter != tempInt) ||
                   (currentYear    != year) )
               {
                  periodCompleted = 1;
                  currentQuarter  = tempInt;
               }
               break;
            case TA_YEARLY:
               if( currentYear != year )
                  periodCompleted = 1;
               break;
            default:
               /* Do nothing */
               break;
            }

            currentDay   = day;
            currentMonth = month;
            currentYear  = year;

            /* If this is not the end of a period (in the new timeframe)
             * just accumulate the data. If this is the end of the period
             * that while loop will be exited.
             * Nothing is done on the first iteration because all the 
             * cur_XXXX variables have been already initialized.
             */
            if( !periodCompleted )
            {
               if( !firstIteration )
               {
                  /* Adjust the new price bar. */
                  SET_CUR_IF_NOT_NULL( high, max( cur_high, old_high[oldPriceBar]) );
                  SET_CUR_IF_NOT_NULL( low,  min( cur_low,  old_low [oldPriceBar]) );
                  SET_CUR_IF_NOT_NULL( volume, cur_volume+old_volume[oldPriceBar] );
                  SET_CUR_IF_NOT_NULL( openInterest, cur_openInterest+old_openInterest[oldPriceBar] );
                  SET_CUR_IF_NOT_NULL( close, old_close[oldPriceBar] );
                  SET_CUR_IF_NOT_NULL( timestamp, old_timestamp[oldPriceBar] );
                  if( newDay )
                     nbDayAccumulated++;
               }
               else
                  firstIteration = 0;

               /* Move to next bar. */
               oldPriceBar++;
            }
         }

         /* In the case that the first consolidated period is not 
          * completed, just skip creating the new price bar and continue 
          * with the next period.
          */
         if( mustSkipFirstIncompletePriceBar )
         {
            mustSkipFirstIncompletePriceBar = 0;
            continue;
         }

         /* We got all the info needed in the cur_XXXXX variables for
          * proceeding with the initialization of the new period price bar.
          */
         TA_ASSERT_DEBUG( newPriceBar < dest_nbBars );
         
         /* Volume and Open interest are changed to daily average. */
         if( old_volume )
            cur_volume = cur_volume / nbDayAccumulated;
         if( old_openInterest )
            cur_openInterest = cur_openInterest / nbDayAccumulated;

         /* The new price bar is initialized here. */
         if(dest_timestamp)
         {
            switch( newPeriod )
            {
            case TA_WEEKLY:
                  /* Now something a little bit tricky, we must
                   * make sure that this new price bar is reported
                   * as being the Sunday of that week.
                   */
                  TA_BackToDayOfWeek( &cur_timestamp, TA_SUNDAY );
                  break;
            case TA_MONTHLY:
                  /* Monthly timestamp are always on the first day of
                   * the month.
                   */
                  TA_BackToBeginOfMonth( &cur_timestamp );
                  break;
            case TA_QUARTERLY:
                  /* Quarterly timestamp are always the first day of
                   * the quarter.
                   * Quarter 1 =  1/1
                   * Quarter 2 =  4/1
                   * Quarter 3 =  7/1
                   * Quarter 4 = 10/1
                   */
                  TA_BackToBeginOfQuarter( &cur_timestamp );
                  break;
            case TA_YEARLY:
                  /* Yearly data always end on 12/31. */
                  TA_BackToBeginOfYear( &cur_timestamp );
                  break;
            default:
                  /* Do nothing. */
                  break;
            }   

            dest_timestamp[newPriceBar].date = cur_timestamp.date;
            dest_timestamp[newPriceBar].time = 0; /* All EOD price bar starts at 00:00:00 */
         }
         #define SET_DEST_PERIOD_IF_NOT_NULL(var) { if( dest_##var ) { dest_##var[newPriceBar] = cur_##var; } }
         SET_DEST_PERIOD_IF_NOT_NULL( open );
         SET_DEST_PERIOD_IF_NOT_NULL( high );
         SET_DEST_PERIOD_IF_NOT_NULL( low  );
         SET_DEST_PERIOD_IF_NOT_NULL( close );
         SET_DEST_PERIOD_IF_NOT_NULL( volume );
         SET_DEST_PERIOD_IF_NOT_NULL( openInterest );
         #undef SET_DEST_PERIOD_IF_NOT_NULL
                     
         /* This new period bar is completed, move to the next one. */
         newPriceBar++;         
      }
      #undef SET_CUR_IF_NOT_NULL

      
      if( (newPeriod > TA_DAILY) && !(flags & TA_ALLOW_INCOMPLETE_PRICE_BARS)  && (newPriceBar >= 1) )
      {
         /* Put in tempConstTimestamp the last weekday in the last period. */
         tempTimestamp = &dest_timestamp[newPriceBar-1];
         TA_TimestampCopy(&tempLocalTimestamp,tempTimestamp);

         
         /* Check if the last price bar is completed. If not, trim it from the output. */
         if( !periodCompleted && !TA_TimestampEqual(&tempLocalTimestamp,tempTimestamp) )
         {
            /* Last price bar is incomplete. Remove it. */
            newPriceBar--;
            /* To be on the safe side, invalidate the price bar from the output. */
            #define INVALIDATE_DATA_REAL(x) { if(dest_##x) dest_##x[newPriceBar] = -1.0; }
            INVALIDATE_DATA_REAL(open);
            INVALIDATE_DATA_REAL(high);
            INVALIDATE_DATA_REAL(low);
            INVALIDATE_DATA_REAL(close);
            #undef INVALIDATE_DATA_REAL
            #define INVALIDATE_DATA_INTEGER(x) { if(dest_##x) dest_##x[newPriceBar] = -1; }
            INVALIDATE_DATA_INTEGER(volume);
            INVALIDATE_DATA_INTEGER(openInterest);
            #undef INVALIDATE_DATA_INTEGER
            TA_SetDate( 0, 0, 0, tempTimestamp );
            TA_SetTime( 0, 0, 0, tempTimestamp );
         }
      }      
   }
   else if( transformToIntraday )
   {
      /* The intraday conversion goes from here --AK-- */
      TA_Timestamp tmp_ts;
      TA_Timestamp next_DEST_ts = {0,0};

      /* There is no open interest on the intraday data --AK-- */
      /* We can free some memory here                          */
      FREE_IF_NOT_NULL( dest_openInterest );

      /* Iterate through the old and new price bar. --AK--     */
      oldPriceBar = 0;
      newPriceBar = -1;

      periodCompleted = 0;
      newDay = 0;

      day = month = year = 0;

      /* Process all old bars --AK-- */
      while( oldPriceBar < old_nbBars )
      {
         #define SET_DEST_FROM_OLD_IF_NOT_NULL(var,todo) { if( dest_##var ) { dest_##var[newPriceBar] = todo; } }
         
         /* Check if we are at a new day */
         currentDay   = TA_GetDay  ( &old_timestamp[oldPriceBar] );
         currentMonth = TA_GetMonth( &old_timestamp[oldPriceBar] );
         currentYear  = TA_GetYear ( &old_timestamp[oldPriceBar] );

         if( (currentDay != day )  || (currentMonth != month) || (currentYear != year) ){
            newDay = 1;
            day    = currentDay;
            month  = currentMonth;
            year   = currentYear;
         }else{
            newDay = 0;
         }

         /* Check if the new bar is already completed --AK-- */
         if( newDay || !TA_TimestampLess(&old_timestamp[oldPriceBar],&next_DEST_ts) ){
             periodCompleted = 1;
         }

         if(periodCompleted){
            /* Move to the next new bar --AK-- */
            newPriceBar++;

            /* Take care of the missing data (market inactivity, market 
               close hours, new day, etc.): recalculate the timestamp 
               for the new bar.  --AK--                                  */
            TA_TimestampAlign(&tmp_ts, &old_timestamp[oldPriceBar], newPeriod);

            /* Create a new bar --AK-- */
            SET_DEST_FROM_OLD_IF_NOT_NULL( timestamp,     tmp_ts );
            SET_DEST_FROM_OLD_IF_NOT_NULL( open,          old_open  [oldPriceBar] );
            SET_DEST_FROM_OLD_IF_NOT_NULL( high,          old_high  [oldPriceBar] );
            SET_DEST_FROM_OLD_IF_NOT_NULL( low,           old_low   [oldPriceBar] );
            SET_DEST_FROM_OLD_IF_NOT_NULL( close,         old_close [oldPriceBar] );
            SET_DEST_FROM_OLD_IF_NOT_NULL( volume,        old_volume[oldPriceBar] );

            /* Find the timestamp of the next new bar --AK-- */
            TA_AddTimeToTimestamp(&next_DEST_ts, &dest_timestamp[newPriceBar], newPeriod);
            periodCompleted = 0;
         }else{
            /* Modify the current new --AK-- */
            SET_DEST_FROM_OLD_IF_NOT_NULL( high, max( dest_high[newPriceBar], old_high[oldPriceBar]) );
            SET_DEST_FROM_OLD_IF_NOT_NULL( low,  min( dest_low[newPriceBar],  old_low [oldPriceBar]) );
            SET_DEST_FROM_OLD_IF_NOT_NULL( close, old_close[oldPriceBar] );
            SET_DEST_FROM_OLD_IF_NOT_NULL( volume, dest_volume[newPriceBar]+old_volume[oldPriceBar] );
         }

         /* Go to the next old bar */
         oldPriceBar ++;

         #undef SET_DEST_FROM_OLD_IF_NOT_NULL
      }
      newPriceBar++;
   }

   /* Do re-allocation as needed. */
   if( (newPriceBar != 0) && (newPriceBar < dest_nbBars) )
   {
      #define REALLOC(x,type) { if(dest_##x) dest_##x = TA_Realloc(dest_##x,newPriceBar*sizeof(type)); }
      REALLOC( open,         TA_Real      );
      REALLOC( high,         TA_Real      );
      REALLOC( low,          TA_Real      );
      REALLOC( close,        TA_Real      );
      REALLOC( volume,       TA_Integer   );
      REALLOC( openInterest, TA_Integer   );
      REALLOC( timestamp,    TA_Timestamp );
      #undef REALLOC
      dest_nbBars = newPriceBar; 
   }

   /* Clean-up if there is no price bar in the new period. */
   if( newPriceBar == 0 )
   {
      #define CLEAN_UP(x) { if(dest_##x){TA_Free(dest_##x); dest_##x=NULL;} }
      CLEAN_UP( open );
      CLEAN_UP( high   );
      CLEAN_UP( low    );
      CLEAN_UP( close  );
      CLEAN_UP( volume );
      CLEAN_UP( openInterest);
      CLEAN_UP( timestamp );
      #undef CLEAN_UP
      dest_nbBars = 0;
   }

   /* All done! Return the final result to the caller. */
   history->period = newPeriod;
   if( !doAllocateNew )
   {
      /* Update pointers in history. */
      #define SET_HISTORY_FIELD(x) { history->x = dest_##x; }
      SET_HISTORY_FIELD( nbBars       );
      SET_HISTORY_FIELD( open         );
      SET_HISTORY_FIELD( high         );
      SET_HISTORY_FIELD( low          );
      SET_HISTORY_FIELD( close        );
      SET_HISTORY_FIELD( volume       );
      SET_HISTORY_FIELD( openInterest );
      SET_HISTORY_FIELD( timestamp    );
      #undef SET_HISTORY_FIELD
   }
   else
   {
      #define SET_IF_NOT_NULL(x) { if(x) *x = dest_##x; }
      SET_IF_NOT_NULL( nbBars       );
      SET_IF_NOT_NULL( open         );
      SET_IF_NOT_NULL( high         );
      SET_IF_NOT_NULL( low          );
      SET_IF_NOT_NULL( close        );
      SET_IF_NOT_NULL( volume       );
      SET_IF_NOT_NULL( openInterest );
      SET_IF_NOT_NULL( timestamp    );
      #undef SET_IF_NOT_NULL
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
/* None */

