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

   int nbEstimateBar;
   int nbField;
   unsigned int nbTotalBarAdded;
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
      TA_SetDate( 1950, 1, 1, &firstBarTimestamp );

   if( end )
      TA_TimestampCopy( &lastBarTimestamp, end );
   else
      TA_SetDateNow( &lastBarTimestamp );

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
      TA_TRACE_RETURN( TA_UNKNOWN_ERR );
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
      TA_TRACE_RETURN( TA_UNKNOWN_ERR );
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
   while( again && (++nbBatch < 100) && (zeroBarAddedAttempt < 10) )
   {      
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
                              &nbTotalBarAdded,
                              &lastBarTimestamp );

      TA_FileSeqClose( libHandle, fileHandle );
      TA_WebPageFree( webPage );

      /*printf( "NB TOTAL BAR ADDED=%d\n", nbTotalBarAdded );*/

      /* Request the data up to the day BEFORE
       * the last batch of data received.
       */
      TA_PrevDay( &lastBarTimestamp );

      /* Verify if more data should be processed. 
       * Yahoo! sometimes slice their data, in 
       * batch of 200 price bars. 
       */
      if( firstTime && nbTotalBarAdded > 200 )
      {
         again = 0; /* All data extracted... exit the loop. */
      }
      else if( nbTotalBarAdded == 0 )
      {
         /* Make multiple attempts when retreiving data succeed,
          * but somehow there is zero bar returned. Yahoo! somehow
          * do not return data for no apparent reason.
          */
         /*if( zeroBarAddedAttempt > 0 )
            TA_Sleep(zeroBarAddedAttempt*2);*/
         zeroBarAddedAttempt++;
      }
      else if( TA_TimestampLess(&lastBarTimestamp,&firstBarTimestamp) )
      {
          /* Return if all the requested data
           * has been retreived already.
           */
         again = 0;
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
   /*printf( "buildUIR\n" );*/
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
   /*printf( "New UIR=[%s]\n", output );*/
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
