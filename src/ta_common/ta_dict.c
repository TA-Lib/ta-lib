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
 *  031900 MF   First version.
 *
 */

/* Implementation Description:
 *        This module is simply encapsulating the Kazlib library
 *        who is providing the whole dictionary functionality.
 *        This dictionary is attempting to simplify the allocation
 *        and de-allocation of the object belonging to the
 *        dictionary.
 */

/**** Headers ****/
#include <string.h>
#include "ta_common.h"
#include "ta_dict.h"
#include "ta_memory.h"
#include "kazlib/dict.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   dict_t d;
   void (*freeValueFunc)(TA_Libc *libHandle, void *);
   dnode_t *accessNode;
   unsigned int flags;
   TA_Libc *libHandle;
} TA_PrivDictInfo;

/**** Local functions declarations.    ****/
static int compareFunction_S ( TA_Libc *libHandle, const void *key1, const void *key2);
static int compareFunction_I ( TA_Libc *libHandle, const void *key1, const void *key2);

/*static const char *stringDuplicate( const char *string );*/

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
TA_Dict *TA_DictAlloc( TA_Libc *libHandle,
                       unsigned int flags,                       
                       void (*freeValueFunc)( TA_Libc *libHandle, void *) )
{
   TA_PrivDictInfo *theDict;

   if( !libHandle )
      return NULL;

   /* Alloc the structure used as an handle for this dictionary. */
   theDict = (TA_PrivDictInfo *) TA_Malloc( libHandle, sizeof( TA_PrivDictInfo ) );

   if( !theDict )
      return NULL;

   theDict->flags = flags;

   /* Create the Kazlib dictionary. */
   if( flags & TA_DICT_KEY_ONE_STRING )
      dict_init( libHandle, &theDict->d, DICTCOUNT_T_MAX, compareFunction_S );
   else if( flags & TA_DICT_KEY_TWO_STRING )
      dict_init( libHandle, &theDict->d, DICTCOUNT_T_MAX, compareFunction_S );
   else if( flags & TA_DICT_KEY_INTEGER )
      dict_init( libHandle, &theDict->d, DICTCOUNT_T_MAX, compareFunction_I );

   /* Keep a copy of the freeValueFunc pointer for later use. */
   theDict->freeValueFunc = freeValueFunc;

   /* Remember to which memory context we belong. */
   theDict->libHandle = libHandle;

   return (TA_Dict *)theDict;
}

TA_RetCode TA_DictFree( TA_Dict *dict )
{
   TA_PrivDictInfo *theDict;

   dnode_t   *node;
   dnode_t   *next;
   TA_String *stringToDelete;
   void      *valueToDelete;
   dict_t    *kazlibDict;
   TA_Libc   *libHandle;
   int        flags;

   theDict = (TA_PrivDictInfo *)dict;

   if( theDict == NULL )
      return TA_BAD_PARAM;

   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Delete all the key-value pair sequentially. */
   node = dict_first( libHandle, kazlibDict );

   while (node != NULL)
   {
      /* Get the next node. */
      next = dict_next( libHandle, kazlibDict, node );

      /* Free the 'node, the 'key' string and the 'value'. */
      flags = theDict->flags;
      valueToDelete  = dnode_get(node);
      if( flags & (TA_DICT_KEY_TWO_STRING|TA_DICT_KEY_ONE_STRING) )
      {
         stringToDelete = TA_StringFromChar(dnode_getkey(node));
         dict_delete_free( libHandle, kazlibDict, node );
         TA_StringFree( TA_GetGlobalStringCache( libHandle ), stringToDelete );
      }
      else
         dict_delete_free( libHandle, kazlibDict, node );

      if( flags & TA_DICT_KEY_TWO_STRING )
      {
         /* The value is a dictionary. Delete it. */
         TA_DictFree( (TA_Dict *)valueToDelete );
      }
      else if( theDict->freeValueFunc )
         theDict->freeValueFunc( libHandle, valueToDelete );

      node = next;
   }

   /* Free the TA_PrivDictInfo */
   TA_Free( libHandle, theDict );

   return TA_SUCCESS;
}


TA_RetCode TA_DictAddPair_S2( TA_Dict *dict,
                              TA_String *key1,
                              TA_String *key2,
                              void *value )
{
   TA_PrivDictInfo *theDict;
   dnode_t *node;
   TA_String *dupKey;
   TA_Dict *subDict;
   TA_Libc *libHandle;
   dict_t  *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) ||
       (key1 == NULL) || (key2 == NULL) || (value == NULL) )
      return TA_BAD_PARAM;
   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Verify if a a dictionary already exist for key1. */
   node = dict_lookup( libHandle, kazlibDict, TA_StringToChar(key1) );

   if( node )
   {
      /* A dictionary already exist with the same key1... re-use it. */
      subDict = (TA_Dict *)dnode_get( node );
   }
   else
   {
      /* Alloc a new directory corresponding to key1. */
      subDict = TA_DictAlloc( libHandle, TA_DICT_KEY_ONE_STRING, theDict->freeValueFunc );

      if( !subDict )
         return TA_ALLOC_ERR;

      dupKey = TA_StringDup( TA_GetGlobalStringCache( libHandle ), key1 );

      if( !dupKey )
      {
         TA_DictFree( subDict );
         return TA_ALLOC_ERR;
      }

      if( !dict_alloc_insert( libHandle, kazlibDict, TA_StringToChar(dupKey), subDict ) )
      {
         TA_DictFree( subDict );
         TA_StringFree( TA_GetGlobalStringCache( libHandle ), dupKey );
         return TA_ALLOC_ERR;
      }
   }

   /* Insert the string in the subDict using key2 */
   return TA_DictAddPair_S( subDict, key2, value );
}

