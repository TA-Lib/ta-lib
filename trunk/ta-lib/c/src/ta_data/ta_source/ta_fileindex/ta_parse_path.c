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
 *  110199 MF   First version.
 *
 */

/* Description:
 *    Parse a path into tokens. This parsing includes recognition
 *    of special fields.
 */

/**** Headers ****/
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ta_system.h"
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_fileindex_priv.h"
#include "ta_global.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#define SEARCH_FORWARD  1 /* For searchIdInLastLeg() direction parameter */
#define SEARCH_BACKWARD 0 /* For searchIdInLastLeg() direction parameter */

/**** Local functions declarations.    ****/
static TA_RetCode flushFixPart( TA_FileIndexPriv *fileIndexPriv,
                                const char *currentTokenStart,
                                char *pos );
static TA_RetCode addToken( TA_FileIndexPriv *fileIndexPriv,
                            TA_TokenId id, const char *value );

static TA_RetCode findTokenId( TA_Libc *libHandle,
                               const char *str,
                               TA_TokenId *id );

static TA_RetCode checkForRedundantToken( TA_FileIndexPriv *fileIndexPriv );

static TA_RetCode checkForConsecutiveToken( TA_FileIndexPriv *fileIndexPriv );

static int isContainingSymbolField( TA_FileIndexPriv *fileIndexPriv );
static TA_RetCode processSymbolFieldSubstitution( TA_FileIndexPriv *fileIndexPriv );

#if 0
static void displayTokens( TA_FileIndexPriv *fileIndexPriv );
#endif

static const char *findDot( const char *str, unsigned int *returnFoundOffset );

