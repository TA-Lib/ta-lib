/* TA-LIB Copyright (c) 1999-2002, Mario Fortier
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
 *  052302 MF   First version.
 *
 */

/* Description:
 *
 */

/**** Headers ****/
#include <string.h>
#include <math.h>
#include "ta_pm.h"
#include "ta_pm_priv.h"
#include "ta_memory.h"
#include "ta_global.h"
#include "ta_magic_nb.h"
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
static TA_RetCode processCache( TA_PMPriv *pmPriv,
                                TA_TradeLogPriv *tradeLog );

static TA_Timestamp *allocTimestampArray( TA_Libc            *libHandle,
                                          const TA_Timestamp *start,
                                          const TA_Timestamp *end,
                                          int                *nbDays );

static int findTimestampIndex( const TA_PMPriv *pmPriv,
                               const TA_Timestamp *exitTimestamp,
                               int *idx );

static TA_RetCode processDailyEquityArray( TA_PMPriv *pmPriv,
                                           TA_PMGroup grp );

static TA_RetCode equityPeriodTransform( TA_Libc       *libHandle,
                                         TA_PMPriv     *pmPriv,
                                         TA_Period      newPeriod, /* The new desired period. */
                                         unsigned int  *nbBars,    /* Return the number of price bar */
                                         TA_Timestamp **timestamp, /* Allocated new timestamp. */
                                         TA_Real      **equity );  /* Allocated new equity. */

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_PMArrayAlloc( TA_PM        *pm,
                            TA_PMArrayId  arrayId,
                            TA_PMGroup    grp,
                            TA_Period     period,
                            TA_PMArray  **allocatedArray )
{
   TA_PMPriv       *pmPriv;
   TA_List         *tradeLogList;
   TA_TradeLogPriv *tradeLogPriv;
   int              timeSerieSize;
   TA_Libc         *libHandle;
   TA_RetCode       retCode;
   TA_PMArray      *newPMArray;
   unsigned int     finalNbBars;
   TA_Real         *finalData;
   TA_Timestamp    *finalTimestamp;

   if( !allocatedArray )
      return TA_BAD_PARAM;

   *allocatedArray = NULL;

   if( !pm || 
       (arrayId >= TA_PM_NB_ARRAYID) ||
       (grp >= TA_PM_NB_GROUP) )
      return TA_BAD_PARAM;

   /* Make sure 'pm' is a ptr on a valid object */
   pmPriv = (TA_PMPriv *)pm->hiddenData; 
   if( pmPriv->magicNb != TA_PMPRIV_MAGIC_NB )
      return TA_BAD_OBJECT;

   libHandle = pmPriv->libHandle;

   #if 0
   /* Get the number of trade that applies to the period.
    * Doing so will also force the update of all
    * "basic calculation" if needed.
    */
   retCode = TA_PMValue( pm, TA_PM_TOTAL_NB_OF_TRADE,
                         TA_PM_ALL_TRADES, &nbTrade );
   if( retCode != TA_SUCCESS )
      return retCode;
   #endif


   /* Because the startDate/endDate are fix in the
    * lifetime of a TA_PM, all the cached time series
    * are allocated once here and freed only when the
    * TA_PM is freed.
    */
   if( !pmPriv->arrayTimestamp )
   {
      /* Allocate the timestamps (excluding week-end)
       * from [startDate..endDate] inclusive.
       * There is only one array of timestamps for all the
       * time series. 
       */
      pmPriv->arrayTimestamp = allocTimestampArray( pmPriv->libHandle,
                                                    &pmPriv->startDate,
                                                    &pmPriv->endDate,
                                                    (int *)&pmPriv->nbDailyBars );
      if( !pmPriv->arrayTimestamp )
         return TA_ALLOC_ERR;
   }


   if( !(pmPriv->flags & TA_PMARRAYCACHE_CALCULATED) )
   {
      /* The cached time serie needs to be recalculated
       * from scratch.
       */
      tradeLogList = &pmPriv->tradeLogList;
      tradeLogPriv = TA_ListAccessHead( tradeLogList );
      
      if( !tradeLogPriv )
         return TA_NO_TRADE_LOG;
      else
      {
         /* Make sure all required cached time series are correctly
          * allocated.
          */
         timeSerieSize = sizeof(TA_Real)*pmPriv->nbDailyBars;
         #define TRY_ALLOC_IF_NULL(x) { \
            if( !x ) \
            { \
               x = TA_Malloc( libHandle, timeSerieSize ); \
               if( !x ) \
                  return TA_ALLOC_ERR; \
            } }

         TRY_ALLOC_IF_NULL( pmPriv->shortArrayCache.investment );
         TRY_ALLOC_IF_NULL( pmPriv->shortArrayCache.profit );
         TRY_ALLOC_IF_NULL( pmPriv->longArrayCache.investment );
         TRY_ALLOC_IF_NULL( pmPriv->longArrayCache.profit );
         #undef TRY_ALLOC_IF_NULL

         /* Reset to zero all the timeseries. */
         memset( pmPriv->shortArrayCache.investment, 0, timeSerieSize );
         memset( pmPriv->shortArrayCache.profit,     0, timeSerieSize );
         memset( pmPriv->longArrayCache.investment,  0, timeSerieSize );
         memset( pmPriv->longArrayCache.profit,      0, timeSerieSize );

         /* Iterate through all the TA_TradeLog */
         do
         {
            if( !(tradeLogPriv->flags & TA_PMARRAYCACHE_CALCULATED) )
               processCache( pmPriv, tradeLogPriv );

            tradeLogPriv = TA_ListAccessNext( tradeLogList );
         } while( tradeLogPriv );
      }

      pmPriv->flags |= TA_PMARRAYCACHE_CALCULATED;
   }
   
   switch( arrayId )
   {
   case TA_PM_ARRAY_EQUITY:
      if( !(pmPriv->flags & TA_EQUITY_CALCULATED) )
      {
         /* Allocate the daily equity. 
          * Keep it cached in "pmPriv->equity".
          */
         retCode = processDailyEquityArray(pmPriv,grp);
         if( retCode != TA_SUCCESS )
            return retCode;
         pmPriv->flags |= TA_EQUITY_CALCULATED;
      }

      /* If requested is not daily, translate to the
       * new period.
       */
      if( period == TA_DAILY )
      {
         finalTimestamp = pmPriv->arrayTimestamp;
         finalData      = pmPriv->equity;
         finalNbBars    = pmPriv->nbDailyBars;
      }
      else
      {
         retCode = equityPeriodTransform( pmPriv->libHandle,
                                          pmPriv, period, &finalNbBars,
                                          &finalTimestamp, &finalData );

         if( retCode != TA_SUCCESS )
            return retCode;
      }
      break;

   /*case TA_PM_ARRAY_RETURNS:
      break;*/

   default:
      return TA_BAD_PARAM;
   }

   TA_ASSERT_RET( pmPriv->libHandle, pmPriv->arrayTimestamp != NULL, TA_UNKNOWN_ERR );
   TA_ASSERT_RET( pmPriv->libHandle, pmPriv->equity != NULL, TA_UNKNOWN_ERR );
   TA_ASSERT_RET( pmPriv->libHandle, finalData != NULL, TA_UNKNOWN_ERR );
   TA_ASSERT_RET( pmPriv->libHandle, finalTimestamp != NULL, TA_UNKNOWN_ERR );
 
   /* At last, allocate and fill up the TA_PMArray. */
   newPMArray = TA_Malloc( libHandle, sizeof( TA_PMArray ) );
   if( !newPMArray )
      return TA_ALLOC_ERR;
   newPMArray->arrayId    = arrayId;
   newPMArray->grp        = grp;
   newPMArray->period     = period;
   newPMArray->data       = finalData;
   newPMArray->timestamp  = finalTimestamp;
   newPMArray->nbData     = finalNbBars;
   newPMArray->hiddenData = pm;

   *allocatedArray = newPMArray;
     
   return TA_SUCCESS;
}

