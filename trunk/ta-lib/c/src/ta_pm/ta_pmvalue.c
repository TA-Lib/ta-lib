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
 *  042002 MF   First version.
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

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#ifndef MAX
   #define MAX(a,b) (a>b?a:b)
#endif

#ifndef MIN
   #define MIN(a,b) (a<b?a:b)
#endif

/* Another max/min macro. That offer the advantage of writing
 * in the memory only when a new max/min is identified.
 */
#define TA_SET_MIN(dst,a) {tempReal3 = a; if( tempReal3 < dst ) dst=tempReal3;}
#define TA_SET_MAX(dst,a) {tempReal3 = a; if( tempReal3 > dst ) dst=tempReal3;}

/**** Local functions declarations.    ****/
static void initValueCache( TA_PMValueCache *dest );
static void mergeValueCache( TA_PMValueCache *dest, const TA_PMValueCache *src );

static TA_RetCode processTradeLog_BasicCalculation( TA_TradeLogPriv *tradeLog );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode TA_PMValue( TA_PM *pm,
                       TA_PMValueId valueId,
                       TA_PMGroup grp,
                       TA_Real *value )
{
   TA_PMPriv         *pmPriv;
   TA_RetCode         retCode;
   TA_List           *tradeLogList;
   TA_TradeLogPriv   *tradeLogPriv;
   unsigned int tempInt, tempInt2;
   TA_Real tempReal, tempReal2, tempReal3;

   /* Validate parameters. */
   if( !pm || !value )
      return TA_BAD_PARAM;
   
   if( (valueId >= TA_PM_NB_VALUEID ) ||
       (grp >= TA_PM_NB_GROUP ) )
      return TA_BAD_PARAM;

   /* Make sure 'pm' is a ptr on a valid object */
   pmPriv = (TA_PMPriv *)pm->hiddenData; 
   if( pmPriv->magicNb != TA_PMPRIV_MAGIC_NB )
      return TA_BAD_OBJECT;
   
   /* Process ALL the basic calculation if not already done. */
   tradeLogList = &pmPriv->tradeLogList;
   tradeLogPriv = TA_ListAccessHead( tradeLogList );

   if( !(pmPriv->flags & TA_PMVALUECACHE_CALCULATED) )
   {
      initValueCache( &pmPriv->longValueCache  );
      initValueCache( &pmPriv->shortValueCache );
      if( !tradeLogPriv )
         return TA_NO_TRADE_LOG;
      else
      {
         do
         {
            if( !(tradeLogPriv->flags & TA_PMVALUECACHE_CALCULATED) )
               retCode = processTradeLog_BasicCalculation( tradeLogPriv );

            mergeValueCache( &pmPriv->longValueCache, &tradeLogPriv->longValueCache );
            mergeValueCache( &pmPriv->shortValueCache, &tradeLogPriv->shortValueCache );
            tradeLogPriv = TA_ListAccessNext( tradeLogList );
         } while( tradeLogPriv );
      }
      pmPriv->flags |= TA_PMVALUECACHE_CALCULATED;
   }

   switch( valueId )
   {
   case TA_PM_TOTAL_NB_OF_TRADE:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->shortValueCache.nbLosingTrade;
         tempInt += pmPriv->longValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->shortValueCache.nbLosingTrade;
         break;
      default:
         return TA_BAD_PARAM;
      }
      *value = (TA_Real)tempInt;
      break;

   case TA_PM_NB_WINNING_TRADE:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbWinningTrade;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbWinningTrade;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         break;
      default:
         return TA_BAD_PARAM;
      }
      *value = (TA_Real)tempInt;
      break;

   case TA_PM_NB_LOSING_TRADE:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt  = pmPriv->shortValueCache.nbLosingTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbLosingTrade;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbLosingTrade;
         break;
      default:
         return TA_BAD_PARAM;
      }
      *value = (TA_Real)tempInt;
      break;

   case TA_PM_TOTAL_NET_PROFIT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->shortValueCache.nbLosingTrade;
         tempInt += pmPriv->longValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal += pmPriv->shortValueCache.sumLoss; /* loss are negative */
         tempReal += pmPriv->longValueCache.sumProfit;
         tempReal += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         tempReal  = pmPriv->longValueCache.sumProfit;
         tempReal += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->shortValueCache.nbLosingTrade;
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal += pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempInt == 0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE; 
      }
      else
         *value = tempReal;
      break;

   case TA_PM_STARTING_CAPITAL:
      *value = pmPriv->initialCapital;
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         break;
      case TA_PM_LONG_TRADES:
      case TA_PM_SHORT_TRADES:
         return TA_VALUE_NOT_APPLICABLE;         
      default:
         return TA_BAD_PARAM;
      }
      break;

   case TA_PM_ENDING_CAPITAL:
      tempReal  = pmPriv->shortValueCache.sumProfit;
      tempReal += pmPriv->shortValueCache.sumLoss; /* loss are negative */
      tempReal += pmPriv->longValueCache.sumProfit;
      tempReal += pmPriv->longValueCache.sumLoss;
      *value = pmPriv->initialCapital + tempReal;
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         break;
      case TA_PM_LONG_TRADES:
      case TA_PM_SHORT_TRADES:
         return TA_VALUE_NOT_APPLICABLE;       
      default:
         return TA_BAD_PARAM;
      }
      break;

   case TA_PM_GROSS_PROFIT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt   = pmPriv->shortValueCache.nbWinningTrade;
         tempInt  += pmPriv->longValueCache.nbWinningTrade;
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal += pmPriv->longValueCache.sumProfit;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbWinningTrade;
         tempReal = pmPriv->longValueCache.sumProfit;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempReal = pmPriv->shortValueCache.sumProfit;
         break;
      default:
         return TA_BAD_PARAM;
      }

      if( tempInt == 0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE; 
      }
      else
         *value = tempReal;
      break;

   case TA_PM_GROSS_LOSS:
      /* loss are negative */
      switch( grp )
      {
      case TA_PM_ALL_TRADES:         
         tempInt   = pmPriv->shortValueCache.nbLosingTrade;
         tempInt  += pmPriv->longValueCache.nbLosingTrade;
         tempReal  = pmPriv->shortValueCache.sumLoss; 
         tempReal += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbLosingTrade;
         tempReal = pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbLosingTrade;
         tempReal = pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempInt == 0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE; 
      }
      else
         *value = tempReal;
      break;

   case TA_PM_PROFIT_FACTOR:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->shortValueCache.nbLosingTrade;
         tempInt += pmPriv->longValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         tempReal   = pmPriv->shortValueCache.sumProfit;
         tempReal  += pmPriv->longValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumLoss;
         tempReal2 += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempInt  = pmPriv->longValueCache.nbWinningTrade;
         tempInt += pmPriv->longValueCache.nbLosingTrade;
         tempReal   = pmPriv->longValueCache.sumProfit;
         tempReal2  = pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt += pmPriv->shortValueCache.nbLosingTrade;
         tempReal   = pmPriv->shortValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( (tempReal2 >= 0.0) || (tempReal <= 0.0) )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal/(-tempReal2);
      break;

   case TA_PM_AVG_PROFIT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt   = pmPriv->shortValueCache.nbWinningTrade;
         tempInt  += pmPriv->longValueCache.nbWinningTrade;
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal += pmPriv->longValueCache.sumProfit;
         break;
      case TA_PM_LONG_TRADES:
         tempInt   = pmPriv->longValueCache.nbWinningTrade;
         tempReal  = pmPriv->longValueCache.sumProfit;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt   = pmPriv->shortValueCache.nbWinningTrade;
         tempReal  = pmPriv->shortValueCache.sumProfit;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempInt <= 0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal/((TA_Real)tempInt);
      break;

   case TA_PM_PERCENT_PROFITABLE:

      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt2  = pmPriv->shortValueCache.nbWinningTrade;
         tempInt2 += pmPriv->longValueCache.nbWinningTrade;
         tempInt   = pmPriv->shortValueCache.nbLosingTrade;
         tempInt  += pmPriv->longValueCache.nbLosingTrade;
         break;
      case TA_PM_LONG_TRADES:
         tempInt2 = pmPriv->longValueCache.nbWinningTrade;
         tempInt  = pmPriv->longValueCache.nbLosingTrade;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt2 = pmPriv->shortValueCache.nbWinningTrade;
         tempInt  = pmPriv->shortValueCache.nbLosingTrade;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempInt == 0 )      
      {
         /* No losing trades. */
         if( tempInt2 == 0 )
         {
            /* No trades at all */
            *value = (TA_Real)0.0;
            return TA_VALUE_NOT_APPLICABLE;
         }
         /* All trades are winning trades. */
         *value = (TA_Real)100.0;
      }
      else
      {
         tempInt += tempInt2;
         *value = ((TA_Real)tempInt2)/((TA_Real)tempInt)*100.0;
      }
      break;
   
   case TA_PM_AVG_PROFIT_PERCENT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal   = pmPriv->shortValueCache.sumProfit;
         tempReal  += pmPriv->longValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumInvestmentProfit;
         tempReal2 += pmPriv->longValueCache.sumInvestmentProfit;
         break;
      case TA_PM_LONG_TRADES:
         tempReal  = pmPriv->longValueCache.sumProfit;
         tempReal2 = pmPriv->longValueCache.sumInvestmentProfit;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumInvestmentProfit;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal2 == 0.0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = (tempReal/tempReal2)*100.0;
      break;

   case TA_PM_AVG_LOSS:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempInt   = pmPriv->shortValueCache.nbLosingTrade;
         tempInt  += pmPriv->longValueCache.nbLosingTrade;
         tempReal  = pmPriv->shortValueCache.sumLoss;
         tempReal += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempInt   = pmPriv->longValueCache.nbLosingTrade;
         tempReal  = pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempInt   = pmPriv->shortValueCache.nbLosingTrade;
         tempReal  = pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempInt <= 0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal/((TA_Real)tempInt);
      break;

   case TA_PM_AVG_LOSS_PERCENT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal   = pmPriv->shortValueCache.sumLoss;
         tempReal  += pmPriv->longValueCache.sumLoss;
         tempReal2  = pmPriv->shortValueCache.sumInvestmentLoss;
         tempReal2 += pmPriv->longValueCache.sumInvestmentLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempReal  = pmPriv->longValueCache.sumLoss;
         tempReal2 = pmPriv->longValueCache.sumInvestmentLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal  = pmPriv->shortValueCache.sumLoss;
         tempReal2 = pmPriv->shortValueCache.sumInvestmentLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal2 == 0.0 )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = (tempReal/tempReal2)*100.0;
      break;

   case TA_PM_LARGEST_PROFIT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MAX( pmPriv->shortValueCache.largestProfit, 
                         pmPriv->longValueCache.largestProfit );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.largestProfit;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.largestProfit;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal == TA_REAL_MIN )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal;
      break;

   case TA_PM_LARGEST_LOSS:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MIN( pmPriv->shortValueCache.largestLoss, 
                         pmPriv->longValueCache.largestLoss );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.largestLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.largestLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }

      if( tempReal == TA_REAL_MAX )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal;
      break;

   case TA_PM_SMALLEST_PROFIT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MIN( pmPriv->shortValueCache.smallestProfit, 
                         pmPriv->longValueCache.smallestProfit );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.smallestProfit;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.smallestProfit;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal == TA_REAL_MAX )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal;
      break;

   case TA_PM_SMALLEST_LOSS:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MAX( pmPriv->shortValueCache.smallestLoss, 
                         pmPriv->longValueCache.smallestLoss );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.smallestLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.smallestLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal == TA_REAL_MIN )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal;
      break;

   case TA_PM_LARGEST_PROFIT_PERCENT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MAX( pmPriv->shortValueCache.largestProfitPercent, 
                         pmPriv->longValueCache.largestProfitPercent );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.largestProfitPercent;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.largestProfitPercent;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal == TA_REAL_MIN )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal*100.0;      
      break;

   case TA_PM_LARGEST_LOSS_PERCENT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MIN( pmPriv->shortValueCache.largestLossPercent, 
                         pmPriv->longValueCache.largestLossPercent );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.largestLossPercent;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.largestLossPercent;
         break;
      default:
         return TA_BAD_PARAM;
      }

      if( tempReal == TA_REAL_MAX )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal*100.0;      
      break;

   case TA_PM_SMALLEST_PROFIT_PERCENT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MIN( pmPriv->shortValueCache.smallestProfitPercent, 
                         pmPriv->longValueCache.smallestProfitPercent );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.smallestProfitPercent;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.smallestProfitPercent;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal == TA_REAL_MAX )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal*100.0;      
      break;

   case TA_PM_SMALLEST_LOSS_PERCENT:
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal = MAX( pmPriv->shortValueCache.smallestLossPercent, 
                         pmPriv->longValueCache.smallestLossPercent );
         break;
      case TA_PM_LONG_TRADES:
         tempReal = pmPriv->longValueCache.smallestLossPercent;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal = pmPriv->shortValueCache.smallestLossPercent;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( tempReal == TA_REAL_MIN )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = tempReal*100.0;      
      break;

   case TA_PM_RATE_OF_RETURN:
      /* One-period rate of return: (profit / initialCapital) */
      tempReal3 = pmPriv->initialCapital;
      if( tempReal3 <= 0.0 )
      {
         *value = 0.0;
         return TA_BAD_STARTING_CAPITAL;
      }
                       
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal   = pmPriv->shortValueCache.sumProfit;
         tempReal  += pmPriv->longValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumLoss; /* loss are negative */
         tempReal2 += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempReal  = pmPriv->longValueCache.sumProfit;
         tempReal2 = pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal2 = pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }
      if( (tempReal <= 0.0) && (tempReal2 >= 0.0) )
      {
         *value = 0.0;     
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
         *value = ((tempReal+tempReal2)/tempReal3)*100.0;
      break;

   case TA_PM_ANNUALIZED_RETURN:
       /* Annualized rate of return on a simple-interest basis:
        *
        *   (Ending Value - Starting Value)     365
        *   -------------------------------  *  ---
        *           Starting Value               n
        *
        * Where 'n' is the number of day between the end and 
        * start date when the TA_PM was created.
        */
      tempReal3 = pmPriv->initialCapital;
      if( tempReal3 <= 0.0 )
      {
         *value = 0.0;
         return TA_BAD_STARTING_CAPITAL;
      }
                       
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal   = pmPriv->shortValueCache.sumProfit;
         tempReal  += pmPriv->longValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumLoss; /* loss are negative */
         tempReal2 += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempReal  = pmPriv->longValueCache.sumProfit;
         tempReal2 = pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal2 = pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }

      retCode = TA_TimestampDeltaDay( &pmPriv->startDate,
                                      &pmPriv->endDate,
                                      &tempInt );
      if( retCode != TA_SUCCESS )
         return retCode;

      if( ((tempReal <= 0.0) && (tempReal2 >= 0.0)) || (tempInt == 0) )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
      {
         tempReal += tempReal2;
         *value = (tempReal/tempReal3)*(36500.0/tempInt);
      }
      break;

   case TA_PM_ANNUALIZED_COMPOUNDED_RETURN:
      /* Annualized compounded rate of return:
       *
       * ((Ending Value / Starting Value)^(1/y)) - 1
       *
       * Where 'y' is the number of year between the end
       * and start date when the TA_PM was created.
       */
      tempReal3 = pmPriv->initialCapital;
      if( tempReal3 <= 0.0 )
      {
         *value = 0.0;
         return TA_BAD_STARTING_CAPITAL;
      }
                 
      switch( grp )
      {
      case TA_PM_ALL_TRADES:
         tempReal   = pmPriv->shortValueCache.sumProfit;
         tempReal  += pmPriv->longValueCache.sumProfit;
         tempReal2  = pmPriv->shortValueCache.sumLoss; /* loss are negative */
         tempReal2 += pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_LONG_TRADES:
         tempReal  = pmPriv->longValueCache.sumProfit;
         tempReal2 = pmPriv->longValueCache.sumLoss;
         break;
      case TA_PM_SHORT_TRADES:
         tempReal  = pmPriv->shortValueCache.sumProfit;
         tempReal2 = pmPriv->shortValueCache.sumLoss;
         break;
      default:
         return TA_BAD_PARAM;
      }

      retCode = TA_TimestampDeltaDay( &pmPriv->startDate,
                                      &pmPriv->endDate,
                                      &tempInt );
      if( retCode != TA_SUCCESS )
         return retCode;

      if( ((tempReal <= 0.0) && (tempReal2 >= 0.0)) || (tempInt == 0) )
      {
         *value = 0.0;
         return TA_VALUE_NOT_APPLICABLE;
      }
      else
      {
         tempReal += (tempReal2+tempReal3);
         *value = (pow(tempReal/tempReal3,1.0/(tempInt/365.0)) - 1.0)*100.0;
      }
      break;

   default:
      return TA_INVALID_VALUE_ID;
   }   

   return TA_SUCCESS;
}

