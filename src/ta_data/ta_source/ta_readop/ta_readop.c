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
 *  MS       Marcel Schaible <marcel@schaible-consulting.de>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  110199 MF   First version.
 *  010503 MF   Fix #644512. Problem with [-I] submitted by MS.
 *  040304 MF   Add support for TA_TOK_SKIP_NON_DIGIT_LINE
 */

/* Description:
 *    Allows to allocate/de-allocate TA_DataSourceHandle structure.
 */

/**** Headers ****/
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "ta_memory.h"
#include "ta_trace.h"
#include "ta_fileindex.h"
#include "ta_token.h"
#include "ta_global.h"
#include "ta_readop.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions.    ****/
static TA_RetCode findTokenId( const char *str,
                               TA_TokenId *id,
                               TA_Integer *optionalParam );

static TA_RetCode buildReadOp( TA_ReadOpInfo *readOpInfo,
                               const char *localBuf,
                               TA_ReadOp *readOp,
                               TA_TokenId *tokenId,
                               unsigned int *intraDayIncrementInSecond );

static TA_RetCode findTokenIdWithParam( const char *str,
                                        TA_TokenId *tokenId,
                                        unsigned int *extractedParam );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

TA_RetCode TA_ReadOpInfoFree( TA_ReadOpInfo *readOpInfoToBeFreed )
{
   if( readOpInfoToBeFreed )
   {
      if( readOpInfoToBeFreed->arrayReadOp )
         TA_Free( readOpInfoToBeFreed->arrayReadOp );

      TA_Free( readOpInfoToBeFreed );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_ReadOpInfoAlloc( const char *sourceInfo,
                               TA_ReadOpInfo **allocatedInfo,
                               unsigned int readOpFlags )
{
   TA_PROLOG
   TA_RetCode retCode;

   TA_ReadOp readOp;
   TA_ReadOp *arrayReadOp;
   TA_ReadOpInfo *newReadOpInfo;
   TA_Field fieldMask, fieldProvided;
   unsigned int timeframeIdx;
   unsigned int intraDayIncPeriod;
   TA_TokenId   intraDayIncToken;
   TA_TokenId   tempToken;
   unsigned int tempInt;
   unsigned int period;
   unsigned int errorOccurred;

   const char *pos;
   unsigned int inField;
   unsigned int nbField;
   unsigned int nbCharInField;
   unsigned int skipNonDigitLine;
   const char *ptrFirstCarInField;

   unsigned char localBuf[10];
   unsigned int bufIdx, opIdx, i;

   register unsigned int flagSet;
   register TA_ReadOp *ptrReadOp;
   
   TA_TRACE_BEGIN(  TA_BuildArrayReadOp );

   newReadOpInfo = (TA_ReadOpInfo *)TA_Malloc( sizeof( TA_ReadOpInfo ) );

   /* These variables are resolved within this function. */
   memset( newReadOpInfo, 0, sizeof( TA_ReadOpInfo ) );

   /* At this point, TA_ReadOpInfoFree can be safely called. */

   /* Keep track of some user provided parameter. */
   newReadOpInfo->sourceInfo = sourceInfo;
   newReadOpInfo->readOpFlags = readOpFlags;

   /* Initialize some defaults. */
   newReadOpInfo->openInterestMult = 100;
   newReadOpInfo->volumeMult       = 100;

   nbField = 0;
   intraDayIncPeriod = 0;
   intraDayIncToken = TA_TOK_END;

   pos = newReadOpInfo->sourceInfo;
   if( !pos || (*pos == '\0') )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_MISSING_FIELD );
   }

   /* Find how many fields are defined and check some syntax
    * at the same time.
    */
   if( *pos != '[' )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_INVALID_FIELD );
   }

   inField = 0;
   nbCharInField = 0;
   skipNonDigitLine = 0;
   ptrFirstCarInField = NULL;
   while( *pos != '\0' )
   {
      switch( *pos )
      {
      case '[':
         if( inField )
         {
            TA_ReadOpInfoFree( newReadOpInfo );
            TA_TRACE_RETURN( TA_INVALID_FIELD );
         }
         inField = 1;
         break;
      case ']':
         if( (!inField) || (nbCharInField == 0) )
         {
            TA_ReadOpInfoFree( newReadOpInfo );
            TA_TRACE_RETURN( TA_INVALID_FIELD );
         }

         nbField++;

         /* Exclude fields not generating a TA_ReadOp.
          * For the time being that means only the -H and -NDL field.
          */
         if( nbCharInField >= 2 )
         {
            TA_ASSERT( ptrFirstCarInField != NULL );
            if( ptrFirstCarInField[0] == '-' ) 
            {
               if( toupper(ptrFirstCarInField[1]) == 'H' )
                  nbField--;
               else if( (toupper(ptrFirstCarInField[1]) == 'N') &&
                        (toupper(ptrFirstCarInField[2]) == 'D') &&
                        (toupper(ptrFirstCarInField[3]) == 'L') )
               {
                  skipNonDigitLine = 1;
                  nbField--;
               }               
             }
         }

         inField = 0;
         nbCharInField = 0;
         ptrFirstCarInField = NULL;
         break;
      default:
         if( !inField )
         {
            TA_ReadOpInfoFree( newReadOpInfo );
            TA_TRACE_RETURN( TA_INVALID_FIELD );
         }

         if( nbCharInField == 0 )
            ptrFirstCarInField = pos;
         nbCharInField++;
         break;
      }

      pos++;
   }

   if( inField || *(pos-1) != ']' )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_INVALID_FIELD );
   }

   /* Build the TA_ReadOp array */
   arrayReadOp = (TA_ReadOp *)TA_Malloc( sizeof( TA_ReadOp ) * nbField );

   if( !arrayReadOp )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   newReadOpInfo->arrayReadOp = arrayReadOp;

   pos = TA_StringToChar(newReadOpInfo->sourceInfo);

   bufIdx = 0;
   opIdx = 0;
   while( *pos != '\0' && (opIdx < nbField) )
   {
      switch( *pos )
      {
      case '[':
        break;

      case ']':
        localBuf[bufIdx] ='\0';
        bufIdx = 0;

        /* Identify the field and build the TA_ReadOp. */
        tempInt = 0;
        retCode = buildReadOp( newReadOpInfo,
                               (const char *)&localBuf[0],
                               &arrayReadOp[opIdx],
                               &tempToken, &tempInt );
        if( retCode != TA_SUCCESS )
        {
           TA_ReadOpInfoFree( newReadOpInfo );
           TA_TRACE_RETURN( retCode );
        }

        if( arrayReadOp[opIdx] != 0 )
        {
           /* Set the replace zero flag as needed */
           if( TA_IS_REPLACE_ZERO(readOpFlags) && TA_IS_REAL_CMD(arrayReadOp[opIdx]) )
           {
              TA_SET_REPLACE_ZERO(arrayReadOp[opIdx]);
           }

           /* Set the skipNonDigitLine flag as needed. */
           if( skipNonDigitLine == 1 )
           {
              TA_SET_SKIP_NDL_FLAG(arrayReadOp[opIdx]);
           }

           /* Ooof... this readOp is now all build! */
           opIdx++;
        }

        /* If this is a time token, make sure this
         * is not in contradiction with an already
         * specified increment.
         */
        if( intraDayIncPeriod )
        {
           errorOccurred = 0;
           switch( tempToken )
           {
           case TA_TOK_SEC:
           case TA_TOK_SS:
              if( (intraDayIncToken == TA_TOK_MIN) ||
                  (intraDayIncToken == TA_TOK_MN) )
                 errorOccurred = 1;
              /* no break */
           case TA_TOK_MIN:
           case TA_TOK_MN:
              if( (intraDayIncToken == TA_TOK_HOUR) ||
                  (intraDayIncToken == TA_TOK_HH) )
                 errorOccurred = 1;
              break;
           case TA_TOK_HOUR:
           case TA_TOK_HH:
              errorOccurred = 1;
              break;
           default:
              /* Do nothing */
              break;
           }

           if( errorOccurred )
           {
              TA_ReadOpInfoFree( newReadOpInfo );
              TA_TRACE_RETURN( TA_INVALID_FIELD );
           }
        }

        /* Check if a period increment is specified. */
        if( (tempInt != 0) && (tempInt != 1) )
        {
           if( intraDayIncPeriod != 0 )
           {
              TA_ReadOpInfoFree( newReadOpInfo );
              TA_TRACE_RETURN( TA_INVALID_FIELD );
           }

           intraDayIncPeriod = tempInt;
           intraDayIncToken  = tempToken;
        }
        break;

      default:
        if( bufIdx >= sizeof(localBuf)-1 )
        {
           TA_ReadOpInfoFree( newReadOpInfo );
           TA_TRACE_RETURN( TA_INVALID_FIELD );
        }

        localBuf[bufIdx++] = *pos;
        break;
      }

      pos++;
   }

   if( opIdx != nbField )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(89) );
   }

   arrayReadOp[opIdx-1] |= TA_CMD_LAST_FLAG;

   /* Build the mask representing the fields provided. */
   fieldProvided = 0;
   timeframeIdx = TA_INTEGER_ARRAY_SIZE;

   for( opIdx=0; opIdx < nbField; opIdx++ )
   {
      readOp = arrayReadOp[opIdx];

      TA_ASSERT( readOp != 0 ); /* Parano test */

      if( !TA_IS_PERMANENT_SKIP_SET(readOp) )
      {
         /* Make sure this field was not specified twice. */
         for( i=opIdx+1; i < nbField; i++ )
         {
            if( (TA_IS_REAL_CMD(readOp) && TA_IS_REAL_CMD(arrayReadOp[i])) ||
                (TA_IS_INTEGER_CMD(readOp) && TA_IS_INTEGER_CMD(arrayReadOp[i])) )
            {
               if( (TA_GET_IDX(readOp) == TA_GET_IDX(arrayReadOp[i])) &&
                   !TA_IS_PERMANENT_SKIP_SET(arrayReadOp[i]) )
               {
                  TA_ReadOpInfoFree( newReadOpInfo );
                  TA_TRACE_RETURN( TA_REDUNDANT_FIELD );
               }
            }
         }

         /* Parano test: Double-check redundant field in a different way. */
         fieldMask = TA_ReadOpToField( readOp );
         TA_ASSERT( fieldMask != 0 );
         if( !(fieldMask & TA_TIMESTAMP) && (fieldProvided & fieldMask) )
         {
            TA_ReadOpInfoFree( newReadOpInfo );
            TA_TRACE_RETURN( TA_REDUNDANT_FIELD );
         }

         /* Set the field. */
         fieldProvided |= fieldMask;

         /* Keep track of the smallest granularity of the timestamp. */
         if( fieldMask & TA_TIMESTAMP )
         {
            if( (timeframeIdx == TA_INTEGER_ARRAY_SIZE) ||
                (TA_GET_IDX(readOp) < timeframeIdx) )
               timeframeIdx = TA_GET_IDX(readOp);
         }
      }
   }


   /* No date/time reference provided!? This is considered an error
    * in the current implementation.
    */
   if( timeframeIdx == TA_INTEGER_ARRAY_SIZE )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_MISSING_DATE_OR_TIME_FIELD );
   }

   /* Determine at which point the timestamp is completed. */
   flagSet = 0;
   for( opIdx=nbField; opIdx > 0; opIdx-- )
   {
      ptrReadOp = &arrayReadOp[opIdx-1];
      readOp = *ptrReadOp;

      if( !flagSet                          && 
          TA_IS_INTEGER_CMD(readOp)         && 
          (TA_GET_IDX(readOp)<=TA_YEAR_IDX) && 
          !TA_IS_PERMANENT_SKIP_SET(readOp) )
      {
         TA_SET_TIMESTAMP_COMPLETE(*ptrReadOp);
         flagSet = 1;
      }
      else
      {
         TA_CLR_TIMESTAMP_COMPLETE(*ptrReadOp);
      }
   }

   /* Validate and identify the period. */
   period = 0;
   if( intraDayIncPeriod )
   {
      errorOccurred = 0;
      switch( timeframeIdx )
      {
      case TA_YEAR_IDX:
      case TA_MONTH_IDX:
      case TA_DAY_IDX:
         errorOccurred = 1;
         break;
      case TA_HOUR_IDX:
         if( (intraDayIncPeriod < TA_1HOUR) || (intraDayIncPeriod >= TA_DAILY) )
            errorOccurred = 1;
         break;
      case TA_MIN_IDX:
         if( (intraDayIncPeriod < TA_1MIN) || (intraDayIncPeriod >= TA_1HOUR) )
            errorOccurred = 1;
         break;
      case TA_SEC_IDX:
         if( (intraDayIncPeriod < TA_1SEC) || (intraDayIncPeriod >= TA_1MIN) )
            errorOccurred = 1;
         break;
      default:
         TA_ReadOpInfoFree( newReadOpInfo );
         TA_FATAL(  NULL, timeframeIdx, fieldProvided );
      }

      if( errorOccurred )
      {
         TA_ReadOpInfoFree( newReadOpInfo );
         TA_TRACE_RETURN( TA_INVALID_FIELD );
      }
            
      period = intraDayIncPeriod;
   }
   else
   {
      switch( timeframeIdx )
      {
      case TA_YEAR_IDX:
         period = TA_YEARLY;
         break;
      case TA_MONTH_IDX:
         period = TA_MONTHLY;
         break;
      case TA_DAY_IDX:
         period = TA_DAILY;
         break;
      case TA_HOUR_IDX:
         period = TA_1HOUR;
         break;
      case TA_MIN_IDX:
         period = TA_1MIN;
         break;
      case TA_SEC_IDX:
         period = TA_1SEC;
         break;
      default:
         TA_FATAL(  NULL, timeframeIdx, fieldProvided );
      }
   }

   /* A last check... */
   if( period == 0 )
   {
      TA_ReadOpInfoFree( newReadOpInfo );
      TA_TRACE_RETURN( TA_INVALID_FIELD );
   }
         
   /* Everything is fine, let's return the information. */
   newReadOpInfo->arrayReadOp = arrayReadOp;
   newReadOpInfo->fieldProvided = fieldProvided;
   newReadOpInfo->nbReadOp = nbField;
   newReadOpInfo->period = period;

   *allocatedInfo = newReadOpInfo;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_ReadOpInfoClone( TA_ReadOpInfo **allocatedInfo, TA_ReadOpInfo *src )
{
   /* Could be speed optimized, keep it simple for now. */
   return TA_ReadOpInfoAlloc( src->sourceInfo,
                              allocatedInfo,
                              src->readOpFlags );
   
}

