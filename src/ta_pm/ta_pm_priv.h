#ifndef TA_PM_PRIV_H
#define TA_PM_PRIV_H

#ifndef TA_LIST_H
   #include "ta_list.h"
#endif

#ifndef TA_PM_H
   #include "ta_pm.h"
#endif

#ifndef TA_DICT_H
   #include "ta_dict.h"
#endif

/* Definitions used by the flags in the 
 * TA_PMPriv and TA_TradeLogPriv.
 */
#define TA_PMVALUECACHE_CALCULATED  (0x00000001)
#define TA_PMARRAYCACHE_CALCULATED  (0x00000002)
#define TA_EQUITY_CALCULATED        (0x00000004)
#define TA_RETURNS_CALCULATED       (0x00000008)

/* Mask used when it is needed to invalidated all
 * cached value. Clearing all these bits will assure
 * that calculation will be done from scratch for
 * everything.
 */
#define TA_INVALIDATION_MASK (TA_PMVALUECACHE_CALCULATED|\
                              TA_PMARRAYCACHE_CALCULATED|\
                              TA_EQUITY_CALCULATED      |\
                              TA_RETURNS_CALCULATED)

/* The same structure is used for logging the entry and
 * the trade information.
 *
 * Both are a TA_DataLog.
 *
 * To make the difference, an entry use a negative
 * quantity while trades use positive quantity.
 *
 * Added to this is an efficient TA_DataLog allocator
 * (see below) and you have overall a very fast
 * mechanism to log all the trades in close proximity in
 * memory (reducing cache hits when later processing
 * all the trades sequentially).
 */
typedef struct
{
   /* Some boolean information is hidden by using the 
    * sign of the quantity and entryPrice.
    */

   /* Positive=trade, negative=entry */
   int quantity;

   /* entryPrice: 
    *   Positive=long, negative=short
    *   but ONLY when this structure
    *   is a trade.
    */
   TA_Real      entryPrice;
   TA_Timestamp entryTimestamp;

   union TA_DataLogUnion
   {
      struct TA_EntryLog
      {
         TA_ListNode  node;
      } entry;

      struct TA_TradeLog
      {
        const TA_Instrument *id;
        TA_Timestamp exitTimestamp;
        TA_Real      profit; /* Positive=Winning, negative=Losing */
      } trade;
   } u;
} TA_DataLog;

/* TA_AllocatorForDataLog is an attempt to 
 * provide speed efficient memory allocation 
 * for the TA_DataLog structures.
 */

/* The TA_DataLog are allocated in block. 
 * Each block is kept in a list for
 * easily freeing them all at once.
 */
#define TA_TRADE_BLOCK_SIZE 20
typedef struct
{
   TA_ListNode node;
   TA_DataLog array[TA_TRADE_BLOCK_SIZE];
} TA_DataLogBlock;

typedef struct
{
   /* Keep a pointer on the next block
    * available to be allocated.
    */   
   TA_DataLog *nextAvailableTrade;

   /* Decrement, and at zero, must allocate 
    * a new TA_DataLogBlock
    */
   unsigned int nbFree;

   TA_List listOfDataLogBlock;
} TA_AllocatorForDataLog;

/* Must be called to initialize a TA_AllocatorForDataLog */
TA_RetCode TA_AllocatorForDataLog_Init( TA_AllocatorForDataLog *allocator );

/* Alllocate a TA_DataLog. Will return NULL if fail. */
TA_DataLog *TA_AllocatorForDataLog_Alloc( TA_AllocatorForDataLog *allocator );

/* Must be called to free all ressources related to a TA_AllocatorForDataLog.
 * Should be called only if TA_AllocatorForDataLog_Init did return TA_SUCCESS.
 */
TA_RetCode TA_AllocatorForDataLog_FreeAll( TA_AllocatorForDataLog *allocator );

typedef struct
{
   /* Number of completed trades. */
   unsigned int        nbWinningTrade;
   unsigned int        nbLosingTrade;

   /* Summation of all profits/loss.
    * Profit is positive. Loss is negative.
    */
   TA_Real             sumProfit;
   TA_Real             sumLoss;

   /* Summation of all investment that result into
    * a profit/loss. Both are positive.
    */
   TA_Real             sumInvestmentProfit;
   TA_Real             sumInvestmentLoss;

   /* Max/min values.
    *
    * Profit are positive. Loss are negative.
    *
    * Note: the first set of variable is in term of 
    *       amount, but can be meaningless if you
    *       are not doing a fix investment amount
    *       strategy.
    *       
    *       The 'percent' variable are a better way
    *       to measure the true max/min.
    */

   /* Max/min in terms of amount. */
   TA_Real             largestProfit;
   TA_Real             largestLoss;
   TA_Real             smallestProfit;
   TA_Real             smallestLoss;

   /* Max/min in terms of percentage. */
   TA_Real             largestProfitPercent;
   TA_Real             largestLossPercent;
   TA_Real             smallestProfitPercent;
   TA_Real             smallestLossPercent;

} TA_PMValueCache;

typedef struct
{
   TA_Real *profit;
   TA_Real *investment;
} TA_PMArrayCache;

/* Each unique instrument is an entry in a dictionary. 
 * Each entry in the dictionary contains the
 * list of transaction for that instrument.
 */
typedef struct
{
   /* Unique Id for the traded instrument. */
   const TA_Instrument *id;

   /* FIFO of TA_DataLog for this instrument. 
    * This FIFO are the entry transactions.
    */
   TA_List shortEntryPrivList; 
   TA_List longEntryPrivList;
} TA_TradeDictEntry;

typedef struct
{
   unsigned int magicNb;

   /* List of TA_TradeLogPriv */
   TA_List tradeLogList;

   TA_Timestamp startDate;
   TA_Timestamp endDate;
   TA_Real initialCapital;

   /* Store result of basic calculation */
   #define TA_PMVALUECACHE_CALCULATED (0x00000001)
   unsigned int flags;
   TA_PMValueCache shortValueCache;
   TA_PMValueCache longValueCache;
   
   /* Store some time series data.
    * Only the daily time serie are cached.
    */
   unsigned int nbDailyBars;
   TA_Timestamp    *arrayTimestamp;
   TA_PMArrayCache  shortArrayCache;
   TA_PMArrayCache  longArrayCache;
   TA_PMArrayCache  totalArrayCache;
   TA_Real         *equity;

} TA_PMPriv;

typedef struct 
{
   unsigned int magicNb;

   /* Dictionaries of TA_TradeDictEntry.
    * Different dictionary are used depending
    * of the key being used for the TA_Instrument
    */
   TA_Dict *tradeDictCATSYM;
   TA_Dict *tradeDictCAT;
   TA_Dict *tradeDictSYM;
   TA_Dict *tradeDictUserKey;

   /* Stored result of calculation for all
    * trades of ALL instruments.
    */
   unsigned int flags;
   TA_PMValueCache shortValueCache;
   TA_PMValueCache longValueCache;

   /* Optimized memory allocation mechanism 
    * for the TA_DataLog.
    */
   TA_AllocatorForDataLog allocator;

   /* A TA_TradeLogPriv can be added only
    * to one TA_PM at the time. This ptr
    * keep track of that parent TA_PM.
    *
    * The TA_TradeLog can be freed only
    * when this value is NULL.
    */
   TA_PMPriv *parentPMPriv;
    
} TA_TradeLogPriv;

#endif
