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
 *  031202 MF   First version.
 *
 */

/* Description:
 *      Performance Measurement interface for the TA_TradeLog
 */

/**** Headers ****/
#include <string.h>
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
/* None */

/**** Local functions declarations.    ****/
static TA_RetCode TA_PMDelTradeLog( TA_TradeLogPriv *tradeLogToDel );
static void freeTradeDictEntry( void *toBeFreed );
                                


/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/

TA_RetCode TA_TradeLogAlloc( TA_TradeLog **allocatedTradeLog )
{
   TA_TradeLog     *tradeLog;
   TA_TradeLogPriv *tradeLogPriv;
   TA_RetCode retCode;

   if( allocatedTradeLog )
      *allocatedTradeLog = NULL;
   else
      return TA_BAD_PARAM;

   /* Allocate the public and private structure. */
   tradeLog = TA_Malloc( sizeof( TA_TradeLog ) + sizeof( TA_TradeLogPriv ) );
   if( !tradeLog )
      return TA_ALLOC_ERR;
   memset( tradeLog, 0, sizeof( TA_TradeLog ) + sizeof( TA_TradeLogPriv ) );
   tradeLogPriv = (TA_TradeLogPriv *)(((char *)tradeLog)+sizeof(TA_TradeLog));
   tradeLogPriv->magicNb   = TA_TRADELOGPRIV_MAGIC_NB;
   tradeLog->hiddenData    = tradeLogPriv;

   /* TA_TradeLogFree can be safely called from this point. */

   tradeLogPriv->tradeDictCAT = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, freeTradeDictEntry );
   if( !tradeLogPriv->tradeDictCAT )
   {
      TA_TradeLogFree( tradeLog );
      return TA_ALLOC_ERR;
   }

   tradeLogPriv->tradeDictSYM = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, freeTradeDictEntry );
   if( !tradeLogPriv->tradeDictSYM )
   {
      TA_TradeLogFree( tradeLog );
      return TA_ALLOC_ERR;
   }

   tradeLogPriv->tradeDictCATSYM = TA_DictAlloc( TA_DICT_KEY_TWO_STRING, freeTradeDictEntry );
   if( !tradeLogPriv->tradeDictCATSYM )
   {
      TA_TradeLogFree( tradeLog );
      return TA_ALLOC_ERR;
   }

   tradeLogPriv->tradeDictUserKey = TA_DictAlloc( TA_DICT_KEY_INTEGER, freeTradeDictEntry );
   if( !tradeLogPriv->tradeDictUserKey )
   {
      TA_TradeLogFree( tradeLog );
      return TA_ALLOC_ERR;
   }

   retCode = TA_AllocatorForDataLog_Init( &tradeLogPriv->allocator );
   if( retCode != TA_SUCCESS )
   {
      TA_TradeLogFree( tradeLog );
      return retCode;
   }

   /* Initialize the caller pointer on the allocated TA_TradeLog */
   *allocatedTradeLog = tradeLog;

   return TA_SUCCESS;
}

