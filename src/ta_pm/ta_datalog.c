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
 *       Provide an allocator and some utility functions for 
 *       the TA_DataLog
 *       
 */

/**** Headers ****/

#include <string.h>
#include "ta_pm_priv.h"
#include "ta_memory.h"

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

TA_RetCode TA_AllocatorForDataLog_Init( TA_Libc *libHandle,
                                        TA_AllocatorForDataLog *allocator )
{
   memset( allocator, 0, sizeof(TA_AllocatorForDataLog) );
   TA_ListInit( libHandle, &allocator->listOfDataLogBlock );
   allocator->libHandle = libHandle;
   return TA_SUCCESS;
}

/* Alllocate a TA_Transaction. Will return NULL if fail. */
TA_DataLog *TA_AllocatorForDataLog_Alloc( TA_AllocatorForDataLog *allocator )
{
   TA_DataLogBlock *block;
   TA_RetCode retCode;
   TA_DataLog *tradePtr;

   if( allocator->nbFree == 0 )
   {
      block = TA_Malloc( allocator->libHandle, sizeof(TA_DataLogBlock) );
      if( !block )
         return (TA_DataLog *)NULL;
      retCode = TA_ListNodeAddTail( &allocator->listOfDataLogBlock,
                                    &block->node, block );                                    
      if( retCode != TA_SUCCESS )
      {
         TA_Free( allocator->libHandle, block );
         return (TA_DataLog *)NULL;
      }

      allocator->nbFree = TA_TRADE_BLOCK_SIZE-1;
      tradePtr = &block->array[0];
      allocator->nextAvailableTrade = tradePtr+1;
      return tradePtr;
   }

   tradePtr = allocator->nextAvailableTrade++;
   allocator->nbFree--;
   return tradePtr;
}

/* Must be called to free all ressources related to a TA_AllocatorForDataLog.
 * Should be called only if TA_AllocatorForDataLog_Init did return TA_SUCCESS.
 */
TA_RetCode TA_AllocatorForDataLog_FreeAll( TA_AllocatorForDataLog *allocator )
{
   TA_DataLogBlock *block;
   TA_List *list;
   TA_Libc *libHandle;

   if( allocator )
   {
      libHandle = allocator->libHandle;
      if( libHandle )
      {
         list = &allocator->listOfDataLogBlock;
         block = TA_ListRemoveTail( list );
         while( block )
         {
            TA_Free( libHandle, block );
            block = TA_ListRemoveTail( list );
         }
      }
   }
   
   return TA_SUCCESS;
}
                                             

/**** Local functions definitions.     ****/
/* None */

