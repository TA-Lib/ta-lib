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
 *  070701 MF   First version.
 *  041802 MF   Add better retry/timeout mechanism.
 *
 */

/* Description:
 *    Get historical data from the Yahoo! Web Site. 
 */

/* #define DEBUG_PRINTF 1 */

/**** Headers ****/
#include <stdlib.h>
#include <string.h>
#include "ta_yahoo_priv.h"
#include "ta_yahoo_handle.h"
#include "ta_yahoo_idx.h"
#include "ta_trace.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   /* Maximum length when concatenating all 
    * the "part" and assuming that the date
    * use no more than 8 characters each.
    */
   unsigned int maxTotalLength;

   /* Divide the UIR suffix in a such way that
    * it is easy to change some fields within it.
    */

   const char *part1;
   unsigned int part1Length;
   /* &a=<dd> start day inserted here */
   /* &b=<mm> start month inserted here */
   /* &c=<yyyy> start year inserted here */
   const char *part2;
   unsigned int part2Length;
   /* &d=<dd> end day inserted here */
   /* &e=<mm> end month inserted here */
   /* &f=<yyyy> end year inserted here */
   const char *part3;
   unsigned int part3Length;
} UIRSuffixParsing;


/**** Local functions declarations.    ****/
static unsigned int nbCommaField( TA_Stream *csvFile );

static int setUIRSuffixParsing( const char *uirSuffix,
                                UIRSuffixParsing *parseOutput );

static void buildUIRSuffix( const UIRSuffixParsing *parsedString,
                            TA_Timestamp *startTimestamp,
                            TA_Timestamp *endTimestamp,
                            char *output );

static int isGapAcceptable( TA_Timestamp *lastBarTimestampAdded,
                            TA_Timestamp *lastBarTimestamp );

static unsigned int TA_DateWithinRange( unsigned int year,
                                        unsigned int month,
                                        unsigned int day,
                                        const TA_Timestamp *t1,
                                        const TA_Timestamp *t2 );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/
