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
#ifndef TA_PM_H
#define TA_PM_H

#ifndef TA_COMMON_H
    #include "ta_common.h"
#endif

#ifndef TA_DATA_H
    #include "ta_data.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   /* Allows to make a unique identifier for an instrument.
    *
    * You can use one or two strings.
    *
    * If you prefer you can set both strings to NULL
    * and use instead a user defined integer value.
    *
    * userKey is ignore whenever catString or symString
    * are != NULL.
    */
   const char *catString;
   const char *symString;
   int userKey;
} TA_Instrument;

typedef enum
{
  TA_LONG_ENTRY,
  TA_LONG_EXIT,
  TA_SHORT_ENTRY,
  TA_SHORT_EXIT,
  TA_NB_TRADE_TYPE
} TA_TransactionType;

/* The TA_Transaction is used exclusively as a way to 
 * pass the parameters to the TA_TradeLogAdd function.
 */
typedef struct
{
   TA_TransactionType type;

   /* TA_Instrument is a unique identifier that allows to
    * distinguish one instrument from another.
    *
    * When 'id' is NULL, all the transaction are assume to refer
    * to the same unspecified instrument.
    *
    * When applicable, the relevant content of the 'id' is
    * copied. Consequently, the TA_Instrument and its inner
    * strings can be freed after a TA_TradeLogAdd call.
    */
   TA_Instrument        *id;

   /* The "when" and "how much" about the transaction */
   TA_Timestamp         timestamp;
   int                  quantity;
   TA_Real              price;

   /* You can provide the historic prices.
    * This is used only for EXIT transaction.
    *
    * Maximum adverse excursion values are
    * derived from this information.
    *
    * Leave these pointers to NULL if
    * you do not care for MAE measurements.
    *
    * It is assumed that highPrice[0] and lowPrice[0] 
    * are for the bar where the ENTRY occured.
    * 
    * It is assumed that highPrice[nbPriceBar-1] and
    * lowPrice[nbPriceBar-1] are for the bar where
    * the EXIT occured.
    *
    * These pointers are used solely while
    * the call to TA_TradeLogAdd, so you
    * can free the data after the call.
    */
   int nbPriceBar;
   double *highPrice;
   double *lowPrice;

} TA_Transaction;

typedef struct
{
   /* Implementation is hidden. */
   void *hiddenData;
} TA_TradeLog;

typedef struct
{
   /* Implementation is hidden. */
   void *hiddenData;   
} TA_PM;

/* Functions for TA_TradeLog
 *
 * TA_TradeLog Design Limitation:
 *
 *  - The TA_TradeLog can not be freed while being part
 *    of a TA_PM. Attempt to free the TA_TradeLog will
 *    fail.
 * 
 *  - You can not add new transaction to a TA_Tradelog
 *    while it is being part of a TA_PM. Attempt to add
 *    transactions will fail.
 *
 * Once all the related TA_PM are freed, these limitations
 * do not exist anymore and the TA_TradeLog can be freed
 * or have new transactions added.
 *
 * These limitations allow some speed optimization useful in
 * the context of TA application doing repetitive measurements 
 * for parameter optimization.
 */
TA_RetCode TA_TradeLogAlloc( TA_TradeLog **allocatedTradeLog );

TA_RetCode TA_TradeLogFree( TA_TradeLog *toBeFreed );

TA_RetCode TA_TradeLogAdd( TA_TradeLog *tradeLog,
                           const TA_Transaction *newTransaction );

/* Functions for TA_PM
 *
 * These functions allows to do the measurements on one
 * or many TA_TradeLog over a specific period of time.
 *
 * You can add the same TA_TradeLog to multiple independent TA_PM.
 * This might be useful if you want measurements related to one or
 * a subset of instrument and at the same time get the measurements
 * for the whole portfolio.
 */
TA_RetCode TA_PMAlloc( const TA_Timestamp  *startDate,
                       const TA_Timestamp  *endDate,
                       TA_Real              initialCapital,
                       TA_PM              **allocatedPM );

TA_RetCode TA_PMFree( TA_PM *toBeFreed );
TA_RetCode TA_PMAddTradeLog( TA_PM *pm, TA_TradeLog *tradeLogToAdd );

/* Measurements are obtained with the 'TA_PMValue' function. */
typedef enum 
{
   TA_PM_LONG_TRADES,
   TA_PM_SHORT_TRADES,
   TA_PM_ALL_TRADES,
   TA_PM_NB_GROUP
} TA_PMGroup;