static TA_TokenInfo *searchIdInLastLeg( int direction,
                                        TA_FileIndexPriv *fileIndexPriv,
                                        TA_TokenId id );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_FileIndexParsePath( TA_FileIndexPriv *fileIndexPriv, TA_String *path )
{
   typedef enum { INIT_PROCESSING,
                  FIX_PROCESSING,
                  FIELD_PROCESSING,
                  WILD_PROCESSING,
                  SEP_PROCESSING } State;

   TA_PROLOG;
   State currentState;
   const char *currentTokenStart;
   unsigned int length;
   char *str;
   char *pos;
   char sepTmp[2];
   TA_RetCode retCode;
   unsigned int tokenSize;
   TA_TokenId tokenId;
   const char *sourcePattern;
   TA_Libc *libHandle;

   libHandle = fileIndexPriv->libHandle;

   TA_TRACE_BEGIN( libHandle, TA_FileIndexParsePath );

   TA_ASSERT( libHandle, path != NULL );

   sepTmp[1] = '\0';
   sourcePattern = TA_StringToChar( path );

   /* The following macro should help for the readability of the parsing logic.
    * These macro are used only inside this function.
    */
   #define RETURN(y) {TA_Free(libHandle,str); TA_TRACE_RETURN( y );}
   #define REJECT_STATE(x,y) { if(currentState==x)RETURN(y);  }
   #define CHANGE_STATE(x) {currentState=x; currentTokenStart=pos+1;}

   #define ADD_TOKEN(id,value) \
   { \
      retCode = addToken(fileIndexPriv,id,value); \
      if( retCode != TA_SUCCESS) RETURN(retCode); \
   }

   #define FLUSH_FIX() \
   { \
      retCode = flushFixPart(fileIndexPriv,currentTokenStart,pos); \
      if( retCode != TA_SUCCESS ) RETURN(retCode); \
   }

   /* This function build a list representing the tokens
    * of the sourcePattern.
    *
    * Example:  "C:\a\*AZD?\[S]\data.txt" becomes
    *
    *            TokenId      Value
    *            TA_TOK_FIX       "C:"
    *            TA_TOK_SEP       "\"
    *            TA_TOK_FIX       "a"
    *            TA_TOK_SEP       "\"
    *            TA_TOK_WILD      "*"
    *            TA_TOK_FIX       "AZD"
    *            TA_TOK_WILD_CHAR "?"
    *            TA_TOK_SEP       "\"
    *            TA_TOK_S         "?*"
    *            TA_TOK_SEP       "\"
    *            TA_TOK_FIX       "data.txt"
    *            TA_TOK_END       (null)
    *
    *        In the values, the '?' and '*' character represent MS-DOS kind
    *        of wildcards:
    *             '?' is any character (but only one).
    *             '*' zero or more of any character
    */
   if( sourcePattern == NULL )
      return TA_INVALID_PATH;

   length = strlen( sourcePattern ) + 1;

   if( (length <= 1) || (length > 2048) )
      return TA_INVALID_PATH;

   str = (char *)TA_Malloc( libHandle, length );
   strcpy( str, sourcePattern );

   pos = str;
   currentState = INIT_PROCESSING;
   currentTokenStart = pos;

   while( *pos != '\0' )
   {

      if( (*pos == '\\') || (*pos == '/') )
      {
         /* Handle directories separator character. */
         REJECT_STATE( FIELD_PROCESSING, TA_INVALID_FIELD );
         REJECT_STATE( SEP_PROCESSING, TA_INVALID_PATH );

         FLUSH_FIX();

#if 0
!!! Needed?
         /* Check that the string prior to the separator
          * does not terminate with a dot '.'
          */
         if( currentState != INIT_PROCESSING )
         {
            if( *(pos-1) == '.' )
               RETURN( TA_INVALID_PATH );
         }
#endif
         /* Transform into the directory delimiter
          * used on the host file system.
          */
         sepTmp[0] = (char)TA_SeparatorASCII();
         ADD_TOKEN( TA_TOK_SEP, sepTmp );

         CHANGE_STATE( SEP_PROCESSING );
      }
      else switch( *pos )
      {
      case '*':
         /* Cannot have a wildcard in a field! */
         REJECT_STATE( FIELD_PROCESSING, TA_INVALID_FIELD );

         FLUSH_FIX();

         /* Add the wild card token. */
         ADD_TOKEN( TA_TOK_WILD, NULL );

         /* Keep track that we just processed a wild card token. */
         CHANGE_STATE( WILD_PROCESSING );
         break;

      case '[':
         /* Cannot have a field inside another field! */
         REJECT_STATE( FIELD_PROCESSING, TA_INVALID_FIELD );
         FLUSH_FIX();
         CHANGE_STATE( FIELD_PROCESSING );
         break;

      case ']':
         /* Cannot close a field when not processing a field! */
         REJECT_STATE( WILD_PROCESSING, TA_INVALID_FIELD );
         REJECT_STATE( INIT_PROCESSING, TA_INVALID_FIELD );
         REJECT_STATE( FIX_PROCESSING, TA_INVALID_FIELD );
         REJECT_STATE( SEP_PROCESSING, TA_INVALID_FIELD );

         /* Return an error if the field is an empty string. */
         if( currentTokenStart == pos )
            RETURN( TA_INVALID_FIELD );

         /* Identify/store the field.
          * Will return an error if this is a non-valid field.
          */
         *pos = '\0';
         if( findTokenId( libHandle, currentTokenStart, &tokenId ) != TA_SUCCESS )
            RETURN( TA_INVALID_FIELD );

         /* At this point the field has been identified and validated. */
         ADD_TOKEN( tokenId, NULL );

         tokenSize = TA_TokenMaxSize( libHandle, tokenId );

         if( tokenSize == 0 )
         {
            /* Everything is fine, now indicates that we
             * just met a wild string...
             */
            CHANGE_STATE( WILD_PROCESSING );
         }
         else
            CHANGE_STATE( FIX_PROCESSING );
         break;

      case '?':
         REJECT_STATE( FIELD_PROCESSING, TA_INVALID_FIELD );

         FLUSH_FIX();

         ADD_TOKEN( TA_TOK_WILD_CHAR, NULL );

         CHANGE_STATE( FIX_PROCESSING );
         break;

#if 0
!!! Needed?
      case '.':
         /* These other characters are correct within
          * a fix section, but strictly forbidden
          * within a field (possibly for future use).
          */
         REJECT_STATE( FIELD_PROCESSING, TA_INVALID_FIELD );
         currentState = FIX_PROCESSING;
         break;
#endif

      default:
         /* Skip the character, and start doing FIX processing
          * except if we are actually handling a field.
          */
         if( currentState != FIELD_PROCESSING )
            currentState = FIX_PROCESSING;
      }

      pos++;
   }

   /* Detect not terminated field. */
   if( currentState == FIELD_PROCESSING )
      RETURN( TA_INVALID_FIELD );

   /* We did end up with a path without any character!?
    * This should never happen.
    */
   if( currentState == INIT_PROCESSING )
   {
      TA_FATAL( libHandle, NULL, pos, currentTokenStart );
      RETURN( TA_INVALID_PATH );
   }

   FLUSH_FIX();

   #if 0
      displayTokens( fileIndexPriv );
   #endif

   /* When terminated with a separator, assume the caller
    * want to include all files within the specified directory.
    * Consequently, by default, append a "[S]" or "*".
    */
   if( currentState == SEP_PROCESSING )
   {
      if( isContainingSymbolField( fileIndexPriv ) )
      {
         ADD_TOKEN( TA_TOK_WILD, NULL );
      }
      else
      {
         ADD_TOKEN( TA_TOK_SYM, NULL );     
      }

      #if 0
         displayTokens( fileIndexPriv );
      #endif
   }

   /* If NO [S] field is specified, perform some "intuitive" substitution
    * for the caller.
    */
   if( isContainingSymbolField( fileIndexPriv ) == 0 )
   {
      processSymbolFieldSubstitution( fileIndexPriv );
      #if 0
         displayTokens( fileIndexPriv );
      #endif
   }

   ADD_TOKEN( TA_TOK_END, NULL );

   /* At this point the list of tokens is determined
    * and should not changed.
    */
   #if 0
      displayTokens( fileIndexPriv );
   #endif

   /* Parano test: Verify again that a [S] symbol field is present. */
   if( isContainingSymbolField( fileIndexPriv ) == 0 )
      RETURN( TA_INVALID_PATH );

   /* Should not have more than one occurence of some of the
    * fields (like [S],[YYYY] etc...)
    */
   retCode = checkForRedundantToken( fileIndexPriv );

   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   /* Validate that no two identical token
    * are consecutive (except for TA_TOK_WILD_CHAR).
    */
   retCode = checkForConsecutiveToken( fileIndexPriv );

   if( retCode != TA_SUCCESS )
      RETURN( retCode );

   /* Succesfully return. */
   RETURN( TA_SUCCESS );

   #undef RETURN
   #undef REJECT_STATE
   #undef ADD_TOKEN
   #undef CHANGE_STATE
   #undef FLUSH_FIX
}