TA_RetCode TA_GetHistoryDataFromWeb( TA_Libc *libHandle,
                                     TA_DataSourceHandle *handle,
                                     TA_CategoryHandle   *categoryHandle,
                                     TA_SymbolHandle     *symbolHandle,
                                     TA_Period            period,
                                     const TA_Timestamp  *start,
                                     const TA_Timestamp  *end,
                                     TA_Field             fieldToAlloc,
                                     TA_ParamForAddData  *paramForAddData )
{
   TA_PROLOG;

   TA_RetCode retCode;
   TA_StringCache *stringCache;
   TA_String *yahooName;
   TA_WebPage *webPage;
   TA_PrivateYahooHandle *yahooHandle;
   TA_DecodingParam localDecodingParam;
   const TA_DecodingParam *decodingParam;
   TA_FileHandle *fileHandle;
   TA_ReadOpInfo *readOpInfo;
   UIRSuffixParsing suffixParsing;
   TA_Timestamp firstBarTimestamp, lastBarTimestamp, prevEndDate;
   TA_InfoFromAddedData infoFromAddedData;
   TA_DayOfWeek dayOfWeek;

   int nbEstimateBar;
   int nbField;
   unsigned int nbBarAdded, nbTotalBarAdded;
   int again, firstTime, nbBatch;
   int zeroBarAddedAttempt;

   TA_TRACE_BEGIN( libHandle, TA_GetHistoryDataFromWeb );

   /* Initialize some local variables. */
   stringCache   = TA_GetGlobalStringCache( libHandle );
   yahooHandle   = (TA_PrivateYahooHandle *)handle->opaqueData;
   readOpInfo    = NULL;
   nbEstimateBar = 0;

   TA_ASSERT( libHandle, categoryHandle != NULL );
   TA_ASSERT( libHandle, symbolHandle != NULL );
   TA_ASSERT( libHandle, categoryHandle->string != NULL );
   TA_ASSERT( libHandle, symbolHandle->string != NULL );

   /* Set the initial first/last timestamp */
   if( start )
      TA_TimestampCopy( &firstBarTimestamp, start );
   else
   {
      TA_SetDate( 1950, 1, 1, &firstBarTimestamp );
      TA_SetTime( 0, 0, 0, &firstBarTimestamp );
   }

   if( end )
      TA_TimestampCopy( &lastBarTimestamp, end );
   else
   {
      TA_SetDateNow( &lastBarTimestamp );
      TA_SetTime( 0, 0, 0, &lastBarTimestamp );
   }

   /* Make sure that lastBarTimestamp is a week-day. */
   dayOfWeek = TA_GetDayOfTheWeek( &lastBarTimestamp );
   if( (dayOfWeek == TA_SUNDAY) || (dayOfWeek == TA_SATURDAY) )
      TA_PrevWeekday( &lastBarTimestamp );

   /* Map the TA-Lib name into the Yahoo! name. */
   retCode = TA_AllocStringFromLibName( libHandle,
                                        categoryHandle->string,
                                        symbolHandle->string,
                                        &yahooName );  
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   TA_ASSERT( libHandle, yahooName != NULL );
   TA_ASSERT( libHandle, yahooHandle != NULL );

   /* Get the decoding parameter for the CSV web page. */
   decodingParam = TA_YahooIdxDecodingParam( yahooHandle->index, TA_YAHOOIDX_CVS_PAGE );
   if( !decodingParam )
   {
      TA_StringFree( stringCache, yahooName );
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(103) );
   }

   /* Use a local copy of the decoding param. 
    * This is because the uirSuffix is replaced with
    * an allocated buffer (so the date field can be
    * manipulated).
    */
   localDecodingParam = *decodingParam;

   /* Parse the uirSuffix so the start/end date can be changed. */
   if( !setUIRSuffixParsing( decodingParam->uirSuffix, &suffixParsing ) )
   {
      /* This should never happen unless the
       * Yahoo! index protocol has been broken.
       */
      /* Clean-up and exit */
      TA_StringFree( stringCache, yahooName );
      TA_TRACE_RETURN( TA_INTERNAL_ERROR(104) );
   }

   /* Replace the uirSuffix with a large local buffer. */
   localDecodingParam.uirSuffix = TA_Malloc( libHandle, suffixParsing.maxTotalLength );
   if( !localDecodingParam.uirSuffix )
   {
      /* Clean-up and exit */
      TA_StringFree( stringCache, yahooName );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Change the dates in the uirSuffix. */
   buildUIRSuffix( &suffixParsing,
                   &firstBarTimestamp, &lastBarTimestamp,
                   (char *)localDecodingParam.uirSuffix );

   /* nbBatch is a safety net to make sure that
    * TA-Lib won't stay forever in the while loop
    * in case Yahoo! changes their protocol.
    */
   nbBatch = 0; 

   /* Sometime Yahoo! return an empty csv file. Make
    * multiple attempts in that case.
    */
   zeroBarAddedAttempt = 0;

   again = 1;
   firstTime = 1;
   nbTotalBarAdded = 0;
   while( again && (++nbBatch < 100) && (zeroBarAddedAttempt < 10) )
   {  
    
      if( TA_TimestampLess(&lastBarTimestamp,&firstBarTimestamp) )
      {
          /* Get out of this loop if all the requested data
           * has been retreived already.
           */
         again = 0;
         break;
      }   

      retCode = TA_WebPageAllocFromYahooName( libHandle,
                                              &localDecodingParam,
                                              TA_StringToChar(yahooName),
                                              &webPage );
           
      if( retCode != TA_SUCCESS )
      {
         TA_StringFree( stringCache, yahooName );
         TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
         TA_TRACE_RETURN( retCode );
      }

      /* Disguise the webPage stream into a "file". That way the speed
       * optimized ASCII decoder can be re-used (TA_ReadOp stuff).
       */
      retCode = TA_FileSeqOpenFromStream( libHandle, webPage->content, &fileHandle );
      if( retCode != TA_SUCCESS )
      {
         /* Clean-up and exit */
         TA_StringFree( stringCache, yahooName );
         TA_WebPageFree( webPage );
         TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
         TA_TRACE_RETURN( retCode );
      }

      if( firstTime )
      {
         /* Make assumption of the data provided
          * base on the number of fields in the CSV file.
          */
         nbField = nbCommaField( webPage->content );
         switch( nbField )
         {
         case 2:
            readOpInfo = yahooHandle->readOp2Fields;
            break;
         case 5:
            readOpInfo = yahooHandle->readOp5Fields;
            break;
         default:
            readOpInfo = yahooHandle->readOp6Fields;
         }

         /* User asking for all the fields? */
         if( fieldToAlloc == TA_ALL )
         {
            switch( nbField )
            {
            case 2:
               fieldToAlloc = TA_CLOSE|TA_TIMESTAMP;
               break;
            case 5:
               fieldToAlloc = TA_OPEN|TA_HIGH|TA_LOW|TA_CLOSE|TA_TIMESTAMP;
               break;
            default:
               fieldToAlloc = TA_OPEN|TA_HIGH|TA_LOW|TA_CLOSE|TA_VOLUME|TA_TIMESTAMP;
            }
         }

         /* Optimize the read op for the requested data. */
         retCode = TA_ReadOp_Optimize( libHandle,
                                       readOpInfo,
                                       period,
                                       fieldToAlloc );
         if( retCode != TA_SUCCESS )
         {
            /* Clean-up and exit */
            TA_StringFree( stringCache, yahooName );
            TA_WebPageFree( webPage );
            TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
            TA_TRACE_RETURN( retCode );
         }

         /* Make an estimation of the number of price bar. */
         nbEstimateBar  = TA_StreamCountChar( webPage->content, '\n' ) + 1;
         if( nbEstimateBar < 100 )
            nbEstimateBar = 100;
      }

      /* Interpret the CSV data. */
      retCode = TA_ReadOp_Do( libHandle, fileHandle,                           
                              readOpInfo,
                              period, &firstBarTimestamp, &lastBarTimestamp,
                              nbEstimateBar, fieldToAlloc,
                              paramForAddData,
                              &nbBarAdded );

      TA_FileSeqClose( libHandle, fileHandle );
      TA_WebPageFree( webPage );

      nbTotalBarAdded += nbBarAdded;

      if( retCode != TA_SUCCESS )
      {
         /* Clean-up and exit */
         TA_StringFree( stringCache, yahooName );
         TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
         TA_TRACE_RETURN( retCode );
      }

      /* Yahoo! does not always return all the data it could, up to
       * the requested end date. It is important to detect these occurence
       * and cancel the usage of all data accumulated up to now. 
       */      
      TA_GetInfoFromAddedData( paramForAddData, &infoFromAddedData );
      if( infoFromAddedData.barAddedSinceLastCall )
      {
         /* Do some more checking by considering holidays, week-end etc... */
         if( !isGapAcceptable(&infoFromAddedData.highestTimestampAddedSinceLastCall, &lastBarTimestamp) )
         {
            /* Clean-up and exit */
            TA_StringFree( stringCache, yahooName );
            TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
            TA_TRACE_RETURN( TA_DATA_GAP );
         }
         
         TA_TimestampCopy( &lastBarTimestamp, &infoFromAddedData.lowestTimestamp );
      }

      #if DEBUG_PRINTF
      printf( "NB BAR ADDED=%d, TOTAL=%d\n", nbBarAdded, nbTotalBarAdded );
      #endif

      /* Verify if more data should be processed. 
       * Yahoo! sometimes slice their data, in 
       * batch of 200 price bars. 
       */
      if( firstTime && (nbBarAdded > 200) )
      {
         again = 0; /* Assume all the data extracted... exit the loop. */
      }
      else if( nbBarAdded == 0 )
      {
         /* Make multiple attempts when retreiving data succeed,
          * but somehow there is zero bar returned. 
          *
          * Sometimes this might be correct when there is truly no
          * more data available, so choosing an algorithm before
          * giving up is a comprimise between reliability and
          * usability. The data source is free... and you get
          * what you pay for after all ;)
          */
         if( (nbTotalBarAdded < 1000) && (zeroBarAddedAttempt >= 1) && (zeroBarAddedAttempt < 7) )
         {
            /* I did choose to add a delay when insufficient total data is returned. When
             * there is already ~5 years of data, most likely there is "Zero" returned
             * because there is NO more data available, so just do the retry without delay.
             */
            TA_Sleep(zeroBarAddedAttempt*2);
         }

         #if DEBUG_PRINTF
         printf( "Retry %d", zeroBarAddedAttempt );
         #endif

         zeroBarAddedAttempt++;
      }
      else
      {
         zeroBarAddedAttempt = 0;

         if( TA_TimestampEqual( &lastBarTimestamp, &prevEndDate ) )
         {
            /* prevEndDate is a "safety net" to
             * exit the loop early in case Yahoo! starts
             * to return always the same batch of data.
             * Just ignore the repetitive data and exit.
             */
            TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
            TA_StringFree( stringCache, yahooName );
            TA_TRACE_RETURN( TA_SUCCESS );
         }
         TA_TimestampCopy( &prevEndDate, &lastBarTimestamp );

         /* Request the data up to the day BEFORE
          * the last batch of data received.
          */
         TA_PrevDay( &lastBarTimestamp );

         /* Make sure that lastBarTimestamp is a week-day. */
         dayOfWeek = TA_GetDayOfTheWeek( &lastBarTimestamp );
         if( (dayOfWeek == TA_SUNDAY) || (dayOfWeek == TA_SATURDAY) )
            TA_PrevWeekday( &lastBarTimestamp );

         /* Change the dates in the uirSuffix. */
         buildUIRSuffix( &suffixParsing,
                         &firstBarTimestamp, &lastBarTimestamp,
                         (char *)localDecodingParam.uirSuffix );

         /* From that point, data is expected to be most likely
          * sent in batch of 200.
          */
         nbEstimateBar = 200;
      }

      firstTime = 0;
   }

   /* Clean-up and exit */
   TA_Free( libHandle, (char *)localDecodingParam.uirSuffix );
   TA_StringFree( stringCache, yahooName );
   TA_TRACE_RETURN( retCode );
}

