#ifndef TA_HISTORY_H
#define TA_HISTORY_H

#ifndef TA_DATA_H
   #include "ta_data.h"
#endif

#ifndef TA_DATA_UDB_H
   #include "ta_data_udb.h"
#endif

/* Interface of the TA_History module.
 *
 * These functions are the only one that could be called from outside
 * the ta_history directory.
 *
 * This module provide functionality for:
 *   - building/allocating TA_History structures.
 *   - validating a TA_History integrity.
 */

/* TA_HistoryBuilder does all the job to interact with the data source
 * for building a TA_History.
 *
 * (function can be found in "ta_history_builder.c")
 */
TA_RetCode TA_HistoryBuilder( TA_UDBasePriv *privUDB,
                              TA_UDB_Symbol *symbolData,
                              TA_Period            period,
                              const TA_Timestamp  *start,
                              const TA_Timestamp  *end,
                              TA_Field             fieldToAlloc,
                              TA_HistoryFlag       flags,      
                              TA_History         **history ); 

/* Validate the integrity of a TA_History.
 *
 * This function attempts to be self-content as much as possible to avoid
 * dependency on the "rest of the code" that we wish to monitor.
 *
 * If TA_INCONSISTENT_PRICE_BAR is returned, an error has been found with
 * a price bar (like an open lower than the low etc...). The faulty price
 * bar and fields are returned into the pointed variables.
 *
 * If 'faultyIndex' is NULL, there is no validation performed on the
 * consistency of each price bar.
 *
 * (function can be found in "ta_historycheck.c")
 */
TA_RetCode TA_HistoryCheck( TA_Period           expectedPeriod,
                            const TA_Timestamp *expectedStart,
                            const TA_Timestamp *expectedEnd,
                            TA_Field            fieldToCheck,
                            const TA_History   *history,
                            unsigned int       *faultyIndex,
                            unsigned int       *faultyField );

/* Allows to allocate an history with a different (longer) timeframe.
 * On success a new allocated history is returned through 'newHistory'.
 * The original history is never modified.
 *
 * If newPeriod is the same as history->period, the call result simply
 * as an allocation of an exact copy of the original TA_History.
 *
 * (function can be found in "ta_period.c")
 */
TA_RetCode TA_HistoryAllocCopyInternal( const TA_History *history,
                                        TA_History **newHistory,
                                        TA_Period newPeriod );


/* The hidden data put in a TA_History. */
typedef struct
{
   TA_UDBasePriv *privUDB;

   /* Ptr on the date that must be freed. */
   const void *open;
   const void *high;
   const void *low;
   const void *close;
   const void *volume;
   const void *openInterest;
   const void *timestamp;
} TA_HistoryHiddenData;

#endif