/**** Local functions definitions.     ****/
static TA_RetCode flushFixPart( TA_FileIndexPriv *fileIndexPriv,
                                const char *currentTokenStart,
                                char *pos )
{
   TA_RetCode retCode;
   char tmp;

   if( currentTokenStart != pos )
   {
      /* Add this FIX string to the list if size is
       * different of zero.
       */
      tmp = *pos;
      *pos = '\0';
      retCode = addToken( fileIndexPriv, TA_TOK_FIX, currentTokenStart );
      *pos = tmp;
      return retCode;
   }

   return TA_SUCCESS;
}

static TA_RetCode addToken( TA_FileIndexPriv *fileIndexPriv,
                            TA_TokenId id, const char *value )
{
   TA_String *str = NULL;
   TA_RetCode retCode;
   TA_Libc *libHandle;
   TA_StringCache *stringCache;

   libHandle = fileIndexPriv->libHandle;
   stringCache = TA_GetGlobalStringCache( libHandle );

   if( value )
   {
      str = TA_StringAlloc(stringCache,value);

      if( str == NULL )
         return TA_ALLOC_ERR;
   }
   else
   {
      /* Some token use pre-defined values. */
      switch( id )
      {
      case TA_TOK_WILD:
         str = TA_StringDup( stringCache,fileIndexPriv->wildZeroOrMoreChar );
         if( !str )
            return TA_ALLOC_ERR;
         break;
      case TA_TOK_WILD_CHAR:
         str = TA_StringDup( stringCache,fileIndexPriv->wildOneChar );
         if( !str )
            return TA_ALLOC_ERR;
         break;
      case TA_TOK_SYM:
      case TA_TOK_CAT:
      case TA_TOK_CATC:
      case TA_TOK_CATX:
      case TA_TOK_CATT:
         str = TA_StringDup( stringCache,fileIndexPriv->wildOneOrMoreChar );
         if( !str )
            return TA_ALLOC_ERR;
         break;
      default:
         /* Do nothing */
         break;
      }
   }

   retCode = TA_FileIndexAddTokenInfo( fileIndexPriv, id, str, NULL );

   /* TA_FileIndexAddTokenInfo keeps its own duplicate of the string, so
    * simply free this local copy.
    */
   if( str )
      TA_StringFree( stringCache, str);

   return retCode;
}