/**** Local functions definitions.     ****/
static int setUIRSuffixParsing( const char *uirSuffix, UIRSuffixParsing *parseOutput )
{
   int again, i;

   memset( parseOutput, 0, sizeof( UIRSuffixParsing ) );

   /* Set the start of part1 */
   parseOutput->part1 = uirSuffix;

   /* Find the end of part1 */
   i = 0;
   again = 1;
   while( again )
   {
      if( uirSuffix[i]   == '&' &&
          uirSuffix[i+1] == 'a' &&
          uirSuffix[i+2] == '=' )
      {
         again = 0;
      }
      else if( uirSuffix[i] == '\0' )
         return 0;
      else
      {
         parseOutput->part1Length++;
         i++;
      }
   }
   
   /* Set the start of part2 */
   again = 1;
   while( again )
   {
      if( uirSuffix[i]   == '&' &&
          uirSuffix[i+1] == 'c' &&
          uirSuffix[i+2] == '=' )
      {
         again = 0;
         parseOutput->part2 = &uirSuffix[i+7];
         i += 7;
      }
      else if( uirSuffix[i] == '\0' )
         return 0;
      else
         i++;
   }

   /* Find the end of part2 */
   again = 1;
   while( again )
   {
      if( uirSuffix[i]   == '&' &&
          uirSuffix[i+1] == 'd' &&
          uirSuffix[i+2] == '=' )
      {
         again = 0;
      }
      else if( uirSuffix[i] == '\0' )
         return 0;
      else
      {
         parseOutput->part2Length++;
         i++;
      }
   }

   /* Set the start of part3 */
   again = 1;
   while( again )
   {
      if( uirSuffix[i]   == '&' &&
          uirSuffix[i+1] == 'f' &&
          uirSuffix[i+2] == '=' )
      {
         again = 0;
         parseOutput->part3 = &uirSuffix[i+7];
         i += 7;
      }
      else if( uirSuffix[i] == '\0' )
         return 0;
      else
         i++;
   }

   /* Find the end of part2 */
   again = 1;
   while( again )
   {
      if( uirSuffix[i] == '\0' )
      {
         parseOutput->maxTotalLength = parseOutput->part3Length +
                                       parseOutput->part2Length +
                                       parseOutput->part1Length +
                                       100;
         again = 0;
      }
      else
      {
         parseOutput->part3Length++;
         i++;
      }
   }

   return 1; /* Success! */
}