unsigned int TA_ReadOpToField( TA_ReadOp readOp )
{
  if( TA_IS_REAL_CMD(readOp) )
  {
     switch( TA_GET_IDX(readOp) )
     {
     case TA_CLOSE_IDX:
        return TA_CLOSE;
     case TA_OPEN_IDX:
        return TA_OPEN;
     case TA_HIGH_IDX:
        return TA_HIGH;
     case TA_LOW_IDX:
        return TA_LOW;
     }
  }
  else if( TA_IS_INTEGER_CMD(readOp) )
  {
     switch( TA_GET_IDX(readOp) )
     {
     case TA_VOLUME_IDX:
        return TA_VOLUME;
     case TA_OPENINTEREST_IDX:
        return TA_OPENINTEREST;
     default:
        return TA_TIMESTAMP;
     }
  }

  TA_FATAL_RET( NULL, readOp, 0, TA_ALL );

  /* Unreachable code:
   * return TA_ALL;
   */
}

/**** Local functions definitions.     ****/

static TA_RetCode buildReadOp( TA_ReadOpInfo *readOpInfo,
                               const char *localBuf,
                               TA_ReadOp *readOp,
                               TA_TokenId *tokenId,
                               unsigned int *intraDayIncrementInSeconds )
{
   TA_PROLOG
   TA_TokenId id;
   TA_ReadOp tmpReadOp;
   TA_RetCode retCode;
   TA_Integer optionalParam;
   unsigned int mult, intraDayIncrement;

   TA_TRACE_BEGIN(  buildReadOp );

   if( !readOp || !intraDayIncrementInSeconds || !tokenId )
   {
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(9) );
   }

   *intraDayIncrementInSeconds = 0;
   *readOp = 0;
   *tokenId = 0;

   retCode = findTokenId( localBuf, &id, &optionalParam );

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( id != TA_TOK_END );

   /* Trap special token not generating a TA_ReadOp. */
   if( id == TA_TOK_SKIP_N_HEADER_LINE )
   {
      if( optionalParam == 0 )
      {
         TA_TRACE_RETURN( TA_INVALID_FIELD );
      }
      readOpInfo->nbHeaderLineToSkip = optionalParam;
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   if( id == TA_TOK_SKIP_NON_DIGIT_LINE )
   {
      if( optionalParam != 1 )
      {
         TA_TRACE_RETURN( TA_INVALID_FIELD );
      }
      TA_TRACE_RETURN( TA_SUCCESS );
   }

   /* Integer or Real operation? */
   switch( id )
   {
   case TA_TOK_OPEN:
   case TA_TOK_HIGH:
   case TA_TOK_LOW:
   case TA_TOK_CLOSE:
   case TA_TOK_SKIP_N_REAL:
      tmpReadOp = TA_CMD_READ_REAL;
      break;
   case TA_TOK_YYYY:
   case TA_TOK_YY:
   case TA_TOK_Y:
   case TA_TOK_M:
   case TA_TOK_MM:
   case TA_TOK_MMM:
   case TA_TOK_D:
   case TA_TOK_DD:
   case TA_TOK_VOLUME:
   case TA_TOK_OPENINTEREST:
   case TA_TOK_HOUR:
   case TA_TOK_MIN:
   case TA_TOK_SEC:
   case TA_TOK_HH:
   case TA_TOK_MN:
   case TA_TOK_SS:
   case TA_TOK_SKIP_N_INTEGER:
      tmpReadOp = TA_CMD_READ_INTEGER;
      break;
   case TA_TOK_SKIP_N_CHAR:
   case TA_TOK_SKIP_NON_DIGIT_LINE:
      tmpReadOp = 0;
      break;
   default:
      TA_TRACE_RETURN( TA_INVALID_FIELD );
   }

   /* Is this a permanent skip operation? */
   switch( id )
   {
   case TA_TOK_SKIP_N_CHAR:
   case TA_TOK_SKIP_N_INTEGER:
   case TA_TOK_SKIP_N_REAL:
      TA_SET_PERMANENT_SKIP_FLAG( tmpReadOp );
      TA_SET_SKIP_FLAG( tmpReadOp );
      break;
   default:
      TA_CLR_PERMANENT_SKIP_FLAG( tmpReadOp );
      TA_CLR_SKIP_FLAG( tmpReadOp );
   }

   /* Set the "numeric" parameter */
   switch( id )
   {
   case TA_TOK_SKIP_N_INTEGER:
   case TA_TOK_SKIP_N_REAL:
   case TA_TOK_SKIP_N_CHAR:
      TA_SET_NB_NUMERIC( tmpReadOp, optionalParam );
      break;
   case TA_TOK_OPENINTEREST:
      readOpInfo->openInterestMult = optionalParam;
      TA_SET_NB_NUMERIC( tmpReadOp, 0 );
      break;
   case TA_TOK_VOLUME:
      readOpInfo->volumeMult = optionalParam;
      TA_SET_NB_NUMERIC( tmpReadOp, 0 );
      break;
   default:
      TA_SET_NB_NUMERIC( tmpReadOp, TA_TokenMaxSize(id) );
      break;
   }

   /* Set the index information. */
   switch( id )
   {
   case TA_TOK_YYYY:
   case TA_TOK_YY:
   case TA_TOK_Y:
      TA_SET_IDX( tmpReadOp, TA_YEAR_IDX );
      break;
   case TA_TOK_M:
   case TA_TOK_MM:
   case TA_TOK_MMM:
      TA_SET_IDX( tmpReadOp, TA_MONTH_IDX );
      break;
   case TA_TOK_D:
   case TA_TOK_DD:
      TA_SET_IDX( tmpReadOp, TA_DAY_IDX );
      break;
   case TA_TOK_OPEN:
      TA_SET_IDX( tmpReadOp, TA_OPEN_IDX );
      break;
   case TA_TOK_HIGH:
      TA_SET_IDX( tmpReadOp, TA_HIGH_IDX );
      break;
   case TA_TOK_LOW:
      TA_SET_IDX( tmpReadOp, TA_LOW_IDX );
      break;
   case TA_TOK_CLOSE:
      TA_SET_IDX( tmpReadOp, TA_CLOSE_IDX );
      break;
   case TA_TOK_VOLUME:
      TA_SET_IDX( tmpReadOp, TA_VOLUME_IDX );
      break;
   case TA_TOK_OPENINTEREST:
      TA_SET_IDX( tmpReadOp, TA_OPENINTEREST_IDX );
      break;
   case TA_TOK_HOUR:
   case TA_TOK_HH:
      TA_SET_IDX( tmpReadOp, TA_HOUR_IDX );
      break;
   case TA_TOK_MIN:
   case TA_TOK_MN:
      TA_SET_IDX( tmpReadOp, TA_MIN_IDX );
      break;
   case TA_TOK_SEC:
   case TA_TOK_SS:
      TA_SET_IDX( tmpReadOp, TA_SEC_IDX );
      break;
   default:
      /* Do nothing. */
      break;
   }

   /* Set a special flag for the TA_TOK_MMM because
    * it must do chat to integer processing.
    */
   if( id == TA_TOK_MMM )
      tmpReadOp |= TA_CMD_READ_MONTH_CHAR;

   /* Identify the time increments. */
   mult = 1;
   intraDayIncrement = 0;

   switch( id )
   {
   case TA_TOK_HOUR:
   case TA_TOK_HH:
      mult *= 60;
   case TA_TOK_MIN:
   case TA_TOK_MN:
      mult *= 60;
   case TA_TOK_SEC:
   case TA_TOK_SS:
      if( optionalParam < 1 )
      {
         /* Shall be at least '1'... */
         TA_TRACE_RETURN( TA_INVALID_FIELD );
      }
      else if( optionalParam > 1 )
      {
         /* When the default '1' is specified, do
          * not return the increment to the caller.
          */
         intraDayIncrement = optionalParam * mult;
      }
      break;
   default:
      /* Do nothing */
      break;
   }

   /* Everything is fine, return the info to the caller. */
   *readOp = tmpReadOp;
   *intraDayIncrementInSeconds = intraDayIncrement;
   *tokenId = id;

   TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode findTokenId( const char *str,
                               TA_TokenId *id,
                               TA_Integer *optionalParam )
{
   TA_PROLOG
   unsigned int i;
   const char *cmp_str;
   TA_TokenId tokenId;
   unsigned int extractedParam;
   TA_RetCode retCode;

   TA_TRACE_BEGIN(  findTokenId );

   *id = TA_TOK_END;
   *optionalParam = 0;

   tokenId = TA_TOK_END;
   extractedParam = 0;

   /* First check for token directly mapping to a simple string. */
   for( i=0; (i < TA_NB_TOKEN_ID) && (tokenId == TA_TOK_END); i++ )
   {
      cmp_str = TA_TokenString( (TA_TokenId)i );

      if( cmp_str )
      {
          #if defined( WIN32 )
          if( stricmp( str, cmp_str ) == 0 )
          #else
          if( strcasecmp( str, cmp_str ) == 0 )
          #endif
          {
             tokenId = (TA_TokenId)i;
             extractedParam = 1;
          }
      }
   }

   /* If not found, look for more advanced token taking
    * optional "=n" parameters.
    */
   if( tokenId == TA_TOK_END )
   {
      retCode = findTokenIdWithParam( &str[0], &tokenId, &extractedParam );

      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN( retCode );
      }
   }

   /* Make sure it is a valid field for a file description. */
   switch( tokenId )
   {
   case TA_TOK_YYYY:
   case TA_TOK_YY:
   case TA_TOK_Y:
   case TA_TOK_M:
   case TA_TOK_MM:
   case TA_TOK_MMM:
   case TA_TOK_D:
   case TA_TOK_DD:
   case TA_TOK_OPEN:
   case TA_TOK_HIGH:
   case TA_TOK_LOW:
   case TA_TOK_CLOSE:
   case TA_TOK_VOLUME:
   case TA_TOK_OPENINTEREST:
   case TA_TOK_HOUR:
   case TA_TOK_MIN:
   case TA_TOK_SEC:
   case TA_TOK_HH:
   case TA_TOK_MN:
   case TA_TOK_SS:
   case TA_TOK_SKIP_N_CHAR:
   case TA_TOK_SKIP_N_HEADER_LINE:
   case TA_TOK_SKIP_N_REAL:
   case TA_TOK_SKIP_N_INTEGER:
   case TA_TOK_SKIP_NON_DIGIT_LINE:
      break;
   default:
      TA_TRACE_RETURN( TA_INVALID_FIELD );
   }

   /* Everything is fine, return the information. */
   *id = tokenId;
   *optionalParam = extractedParam;

   TA_TRACE_RETURN( TA_SUCCESS );
}