static TA_RetCode findTokenId( TA_Libc *libHandle, const char *str, TA_TokenId *id )
{
   unsigned int i;
   const char *cmp_str;

   *id = (TA_TokenId)NULL;

   for( i=0; i < TA_NB_TOKEN_ID; i++ )
   {
      cmp_str = TA_TokenString( libHandle, (TA_TokenId)i );

      if( cmp_str )
      {
	  #if defined( WIN32 )
          if( stricmp( str, cmp_str ) == 0 )
          #else
          if( strcasecmp( str, cmp_str ) == 0 )
          #endif
          {
             /* Make sure it is a valid field that can be found
              * in a path.
              */
             switch( i )
             {
             /* !!! For the time being, we do not accept date
              * within the path
             case TA_YYYY:
             case TA_MM:
             case TA_DD:
             */
             case TA_TOK_SYM:
             case TA_TOK_CAT:
             case TA_TOK_CATC:
             case TA_TOK_CATX:
             case TA_TOK_CATT:
                *id = (TA_TokenId)i;
                return TA_SUCCESS;
             default:
                return TA_INVALID_FIELD;
             }
          }
      }
   }

   return TA_INVALID_FIELD;
}

static TA_RetCode checkForRedundantToken( TA_FileIndexPriv *fileIndexPriv )
{
   int i;
   int tokenSeenOnce;
   TA_TokenInfo *token;

   TA_List *list = fileIndexPriv->listLocationToken;

   for( i=0; i < TA_NB_TOKEN_ID; i++ )
   {
      if( (i != TA_TOK_FIX) &&
          (i != TA_TOK_SEP) &&
          (i != TA_TOK_WILD_CHAR) &&
          (i != TA_TOK_WILD) )
      {
         token = (TA_TokenInfo *)TA_ListAccessHead( list );

         if( token == NULL )
            return TA_INVALID_PATH; /* Empty list!? */
         else
         {
            tokenSeenOnce = 0;
            do
            {
               if( i == token->id )
               {
                  if( tokenSeenOnce )
                     return TA_INVALID_FIELD;
                  tokenSeenOnce++;
               }

               token = (TA_TokenInfo *)TA_ListAccessNext( list );
            } while( token );
         }
      }
   }

   return TA_SUCCESS;
}

