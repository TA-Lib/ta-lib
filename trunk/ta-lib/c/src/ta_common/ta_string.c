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
 *  ML       Matt Lindblom
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110199 MF   First version.
 *  061503 ML   Fix#731857 to stringAllocNInternal (bad memory access).
 *  032104 MF   Add TA_StringAlloc_ULong & TA_StringAlloc_Path
 */

/* Description:
 *    This is a simple reference counting mechanism for constant string.
 *
 *    The implementation is done for assuring both speed/memory efficiency
 *    in the context of the TA-LIB.
 *
 *    In particular, the TA-LIB is sometime using a large amount of repeating
 *    small strings when building the symbol index (like a constantly recurring
 *    wildcard value in the middle of a path). For this reason, a small cache
 *    allows to re-use recently allocated strings (hash table contains a copy
 *    of the last 256 allocated strings).
 *
 *    Allocation of string can be done only from a particular cache.
 *
 *    This module is multithread safe.
 */

/**** Headers ****/
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "ta_string.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_system.h"
#include "ta_global.h"


/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#define NB_CACHE_ENTRY 256
typedef struct
{
   #if !defined( TA_SINGLE_THREAD )
   TA_Sema    sema;
   #endif
   TA_String *cache[NB_CACHE_ENTRY];
} TA_StringCachePriv;

/* Different way to handle some string abstraction like upper/lower case or
 * the separator in a path.
 */
typedef enum { NO_CASE, UPPER_CASE, PATH } TA_CharCase;

/**** Local functions declarations.    ****/
static unsigned int calcHash( const char *string, TA_CharCase caseType );
static void stringFreeInternal( TA_String *string );
TA_String *stringAllocNInternal( TA_StringCache *stringCache,
                                 const char *string,
                                 unsigned int maxNbChar,
                                 TA_CharCase caseType );

static TA_String *stringDupInternal( TA_String *string );

static TA_String *stringAllocInternal( const char *string,
                                       unsigned int newStringLength,
                                       TA_CharCase caseType );

static TA_String *stringValueAllocInternal( const char *string,
                                            unsigned int value );

static TA_String *valueAllocInternal( unsigned long value );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/


/* Initialize the cache mechanism. */
TA_RetCode TA_StringCacheAlloc( TA_StringCache **newStringCache )
{
   TA_StringCachePriv *stringCachePriv;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( !newStringCache )
      return TA_BAD_PARAM;

   *newStringCache = NULL;

   stringCachePriv = (TA_StringCachePriv *)TA_Malloc( sizeof( TA_StringCachePriv ) );

   if( !stringCachePriv )
      return TA_ALLOC_ERR;

   memset( stringCachePriv, 0, sizeof( TA_StringCachePriv ) );

   #if !defined( TA_SINGLE_THREAD )
   retCode = TA_SemaInit( &stringCachePriv->sema, 1 );
   if( retCode != TA_SUCCESS )
   {
      TA_Free(  stringCachePriv );
      return retCode;
   }
   #endif

   /* Success, return the cache to the caller. */
   *newStringCache = (TA_StringCache *)stringCachePriv;

   return TA_SUCCESS;
}

TA_RetCode TA_StringCacheFree( TA_StringCache *stringCacheToFree )
{
   TA_StringCachePriv *stringCachePriv;
   unsigned int i;

   if( !stringCacheToFree )
      return TA_BAD_PARAM;

   stringCachePriv = (TA_StringCachePriv *)stringCacheToFree;

   for( i=0; i < NB_CACHE_ENTRY; i++ )
   {
      if( stringCachePriv->cache[i] != NULL )
         TA_StringFree( stringCacheToFree,
                        (TA_String *)stringCachePriv->cache[i] );
   }

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaDestroy( &stringCachePriv->sema );
   #endif

   TA_Free(  stringCachePriv );

   return TA_SUCCESS;
}

/* Allocate dynamically a copy of 'string'. */
TA_String *TA_StringAlloc( TA_StringCache *stringCache, const char *string )
{
   return stringAllocNInternal( stringCache, string, 0, NO_CASE );
}

TA_String *TA_StringAlloc_UC( TA_StringCache *stringCache, const char *string )
{
   return stringAllocNInternal( stringCache, string, 0, UPPER_CASE );
}

