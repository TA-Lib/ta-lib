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
 *  JS       Jon Sudul
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070800 MF     First version.
 *  012504 MF,JS  Fix mem leak in TA_HistoryAddData (Bug#881950).
 *  013104 MF     TA_History now contains names of sources + adjust
 *                timestamp to cover the requested period (Bug#888470)
 */

/* Description:
 *    This is where the data from all data source is combined for building
 *    the TA_History.
 */

/**** Headers ****/
#include <string.h>
#include "ta_common.h"
#include "ta_history.h"
#include "ta_trace.h"
#include "ta_parallel.h"
#include "ta_memory.h"
#include "ta_list.h"
#include "ta_history_priv.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
TA_FILE_INFO;

/**** Local functions declarations.    ****/
void getDataThread( void *args );

static TA_BuilderSupport *allocBuilderSupport( unsigned int nbDataSource );
static void freeBuilderSupport( TA_BuilderSupport *builderSupport );

static TA_RetCode freeSupportForDataSource( void *toBeFreed );
static TA_RetCode freeDataBlock( void *toBeFreed );
static TA_RetCode freeSplitAdjust( void *toBeFreed );
static TA_RetCode freeValueAdjust( void *toBeFreed );
static TA_RetCode freeMergeOp( void *toBeFreed );

static TA_RetCode allocHistory( TA_UDBasePriv *privUDB,
                                TA_History **history,
                                TA_Period period,
                                TA_Field fieldToAlloc,
                                TA_BuilderSupport *builderSupport );

static TA_RetCode allocHistoryFromOneDataSource( TA_UDBasePriv *privUDB,
                                                 TA_History **history,
                                                 TA_Period period,
                                                 TA_Field fieldToAlloc,
                                                 TA_SupportForDataSource *curSupportForDataSource,
                                                 TA_BuilderSupport *builderSupport );

static TA_RetCode verifyDataBlockValid( TA_DataBlock *dataBlock,
                                        TA_Field fieldToAlloc );

static void stopAllGetDataThread( TA_BuilderSupport *builderSupport );

static TA_RetCode buildListMergeOp( TA_BuilderSupport *builderSupport );

static TA_RetCode buildHistoryFromMergeOp( TA_History *newHistory,
                                           TA_BuilderSupport *builderSupport );


static const TA_Timestamp *adjLowerPrecedenceSupport( TA_ListIter *curSupport,
                                                      const TA_Timestamp *minTimestamp );

static const TA_Timestamp *nextPriceBar( TA_SupportForDataSource *supportForDataSource,
                                         const TA_Timestamp *minTimestamp );

static TA_History *allocEmptyHistory( TA_UDBasePriv *privUDB, 
                                      TA_Period period );

static void reverseRealElement( unsigned int nbElement,  TA_Real *table );
static void reverseIntegerElement( unsigned int nbElement,  TA_Integer *table );
static void reverseTimestampElement( unsigned int nbElement, TA_Timestamp *table );
static int sortSplitAdjust(const void *a, const void *b);
static int sortValueAdjust(const void *a, const void *b);
static int equalSplitAdjust(const void *a, const void *b);
static int equalValueAdjust(const void *a, const void *b);

static TA_RetCode historyAdjustData( TA_BuilderSupport *builderSupport );

static void trimBeforeStart( const TA_Timestamp *start, TA_Period period, TA_History *history );
static void trimAfterEnd( const TA_Timestamp *end, TA_Period period, TA_History *history );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode TA_HistoryBuilder( TA_UDBasePriv       *privUDB,
                              TA_UDB_Symbol       *symbolData,
                              TA_Period            period,
                              const TA_Timestamp  *start,
                              const TA_Timestamp  *end,
                              TA_Field             fieldToAlloc,
                              TA_History         **history )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_List *listDriverHandle;
   TA_List *listOfSupportForDataSource;
   TA_UDB_Driver *driverHandles;
   TA_SupportForDataSource *supportForDataSource;
   TA_BuilderSupport *builderSupport;
   unsigned int nbDataSource;
   unsigned int driverIndex;
   const TA_DataSourceDriver *driver;
   TA_HistoryHiddenData *historyHiddenData;
   TA_Timestamp startLocal, endLocal;

   TA_PAR_VARS;

   TA_TRACE_BEGIN( TA_HistoryBuilder );

   TA_ASSERT( symbolData != NULL );
   TA_ASSERT( history != NULL );

   *history = NULL;

   listDriverHandle = &symbolData->listDriverHandle;

   /* From now, use a copy of end/start since we might modify it. */
   if( start )
   {
      retCode = TA_TimestampCopy( &startLocal, start );
      TA_ASSERT( retCode == TA_SUCCESS );
      start = &startLocal;
   }

   if( end )
   {
      retCode = TA_TimestampCopy( &endLocal, end );
      TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
      end = &endLocal;
   }

   /* Adjust start/end to the upper/lower limit of the requested period. 
    * As an example, if someone ask monthly data, the day and time component
    * of the date is adjusted to the begining and end of the month for
    * respectively the start and end variables.
    */
   if( start )
   {
      switch( period )
      {
      case TA_DAILY:
         retCode = TA_SetTime( 0, 0, 0, &startLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      case TA_MONTHLY:
         retCode = TA_SetTime( 0, 0, 0, &startLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         retCode = TA_BackToBeginOfMonth(&startLocal);
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      case TA_QUARTERLY:
         retCode = TA_SetTime( 0, 0, 0, &startLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         retCode = TA_BackToBeginOfQuarter(&startLocal);
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      case TA_YEARLY:
         retCode = TA_SetTime( 0, 0, 0, &startLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         retCode = TA_BackToBeginOfYear(&startLocal);
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      default:
         /* Do nothing */
         break;
      }
   }

   if( end )
   {
      switch( period )
      {
      case TA_DAILY:
         retCode = TA_SetTime( 23, 59, 59, &endLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      case TA_MONTHLY:
         retCode = TA_SetTime( 23, 59, 59, &endLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         retCode = TA_JumpToEndOfMonth(&endLocal);
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      case TA_QUARTERLY:
         retCode = TA_SetTime( 23, 59, 59, &endLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         retCode = TA_JumpToEndOfQuarter(&endLocal);
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      case TA_YEARLY:
         retCode = TA_SetTime( 23, 59, 59, &endLocal );
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         retCode = TA_JumpToEndOfYear(&endLocal);
         TA_ASSERT_DEBUG( retCode == TA_SUCCESS );
         break;
      default:
         /* Do nothing */
         break;
      }
   }

   /* Allocate the builder support. Everything allocated from this
    * point will be referred one way or another from this structure.
    * If something goes wrong, this structure and all sub-elements will
    * be freed.
    */
   nbDataSource = TA_ListSize(listDriverHandle);
   TA_ASSERT( nbDataSource != 0 );
   builderSupport = allocBuilderSupport( nbDataSource );

   if( !builderSupport )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Start the thread for each data source. */
   driverHandles = (TA_UDB_Driver *)TA_ListAccessHead( listDriverHandle );

   if( !driverHandles )
   {
      freeBuilderSupport( builderSupport );
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(34)  );
   }

   listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;
   TA_ASSERT( listOfSupportForDataSource != NULL );
   supportForDataSource = (TA_SupportForDataSource *)TA_ListAccessHead( listOfSupportForDataSource );

   /* Parano test: The following code assume that the listDriverHandle and
    * listOfSupportForDataSource have the same number of elements.
    */
   TA_ASSERT( TA_ListSize(listOfSupportForDataSource) == nbDataSource );

   /* Initialize variable used for parallel processing. */
   TA_PAR_INIT();

   while( driverHandles && (builderSupport->retCode == TA_SUCCESS) )
   {
      TA_ASSERT( supportForDataSource != NULL );


      /* Set-up the parameters. */
      supportForDataSource->addDataSourceParamPriv = driverHandles->addDataSourceParamPriv;
      supportForDataSource->sourceHandle   = driverHandles->sourceHandle;
      supportForDataSource->categoryHandle = driverHandles->categoryHandle;
      supportForDataSource->symbolHandle   = &driverHandles->symbolHandle;
      supportForDataSource->period         = period;
      supportForDataSource->start          = start;
      supportForDataSource->end            = end;
      supportForDataSource->fieldToAlloc   = fieldToAlloc;
      
      driverIndex = driverHandles->addDataSourceParamPriv->id;
      TA_ASSERT( driverIndex < TA_gDataSourceTableSize );

      driver = &TA_gDataSourceTable[ driverIndex ];
      TA_ASSERT( driver != NULL );

      if( driver->getParameters == NULL )
      {
         TA_FATAL(  "Get parameters must be implemented", 0, 0 );
		 /* Should never return and get here */
      }
      else
      {
         retCode = (*(driver->getParameters))( &supportForDataSource->supportedParameter );
         if( retCode != TA_SUCCESS )
         {
            stopAllGetDataThread( builderSupport );
            builderSupport->retCode = retCode;
            break; /* Exit the loop righ away! */
         }
      }

      /* When a data source needs to do split/value adjustment,
       * it requires to do it up to the latest available data.
       * To keep that simple, force the driver to provide up to
       * the most recent data. The exceeding data will be trim later.
       */
      if( supportForDataSource->supportedParameter.flags & TA_DO_NOT_SPLIT_ADJUST )
      {
         if( !(supportForDataSource->addDataSourceParamPriv->flags & TA_DO_NOT_SPLIT_ADJUST) )
            supportForDataSource->end = 0;
      }

      if( supportForDataSource->supportedParameter.flags & TA_DO_NOT_VALUE_ADJUST )
      {
         if( !(supportForDataSource->addDataSourceParamPriv->flags & TA_DO_NOT_VALUE_ADJUST) )
            supportForDataSource->end = 0;
      }

      /* Check immediatly for the next driver handles. */
      driverHandles = (TA_UDB_Driver *)TA_ListAccessNext( listDriverHandle );

      /* Get the data from each slowAccess data source in a different thread.
       * Except for the last driver which always get executed in the
       * main thread.
       */
      if( (driverHandles != NULL) && (supportForDataSource->supportedParameter.flags & TA_SLOW_ACCESS) )
      {
         /* Start a thread for this data source. */
         TA_PAR_EXEC( getDataThread, supportForDataSource );
      }
      else
      {
         /* Sequential execution for fast local database and/or last
          * driver.
          */
         getDataThread( supportForDataSource );
      }

      /* Next data source. */
      supportForDataSource = (TA_SupportForDataSource *)TA_ListAccessNext( listOfSupportForDataSource );
   }

   /* Join all the thread before continuing (barrier sync). */
   TA_PAR_JOIN;

   /* Clean-up variable that were used for parallel processing. */
   TA_PAR_END;

   /* Check if an error was reported at any point. If yes, the whole operation
    * failed, even if that error was cause by only one of the data source.
    */
   if( builderSupport->retCode != TA_SUCCESS )
   {
      retCode = builderSupport->retCode;
      freeBuilderSupport( builderSupport );
      TA_TRACE_RETURN( retCode );
   }

   /* Finally, allocate and build the TA_History. */
   retCode = allocHistory( privUDB, history, period, fieldToAlloc, builderSupport );
   freeBuilderSupport( builderSupport );

   if( retCode != TA_SUCCESS )
   {
      *history = NULL;
      TA_TRACE_RETURN( retCode );
   }

   /* Save in the hidden data a pointer on the data that will need to be freed. */
   historyHiddenData = (TA_HistoryHiddenData *)((*history)->hiddenData);
   historyHiddenData->open   = (*history)->open;
   historyHiddenData->high   = (*history)->high;
   historyHiddenData->low    = (*history)->low;
   historyHiddenData->close  = (*history)->close;
   historyHiddenData->volume = (*history)->volume;
   historyHiddenData->openInterest = (*history)->openInterest;
   historyHiddenData->timestamp = (*history)->timestamp;

   /* Sometimes, for optimization reason, the builder might returns
    * more data than requested. In that case, trim the output such
    * that only the requested range is "visible" to the caller.
    */
   if( (*history)->nbBars >= 1 )
   {
      if( start )
      {
         trimBeforeStart( start, period, *history );
      }
      if( end )
      {
         trimAfterEnd( end, period, *history );
      }
   }

   /* Until the library is very stable, we will make a double check
    * of the integrity of the "almost final" TA_History.
    */
   retCode = TA_HistoryCheckInternal( period, start, end,
                                      fieldToAlloc, *history,
                                      NULL, NULL );
   if( retCode != TA_SUCCESS )
   {
      TA_HistoryFree( *history );
      *history = NULL;
      TA_TRACE_RETURN( retCode );
   }

   /* Free the timestamps if it is not requested by the caller. */
   if( (fieldToAlloc != TA_ALL) && !(fieldToAlloc & TA_TIMESTAMP) )
   {
      FREE_IF_NOT_NULL( historyHiddenData->timestamp );
      historyHiddenData->timestamp = NULL;
      (*history)->timestamp = NULL;
   }

   TA_TRACE_RETURN( retCode );
}

/* A function to provide some info to the data source driver.
 *
 * It might have been simple to simply return the TA_SupportForDataSource
 * pointer to the driver, but I choose not to do this to make 100% sure
 * that the driver won't modify anything that might break the core
 * of the TA-Lib logic. So the relevant info is instead copied into
 * that TA_InfoFromAddedData structure.
 */
TA_RetCode TA_GetInfoFromAddedData( TA_ParamForAddData *paramForAddData,
                                    TA_InfoFromAddedData *info )
{
   TA_SupportForDataSource *supportForDataSource;

   if( !paramForAddData || !info )
      return TA_BAD_PARAM;

   supportForDataSource = (TA_SupportForDataSource *)paramForAddData;

   TA_TimestampCopy( &info->highestTimestamp, supportForDataSource->highestTimestamp );
   TA_TimestampCopy( &info->lowestTimestamp, supportForDataSource->lowestTimestamp );

   if( supportForDataSource->barAddedSinceLastCall )
   {
      info->barAddedSinceLastCall = supportForDataSource->barAddedSinceLastCall;
      supportForDataSource->barAddedSinceLastCall = 0;
      TA_TimestampCopy( &info->highestTimestampAddedSinceLastCall,
                        supportForDataSource->highestTimestampAddedSinceLastCall );
      TA_TimestampCopy( &info->lowestTimestampAddedSinceLastCall,
                        supportForDataSource->lowestTimestampAddedSinceLastCall );
   }
   else
   {
      info->barAddedSinceLastCall = 0;
      TA_SetDefault( &info->lowestTimestampAddedSinceLastCall );
      TA_SetDefault( &info->highestTimestampAddedSinceLastCall );
   }
   
   return TA_SUCCESS;
}

/* Allows the data source driver to cancel all the data
 * who was added up to now. This might be useful if
 * the data source driver needs to restart the processing
 * of adding the data.
 */
TA_RetCode TA_HistoryAddDataReset( TA_ParamForAddData *paramForAddData )
{
   TA_SupportForDataSource *supportForDataSource;
   TA_RetCode retCode;

   supportForDataSource = (TA_SupportForDataSource *)paramForAddData;

   if( supportForDataSource )
   {
      /* Free all the current data that has been added. */
      if( supportForDataSource->listOfDataBlock )
      {
         retCode = TA_ListFreeAll( supportForDataSource->listOfDataBlock, freeDataBlock );
         if( retCode != TA_SUCCESS )
            return retCode;
      }

      if( supportForDataSource->listOfSplitAdjust )
      {
         retCode = TA_ListFreeAll( supportForDataSource->listOfSplitAdjust, freeSplitAdjust );
         if( retCode != TA_SUCCESS )
            return retCode;
      }

      if( supportForDataSource->listOfValueAdjust )
      {
         retCode = TA_ListFreeAll( supportForDataSource->listOfValueAdjust, freeValueAdjust );
         if( retCode != TA_SUCCESS )
            return retCode;
      }

      /* Re-alloc the list used to accumulate the series of data. */
      supportForDataSource->listOfDataBlock = TA_ListAlloc();
      if( !supportForDataSource->listOfDataBlock )
         return TA_ALLOC_ERR;

      supportForDataSource->listOfSplitAdjust = TA_ListAlloc();
      if( !supportForDataSource->listOfSplitAdjust )
      {
         TA_ListFree( supportForDataSource->listOfDataBlock );
         supportForDataSource->listOfDataBlock = NULL;
         return TA_ALLOC_ERR;
      }

      supportForDataSource->listOfValueAdjust = TA_ListAlloc();
      if( !supportForDataSource->listOfValueAdjust )
      {
         TA_ListFree( supportForDataSource->listOfValueAdjust );
         supportForDataSource->listOfDataBlock = NULL;
         TA_ListFree(supportForDataSource->listOfSplitAdjust);
         supportForDataSource->listOfSplitAdjust = NULL;
         return TA_ALLOC_ERR;
      }

      /* Re-initialize to zero many of the fields used
       * by TA_HistoryAddData.
       */
      supportForDataSource->fieldProvided = 0;
      supportForDataSource->periodProvided = 0;
      supportForDataSource->lowestTimestamp = NULL;
      supportForDataSource->highestTimestamp = NULL;
      supportForDataSource->barAddedSinceLastCall = 0;
      supportForDataSource->lowestTimestampAddedSinceLastCall = NULL;
      supportForDataSource->highestTimestampAddedSinceLastCall = NULL; 
   }

   return TA_SUCCESS;
}

TA_RetCode TA_HistoryAddData( TA_ParamForAddData *paramForAddData,
                              unsigned int nbBarAdded,
                              TA_Period period,
                              TA_Timestamp *timestamp,
                              TA_Real *open,
                              TA_Real *high,
                              TA_Real *low,
                              TA_Real *close,
                              TA_Integer *volume,
                              TA_Integer *openInterest )
{ 
   TA_PROLOG
   TA_RetCode retCode;
   TA_SupportForDataSource *supportForDataSource;
   TA_BuilderSupport   *builderSupport;
   TA_DataBlock *newBlock;
   TA_Field fieldProvided;
   const TA_Timestamp *lowTimestamp;
   const TA_Timestamp *highTimestamp;
   unsigned int thisBlockGoesAfterTheExistingOnes;

   TA_TRACE_BEGIN( TA_HistoryAddData );

   newBlock = NULL;

   supportForDataSource = (TA_SupportForDataSource *)paramForAddData;

   builderSupport = supportForDataSource->parent;
   TA_ASSERT( builderSupport != NULL );

   /* Check the parameters, and identify the context of this data. */
   if( paramForAddData == NULL )
   {
      retCode = TA_BAD_PARAM;
      goto TA_HistoryAddData_EXIT;
   }

   if( (period == 0) || !timestamp )
   {
      retCode = TA_BAD_PARAM;
      goto TA_HistoryAddData_EXIT;
   }

   if( nbBarAdded == 0 )
   {
      retCode = TA_SUCCESS;
      goto TA_HistoryAddData_EXIT;
   }

   if( supportForDataSource->enoughValidDataProvided )
   {
      /* Inform the driver that we got already enough data and it should
       * exit its GetHistoryData with TA_SUCCESS.
       */
      retCode = TA_ENOUGH_DATA;
      goto TA_HistoryAddData_EXIT;
   }

   /* Identify the fields provided. */
   fieldProvided = TA_TIMESTAMP;
   if( open ) fieldProvided |= TA_OPEN;
   if( high ) fieldProvided |= TA_HIGH;
   if( low ) fieldProvided |= TA_LOW;
   if( close ) fieldProvided |= TA_CLOSE;
   if( volume ) fieldProvided |= TA_VOLUME;
   if( openInterest ) fieldProvided |= TA_OPENINTEREST;

   if( supportForDataSource->fieldToAlloc != TA_ALL )
   {
      /* A specific set of fields has been requested. */

      /* Validate that the driver is providing all the needed fields. */
      if( (fieldProvided & (supportForDataSource->fieldToAlloc)) !=
          (supportForDataSource->fieldToAlloc) )

      {
         /* It is the responsibility of the driver to provides ALL the
          * requested fields (plus the timestamp). If the driver cannot
          * provides the requested field, it shall never call TA_HistoryAddData.
          */
         retCode = TA_INTERNAL_ERROR(37);
         goto TA_HistoryAddData_EXIT;
      }

      /* Eliminates possible extra fields that are not requested. */
      #define ELIMINATE_NOT_REQUESTED(lowerc,upperc) \
      { \
         if( lowerc && !(fieldProvided&TA_##upperc) ) \
         { \
            TA_Free( lowerc ); \
            lowerc = NULL; \
            fieldProvided &= ~TA_##upperc; \
         } \
      }
      ELIMINATE_NOT_REQUESTED( open,  OPEN );
      ELIMINATE_NOT_REQUESTED( high,  HIGH );
      ELIMINATE_NOT_REQUESTED( low,   LOW );
      ELIMINATE_NOT_REQUESTED( close, CLOSE );
      ELIMINATE_NOT_REQUESTED( volume, VOLUME );
      ELIMINATE_NOT_REQUESTED( openInterest, OPENINTEREST );
      #undef ELIMINATE_NOT_REQUESTED
   }

   /* Verify that all added data provides the same set of fields! */
   if( !supportForDataSource->fieldProvided )
      supportForDataSource->fieldProvided = fieldProvided;
   else if( fieldProvided != supportForDataSource->fieldProvided )
   {
      /* The driver is not consistent about the fields provided. */
      retCode = TA_INTERNAL_ERROR(38);
      goto TA_HistoryAddData_EXIT;
   }

   /* Verify that all added data are in the same timeframe. */
   if( !supportForDataSource->periodProvided )
      supportForDataSource->periodProvided = period;
   else if( period != supportForDataSource->periodProvided )
   {
      /* The driver is not consistent about the period provided. */
      retCode = TA_INTERNAL_ERROR(39);
      goto TA_HistoryAddData_EXIT;
   }

   /* Validate that extremity timestamps are good. */
   lowTimestamp = &timestamp[0];
   highTimestamp = &timestamp[nbBarAdded-1];

   if( (TA_TimestampValidate(lowTimestamp) != TA_SUCCESS) ||
       (TA_TimestampValidate(highTimestamp) != TA_SUCCESS) )
   {
      retCode = TA_INVALID_DATE;
      goto TA_HistoryAddData_EXIT;
   }

   if( TA_TimestampEqual( highTimestamp, lowTimestamp) )
   {
      /* If timestamp are equal, there should have only one bar! */
      if( nbBarAdded != 1 )
      {
         retCode = TA_INVALID_DATE;
         goto TA_HistoryAddData_EXIT;
      }
   }
   else if( TA_TimestampLess( highTimestamp, lowTimestamp ) )
   {
      /* Time stamp shall be ascending (older first). */

      /* Re-order needed. */
      if( timestamp    ) reverseTimestampElement( nbBarAdded, timestamp    );
      if( open         ) reverseRealElement     ( nbBarAdded, open         );
      if( high         ) reverseRealElement     ( nbBarAdded, high         );
      if( low          ) reverseRealElement     ( nbBarAdded, low          );
      if( close        ) reverseRealElement     ( nbBarAdded, close        );
      if( volume       ) reverseIntegerElement  ( nbBarAdded, volume       );
      if( openInterest ) reverseIntegerElement  ( nbBarAdded, openInterest );
   }

   /* Find where this block shall be added in the list.
    * Continue some validation of the timestamp at the same time.
    */
   if( supportForDataSource->highestTimestamp == NULL )
   {
      /* Logic for first block added. */
      thisBlockGoesAfterTheExistingOnes = 1;
   }
   else
   {
      TA_ASSERT( supportForDataSource->lowestTimestamp != NULL );

      /* Trap obvious abnormality (same data shall never be added twice!). */
      if( TA_TimestampEqual( supportForDataSource->lowestTimestamp, lowTimestamp ) ||
          TA_TimestampEqual( supportForDataSource->lowestTimestamp, highTimestamp ) ||
          TA_TimestampEqual( supportForDataSource->lowestTimestamp, lowTimestamp ) ||
          TA_TimestampEqual( supportForDataSource->highestTimestamp, highTimestamp ) )
      {
         retCode = TA_INVALID_DATE;
         goto TA_HistoryAddData_EXIT;
      }

      if( TA_TimestampLess( lowTimestamp, supportForDataSource->lowestTimestamp ) )
      {
         if( TA_TimestampLess( supportForDataSource->lowestTimestamp, highTimestamp ) )
         {
            retCode = TA_INVALID_DATE;
            goto TA_HistoryAddData_EXIT;
         }

         thisBlockGoesAfterTheExistingOnes = 0;
      }
      else
      {
         if( TA_TimestampLess( lowTimestamp, supportForDataSource->highestTimestamp) )
         {
            retCode = TA_INVALID_DATE;
            goto TA_HistoryAddData_EXIT;
         }

         thisBlockGoesAfterTheExistingOnes = 1;
      }
   }

   /* Everything looks fine, go forward to keep reference on this new data
    * in the 'listOfDataSource'.
    */
   newBlock = (TA_DataBlock *)TA_Malloc( sizeof( TA_DataBlock ) );

   if( !newBlock )
   {
      retCode = TA_ALLOC_ERR;
      goto TA_HistoryAddData_EXIT;
   }

   memset( newBlock, 0, sizeof( TA_DataBlock ) );

   #define SET_NEWBLOCK(par) newBlock->par=par;
   SET_NEWBLOCK(timestamp);
   SET_NEWBLOCK(open);
   SET_NEWBLOCK(high);
   SET_NEWBLOCK(low);
   SET_NEWBLOCK(close);
   SET_NEWBLOCK(volume);
   SET_NEWBLOCK(openInterest);
   #undef SET_NEWBLOCK
   newBlock->period = period;
   newBlock->nbBars = nbBarAdded;
   newBlock->fieldProvided = fieldProvided;

   /* Make an "independant" verification of the "newBlock".
    * This is slightly parano, but this double checking is a quick way to
    * validate that this function is delivering the goods...
    */
   retCode = verifyDataBlockValid( newBlock, supportForDataSource->fieldToAlloc );
   if( retCode != TA_SUCCESS )
      goto TA_HistoryAddData_EXIT;

   /* Everything is fine, add it to the list. */
   if( thisBlockGoesAfterTheExistingOnes )
      retCode = TA_ListAddTail( supportForDataSource->listOfDataBlock, newBlock );
   else
      retCode = TA_ListAddHead( supportForDataSource->listOfDataBlock, newBlock );

   if( retCode != TA_SUCCESS )
      goto TA_HistoryAddData_EXIT;

   /* The 'listOfDataBlock' now owns the newBlock and the vectors of data. */
   newBlock     = NULL;
   timestamp    = NULL;
   open         = NULL;
   high         = NULL;
   low          = NULL;
   close        = NULL;
   volume       = NULL;
   openInterest = NULL;

   /* On success, adjust the lowest/highest information. */
   if( supportForDataSource->highestTimestamp == NULL )
   {
      /* First block added, so initialize the variables.  */
      supportForDataSource->lowestTimestamp = lowTimestamp;
      supportForDataSource->highestTimestamp = highTimestamp;
   }
   else if( thisBlockGoesAfterTheExistingOnes )
      supportForDataSource->highestTimestamp = highTimestamp;
   else
      supportForDataSource->lowestTimestamp = lowTimestamp;

   /* Same lowest/highest principle, except that these
    * are going to be reset everytime the data source
    * driver will call TA_GetInfoFromAddedData
    */
   if( supportForDataSource->barAddedSinceLastCall == 0 )
   {
      /* First block added since last call, so initialize the variables.  */
      supportForDataSource->lowestTimestampAddedSinceLastCall = lowTimestamp;
      supportForDataSource->highestTimestampAddedSinceLastCall = highTimestamp;
      supportForDataSource->barAddedSinceLastCall += nbBarAdded;
   }
   else if( thisBlockGoesAfterTheExistingOnes )
      supportForDataSource->highestTimestampAddedSinceLastCall = highTimestamp;
   else
      supportForDataSource->lowestTimestampAddedSinceLastCall = lowTimestamp;

   retCode = TA_SUCCESS;

TA_HistoryAddData_EXIT:  /* The only point of this function. */

   /* Free locally allocated resource (if still owner). */
   FREE_IF_NOT_NULL( newBlock );

   /* ALWAYS make sure all the provided data is freed. ALWAYS. */
   FREE_IF_NOT_NULL( open );
   FREE_IF_NOT_NULL( high );
   FREE_IF_NOT_NULL( low );
   FREE_IF_NOT_NULL( close );
   FREE_IF_NOT_NULL( volume );
   FREE_IF_NOT_NULL( openInterest );
   FREE_IF_NOT_NULL( timestamp );

   /* Any major error will fail the entire read operation
    * (including all other data source, even if they did succeed).
    */
   if( builderSupport && (retCode != TA_SUCCESS) && (retCode != TA_ENOUGH_DATA) )
   {
      builderSupport->retCode = retCode;
   }

   TA_TRACE_RETURN( retCode );
}

TA_RetCode TA_HistoryAddSplitAdjust( TA_ParamForAddData *paramForAddData, TA_Timestamp *when, double factor )
{
   TA_SupportForDataSource *supportForDataSource;
   TA_SplitAdjust *splitAdjust;
   TA_RetCode retCode;

   if( !paramForAddData || !when || factor <= 0.0 )
      return TA_BAD_PARAM;

   supportForDataSource = (TA_SupportForDataSource *)paramForAddData;

   if( !supportForDataSource->listOfSplitAdjust )
   {
      supportForDataSource->listOfSplitAdjust = TA_ListAlloc();
      if( !supportForDataSource->listOfSplitAdjust )
          return TA_ALLOC_ERR;
   }

   /* Allocate the event and keep it in a list. */
   splitAdjust = TA_Malloc( sizeof(TA_SplitAdjust) );
   if( !splitAdjust )
      return TA_ALLOC_ERR;
   memset( splitAdjust, 0, sizeof(TA_SplitAdjust) );
   TA_TimestampCopy( &splitAdjust->timestamp, when );
   splitAdjust->factor = factor;
   retCode = TA_ListAddTail( supportForDataSource->listOfSplitAdjust, splitAdjust );
   if( retCode != TA_SUCCESS )
   {
      TA_Free( splitAdjust );
      return retCode;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_HistoryAddValueAdjust( TA_ParamForAddData *paramForAddData, TA_Timestamp *when, double amount )
{
   TA_SupportForDataSource *supportForDataSource;
   TA_ValueAdjust *valueAdjust;
   TA_RetCode retCode;

   if( !paramForAddData || !when || amount <= 0.0 )
      return TA_BAD_PARAM;

   supportForDataSource = (TA_SupportForDataSource *)paramForAddData;

   if( !supportForDataSource->listOfValueAdjust )  
   {
      supportForDataSource->listOfValueAdjust = TA_ListAlloc();
      if( !supportForDataSource->listOfValueAdjust )
         return TA_ALLOC_ERR;
   }

   /* Allocate the event and keep it in a list. */
   valueAdjust = TA_Malloc( sizeof(TA_ValueAdjust) );
   if( !valueAdjust )
      return TA_ALLOC_ERR;
   memset( valueAdjust, 0, sizeof(TA_ValueAdjust) );
   TA_TimestampCopy( &valueAdjust->timestamp, when );
   valueAdjust->amount = amount;
   retCode = TA_ListAddTail( supportForDataSource->listOfValueAdjust, valueAdjust );
   if( retCode != TA_SUCCESS )
   {
      TA_Free( valueAdjust );
      return retCode;
   }

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
static TA_RetCode historyAdjustData( TA_BuilderSupport *builderSupport )
{
   TA_RetCode retCode;
   TA_ListIter iterSource;
   TA_ListIter blockIter;
   TA_DataBlock *curDataBlock;
   TA_SupportForDataSource *supportForDataSource;
   TA_ValueAdjust *curValueAdjust;
   TA_SplitAdjust *curSplitAdjust;
   double factor, temp;
   int i;

   if( !builderSupport )
      return TA_BAD_PARAM;

   /* Iterate through all the data sources. */
   TA_ListIterInit( &iterSource, builderSupport->listOfSupportForDataSource );

   supportForDataSource = (TA_SupportForDataSource *)TA_ListIterHead(&iterSource);

   while( supportForDataSource )
   {
      curValueAdjust = NULL;
      curSplitAdjust = NULL;

      /* Sort all adjustments and eliminate redundant entries. */  
      if( supportForDataSource->listOfSplitAdjust && !(supportForDataSource->addDataSourceParamPriv->flags&TA_DO_NOT_SPLIT_ADJUST) )
      {
         if( TA_ListSize(supportForDataSource->listOfSplitAdjust) > 0 )
         {
            retCode = TA_ListRemoveDuplicate( supportForDataSource->listOfSplitAdjust, equalSplitAdjust, freeSplitAdjust );
            if( retCode != TA_SUCCESS )
               return retCode;

            retCode = TA_ListSort( supportForDataSource->listOfSplitAdjust, sortSplitAdjust );
            if( retCode != TA_SUCCESS )
               return retCode;
            curSplitAdjust = TA_ListAccessHead( supportForDataSource->listOfSplitAdjust);
         }
      }
      if( supportForDataSource->listOfValueAdjust && !(supportForDataSource->addDataSourceParamPriv->flags&TA_DO_NOT_VALUE_ADJUST) )
      {
         if( TA_ListSize( supportForDataSource->listOfValueAdjust ) )
         {
            retCode = TA_ListRemoveDuplicate( supportForDataSource->listOfValueAdjust, equalValueAdjust, freeValueAdjust );
            if( retCode != TA_SUCCESS )
               return retCode;

            retCode = TA_ListSort( supportForDataSource->listOfValueAdjust, sortValueAdjust );
            if( retCode != TA_SUCCESS )
               return retCode;
            curValueAdjust = TA_ListAccessHead(supportForDataSource->listOfValueAdjust);
         }
      }

      if( curSplitAdjust || curValueAdjust )
      {
         factor = 1.0;

         /* Iterate throught the price bar starting with the most recent going backward. */
         TA_ListIterInit( &blockIter, supportForDataSource->listOfDataBlock );
         curDataBlock = TA_ListIterTail(&blockIter);
         while( curDataBlock )
         {
             for( i=curDataBlock->nbBars-1; i >= 0; i-- )
             {
                /* Change the factor? */
                if( curSplitAdjust )
                {
                   if( TA_TimestampGreater(&curSplitAdjust->timestamp,&curDataBlock->timestamp[i]) )
                   {
                      factor *= curSplitAdjust->factor;
                      curSplitAdjust = TA_ListAccessNext(supportForDataSource->listOfSplitAdjust);
                   }
                }

                if( curValueAdjust )
                {
                   if( TA_TimestampGreater(&curValueAdjust->timestamp,&curDataBlock->timestamp[i]) )
                   {
                      /* Find one of the price value to build a proportional factor. 
                       * close is the first choice for this calculation.
                       */
                      if( curDataBlock->close ) temp = curDataBlock->close[i];
                      else if( curDataBlock->open ) temp = curDataBlock->open[i];
                      else if( curDataBlock->high ) temp = curDataBlock->high[i];
                      else if( curDataBlock->low) temp = curDataBlock->low[i];
                      else return TA_MISSING_PRICE_FOR_ADJUSTMENT;
                      factor *= 1.0-(curValueAdjust->amount/temp);
                      curValueAdjust = TA_ListAccessNext(supportForDataSource->listOfValueAdjust);
                   }
                }

                #define ADJUST_PRICE(a) \
                { \
                   if( curDataBlock->a ) \
                    curDataBlock->a[i] *= factor; \
                }
                ADJUST_PRICE(open);
                ADJUST_PRICE(high);
                ADJUST_PRICE(low);
                ADJUST_PRICE(close);
                #undef ADJUST_PRICE
             }         

             curDataBlock = TA_ListIterPrev(&blockIter);
         }
      }

      /* Check for handling the next data source. */
      supportForDataSource = (TA_SupportForDataSource *)TA_ListIterNext(&iterSource);
   }

   return TA_SUCCESS;
}

static int equalSplitAdjust(const void *a, const void *b)
{
   return TA_TimestampEqual( &((TA_SplitAdjust *)a)->timestamp, &((TA_SplitAdjust *)b)->timestamp ) &&
          (((TA_SplitAdjust *)a)->factor == ((TA_SplitAdjust *)b)->factor );
}

static int equalValueAdjust(const void *a, const void *b)
{
   return TA_TimestampEqual( &((TA_ValueAdjust *)a)->timestamp, &((TA_ValueAdjust *)b)->timestamp ) &&
          (((TA_ValueAdjust *)a)->amount == ((TA_ValueAdjust *)b)->amount );
}

static int sortSplitAdjust(const void *a, const void *b)
{
   /* Will be sorted with most recent first */
   return TA_TimestampCompare( &((TA_SplitAdjust *)b)->timestamp, &((TA_SplitAdjust *)a)->timestamp );
}

static int sortValueAdjust(const void *a, const void *b)
{
   /* Will be sorted with most recent first */
   return TA_TimestampCompare( &((TA_ValueAdjust *)b)->timestamp, &((TA_ValueAdjust *)a)->timestamp );
}

void getDataThread( void *args  )
{
   TA_RetCode retCode;

   const TA_DataSourceDriver *driver;
   unsigned int driverIndex;
   TA_SupportForDataSource *supportForDataSource;

   supportForDataSource = (TA_SupportForDataSource *)args;
   if( !args )
      return;

   driverIndex = supportForDataSource->addDataSourceParamPriv->id;
   TA_ASSERT_NO_RET( driverIndex < TA_gDataSourceTableSize );

   driver = &TA_gDataSourceTable[ driverIndex ];
   TA_ASSERT_NO_RET( driver != NULL );

   retCode = TA_SUCCESS;

   if( driver->getHistoryData )
   {
      if( !supportForDataSource->enoughValidDataProvided )
      {
         retCode = (*(driver->getHistoryData))( supportForDataSource->sourceHandle,
                                                supportForDataSource->categoryHandle,
                                                supportForDataSource->symbolHandle,
                                                supportForDataSource->period,
                                                supportForDataSource->start,
                                                supportForDataSource->end,
                                                supportForDataSource->fieldToAlloc,
                                                (void *)supportForDataSource );

      }
   }

   supportForDataSource->finishIndication = 1;
   supportForDataSource->retCode = retCode;
   if( (retCode != TA_SUCCESS) && (retCode != TA_ENOUGH_DATA) )
   {
      stopAllGetDataThread( supportForDataSource->parent );
      supportForDataSource->parent->retCode = retCode;
   }
}

static TA_BuilderSupport *allocBuilderSupport( unsigned int nbDataSource )
{
  TA_RetCode retCode;
  unsigned int i;
  TA_BuilderSupport *builderSupport;
  TA_List *listOfSupportForDataSource; /* List of TA_SupportForDataSource. */
  TA_SupportForDataSource *supportForDataSource;

  TA_ASSERT_RET( nbDataSource > 0, (TA_BuilderSupport *)NULL );

  builderSupport = (TA_BuilderSupport *)TA_Malloc( sizeof( TA_BuilderSupport ) );

  if( !builderSupport )
     return (TA_BuilderSupport *)NULL;

  builderSupport->nbPriceBar = 0;
  builderSupport->retCode = TA_SUCCESS;
  builderSupport->listOfMergeOp = NULL;
  builderSupport->commonFieldProvided = TA_ALL;

  builderSupport->listOfSupportForDataSource = TA_ListAlloc();
  if( !builderSupport->listOfSupportForDataSource )
  {
     freeBuilderSupport( builderSupport );
     return (TA_BuilderSupport *)NULL;
  }
  listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;

  for( i=nbDataSource; i > 0; i-- )
  {
     supportForDataSource = (TA_SupportForDataSource *)TA_Malloc(sizeof(TA_SupportForDataSource));
     if( !supportForDataSource )
     {
        freeBuilderSupport( builderSupport );
        return (TA_BuilderSupport *)NULL;
     }
     
     /* Initialize all fields to NULL. */
     memset( supportForDataSource, 0, sizeof( TA_SupportForDataSource ) );

     /* Make parent the 'builderSupport' */
     supportForDataSource->parent = builderSupport;

     /* Add this TA_SupportForDataSource to the list. */
     retCode = TA_ListAddTail( listOfSupportForDataSource, supportForDataSource );
     if( retCode != TA_SUCCESS )
     {
        freeBuilderSupport( builderSupport );
        return (TA_BuilderSupport *)NULL;
     }

     /* Initialize an empty 'listOfDataBlock'. */
     supportForDataSource->listOfDataBlock = TA_ListAlloc();

     if( !supportForDataSource->listOfDataBlock )
     {
        freeBuilderSupport( builderSupport );
        return (TA_BuilderSupport *)NULL;
     }
  }

  /* Initialize an empty 'listOfMergeOp'. */
  builderSupport->listOfMergeOp = TA_ListAlloc();

  if( !builderSupport->listOfMergeOp )
  {
     freeBuilderSupport( builderSupport );
     return (TA_BuilderSupport *)NULL;
  }

  return builderSupport;
}


static void stopAllGetDataThread( TA_BuilderSupport *builderSupport )
{
   /* All thread will get their 'enoughValidDataProvided' sets. */
   TA_RetCode retCode;
   TA_SupportForDataSource *supportForDataSource;

   TA_ListIter iter;

   retCode = TA_ListIterInit( &iter, builderSupport->listOfSupportForDataSource );
   if( retCode != TA_SUCCESS )
      return;

   supportForDataSource = (TA_SupportForDataSource *)TA_ListIterHead( &iter );

   while( supportForDataSource )
   {
      supportForDataSource->enoughValidDataProvided = 1;
      supportForDataSource = (TA_SupportForDataSource *)TA_ListIterNext( &iter );
   }
}

static TA_RetCode freeSupportForDataSource( void *toBeFreed )
{
   TA_PROLOG
   TA_SupportForDataSource *supportForDataSource;

   TA_TRACE_BEGIN(  freeSupportForDataSource );

   supportForDataSource = (TA_SupportForDataSource *)toBeFreed;

   if( supportForDataSource )
   {
      if( supportForDataSource->listOfDataBlock )
         TA_ListFreeAll( supportForDataSource->listOfDataBlock, freeDataBlock );
      supportForDataSource->listOfDataBlock = NULL;

      if( supportForDataSource->listOfSplitAdjust )
         TA_ListFreeAll( supportForDataSource->listOfSplitAdjust, freeSplitAdjust );
      supportForDataSource->listOfSplitAdjust = NULL;

      if( supportForDataSource->listOfValueAdjust )
         TA_ListFreeAll( supportForDataSource->listOfValueAdjust, freeValueAdjust );
      supportForDataSource->listOfValueAdjust = NULL;

      TA_Free( supportForDataSource );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeDataBlock( void *toBeFreed )
{
   TA_PROLOG
   TA_DataBlock *dataBlock;

   TA_TRACE_BEGIN( freeDataBlock );

   dataBlock = (TA_DataBlock *)toBeFreed;

   if( dataBlock )
   {
      FREE_IF_NOT_NULL( dataBlock->open );
      FREE_IF_NOT_NULL( dataBlock->high );
      FREE_IF_NOT_NULL( dataBlock->low );
      FREE_IF_NOT_NULL( dataBlock->close );
      FREE_IF_NOT_NULL( dataBlock->volume );
      FREE_IF_NOT_NULL( dataBlock->openInterest );
      FREE_IF_NOT_NULL( dataBlock->timestamp );

      TA_Free(  dataBlock );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeSplitAdjust( void *toBeFreed )
{
   TA_PROLOG

   TA_TRACE_BEGIN( freeSplitAdjust );

   FREE_IF_NOT_NULL( toBeFreed );

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeValueAdjust( void *toBeFreed )
{
   TA_PROLOG

   TA_TRACE_BEGIN( freeValueAdjust );

   FREE_IF_NOT_NULL( toBeFreed );

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeMergeOp( void *toBeFreed )
{
   TA_PROLOG

   TA_TRACE_BEGIN( freeMergeOp );
    
   FREE_IF_NOT_NULL( toBeFreed );

   TA_TRACE_RETURN( TA_SUCCESS );
}

static void freeBuilderSupport( TA_BuilderSupport *builderSupport )
{
   if( builderSupport != NULL )
   {
      TA_ListFreeAll( builderSupport->listOfSupportForDataSource,
                      freeSupportForDataSource );

      builderSupport->listOfSupportForDataSource = NULL;

      TA_ListFreeAll( builderSupport->listOfMergeOp, freeMergeOp );

      builderSupport->listOfMergeOp = NULL;

      TA_Free(  builderSupport );
   }
}

static TA_RetCode allocHistory( TA_UDBasePriv *privUDB,
                                TA_History **history,
                                TA_Period period,
                                TA_Field fieldToAlloc,
                                TA_BuilderSupport *builderSupport
                               )
{
   TA_PROLOG

   TA_RetCode retCode;
   unsigned int nbDataSource;
   TA_List *listOfSupportForDataSource;
   TA_SupportForDataSource *curSupportForDataSource;

   /* Variable used when changing the period. */
   TA_Real    *open, *high, *low, *close;
   TA_Integer *volume, *openInterest, nbBars;
   TA_Timestamp *timestamp;

   TA_TRACE_BEGIN( allocHistory );

   TA_ASSERT( builderSupport != NULL );
   TA_ASSERT( history != NULL );

   *history = NULL;
   listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;
   TA_ASSERT( listOfSupportForDataSource != NULL );

   nbDataSource = TA_ListSize( listOfSupportForDataSource );
   TA_ASSERT( nbDataSource != 0 );

   curSupportForDataSource = (TA_SupportForDataSource *)TA_ListAccessHead( listOfSupportForDataSource );
   TA_ASSERT( curSupportForDataSource != NULL );

   /* Do "Normalization" of each data source. Normalization will bring all the
    * data source to the same common period so that the merge can take place.
    */
   retCode = TA_PeriodNormalize( builderSupport );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Check for split/value adjustment for all data sources. */
   retCode = historyAdjustData( builderSupport );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Merge the data from the multiple data sources. */
   if( nbDataSource == 1 )
   {
      /* Handle seperatly the simplified case where there is only one data source.
       * In that case, all the logic for merging operations can be bypass.
       */
      retCode = allocHistoryFromOneDataSource( privUDB, history, period,
                                               fieldToAlloc,
                                               curSupportForDataSource,
                                               builderSupport );
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
   }
   else
   {
       *history = allocEmptyHistory( privUDB, period );
       if( *history == NULL )
       {
          TA_TRACE_RETURN( TA_ALLOC_ERR );
       }

       retCode = buildListMergeOp( builderSupport );
       if( retCode != TA_SUCCESS )
       {
          TA_HistoryFree( *history );
          *history = NULL;
          TA_TRACE_RETURN( retCode );
       }

       retCode = buildHistoryFromMergeOp( *history, builderSupport );
       if( retCode != TA_SUCCESS )
       {
          TA_HistoryFree( *history );
          *history = NULL;
          TA_TRACE_RETURN( retCode );
       }
   }

   /* At this point the history merge is completed but not 
    * necesseraly at the requested period. Adjust the
    * timeframe as needed.
    */
   TA_ASSERT( *history != NULL );

   if( (*history)->period != period )
   {
      retCode = TA_PeriodTransform( *history,
                                    period,
                                    &nbBars, &timestamp,
                                    &open, &high, &low, &close,
                                    &volume, &openInterest );

      if( retCode != TA_SUCCESS )
      {
         TA_HistoryFree( *history );
         *history = NULL;
         TA_TRACE_RETURN( retCode );
      }

      /* Success. Free the original data. */
      FREE_IF_NOT_NULL( (*history)->open );
      FREE_IF_NOT_NULL( (*history)->high );
      FREE_IF_NOT_NULL( (*history)->low );
      FREE_IF_NOT_NULL( (*history)->close );
      FREE_IF_NOT_NULL( (*history)->volume );
      FREE_IF_NOT_NULL( (*history)->openInterest );
      FREE_IF_NOT_NULL( (*history)->timestamp );

      /* Replace what was the original data with the new one. */
      (*history)->open         = open;
      (*history)->high         = high;
      (*history)->low          = low;
      (*history)->close        = close;
      (*history)->volume       = volume;
      (*history)->openInterest = openInterest;
      (*history)->timestamp    = timestamp;
      (*history)->nbBars       = nbBars;

      /* Finally, reflect the new period. */
      (*history)->period = period;
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode allocHistoryFromOneDataSource( TA_UDBasePriv *privUDB,
                                                 TA_History **history,
                                                 TA_Period period,
                                                 TA_Field fieldToAlloc,
                                                 TA_SupportForDataSource *supportForDataSource,
                                                 TA_BuilderSupport *builderSupport )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_History *newHistory;
   TA_DataBlock *curDataBlock;
   TA_MergeOp *mergeOp;
   unsigned int nbPriceBar;
   TA_String *sourceName;

   TA_TRACE_BEGIN( allocHistoryFromOneDataSource );

   TA_ASSERT( history != NULL );
   TA_ASSERT( supportForDataSource != NULL );
   TA_ASSERT( builderSupport != NULL );

   *history = NULL;
   retCode = TA_SUCCESS;

   /* Initialize the default history. */
   newHistory = allocEmptyHistory( privUDB, period );
   if( !newHistory )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Indicate the contributing data source. */
   newHistory->listOfSource.string = (const char **)TA_Malloc( sizeof( const char *) );
   if( !newHistory->listOfSource.string )
   {
      TA_HistoryFree( newHistory );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   sourceName = TA_StringDup(TA_GetGlobalStringCache(), supportForDataSource->addDataSourceParamPriv->name );
   TA_ASSERT( sourceName != NULL );
   (newHistory->listOfSource.string)[0] = TA_StringToChar(sourceName);
   newHistory->listOfSource.size = 1;

   /* Transfer the data into the TA_History. */
   curDataBlock = (TA_DataBlock *)TA_ListAccessHead(supportForDataSource->listOfDataBlock);

   if( curDataBlock == NULL )
   {
      /* No data provided! Return success since it could be possible
       * to be normal when the requested start/end period is not available.
       */
      *history = newHistory;
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* Since all data block comes from the same data source, we get the
    * list of provided fields only from the first data block.
    */
   builderSupport->commonFieldProvided = curDataBlock->fieldProvided;

   if( TA_ListSize(supportForDataSource->listOfDataBlock) == 1 )
   {
      /* Optimization if there is only one data block. In that case, the
       * ownership of the data is simply blindly passed to the newHistory.
       */

      /* Find the range of data between start/end */
      
      newHistory->timestamp = curDataBlock->timestamp;
      newHistory->nbBars    = curDataBlock->nbBars;
      newHistory->period    = curDataBlock->period;

      if( fieldToAlloc == TA_ALL )
      {
         newHistory->open         = curDataBlock->open ;
         newHistory->high         = curDataBlock->high;
         newHistory->low          = curDataBlock->low;
         newHistory->close        = curDataBlock->close;
         newHistory->volume       = curDataBlock->volume;
         newHistory->openInterest = curDataBlock->openInterest;
      }
      else
      {
         #define PASS_OWNERSHIP_OR_FREE(upperc,lowerc) \
         { \
            if( fieldToAlloc & TA_##upperc ) \
               newHistory->lowerc = curDataBlock->lowerc; \
            else \
            { \
               FREE_IF_NOT_NULL( curDataBlock->lowerc ); \
            } \
         }
         PASS_OWNERSHIP_OR_FREE(OPEN,open);
         PASS_OWNERSHIP_OR_FREE(HIGH,high);
         PASS_OWNERSHIP_OR_FREE(LOW,low);
         PASS_OWNERSHIP_OR_FREE(CLOSE,close);
         PASS_OWNERSHIP_OR_FREE(VOLUME,volume);
         PASS_OWNERSHIP_OR_FREE(OPENINTEREST,openInterest);
         #undef PASS_OWNERSHIP_OR_FREE
      }

      /* Safety net: since the curDataBlock does not own anymore any
       * allocated memory, make sure all its pointers are set to NULL.
       */
      memset( curDataBlock, 0, sizeof( TA_DataBlock ) );

      *history = newHistory;
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* All data block must be concatenated. Build the list of merge operations
    * required to be done.
    */
   nbPriceBar = 0;
   newHistory->period = curDataBlock->period;

   while( curDataBlock )
   {
      mergeOp = (TA_MergeOp *)TA_Malloc( sizeof( TA_MergeOp ) );
      if( !mergeOp )
      {
         TA_HistoryFree( newHistory );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* Parano test: All datablock shall have the same period. */
      TA_ASSERT( newHistory->period == curDataBlock->period );

      /* Parano test: All datablock shall provide the same fields. */
      TA_ASSERT( builderSupport->commonFieldProvided == curDataBlock->fieldProvided );

      /* Add this TA_MergeOp to the list. */
      builderSupport = supportForDataSource->parent;
      TA_ASSERT( builderSupport != NULL );

      retCode = TA_ListAddTail( builderSupport->listOfMergeOp, mergeOp );
      if( retCode != TA_SUCCESS )
      {
         TA_Free( mergeOp );
         TA_HistoryFree( newHistory );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      mergeOp->srcDataBlock = curDataBlock;
      mergeOp->srcIndexForCopy = 0;
      mergeOp->nbElementToCopy = curDataBlock->nbBars;

      nbPriceBar += curDataBlock->nbBars;

      curDataBlock = (TA_DataBlock *)TA_ListAccessNext( supportForDataSource->listOfDataBlock );
   }

   /* Do the merge operations. */
   builderSupport->nbPriceBar = nbPriceBar;
   retCode = buildHistoryFromMergeOp( newHistory, builderSupport );

   if( retCode != TA_SUCCESS )
   {
      TA_HistoryFree( newHistory );
      TA_TRACE_RETURN( retCode );
   }

   *history = newHistory;
   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode verifyDataBlockValid( TA_DataBlock *dataBlock,
                                        TA_Field fieldToAlloc )
{
   /* This functions attempts to detect potential internal error related
    * a dataBlock. All errors find here are sign of a bug in the
    * software (not an end-user error because of bad data in file etc...).
    */

   /* Timestamp is mandatory. */
   if( !dataBlock->timestamp || !(dataBlock->fieldProvided&TA_TIMESTAMP))
   {
      return TA_INTERNAL_ERROR(40);
   }

   if( fieldToAlloc == TA_ALL )
   {
      /* Verify that at least one field other than timestamp is provided. */
      if( !dataBlock->close &&
          !dataBlock->volume &&
          !dataBlock->open &&
          !dataBlock->high &&
          !dataBlock->low &&
          !dataBlock->openInterest )
      {
         return TA_INTERNAL_ERROR(41);
      }
   }
   else
   {
      /* Verify that the data for the requested fields are provided. */
      #define CONSISTENCY_CHECK(upperc,lowerc) \
         ( (fieldToAlloc&TA_##upperc) && \
           (!(dataBlock->lowerc) || !(dataBlock->fieldProvided&TA_##upperc)) \
         )
      if( CONSISTENCY_CHECK(OPEN, open)    ||
          CONSISTENCY_CHECK(HIGH, high)    ||
          CONSISTENCY_CHECK(LOW, low)      ||
          CONSISTENCY_CHECK(CLOSE,close)   ||
          CONSISTENCY_CHECK(VOLUME,volume) ||
          CONSISTENCY_CHECK(OPENINTEREST, openInterest))
      {
         return TA_INTERNAL_ERROR(42);
      }
      #undef CONSISTENCY_CHECK
   }

   /* If a datablock was created, that means there is at least one bar! */
   if( dataBlock->nbBars == 0 )
   {
      return TA_INTERNAL_ERROR(43);
   }

   /* That datablock looks good. */
   return TA_SUCCESS;
}

static TA_RetCode buildHistoryFromMergeOp( TA_History *newHistory,
                                           TA_BuilderSupport *builderSupport )
{
   TA_PROLOG
   TA_Real *open, *high, *low, *close;
   TA_Integer *openInterest, *volume;
   TA_Timestamp *timestamp;

   TA_List *listOfMergeOp;
   TA_MergeOp *curMergeOp;

   TA_DataBlock *srcDataBlock;
   int srcIndexForCopy;
   int nbElementToCopy;

   int nbPriceBar;
   int nbPriceBarCopied;

   TA_Field fieldToAlloc;

   TA_TRACE_BEGIN( buildHistoryFromMergeOp );

   TA_ASSERT( builderSupport != NULL );

   /* We will merge only the set of fields common to all data source.
    * Particularly useful when the user specify TA_ALL. In other cases,
    * the set kept from each data source will already correspond to the
    * requested fields.
    */
   fieldToAlloc = builderSupport->commonFieldProvided;

   /* At this point, it is still assume that the timestamp was provided by all
    * the datasource. We still want to keep all the timestamp for possibly
    * transposing the TA_History into a different timeframe before returning it
    * to the user.
    */
   TA_ASSERT( fieldToAlloc & TA_TIMESTAMP );

   nbPriceBar = builderSupport->nbPriceBar;
   open = high = low = close = NULL;
   openInterest = volume = NULL;
   timestamp = NULL;

   newHistory->nbBars = 0;
   newHistory->period = 0;

   /* Allocate the price bar data */
   #define FIELD_ALLOC(var,msk,typ) \
   { \
      if(fieldToAlloc&msk) \
      { \
         var = (typ *)TA_Malloc( nbPriceBar * sizeof( typ ) ); \
         if( !var ) {\
            TA_TRACE_RETURN( TA_ALLOC_ERR ); } \
         newHistory->var = var; \
      } \
   }
   FIELD_ALLOC( open,  TA_OPEN,  TA_Real );
   FIELD_ALLOC( high,  TA_HIGH,  TA_Real );
   FIELD_ALLOC( low,   TA_LOW,   TA_Real );
   FIELD_ALLOC( close, TA_CLOSE, TA_Real );
   FIELD_ALLOC( volume, TA_VOLUME, TA_Integer );
   FIELD_ALLOC( openInterest, TA_OPENINTEREST, TA_Integer );
   FIELD_ALLOC( timestamp, TA_TIMESTAMP, TA_Timestamp );
   #undef FIELD_ALLOC

   listOfMergeOp = builderSupport->listOfMergeOp;
   TA_ASSERT( listOfMergeOp != NULL );

   curMergeOp = (TA_MergeOp *)TA_ListAccessHead( listOfMergeOp );

   nbPriceBarCopied = 0;

   if( curMergeOp )
   {
      TA_ASSERT( curMergeOp->srcDataBlock != NULL );
      newHistory->period = curMergeOp->srcDataBlock->period;
   }

   while( curMergeOp )
   {
      srcDataBlock    = curMergeOp->srcDataBlock;
      TA_ASSERT( srcDataBlock != NULL );
      nbElementToCopy = curMergeOp->nbElementToCopy;
      srcIndexForCopy = curMergeOp->srcIndexForCopy;

      /* Parano test: All datablock shall be normalize at the same period at
       *              this point.
       */
      TA_ASSERT( newHistory->period == srcDataBlock->period );

      nbPriceBarCopied += nbElementToCopy;

      /* Parano test: we want to make sure we will not write out of bound
       *              on the allocated newHistory fields.
       */
      if( nbPriceBarCopied > nbPriceBar )
      {
         TA_FATAL(  NULL, nbPriceBarCopied, nbPriceBar );
      }

      /* Parano test: we want to make sure we will not copy out of bound
       *              from the source.
       */
      if( (nbElementToCopy+srcIndexForCopy) > srcDataBlock->nbBars )
      {
         TA_FATAL(  NULL, nbElementToCopy, srcIndexForCopy );
      }

      #define COPY_PRICE_DATA(var,typ) \
      { \
         if (var) \
         { \
           TA_ASSERT(srcDataBlock->var); \
           memcpy(var,&srcDataBlock->var[srcIndexForCopy],nbElementToCopy*sizeof(typ)); \
           var+=nbElementToCopy; \
         } \
      }
      /* Note: Pointer pass as 'var' parameter to the macro is incremented. */
      COPY_PRICE_DATA( open,  TA_Real );
      COPY_PRICE_DATA( high,  TA_Real );
      COPY_PRICE_DATA( low,   TA_Real );
      COPY_PRICE_DATA( close, TA_Real );
      COPY_PRICE_DATA( volume, TA_Integer );
      COPY_PRICE_DATA( openInterest, TA_Integer );
      COPY_PRICE_DATA( timestamp, TA_Timestamp );
      #undef COPY_PRICE_DATA

      curMergeOp = (TA_MergeOp *)TA_ListAccessNext( listOfMergeOp );
   }

   if( nbPriceBarCopied != nbPriceBar )
   {
      TA_FATAL(  NULL, nbPriceBarCopied, nbPriceBar );
   }

   newHistory->nbBars = nbPriceBarCopied;

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode buildListMergeOp( TA_BuilderSupport *builderSupport )
{
   TA_PROLOG
   TA_RetCode retCode;

   unsigned int again, moveForwardCurPos;

   unsigned int nbSupportHavingData;
   unsigned int nbDataToBeCopied;
   unsigned int startIndex;
   unsigned int nbPriceBar;
   TA_DataBlock *tmpDataBlock;
   TA_MergeOp *mergeOp;

   const TA_Timestamp *curLowerPrecedenceTS;
   const TA_Timestamp *oldestTimestamp;
   const TA_Timestamp *nextOverideTimestamp;

   /* All these 'support' variable are tightly coupled. */
   TA_List       *listOfSupportForDataSource;
   TA_ListIter    iterSupport;
   TA_ListIterPos curSupportPos;
   TA_SupportForDataSource *curSupport;

   TA_TRACE_BEGIN(  buildListMergeOp );

   TA_ASSERT( builderSupport != NULL );

   listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;
   TA_ASSERT( listOfSupportForDataSource != NULL );

   retCode = TA_PeriodNormalize( builderSupport );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   /* Will keep track of the total of price bar in the final merged history. */
   nbPriceBar = 0;
   builderSupport->nbPriceBar = 0;

   /* Initialize all the iterators/variables for each data source.
    * At the same time identify 'nbSupportHavingData'.
    */
   nbSupportHavingData = TA_ListSize( listOfSupportForDataSource );
   curSupport = (TA_SupportForDataSource *)TA_ListAccessHead( listOfSupportForDataSource );
   TA_ASSERT( curSupport != NULL );

   while( curSupport )
   {
      TA_ASSERT( curSupport->listOfDataBlock != NULL );

      tmpDataBlock = (TA_DataBlock *)TA_ListAccessHead( curSupport->listOfDataBlock );
      curSupport->curDataBlock = tmpDataBlock;

      if( (tmpDataBlock == NULL) || (tmpDataBlock->nbBars == 0) )
      {
         /* This data source does not provides any data! */
         --nbSupportHavingData;
         curSupport->allDataConsumed = 1;
      }
      else
      {
         TA_ASSERT( tmpDataBlock->timestamp != NULL );
         curSupport->curTimestamp = &tmpDataBlock->timestamp[0];
         curSupport->allDataConsumed = 0;
         curSupport->curIndex = 0;
         curSupport->curLastTimestamp = &tmpDataBlock->timestamp[tmpDataBlock->nbBars-1];
      }

      curSupport = (TA_SupportForDataSource *)TA_ListAccessNext( listOfSupportForDataSource );
   }

   if( nbSupportHavingData < 1 )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(44)  ); /* No data to merge!?!? */
   }

   /* At this point, all TA_SupportForDataSource are setup to point on their
    * first price bar. If no price bar is available 'allDataConsumed' is '1'.
    */

   /* Now, use a more advanced iterator for listOfSupportDataSource. This
    * iterator have the capability to save/restore a position in the list.
    * This kind of "jumping access" into the list is needed for the merging
    * logic.
    */
   retCode = TA_ListIterInit( &iterSupport, listOfSupportForDataSource );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   while( nbSupportHavingData > 0 )
   {
      /* Still have some data to process... here we go... */

      /* Within one iteration in this while loop there is one and only
       * one TA_MergeOp created.
       */

      /* -1- Identify some key values before iterating the price bars
       *
       *  'oldestTimestamp' indicates the oldest timestamp with
       *   higher precedence.
       *
       *   'curSupport' indicates to which data source the 'oldestTimestmap'
       *   belongs to.
       *
       *   'curSupportPos' is a little optimization trick. It will point in
       *   the listOfSupportForDataSource at the position corresponding to
       *   the 'curSupport'.
       *
       *   'nextOverideTimestamp' indicates the next timestamp where
       *   another higher precedence data source has a price bar.
       *
       *   'curLowerPrecedenceTS' indicates the older timestamp among all the
       *   data source of lower precedence. This 'older' timestamp is NEXT to
       *   oldestTimestamp.
       */
      nextOverideTimestamp = NULL;
      curLowerPrecedenceTS = NULL;

      curSupport = (TA_SupportForDataSource *)TA_ListIterHead( &iterSupport );
      TA_ASSERT( curSupport != NULL );

      /* Immediatly bypass data sources with no data left. */
      while( curSupport && (curSupport->allDataConsumed == 1) )
         curSupport = (TA_SupportForDataSource *)TA_ListIterNext( &iterSupport );

      /* Parano test: because nbSupportHavingData > 0, we should have at least
       *              one curSupport with some data!
       */
      TA_ASSERT( curSupport != NULL );

      oldestTimestamp = curSupport->curTimestamp;
      TA_ListIterSavePos( &iterSupport, &curSupportPos );

      curSupport = (TA_SupportForDataSource *)TA_ListIterNext( &iterSupport );
      while( curSupport )
      {
         if( TA_TimestampLess( curSupport->curTimestamp, oldestTimestamp ) )
         {
            nextOverideTimestamp = oldestTimestamp;
            oldestTimestamp = curSupport->curTimestamp;
            TA_ListIterSavePos( &iterSupport, &curSupportPos );
         }

         curSupport = (TA_SupportForDataSource *)TA_ListIterNext( &iterSupport );
      }

      curSupport = TA_ListIterRestorePos( &curSupportPos );

      /* Find the 'curLowerPrecedenceTS'. At the same time
       * make sure all lower precedence data source current price bar
       * is after the oldestTimestamp.
       */
      curLowerPrecedenceTS = adjLowerPrecedenceSupport( &iterSupport,
                                                        oldestTimestamp );
      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }

      /* -2- Iterates into curDataBlock until one of the following event
       *     occur:
       *       - an higher precedence data source price bar is available
       *         (detected by using 'nextOverideTimestamp').
       *
       *       - a lower precedence data source is providing a price bar not
       *         into the curDatablock.
       *
       *       - last time stamp of the curDataBlock.
       *
       *     While iterating, the data is NOT copied. We simply increment
       *     nbDataToBeCopied who is going to be used to build the TA_MergeOp.
       *
       *     While iterating, all lowest precedence data source will get
       *     iterated as well for overlaping price bar.
       *
       *     While iterating, we check that the timestamp are really ascending
       *     (quick data integrity check).
       */
      TA_ASSERT( curSupport != NULL );
      TA_ASSERT( oldestTimestamp != NULL );

      /* Create the TA_MergeOp. */
      mergeOp = (TA_MergeOp *)TA_Malloc( sizeof( TA_MergeOp ) );
      if( !mergeOp )
      {
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      nbDataToBeCopied = 0;
      startIndex = curSupport->curIndex;
      again = 1;
      while( again )
      {
         moveForwardCurPos = 0;

         if( (nextOverideTimestamp && !TA_TimestampLess(oldestTimestamp,nextOverideTimestamp))  )
         {
            /* A price bar of higher precedence has been detected. */
            again = 0;

            /* If the overiding price bar is also overlapping, we can skip the
             * current price bar in curPos.
             */
            if( TA_TimestampEqual( oldestTimestamp, nextOverideTimestamp ) )
               moveForwardCurPos = 1;
         }
         else if( curLowerPrecedenceTS == NULL )
         {
            moveForwardCurPos = 1;
            nbDataToBeCopied++;
         }
         else
         {
            /* Check among the lowest precedence data source if they provides
             * a price bar that this curSupport does not have.
             */
            if( TA_TimestampLess( curLowerPrecedenceTS, oldestTimestamp ) )
            {
               /* A lower precedence data source can provide a price bar not
                * found in curSupport.
                */
               again = 0;
            }
            else if( TA_TimestampEqual( curLowerPrecedenceTS, oldestTimestamp ) )
            {
               /* Overlaping from lower precedence data source. */

               /* The price bar will be taken from curSupport. */
               moveForwardCurPos = 1;
               nbDataToBeCopied++;

               /* Adjust all the lower precedence data source.
                * Their current price bar will be adjusted to be after
                * the curLowerPrecedenceTS.
                */
               curLowerPrecedenceTS = adjLowerPrecedenceSupport( &iterSupport,
                                                                 curLowerPrecedenceTS  );
            }
         }

         /* Trap the case when this was the last price bar of the data block. */
         if( again && (oldestTimestamp == curSupport->curLastTimestamp) )
         {
            again = 0;
            moveForwardCurPos = 1;
         }

         /* Initialize the TA_MergeOp if we are about to exit the loop */
         if( again == 0 )
         {
            /* Keep track of the fields common to all data block merged. */
            if( builderSupport->commonFieldProvided == TA_ALL )
               builderSupport->commonFieldProvided = curSupport->fieldProvided;
            else
               builderSupport->commonFieldProvided &= curSupport->fieldProvided;

            /* Setup the mergOp */
            mergeOp->srcDataBlock = curSupport->curDataBlock;
            mergeOp->srcIndexForCopy = startIndex;
            mergeOp->nbElementToCopy = nbDataToBeCopied;
            nbPriceBar += nbDataToBeCopied;
         }

         /* Move forward to the next timestamp into curSupport. */
         if( moveForwardCurPos )
         {
            oldestTimestamp = nextPriceBar( curSupport, oldestTimestamp );
            TA_ASSERT( !((oldestTimestamp == NULL) && (again == 1)) );
         }
      }

      TA_ASSERT( nbDataToBeCopied > 0 );

      /* -3- Add the TA_MergeOp to the list. */
      TA_ASSERT( builderSupport->listOfMergeOp != NULL );
      retCode = TA_ListAddTail( builderSupport->listOfMergeOp, mergeOp );
      if( retCode != TA_SUCCESS )
      {
         TA_Free(  mergeOp );
         TA_TRACE_RETURN( retCode );
      }

      /* At this point, all TA_SupportForDataSource are setup to point on their
       * next price bar who need processing. When no further price bar is
       * available their repsective 'allDataConsumed' is '1'.
       */

      /* If there is some data left, start over the whole logic for
       * creating another TA_MergeOp.
       */
   }

   builderSupport->nbPriceBar = nbPriceBar;

   TA_TRACE_RETURN( TA_SUCCESS );
}

static const TA_Timestamp *adjLowerPrecedenceSupport( TA_ListIter *curSupport,
                                                      const TA_Timestamp *minTimestamp )
{
   TA_RetCode retCode;
   TA_SupportForDataSource *curSupportForDataSource;
   const TA_Timestamp *newLowerLimit;
   const TA_Timestamp *tmpLowerLimit;

   TA_ListIterPos savedListPos;

   /* Bring all lower precedence data source to a price bar after
    * the specified 'curLowerPrecedenceTS'.
    * On TA_SUCCESS, the pointer 'curLowerPrecedenceTS' will indicates
    * the new oldest timestamp among the lower precedence data source.
    */
   newLowerLimit = NULL;

   retCode = TA_ListIterSavePos( curSupport, &savedListPos );
   if( retCode != TA_SUCCESS )
      return NULL;

   /* Skip current data source. */
   curSupportForDataSource = (TA_SupportForDataSource *)TA_ListIterNext( curSupport );

   while( curSupportForDataSource )
   {
      if( TA_TimestampEqual( curSupportForDataSource->curTimestamp, minTimestamp ) ||
          TA_TimestampLess  ( curSupportForDataSource->curTimestamp, minTimestamp ) )
      {
         tmpLowerLimit = nextPriceBar( curSupportForDataSource, minTimestamp );

         if( tmpLowerLimit )
         {
            if( !newLowerLimit || TA_TimestampLess(tmpLowerLimit,newLowerLimit) )
               newLowerLimit = tmpLowerLimit;
         }
      }

      curSupportForDataSource = (TA_SupportForDataSource *)TA_ListIterNext( curSupport );
   }

   TA_ListIterRestorePos( &savedListPos );

   return newLowerLimit;
}

static const TA_Timestamp *nextPriceBar( TA_SupportForDataSource *supportForDataSource,
                                         const TA_Timestamp *minTimestamp )
{
   TA_DataBlock *curDataBlock;
   int curIndex;

   if( supportForDataSource->allDataConsumed )
      return NULL;

   while( !TA_TimestampLess( minTimestamp, supportForDataSource->curTimestamp ) )
   {
      curDataBlock = supportForDataSource->curDataBlock;
      curIndex     = supportForDataSource->curIndex;

      if( curIndex == (curDataBlock->nbBars-1) )
      {
         /* Must move to the next data block. */
         curDataBlock = (TA_DataBlock *)TA_ListAccessNext( supportForDataSource->listOfDataBlock );
         if( !curDataBlock )
         {
            supportForDataSource->allDataConsumed = 1;
            return NULL;
         }

         supportForDataSource->curDataBlock = curDataBlock;
         supportForDataSource->curIndex = 0;
         supportForDataSource->curLastTimestamp = &curDataBlock->timestamp[curDataBlock->nbBars-1];
      }
      else
         supportForDataSource->curIndex++;

      supportForDataSource->curTimestamp = &curDataBlock->timestamp[supportForDataSource->curIndex];
   }

   return supportForDataSource->curTimestamp;
}

static TA_History *allocEmptyHistory( TA_UDBasePriv *privUDB, TA_Period period )
{
   TA_History *newHistory;
   TA_HistoryHiddenData *hiddenData;

   /* Alloc the TA_History for the user. */
   newHistory = (TA_History *)TA_Malloc( sizeof( TA_History ) );

   if( !newHistory )
      return NULL;

   memset( newHistory, 0, sizeof( TA_History ) );
   newHistory->period = period;

   /* Alloc and initialize the hidden data. */
   hiddenData = (TA_HistoryHiddenData *)TA_Malloc( sizeof( TA_HistoryHiddenData ) );
   memset( hiddenData,0,sizeof(TA_HistoryHiddenData));
   if( !hiddenData )
   {
      TA_Free( newHistory );
      return NULL;
   }

   newHistory->hiddenData = hiddenData;
   hiddenData->privUDB = privUDB;

   return newHistory;
}


#define REVERSE_MACRO( varType ) \
   unsigned int nbSwap, begIndex, endIndex; \
   varType temp; \
   nbSwap = nbElement>>1; \
   endIndex = nbElement-1; \
   for( begIndex=0; begIndex < nbSwap; begIndex++, endIndex-- ) \
   { \
      temp = table[begIndex]; \
      table[begIndex] = table[endIndex]; \
      table[endIndex] = temp; \
   }
static void reverseRealElement( unsigned int nbElement,  TA_Real *table )
{ REVERSE_MACRO(TA_Real)}
static void reverseIntegerElement( unsigned int nbElement,  TA_Integer *table )
{ REVERSE_MACRO(TA_Integer) }
static void reverseTimestampElement( unsigned int nbElement, TA_Timestamp *table )
{ REVERSE_MACRO(TA_Timestamp) }
#undef REVERSE_MACRO

static void trimBeforeStart( const TA_Timestamp *start, TA_Period period, TA_History *history )
{
   /* !!! Could be speed optimized */
   int found;
   unsigned int i;

   /* Trap special case (nothing to do) */
   if( history->nbBars == 0 )
      return;

   /* Find the first time timestamp[i] >= start */
   found = 0;
   i = 0;

   if( period < TA_DAILY )
   {
      while( !found && (i < history->nbBars) )
      {
         if( !TA_TimestampLess( &history->timestamp[i], start ) )
            found = 1;
         else
            i++;
      }
   }
   else
   { 
      while( !found && (i < history->nbBars) )
      {
         if( !TA_TimestampDateLess( &history->timestamp[i], start ) )
            found = 1;
         else
            i++;
      }
   }

   /* First price bar is at or after "start". Do nothing. */
   if( i == 0 )
      return;

   /* Adjust the history */
   history->nbBars -= i;

   if( history->nbBars == 0 )
   {
      history->open         = NULL;
      history->high         = NULL;
      history->low          = NULL;
      history->close        = NULL;
      history->volume       = NULL;
      history->openInterest = NULL;      
      history->timestamp    = NULL;
   }
   else
   {
      #define ADJUST_START(a) {if(history->a) history->a=&history->a[i];}
      ADJUST_START(open);
      ADJUST_START(high);
      ADJUST_START(low);
      ADJUST_START(close);
      ADJUST_START(volume);
      ADJUST_START(openInterest);
      ADJUST_START(timestamp);
      #undef ADJUST_START
   }
}

static void trimAfterEnd( const TA_Timestamp *end, TA_Period period, TA_History *history )
{
   /* !!! Could be speed optimized */
   int found;
   int i;

   /* Trap special case (nothing to do) */
   if( history->nbBars == 0 )
      return;
   
   /* Find the first time timestamp[i] <= end 
    * (starting from the end).
    */
   found = 0;
   i = history->nbBars-1;
   if( period < TA_DAILY )
   {
      while( !found && (i >= 0) )
      {
         if( !TA_TimestampGreater( &history->timestamp[i], end ) )
            found = 1;
         else
            i--;
      }
   }
   else
   {
      while( !found && (i >= 0) )
      {
         if( !TA_TimestampDateGreater( &history->timestamp[i], end ) )
            found = 1;
         else
            i--;
      }
   }

   if( !found  || (i < 0) )
   {
      history->nbBars = 0;
      history->open         = NULL;
      history->high         = NULL;
      history->low          = NULL;
      history->close        = NULL;
      history->volume       = NULL;
      history->openInterest = NULL;      
      history->timestamp    = NULL;
   }
   else
   {
      history->nbBars -= (history->nbBars-1)-i;
   }
}