static TA_RetCode checkForConsecutiveToken( TA_FileIndexPriv *fileIndexPriv )
{
   TA_TokenInfo *token;
   TA_TokenInfo *prevToken;

   TA_List *list = fileIndexPriv->listLocationToken;

   token = (TA_TokenInfo *)TA_ListAccessHead( list );

   if( token == NULL )
      return TA_INVALID_PATH; /* Empty list!? */
   else
   {
      prevToken = NULL;
      do
      {
         if( prevToken )
         {
            if( (prevToken->id == token->id) &&
                (token->id != TA_TOK_WILD_CHAR) &&
                (token->id != TA_TOK_WILD) )
               return TA_INVALID_PATH;
         }

         prevToken = token;
         token = (TA_TokenInfo *)TA_ListAccessNext( list );
      }while( token );
   }

   return TA_SUCCESS;
}

static int isContainingSymbolField( TA_FileIndexPriv *fileIndexPriv )
{
   TA_TokenInfo *token;
   TA_List *list = fileIndexPriv->listLocationToken;

   token = (TA_TokenInfo *)TA_ListAccessHead( list );

   if( token == NULL )
      return TA_INVALID_PATH; /* Empty list!? */
   else
   {
      do
      {
         if( (token->id == TA_TOK_SYM) || (token->id == TA_TOK_SYMF) )
            return 1;
         token = (TA_TokenInfo *)TA_ListAccessNext( list );
      }while( token );
   }

   return 0;
}

static TA_TokenInfo *searchIdInLastLeg( int direction,
                                        TA_FileIndexPriv *fileIndexPriv,
                                        TA_TokenId id )
{
   TA_TokenInfo *token;
   TA_List *list;
   TA_TokenInfo *lastTokenSeen;

   /* if 'direction' is SEARCH_FORWARD, the left most occurence of id is returned.
    * if 'direction' is SEARCH_BACKWARD, the last occurence of id is returned.
    */

   lastTokenSeen = NULL;

   list = fileIndexPriv->listLocationToken;
   token = (TA_TokenInfo *)TA_ListAccessTail( list );

   while( token )
   {
      if( token->id == TA_TOK_SEP )
      {
         /* Got at the beginning of the last leg. */
         if( direction == SEARCH_FORWARD )
            return lastTokenSeen;
         else
            return NULL;
      }

      /* Look for 'id' while going backward. */
      if( token->id == id )
      {
         /* We found the first occurence while going backward... just return it! */
         if( direction == SEARCH_BACKWARD )
            return token;

         lastTokenSeen = token;
      }

      token = (TA_TokenInfo *)TA_ListAccessPrev( list );
   }

   return lastTokenSeen;
}

/* Find leftmost dot in the str. Return NULL if not found. */
static const char *findDot( const char *str, unsigned int *returnFoundOffset )
{
   unsigned int i = 0;

   while( *str )
   {
     if( *str == '.' )
     {
        *returnFoundOffset = i;
        return str;
     }
     str++;
     i++;
   }

   return NULL;
}

