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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   First version.
 *  062203 MF   Ignore "time" component of the start/end range when
 *              there is no time fields in the file.
 *  040304 MF   Allows skipping lines not starting with a digit.
 */

/* Description:
 *    Read sequentially an ASCII file and extract the information needed
 *    for building a price bar.
 *
 *    This function is coded for speed optimizing the most common and
 *    probable operations. One of the goal is to keep the code and data
 *    localize. This may help to keep the code in the CPU cache while
 *    processing a large ASCII file.
 *
 *    Macro are also used for redundant logic to avoid costly
 *    function calls.
 */

/**** Headers ****/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ta_ascii_handle.h"
#include "ta_readop.h"

#include "ta_memory.h"
#include "ta_trace.h"

#ifdef WIN32
#pragma warning( disable : 4127 ) /* Disable 'conditional expression is constant' */
#endif

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
TA_FILE_INFO;

/**** Local functions declarations.    ****/
static unsigned isTimeNeeded( const TA_ReadOp *readOp );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/

/* #define DEBUG_PRINTF */

#define GET_CHAR \
{ \
   if( nbByteRead == 0 ) \
      car = TA_FileSeqRead( fileHandle, &nbByteRead ); \
   else \
      ++car; \
   --nbByteRead; \
   nbTotalByteDone++; \
}

#define CHECK_EOL_EOF \
{ \
   if( car == NULL ) goto exit_loops; \
   if( *car == '\n' ) goto line_loop; \
}

#define SKIP_UNTIL_NUMERIC \
{ \
   while( !isdigit(*car) ) \
   { \
      GET_CHAR; \
      CHECK_EOL_EOF; \
   } \
}

#define SKIP_NUMERIC \
{ \
   while( isdigit(*car) ) \
   { \
      GET_CHAR; \
      CHECK_EOL_EOF; \
   } \
}

#define CNVT_ARRAY_SIZE 40
#define READ_IN_CNVT_ARRAY \
{ \
   do \
   { \
      cnvtArray[cnvtArrayIdx++] = *car; \
      GET_CHAR; \
   }while( (car != NULL) && isdigit(*car) && (cnvtArrayIdx < CNVT_ARRAY_SIZE) ); \
}

#define READ_N_CHAR_IN_CNVT_ARRAY(n) \
{ \
   TA_ASSERT( n > 0 ); \
   do \
   { \
      cnvtArray[cnvtArrayIdx++] = *car; \
      GET_CHAR; \
      CHECK_EOL_EOF; \
   }while( --n && (car != NULL) && isdigit(*car) ); \
   if( n != 0 ) \
   { \
      retCode = TA_MISSING_INPUT_DIGITS; \
      goto exit_loops; \
   } \
}

#define SKIP_LINE \
{ \
   while(1) \
   { \
      CHECK_EOL_EOF; \
      GET_CHAR; \
   } \
}

#define REVERT_OPERATIONS \
{ \
   for( tmpInt=0; tmpInt < curOp; tmpInt++ ) \
   { \
      op = readOpInfo->arrayReadOp[tmpInt]; \
      if( !TA_IS_SKIP_SET(op) ) \
      { \
         tmpIdx = TA_GET_IDX(op); \
         if( TA_IS_REAL_CMD(op) ) \
            arrayReal[ tmpIdx ]--; \
         else if( tmpIdx > TA_YEAR_IDX )\
            arrayInteger[ tmpIdx ]--; \
      } \
   } \
}

