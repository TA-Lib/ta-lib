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
 *  070800 MF   First version.
 *
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

static TA_BuilderSupport *allocBuilderSupport( TA_Libc *libHandle,
                                               unsigned int nbDataSource );
static void freeBuilderSupport( TA_Libc *libHandle,
                                TA_BuilderSupport *builderSupport );

static TA_RetCode freeSupportForDataSource( TA_Libc *libHandle,
                                            void *toBeFreed );
static TA_RetCode freeDataBlock( TA_Libc *libHandle,
                                 void *toBeFreed );
static TA_RetCode freeMergeOp( TA_Libc *libHandle,
                               void *toBeFreed );

static TA_RetCode allocHistory( TA_Libc *libHandle,
                                TA_UDBasePriv *privUDB,
                                TA_History **history,
                                TA_Period period,
                                TA_Field fieldToAlloc,
                                TA_BuilderSupport *builderSupport );

static TA_RetCode allocHistoryFromOneDataSource( TA_Libc *libHandle,
                                                 TA_UDBasePriv *privUDB,
                                                 TA_History **history,
                                                 TA_Period period,
                                                 TA_Field fieldToAlloc,
                                                 TA_SupportForDataSource *curSupportForDataSource,
                                                 TA_BuilderSupport *builderSupport );

static TA_RetCode verifyDataBlockValid( TA_DataBlock *dataBlock,
                                        TA_Field fieldToAlloc );

static void stopAllGetDataThread( TA_BuilderSupport *builderSupport );

static TA_RetCode buildListMergeOp( TA_Libc *libHandle,
                                    TA_BuilderSupport *builderSupport );

static TA_RetCode buildHistoryFromMergeOp( TA_Libc *libHandle,
                                           TA_History *newHistory,
                                           TA_BuilderSupport *builderSupport );


static const TA_Timestamp *adjLowerPrecedenceSupport( TA_ListIter *curSupport,
                                                      const TA_Timestamp *minTimestamp );

static const TA_Timestamp *nextPriceBar( TA_SupportForDataSource *supportForDataSource,
                                         const TA_Timestamp *minTimestamp );

static TA_History *allocEmptyHistory( TA_Libc *libHandle,
                                      TA_UDBasePriv *privUDB, 
                                      TA_Period period );