static TA_RetCode processSymbolFieldSubstitution( TA_FileIndexPriv *fileIndexPriv )
{
   TA_PROLOG;
   TA_String *tmpPtr;
   TA_TokenInfo *token;
   char *str;
   TA_RetCode retCode;
   TA_Libc *libHandle;
   TA_StringCache *stringCache;
   unsigned int dotPos;

   libHandle = fileIndexPriv->libHandle;

   if( !libHandle )
      return TA_UNKNOWN_ERR;

   TA_TRACE_BEGIN( libHandle, processSymbolFieldSubstitution );

   stringCache = TA_GetGlobalStringCache( libHandle );

   /* For the time being, when there is no symbol defined,
    * it is not allowed to use the '?' wildcard in the last
    * leg of the path.
    */
   token = searchIdInLastLeg( SEARCH_BACKWARD, fileIndexPriv, TA_TOK_WILD_CHAR );
   if( token )
   {
      TA_TRACE_RETURN( TA_INVALID_PATH );
   }

   /* Perform the following default substitution:
    * - when last leg contains at least one [TA_TOK_WILD]:
    *      Replace the first [TA_TOK_WILD] with a [TA_TOK_S].
    *
    * - when last leg finish with a [TA_TOK_FIX] without any [TA_TOK_WILD]:
    *      Split the [TA_TOK_FIX] into [TA_TOK_SF][TA_TOK_FIX] if the original
    *      [TA_TOK_FIX] contains at least one dot '.' The dot determine
    *      where the split occur.
    *      If there is no dot, replace [TA_TOK_FIX] with [TA_TOK_SF].
    *
    */
   token = searchIdInLastLeg( SEARCH_FORWARD, fileIndexPriv, TA_TOK_WILD );
   if( token )
   {
      TA_ASSERT( libHandle, token->id == TA_TOK_WILD );
      TA_ASSERT( libHandle, token->value != NULL );

      /* Substitute the TA_TOK_WILD with TA_TOK_SYM. */
      token->id = TA_TOK_SYM;
      TA_StringFree( stringCache, token->value );
      token->value = TA_StringDup( stringCache, fileIndexPriv->wildOneOrMoreChar );
   }
   else
   {
      token = searchIdInLastLeg( SEARCH_BACKWARD, fileIndexPriv, TA_TOK_FIX );

      if( token )
      {
         TA_ASSERT( libHandle, token->id == TA_TOK_FIX );
         TA_ASSERT( libHandle, token->value != NULL );

         /* str will point on the leftmost dot if one is found.
          * 'dotPos' will indicate offset of the dot in the string (zero base).
          */
         str = (char *)findDot( TA_StringToChar( token->value ), &dotPos );

         if( str )
         {
            /* !!! needed?
            TA_ASSERT( libHandle, dotPos != 0 ); */
             
            /* Some validity check (parano).
             * Check that there is something before and after the dot...
             */
            if( (*(str+1) == '\0') || (str == TA_StringToChar( token->value )) )
            {
               TA_TRACE_RETURN( TA_INVALID_PATH );
            }

            /* Create a new TA_TOK_SF field using the character before the dot. */
            tmpPtr = TA_StringAllocN( stringCache, TA_StringToChar( token->value ), dotPos );
            if( !tmpPtr )
            {
               TA_TRACE_RETURN( TA_ALLOC_ERR );
            }

            retCode = TA_FileIndexAddTokenInfo( fileIndexPriv, TA_TOK_SYMF, tmpPtr, token );
            TA_StringFree( stringCache, tmpPtr );
            if( retCode != TA_SUCCESS )
            {
               TA_TRACE_RETURN( retCode );
            }

            /* Adjust the TA_TOK_FIX by keeping only the characters after the dot. */
            tmpPtr = TA_StringAlloc( stringCache, str );
            if( !tmpPtr )
            {
               TA_TRACE_RETURN( TA_ALLOC_ERR );
            }

            TA_StringFree( stringCache, token->value );
            token->value = tmpPtr;
         }
         else
         {
             /* There is no dot... the TA_TOK_FIX becomes a TA_TOK_SF. */
             token->id = TA_TOK_SYMF;
         }
      }
   }

   TA_TRACE_RETURN( TA_INVALID_PATH );
}

#if 0
static void displayTokens( TA_FileIndexPriv *fileIndexPriv )
{
   TA_TokenInfo *token;
   TA_List *list = fileIndexPriv->listLocationToken;
   const char *str;

   printf( "Token List: " );

   token = (TA_TokenInfo *)TA_ListAccessHead( list );

   if( token == NULL )
      printf( "empty!\n" );
   else
   {
      do
      {
         str = TA_TokenDebugString( fileIndexPriv->libHandle, token->id );

         if( !str )
            printf( "[(null)] " );
         else
         {
            if( token->id == TA_TOK_SEP )
               printf( "%s", TA_StringToChar( token->value ) );
            else if( token->id == TA_TOK_FIX )
               printf( "%s", TA_StringToChar( token->value ) );
            else if( token->value )
               printf( "[%s=%s]", str, TA_StringToChar( token->value ) );
            else
               printf( "[%s]", str );
         }

         token = (TA_TokenInfo *)TA_ListAccessNext( list );
      }while( token );

      printf( "\n" );
   }
}

#endif