TA_RetCode TA_PMArrayFree( TA_PMArray *toBeFreed )
{
   TA_PMPriv *pmPriv;
   TA_PM *pm;
   TA_Libc *libHandle;

   if( toBeFreed )
   {
      pm = (TA_PM *)toBeFreed->hiddenData;
      if( pm )
      {
         pmPriv = (TA_PMPriv *)pm->hiddenData; 
         if( pmPriv )
         {
            if( pmPriv->magicNb != TA_PMPRIV_MAGIC_NB )
               return TA_BAD_OBJECT;

            libHandle = pmPriv->libHandle;
            if( libHandle )
            {
               if( toBeFreed->period != TA_DAILY ) 
               {
                  if( toBeFreed->timestamp )
                     TA_Free( libHandle, (void *)toBeFreed->timestamp );

                  if( toBeFreed->data )
                     TA_Free( libHandle, (void *)toBeFreed->data );
               }
   
               TA_Free( libHandle, toBeFreed );
            }
         }
      }
   }

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/

static TA_Timestamp *allocTimestampArray( TA_Libc            *libHandle,
                                          const TA_Timestamp *start,
                                          const TA_Timestamp *end,
                                          int                *nbDays )
{
   TA_RetCode    retCode;
   TA_Timestamp *array;
   int outIdx;
   TA_Timestamp curDate;
   TA_DayOfWeek dayOfTheWeek;

   TA_ASSERT_RET( libHandle, TA_TimestampValidate(start) == TA_SUCCESS, (TA_Timestamp *)NULL );
   TA_ASSERT_RET( libHandle, TA_TimestampValidate(end  ) == TA_SUCCESS, (TA_Timestamp *)NULL );
   TA_ASSERT_RET( libHandle, nbDays != NULL, (TA_Timestamp *)NULL );

   /* Calculate the exact number of week days
    * between start and end inclusive.
    * Excluding week-ends.
    */
   retCode = TA_TimestampDeltaWeekday( start, end, (unsigned int *)nbDays );
   if( retCode != TA_SUCCESS )
      return (TA_Timestamp *)NULL;

   /* Allocate the array. Add two element just to be on the safe side. */
   array = TA_Malloc( libHandle, sizeof( TA_Timestamp ) * ((*nbDays)+2) );
   if( !array )
      return (TA_Timestamp *)NULL;

   /* Fill up the array. */
   TA_TimestampCopy( &curDate, start );

   /* Write the start point, if it is a weekday. */
   outIdx = 0;
   dayOfTheWeek = TA_GetDayOfTheWeek( &curDate );
   if( (dayOfTheWeek != TA_SUNDAY) && (dayOfTheWeek != TA_SATURDAY) )
   {
      TA_TimestampCopy( &array[outIdx], &curDate );
      outIdx++;
      TA_NextWeekday( &curDate );
      TA_ASSERT_RET( libHandle, TA_TimestampValidate(&curDate) == TA_SUCCESS, (TA_Timestamp *)NULL );
   }

   /* Count the number of weekday */
   while( TA_TimestampLess( &curDate, end ) )
   {
      TA_TimestampCopy( &array[outIdx], &curDate );
      outIdx++;
      TA_NextWeekday( &curDate );
      TA_ASSERT_RET( libHandle, TA_TimestampValidate(&curDate) == TA_SUCCESS, (TA_Timestamp *)NULL );
   } 

   /* Check if the ending point is a weekday. */
   if( TA_TimestampEqual( &curDate, end ) )
   {
      dayOfTheWeek = TA_GetDayOfTheWeek( &curDate );
      if( (dayOfTheWeek != TA_SUNDAY) && (dayOfTheWeek != TA_SATURDAY) )
         TA_TimestampCopy( &array[outIdx++], &curDate );
   }

   TA_ASSERT_RET( libHandle, outIdx == (*nbDays), (TA_Timestamp *)NULL );

   return array;
}

/* Build the investment/profit time series.
 *
 * Note: A trade is a TA_DataLog with the quantity > 0
 */
static TA_RetCode processCache( TA_PMPriv *pmPriv, TA_TradeLogPriv *tradeLog )
{
   TA_AllocatorForDataLog *allocator;
   TA_DataLogBlock *block;
   TA_List *list;
   TA_DataLog *invalidDataLog, *curDataLog;
   int i, idx;

   TA_Timestamp     *exitTimestamp;
 
   /* Temporary values for calculation. */
   register TA_Real tempReal1, tempReal2;
   register int tempInt1;

   /* Simply iterate through all the 
    * TA_DataLogBlock and update the cached.
    */
   allocator = &tradeLog->allocator;
   if( allocator )
   {
      list = &allocator->listOfDataLogBlock;
      block = TA_ListAccessHead( list );
      while( block )
      {
         /* Process each blocks. */
         invalidDataLog = allocator->nextAvailableTrade;
         curDataLog = block->array;
         for( i=0; i < TA_TRADE_BLOCK_SIZE; i++ )
         {
            if( curDataLog == invalidDataLog )
            {
               break;
            }
            else
            {
               /* Process each TA_DataLog being
                * a trade (not an entry)
                * An entry have a negative 'quantity'.
                */
               tempInt1 = curDataLog->quantity;
               if( tempInt1 > 0 )
               {
                  /* This is a trade. Consider only the trades that are
                   * closed within the TA_PM start/end period.
                   */
                  exitTimestamp = &curDataLog->u.trade.exitTimestamp;
                  if( findTimestampIndex( pmPriv, exitTimestamp, &idx ) )
                  {                   
                     tempReal1 = curDataLog->entryPrice; /* Positive = long, negative = short */
                     tempReal2 = curDataLog->u.trade.profit; /* Positive = winning, negative = losing */
                     if( tempReal1 > 0.0 )
                     {
                        /* This is a long trade. */            
                        pmPriv->longArrayCache.investment[idx] += tempReal1;
                        pmPriv->longArrayCache.profit[idx] += tempReal2;
                     }
                     else
                     {
                        /* This is a short trade. */            
                        pmPriv->shortArrayCache.investment[idx] -= tempReal1;
                        pmPriv->shortArrayCache.profit[idx] += tempReal2;
                     }
                  }
               }
            }
            curDataLog++;
         }
         block = TA_ListAccessNext( list );
      }
   }

   return TA_SUCCESS;
}

static int findTimestampIndex( const TA_PMPriv *pmPriv,
                               const TA_Timestamp *exitTimestamp,
                               int *idx )
{
   const TA_Timestamp *startDate;
   const TA_Timestamp *endDate;
   TA_DayOfWeek dayOfTheWeek;

   #ifdef TA_DEBUG
   TA_Libc *libHandle;
   libHandle = pmPriv->libHandle;
   #endif

   /* Return 0 when no index can be resolved. */
   startDate = &pmPriv->startDate;
   endDate   = &pmPriv->endDate;

   /* Make sure the exitTimestamp is within the start/end date. */ 
   if( (TA_TimestampGreater(exitTimestamp,startDate)&&TA_TimestampLess(exitTimestamp,endDate)) ||
       TA_TimestampEqual(exitTimestamp, startDate) ||
       TA_TimestampEqual(exitTimestamp, endDate) )
   {
      /* Make sure the exitTimestamp is NOT on week-end. Week-end
       * trades are currently ignored.
       */
      dayOfTheWeek = TA_GetDayOfTheWeek( exitTimestamp );
      if( (dayOfTheWeek != TA_SUNDAY) && (dayOfTheWeek != TA_SATURDAY) )
      {
         TA_TimestampDeltaWeekday( startDate, exitTimestamp, (unsigned int *)idx );
         *idx -= 1;

         #ifdef TA_DEBUG
         TA_ASSERT_RET( libHandle, *idx >= 0, 0 );
         TA_ASSERT_RET( libHandle, (unsigned int)*idx < pmPriv->nbDailyBars, 0 );             
         TA_ASSERT_RET( libHandle, TA_TimestampEqual(&pmPriv->arrayTimestamp[*idx], exitTimestamp ), 0 );
         #endif

         return 1;
      }
   }

   /* No index can be found, initialize to zero, just to
    * be safe.
    */
   *idx = 0; 

   return 0;
}

static TA_RetCode processDailyEquityArray( TA_PMPriv *pmPriv, TA_PMGroup grp )
{
   TA_Real *equity;
   unsigned int equityByteSize, nbDailyBars;
   TA_Real *longProfit;
   TA_Real *shortProfit;
   TA_Real  curEquity;
   unsigned int i;

   equityByteSize = sizeof(TA_Real)*pmPriv->nbDailyBars;
   equity = pmPriv->equity;

   /* Allocate the array if not already done. */
   if( !equity )
   {
      pmPriv->equity = TA_Malloc( pmPriv->libHandle, equityByteSize );

      if( !pmPriv->equity )
         return TA_ALLOC_ERR;      

      equity = pmPriv->equity;
   }

   /* Initialize the array to zero. */
   memset( equity, 0, equityByteSize );

   /* Seed the initial value using the user provided
    * value when the TA_PM was allocated.
    */
   curEquity = pmPriv->initialCapital;

   /* Iterate through each day and adjust the 
    * equity using the daily profit/loss.
    */
   nbDailyBars = pmPriv->nbDailyBars;
   switch( grp )
   {
   case TA_PM_ALL_TRADES:
      longProfit  = pmPriv->longArrayCache.profit;
      shortProfit = pmPriv->shortArrayCache.profit;
      for( i=0; i < nbDailyBars; i++ )
      {
         curEquity += longProfit[i] + shortProfit[i];
         equity[i] = curEquity;
      }
      break;

   case TA_PM_SHORT_TRADES:
      shortProfit = pmPriv->shortArrayCache.profit;
      for( i=0; i < nbDailyBars; i++ )
      {
         curEquity += shortProfit[i];
         equity[i] = curEquity;
      }
      break;

   case TA_PM_LONG_TRADES:
      longProfit = pmPriv->longArrayCache.profit;
      for( i=0; i < nbDailyBars; i++ )
      {
         curEquity += longProfit[i];
         equity[i] = curEquity;
      }
      break;
   default:
      TA_Free( pmPriv->libHandle, equity );
      pmPriv->equity = NULL;
      return TA_BAD_PARAM;
   }

   return TA_SUCCESS;
}

static TA_RetCode equityPeriodTransform( TA_Libc       *libHandle,
                                         TA_PMPriv     *pmPriv,
                                         TA_Period      newPeriod, /* The new desired period. */
                                         unsigned int  *nbBars,    /* Return the number of price bar */
                                         TA_Timestamp **timestamp, /* Allocate new timestamp. */
                                         TA_Real      **equity )   /* Allocate new equity. */
{
   TA_PROLOG;

   /* Notice that this function is very similar to the
    * TA_PeriodTransform function in ta_period.c
    *
    * If you find a bug here, may be worth double
    * checking TA_PeriodTransform as well...
    */
   TA_RetCode retCode;

   /* Temporaries. */
   const TA_Timestamp *tempTimestamp;
   unsigned int tempInt, tempInt2;

   /* Variable used to identify period crossing. */
   unsigned int currentWeek, currentMonth, currentYear, currentQuarter;

   /* Pointer on the history being transformed. */
   const TA_Timestamp *old_timestamp;
   const TA_Real      *old_equity;
   TA_Integer          old_nbBars;

   /* Pointer on the transformed data. */
   TA_Timestamp *new_timestamp;    /* New allocated timestamp. */
   TA_Real      *new_equity;       /* New allocated open. */
   TA_Integer    new_nbBars;

   TA_Timestamp  cur_timestamp;    /* Current new timestamp of new period. */
   TA_Real       cur_equity;       /* Current new equity of new period. */

   int oldPriceBar, newPriceBar; /* Array iterators. */
   unsigned int again, periodCompleted; /* Boolean */
   int firstIteration;

   TA_TRACE_BEGIN( libHandle, TA_PeriodTransform );

   /* Validate some mandatory parameter. */
   TA_ASSERT( libHandle, newPeriod != 0 );
   TA_ASSERT( libHandle, nbBars  != NULL );

   /* It is assume that the caller call this function
    * when there is really a transform to do.
    */
   TA_ASSERT( libHandle, newPeriod != TA_DAILY );

   /* Of course, timestamps from the source are needed. */
   TA_ASSERT( libHandle, pmPriv->arrayTimestamp != NULL );

   /* Initialize all callers pointers to NULL.
    * These will be initialize only on success.
    * In the meantime, new_XXXX pointers are
    * going to be used on the new allocated data.
    */
   if( !timestamp || !equity ) 
      return TA_BAD_PARAM;

   *timestamp = NULL;
   *equity    = NULL;
   *nbBars    = 0;

   /* Validate the supported transformation. */

   /* Eliminate all the transform that
    * are currently not supported. 
    * Identify also the major steps
    * needed to perform the transformation.
    */
   switch( newPeriod )
   {
   case TA_WEEKLY:
   case TA_MONTHLY:
   case TA_QUARTERLY:
   case TA_YEARLY:
      /* These are supported. */
      break; 
   default:
      TA_TRACE_RETURN( TA_PERIOD_NOT_AVAILABLE );
   }

   /* OK.. now proceed with the transformations. 
    * The strategy is simple:
    *   The equity for the whole period will be the 
    *   equity at the last daily price bar of 
    *   that period.
    */
   old_timestamp    = &pmPriv->arrayTimestamp[0];
   old_equity       = &pmPriv->equity[0];
   old_nbBars       = pmPriv->nbDailyBars;

   new_timestamp    = NULL;
   newPriceBar      = 0;

   cur_timestamp.date = 0;    /* Current new timestamp of new period. */
   cur_timestamp.time = 0;    /* Current new timestamp of new period. */
   

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
      TA_TRACE_RETURN( TA_UNKNOWN_ERR );
   }

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   new_nbBars += 2; /* To be on the safe side */

   /* Allocate the new data. */      
   new_timestamp = TA_Malloc( libHandle, new_nbBars * sizeof( TA_Timestamp ) );
   if( !new_timestamp )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   new_equity = TA_Malloc( libHandle, new_nbBars * sizeof( TA_Real ) );
   if( !new_equity )
   {
      TA_Free( libHandle, new_timestamp );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
      
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
      cur_timestamp = old_timestamp[oldPriceBar];
      cur_equity    = old_equity   [oldPriceBar];

      /* Go through the bars and accumulate the info
       * until the end of the requested period is reach.
       */
      periodCompleted = 0;
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
               cur_timestamp = old_timestamp[oldPriceBar];
               cur_equity    = old_equity   [oldPriceBar];
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
      TA_DEBUG_ASSERT( libHandle, newPriceBar < new_nbBars );

      /* If the timestamp is requested, some adjustment could be
       * needed to cur_timestamp.
       */
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

      /* The new price bar is initialized here. */
      new_timestamp[newPriceBar] = cur_timestamp;
      new_equity   [newPriceBar] = cur_equity;
                     
      /* This new period bar is completed, move to the next one. */
      newPriceBar++;

      /* Any more data to process? */
      if( oldPriceBar >= old_nbBars)
         again = 0; /* All bars have been processsed. */
   }

   /* All done! Return the final result to the caller. */
   *equity    = new_equity;
   *timestamp = new_timestamp;
   *nbBars    = newPriceBar;

   TA_TRACE_RETURN( TA_SUCCESS );
}