typedef enum
{
   /*******************************
    * Measurements including both *
    * winning and losing trades.  *
    *******************************/
   TA_PM_TOTAL_NB_OF_TRADE,

   /* Capital at the beginning of the measured period. */
   TA_PM_STARTING_CAPITAL,

   /* Capital at the end of the measured period. */
   TA_PM_ENDING_CAPITAL,

   /* Profit minus loss of all trades. */
   TA_PM_TOTAL_NET_PROFIT,

   /* Gross profit divided by gross loss. */
   TA_PM_PROFIT_FACTOR,

   /* Nb winning trade divided by number of trades. */
   TA_PM_PERCENT_PROFITABLE,

   /* One-period rate of return:
    *   (Ending Value / Starting Value) - 1
    */
   TA_PM_RATE_OF_RETURN,

   /* Annualized rate of return on a simple-interest basis:
    *
    *   (Ending Value - Starting Value)     365
    *   -------------------------------  *  ---
    *           Starting Value               n
    *
    * Where 'n' is the number of day between the end and 
    * start date when the TA_PM was created.
    */
   TA_PM_ANNUALIZED_RETURN,

   /* Annualized compounded rate of return:
    *
    * ((Ending Value / Starting Value)^(1/y)) - 1
    *
    * Where 'y' is the number of year between the end
    * and start date when the TA_PM was created.
    */
   TA_PM_ANNUALIZED_COMPOUNDED_RETURN,

   /***************************************
    * Winning trade related measurements. *
    ***************************************/
   TA_PM_NB_WINNING_TRADE,

   /* Summation of all winning trades. */
   TA_PM_GROSS_PROFIT,

   /* Average profit per trade (for all winning trade). */
   TA_PM_AVG_PROFIT,         
   TA_PM_AVG_PROFIT_PERCENT,

   /* Trade with the largest profit. */
   TA_PM_LARGEST_PROFIT, 
   TA_PM_LARGEST_PROFIT_PERCENT,

   /**************************************
    * Losing trade related measurements. *
    **************************************/
   TA_PM_NB_LOSING_TRADE,

   /* Summation of all losing trades. */
   TA_PM_GROSS_LOSS,

   /* Average loss per trade (for all losing trades). */
   TA_PM_AVG_LOSS,         
   TA_PM_AVG_LOSS_PERCENT,

   /* Trade with the largest lost. */
   TA_PM_LARGEST_LOSS,  
   TA_PM_LARGEST_LOSS_PERCENT,

   TA_PM_NB_VALUEID
} TA_PMValueId;

/* The following functions allows to extract a single 
 * value covering the whole period who was specified
 * when TA_PM was created.
 */
TA_RetCode TA_PMValue( TA_PM        *pm,
                       TA_PMValueId  valueId,
                       TA_PMGroup    grp,
                       TA_Real      *value );

/* Return a string corresponding to a TA_PMValueId.
 * Aim at being used in a user interface.
 *
 * Will always return a null terminated string.
 * On error will return the string TA_PMSTRING_ERROR.
 */
#define TA_PMSTRING_ERROR "#VALUE!"
const char *TA_PMValueIdString( TA_PMValueId valueId );

/* Return a short text about a given TA_PMValueId */
const char *TA_PMValueIdHint( TA_PMValueId valueId );

/* Return some flags who might help to make
 * the TA_PMValueId more meaningful.
 */
#define TA_PM_VALUE_ID_IS_CURRENCY      0x00000001
#define TA_PM_VALUE_ID_IS_PERCENT       0x00000002
#define TA_PM_VALUE_ID_IS_INTEGER       0x00000004

/* Each measurements must be in at least one of the 
 * following four category.
 */
#define TA_PM_VALUE_ID_GENERAL          0x00000100
#define TA_PM_VALUE_ID_LOSING_RELATED   0x00000200
#define TA_PM_VALUE_ID_WINNING_RELATED  0x00000400
#define TA_PM_VALUE_ID_NOT_RECOMMENDED  0x00000800

unsigned int TA_PMValueIdFlags( TA_PMValueId valueId );

/* Some measurement can also be requested for the whole trading
 * period (like monthly/daily returns, the equity line etc...).
 * These are returned as a time serie and should be freed by
 * the caller when not needed anymore.
 */ 
typedef enum
{
   TA_PM_ARRAY_EQUITY,
   /* TA_PM_ARRAY_RETURNS,*/
   TA_PM_NB_ARRAYID
} TA_PMArrayId;

typedef struct
{
   /* Never modify anything in that stucture. */

   /* This is the time serie. */
   TA_Integer          nbData;
   const TA_Timestamp *timestamp;
   const TA_Real      *data;

   /* Parameter who were used when this
    * PM array was allocated.
    */
   TA_PMArrayId arrayId;
   TA_PMGroup   grp;
   TA_Period    period;

   /* Private data. Never access this */
   void *hiddenData;
} TA_PMArray;

/* Functions to Allocate/Free a TA_PMArray.
 *
 * The date range is the one who was defined when TA_PM
 * was created with [startDate..endDate].
 *
 * Week-end days are not included in the array.
 *
 * An example displaying the daily equity line values
 * when considering all the trades:
 *
 *       TA_RetCode retCode;
 *       TA_PMArray *equity;
 *
 *       retCode = TA_PMArrayAlloc( pm, TA_PM_ARRAY_EQUITY,
 *                                  TA_PM_ALL_TRADES,
 *                                  TA_DAILY, &equity );
 *
 *       if( retCode == TA_SUCCESS )
 *       {
 *          for( i=0; i < equity->nbData; i++ )
 *             printf( "%d-%d-%d %g$\n",
 *                     TA_Month( &equity->timestamp[i] ),
 *                     TA_Day  ( &equity->timestamp[i] ),
 *                     TA_Year ( &equity->timestamp[i] ),
 *                     equity->data[i] );
 *       }
 *
 */