TA_String *TA_StringAllocN( TA_StringCache *stringCache, const char *string, unsigned int maxNbChar )
{
   return stringAllocNInternal( stringCache, string, maxNbChar, NO_CASE );
}

TA_String *TA_StringAllocN_UC( TA_StringCache *stringCache, const char *string, unsigned int maxNbChar )
{
   return stringAllocNInternal( stringCache, string, maxNbChar, UPPER_CASE );
}

TA_String *TA_StringAlloc_Path( TA_StringCache *stringCache, const char *string )
{
   return stringAllocNInternal( stringCache, string, 0, PATH );
}

TA_String *TA_StringValueAlloc( TA_StringCache *stringCache,
                                const char *string,
                                unsigned int value )
{
   TA_StringCachePriv *stringCachePriv;
   TA_String *retString;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( stringCache == NULL )
      return NULL;

   stringCachePriv = (TA_StringCachePriv *)stringCache;

   #if !defined( TA_SINGLE_THREAD )
      /* Get the semaphore for this cache. */
      retCode = TA_SemaWait( &stringCachePriv->sema );

      if( retCode != TA_SUCCESS )
         return NULL;
   #endif

   /* The "StringValue" strings are not stored in the cache.
    * (Because they are known of not being copied often in the context
    *  of the TA-LIB).
    */
   retString = stringValueAllocInternal( string, value );

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &stringCachePriv->sema );
   #endif

   return retString; 
}

TA_String *TA_StringAlloc_ULong( TA_StringCache *stringCache, unsigned long value )
{
   TA_StringCachePriv *stringCachePriv;
   TA_String *retString;
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( stringCache == NULL )
      return NULL;

   stringCachePriv = (TA_StringCachePriv *)stringCache;

   #if !defined( TA_SINGLE_THREAD )
      /* Get the semaphore for this cache. */
      retCode = TA_SemaWait( &stringCachePriv->sema );

      if( retCode != TA_SUCCESS )
         return NULL;
   #endif

   /* The "value" strings are not stored in the cache.
    * (Because they are known of not being copied often in
    *  the context of TA-LIB).
    */
   retString = valueAllocInternal( value );

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &stringCachePriv->sema );
   #endif

   return retString; 
}


/* Allocate dyamically a copy of string.
 * Eliminate also all whitespace in the copy.
 */
TA_String *TA_StringAllocTrim( TA_StringCache *stringCache,
                               const char *string )
{
   char *str;
   char *ptrCharTmp1;
   const char *ptrCharTmp2;
   unsigned int trimmedSize;
   unsigned int nonTrimmedSize;
   TA_StringCachePriv *stringCachePriv;

   if( stringCache == NULL )
      return NULL;

   stringCachePriv = (TA_StringCachePriv *)stringCache;

   if( !string )
      return NULL;

   /* Evaluate the size and see if any trimming is needed. */
   trimmedSize = 0;
   nonTrimmedSize = 0;
   ptrCharTmp2 = string;
   while( *ptrCharTmp2++ != '\0' )
   {
      if( !isspace(*ptrCharTmp2) )
         trimmedSize++;
      nonTrimmedSize++;
   }

   if( trimmedSize == nonTrimmedSize )
      return TA_StringAlloc( stringCache, string );

   trimmedSize = (trimmedSize + 1)*sizeof( unsigned char );

   str = (char *)TA_Malloc( trimmedSize );

   if( str != NULL )
   {
      /* Eliminate all whitespaces (not speed optimize...) */
      ptrCharTmp1 = &str[0];
      ptrCharTmp2 = string;
      while( *ptrCharTmp2 != '\0' )
      {
         if( !isspace(*ptrCharTmp2) )
         {
            *ptrCharTmp1 = *ptrCharTmp2;
            ptrCharTmp1++;
         }
         ptrCharTmp2++;
      }

      *ptrCharTmp1 = '\0';

      return TA_StringAlloc( stringCache, str );
   }

   /* Allocation error. */
   return (TA_String *)NULL;
}


