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
 *      Public interface for a TA_Instrument
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
/* None */

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_RetCode TA_InstrumentInit ( TA_Instrument *instrument,
                               const char *category,
                               const char *symbol )
{
   if( !instrument )
      return TA_BAD_PARAM;
   
   /* Trap cases where the string is NULL */
   if( category && category[0] == '\0' )
      category = NULL;

   if( symbol && symbol[0] == '\0' )
      symbol = NULL;

   /* Remember with a flag the type of key. */
   if( category )
   {
      if( symbol )
         instrument->flags = TA_INSTRUMENT_USE_CATSYMSTRING;
      else
         instrument->flags = TA_INSTRUMENT_USE_CATSTRING;
   }
   else if( symbol )
      instrument->flags = TA_INSTRUMENT_USE_SYMSTRING;
   else
      return TA_BAD_PARAM;

   instrument->key.catSym.catString = category;
   instrument->key.catSym.symString = symbol;

   return TA_SUCCESS;
}

TA_RetCode TA_InstrumentInitWithUserKey ( TA_Instrument *instrument,
                                          unsigned int uniqueUserDefinedKey )
{
   if( !instrument )
      return TA_BAD_PARAM;

   instrument->flags = TA_INSTRUMENT_USE_USERKEY;
   instrument->key.userKey = uniqueUserDefinedKey;

   return TA_SUCCESS;
}

TA_RetCode TA_InstrumentAttachData( TA_Instrument      *instrument,
                                    unsigned int        nbPrice,
                                    const TA_Timestamp *timestamp,
                                    const TA_Real      *lowPrice,
                                    const TA_Real      *price,
                                    const TA_Real      *highPrice )
{
   /* All the parameters are mandatory. */
   if( !instrument || !timestamp || !price || (nbPrice == 0) )
      return TA_BAD_PARAM;

   /* Sanity check of the first and last timestamp. */
   if( !TA_TimestampValidate( &timestamp[0] ) ||
       !TA_TimestampValidate( &timestamp[nbPrice-1] ) )
      return TA_BAD_PARAM;

   /* Make sure that this instrument was initialized. */
   if( !(instrument->flags & (TA_INSTRUMENT_USE_USERKEY|TA_INSTRUMENT_USE_CATSYMSTRING)) )
      return TA_BAD_PARAM;

   instrument->flags |= TA_INSTRUMENT_USE_USERDATA;
   instrument->ds.userData.nbPrice   = nbPrice;
   instrument->ds.userData.price     = price;
   instrument->ds.userData.timestamp = timestamp;

   if( lowPrice )
      instrument->ds.userData.lowPrice = lowPrice;
   else
      instrument->ds.userData.lowPrice = price;

   if( highPrice )
      instrument->ds.userData.highPrice = highPrice;
   else
      instrument->ds.userData.highPrice = price;

   return TA_SUCCESS;
}

/* Attach a UDBase to retreive data for this instrument. */
TA_RetCode TA_InstrumentAttachUDBase( TA_Instrument   *instrument,
                                      TA_UDBase *udBase )
{
   /* All the parameters are mandatory. */
   if( !instrument || !udBase )
      return TA_BAD_PARAM;

   instrument->flags |= TA_INSTRUMENT_USE_UDBASE;
   instrument->ds.udBase = udBase;

   return TA_SUCCESS;
}

/**** Local functions definitions.     ****/
/* None */
