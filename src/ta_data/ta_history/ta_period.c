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
 *    Provides functionality for changing the timeframe of the price bar.
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
TA_RetCode TA_PeriodTransform( const TA_History *history, /* The original history. */
                               TA_Period newPeriod,       /* The new desired period. */
                               TA_Integer *nbBars,        /* Return the number of price bar */
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

   unsigned int transformFromDaily; /* Boolean */

   /* Temporaries. */
   const TA_Timestamp *tempTimestamp;
   unsigned int tempInt, tempInt2;

   /* Variable used to identify period crossing. */
   unsigned int currentWeek, currentMonth, currentYear, currentQuarter;

   /* Pointer on the history being transformed. */
   const TA_Timestamp *old_timestamp;    /* Old timestamp. */
   const TA_Real      *old_open;         /* Old open. */
   const TA_Real      *old_high;         /* Old high. */
   const TA_Real      *old_low;          /* Old low. */
   const TA_Real      *old_close;        /* Old close. */
   const TA_Integer   *old_volume;       /* Old volume. */
   const TA_Integer   *old_openInterest; /* Old openInterest. */
   TA_Integer          old_nbBars;

   /* Pointer on the transformed history. */
   TA_Timestamp *new_timestamp;    /* New allocated timestamp. */
   TA_Real      *new_open;         /* New allocated open. */
   TA_Real      *new_high;         /* New allocated high. */
   TA_Real      *new_low;          /* New allocated low. */
   TA_Real      *new_close;        /* New allocated close. */
   TA_Integer   *new_volume;       /* New allocated volume. */
   TA_Integer   *new_openInterest; /* New allocated openInterest. */
   TA_Integer    new_nbBars;

   TA_Timestamp  cur_timestamp;    /* Current new timestamp of new period. */
   TA_Real       cur_open;         /* Current new open of new period. */
   TA_Real       cur_high;         /* Current new high of new period. */
   TA_Real       cur_low;          /* Current new low of new period. */
   TA_Real       cur_close;        /* Current new close of new period. */
   unsigned long cur_volume;       /* Current new volume of new period. */
   unsigned long cur_openInterest; /* Current new openInterest of new period. */

   int oldPriceBar, newPriceBar; /* Array iterators. */
   unsigned int again, periodCompleted, errorOccured; /* Boolean */
   int periodBarAccumulated, firstIteration;

   TA_TRACE_BEGIN(  TA_PeriodTransform );

   /* Validate some mandatory parameter. */
   TA_ASSERT( history != NULL );

   TA_ASSERT( newPeriod != 0 );
   TA_ASSERT( nbBars  != NULL );

   /* It is assume that the caller call this function
    * when there is really a transform to do.
    */
   TA_ASSERT( history->period != newPeriod );
   TA_ASSERT( history->nbBars > 0 );

   /* Of course, timestamps from the source are needed. */
   TA_ASSERT( history->timestamp != NULL );

   /* Initialize all callers pointers to NULL.
    * These will be initialize only on success.
    * In the meantime, new_XXXX pointers are
    * going to be used on the new allocated data.
    */
   if( timestamp    ) *timestamp    = NULL;
   if( open         ) *open         = NULL;
   if( high         ) *high         = NULL;
   if( low          ) *low          = NULL;
   if( close        ) *close        = NULL;
   if( volume       ) *volume       = NULL;
   if( openInterest ) *openInterest = NULL;
   *nbBars = 0;

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
    * (only transformFromDaily is implemented).
    */
   transformFromDaily = 0;
   switch( history->period )
   {
   case TA_DAILY:
      switch( newPeriod )
      {
      case TA_WEEKLY:
      case TA_MONTHLY:
      case TA_QUARTERLY:
      case TA_YEARLY:
         /* These are supported. */
         transformFromDaily = 1;
         break; 
      default:
         TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
      }
      break;

   /* All these are not implemented yet. */
   default:      
      TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
   }

   /* OK.. now proceed with the transformations. */
   old_timestamp    = &history->timestamp   [0];
   old_open         = &history->open        [0];
   old_high         = &history->high        [0];
   old_low          = &history->low         [0];
   old_close        = &history->close       [0];
   old_volume       = &history->volume      [0];
   old_openInterest = &history->openInterest[0];
   old_nbBars       = history->nbBars;

   new_timestamp    = NULL;
   new_open         = NULL;
   new_high         = NULL;
   new_low          = NULL;
   new_close        = NULL;
   new_volume       = NULL;
   new_openInterest = NULL;
   newPriceBar      = 0;

   cur_timestamp.date = 0;    /* Current new timestamp of new period. */
   cur_timestamp.time = 0;    /* Current new timestamp of new period. */

   cur_open         = 0.0;     /* Current new open of new period. */
   cur_high         = 0.0;     /* Current new high of new period. */
   cur_low          = 0.0;     /* Current new low of new period. */
   cur_close        = 0.0;     /* Current new close of new period. */
   cur_volume       = 0;       /* Current new volume of new period. */
   cur_openInterest = 0;       /* Current new openInterest of new period. */
   
   /* If needed, we will proceed with intra-day
    * to intra-day transform... not implemented
    * yet.
    */

   /* If not intra-day, we may need to proceed with
    * intra day to daily transform... not implemented yet.
    */

   /* If needed, proceed with the transform from
    * daily to other period.
    */
   if( transformFromDaily )
   {
      /* Overestimate the number of required new price bar. */
      switch( newPeriod )
      {
      case TA_WEEKLY:
         retCode = TA_TimestampDeltaWeek( &old_timestamp[0], &old_timestamp[old_nbBars-1], (unsigned int *)&new_nbBars );
         break;
      case TA_MONTHLY:
         retCode = TA_TimestampDeltaMonth( &old_timestamp[0], &old_timestamp[old_nbBars-1], (unsigned int *)&new_nbBars );
         break;
      case TA_QUARTERLY:
         retCode = TA_TimestampDeltaQuarter( &old_timestamp[0], &old_timestamp[old_nbBars-1], (unsigned int *)&new_nbBars );
         break;
      case TA_YEARLY:
         retCode = TA_TimestampDeltaYear( &old_timestamp[0], &old_timestamp[old_nbBars-1], (unsigned int *)&new_nbBars );
         break;
      default:
         TA_TRACE_RETURN( TA_INTERNAL_ERROR(48) );
      }
      new_nbBars += 2;

      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }

      /* Allocate the new data. */      
      errorOccured = 0;
      #define TRY_ALLOC( varName, varType ) \
      { \
         if( !errorOccured && varName && old_##varName ) \
         { \
            new_##varName = TA_Malloc( new_nbBars * sizeof( TA_##varType ) ); \
            if( !new_##varName ) \
               errorOccured = 1; \
         } \
      }
      TRY_ALLOC( timestamp, Timestamp );
      TRY_ALLOC( open, Real );
      TRY_ALLOC( high, Real );
      TRY_ALLOC( low, Real );
      TRY_ALLOC( close, Real );
      TRY_ALLOC( volume, Integer );
      TRY_ALLOC( openInterest, Integer );

      if( errorOccured )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* Macro used to adjust the current value for the new period price bar. */
      #define SET_CUR_IF_NOT_NULL(var,todo) {if(new_##var) { cur_##var=todo; }}
      
      /* Allows to detect crossing of the new period. */
      currentYear    = TA_GetYear ( &old_timestamp[0] );
      currentMonth   = TA_GetMonth( &old_timestamp[0] );
      currentWeek    = TA_GetWeekOfTheYear( &old_timestamp[0] );
      currentQuarter = TA_GetQuarterOfTheYear( &old_timestamp[0] );

      /* Iterate through the old price bar. */
      oldPriceBar = 0;

      /* Iterate through the new price bar. */
      newPriceBar = 0;

      again = 1; /* Becomes false when all bars are processed. */
      while( again )
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
         periodBarAccumulated = 1;
         firstIteration = 1;
         while( (oldPriceBar < old_nbBars) && !periodCompleted )
         {
            tempTimestamp = &old_timestamp[oldPriceBar];

            /* Check if we reached an end of period. */
            switch( newPeriod )
            {
            case TA_WEEKLY:
               tempInt  = TA_GetWeekOfTheYear( tempTimestamp );

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
               tempInt  = TA_GetMonth( tempTimestamp );
               tempInt2 = TA_GetYear(tempTimestamp);
               if( (currentMonth != tempInt) ||
                   (currentYear  != tempInt2) )
               {
                  periodCompleted = 1;
                  currentMonth    = tempInt;
                  currentYear     = tempInt2;
               }
               break;
            case TA_QUARTERLY:
               tempInt = TA_GetQuarterOfTheYear( tempTimestamp );
               tempInt2 = TA_GetYear(tempTimestamp);

               if( (currentQuarter != tempInt) ||
                   (currentYear    != tempInt2) )
               {
                  periodCompleted = 1;
                  currentQuarter  = tempInt;
                  currentYear     = tempInt2;
               }
               break;
            case TA_YEARLY:
               tempInt = TA_GetYear( tempTimestamp );
               if( currentYear != tempInt )
               {
                  periodCompleted = 1;
                  currentYear = tempInt;
               }
               break;
            default:
               /* Do nothing */
               break;
            }

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
                  SET_CUR_IF_NOT_NULL( timestamp, old_timestamp[oldPriceBar] );
                  SET_CUR_IF_NOT_NULL( high, max( cur_high, old_high[oldPriceBar]) );
                  SET_CUR_IF_NOT_NULL( low,  min( cur_low,  old_low [oldPriceBar]) );
                  SET_CUR_IF_NOT_NULL( close, old_close[oldPriceBar] );
                  SET_CUR_IF_NOT_NULL( volume, cur_volume+old_volume[oldPriceBar] );
                  SET_CUR_IF_NOT_NULL( openInterest, cur_openInterest+old_openInterest[oldPriceBar] );
                  periodBarAccumulated++;
               }
               else
                  firstIteration = 0;

               /* Move to next bar. */
               oldPriceBar++;
            }
         }

         /* We got all the info needed in the cur_XXXXX variables for
          * proceeding with the initialization of the new period price bar.
          */
         TA_DEBUG_ASSERT( newPriceBar < new_nbBars );

         /* Volume and Open interest are changed to daily average. */
         if( old_volume )
            cur_volume = cur_volume / periodBarAccumulated;
         if( old_openInterest )
            cur_openInterest = cur_openInterest / periodBarAccumulated;

         /* If the timestamp is requested, some adjustment could be
          * needed to cur_timestamp.
          */
         if( timestamp )
         {
            switch( newPeriod )
            {
            case TA_WEEKLY:
                  /* Now something a little bit tricky, we must
                   * make sure that this new price bar is reported
                   * as being the Friday of that week (even if there 
                   * is no price bar for that Friday).
                   */
                  TA_JumpToDayOfWeek( &cur_timestamp, TA_FRIDAY );
                  break;
            case TA_MONTHLY:
                  /* Monthly timestamp always end with the last day of
                   * the month. Even if there was no actual transaction
                   * the last day.
                   */
                  TA_JumpToEndOfMonth( &cur_timestamp );
                  break;
            case TA_QUARTERLY:
                  /* Quarterly timestamp always end with the last day of
                   * the quarter. Even if there was no actual transaction
                   * the last day.
                   * Quarter 1 =  3/31
                   * Quarter 2 =  6/30
                   * Quarter 3 =  9/30
                   * Quarter 4 = 12/31
                   */
                  TA_JumpToEndOfQuarter( &cur_timestamp );
                  break;
            case TA_YEARLY:
                  /* Yearly data always end on 12/31. */
                  TA_JumpToEndOfYear( &cur_timestamp );
                  break;
            default:
                  /* Do nothing. */
                  break;
            }   
         }

         /* The new price bar is initialized here. */
         #define SET_NEW_PERIOD_IF_NOT_NULL(var) { if( new_##var ) { new_##var[newPriceBar] = cur_##var; } }
         SET_NEW_PERIOD_IF_NOT_NULL( timestamp );
         SET_NEW_PERIOD_IF_NOT_NULL( open );
         SET_NEW_PERIOD_IF_NOT_NULL( high );
         SET_NEW_PERIOD_IF_NOT_NULL( low  );
         SET_NEW_PERIOD_IF_NOT_NULL( close );
         SET_NEW_PERIOD_IF_NOT_NULL( volume );
         SET_NEW_PERIOD_IF_NOT_NULL( openInterest );
         #undef SET_NEW_PERIOD_IF_NOT_NULL
                     
         /* This new period bar is completed, move to the next one. */
         newPriceBar++;

         /* Any more data to process? */
         if( oldPriceBar >= old_nbBars)
            again = 0; /* All bars have been processsed. */
      }

      #undef SET_CUR_IF_NOT_NULL
      /* Daily to other timeframe transform is completed. */
   }

   /* All done! Return the final result to the caller. */
   if( open         ) *open         = new_open;
   if( high         ) *high         = new_high;
   if( low          ) *low          = new_low;
   if( close        ) *close        = new_close;
   if( volume       ) *volume       = new_volume;
   if( openInterest ) *openInterest = new_openInterest;
   *timestamp = new_timestamp;
   *nbBars = newPriceBar;

   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
/* None */