/* Free this copy of the 'string'. */
void TA_StringFree( TA_StringCache *stringCache,
                    TA_String *string )
{
   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif
   TA_StringCachePriv *stringCachePriv;

   if( stringCache == NULL )
      return;

   stringCachePriv = (TA_StringCachePriv *)stringCache;

   if( !string )
      return;

   #if !defined( TA_SINGLE_THREAD )
      /* Get the semaphore for this cache. */
      retCode = TA_SemaWait( &stringCachePriv->sema );
      if( retCode != TA_SUCCESS )
         return;
   #endif

   stringFreeInternal( string );

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &stringCachePriv->sema );      
   #endif
}

/* Make a duplicate of a string. */
TA_String *TA_StringDup( TA_StringCache *stringCache, TA_String *string )
{
   TA_String *str;

   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   TA_StringCachePriv *stringCachePriv;

   if( stringCache == NULL )
      return NULL;

   stringCachePriv = (TA_StringCachePriv *)stringCache;

   if( !string )
      return NULL;

   #if !defined( TA_SINGLE_THREAD )
      /* Get the semaphore for this cache. */
      retCode = TA_SemaWait( &stringCachePriv->sema );
      if( retCode != TA_SUCCESS )
         return NULL;
   #endif

   str = stringDupInternal( string );

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &stringCachePriv->sema );
   #endif

   return str;
}


/**** Local functions definitions.     ****/
static unsigned int calcHash( const char *string, TA_CharCase caseType )
{
   unsigned int i;
   unsigned int retValue = 0;
   unsigned char tmp;

   for( i=0; i < 10; i++ )
   {
      tmp = string[i];

      if( tmp )
      {
         if( caseType == UPPER_CASE )
            tmp = (unsigned char)toupper( tmp );
         else if( caseType == PATH && TA_IsSeparatorChar(tmp) )
            tmp = TA_SeparatorASCII();
         
         retValue += tmp;
      }
      else
         return retValue%NB_CACHE_ENTRY;
   }

   return retValue%NB_CACHE_ENTRY;
}

static void stringFreeInternal( TA_String *string )
{
   char *str = (char *)string;

   if( !string )
      return;

   if( str[0] == 1 )
   {
      /* This was the last reference, delete the complete string. */
      str[0] = 0; /* To trap memory allocation problem... */
      TA_Free(  str );
   }
   else if( str[0] == 0 )
      return; /* Already freed!? Just ignore. */
   else
   {
      /* Decrement the number of reference. */
      str[0]--;
   }
}

static TA_String *stringDupInternal( TA_String *string )
{
   char *str = (char *)string;

   if( !string || (str[0] == 0) )
      return NULL;

   /* We have to make a true copy when we have reach the
    * maximum that could be handled by the instance counter.
    * From a functional point of view, the caller will not notice
    * the difference...
    */
   if( ((unsigned char)str[0]) == 0xFF )
      return stringAllocInternal( &str[1], strlen(&str[1]), NO_CASE );

   /* Increment the number of reference. */
   str[0]++;

   return string;
}

static TA_String *stringAllocInternal( const char *string,
                                       unsigned int newStringLength,
                                       TA_CharCase caseType )
{
   unsigned int size, i;
   char *str;

   /* Not in the cache, or could have reach duplicaton limit.
    * No choice to do an allocation.
    */
   size = ( newStringLength + 2 ) * sizeof( unsigned char );

   str = (char *)TA_Malloc( size );

   if( str != NULL )
   {
      if( caseType == UPPER_CASE )
      {
         for( i=0; i < newStringLength; i++ )
            str[i+1] = (unsigned char)toupper( string[i] );
      }
      else if( caseType == PATH )
      {
         for( i=0; i < newStringLength; i++ )
         {
            if( TA_IsSeparatorChar(string[i]) )
               str[i+1] = TA_SeparatorASCII();
            else
               str[i+1] = string[i];
         }
      }
      else
      {
         for( i=0; i < newStringLength; i++ )
            str[i+1] = string[i];
      }

      str[newStringLength+1] = '\0';
      str[0] = 1;
   }

   return (TA_String *)str;
}