TA_RetCode TA_TradeLogFree( TA_TradeLog *toBeFreed )
{
   TA_TradeLogPriv *tradeLogPriv;

   if( toBeFreed )
   {
      /* Make sure this is a valid object. */
      tradeLogPriv = (TA_TradeLogPriv *)toBeFreed->hiddenData;
      if( tradeLogPriv->magicNb != TA_TRADELOGPRIV_MAGIC_NB )
         return TA_BAD_OBJECT;

      /* If it currently belong to a TA_PM, remove it. */       
      if( tradeLogPriv->parentPMPriv )
         TA_PMDelTradeLog( tradeLogPriv );

      /* Free the private members */
      if( tradeLogPriv->tradeDictCAT )
         TA_DictFree(tradeLogPriv->tradeDictCAT);
      if( tradeLogPriv->tradeDictSYM )
         TA_DictFree(tradeLogPriv->tradeDictSYM);
      if( tradeLogPriv->tradeDictCATSYM )
         TA_DictFree(tradeLogPriv->tradeDictCATSYM);
      if( tradeLogPriv->tradeDictUserKey )
         TA_DictFree(tradeLogPriv->tradeDictUserKey);

      TA_AllocatorForDataLog_FreeAll( &tradeLogPriv->allocator );

      /* Free the public portion. */
      TA_Free( toBeFreed );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_TradeLogAdd( TA_TradeLog    *tradeLog,
                           const TA_Transaction *newTransaction )
{
   TA_TradeLogPriv *tradeLogPriv;
   const TA_Instrument *id;
   unsigned int flags;
   TA_Dict *theDict;
   TA_DataLog *dataLog;
   TA_TradeDictEntry *dictEntry;
   TA_StringCache *stringCache;
   TA_String *catString;
   TA_String *symString;
   TA_RetCode retCode;
   int quantity, entryTradeQuantity;
   TA_List *entryListToUse;
   TA_DataLog *entryTradeLog;
   TA_PMPriv *pmPriv;

   TA_Real tempReal;

   retCode = TA_INTERNAL_ERROR(120);

   /* This function will transform the TA_Transaction into
    * an "entry" or multiple "trades" (because an exit can
    * be translated into multiple trade if there was multiple
    * entry point).
    */

   if( !tradeLog || !newTransaction )
      return TA_BAD_PARAM;

   /* Check that the TA_Transaction makes sense. */
   if( (newTransaction->price <= 0.0)   ||
       (newTransaction->quantity <= 0) ||
       (TA_TimestampValidate(&newTransaction->timestamp)!= TA_SUCCESS) ||
       (newTransaction->type >= TA_NB_TRADE_TYPE))
      return TA_BAD_PARAM;

   /* Check that the id makes sense. */
   id = newTransaction->id;
   if( !id )
      return TA_INSTRUMENT_ID_BAD;

   flags = id->flags;
   if( !(flags & (TA_INSTRUMENT_USE_USERKEY|
                  TA_INSTRUMENT_USE_CATSTRING|
                  TA_INSTRUMENT_USE_SYMSTRING|
                  TA_INSTRUMENT_USE_CATSYMSTRING)) )
   {
      return TA_INSTRUMENT_ID_BAD;
   }

   /* Get access to the hidden data of the TA_TradeLog. */
   tradeLogPriv = (TA_TradeLogPriv *)tradeLog->hiddenData;

   /* Make sure this is a valid object. */
   if( tradeLogPriv->magicNb != TA_TRADELOGPRIV_MAGIC_NB )
      return TA_BAD_OBJECT;

   /* Find the TA_TradeDictEntry corresponding to
    * the TA_Instrument.
    *
    * Use the dictionary corresponding to the type of
    * key of the TA_Instrument.
    */
   if( flags & TA_INSTRUMENT_USE_CATSTRING )
   {
      if( flags & TA_INSTRUMENT_USE_SYMSTRING )
      {
         theDict = tradeLogPriv->tradeDictCATSYM;
         dictEntry = TA_DictGetValue_S2( theDict, id->key.catSym.catString, id->key.catSym.symString );
      }
      else
      {
         theDict = tradeLogPriv->tradeDictCAT;
         dictEntry = TA_DictGetValue_S( theDict, id->key.catSym.catString );
      }
   }
   else if( flags & TA_INSTRUMENT_USE_SYMSTRING )
   {
      theDict = tradeLogPriv->tradeDictCAT;
      dictEntry = TA_DictGetValue_S( theDict, id->key.catSym.symString );
   }
   else if( flags & TA_INSTRUMENT_USE_USERKEY )
   {
      theDict = tradeLogPriv->tradeDictUserKey;
      dictEntry = TA_DictGetValue_I( theDict, id->key.userKey );
   }
   else
      return TA_INSTRUMENT_ID_BAD;
   
   if( !dictEntry )
   {
      /* The TA_TradeDictEntry was not found, create it! */
      dictEntry = TA_Malloc( sizeof( TA_TradeDictEntry ) );
      if( !dictEntry )
         return TA_ALLOC_ERR;

      dictEntry->id = id;
      TA_ListInit(  &dictEntry->shortEntryPrivList );
      TA_ListInit(  &dictEntry->longEntryPrivList );

      /* Add the dictEntry to the corresponding dictionary. */
      stringCache = TA_GetGlobalStringCache();

      if( flags & TA_INSTRUMENT_USE_CATSTRING )
      {         
         catString = TA_StringAlloc( stringCache, id->key.catSym.catString );
         if( !catString )
         {
            TA_Free(  dictEntry );
            return TA_ALLOC_ERR;
         }

         if( flags & TA_INSTRUMENT_USE_SYMSTRING )
         {
            symString = TA_StringAlloc( stringCache, id->key.catSym.symString );
            if( !symString )
            {
               TA_Free(  dictEntry );
               TA_StringFree( stringCache, catString );
               return TA_ALLOC_ERR;
            }
            retCode = TA_DictAddPair_S2( theDict, catString, symString, dictEntry );
            TA_StringFree( stringCache, symString );
         }
         else
            retCode = TA_DictAddPair_S( theDict, catString, dictEntry );

        TA_StringFree( stringCache, catString );
      }
      else if( flags & TA_INSTRUMENT_USE_SYMSTRING )
      {
         symString = TA_StringAlloc( stringCache, id->key.catSym.symString );
         if( !symString )
         {
           TA_Free(  dictEntry );
           return TA_ALLOC_ERR;
         }

         retCode = TA_DictAddPair_S( theDict, symString, dictEntry );
         TA_StringFree( stringCache, symString );
      }
      else if( flags & TA_INSTRUMENT_USE_USERKEY )
         retCode = TA_DictAddPair_I( theDict, id->key.userKey, dictEntry );

      /* Check the retCode of the TA_DictAddXXXXX function. */
      if( retCode != TA_SUCCESS )
      {
         TA_Free(  dictEntry );
         return TA_ALLOC_ERR;
      }
   }

   /* Identify the approriate list of entry. */
   switch( newTransaction->type )
   {
   case TA_LONG_ENTRY:
   case TA_LONG_EXIT:
      entryListToUse = &dictEntry->longEntryPrivList;
      break;
   case TA_SHORT_ENTRY:
   case TA_SHORT_EXIT:
      entryListToUse = &dictEntry->shortEntryPrivList;
      break;
   default:
      return TA_BAD_PARAM;
   }

   /* The sign of the quantity indicates if the
    * data log is a completed trade (+) or an
    * entry (-).
    */
   switch( newTransaction->type )
   {
   case TA_LONG_ENTRY:
   case TA_SHORT_ENTRY:
      /* Allocate a data log and add it to the list. */
      dataLog = TA_AllocatorForDataLog_Alloc( &tradeLogPriv->allocator );
      if( !dataLog )
         return TA_ALLOC_ERR;
      dataLog->quantity  = -(newTransaction->quantity);
      dataLog->entryPrice = newTransaction->price;
      TA_TimestampCopy( &dataLog->entryTimestamp,
                        &newTransaction->timestamp );
      TA_ListNodeAddTail( entryListToUse, &dataLog->u.entry.node, dataLog );
      break;

   case TA_LONG_EXIT:
   case TA_SHORT_EXIT:
      /* Invalidate cached calculation of this trade log. */
      tradeLogPriv->flags &= ~TA_PMVALUECACHE_CALCULATED;

      /* Invalidate cached calculation of the parent TA_PM (if applicable) */
      pmPriv = tradeLogPriv->parentPMPriv;
      if( pmPriv )
         pmPriv->flags &= ~TA_PMVALUECACHE_CALCULATED;

      /* Transform this transaction into one or 
       * multiple trade(s).
       */
      entryTradeLog = TA_ListRemoveHead( entryListToUse );
      if( !entryTradeLog )
          return TA_ENTRY_TRANSACTION_MISSING;

      quantity = newTransaction->quantity;
      while( quantity )
      {
         entryTradeQuantity = -entryTradeLog->quantity;
         if( entryTradeQuantity == quantity )
         {
            /* This entry have exactly the right amount of 
             * position for what needs to be closed.
             * Just transform the entry into a trade.
             */
             entryTradeLog->quantity = quantity;
             entryTradeLog->u.trade.id = id;
             TA_TimestampCopy( &entryTradeLog->u.trade.exitTimestamp,
                               &newTransaction->timestamp );
             /* Calculate the profit and make the entryPrice
              * negative if this is a short trade.
              * Both are multiplied by the quantity being
              * traded.
              */
             tempReal = entryTradeLog->entryPrice;
             if( newTransaction->type == TA_LONG_EXIT )
             {
                entryTradeLog->u.trade.profit = (newTransaction->price-tempReal)*quantity;
             }
             else
             {
                entryTradeLog->u.trade.profit = (tempReal-newTransaction->price)*quantity;
                tempReal = -tempReal;
             }
             entryTradeLog->entryPrice = tempReal * quantity;
             return TA_SUCCESS; /* Done! */
         }
         else if( entryTradeQuantity < quantity )
         {
            /* This entry have less than the amount of 
             * position that needs to be closed.
             * Just transform the entry into a trade
             * and move to the next entry.
             */
             entryTradeLog->quantity = entryTradeQuantity;
             quantity -= entryTradeQuantity;
             entryTradeLog->u.trade.id = id;
             TA_TimestampCopy( &entryTradeLog->u.trade.exitTimestamp,
                               &newTransaction->timestamp );
             /* Calculate the profit and make the entryPrice
              * negative if this is a short trade.
              * Both are multiplied by the quantity being
              * traded.
              */
             tempReal = entryTradeLog->entryPrice;
             if( newTransaction->type == TA_LONG_EXIT )
                entryTradeLog->u.trade.profit = (newTransaction->price-tempReal)*entryTradeQuantity;
             else
             {
                entryTradeLog->u.trade.profit = (tempReal-newTransaction->price)*entryTradeQuantity;
                tempReal = -tempReal;
             }
             entryTradeLog->entryPrice = tempReal * entryTradeQuantity;

             /* Move to the next entry. If none available, that means there
              * was more "exit" than "entry" and this is considered an
              * error.
              */
             entryTradeLog = TA_ListRemoveHead( entryListToUse );
             if( !entryTradeLog )
                return TA_ENTRY_TRANSACTION_MISSING;
         }
         else
         {
            /* This entry have more position than what the
             * exit requires, so the entry must be preserved.
             * Consequently, a new tradeLog must be allocated.
             */
            dataLog = TA_AllocatorForDataLog_Alloc( &tradeLogPriv->allocator );
            if( !dataLog )
               return TA_ALLOC_ERR;

            TA_TimestampCopy( &dataLog->entryTimestamp,
                              &entryTradeLog->entryTimestamp );
            TA_TimestampCopy( &dataLog->u.trade.exitTimestamp,
                              &newTransaction->timestamp );
            dataLog->quantity = quantity;
            tempReal = entryTradeLog->entryPrice;
            if( newTransaction->type == TA_LONG_EXIT )
                dataLog->u.trade.profit = (newTransaction->price-tempReal)*quantity;
            else
            {
                dataLog->u.trade.profit = (tempReal-newTransaction->price)*quantity;
                tempReal = -tempReal;
            }
            dataLog->entryPrice = tempReal*quantity;
            dataLog->u.trade.id = id;

            /* Adjust the entry and put it back for being process
             * again later.
             */
            entryTradeLog->quantity += quantity;
            TA_ListNodeAddHead( entryListToUse, &entryTradeLog->u.entry.node, entryTradeLog );            
            return TA_SUCCESS; /* Done! */
         }
      }
      break;
   default:
      return TA_INTERNAL_ERROR(121);
      break;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_PMAlloc( const TA_Timestamp  *startDate,
                       const TA_Timestamp  *endDate,
                       TA_Real              initialCapital,
                       TA_PM              **allocatedPM )
{
   TA_PM     *pm;
   TA_PMPriv *pmPriv;
   unsigned int delta;
   TA_RetCode retCode;

   /* Check all the parameters. */
   if( !allocatedPM )
      return TA_BAD_PARAM;
   *allocatedPM = NULL;

   if( !startDate || !endDate )
      return TA_BAD_PARAM;

   if( TA_TimestampValidate( startDate ) )
      return TA_BAD_START_DATE;

   if( TA_TimestampValidate( endDate ) )
      return TA_BAD_END_DATE;

   /* To keep things simple, it is assumed that
    * the requested date range contains at least
    * one weekday.
    */
   retCode = TA_TimestampDeltaWeekday( startDate, endDate, &delta );
   if( retCode != TA_SUCCESS )
      return retCode;
   if( delta <= 0 )
      return TA_NO_WEEKDAY_IN_DATE_RANGE;

   /* Allocate the public and private structure. */
   pm = TA_Malloc( sizeof( TA_PM ) + sizeof( TA_PMPriv ) );
   if( !pm )
   {
      *allocatedPM = NULL;
      return TA_ALLOC_ERR;
   }

   memset( pm, 0, sizeof( TA_PM ) + sizeof( TA_PMPriv ) );
   pmPriv = (TA_PMPriv *)(((char *)pm)+sizeof(TA_PM));
   pmPriv->magicNb        = TA_PMPRIV_MAGIC_NB;
   pmPriv->initialCapital = initialCapital;
   pm->hiddenData         = pmPriv;

   /* TA_PMFree can be safely called from this point. */

   TA_ListInit(  &pmPriv->tradeLogList );
   if( endDate )
      TA_TimestampCopy( &pmPriv->endDate, endDate );
            
   if( startDate )
      TA_TimestampCopy( &pmPriv->startDate, startDate );

   /* Success, return the allocated data to the caller. */
   *allocatedPM = pm;

   return TA_SUCCESS;
}

TA_RetCode TA_PMFree( TA_PM *toBeFreed )
{
   TA_PMPriv *pmPriv;
   TA_TradeLogPriv *tradeLogPriv;

   if( toBeFreed )
   {      
      /* Make sure this is a valid object */
      pmPriv = (TA_PMPriv *)toBeFreed->hiddenData; 
      if( pmPriv->magicNb != TA_PMPRIV_MAGIC_NB )
         return TA_BAD_OBJECT;

      /* Clearly mark this object as being unusable. */
      pmPriv->magicNb = 0;

      /* Indicate to all TA_TradeLog that they do
       * no belong to this TA_PM anymore.
       */
      tradeLogPriv = TA_ListAccessHead( &pmPriv->tradeLogList );
      while( tradeLogPriv )
      {
         tradeLogPriv->parentPMPriv = NULL;
         tradeLogPriv = TA_ListAccessNext( &pmPriv->tradeLogList );
      }
      TA_ListFree( &pmPriv->tradeLogList );   

      /* Free all the cached arrays. */
      FREE_IF_NOT_NULL( pmPriv->equity );
      FREE_IF_NOT_NULL( pmPriv->arrayTimestamp );

      FREE_IF_NOT_NULL( pmPriv->shortArrayCache.investment );
      FREE_IF_NOT_NULL( pmPriv->shortArrayCache.profit );

      FREE_IF_NOT_NULL( pmPriv->longArrayCache.investment );
      FREE_IF_NOT_NULL( pmPriv->longArrayCache.profit );

      FREE_IF_NOT_NULL( pmPriv->totalArrayCache.investment );
      FREE_IF_NOT_NULL( pmPriv->totalArrayCache.profit );

      /* Last thing that must be freed... */
      TA_Free(  toBeFreed );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_PMAddTradeLog( TA_PM *pm, TA_TradeLog *tradeLogToAdd )
{
   TA_TradeLogPriv *tradeLogPriv;
   TA_PMPriv *pmPriv;
   TA_RetCode retCode;

   if( !pm || !tradeLogToAdd )
      return TA_BAD_PARAM;

   /* Make sure this TA_PM is a valid object */
   pmPriv = (TA_PMPriv *)pm->hiddenData; 
   if( pmPriv->magicNb != TA_PMPRIV_MAGIC_NB )
      return TA_BAD_OBJECT;

   /* Make sure this TA_TradeLog is a valid object. */
   tradeLogPriv = (TA_TradeLogPriv *)tradeLogToAdd->hiddenData;
   if( tradeLogPriv->magicNb != TA_TRADELOGPRIV_MAGIC_NB )
      return TA_BAD_OBJECT;
   
   /* Make sure it is not already added to another TA_PM */
   if( tradeLogPriv->parentPMPriv )
      return TA_TRADELOG_ALREADY_ADDED;

   tradeLogPriv->parentPMPriv = pmPriv;

   /* Just add it to the list. */
   retCode = TA_ListAddTail( &pmPriv->tradeLogList, tradeLogPriv );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Invalidate cached calculation. */
   pmPriv->flags &= ~TA_PMVALUECACHE_CALCULATED;
   tradeLogPriv->flags &= ~TA_PMVALUECACHE_CALCULATED;

   return TA_SUCCESS;
}


/**** Local functions definitions.     ****/
static void freeTradeDictEntry( void *toBeFreed )
{
   if( !toBeFreed )
      return;

   TA_Free(  (TA_TradeDictEntry *)toBeFreed );
}

static TA_RetCode TA_PMDelTradeLog( TA_TradeLogPriv *tradeLogToDel )
{
   TA_PMPriv *pmPriv;
   TA_RetCode retCode;

   if( !tradeLogToDel )
      return TA_BAD_PARAM;

   pmPriv = tradeLogToDel->parentPMPriv;
   if( pmPriv )
   {
      /* Make sure this TA_PM is a valid object */
      if( pmPriv->magicNb != TA_PMPRIV_MAGIC_NB )
         return TA_BAD_OBJECT;

      /* Make sure this TA_TradeLog is a valid object. */
      if( tradeLogToDel->magicNb != TA_TRADELOGPRIV_MAGIC_NB )
         return TA_BAD_OBJECT;
   
      /* Invalidate cached calculation. */
      pmPriv->flags &= ~TA_PMVALUECACHE_CALCULATED;

      /* Just remove it from the list. */
      retCode = TA_ListRemoveEntry( &pmPriv->tradeLogList, tradeLogToDel );
      if( retCode != TA_SUCCESS )
         return retCode;
   }

   return TA_SUCCESS;
}