static TA_RetCode findTokenIdWithParam( const char *str,
                                        TA_TokenId *tokenId,
                                        unsigned int *extractedParam )
{
   unsigned int i, length;
   const char *cmp_str;

   *tokenId = TA_TOK_END;
   length = 0;

   /* Identify the token.  */
   for( i=0; (i < TA_NB_TOKEN_ID) && (*tokenId == TA_TOK_END); i++ )
   {
      cmp_str = TA_TokenString( (TA_TokenId)i );

      if( cmp_str )
      {
          length = strlen(cmp_str);

          #if defined( WIN32 )
          if( strnicmp( str, cmp_str, length ) == 0 )
          #else
          if( strncasecmp( str, cmp_str, length ) == 0 )
          #endif
          {
             if( (str[length] == '=') && isdigit(str[length+1])) 
             {
                *tokenId = (TA_TokenId)i;
             }
         }
      }
   }

   /* Make sure this is a field that can have a parameter. */
   switch( *tokenId )
   {
   case TA_TOK_HOUR:
   case TA_TOK_MIN:
   case TA_TOK_SEC:
   case TA_TOK_HH:
   case TA_TOK_MN:
   case TA_TOK_SS:
   case TA_TOK_SKIP_N_CHAR:
   case TA_TOK_SKIP_N_REAL:
   case TA_TOK_SKIP_N_INTEGER:
   case TA_TOK_SKIP_N_HEADER_LINE:
   case TA_TOK_VOLUME:
   case TA_TOK_OPENINTEREST:
      break;
   default:
      return TA_INVALID_FIELD;
   }

   /* Make sure the "= n" is specified. */
   if( (str[length] != '=') ||
       (!isdigit( str[length+1] )) ) 
      return TA_INVALID_FIELD;

   *extractedParam = atoi( &str[length+1] );

   /* Validate some ranges. */
   switch( *tokenId )
   {
   case TA_TOK_HOUR:
   case TA_TOK_HH:
      switch( *extractedParam )
      {
      case 1: case 2: case 3: case 4: case 12:
         /* All these are valid values. */
         break;
      default:
         return TA_INVALID_FIELD;
      }
      break;
   case TA_TOK_MN:
   case TA_TOK_SS:
   case TA_TOK_MIN:
   case TA_TOK_SEC:
      switch( *extractedParam )
      {
      case  1: case  2: case  3: case  4: case  5:
      case  6: case 10: case 12: case 15: case 20:
      case 30:
         /* All these are valid values. */
         break;
      default:
         return TA_INVALID_FIELD;
      }
      break;
   case TA_TOK_VOLUME:
   case TA_TOK_OPENINTEREST:
      switch( *extractedParam )
      {
      case     1:
      case    10:
      case   100: 
      case  1000:
      case 10000:
         /* All these are valid values. */
         break;
      default:
         return TA_INVALID_FIELD;
      }
   default:
      /* Do nothing */
      break;
   }

   return TA_SUCCESS;
}