static void buildUIRSuffix( const UIRSuffixParsing *parsedString,
                            TA_Timestamp *startTimestamp,
                            TA_Timestamp *endTimestamp,
                            char *output )
{  
   int i;

   strncpy( output, parsedString->part1, parsedString->part1Length );
   i = parsedString->part1Length;

   sprintf( output,
            "&a=%02d&b=%02d&c=%04d",
            TA_GetMonth( startTimestamp )-1, /* Yahoo! month is zero base */
            TA_GetDay( startTimestamp ),
            TA_GetYear( startTimestamp ) );
   i += 17;

   strncpy( &output[i], parsedString->part2, parsedString->part2Length );
   i += parsedString->part2Length;

   sprintf( &output[i],
            "&d=%02d&e=%02d&f=%04d",
            TA_GetMonth( endTimestamp )-1, /* Yahoo! month is zero base */
            TA_GetDay( endTimestamp ),
            TA_GetYear( endTimestamp ) );
   i += 17;

   strncpy( &output[i], parsedString->part3, parsedString->part3Length );
   i += parsedString->part3Length;

   output[i] = '\0';

   #if DEBUG_PRINTF
   printf( "New UIR=[%s]\n", output );
   #endif
}

static unsigned int nbCommaField( TA_Stream *csvFile )
{
   TA_RetCode retCode;
   TA_StreamAccess *streamAccess;
   unsigned int nbComma;
   unsigned char data;

   streamAccess = TA_StreamAccessAlloc( csvFile );
   if( !streamAccess )
      return 0;

   nbComma = 0;

#ifdef WIN32
#pragma warning( disable : 4127 ) /* Disable 'conditional expression is constant' */
#endif
   while(1)
   {
      retCode = TA_StreamAccessGetByte( streamAccess, &data );
      if( retCode != TA_SUCCESS )
      {
         TA_StreamAccessFree( streamAccess );
         return 0;
      }
      if( data == ',' )
         nbComma++;
      else if( data == '\n' )
      {
         TA_StreamAccessFree( streamAccess );
         return nbComma + 1;
      }
   }
#ifdef WIN32
#pragma warning( default : 4127 ) /* Restore warning settings. */
#endif
}

