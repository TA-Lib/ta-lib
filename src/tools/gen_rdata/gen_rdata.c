/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
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
 *  060201 MF   First version.
 *  060304 MF   Option -u updates only invalid or 2 days old indexes.
 */

/* Description:
 *    Build an index of stocks provided by Yahoo! Finance.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "ta_common.h"
#include "ta_yahoo_idx.h"
#include "ta_country_info.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static int verifyFileIntegrity( TA_CountryId countryId, const char *fileStr, int silent, TA_Timestamp *expirationDate  );
static int createFile         ( TA_CountryId countryId, const char *fileStr, TA_YahooIdxStrategy strategy, int silent );

static int displayInfo( TA_YahooIdx *idx );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
int main(int argc, char *argv[] )
{
   TA_Timestamp expirationDate;
   int badParam = 0;
   int retCode;
   unsigned char option = 0;
   TA_CountryId countryId;

   printf( "\n" );
   printf( "gen_rdata V%s - Maintenance tool of Yahoo! index\n", TA_GetVersionString() );
   printf( "\n" );

   countryId = TA_CountryAbbrevToId( "US" ); /* Default country. */

   if( argc < 2 )
   {  
      badParam = 1;
   }
   else if( (strlen( argv[1] ) != 2) || (argv[1][0] != '-') ) 
   {
      printf( "Bad option specified [%s]\n\n", argv[1] );
      badParam = 1;
   }
   
   if( !badParam )
   {
      /* Being here means we can assume there is 3 parameters.
       * Let's validate these.
       */

      /* Validate first parameter (the command) */
      option = (unsigned char)toupper( argv[1][1] );
      switch( option )
      {
      case 'V':
      case 'N':
      case 'C':
         if( argc != 4 )
         {  
            badParam = 1;
         }
         else
         {
            /* Validate second parameter (the country). */
            countryId = TA_CountryAbbrevToId( argv[2] );
            if( !countryId )
            {
               printf( "\nBad country ID specified! [%s]\n", argv[2] );
               badParam = 1;
            }

            /* The third parameter is the indexFile and this one does not
             * requires validation.
             */
         }
         break;
      case 'H':
      case 'U':
      case '?':
         break; 
      default:
         printf( "\nBad option specified! [%s]\n", argv[1] );
         badParam = 1;
      }

   }

      
   if( badParam || option == 'H' || option == '?' )
   {
      printf( "  Usage:  %s -u\n", argv[0] );
      printf( "          %s -v|-n|-c <country> <indexFile>\n", argv[0] );
      printf( "\n");
      printf( "    -u   Update all .dat files (try this first!)\n" );
      printf( "    -v   Verify the integrity of <indexFile>\n" );
      printf( "    -n   Create a new <indexFile> from Yahoo!\n" );
      printf( "    -c   Retreive the <indexFile> from TA-Lib.org\n" );
      printf( "\n" );      
      printf( "  <country>   2 letter abbreviation (CA,US...)\n" );
      printf( "  <indexFile> is a path/filename for this operation.\n" );
      printf( "\n" );
      printf( "  This tool is used for the maintenance of local cache\n" );
      printf( "  of the index of symbols provided by Yahoo!\n" );
      printf( "\n" );
      printf( "  This tool is mainly used by ta-lib.org to generate\n" );
      printf( "  the 'y_xx.dat' files.\n" );
      printf( "\n" );
      printf( "  Example: gen_rdata -u\n" );
      printf( "           gen_rdata -n us y_us.dat\n" );
      printf( "           gen_rdata -c ca y_ca.dat\n" );
      printf( "           gen_rdata -v ca myLocalCopy.dat\n" );
      exit(-1);
   }

   switch( option )
   {
   case 'U':
      /* Will regenerate the file only if broken or more than 2 days old. */
      TA_SetTimeNow(&expirationDate);
      TA_SetDateNow(&expirationDate);
      TA_PrevDay(&expirationDate);
      TA_PrevDay(&expirationDate);

      #define GET_INDEX(country)  \
      { \
        retCode = verifyFileIntegrity( TA_CountryAbbrevToId(country), "y_" country ".dat", 1, &expirationDate ); \
        if( retCode != 0 ) \
        { \
           printf( "Updating index [y_%s.dat]\n", country ); \
           retCode = createFile( TA_CountryAbbrevToId(country), "y_" country ".dat", TA_USE_YAHOO_AND_REMOTE_MERGE, 1 ); \
           if( retCode != 0 ) \
           { \
              printf( "Failed updating index [y_%s.dat]\n", country ); \
           } \
        } \
      }

      #define VERIFY_INDEX(country) \
      { \
        retCode = verifyFileIntegrity( TA_CountryAbbrevToId(country), "y_" country ".dat", 0, &expirationDate ); \
        if( retCode != 0 ) \
        { \
           printf( "Index invalid or expired [y_%s.dat]\n", country ); \
           return retCode; \
        } \
      }

      GET_INDEX("us");
      GET_INDEX("ca");
      GET_INDEX("de");
      GET_INDEX("uk");
      GET_INDEX("es");
      GET_INDEX("fr");
      GET_INDEX("it");
      GET_INDEX("no");
      GET_INDEX("se");
      GET_INDEX("dk");

      VERIFY_INDEX("us");
      VERIFY_INDEX("ca");
      VERIFY_INDEX("de");
      VERIFY_INDEX("uk");
      VERIFY_INDEX("es");
      VERIFY_INDEX("fr");
      VERIFY_INDEX("it");
      VERIFY_INDEX("no");
      VERIFY_INDEX("se");
      VERIFY_INDEX("dk");

      /* Get here only when all index are successfully updated. */
      printf( "\n**** All index successfully updated.\n" );
      return 0;

   case 'V':
      return verifyFileIntegrity( countryId, argv[3], 0, NULL );

   case 'N':
      return createFile( countryId, argv[3], TA_USE_YAHOO_SITE, 0 );

   case 'C':
      return createFile( countryId, argv[3], TA_USE_REMOTE_CACHE, 0 );
   }

   return -1;
}


