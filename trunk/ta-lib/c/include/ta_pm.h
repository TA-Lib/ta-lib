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
   /* This structure must be initialized only
    * by using the following functions:
    *      TA_InstrumentInit
    *             OR
    *      TA_InstrumentInitWithUserKey
    *
    * Data "between the transaction" can
    * be attach by using only:
    *      TA_InstrumentAttachData
    *             OR
    *      TA_InstrumentAttachUDBase
    */

   /* Application should not access directly these
    * structure members.
    */
   #define TA_INSTRUMENT_USE_UDBASE       0x00000001
   #define TA_INSTRUMENT_USE_USERDATA     0x00000002
   #define TA_INSTRUMENT_USE_CATSTRING    0x00000004
   #define TA_INSTRUMENT_USE_SYMSTRING    0x00000008
   #define TA_INSTRUMENT_USE_USERKEY      0x00000010
   #define TA_INSTRUMENT_USE_CATSYMSTRING 0x0000000C
   unsigned int flags;

   union DataSource
   {
      TA_UDBase *udBase;
      struct UserData
      {
         unsigned int        nbPrice;
         const TA_Real      *price;
         const TA_Real      *lowPrice;
         const TA_Real      *highPrice;
         const TA_Timestamp *timestamp;
      } userData;
   } ds;

   union InstrumentKey
   {
      struct CatSym
      {
         const char   *catString;
         const char   *symString;
      } catSym;
      int userKey;
   } key;

} TA_Instrument;

typedef enum
{
  TA_LONG_ENTRY,
  TA_LONG_EXIT,
  TA_SHORT_ENTRY,
  TA_SHORT_EXIT,
  TA_NB_TRADE_TYPE
} TA_TransactionType;

typedef struct
{
   TA_TransactionType   type;
   const TA_Instrument *id;
   TA_Timestamp         timestamp;
   int                  quantity;
   TA_Real              price;
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

/* Make a TA_Instrument to represent a unique entity being traded. 
 * Can be initialized by using a category/symbol string (like
 * the TA-Lib data management module does), or the user can 
 * provide their own unique key (assumed to not change in the
 * lifetime of the usage of that TA_Instrument).
 * 
 * A TA_Instrument must be initialize either with TA_InstrumentInit
 * or TA_InstrumentInitWithUserKey, not both!
 */
TA_RetCode TA_InstrumentInit ( TA_Instrument *instrument,
                               const char *category,
                               const char *symbol ); 

TA_RetCode TA_InstrumentInitWithUserKey ( TA_Instrument *instrument,
                                          unsigned int uniqueUserDefinedKey );

/* Allows to link historical data to a particular instrument.
 * This will help for more "fine grain" measurements.
 *
 * Example: Without historical data, the equity line is based
 * only from the entry/exit transaction being log.
 *
 * The price variation between these transactions won’t be
 * accounted for.
 *
 * There is two way to attach data to the instrument:
 *  - Using TA-Lib data management (use a unified database).
 *  - Your application can provide pointers on the historical price.
 *
 * For a given instrument, data can be attach either with
 * TA_InstrumentAttachData or TA_InstrumentAttachUDBase, but
 * not both
 */

/* Note: In V0.0.5, you can attach data, though there is
 *       not yet a measurements who used that data.
 */

/* Attach user owned buffers to retrieve data for this instrument. */
TA_RetCode TA_InstrumentAttachData( TA_Instrument      *instrument,
                                    unsigned int        nbPrice,
                                    const TA_Timestamp *timestamp,
                                    const TA_Real      *lowPrice,
                                    const TA_Real      *price,
                                    const TA_Real      *highPrice );

/* Attach a UDBase to retreive data for this instrument. */
TA_RetCode TA_InstrumentAttachUDBase( TA_Instrument   *instrument,
                                      TA_UDBase       *udBase );

/* Function for building a TA_TradeLog */
TA_RetCode TA_TradeLogAlloc( TA_TradeLog **allocatedTradeLog );

TA_RetCode TA_TradeLogFree( TA_TradeLog *toBeFreed );

TA_RetCode TA_TradeLogAdd( TA_TradeLog *tradeLog,
                           const TA_Transaction *newTransaction );

/* Functions for performing Performance measurements for one
 * or many TA_TradeLog.
 *
 * Important: 
 *  - The TA_TradeLog must not be modified while
 *    being added to a TA_PM. Once the TA_PM is 
 *    freed, you can safely continue to add trades
 *    to the TA_TradeLog if you wish.
 *
 *  - It is the caller responsibility to maintain
 *    a coherent startDate/endDate. In other word,
 *    the TA_TradeLog must contains data only within
 *    the specified startDate/endDate, else some
 *    measurements will be incorrect.
 */
TA_RetCode TA_PMAlloc( const TA_Timestamp  *startDate,
                       const TA_Timestamp  *endDate,
                       TA_Real              initialCapital,
                       TA_PM              **allocatedPM );

TA_RetCode TA_PMFree( TA_PM *toBeFreed );
TA_RetCode TA_PMAddTradeLog( TA_PM *pm, TA_TradeLog *tradeLogToAdd );

/* Measurements are individual field extracted with 'TA_PMValue'. */
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

   /* Trade with the smallest profit. */
   TA_PM_SMALLEST_PROFIT, 
   TA_PM_SMALLEST_PROFIT_PERCENT,

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

   /* Trade with the smallest lost. */
   TA_PM_SMALLEST_LOSS, 
   TA_PM_SMALLEST_LOSS_PERCENT,

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

/* 
TA_RetCode TA_PMErrMargin( TA_PM        *pm,
                           TA_PMValueId  valueId,
                           TA_PMGroup    grp,
                           TA_Real      *value );
*/

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
#define TA_PM_VALUE_ID_CURRENCY_SYMBOL  0x00000001
#define TA_PM_VALUE_ID_PERCENT_SYMBOL   0x00000002
#define TA_PM_VALUE_ID_IS_INTEGER       0x00000004

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


/* Allocate and Free a report into a memory buffer. */
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

/* Append all measurements into a file. */
TA_RetCode TA_PMReportToFile( TA_PM *pm, FILE *out ); 

#ifdef __cplusplus
}
#endif

#endif