static int isGapAcceptable( TA_Timestamp *lastBarTimestampAdded,
                            TA_Timestamp *lastBarTimestamp )
{
   TA_RetCode retCode;
   unsigned int deltaDay, i;

   /* Verify if that gap in the data is acceptable. This is not
    * a "perfect" algorithm, but the idea is to avoid obvious
    * failure. Small failure might get through, but the consequence
    * won't be worst than a long week-end gap.
    */

   retCode = TA_TimestampDeltaDay( lastBarTimestampAdded, lastBarTimestamp, &deltaDay );
   if( (retCode != TA_SUCCESS) || (deltaDay >= 7) )
   {
      /* A gap of more than 7 days is an error for sure. Or may be the symbol
       * is not being traded anymore? Don't take a chance and return an error.
       */
      return 0;
   }

   /* The gap should not be more than 3 weekdays (after removing special case) */
   retCode = TA_TimestampDeltaWeekday( lastBarTimestampAdded, lastBarTimestamp, &deltaDay );
   if(  retCode != TA_SUCCESS ) 
   {
      return 0;
   }

   /* Handle special cases */

   /* Trading were suspended on many exchange on september 11 2001 to september 14 2001  */
   for( i=11; i <= 14; i++ )
   {
      if( TA_DateWithinRange( 2001,9,i, lastBarTimestampAdded, lastBarTimestamp ) )
         --deltaDay;
   }
      
   
   /* Handling holidays would be better here, any volunteer to implement this? */
   if( deltaDay > 3 )
      return 0;

   return 1; /* The gap is acceptable. */
}

/* Check if t0 is within the provided [t1..t2] range (inclusive check) */
static unsigned int TA_DateWithinRange( unsigned int year,
                                        unsigned int month,
                                        unsigned int day,
                                        const TA_Timestamp *t1,
                                        const TA_Timestamp *t2 )
{
   TA_Timestamp stamp;
   const TA_Timestamp *lowBorder;
   const TA_Timestamp *highBorder;

   /* Inverse t1 and t2 if not chronilogical order. */
   if( TA_TimestampLess( t2, t1 ) )
   {
      lowBorder = t2;
      highBorder = t1;
   }
   else
   {
      lowBorder = t1;
      highBorder = t2;
   }

   /* Build a timestamp for the date to be check */
   TA_SetDate( year, month, day, &stamp );  

   /* Check if exactly on boundary */
   if( TA_TimestampEqual( &stamp, lowBorder ) && TA_TimestampEqual( &stamp, highBorder ) )
   {
      return 1;
   }

   /* Check if within range. */
   if( TA_TimestampGreater( &stamp, lowBorder ) && TA_TimestampLess( &stamp, highBorder ) )
   {
      return 1;
   }

   return 0; /* Out-of-range */
}