TA_RetCode TA_PMErrMargin( TA_PM *pm,
                           TA_PMValueId valueId,
                           TA_PMGroup grp,
                           TA_Real *value )
{
   (void)pm;
   (void)valueId;
   (void)grp;
   (void)value;
   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
/* Perform the very first level of calculation among all
 * the trades allocated.
 *
 * Note: A trade is a TA_DataLog with the quantity > 0
 */
static TA_RetCode processTradeLog_BasicCalculation( TA_TradeLogPriv *tradeLog )
{
   TA_AllocatorForDataLog *allocator;
   TA_DataLogBlock *block;
   TA_List *list;
   TA_DataLog *invalidDataLog, *curDataLog;
   int i;

   TA_PMValueCache *shortValueCache, *longValueCache;
 
   /* Temporary values for calculation. */
   register TA_Real tempReal1, tempReal2, tempReal3;
   register int tempInt1;

   /* The following varaiables are all the
    * accumulators.
    *
    * Some are suggested to be kept in
    * registers, most of the others will
    * be maintain within the local TA_PMValueCache.
    *
    * All these value are then merge within the
    * tradeLog at the very end.
    */
   register int     long_nbLosingTrade;
   register int     short_nbLosingTrade;
   register int     long_nbWinningTrade;
   register int     short_nbWinningTrade;   
   TA_PMValueCache  shortV, longV;


   /* Initialize all accumulators. */
   initValueCache( &shortV );
   initValueCache( &longV );

   long_nbLosingTrade    =
   short_nbLosingTrade   =
   long_nbWinningTrade   =
   short_nbWinningTrade  = 0;   

   /* Simply iterate through all the TA_TradeLog
    * and update the accumulators.
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
                  tempReal1 = curDataLog->entryPrice; /* Positive = long, negative = short */
                  tempReal2 = curDataLog->u.trade.profit; /* Positive = winning, negative = losing */
                  if( tempReal1 > 0.0 )
                  {
                     /* This is a long trade. */            
                     if( tempReal2 > 0.0 )
                     {
                        /* This is a winning long trade */
                        longV.sumInvestmentProfit += tempReal1;
                        longV.sumProfit += tempReal2;
                        TA_SET_MAX(longV.largestProfit, tempReal2 );
                        TA_SET_MIN(longV.smallestProfit, tempReal2 );
                        tempReal1 = tempReal2/tempReal1;
                        TA_SET_MAX(longV.largestProfitPercent, tempReal1 );
                        TA_SET_MIN(longV.smallestProfitPercent, tempReal1 );
                        long_nbWinningTrade++;
                     }
                     else
                     {
                        /* This is a losing long trade */
                        longV.sumInvestmentLoss += tempReal1;
                        longV.sumLoss += tempReal2;
                        TA_SET_MIN(longV.largestLoss, tempReal2 );
                        TA_SET_MAX(longV.smallestLoss, tempReal2 );
                        tempReal1 = tempReal2/tempReal1;
                        TA_SET_MIN(longV.largestLossPercent, tempReal1 );
                        TA_SET_MAX(longV.smallestLossPercent, tempReal1 );
                        long_nbLosingTrade++;
                     }
                  }
                  else
                  {
                     /* This is a short trade. */
                     if( tempReal2 > 0.0 )
                     {
                        /* This is a winning short trade */
                        tempReal1 = -tempReal1;
                        shortV.sumInvestmentProfit += tempReal1;
                        shortV.sumProfit += tempReal2;
                        TA_SET_MAX(shortV.largestProfit, tempReal2 );
                        TA_SET_MIN(shortV.smallestProfit, tempReal2 );
                        tempReal1 = tempReal2/tempReal1;
                        TA_SET_MAX(shortV.largestProfitPercent, tempReal1 );
                        TA_SET_MIN(shortV.smallestProfitPercent, tempReal1 );
                        short_nbWinningTrade++;
                     }
                     else
                     {
                        /* This is a losing short trade */
                        tempReal1 = -tempReal1;
                        shortV.sumInvestmentLoss += tempReal1;
                        shortV.sumLoss += tempReal2;
                        TA_SET_MIN(shortV.largestLoss, tempReal2 );
                        TA_SET_MAX(shortV.smallestLoss, tempReal2 );
                        tempReal1 = tempReal2/tempReal1;
                        TA_SET_MIN(shortV.largestLossPercent, tempReal1 );
                        TA_SET_MAX(shortV.smallestLossPercent, tempReal1 );
                        short_nbLosingTrade++;
                     }
                  }
               }
            }
            curDataLog++;
         }
         block = TA_ListAccessNext( list );
      }
   }

   /* Initialize the output with the accumulated results. */
   shortValueCache  = &tradeLog->shortValueCache;
   longValueCache   = &tradeLog->longValueCache;
   *shortValueCache = shortV;
   *longValueCache  = longV;

   shortValueCache->nbLosingTrade = short_nbLosingTrade;
   shortValueCache->nbWinningTrade = short_nbWinningTrade;
   longValueCache->nbLosingTrade = long_nbLosingTrade;
   longValueCache->nbWinningTrade = long_nbWinningTrade;

   /* Indicate that the value caches are now calculated. */
   tradeLog->flags |= TA_PMVALUECACHE_CALCULATED;
   return TA_SUCCESS;
}