TA_RetCode TA_ReadOp_Do( TA_FileHandle       *fileHandle,
                         const TA_ReadOpInfo *readOpInfo,
                         TA_Period            period,
                         const TA_Timestamp  *start,
                         const TA_Timestamp  *end,
                         unsigned int         minimumNbBar,
                         TA_Field             fieldToAlloc,
                         TA_ParamForAddData  *paramForAddData,
                         unsigned int        *nbBarAdded )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_EstimateInfo estimationInfo;

   unsigned int nbElementToAllocate;
   unsigned int memoryNeeded; /* Boolean */
   unsigned int timeNeeded;   /* Boolean */

   TA_Real    *arrayReal[TA_REAL_ARRAY_SIZE];
   TA_Integer *arrayInteger[TA_INTEGER_ARRAY_SIZE];
   TA_Timestamp *timestamp;

   TA_Real *openBeg, *highBeg, *lowBeg, *closeBeg;
   TA_Integer *volumeBeg, *openInterestBeg;
   TA_Timestamp *timestampBeg;
   TA_Timestamp tmpTimestamp;

   TA_ReadOp op;

   TA_Field fieldToProcess;
   unsigned int year, month, day, hour, min, sec;
   TA_Integer curOp;

   unsigned int nbTotalByteDone, nbTotalBarDone;
   unsigned int nbBarAddedInTheBlock;

   char monthChar[4];
   char cnvtArray[CNVT_ARRAY_SIZE];
   unsigned int  cnvtArrayIdx;

   unsigned int nbByteToAllocReal;
   unsigned int nbByteToAllocInteger;

   unsigned int fileSize;
   unsigned int skipField;
   unsigned int lineToSkip;

   unsigned int nbByteRead;
   unsigned int nbLetter;

   TA_Real lastValidClose;

   const char *car;

   register TA_Real tmpReal;
   register TA_Integer tmpInt;
   register unsigned int tmpIdx;
   register unsigned int nbCharToRead;

   unsigned int lastOpFieldIncremented;
   TA_TRACE_BEGIN( TA_PriceBarRead );

   /* Initialization of local variables. */
   openBeg = highBeg = lowBeg = closeBeg = NULL;
   timestampBeg = NULL;
   volumeBeg = openInterestBeg = NULL;
   timestamp = NULL;
   retCode = TA_SUCCESS;
   lastValidClose = 0.0;

   fieldToProcess = readOpInfo->fieldProvided & fieldToAlloc;
   if( (fieldToProcess & fieldToAlloc) != fieldToAlloc )
   {
      /* Nothing to read because not all the requested
       * fields are provided by this data source!
       */
      return TA_SUCCESS;
   }

   /* Estimate the initial amount of memory to allocate. */
   fileSize = TA_FileSize( fileHandle );
   retCode = TA_EstimateAllocInit( start, end, period,                                   
                                   minimumNbBar, 2048,
                                   &estimationInfo,
                                   &nbElementToAllocate );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   if( nbElementToAllocate == 0 )
   {
      TA_TRACE_RETURN( TA_SUCCESS ); /* Nothing to read!? Just return... */
   }

   memset( arrayInteger, 0, sizeof( arrayInteger ) );
   memset( arrayReal,    0, sizeof( arrayReal ) );

   /* Set the date/time pointers to where the information will be stored. */
   arrayInteger[TA_HOUR_IDX]  = (TA_Integer *)&hour;
   arrayInteger[TA_MIN_IDX]   = (TA_Integer *)&min;
   arrayInteger[TA_SEC_IDX]   = (TA_Integer *)&sec;
   arrayInteger[TA_MONTH_IDX] = (TA_Integer *)&month;
   arrayInteger[TA_YEAR_IDX]  = (TA_Integer *)&year;
   arrayInteger[TA_DAY_IDX]   = (TA_Integer *)&day;

   /* Set default time/date. */
   year  = 1900;
   month = day = 1;
   hour  = min = sec = 0;

   /* Check if the processing of the time will be needed. */
   timeNeeded = isTimeNeeded( readOpInfo->arrayReadOp);

   /* 'car' always point to the character being currently handled. */
   nbByteRead = 0;
   car = TA_FileSeqRead( fileHandle, &nbByteRead );
   if( (car == NULL) || (nbByteRead == 0) )
      return TA_SUCCESS; /* End of file! */
   --nbByteRead;

   nbTotalByteDone = 0;
   nbTotalBarDone  = 0;
   nbBarAddedInTheBlock = 0;
   memoryNeeded    = 1;
   curOp           = 0;
   skipField       = 0;

   monthChar[3] = '\0';

   /* When requested, skip header lines. */
   lineToSkip = readOpInfo->nbHeaderLineToSkip;
   while( lineToSkip-- )
   {
      while( *car != '\n' )
      {
         GET_CHAR;
         if( car == NULL )
            goto exit_loops;
      }
   }