/**** Local functions definitions.     ****/
static int createFile( TA_CountryId countryId, const char *fileStr, TA_YahooIdxStrategy strategy, int silent )
{
   FILE *out;
   TA_YahooIdx *idx;
   TA_RetCode retCode;
   int retValue;
   TA_Stream *stream;
   TA_InitializeParam initializeParam;

   retValue = -1;

   if( !silent )
      printf( "Building index [%s]\n", fileStr );

   out = fopen( fileStr, "wb" );
   if( !out )
   {
      printf( "Cannot create %s\n", fileStr );
      return -1;
   }

   memset( &initializeParam, 0, sizeof( TA_InitializeParam ) );
   initializeParam.logOutput = stdout;
   retCode = TA_Initialize( &initializeParam );

   if( retCode != TA_SUCCESS )
   {
      fclose( out );
      printf( "\nLibrary initialization failed [%d]\n", retCode );
      return -1;
   }

   retCode = TA_YahooIdxAlloc( countryId, &idx, strategy, NULL, NULL, NULL );

   if( !idx )
   {
      printf( "Index creation failed! Internet connection down? [ErrorCode=%d]\n", retCode );
   }
   else
   {
      retValue = 0; /* Will change if an error occured. */

      retCode = TA_YahooIdxStream( idx, &stream );
      if( retCode == TA_SUCCESS )
         retCode = TA_StreamToFile( stream, out );

      if( retCode != TA_SUCCESS )
      {
         printf( "Streaming to file %s failed! [%d]\n", fileStr, retCode );
         retValue |= 0x00000001;
      }

      retCode = TA_StreamFree( stream );
      if( retCode != TA_SUCCESS )
      {
         printf( "Stream freeing failed [%d]\n", retCode );
         retValue |= 0x00000002;
      }

      retCode = TA_YahooIdxFree( idx );
      if( retCode != TA_SUCCESS )
      {
         printf( "Yahoo index freeing failed [%d]\n", retCode );
         retValue |= 0x00000004;
      }
   }

   /* Clean-up and exit */
   retCode = TA_Shutdown();
   if( retCode != TA_SUCCESS )
   {
      printf( "Library shutdown failed! [%d]\n", retCode );
      retValue |= 0x00000008;
   }

   fclose( out );

   /* On success, as an additional safety net, let's see if we can read
    * back the file.
    */
   if( retValue == 0 )
   {
      retValue = verifyFileIntegrity( countryId, fileStr, silent, NULL );
      if( retValue != 0 )
         printf( "Verification of file integrity failed [%d]\n", retValue );
   }

   return retValue;
}