static void mergeValueCache( TA_PMValueCache *dest, const TA_PMValueCache *src )
{
   TA_Real tempReal3;

   dest->nbLosingTrade  += src->nbLosingTrade;
   dest->nbWinningTrade += src->nbWinningTrade;

   dest->sumProfit      += src->sumProfit;
   dest->sumLoss        += src->sumLoss;

   dest->sumInvestmentLoss   += src->sumInvestmentLoss;
   dest->sumInvestmentProfit += src->sumInvestmentProfit;

   TA_SET_MAX( dest->largestProfit, src->largestProfit );
   TA_SET_MAX( dest->smallestLoss, src->smallestLoss );

   TA_SET_MAX( dest->largestProfitPercent, src->largestProfitPercent );
   TA_SET_MAX( dest->smallestLossPercent, src->smallestLossPercent );

   TA_SET_MIN( dest->largestLoss, src->largestLoss );
   TA_SET_MIN( dest->smallestProfit, src->smallestProfit );

   TA_SET_MIN( dest->largestLossPercent, src->largestLossPercent );
   TA_SET_MIN( dest->smallestProfitPercent, src->smallestProfitPercent );

}

static void initValueCache( TA_PMValueCache *dest )
{
   memset( dest, 0, sizeof( TA_PMValueCache ) );

   dest->largestProfit  = TA_REAL_MIN;
   dest->largestLoss    = TA_REAL_MAX;
   dest->smallestProfit = TA_REAL_MAX;
   dest->smallestLoss   = TA_REAL_MIN;

   dest->largestProfitPercent  = TA_REAL_MIN;
   dest->largestLossPercent    = TA_REAL_MAX;
   dest->smallestProfitPercent = TA_REAL_MAX;
   dest->smallestLossPercent   = TA_REAL_MIN;
}