TA_RetCode TA_PMArrayAlloc( TA_PM        *pm,
                            TA_PMArrayId  arrayId,
                            TA_PMGroup    grp,
                            TA_Period     period,
                            TA_PMArray  **allocatedArray );

TA_RetCode TA_PMArrayFree( TA_PMArray *toBeFreed );

/* Functions for TA_PMReport
 *
 * Allows to allocate/free a report into a memory buffer or
 * output to a file.
 *
 * Design limitation:
 *   - You cannot free a TA_PM while a TA_TradeReport is 
 *     still allocated. Attempt to free the TA_PM will fail.
 *
 * In other word, you have to free all the TA_PMReport before 
 * freeing the corresponding TA_PM.
 *
 */
typedef struct
{
   /* The report is in the 'buffer'.
    *
    * Each line have the same length and are 
    * null terminated.
    *
    * Example to display the report on the console:
    *
    *   TA_PMReport *theReport;
    *
    *   TA_PMReportAlloc( pm, &theReport );
    *
    *   for( i=0; i < theReport->nbLine; i++ )
    *      printf( "%s\n", theReport->buffer+(i*theReport->lineLength) );
    *
    */   
   unsigned int nbLine;
   unsigned int lineLength;
   const char  *buffer;
   
   /* Used internally by TA-Lib. Do not modify. */
   void *hiddenData;
} TA_PMReport;

TA_RetCode TA_PMReportAlloc( TA_PM *pm, TA_PMReport **newAllocatedReport );
TA_RetCode TA_PMReportFree ( TA_PMReport *reportToBeFreed );

/* Append all measurements into a file. 
 *
 * The report is in text and formatted for
 * being printer friendly.
 */
TA_RetCode TA_PMReportToFile( TA_PM *pm, FILE *out ); 

/* Save the TA_PM into a file.
 *
 * The report is written in binary and is design for
 * being read back with TA_PMRead.
 *
 * Keep in mind that this read back TA_PM cannot
 * have further TA_TradeLog added to it. This
 * TA_PM can be used only to generate reports.
 */
TA_RetCode TA_PMWrite( FILE *out, const TA_PM *pmToWrite );
TA_RetCode TA_PMRead ( FILE *in, TA_PM **newAllocatedPM );

/* Functions for TA_TradeReport
 *
 * Allows to get info trade-by-trade.
 *
 * Design limitation:
 *   - You cannot free a TA_PM while a TA_TradeReport is
 *     still allocated. Attempt to free the TA_PM will fail.
 *
 * In other word, you have to free all the TA_TradeReport before 
 * freeing the corresponding TA_PM.
 * 
 * Here is an example displaying many fields of the report:
 *
 *
 * void displayAllTrades( TA_PM *thePM )
 * {
 *    TA_RetCode retCode;
 *    TA_PMTradeReport *report;
 *
 *    retCode = TA_PMTradeReportAlloc( thePM, &report );
 *
 *    if( retCode == TA_SUCCESS )
 *    {
 *       printf( "Entry Price   Exit Price   Profit   Qty   MAE   MinFE" );
 *       printf( "=====================================================" );
 *       for( i=0; i < report->nbTrades; i++ )
 *       {
 *          ...
 *       }
 *       TA_PMTradeReportFree( report );
 *    }
 * }
 */
typedef struct
{
   int quantity;

   double entryPrice; /* Positive=long, negative=short */

   TA_Timestamp entryTimestamp;
   TA_Timestamp exitTimestamp;

   double profit; /* Positive=Winning, negative=Losing */

   /* Identify the instrument */
   TA_Instrument *id;     

   /* Excursion info (in percent from entry price).
    *
    * If price info was not available, it is not possible
    * to calculate these so they are going to be set
    * to TA_REAL_DEFAULT (see ta_defs.h).
    *
    * These calculations are the same as defined by
    * John Sweeney in his books:
    *
    *
    *
    */
   double mae;   /* Maximum Adverse Excursion   */
   double minfe; /* Minimum Favorable Excursion */
   double maxfe; /* Maximum Favorable Excursion */

} TA_Trade;

typedef struct
{
   unsigned int nbTrades;
   const TA_Trade **trades;

   /* Used internally by TA-Lib. Do not modify. */
   void *hiddenData;
} TA_TradeReport;

TA_RetCode TA_PMTradeReportAlloc( TA_PM *pm, TA_TradeReport **tradeReportAllocated );
TA_RetCode TA_PMTradeReportFree ( TA_PM *pm, TA_TradeReport *tradeReport );

#ifdef __cplusplus
}
#endif

#endif