static int verifyFileIntegrity( TA_CountryId countryId, const char *fileStr, int silent, TA_Timestamp *expirationDate )
{
   FILE *in;
   TA_RetCode retCode;
   TA_Stream *stream;
   TA_YahooIdx *idx = NULL;
   int retValue = -1;
   TA_InitializeParam initializeParam;

   if( !silent )
      printf( "\nVerifying index [%s]\n", fileStr );

   in = fopen( fileStr, "rb" );
   if( !in )
   {
      if( !silent )
         printf( "Cannot re-open %s for reading\n", fileStr );
      return 0x10000001;
   }

   memset( &initializeParam, 0, sizeof( TA_InitializeParam  ) );
   initializeParam.logOutput = stdout;
   retCode = TA_Initialize( &initializeParam );
   if( retCode != TA_SUCCESS )
   {
      fclose( in );
      if( !silent )
         printf( "\nLibrary initialization failed [%d]\n", retCode );
      return 0x10000002;
   }

   stream = TA_StreamAlloc();
   if( stream )
   {
      retValue = 0; /* Will change if an error occured. */

      retCode = TA_StreamAddFile( stream, in );
      if( retCode != TA_SUCCESS )
      {
         if( !silent )
            printf( "\nAdding file to stream failed [%d]\n", retCode );
         retValue = 0x10000004;
      }
      else
      {
         retCode = TA_YahooIdxAlloc( countryId, &idx, TA_USE_STREAM, stream, NULL, NULL );

         if( (retCode != TA_SUCCESS) || !idx )
         {
            if( !silent )
               printf( "Index verification failed! [%d]\n", retCode );
            retValue = 0x10000008;
         }
      }

      if( !silent && idx && (displayInfo( idx ) != 0) )
      {
         if( !silent )
            printf( "Accessing index failed.\n" );
         retValue |= 0x10000020;
      }
      

      /* Check for expiration date of the index. */
      if( expirationDate && idx && TA_TimestampLess(&idx->creationDate,expirationDate) )
      {         
         if( !silent )
            printf( "Index is expired (more than one day old).\n" );
         retValue |= 0x10000200;
      }

      retCode = TA_StreamFree( stream );
      if( retCode != TA_SUCCESS )
      {
         if( !silent )
            printf( "Stream freeing failed [%d]\n", retCode );
         retValue |= 0x10000040;
      }
   }

   /* Clean-up and exit */
   if( idx )
   {
      retCode = TA_YahooIdxFree( idx );
      if( retCode != TA_SUCCESS )
      {
         if( !silent )
            printf( "Yahoo index freeing failed [%d]\n", retCode );
         retValue |= 0x10000080;
      }
   }

   /* Clean-up and exit. */
   fclose( in );

   retCode = TA_Shutdown();
   if( retCode != TA_SUCCESS )
   {
      if( !silent )
         printf( "Library shutdown failed! [%d]\n", retCode );
      retValue |= 0x10000100;
   }

   if( retValue == 0 && !silent )
      printf( "No error found - Index file is good.\n" );

   return retValue;
}

static int displayInfo( TA_YahooIdx *idx )
{
   unsigned int i;
   TA_YahooCategory *category;

   printf( "Content of Index: " );
   if( idx->nbCategory == 0 )
      printf( "(empty)\n" );
   else
   {
      printf( "\n" );
      for( i=0; i < idx->nbCategory; i++ )
      {
         category = idx->categories[i];
         printf( "   %s contains %d symbol%s.\n",
                 TA_StringToChar(category->name),
                 category->nbSymbol,
                 category->nbSymbol > 1? "s":"" );
      }
   }

   printf( "Creation timestamp (mm-dd-yyyy,hh:mm.ss): %02d-%02d-%02d,%02d:%02d.%02d\n",
            TA_GetMonth( &idx->creationDate ),
            TA_GetDay( &idx->creationDate ),
            TA_GetYear( &idx->creationDate ),
            TA_GetHour( &idx->creationDate ),
            TA_GetMin( &idx->creationDate ),
            TA_GetSec( &idx->creationDate ) );

   return 0;
}