static TA_String *stringValueAllocInternal( const char *string,
                                            unsigned int value )
{
   unsigned int size;
   char *str;

   size = ( strlen(string) + 2 + 8 );
   str = (char *)TA_Malloc( size );

   if( str != NULL )
   {
      if( value > 0xFFFF )
         value = 0xFFFF;
      sprintf( &str[1], "%08X%s", value, string );
      str[0] = 1;
   }

   return (TA_String *)str;
}

static TA_String *valueAllocInternal( unsigned long value )
{
   unsigned int size;
   char buffer[100];
   char *str;

   buffer[0] = '\0';
   sprintf(buffer, "%u", (unsigned int)value );
   size = strlen(buffer)+2;
   str = (char *)TA_Malloc( size );
   
   if( str != NULL )
   {
      strcpy( &str[1], buffer );
      str[0] = 1;
   }
   
   return (TA_String *)str;
}

TA_String *stringAllocNInternal( TA_StringCache *stringCache,
                                 const char *string,
                                 unsigned int maxNbChar,
                                 TA_CharCase caseType )
{
   TA_String *tmp;
   unsigned int hashIndex;
   char *hashEntry;
   TA_StringCachePriv *stringCachePriv;
   unsigned int newStringLength;
   int sameString;
   unsigned int i;

   #if !defined( TA_SINGLE_THREAD )
   TA_RetCode retCode;
   #endif

   if( stringCache == NULL )
      return NULL;

   stringCachePriv = (TA_StringCachePriv *)stringCache;

   if( !string )
      return NULL;

   hashIndex = calcHash( string, caseType );
   TA_ASSERT_RET( hashIndex < NB_CACHE_ENTRY, (TA_String *)NULL );

   /* Evaluate the final length of the new string. */
   newStringLength = strlen( string );
   if( maxNbChar != 0 )
   {
      if( maxNbChar < newStringLength )
         newStringLength = maxNbChar;
   }

   #if !defined( TA_SINGLE_THREAD )
      /* Get the semaphore for this cache. */
      retCode = TA_SemaWait( &stringCachePriv->sema );

      if( retCode != TA_SUCCESS )
         return NULL;
   #endif

   /* Check if already in the hash table. If yes, re-use it. */
   hashEntry = (char *)stringCachePriv->cache[ hashIndex ];
   if( hashEntry && ((unsigned char)hashEntry[0] < 255) )
   {
      /* Check that this is the same string, same size. */
      if( (caseType == NO_CASE) && 
          (!strncmp( string, &hashEntry[1], newStringLength)) &&
          (hashEntry[newStringLength+1] == '\0') )
         sameString = 1;     
      else if( caseType == PATH )
      {
          sameString = 1;
          for( i=0; i <= newStringLength && (sameString == 1); i++ )
          {
             if( hashEntry[i+1] == '\0' )
                sameString = 0;
             if( (string[i] != hashEntry[i+1]) && !TA_IsSeparatorChar(string[i]) )
                sameString = 0;
          }

          if( sameString && (hashEntry[newStringLength+1] != '\0') )
            sameString = 0;
      }
      else if( caseType == UPPER_CASE )
      {
          sameString = 1;
          for( i=0; i <= newStringLength && (sameString == 1); i++ )
          {
             if( hashEntry[i+1] == '\0' )
                sameString = 0;
             if( (toupper(string[i]) != hashEntry[i+1]) )
                sameString = 0;
          }

          if( sameString && (hashEntry[newStringLength+1] != '\0') )
            sameString = 0;
      }
      else
         sameString = 0;

      if( sameString )
      {
         tmp = stringDupInternal( (TA_String *)hashEntry );
         #if !defined( TA_SINGLE_THREAD )
            TA_SemaPost( &stringCachePriv->sema );         
         #endif
         return tmp;
      }
   }

   tmp = stringAllocInternal( string, newStringLength, caseType );

   if( tmp != NULL )
   {
      /* Store in cache. */

      /* Delete previous entry in the same position in the cache. */
      if( hashEntry )
         stringFreeInternal( (TA_String *)hashEntry );

      /* Keep track of this latest allocation. */
      stringCachePriv->cache[hashIndex] = stringDupInternal( tmp );
   }

   #if !defined( TA_SINGLE_THREAD )
      TA_SemaPost( &stringCachePriv->sema );
   #endif

   return tmp;
}