TA_RetCode TA_DictAddPair_S( TA_Dict *dict,
                             TA_String *key,
                             void *value )
{
   TA_PrivDictInfo *theDict;
   dnode_t *node;
   TA_String *dupKey;
   TA_Libc *libHandle;
   dict_t  *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (key == NULL) || (value == NULL) )       
      return TA_BAD_PARAM;
   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Verify if an entry exist already with the same key. */
   node = dict_lookup( libHandle, kazlibDict, TA_StringToChar(key) );

   if( node )
   {
      /* An entry already exist with the same key...
       * Re-use the existing node. Just replace the
       * 'value' part.
       * De-allocate the older 'value'.
       */
      if( theDict->freeValueFunc )
         (*theDict->freeValueFunc)( libHandle, dnode_get( node ) );
      dnode_put( node, value );
   }
   else
   {
      /* Alloc/insert a new key-value pair in the dictionary. */
      dupKey = TA_StringDup( TA_GetGlobalStringCache( libHandle ), key );

      if( !dupKey )
         return TA_ALLOC_ERR;

      if( !dict_alloc_insert( libHandle, kazlibDict, TA_StringToChar(dupKey), value ) )
      {
         TA_StringFree( TA_GetGlobalStringCache( libHandle ), dupKey );
         return TA_ALLOC_ERR;
      }
   }

   return TA_SUCCESS;
}

TA_RetCode TA_DictAddPair_I( TA_Dict *dict,
                             int key,
                             void *value )
{
   TA_PrivDictInfo *theDict;
   dnode_t *node;
   TA_Libc *libHandle;
   dict_t  *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (value == NULL) )
      return TA_BAD_PARAM;

   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Verify if an entry exist already with the same key. */
   node = dict_lookup( libHandle, kazlibDict, (void *)key );

   if( node )
   {
      /* An entry already exist with the same key...
       * Re-use the existing node. Just replace the
       * 'value' part.
       * De-allocate the older 'value'.
       */
      if( theDict->freeValueFunc )
         (*theDict->freeValueFunc)( libHandle, dnode_get( node ) );
      dnode_put( node, value );
   }
   else
   {
      /* Insert the new key-value pair in the dictionary. */
      if( !dict_alloc_insert( libHandle, kazlibDict, (void *)key, value ) )
         return TA_ALLOC_ERR;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_DictDeletePair_S( TA_Dict *dict, const char *key )
{
   TA_PrivDictInfo *theDict;
   TA_String *stringToDelete;
   void      *valueToDelete;
   dnode_t   *node;
   TA_Libc   *libHandle;
   dict_t    *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (key == NULL) )
      return TA_BAD_PARAM;
   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Find the key-value pair. */
   node = dict_lookup( libHandle, kazlibDict, key );

   if( node )
   {
      /* Free the 'node', the 'key' string and the 'value'. */
      stringToDelete = TA_StringFromChar( dnode_getkey(node) );
      valueToDelete  = dnode_get(node);
      dict_delete_free( libHandle, kazlibDict, node );
      TA_StringFree( TA_GetGlobalStringCache( libHandle ), stringToDelete );
      if( theDict->freeValueFunc )
         theDict->freeValueFunc( libHandle, valueToDelete );
   }
   else
      return TA_KEY_NOT_FOUND;

   return TA_SUCCESS;
}

TA_RetCode TA_DictDeletePair_S2( TA_Dict *dict, const char *key1, const char *key2 )
{
   TA_PrivDictInfo *theDict;
   TA_String *stringToDelete;
   dnode_t   *node;
   TA_Dict   *subDict;
   TA_RetCode retCode;
   TA_Libc   *libHandle;
   dict_t    *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL)    ||
       (key1 == NULL)       || 
       (key2 == NULL))
      return TA_BAD_PARAM;
   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Find the dictionary for this 'key1'. */
   node = dict_lookup( libHandle, kazlibDict, key1 );

   if( !node )
      return TA_KEY_NOT_FOUND;

   subDict = (TA_Dict *)dnode_get(node);

   retCode = TA_DictDeletePair_S( subDict, key2 );

   /* Delete the dictionary if it is empty. */
   if( (retCode == TA_SUCCESS) && (TA_DictSize(subDict) == 0) )
   {
      TA_DictFree( subDict );
      /* Free the 'node' and the 'key1' string. */
      stringToDelete = TA_StringFromChar( dnode_getkey(node) );
      dict_delete_free( theDict->libHandle, kazlibDict, node );
      TA_StringFree( TA_GetGlobalStringCache( libHandle ), stringToDelete );
   }
   return TA_SUCCESS;
}