line_loop: /* Always jump here when end-of-line is found (EOL). */

      /* Absorb empty lines. */
      while( *car == '\n' )
      {
         GET_CHAR;
         CHECK_EOL_EOF;
      }

      /* Skip lines not starting with a digit (if format wants such skip). */
      if( !isdigit(*car) )
      {
         op = readOpInfo->arrayReadOp[0];
         if( TA_IS_SKIP_NDL_FLAG(op) )
         {
           while(1)
           {
              GET_CHAR;
              CHECK_EOL_EOF;
           }
         }
      }

      /* If curOp != 0, the last operations are canceled. */
      REVERT_OPERATIONS;
      curOp = 0;
      lastOpFieldIncremented = 0;

      /* Start over a new line. */

      if( memoryNeeded )
      {
         /* Allocate the memory. */
         nbByteToAllocReal = nbElementToAllocate * sizeof( TA_Real );
         nbByteToAllocInteger = nbElementToAllocate * sizeof( TA_Integer );

         timestamp = (TA_Timestamp *)TA_Malloc( nbElementToAllocate * sizeof( TA_Timestamp ) );
         timestampBeg = timestamp;

         if( !timestampBeg )
         {
            retCode = TA_ALLOC_ERR;
            goto exit_loops;
         }

         #define TA_ALLOC_MEM(upperc,lowerc,typepar) \
         { \
            if( fieldToProcess & TA_##upperc ) \
            { \
               lowerc##Beg = (TA_##typepar *)TA_Malloc( nbByteToAlloc##typepar ); \
               array##typepar[TA_##upperc##_IDX] = lowerc##Beg; \
               if( !lowerc##Beg ) \
               { \
                  retCode = TA_ALLOC_ERR; \
                  goto exit_loops; \
               } \
            } \
         }
         TA_ALLOC_MEM( OPEN,  open,  Real );
         TA_ALLOC_MEM( HIGH,  high,  Real );
         TA_ALLOC_MEM( LOW,   low,   Real );
         TA_ALLOC_MEM( CLOSE, close, Real );

         TA_ALLOC_MEM( VOLUME, volume, Integer );
         TA_ALLOC_MEM( OPENINTEREST, openInterest, Integer );
         #undef TA_ALLOC_MEM

         memoryNeeded = 0;
      }

op_loop: /* Jump here when ready to proceed with the next command. */

      op = readOpInfo->arrayReadOp[curOp];

      if( !(op&TA_CMD_READ_MONTH_CHAR) )
      {
         /* Skip leading non-numeric character. */
         SKIP_UNTIL_NUMERIC;
      }

      /* Shall we skip this field? */
      if( TA_IS_SKIP_SET(op) )
      {
         if( (op&(TA_CMD_READ_REAL|TA_CMD_READ_INTEGER)) == 0 )
         {
            tmpInt = TA_GET_NB_NUMERIC(op);
            curOp++;
            while( tmpInt-- )
            {
               GET_CHAR;
               CHECK_EOL_EOF;
            }
         }
         else
         {
            if( skipField == 0 )
            {
               skipField = TA_GET_NB_NUMERIC(op);
               TA_ASSERT( skipField > 0 );
            }

            if( --skipField == 0 )
               curOp++;

            SKIP_NUMERIC;
            if( (op&TA_CMD_READ_REAL) && (*car == '.') )
            {
               GET_CHAR;
               CHECK_EOL_EOF;
               SKIP_NUMERIC;
            }
         }
      }
      else
      {
         cnvtArrayIdx = 0;

         if( TA_IS_REAL_CMD(op) )
         {
            /* Extract a numeric into cnvtArray. */
            READ_IN_CNVT_ARRAY;

            /* This is a TA_Real. */
            if( car && (*car == '.') )
            {
               /* Read rest of the float after the '.' */
               READ_IN_CNVT_ARRAY;
            }
            cnvtArray[cnvtArrayIdx] = '\0';
            tmpReal = atof( &cnvtArray[0] );

            /* Write the TA_Real in memory. */
            tmpIdx = TA_GET_IDX(op);
            TA_ASSERT( tmpIdx < TA_REAL_ARRAY_SIZE );
            TA_ASSERT( arrayReal[tmpIdx] != NULL );
            if( tmpReal != 0.0 )
            {
               *(arrayReal[tmpIdx]) = tmpReal;
               if( tmpIdx == TA_CLOSE_IDX )
                  lastValidClose = tmpReal;
            }
            else if( TA_IS_REPLACE_ZERO(op) )
            {
               /* Replace this zero value with the last known close.
                * If there is no previous close, this line is ignored.
                */
               if( lastValidClose != 0.0 )
                  *(arrayReal[tmpIdx]) = lastValidClose;
               else
               {
                  SKIP_LINE;
               }
            }
            else
            {
               /* Zero are not expected, consider this as a failure
                * and ignore all further data from this file.
                */
               retCode = TA_PRICE_BAR_CONTAINS_ZERO;
               goto exit_loops;
            }

            arrayReal[tmpIdx]++;
            curOp++;
         }
         else
         {
            /* This is a TA_Integer. */
            if( !(op&TA_CMD_READ_MONTH_CHAR) )
            {
               nbCharToRead = TA_GET_NB_NUMERIC(op);
               if( nbCharToRead )
               {
                  READ_N_CHAR_IN_CNVT_ARRAY(nbCharToRead);
               }
               else
               {
                  READ_IN_CNVT_ARRAY;
               }

               cnvtArray[cnvtArrayIdx] = '\0';
               tmpInt = atoi( &cnvtArray[0] );
            }
            else
            {
               /* Try to find a 3 letters month string. 
                * Translate it to a [1..12] integer.
                */
               nbLetter = 1;
               do
               {
                  CHECK_EOL_EOF;
                  monthChar[nbLetter] = (char)toupper(*car);
                  GET_CHAR;
                  nbLetter++;
               } while( nbLetter != 3 );
               
               do
               {
                  CHECK_EOL_EOF;
                  monthChar[0] = monthChar[1];
                  monthChar[1] = monthChar[2];
                  monthChar[2] = (char)toupper(*car);
                  if( strncmp("JAN",monthChar,3) == 0 )
                     tmpInt = 1;
                  else if( strncmp("FEB",monthChar,3) == 0 )
                     tmpInt = 2;
                  else if( strncmp("MAR",monthChar,3) == 0 )
                     tmpInt = 3;
                  else if( strncmp("APR",monthChar,3) == 0 )
                     tmpInt = 4;
                  else if( strncmp("MAY",monthChar,3) == 0 )
                     tmpInt = 5;
                  else if( strncmp("JUN",monthChar,3) == 0 )
                     tmpInt = 6;
                  else if( strncmp("JUL",monthChar,3) == 0 )
                     tmpInt = 7;
                  else if( strncmp("AUG",monthChar,3) == 0 )
                     tmpInt = 8;
                  else if( strncmp("SEP",monthChar,3) == 0 )
                     tmpInt = 9;
                  else if( strncmp("OCT",monthChar,3) == 0 )
                     tmpInt = 10;
                  else if( strncmp("NOV",monthChar,3) == 0 )
                     tmpInt = 11;
                  else if( strncmp("DEC",monthChar,3) == 0 )
                     tmpInt = 12;
                  else
                     tmpInt = 0;

                  GET_CHAR;
               } while( tmpInt == 0 );
            }

            /* Write the TA_Integer in memory. */
            tmpIdx = TA_GET_IDX(op);
            TA_ASSERT( tmpIdx < TA_INTEGER_ARRAY_SIZE );
            TA_ASSERT( arrayInteger[tmpIdx] != NULL );
            *(arrayInteger[tmpIdx]) = tmpInt;

            if( tmpIdx > TA_YEAR_IDX )
               arrayInteger[tmpIdx]++;
            curOp++;

            if( TA_IS_TIMESTAMP_COMPLETE(op) )
            {
               /* Build the timestamp. */
               retCode = TA_SetDate( year, month, day, &tmpTimestamp );
               if( retCode != TA_SUCCESS )
                  goto exit_loops; /* Invalid date */

               if( !timeNeeded )
               {
                  /* Ignore time in comparison and use default to build the price bar. */
                  tmpTimestamp.time = 23595900; /* Default EOD time */
                  if( start && (tmpTimestamp.date < start->date) )
                  {
                     /* This price bar is not needed, jump to the next line. */
                     retCode = TA_SUCCESS;
                     SKIP_LINE;
                  }

                  if( end && (tmpTimestamp.date > end->date) )
                  {
                     /* This price bar is beyond the upper limit, just exit. */
                     goto exit_loops;
                  }
               }
               else
               {
                  retCode = TA_SetTime( hour, min, sec, &tmpTimestamp );
                  if( retCode != TA_SUCCESS )
                     goto exit_loops; /* Invalid time */
                  if( start && TA_TimestampLess(&tmpTimestamp,start) )
                  {
                     /* This price bar is not needed, jump to the next line. */
                     retCode = TA_SUCCESS;
                     SKIP_LINE;
                  }

                  if( end && TA_TimestampGreater(&tmpTimestamp, end) )
                  {
                     /* This price bar is beyond the upper limit, just exit. */
                     goto exit_loops;
                  }
               }

               /* Write the timestamp in memory. */
               *timestamp = tmpTimestamp;
            }
         }

         if( TA_IS_READ_STOP_FLAG_SET(op) )
         {
            #ifdef DEBUG_PRINTF
               printf( "(%d%d%d,%e,%e,%e,%e,%d)\n",
                  timestampBeg?TA_GetYear(&timestampBeg[nbBarAddedInTheBlock]):0,
                  timestampBeg?TA_GetMonth(&timestampBeg[nbBarAddedInTheBlock]):0,
                  timestampBeg?TA_GetDay(&timestampBeg[nbBarAddedInTheBlock]):0,
                  openBeg?openBeg[nbBarAddedInTheBlock]:0.0,
                  highBeg?highBeg[nbBarAddedInTheBlock]:0.0,
                  lowBeg?lowBeg[nbBarAddedInTheBlock]:0.0,
                  closeBeg?closeBeg[nbBarAddedInTheBlock]:0.0,
                  volumeBeg?volumeBeg[nbBarAddedInTheBlock]:0 );
            #endif

            /* At this point, the price bar is completely written in memory. */
            timestamp++;
            curOp = 0;
            nbBarAddedInTheBlock++;
            if( nbBarAddedInTheBlock < nbElementToAllocate )
            {
              /* Go to next line. */
              SKIP_LINE;
            }
            else
            {
               /* There is not enough memory for another bar, so re-allocate
                * some more.
                */
               retCode = TA_HistoryAddData( paramForAddData, nbBarAddedInTheBlock,
                                            period, timestampBeg,
                                            openBeg, highBeg, lowBeg, closeBeg,
                                            volumeBeg, openInterestBeg );

               /* TA_HistoryAddData is ALWAYS the owner of these memory block
                * when called. So we set these to NULL to make sure there will
                * be no attempt to free these from this function.
                */
               openBeg = highBeg = lowBeg = closeBeg = NULL;
               timestampBeg = NULL;
               volumeBeg = openInterestBeg = NULL;
               nbTotalBarDone += nbBarAddedInTheBlock;
               nbBarAddedInTheBlock = 0;

               if( retCode != TA_SUCCESS )
                  goto exit_loops;

               retCode = TA_EstimateAllocNext( &estimationInfo, &nbElementToAllocate );
               if( retCode != TA_SUCCESS )
                  goto exit_loops;

               memoryNeeded = 1;
               SKIP_LINE;
            }
         }
         else
         {
            /* Make sure we did not hit an EOL or EOF prematurly, if yes,
             * this line will be silently ignored.
             */
            CHECK_EOL_EOF;
         }
      }

      goto op_loop;

exit_loops: /* Jump here when the end-of-file is hit (or an error occured) */

   /* On succesful exit, process possibly remaining data. */
   if( (retCode == TA_SUCCESS) && (nbBarAddedInTheBlock != 0) )
   {
      retCode = TA_HistoryAddData( paramForAddData, nbBarAddedInTheBlock,
                                   period, timestampBeg,
                                   openBeg, highBeg, lowBeg, closeBeg,
                                   volumeBeg, openInterestBeg );

      openBeg = highBeg = lowBeg = closeBeg = NULL;
      timestampBeg = NULL;
      volumeBeg = openInterestBeg = NULL;
      nbTotalBarDone += nbBarAddedInTheBlock;
   }
   
   /* ALWAYS verify if locally allocated memory needs to be freed. ALWAYS. */
   FREE_IF_NOT_NULL( openBeg );
   FREE_IF_NOT_NULL( highBeg );
   FREE_IF_NOT_NULL( lowBeg );
   FREE_IF_NOT_NULL( closeBeg );
   FREE_IF_NOT_NULL( volumeBeg );
   FREE_IF_NOT_NULL( openInterestBeg );
   FREE_IF_NOT_NULL( timestampBeg );

   /* An indication that no more data needs to be provided is not a failure. */
   if( retCode == TA_ENOUGH_DATA )
      retCode = TA_SUCCESS;

   /* Return the number of added price bar to the caller. */
   if( nbBarAdded )
      *nbBarAdded = nbTotalBarDone;

   TA_TRACE_RETURN( retCode );
}

/**** Local functions definitions.     ****/
static unsigned isTimeNeeded( const TA_ReadOp *readOp )
{
   unsigned int i;
   TA_ReadOp op;

   i=0;
   do
   {
     op = readOp[i++];

     if( TA_IS_INTEGER_CMD(op) && (TA_GET_IDX(op)<=TA_HOUR_IDX) )
        return 1;
   } while( !TA_IS_READ_STOP_FLAG_SET(op) );

   return 0;
}

#ifdef WIN32
#pragma warning( default : 4127 ) /* Restore warnings. */
#endif