static void reverseRealElement( unsigned int nbElement,  TA_Real *table );
static void reverseIntegerElement( unsigned int nbElement,  TA_Integer *table );
static void reverseTimestampElement( unsigned int nbElement, TA_Timestamp *table );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode TA_HistoryBuilder( TA_Libc             *libHandle,
                              TA_UDBasePriv       *privUDB,
                              TA_UDB_Symbol       *symbolData,
                              TA_Period            period,
                              const TA_Timestamp  *start,
                              const TA_Timestamp  *end,
                              TA_Field             fieldToAlloc,
                              TA_History         **history )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_List *listDriverHandle;
   TA_List *listOfSupportForDataSource;
   TA_UDB_Driver *driverHandles;
   TA_SupportForDataSource *supportForDataSource;
   TA_BuilderSupport *builderSupport;
   unsigned int nbDataSource;
   unsigned int driverIndex;
   const TA_DataSourceDriver *driver;

   TA_PAR_VARS;

   TA_TRACE_BEGIN( libHandle, TA_HistoryBuilder );

   TA_ASSERT( libHandle, symbolData != NULL );
   TA_ASSERT( libHandle, history != NULL );

   *history = NULL;

   listDriverHandle = &symbolData->listDriverHandle;

   /* Cover for a common potential error (passing -1 as
    * a default parameter!?).
    */
   if( start == (TA_Timestamp *)-1 )
      start = (TA_Timestamp *)0;
   if( end == (TA_Timestamp *)-1 )
      end = (TA_Timestamp *)0;

   /* Allocate the builder support. Everything allocated from this
    * point will be referred one way or another from this structure.
    * If something goes wrong, this structure and all sub-elements will
    * be freed.
    */
   nbDataSource = TA_ListSize(listDriverHandle);
   TA_ASSERT( libHandle, nbDataSource != 0 );
   builderSupport = allocBuilderSupport( libHandle, nbDataSource );

   if( !builderSupport )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Start the thread for each data source. */
   driverHandles = (TA_UDB_Driver *)TA_ListAccessHead( listDriverHandle );

   if( !driverHandles )
   {
      freeBuilderSupport( libHandle, builderSupport );
      TA_TRACE_RETURN( TA_UNKNOWN_ERR  );
   }

   listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;
   TA_ASSERT( libHandle, listOfSupportForDataSource != NULL );
   supportForDataSource = (TA_SupportForDataSource *)TA_ListAccessHead( listOfSupportForDataSource );

   /* Parano test: The following code assume that the listDriverHandle and
    * listOfSupportForDataSource have the same number of elements.
    */
   TA_ASSERT( libHandle, TA_ListSize(listOfSupportForDataSource) == nbDataSource );

   /* Initialize variable used for parallel processing. */
   TA_PAR_INIT(libHandle);

   while( driverHandles && (builderSupport->retCode == TA_SUCCESS) )
   {
      TA_ASSERT( libHandle, supportForDataSource != NULL );

      /* Set-up the parameters. */
      supportForDataSource->libHandle      = libHandle;
      supportForDataSource->index          = driverHandles->index;
      supportForDataSource->sourceHandle   = driverHandles->sourceHandle;
      supportForDataSource->categoryHandle = driverHandles->categoryHandle;
      supportForDataSource->symbolHandle   = &driverHandles->symbolHandle;
      supportForDataSource->period         = period;
      supportForDataSource->start          = start;
      supportForDataSource->end            = end;
      supportForDataSource->fieldToAlloc   = fieldToAlloc;

      driverIndex = supportForDataSource->index;
      TA_ASSERT( libHandle, driverIndex < TA_gDataSourceTableSize );

      driver = &TA_gDataSourceTable[ driverIndex ];
      TA_ASSERT( libHandle, driver != NULL );

      if( driver->getParameters == NULL )
      {
         TA_FATAL( libHandle, "Get parameters must be implemented", 0, 0 );
         stopAllGetDataThread( builderSupport );
         builderSupport->retCode = TA_UNKNOWN_ERR;
         break; /* Exit the loop righ away! */
      }
      else
      {
         retCode = (*(driver->getParameters))( libHandle, &supportForDataSource->param );
         if( retCode != TA_SUCCESS )
         {
            stopAllGetDataThread( builderSupport );
            builderSupport->retCode = retCode;
            break; /* Exit the loop righ away! */
         }
      }

      /* Check immediatly for the next driver handles. */
      driverHandles = (TA_UDB_Driver *)TA_ListAccessNext( listDriverHandle );

      /* Get the data from each slowAccess data source in a different thread.
       * Except for the last driver which always get executed in the
       * main thread.
       */
      if( (driverHandles != NULL) && supportForDataSource->param.slowAccess )
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
      freeBuilderSupport( libHandle, builderSupport );
      TA_TRACE_RETURN( retCode );
   }

   /* Finally, allocate and build the TA_History. */
   retCode = allocHistory( libHandle, privUDB, history, period, fieldToAlloc, builderSupport );
   freeBuilderSupport( libHandle, builderSupport );

   if( retCode != TA_SUCCESS )
   {
      *history = NULL;
      TA_TRACE_RETURN( retCode );
   }

   /* Until the library is very stable, we will make a double check
    * of the integrity of the "almost final" TA_History.
    */
   retCode = TA_HistoryCheckInternal( libHandle, period, start, end,
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
      FREE_IF_NOT_NULL( libHandle, (*history)->timestamp );
      (*history)->timestamp = NULL;
   }

   TA_TRACE_RETURN( retCode );
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
   TA_PROLOG;
   TA_RetCode retCode;
   TA_SupportForDataSource *supportForDataSource;
   TA_BuilderSupport   *builderSupport;
   TA_DataBlock *newBlock;
   TA_Field fieldProvided;
   const TA_Timestamp *lowTimestamp;
   const TA_Timestamp *highTimestamp;
   unsigned int thisBlockGoesAfterTheExistingOnes;
   TA_Libc *libHandle;

   newBlock = NULL;

   /* Check the parameters, and identify the context of this data. */
   if( paramForAddData == NULL )
      return TA_BAD_PARAM;

   supportForDataSource = (TA_SupportForDataSource *)paramForAddData;
   if( !supportForDataSource )
      return TA_UNKNOWN_ERR;

   libHandle = supportForDataSource->libHandle;
   if( !libHandle )
      return TA_UNKNOWN_ERR;

   TA_TRACE_BEGIN( libHandle, TA_HistoryAddData );

   builderSupport = supportForDataSource->parent;
   TA_ASSERT( libHandle, builderSupport != NULL );

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
         retCode = TA_UNKNOWN_ERR;
         goto TA_HistoryAddData_EXIT;
      }

      /* Eliminates possible extra fields that are not requested. */
      #define ELIMINATE_NOT_REQUESTED(lowerc,upperc) \
      { \
         if( lowerc && !(fieldProvided&TA_##upperc) ) \
         { \
            TA_Free( libHandle, lowerc ); \
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
      retCode = TA_UNKNOWN_ERR;
      goto TA_HistoryAddData_EXIT;
   }

   /* Verify that all added data are in the same timeframe. */
   if( !supportForDataSource->periodProvided )
      supportForDataSource->periodProvided = period;
   else if( period != supportForDataSource->periodProvided )
   {
      /* The driver is not consistent about the period provided. */
      retCode = TA_UNKNOWN_ERR;
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
      TA_ASSERT( libHandle, supportForDataSource->lowestTimestamp != NULL );

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
   newBlock = (TA_DataBlock *)TA_Malloc( libHandle, sizeof( TA_DataBlock ) );

   if( !newBlock )
   {
      retCode = TA_ALLOC_ERR;
      goto TA_HistoryAddData_EXIT;
   }

   memset( newBlock, 0, sizeof( TA_DataBlock ) );

   #define TRANSFER_OWNERSHIP_TO_NEWBLOCK(par) newBlock->par=par; par=NULL;
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(timestamp);
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(open);
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(high);
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(low);
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(close);
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(volume);
   TRANSFER_OWNERSHIP_TO_NEWBLOCK(openInterest);
   #undef TRANSFER_OWNERSHIP_TO_NEWBLOCK
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

   /* The 'listOfDataBlock' now owns the newBlock. */
   newBlock = NULL;

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

   retCode = TA_SUCCESS;

TA_HistoryAddData_EXIT:  /* The only point of this function. */

   /* Free locally allocated resource (if still owner). */
   FREE_IF_NOT_NULL( libHandle, newBlock );

   /* ALWAYS make sure all the provided data is freed. ALWAYS. */
   FREE_IF_NOT_NULL( libHandle, open );
   FREE_IF_NOT_NULL( libHandle, high );
   FREE_IF_NOT_NULL( libHandle, low );
   FREE_IF_NOT_NULL( libHandle, close );
   FREE_IF_NOT_NULL( libHandle, volume );
   FREE_IF_NOT_NULL( libHandle, openInterest );
   FREE_IF_NOT_NULL( libHandle, timestamp );

   /* Any major error will fail the entire read operation
    * (including all other data source, even if they did succeed).
    */
   if( (retCode != TA_SUCCESS) && (retCode != TA_ENOUGH_DATA) )
      builderSupport->retCode = retCode;

   TA_TRACE_RETURN( retCode );
}

/**** Local functions definitions.     ****/
void getDataThread( void *args  )
{
   TA_RetCode retCode;
   TA_Libc *libHandle;

   const TA_DataSourceDriver *driver;
   unsigned int driverIndex;
   TA_SupportForDataSource *supportForDataSource;

   supportForDataSource = (TA_SupportForDataSource *)args;
   if( !args )
      return;

   libHandle = supportForDataSource->libHandle;
   driverIndex = supportForDataSource->index;
   TA_ASSERT_NO_RET( libHandle, driverIndex < TA_gDataSourceTableSize );

   driver = &TA_gDataSourceTable[ driverIndex ];
   TA_ASSERT_NO_RET( libHandle, driver != NULL );

   retCode = TA_SUCCESS;

   if( driver->getHistoryData )
   {
      if( !supportForDataSource->enoughValidDataProvided )
      {
         retCode = (*(driver->getHistoryData))( supportForDataSource->libHandle,
                                                supportForDataSource->sourceHandle,
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

static TA_BuilderSupport *allocBuilderSupport( TA_Libc *libHandle,
                                               unsigned int nbDataSource )
{
  TA_RetCode retCode;
  unsigned int i;
  TA_BuilderSupport *builderSupport;
  TA_List *listOfSupportForDataSource; /* List of TA_SupportForDataSource. */
  TA_SupportForDataSource *supportForDataSource;

  TA_ASSERT_RET( libHandle, nbDataSource > 0, (TA_BuilderSupport *)NULL );

  builderSupport = (TA_BuilderSupport *)TA_Malloc( libHandle,
                                                   sizeof( TA_BuilderSupport ) );

  if( !builderSupport )
     return (TA_BuilderSupport *)NULL;

  builderSupport->nbPriceBar = 0;
  builderSupport->retCode = TA_SUCCESS;
  builderSupport->listOfMergeOp = NULL;
  builderSupport->commonFieldProvided = TA_ALL;

  builderSupport->listOfSupportForDataSource = TA_ListAlloc( libHandle );
  if( !builderSupport->listOfSupportForDataSource )
  {
     freeBuilderSupport( libHandle, builderSupport );
     return (TA_BuilderSupport *)NULL;
  }
  listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;

  for( i=nbDataSource; i > 0; i-- )
  {
     supportForDataSource = (TA_SupportForDataSource *)TA_Malloc( libHandle,
                                                                  sizeof(TA_SupportForDataSource));
     if( !supportForDataSource )
     {
        freeBuilderSupport( libHandle, builderSupport );
        return (TA_BuilderSupport *)NULL;
     }

     supportForDataSource->libHandle = libHandle;
     
     /* Initialize all fields to NULL. */
     memset( supportForDataSource, 0, sizeof( TA_SupportForDataSource ) );

     /* Make parent the 'builderSupport' */
     supportForDataSource->parent = builderSupport;

     /* Add this TA_SupportForDataSource to the list. */
     retCode = TA_ListAddTail( listOfSupportForDataSource, supportForDataSource );
     if( retCode != TA_SUCCESS )
     {
        freeBuilderSupport( libHandle, builderSupport );
        return (TA_BuilderSupport *)NULL;
     }

     /* Initialize an empty 'listOfDataBlock'. */
     supportForDataSource->listOfDataBlock = TA_ListAlloc( libHandle );

     if( !supportForDataSource->listOfDataBlock )
     {
        freeBuilderSupport( libHandle, builderSupport );
        return (TA_BuilderSupport *)NULL;
     }
  }

  /* Initialize an empty 'listOfMergeOp'. */
  builderSupport->listOfMergeOp = TA_ListAlloc( libHandle );

  if( !builderSupport->listOfMergeOp )
  {
     freeBuilderSupport( libHandle, builderSupport );
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

static TA_RetCode freeSupportForDataSource( TA_Libc *libHandle, void *toBeFreed )
{
   TA_PROLOG;
   TA_SupportForDataSource *supportForDataSource;

   TA_TRACE_BEGIN( libHandle, freeSupportForDataSource );

   supportForDataSource = (TA_SupportForDataSource *)toBeFreed;

   if( supportForDataSource )
   {
      if( supportForDataSource->listOfDataBlock )
         TA_ListFreeAll( supportForDataSource->listOfDataBlock, freeDataBlock );

      supportForDataSource->listOfDataBlock = NULL;

      TA_Free( libHandle, supportForDataSource );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeDataBlock( TA_Libc *libHandle, void *toBeFreed )
{
   TA_PROLOG;
   TA_DataBlock *dataBlock;

   TA_TRACE_BEGIN( libHandle, freeDataBlock );

   dataBlock = (TA_DataBlock *)toBeFreed;

   if( dataBlock )
   {
      FREE_IF_NOT_NULL( libHandle, dataBlock->open );
      FREE_IF_NOT_NULL( libHandle, dataBlock->high );
      FREE_IF_NOT_NULL( libHandle, dataBlock->low );
      FREE_IF_NOT_NULL( libHandle, dataBlock->close );
      FREE_IF_NOT_NULL( libHandle, dataBlock->volume );
      FREE_IF_NOT_NULL( libHandle, dataBlock->openInterest );
      FREE_IF_NOT_NULL( libHandle, dataBlock->timestamp );

      TA_Free( libHandle, dataBlock );
   }

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode freeMergeOp( TA_Libc *libHandle, void *toBeFreed )
{
   TA_PROLOG;

   TA_TRACE_BEGIN( libHandle, freeMergeOp );
    
   FREE_IF_NOT_NULL( libHandle, toBeFreed );

   TA_TRACE_RETURN( TA_SUCCESS );
}

static void freeBuilderSupport( TA_Libc *libHandle, TA_BuilderSupport *builderSupport )
{
   if( builderSupport != NULL )
   {
      TA_ListFreeAll( builderSupport->listOfSupportForDataSource,
                      freeSupportForDataSource );

      builderSupport->listOfSupportForDataSource = NULL;

      TA_ListFreeAll( builderSupport->listOfMergeOp, freeMergeOp );

      builderSupport->listOfMergeOp = NULL;

      TA_Free( libHandle, builderSupport );
   }
}

static TA_RetCode allocHistory( TA_Libc *libHandle,
                                TA_UDBasePriv *privUDB,
                                TA_History **history,
                                TA_Period period,
                                TA_Field fieldToAlloc,
                                TA_BuilderSupport *builderSupport
                               )
{
   TA_PROLOG;

   TA_RetCode retCode;
   unsigned int nbDataSource;
   TA_List *listOfSupportForDataSource;
   TA_SupportForDataSource *curSupportForDataSource;

   /* Variable used when changing the period. */
   TA_Real    *open, *high, *low, *close;
   TA_Integer *volume, *openInterest, nbBars;
   TA_Timestamp *timestamp;

   TA_TRACE_BEGIN( libHandle, allocHistory );

   TA_ASSERT( libHandle, builderSupport != NULL );
   TA_ASSERT( libHandle, history != NULL );

   *history = NULL;
   listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;
   TA_ASSERT( libHandle, listOfSupportForDataSource != NULL );

   nbDataSource = TA_ListSize( listOfSupportForDataSource );
   TA_ASSERT( libHandle, nbDataSource != 0 );

   curSupportForDataSource = (TA_SupportForDataSource *)TA_ListAccessHead( listOfSupportForDataSource );
   TA_ASSERT( libHandle, curSupportForDataSource != NULL );

   if( nbDataSource == 1 )
   {
      /* Handle seperatly the simplified case where there is only one data source.
       * In that case, all the logic for merging operations can be bypass.
       */
      retCode = allocHistoryFromOneDataSource( libHandle, privUDB, history, period,
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
       *history = allocEmptyHistory( libHandle, privUDB, period );
       if( *history == NULL )
       {
          TA_TRACE_RETURN( TA_ALLOC_ERR );
       }

       retCode = buildListMergeOp( libHandle, builderSupport );
       if( retCode != TA_SUCCESS )
       {
          TA_HistoryFree( *history );
          *history = NULL;
          TA_TRACE_RETURN( retCode );
       }

       retCode = buildHistoryFromMergeOp( libHandle, *history, builderSupport );
       if( retCode != TA_SUCCESS )
       {
          TA_HistoryFree( *history );
          *history = NULL;
          TA_TRACE_RETURN( retCode );
       }
   }

   /* At this point the "normalized" history is allocated but
    * not necesseraly at the requested period. Adjust the
    * timeframe as needed.
    */
   TA_ASSERT( libHandle, *history != NULL );

   if( (*history)->period != period )
   {
      retCode = TA_PeriodTransform( libHandle,
                                    *history,
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
      FREE_IF_NOT_NULL( libHandle, (*history)->open );
      FREE_IF_NOT_NULL( libHandle, (*history)->high );
      FREE_IF_NOT_NULL( libHandle, (*history)->low );
      FREE_IF_NOT_NULL( libHandle, (*history)->close );
      FREE_IF_NOT_NULL( libHandle, (*history)->volume );
      FREE_IF_NOT_NULL( libHandle, (*history)->openInterest );
      FREE_IF_NOT_NULL( libHandle, (*history)->timestamp );

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

static TA_RetCode allocHistoryFromOneDataSource( TA_Libc *libHandle,
                                                 TA_UDBasePriv *privUDB,
                                                 TA_History **history,
                                                 TA_Period period,
                                                 TA_Field fieldToAlloc,
                                                 TA_SupportForDataSource *supportForDataSource,
                                                 TA_BuilderSupport *builderSupport )
{
   TA_PROLOG;
   TA_RetCode retCode;
   TA_History *newHistory;
   TA_DataBlock *curDataBlock;
   TA_MergeOp *mergeOp;
   unsigned int nbPriceBar;

   TA_TRACE_BEGIN( libHandle, allocHistoryFromOneDataSource );

   TA_ASSERT( libHandle, history != NULL );
   TA_ASSERT( libHandle, supportForDataSource != NULL );
   TA_ASSERT( libHandle, builderSupport != NULL );

   *history = NULL;
   retCode = TA_SUCCESS;

   /* Initialize the default history. */
   newHistory = allocEmptyHistory( libHandle, privUDB, period );
   if( !newHistory )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Transfer the data into the TA_History. */
   curDataBlock = (TA_DataBlock *)TA_ListAccessHead(supportForDataSource->listOfDataBlock);

   if( curDataBlock == NULL )
   {
      /* No data provided! Return success though since it could
       * be possible if the requested start/end period is not available.
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
               FREE_IF_NOT_NULL( libHandle, curDataBlock->lowerc ); \
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
      mergeOp = (TA_MergeOp *)TA_Malloc( libHandle, sizeof( TA_MergeOp ) );
      if( !mergeOp )
      {
         TA_HistoryFree( newHistory );
         TA_TRACE_RETURN( TA_ALLOC_ERR );
      }

      /* Parano test: All datablock shall have the same period. */
      TA_ASSERT( libHandle, newHistory->period == curDataBlock->period );

      /* Parano test: All datablock shall provide the same fields. */
      TA_ASSERT( libHandle, builderSupport->commonFieldProvided == curDataBlock->fieldProvided );

      /* Add this TA_MergeOp to the list. */
      builderSupport = supportForDataSource->parent;
      TA_ASSERT( libHandle, builderSupport != NULL );

      retCode = TA_ListAddTail( builderSupport->listOfMergeOp, mergeOp );
      if( retCode != TA_SUCCESS )
      {
         TA_Free( libHandle, mergeOp );
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
   retCode = buildHistoryFromMergeOp( libHandle, newHistory, builderSupport );

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
      return TA_UNKNOWN_ERR;
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
         return TA_UNKNOWN_ERR;
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
         return TA_UNKNOWN_ERR;
      }
      #undef CONSISTENCY_CHECK
   }

   /* If a datablock was created, that means there is at least one bar! */
   if( dataBlock->nbBars == 0 )
   {
      return TA_UNKNOWN_ERR;
   }

   /* That datablock looks good. */
   return TA_SUCCESS;
}

static TA_RetCode buildHistoryFromMergeOp( TA_Libc *libHandle,
                                           TA_History *newHistory,
                                           TA_BuilderSupport *builderSupport )
{
   TA_PROLOG;
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

   TA_TRACE_BEGIN( libHandle, buildHistoryFromMergeOp );

   TA_ASSERT( libHandle, builderSupport != NULL );

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
   TA_ASSERT( libHandle, fieldToAlloc & TA_TIMESTAMP );

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
         var = (typ *)TA_Malloc( libHandle, nbPriceBar * sizeof( typ ) ); \
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
   TA_ASSERT( libHandle, listOfMergeOp != NULL );

   curMergeOp = (TA_MergeOp *)TA_ListAccessHead( listOfMergeOp );

   nbPriceBarCopied = 0;

   if( curMergeOp )
   {
      TA_ASSERT( libHandle, curMergeOp->srcDataBlock != NULL );
      newHistory->period = curMergeOp->srcDataBlock->period;
   }

   while( curMergeOp )
   {
      srcDataBlock    = curMergeOp->srcDataBlock;
      TA_ASSERT( libHandle, srcDataBlock != NULL );
      nbElementToCopy = curMergeOp->nbElementToCopy;
      srcIndexForCopy = curMergeOp->srcIndexForCopy;

      /* Parano test: All datablock shall be normalize at the same period at
       *              this point.
       */
      TA_ASSERT( libHandle, newHistory->period == srcDataBlock->period );

      nbPriceBarCopied += nbElementToCopy;

      /* Parano test: we want to make sure we will not write out of bound
       *              on the allocated newHistory fields.
       */
      if( nbPriceBarCopied > nbPriceBar )
      {
         TA_FATAL( libHandle, NULL, nbPriceBarCopied, nbPriceBar );
      }

      /* Parano test: we want to make sure we will not copy out of bound
       *              from the source.
       */
      if( (nbElementToCopy+srcIndexForCopy) > srcDataBlock->nbBars )
      {
         TA_FATAL( libHandle, NULL, nbElementToCopy, srcIndexForCopy );
      }

      #define COPY_PRICE_DATA(var,typ) \
      { \
         if (var) \
         { \
           TA_ASSERT(libHandle,srcDataBlock->var); \
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
      TA_FATAL( libHandle, NULL, nbPriceBarCopied, nbPriceBar );
   }

   newHistory->nbBars = nbPriceBarCopied;

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode buildListMergeOp( TA_Libc *libHandle,
                                    TA_BuilderSupport *builderSupport )
{
   TA_PROLOG;
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

   TA_TRACE_BEGIN( libHandle, buildListMergeOp );

   TA_ASSERT( libHandle, builderSupport != NULL );

   listOfSupportForDataSource = builderSupport->listOfSupportForDataSource;
   TA_ASSERT( libHandle, listOfSupportForDataSource != NULL );

   retCode = TA_PeriodNormalize( libHandle, builderSupport );
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
   TA_ASSERT( libHandle, curSupport != NULL );

   while( curSupport )
   {
      TA_ASSERT( libHandle, curSupport->listOfDataBlock != NULL );

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
         TA_ASSERT( libHandle, tmpDataBlock->timestamp != NULL );
         curSupport->curTimestamp = &tmpDataBlock->timestamp[0];
         curSupport->allDataConsumed = 0;
         curSupport->curIndex = 0;
         curSupport->curLastTimestamp = &tmpDataBlock->timestamp[tmpDataBlock->nbBars-1];
      }

      curSupport = (TA_SupportForDataSource *)TA_ListAccessNext( listOfSupportForDataSource );
   }

   if( nbSupportHavingData < 1 )
   {
      TA_TRACE_RETURN( TA_UNKNOWN_ERR  ); /* No data to merge!?!? */
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
      TA_ASSERT( libHandle, curSupport != NULL );

      /* Immediatly bypass data sources with no data left. */
      while( curSupport && (curSupport->allDataConsumed == 1) )
         curSupport = (TA_SupportForDataSource *)TA_ListIterNext( &iterSupport );

      /* Parano test: because nbSupportHavingData > 0, we should have at least
       *              one curSupport with some data!
       */
      TA_ASSERT( libHandle, curSupport != NULL );

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
      TA_ASSERT( libHandle, curSupport != NULL );
      TA_ASSERT( libHandle, oldestTimestamp != NULL );

      /* Create the TA_MergeOp. */
      mergeOp = (TA_MergeOp *)TA_Malloc( libHandle, sizeof( TA_MergeOp ) );
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
            TA_ASSERT( libHandle, !((oldestTimestamp == NULL) && (again == 1)) );
         }
      }

      TA_ASSERT( libHandle, nbDataToBeCopied > 0 );

      /* -3- Add the TA_MergeOp to the list. */
      TA_ASSERT( libHandle, builderSupport->listOfMergeOp != NULL );
      retCode = TA_ListAddTail( builderSupport->listOfMergeOp, mergeOp );
      if( retCode != TA_SUCCESS )
      {
         TA_Free( libHandle, mergeOp );
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

static TA_History *allocEmptyHistory( TA_Libc *libHandle, TA_UDBasePriv *privUDB, TA_Period period )
{
   TA_History *newHistory;
   TA_HistoryHiddenData *hiddenData;

   /* Alloc the TA_History for the user. */
   newHistory = (TA_History *)TA_Malloc( libHandle, sizeof( TA_History ) );

   if( !newHistory )
      return NULL;

   memset( newHistory, 0, sizeof( TA_History ) );
   newHistory->period = period;

   /* Alloc and initialize the hidden data. */
   hiddenData = (TA_HistoryHiddenData *)TA_Malloc( libHandle, sizeof( TA_HistoryHiddenData ) );
   if( !hiddenData )
   {
      TA_Free( libHandle, newHistory );
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