TA_RetCode TA_DictDeletePair_I( TA_Dict *dict, int key )
{
   TA_PrivDictInfo *theDict;
   void      *valueToDelete;
   dnode_t   *node;
   TA_Libc   *libHandle;
   dict_t    *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( theDict == NULL )
      return TA_BAD_PARAM;
   kazlibDict = &theDict->d;
   libHandle  = theDict->libHandle;

   /* Find the key-value pair. */
   node = dict_lookup( libHandle, kazlibDict, (void *)key );

   if( node )
   {
      /* Free the 'node' and the 'value'. */
      valueToDelete  = dnode_get(node);
      dict_delete_free( libHandle, kazlibDict, node );
      if( theDict->freeValueFunc )
         theDict->freeValueFunc( libHandle, valueToDelete );
   }
   else
      return TA_KEY_NOT_FOUND;

   return TA_SUCCESS;
}

void *TA_DictGetValue_S( TA_Dict *dict, const char *key )
{
   TA_PrivDictInfo *theDict;
   dnode_t *node;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (key == NULL) )
      return NULL;

   /* Find the key-value pair. */
   node = dict_lookup( theDict->libHandle, &theDict->d, key );

   if( !node )
      return NULL;

   return dnode_get(node);
}

void *TA_DictGetValue_S2( TA_Dict *dict, const char *key1, const char *key2 )
{
   TA_PrivDictInfo *theDict;
   dnode_t *node;
   TA_PrivDictInfo *subDict;
   TA_Libc *libHandle;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (key1 == NULL) || (key2 == NULL))
      return NULL;
   libHandle = theDict->libHandle;

   /* Find the dictionary for key1. */
   node = dict_lookup( libHandle, &theDict->d, key1 );

   if( !node )
      return NULL;

   subDict = dnode_get(node);

   /* Find the key-value pair using key2. */
   node = dict_lookup( libHandle, &subDict->d, key2 );

   if( !node )
      return NULL;

   return dnode_get(node);
}

void *TA_DictGetValue_I( TA_Dict *dict, int key )
{
   TA_PrivDictInfo *theDict;
   dnode_t *node;

   theDict = (TA_PrivDictInfo *)dict;

   if( theDict == NULL )
      return NULL;

   /* Find the key-value pair. */
   node = dict_lookup( theDict->libHandle, &theDict->d, (void *)key );

   if( !node )
      return NULL;

   return dnode_get(node);
}

int TA_DictAccessFirst( TA_Dict *dict )
{
   TA_PrivDictInfo *theDict;
   dict_t *kazlibDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( theDict == NULL )
      return (int)NULL;
   kazlibDict = &theDict->d;

   if( !(theDict->flags & TA_DICT_KEY_ONE_STRING) )
      return 0; /* All other dictionary type are not supported. */

   /* Get the first node. */
   theDict->accessNode = dict_first( theDict->libHandle, kazlibDict );

   if( !theDict->accessNode )
      return 0;

   return dict_count( kazlibDict );
}

TA_String *TA_DictAccessKey( TA_Dict *dict )
{
   TA_PrivDictInfo *theDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (theDict->accessNode == NULL) )
      return NULL;

   return TA_StringFromChar( dnode_getkey( theDict->accessNode ) );
}

void *TA_DictAccessValue( TA_Dict *dict )
{
   TA_PrivDictInfo *theDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (theDict->accessNode == NULL) )
      return NULL;

   return dnode_get( theDict->accessNode );
}

int TA_DictAccessNext( TA_Dict *dict )
{
   TA_PrivDictInfo *theDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( (theDict == NULL) || (theDict->accessNode == NULL) )
      return 0;

   /* Move to the next node. */
   theDict->accessNode = dict_next( theDict->libHandle, &theDict->d, theDict->accessNode );

   if( !theDict->accessNode )
   {
      /* There is no element left... */
      return 0;
   }

   return 1;
}

unsigned int TA_DictSize( TA_Dict *dict )
{
   TA_PrivDictInfo *theDict;

   theDict = (TA_PrivDictInfo *)dict;

   if( theDict == NULL)
      return 0;

   return (unsigned int)dict_count( &theDict->d );
}

/**** Local functions definitions.     ****/
static int compareFunction_S( TA_Libc *libHandle, const void *key1, const void *key2 )
{
   (void)libHandle;
   return strcmp( key1, key2 );
}

static int compareFunction_I( TA_Libc *libHandle, const void *key1, const void *key2 )
{
   (void)libHandle;
   return (int)key1 - (int)key2;
}
