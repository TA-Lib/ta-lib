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
 *  061201 MF   First version.
 *  062803 MF   Add printf when failed to add the symbol to index.
 */

/* Description:
 *    Allows to build the index representing all securities provided
 *    by Yahoo!
 */

/**** Headers ****/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ta_yahoo_idx.h"

#include "ta_dict.h"
#include "ta_list.h"
#include "ta_trace.h"
#include "ta_memory.h"
#include "ta_global.h"
#include "ta_magic_nb.h"
#include "ta_network.h"
#include "ta_yahoo_priv.h"
#include "ta_system.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
TA_HardCodedIndice hardCodedIndice[] = 
{
  { TA_Country_ID_US, "^DJI",   "DJI", "US.CBOT.INDEX" }, 
  { TA_Country_ID_US, "^DJT",   "DJT", "US.CBOT.INDEX" }, 
  { TA_Country_ID_US, "^DJU",   "DJU", "US.CBOT.INDEX" }, 
  { TA_Country_ID_US, "^DJA",   "DJC", "US.CBOT.INDEX" },

  { TA_Country_ID_US, "^NYA",   "NYA", "US.NYSE.INDEX" },
  { TA_Country_ID_US, "^NFA",   "NFA", "US.NYSE.INDEX" },
  { TA_Country_ID_US, "^NDA",   "NDA", "US.NYSE.INDEX" },
  { TA_Country_ID_US, "^NNA",   "NNA", "US.NYSE.INDEX" },
  { TA_Country_ID_US, "^NNA",   "NHB", "US.NYSE.INDEX" },
  { TA_Country_ID_US, "^STI.N", "TRIN", "US.NYSE.INDEX" },
  { TA_Country_ID_US, "^TIC.N", "TICK", "US.NYSE.INDEX" },

  { TA_Country_ID_US, "^IXIC",  "IXIC",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXQ",   "IXQ",   "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^NDX",   "IXNDX", "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXBQ",  "IXBQ",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXFN",  "IXFN",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXF",   "IXF",   "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXID",  "IXID",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXIS",  "IXIS",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXK",   "IXK",   "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXTR",  "IXTR",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^IXUT",  "IXUT",  "US.NASDAQ.INDEX" },
  { TA_Country_ID_US, "^NBI",   "NBI",   "US.NASDAQ.INDEX" },

  { TA_Country_ID_US, "^GSPC",  "SP500", "US.OTHER.INDEX" },
  { TA_Country_ID_US, "^OEX",   "SP100", "US.OTHER.INDEX" },
  { TA_Country_ID_US, "^MID",   "SP400", "US.OTHER.INDEX" },
  { TA_Country_ID_US, "^SML",   "SP600", "US.OTHER.INDEX" },

  { TA_Country_ID_US, "^RUI",   "RUI", "US.OTHER.INDEX" },
  { TA_Country_ID_US, "^RUT",   "RUT", "US.OTHER.INDEX" },
  { TA_Country_ID_US, "^RUA",   "RUA", "US.OTHER.INDEX" },

  { TA_Country_ID_US, "^XAX",   "XAX", "US.AMEX.INDEX" },
  { TA_Country_ID_US, "^IIX",   "IIX", "US.AMEX.INDEX" },
  { TA_Country_ID_US, "^NWX",   "NWX", "US.AMEX.INDEX" },
  { TA_Country_ID_US, "^NDI",   "NDI", "US.AMEX.INDEX" },
  { TA_Country_ID_US, "^XMI",   "XMI", "US.AMEX.INDEX" },
  { TA_Country_ID_US, "^TMW",   "TMW", "US.AMEX.INDEX" },

  { TA_Country_ID_US, "^VLIC",  "VLIC", "US.KCBT.INDEX" },

  { TA_Country_ID_US, "^DOT",   "DOT", "US.PHLX.INDEX" },
  { TA_Country_ID_US, "^XAU",   "XAU", "US.PHLX.INDEX" },
  { TA_Country_ID_US, "^SOXX",  "SOX", "US.PHLX.INDEX" },
  { TA_Country_ID_US, "^BKX",   "BKX", "US.PHLX.INDEX" },

  { TA_Country_ID_US, "^PSE",   "PSE", "US.PSE.INDEX" },

  { TA_Country_ID_US, "^YHOh227", "YHOh227", "US.YAHOO.INDEX" },
  { TA_Country_ID_US, "^YHOh240", "YHOh240", "US.YAHOO.INDEX" },
  { TA_Country_ID_US, "^YHOh301", "YHOh301", "US.YAHOO.INDEX" },  

  { TA_Country_ID_CA, "^TSE",   "TSE300", "CA.TSE.INDEX" }
};

const unsigned int hardCodedIndiceSize = sizeof(hardCodedIndice)/sizeof(TA_HardCodedIndice);

/**** Local declarations.              ****/

/* Hidden data within a TA_YahooCategory. */
typedef struct
{
   /* Variable used only while building the 
    * idx. Put here for convenience of 
    * freeing everything when an error occured.
    */
   TA_Dict *symDict; /* Dict of TA_String representing symbols. */
} TA_YahooCategoryHidden;

/* Hidden data within a TA_YahooIdx. */
typedef struct
{
   TA_Timestamp timestamp;

   /* Variable used only while building the 
    * idx. Put here for convenience of 
    * freeing everything when an error occured.
    */
   TA_Dict *catDict; /* Dict of TA_YahooCategory. */

   TA_YahooDecodingParam decodingParam;

} TA_YahooIdxHidden;


/* Opaque data pass to the functions doing the 
 * parsing of the HTML tables.
 */
typedef struct
{
   TA_YahooIdx *idx;
   TA_CountryId countryId;
   const char *serverName;
   const char *topIndex;
   TA_YahooIdx *idx_remote;
} TA_TableParseOpaqueData;

/**** Local functions declarations.    ****/
static void freeYahooCategory( void *toBeFreed );
static TA_RetCode addYahooSymbol( TA_YahooIdx *idx, 
                                  TA_String *catString,
                                  TA_String *symString );

static TA_RetCode buildIndexFromStream( TA_YahooIdx *idx, 
                                        TA_Stream *stream,
                                        TA_Timestamp *cacheTimeout );

static TA_RetCode buildIndexFromLocalCache( TA_YahooIdx *idx,
                                            TA_Timestamp *cacheTimeout );

static TA_RetCode buildIndexFromRemoteCache( TA_YahooIdx *idx,
                                             TA_Timestamp *cacheTimeout );

static TA_RetCode buildIndexFromYahooWebSite( TA_YahooIdx *idx, int buildFromScratch );

static TA_RetCode convertDictToTables( TA_YahooIdx *idx );
static TA_RetCode convertStreamToTables( TA_YahooIdx *idx, TA_StreamAccess *stream );
static TA_RetCode buildIdxStream( const TA_YahooIdx *idx, TA_Stream *stream );

static TA_RetCode buildDictFromWebSite( TA_YahooIdx *yahooIdx, 
                                        TA_CountryId countryId,
                                        int buildFromScratch );

/* Write the equivalent of a TA_DecodingParam to a stream. */
static TA_RetCode writeDecodingParam( TA_Stream *stream,
                                      TA_DecodingParam *param );

/* Alloc a TA_DecodingParam from a stream. */
static TA_RetCode allocDecodingParam( TA_StreamAccess *streamAccess,
                                      TA_DecodingParam **paramAllocated );

/* Free a TA_DecodingParam */
static TA_RetCode freeDecodingParam( TA_DecodingParam *param );

/* Function where the symbols are extracted from the Yahoo! index page. */
static TA_RetCode addSymbolsFromWebPage( TA_WebPage *webPage,
                                         void *opaqueData );

/* Function for parsing HTML tables. */
static TA_RetCode processTopIndex( unsigned int line,
                                   unsigned int column,
                                   const char *data,
                                   const char *href,
                                   void *opaqueData);

static TA_RetCode processIndex( unsigned int line,
                                unsigned int column,
                                const char *data,
                                const char *href,
                                void *opaqueData);

static TA_RetCode addTheSymbolFromWebPage( unsigned int line,
                                           unsigned int column,
                                           const char *data,
                                           const char *href,
                                           void *opaqueData);

static TA_RetCode findStringFromCacheUsingYahooName( TA_YahooIdx *idx,
                                                     const char *data,                                           
                                                     TA_String **category,
                                                     TA_String **symbol );


/**** Local variables definitions.     ****/
TA_FILE_INFO;

/* American and Canadian decoding info. */
static TA_DecodingParam defaultHistoricalDecoding =
{
   "chart.yahoo.com",
   "/table.csv?s=",
   "&a=1&b=1&c=1950&d=1&e=1&f=3000&g=d&q=q&y=0&z=file&x=.csv",
   0x00000000,0x00,0x00,0x00,0x00,0x0000,0x0000
};

static TA_DecodingParam defaultMarketDecoding =
{
   "finance.yahoo.com",
   "/q?s=",
   "&d=t",
   0x00000000,0x00,0x00,0x00,0x00,0x0000,0x0000
};

static TA_DecodingParam defaultInfoDecoding =
{
   "finance.yahoo.com",
   "/d/quotes.csv?s=",
   "&f=sl1d1t1c1ohgv&e=.csv",
   0x00000000,0x00,0x00,0x00,0x00,0x0000,0x0000
};

/* European Web Site decoding info.
 *
 * Just use the UK site for all european
 * data.
 */
static TA_DecodingParam euHistoricalDecoding =
{
   "uk.table.finance.yahoo.com",
   "/table.csv?s=",
   "&a=1&b=1&c=1950&d=1&e=1&f=3000&g=d&q=q&y=0&z=file&x=.csv",
   0x00000000,0x00,0x00,0x00,0x00,0x0000,0x0000
};

static TA_DecodingParam euMarketDecoding =
{
   "finance.yahoo.com",
   "/q?s=",
   "&d=t",
   0x00000000,0x00,0x00,0x00,0x00,0x0000,0x0000
};

static TA_DecodingParam euInfoDecoding =
{
   "finance.yahoo.com",
   "/d/quotes.csv?s=",
   "&f=sl1d1t1c1ohgv&e=.csv",
   0x00000000,0x00,0x00,0x00,0x00,0x0000,0x0000
};


/**** Global functions definitions.   ****/

TA_DecodingParam *TA_YahooIdxDecodingParam( TA_YahooIdx *idx, TA_DecodeType type )
{
   TA_YahooIdxHidden *idxHidden;

   TA_ASSERT_RET( idx != NULL, NULL );

   idxHidden = (TA_YahooIdxHidden *)idx->hiddenData;
   if( !idxHidden )
      return NULL;

   switch( type )
   {
   case TA_YAHOOIDX_CVS_PAGE:
      return idxHidden->decodingParam.historical;

   case TA_YAHOOIDX_MARKET_PAGE:
      return idxHidden->decodingParam.market;

   case TA_YAHOOIDX_INFO:
      return idxHidden->decodingParam.info;

   default:
      return NULL;
   }
}

TA_RetCode TA_YahooIdxAlloc( TA_CountryId           countryId,
                             TA_YahooIdx          **yahooIdxAllocated,
                             TA_YahooIdxStrategy    strategy,
                             TA_Stream             *stream,
                             TA_Timestamp          *localCacheTimeout,
                             TA_Timestamp          *remoteCacheTimeout )
{
   TA_PROLOG
   TA_YahooIdx *idx;
   TA_RetCode retCode;
   TA_YahooIdxHidden *idxHidden;
   unsigned int idxDone, i;
   const char *tempPtr;

   if( !yahooIdxAllocated )
      return TA_BAD_PARAM;

   (void)theFileDatePtr;
   (void)theFileTimePtr;

   *yahooIdxAllocated = NULL;

   TA_TRACE_BEGIN( TA_YahooIdxAlloc );

   /* Allocate the basic infrastructure for building the index. */
   idx = (TA_YahooIdx *)TA_Malloc( sizeof( TA_YahooIdx ) );
   if( !idx )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   memset( idx, 0, sizeof( TA_YahooIdx ) );
   
   idxHidden = (TA_YahooIdxHidden *)TA_Malloc( sizeof( TA_YahooIdxHidden ) );
   if( !idxHidden )
   {
      TA_Free(  idx );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   memset( idxHidden, 0, sizeof( TA_YahooIdxHidden ) );

   idx->hiddenData = (void *)idxHidden;

   /* From this point TA_YahooIdxFree can be safely called. */

   /* Set default creation date. */
   TA_SetDefault( &idx->creationDate );

   /* Validate the countryId. */
   tempPtr = TA_CountryIdToAbbrev(countryId);
   if( !tempPtr )
   {
      TA_YahooIdxFree( idx );
      TA_TRACE_RETURN( TA_UNSUPPORTED_COUNTRY );
   }
   idx->countryId = countryId;
   idx->countryAbbrev[0] = tempPtr[0];
   idx->countryAbbrev[1] = tempPtr[1];
   idx->countryAbbrev[2] = '\0';

   idxDone = 0;
   /* Start to build using the proposed strategy. */
   if( stream )
   {
      retCode = buildIndexFromStream( idx, stream, NULL );
      if( retCode != TA_SUCCESS )
      {
         TA_YahooIdxFree( idx );
         TA_TRACE_RETURN( retCode );
      }

      idxDone = 1; /* idx is now build. */
   }
   else
   {

      if( !idxDone && (strategy & TA_USE_LOCAL_CACHE) )
      {
         retCode = buildIndexFromLocalCache( idx, localCacheTimeout );

         if( retCode == TA_SUCCESS )
            idxDone = 1; /* idx is now build. */
         else if( !(strategy & (TA_USE_REMOTE_CACHE|TA_USE_YAHOO_SITE|TA_USE_YAHOO_AND_REMOTE_MERGE)) )
         {
            /* No other alternative offer, so return with failure. */
            TA_YahooIdxFree( idx );
            TA_TRACE_RETURN( TA_YAHOO_IDX_UNAVAILABLE_1 );
         }
      }

      if( !idxDone && (strategy & TA_USE_REMOTE_CACHE) )
      {
         /* Try to retreive the index from the 
          * remote cache (ta-lib.org web site).
          */
         retCode = TA_YAHOO_IDX_UNAVAILABLE_2;
         for( i=0; (i < 3) && (retCode != TA_SUCCESS); i++ )
            retCode = buildIndexFromRemoteCache( idx, remoteCacheTimeout );

         if( retCode == TA_SUCCESS )
            idxDone = 1; /* idx is now build. */
         else
         { 
            if( !(strategy & (TA_USE_YAHOO_SITE|TA_USE_YAHOO_AND_REMOTE_MERGE)) )
            {
               /* No other alternative offer, so return with failure. */
               TA_YahooIdxFree( idx );
               TA_TRACE_RETURN( TA_YAHOO_IDX_UNAVAILABLE_2 );
            }
         }
      }
   
      if( !idxDone && (strategy & TA_USE_YAHOO_SITE) )
      {
         retCode = buildIndexFromYahooWebSite( idx, 1 );
         if( retCode == TA_SUCCESS )
            idxDone = 1; /* idx is now build. */
         else
         { 
            if( !(strategy & (TA_USE_YAHOO_AND_REMOTE_MERGE)) )
            {
               /* No other alternative offer, so return with failure. */
               TA_YahooIdxFree( idx );
               TA_TRACE_RETURN( TA_YAHOO_IDX_UNAVAILABLE_4 );
            }
         }  
      }

      if( !idxDone && (strategy & TA_USE_YAHOO_AND_REMOTE_MERGE) )
      {
         retCode = buildIndexFromYahooWebSite( idx, 0 );
         if( retCode == TA_SUCCESS )
            idxDone = 1; /* idx is now build. */
      }
   }

   if( !idxDone )
   {
      TA_YahooIdxFree( idx );
      if( strategy & (TA_USE_YAHOO_SITE|TA_USE_REMOTE_CACHE|TA_USE_YAHOO_AND_REMOTE_MERGE) )
      {
         TA_TRACE_RETURN( TA_YAHOO_IDX_UNAVAILABLE_3 );
      }
      else
      {
         TA_TRACE_RETURN( TA_YAHOO_IDX_FAILED );
      }
   }

   /* Success! Return the index to the caller. */
   *yahooIdxAllocated = idx;

   TA_TRACE_RETURN( TA_SUCCESS );
}


TA_RetCode TA_YahooIdxFree( TA_YahooIdx *idxToBeFreed )
{
   TA_YahooIdxHidden *idxHidden;
   unsigned int i;
   TA_StringCache *stringCache;
   TA_YahooCategory *category;
   unsigned int catBelongToDict;

   if( idxToBeFreed )
   {
      /* Get pointers used to free the rest... */
      idxHidden = (TA_YahooIdxHidden *)idxToBeFreed->hiddenData;
      if( !idxHidden )
         return TA_INTERNAL_ERROR(105);

      stringCache = TA_GetGlobalStringCache();
      if( !stringCache )
         return TA_INTERNAL_ERROR(106);

      /* Identify who owns the category, consequently
       * who should free these (when a dictionary exist
       * the pointers in 'categories' are just alias).
       */
      if( idxHidden->catDict )
         catBelongToDict = 1;
      else
         catBelongToDict = 0;
      
      /* Free public data. */      
      if( idxToBeFreed->categories )
      {         
         /* Free categories if not own by catDict. */
         if( !catBelongToDict )
         {
            for( i=0; i < idxToBeFreed->nbCategory; i++ )
            {
               category = idxToBeFreed->categories[i];
               if( category )
                  freeYahooCategory( category );
            }
         }

         TA_Free(  idxToBeFreed->categories );
      }

      /* Free hidden data. */
      if( idxHidden->catDict )
         TA_DictFree( idxHidden->catDict );
      if( idxHidden->decodingParam.historical )
         freeDecodingParam( idxHidden->decodingParam.historical );
      if( idxHidden->decodingParam.market )
         freeDecodingParam( idxHidden->decodingParam.market );
      if( idxHidden->decodingParam.info )
         freeDecodingParam( idxHidden->decodingParam.info );

      TA_Free(  idxHidden );

      /* Finally, free the TA_YahooIdx itself. */
      TA_Free(  idxToBeFreed );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_YahooIdxStream( const TA_YahooIdx *idx, TA_Stream **streamAllocated )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_YahooIdxHidden *idxHidden;
   TA_Stream *stream;
   TA_Stream *compressedStream;
   TA_Timestamp now;
   TA_Integer identifier;

   unsigned int beforeSize, afterSize;
   #ifdef TA_DEBUG
   FILE *out;
   #endif

   if( !idx || !streamAllocated )
      return TA_BAD_PARAM;

   idxHidden = idx->hiddenData;

   *streamAllocated = NULL;

   TA_TRACE_BEGIN(  TA_YahooIdxStream );

   #ifdef TA_DEBUG
      out = TA_GetStdioFilePtr();
   #endif

   stream = TA_StreamAlloc();
   if( !stream )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Build the stream. */
   retCode = buildIdxStream( idx, stream );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( stream );
      TA_TRACE_RETURN( retCode );
   }

   /* Compress the stream. */
   compressedStream = TA_StreamAlloc();
   if( !compressedStream )
   {
      TA_StreamFree( stream );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   retCode = TA_StreamCompress( compressedStream, stream );
   beforeSize = TA_StreamSizeInBit( stream );

   #if 0
      #ifdef TA_DEBUG
      /* Useful when very low-level debugging is needed. */
      if( out )
      {
         fprintf( out, "ORIG: " );
         TA_StreamPrint( stream );
         fprintf( out, "COMP: " );
         TA_StreamPrint( compressedStream );
      }
      #endif
   #endif
    
   TA_StreamFree( stream ); /* The uncompress one not needed anymore. */

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }
   afterSize = TA_StreamSizeInBit( compressedStream );

   /* Encapsulate the stream. */
   TA_SetDateNow( &now );
   TA_SetTimeNow( &now );
   identifier = 0;
   retCode = TA_StreamEncapsulate( &compressedStream, &now, identifier );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( stream );
      TA_TRACE_RETURN( retCode );
   }

   #if 0
      #ifdef TA_DEBUG
      /* Useful when very low-level debugging is needed. */
      if( out )
      {
         fprintf( out, "ENCP: " );
         TA_StreamPrint( compressedStream );
         fprintf( out, "Yahoo! index final Size = %d\n", TA_StreamSizeInByte(compressedStream) );
      }
      #endif
   #endif

   /* All done! Return the stream to the caller. */
   *streamAllocated = compressedStream;
   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/

/* The string are duplicated and added to the dictionaries. */
static TA_RetCode addYahooSymbol( TA_YahooIdx *idx, 
                                  TA_String  *catString,
                                  TA_String  *symString )
{
   TA_RetCode retCode;
   TA_YahooCategory *category;
   TA_YahooCategoryHidden *categoryHidden;
   TA_StringCache *stringCache;
   TA_YahooIdxHidden *idxHidden;

   /* Check parameters. */
   if( !idx || !catString || !symString )
      return TA_BAD_PARAM;

   if( !strlen(TA_StringToChar(catString)) || !strlen(TA_StringToChar(symString)) )
      return TA_BAD_PARAM;

   idxHidden = (TA_YahooIdxHidden *)idx->hiddenData;
   if( !idxHidden )
      return TA_INTERNAL_ERROR(107);

   stringCache = TA_GetGlobalStringCache();
   if( !stringCache )
      return TA_INTERNAL_ERROR(109);

   /* Add the symbol/category to the dictionnary. */
   category = TA_DictGetValue_S( idxHidden->catDict, TA_StringToChar(catString) );

   if( category )
   {
      /* The category is already in the dictionnary. */
      categoryHidden = (TA_YahooCategoryHidden *)category->hiddenData;
      if( !categoryHidden )
         return TA_INTERNAL_ERROR(110);
   }
   else
   {
      /* The category is NOT in the dictionary.
       * Allocate a new category.
       */
      category = (TA_YahooCategory *)TA_Malloc( sizeof( TA_YahooCategory ) );
      if( !category )
         return TA_ALLOC_ERR;

      memset( category, 0, sizeof( TA_YahooCategory ) );
      
      /* From this point, freeYahooCategory can be safely called. */

      category->name = TA_StringDup( stringCache, catString );
      if( !category->name )
      {
         freeYahooCategory( category );
         return TA_ALLOC_ERR;
      }

      category->hiddenData = (TA_YahooCategoryHidden *)TA_Malloc( sizeof( TA_YahooCategoryHidden ) );
      if( !category->hiddenData )
      {
         freeYahooCategory( category );
         return TA_ALLOC_ERR;
      }

      categoryHidden = category->hiddenData;
      categoryHidden->symDict = TA_DictAlloc( TA_DICT_KEY_ONE_STRING, NULL );
      if( !categoryHidden->symDict )
      {
         freeYahooCategory( category );
         return TA_ALLOC_ERR;
      }

      retCode = TA_DictAddPair_S( idxHidden->catDict, category->name, category );
      if( retCode != TA_SUCCESS )
      {
         freeYahooCategory( category );
         return retCode;
      }

      /* If something goes wrong from this point, the category will
       * be free by TA_YahooIdxFree because it is now part of the
       * dictionary.
       */
   }

   /* Add symbol to the dictionary of this category. 
    * The value is not important, only the key is significant.
    * This is why the value is a dummy '1'.
    */
   retCode = TA_DictAddPair_S( categoryHidden->symDict, symString, (void *)1 );
   if( retCode != TA_SUCCESS )
      return TA_ALLOC_ERR;
   
   /* Everything went well. */
   return TA_SUCCESS;
}

static void freeYahooCategory( void *toBefreed )
{
   TA_YahooCategory *category;
   unsigned int i;
   TA_StringCache *stringCache;
   TA_YahooCategoryHidden *hiddenData;
   TA_String *stringPtr;

   category = (TA_YahooCategory *)toBefreed;
   stringCache = TA_GetGlobalStringCache();

   if( category )
   {
      if( category->name )
         TA_StringFree( stringCache, category->name );
 
      if( category->symbols != 0 )
      {
         for( i=0; i < category->nbSymbol; i++ )
         {
            stringPtr = category->symbols[i];
            if( stringPtr )
               TA_StringFree( stringCache, stringPtr );
         }

         TA_Free(  (void *)category->symbols );
      }

      hiddenData = category->hiddenData;
      if( hiddenData )
      {
         if( hiddenData->symDict )
            TA_DictFree( hiddenData->symDict );
         TA_Free(  (void *)hiddenData );
      } 

      TA_Free(  (void *)category );
   }
}

static TA_RetCode buildIndexFromStream( TA_YahooIdx *idx, 
                                        TA_Stream *stream,
                                        TA_Timestamp *timeoutDate )
{
   TA_RetCode retCode;
   TA_Stream *finalStream;
   TA_StreamAccess *streamAccess;
   TA_Timestamp streamTimestamp;
   TA_Integer streamIdentifier;

   #ifdef TA_DEBUG
   FILE *out;
   #endif
      
   #ifdef TA_DEBUG
      out = TA_GetStdioFilePtr();
   #endif

   /* Build the public tables from the stream. */

   /* Decapsulate the stream. */
   #ifdef TA_DEBUG
      #if 0
      /* Useful when very low-level debugging is needed. */
      if( out )
      {
         fprintf( out, "ENCP: " );
         TA_StreamPrint( stream );
      }
      #endif
   #endif

   retCode = TA_StreamDecapsulate( stream, &streamTimestamp, &streamIdentifier );
   if( retCode != TA_SUCCESS )
      return retCode;

   #ifdef TA_DEBUG
      #if 0
      if( out )
      {
         fprintf( out, "COMP: " );
         TA_StreamPrint( stream );
      }
      #endif
   #endif

   retCode = TA_TimestampValidate( &streamTimestamp );
   if( retCode != TA_SUCCESS )
      return retCode;

   TA_TimestampCopy( &idx->creationDate, &streamTimestamp );

   /* Check for timeout, if the caller cares. */
   if( timeoutDate )
   {
      if( TA_TimestampLess( &streamTimestamp, timeoutDate ) )
         return TA_YAHOO_IDX_EXPIRED;
   }

   /* Decompress the stream. */
   streamAccess = TA_StreamAccessAlloc( stream );
   if( !streamAccess )
      return TA_ALLOC_ERR;

   retCode = TA_StreamDecompress( streamAccess, &finalStream );
   TA_StreamAccessFree( streamAccess );

   if( retCode != TA_SUCCESS )
      return retCode;

   #ifdef TA_DEBUG
      #if 0
      /* Useful when very low-level debugging is needed. */
      if( out )
      {
         fprintf( out, "ORIG: " );
         TA_StreamPrint( finalStream );
      }
      #endif
   #endif

   /* Convert the stream to the public interface. 
    * Needs a streamAccess on the finalStream.
    */
   streamAccess = TA_StreamAccessAlloc( finalStream );
   if( !streamAccess )
   {
      TA_StreamFree( finalStream );
      return TA_ALLOC_ERR;
   }
   retCode = convertStreamToTables( idx, streamAccess );
   TA_StreamAccessFree( streamAccess );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( finalStream );
      return retCode;
   }

   /* All done, get ride of the stream. */
   retCode = TA_StreamFree( finalStream );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

static TA_RetCode buildIndexFromRemoteCache( TA_YahooIdx *idx, TA_Timestamp *cacheTimeout )
{
   TA_RetCode retCode;
   TA_WebPage *webPage;
   TA_Stream  *stream;
   TA_YahooIdxHidden *idxHidden;
   const char *cachePath;
   char *pathBuffer;
   FILE *out;
   int pathLength;

   char buffer[100];

   if( !idx )
      return TA_BAD_PARAM;

   idxHidden= idx->hiddenData;
   if( !idxHidden )
      return TA_BAD_PARAM;

   /* Get the cache online from TA-LIB org */
   if( strlen( (const char *)&idx->countryAbbrev[0] ) != 2 )
      return TA_INTERNAL_ERROR(111);

   sprintf( buffer, "/rdata/y_%c%c.dat",
            tolower(idx->countryAbbrev[0]),
            tolower(idx->countryAbbrev[1]) );
   retCode = TA_WebPageAlloc( "ta-lib.org",
                              buffer,
                              NULL, NULL,
                              &webPage, 2 );

   if( retCode != TA_SUCCESS )
      return retCode;

   stream = webPage->content;
   if( !stream )
   {
      TA_WebPageFree( webPage );
      return TA_ALLOC_ERR;
   }

   /* From this point, the webPage will be freed when the stream will
    * be freed.
    */

   /* Save the remote cache locally. */
   cachePath = TA_GetLocalCachePath();

   if( cachePath )
   {
      /* Create the local file */
      pathLength = strlen( cachePath );
      pathBuffer = TA_Malloc( pathLength + 11 );
      if( pathLength )
      {
         sprintf( pathBuffer, "%s%cy_%c%c.dat",
                  cachePath,
                  TA_SeparatorASCII(),
                  tolower(idx->countryAbbrev[0]),
                  tolower(idx->countryAbbrev[1]) );
      }
      else
      {
         sprintf( pathBuffer, "y_%c%c.dat",
                  tolower(idx->countryAbbrev[0]),
                  tolower(idx->countryAbbrev[1]) );
      }


      out = fopen( pathBuffer, "wb" );
      if( out )
      {
         /* printf( "Saving local [%s]\n", pathBuffer ); */
         TA_StreamToFile( stream, out );
      }
      TA_Free(  pathBuffer );
      fclose(out);
   }

   /* Call buildIndexFromStream to build the tables. */
   retCode = buildIndexFromStream( idx, stream, cacheTimeout );
   if( retCode != TA_SUCCESS )
   {
      TA_WebPageFree( webPage );
      return retCode;
   }

      
   retCode = TA_WebPageFree( webPage );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

static TA_RetCode buildIndexFromYahooWebSite( TA_YahooIdx *idx, int buildFromScratch )
{
   TA_RetCode retCode;
   TA_YahooIdxHidden *idxHidden;

   idxHidden = (TA_YahooIdxHidden *)idx->hiddenData;

   /* Allocate a dictionary to make it easier to build the whole index. */
   idxHidden->catDict = (void *)TA_DictAlloc( TA_DICT_KEY_ONE_STRING, freeYahooCategory );
   if( !idxHidden->catDict )
   {
      return TA_ALLOC_ERR;
   }

   retCode = buildDictFromWebSite( idx, idx->countryId, buildFromScratch );
   if( retCode != TA_SUCCESS )
   {
      return retCode;
   }

   /* Convert the dictionaries into the public tables. */
   retCode = convertDictToTables( idx );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

static TA_RetCode convertDictToTables( TA_YahooIdx *idx )
{
   TA_YahooIdxHidden *idxHidden;
   TA_StringCache *stringCache;
   TA_YahooCategoryHidden *categoryHidden;
   TA_YahooCategory *category;
   int i, j, nbCategory, nbSymbol;
   int again1, again2; /* boolean */
   TA_Dict *symDict;

   /* Check parameters. */
   if( !idx | (idx->categories != NULL) )
      return TA_BAD_PARAM;
   
   idxHidden = (TA_YahooIdxHidden *)idx->hiddenData;
   stringCache = TA_GetGlobalStringCache();

   nbCategory = TA_DictAccessFirst( idxHidden->catDict );
   if( nbCategory == 0 )
      idx->categories = NULL;
   else
   {
      idx->categories = (TA_YahooCategory **)TA_Malloc( sizeof( TA_YahooCategory * ) * nbCategory );
      memset( idx->categories, 0, sizeof( TA_YahooCategory * ) * nbCategory );
      if( !idx->categories )
         return TA_ALLOC_ERR;
      idx->nbCategory = nbCategory;

      /* Iterate through the category. */
      again1 = 1;
      for( i=0; again1 && (i < nbCategory); i++ )
      {
        category = (TA_YahooCategory *)TA_DictAccessValue( idxHidden->catDict );
        if( !category )
           return TA_INTERNAL_ERROR(112);

        categoryHidden = category->hiddenData;
        if( !categoryHidden )
           return TA_INTERNAL_ERROR(113);

        /* Iterate through the symbols. */
        symDict = categoryHidden->symDict;
        nbSymbol = TA_DictAccessFirst( symDict );
        if( nbSymbol == 0 )
           return TA_INTERNAL_ERROR(114);
        else
        {
           category->symbols = (TA_String **)TA_Malloc( sizeof( TA_String *) * nbSymbol );
           if( !category->symbols )
              return TA_ALLOC_ERR;
           category->nbSymbol = nbSymbol;
           again2 = 1;
           for( j=0; again2 && (j < nbSymbol); j++ )
           {
              category->symbols[j] = TA_StringDup( stringCache, TA_DictAccessKey( symDict ) );
              again2 = TA_DictAccessNext( symDict );
           }

           TA_DictFree( symDict );
           categoryHidden->symDict = NULL;
        }           

        /* Alias the category in the public interface.
         * The free function will free the dictionary and not
         * the alias in the public interface (when the catDict != NULL
         * of course, else the alias are considered the original element).
         */
        idx->categories[i] = (TA_YahooCategory *)TA_DictAccessValue( idxHidden->catDict );        
        again1 = TA_DictAccessNext( idxHidden->catDict );
      }
   }

   return TA_SUCCESS;
}

/* The stream is defined as follow:
 *
 *   Write the header:
 *    (32 bits)   TA_YAHOO_IDX_MAGIC_NB
 *    ( 8 bits)   Version of this encoding
 *
 *    ( 8 bits)   Country Abbreviation letter 1
 *    ( 8 bits)   Country Abbreviation letter 2
 *
 *    (TA_Timestamp) Creation date/time of this index.
 *
 *   Write the TA_DecodingParam for historical data:
 *    (TA_String) Web Site Server
 *    (TA_String) UIR Prefix
 *    (TA_String) UIR Suffix
 *    (32 bits)   Decoding Flow Flags
 *    ( 8 bits)   Decoding Param8_1
 *    ( 8 bits)   Decoding Param8_2
 *    ( 8 bits)   Decoding Param8_3
 *    ( 8 bits)   Decoding Param8_4
 *    (16 bits)   Decoding Param16_1
 *    (16 bits)   Decoding Param16_2
 *
 *   Write the TA_DecodingParam for market info:
 *    (TA_String) Web Site Server
 *    (TA_String) UIR Prefix
 *    (TA_String) UIR Suffix
 *    (32 bits)   Decoding Flow Flags
 *    ( 8 bits)   Decoding Param8_1
 *    ( 8 bits)   Decoding Param8_2
 *    ( 8 bits)   Decoding Param8_3
 *    ( 8 bits)   Decoding Param8_4
 *    (16 bits)   Decoding Param16_1
 *    (16 bits)   Decoding Param16_2
 *
 *   Write the TA_DecodingParam for security info:
 *    (TA_String) Web Site Server
 *    (TA_String) UIR Prefix
 *    (TA_String) UIR Suffix
 *    (32 bits)   Decoding Flow Flags
 *    ( 8 bits)   Decoding Param8_1
 *    ( 8 bits)   Decoding Param8_2
 *    ( 8 bits)   Decoding Param8_3
 *    ( 8 bits)   Decoding Param8_4
 *    (16 bits)   Decoding Param16_1
 *    (16 bits)   Decoding Param16_2
 *
 *    (16 bits)   Nb Of Category
 *
 *   For each category:
 *        (8 bits)        A sanity indicator (0xAF)
 *        (TA_String)     The name of the category.
 *        (32 bits)       The Nb Of Symbol.
 *        (8 bits)        A sanity indicator (0xEB)
 *        n x (TA_String) A string for each symbols.
 *        (8 bits)        A sanity indicator (0xF2)
 *   End for each category.
 *
 */    
static TA_RetCode buildIdxStream( const TA_YahooIdx *idx, TA_Stream *stream )
{
   TA_RetCode retCode;
   TA_YahooCategory *category;
   unsigned int i, j;
   TA_Timestamp timestamp;

   retCode = TA_StreamAddInt32( stream, TA_YAHOO_IDX_MAGIC_NB );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = TA_StreamAddByte( stream, 0x02 ); /* Version 0.2 */
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Add timestamp. */
   retCode = TA_SetDateNow( &timestamp );
   if( retCode != TA_SUCCESS )
      return retCode;
   retCode = TA_SetTimeNow( &timestamp );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = TA_StreamAddTimestamp( stream, &timestamp );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Write the last known valid decoding
    * parameters for this country.
    */
   for( i=0; i < 2; i++ )
   {
      retCode = TA_StreamAddByte( stream, idx->countryAbbrev[i] );
      if( retCode != TA_SUCCESS )
         return retCode;
   }

   switch( idx->countryId )
   {
   case TA_Country_ID_US:
   case TA_Country_ID_CA:
       /* Write historical decoding data. */
       retCode = writeDecodingParam( stream,
                                     &defaultHistoricalDecoding );

       /* Write market decoding data. */
       if( retCode == TA_SUCCESS )
          retCode = writeDecodingParam( stream,
                                        &defaultMarketDecoding );

       /* Write info decoding data. */
       if( retCode == TA_SUCCESS )
          retCode = writeDecodingParam( stream,
                                        &defaultInfoDecoding );
       break;

   case TA_Country_ID_UK: /* United Kingdom */
   case TA_Country_ID_DE: /* Germany */
   case TA_Country_ID_DK: /* Denmark */
   case TA_Country_ID_ES: /* Spain */
   case TA_Country_ID_FR: /* France */
   case TA_Country_ID_IT: /* Italy */
   case TA_Country_ID_SE: /* Sweden */
   case TA_Country_ID_NO: /* Norway */
       /* Write historical decoding data. */
       retCode = writeDecodingParam( stream,
                                     &euHistoricalDecoding );

       /* Write market decoding data. */
       if( retCode == TA_SUCCESS )
          retCode = writeDecodingParam( stream,
                                        &euMarketDecoding );

       /* Write info decoding data. */
       if( retCode == TA_SUCCESS )
          retCode = writeDecodingParam( stream,
                                        &euInfoDecoding );       
       break;

   default:
      return TA_UNSUPPORTED_COUNTRY;
   }
      
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Add the number of category. */   
   retCode = TA_StreamAddInt16( stream, (unsigned short)idx->nbCategory );
   if( retCode != TA_SUCCESS )
      return retCode;
   
   /* Add the data of each category. */
   for( i=0; i < idx->nbCategory; i++ )
   {
      retCode = TA_StreamAddByte( stream, 0xAF );
      if( retCode != TA_SUCCESS )
         return retCode;

      category = idx->categories[i];
      if( !category )
         return TA_INTERNAL_ERROR(115);

      retCode = TA_StreamAddString( stream, category->name );
      if( retCode != TA_SUCCESS )
         return retCode;

      retCode = TA_StreamAddInt32( stream, category->nbSymbol );
      if( retCode != TA_SUCCESS )
         return retCode;

      retCode = TA_StreamAddByte( stream, 0xEB );
      if( retCode != TA_SUCCESS )
         return retCode;

      for( j=0; j < category->nbSymbol; j++ )
      {
         retCode = TA_StreamAddString( stream, category->symbols[j] );
         if( retCode != TA_SUCCESS )
            return retCode;         
      }

      retCode = TA_StreamAddByte( stream, 0xF2 );
      if( retCode != TA_SUCCESS )
         return retCode;
   }

   /* The stream representing the index is completed! */

   return TA_SUCCESS;
}


static TA_RetCode convertStreamToTables( TA_YahooIdx *idx, TA_StreamAccess *streamAccess )
{
   TA_RetCode retCode;
   unsigned int i, j, intData, tempInt;
   unsigned int nbCategory, nbSymbol;
   unsigned char data;
   TA_YahooIdxHidden *idxHidden;
   TA_YahooCategory *category;
   unsigned char countryAbbrev[3];

   if( !idx || !streamAccess )
      return TA_BAD_PARAM;

   idxHidden = (TA_YahooIdxHidden *)idx->hiddenData;
   if( !idxHidden )
      return TA_INTERNAL_ERROR(116);

   retCode = TA_StreamAccessGetInt32( streamAccess, &intData );
   if( retCode != TA_SUCCESS )
      return retCode;

   if( intData != TA_YAHOO_IDX_MAGIC_NB )
      return TA_BAD_YAHOO_IDX_HDR;

   /* Get the version. */    
   retCode = TA_StreamAccessGetByte( streamAccess, &data );
   if( retCode != TA_SUCCESS )
      return retCode;
   
   /* This code support only Version 0.2 of this encoding. */
   if( data != 0x02 )
      return TA_UNSUPORTED_YAHOO_IDX_VERSION;

   /* Get the timestamp. This is unused here. */
   retCode = TA_StreamAccessGetTimestamp( streamAccess, &idxHidden->timestamp );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Get and validate the country info. */
   countryAbbrev[2] = '\0';
   for( i=0; i < 2; i++ )
   {
      retCode = TA_StreamAccessGetByte( streamAccess, &countryAbbrev[i] );
      if( retCode != TA_SUCCESS )
         return retCode;
   }
   if( TA_CountryAbbrevToId( (const char *)&countryAbbrev[0] ) != idx->countryId )
      return TA_UNSUPPORTED_COUNTRY;

   /* Read the web page decoding parameters. */
   retCode = allocDecodingParam( streamAccess, &idxHidden->decodingParam.historical );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = allocDecodingParam( streamAccess, &idxHidden->decodingParam.market );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = allocDecodingParam( streamAccess, &idxHidden->decodingParam.info );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Get the number of category and allocate the needed space. */
   retCode = TA_StreamAccessGetInt16( streamAccess, &nbCategory );
   if( retCode != TA_SUCCESS )
      return retCode;
   tempInt = sizeof( TA_YahooCategory *) * nbCategory;
   idx->categories = TA_Malloc( tempInt );
   if( !idx->categories )
      return TA_ALLOC_ERR;
   idx->nbCategory = 0; /* Will get incremented as the category are allocated. */

   /* Allocate each category. */
   for( i=0; i < nbCategory; i++ )
   {
      /* Verify the sanity indicator (0xAF) */
      retCode = TA_StreamAccessGetByte( streamAccess, &data );
      if( retCode != TA_SUCCESS )
         return retCode;
      if( data != 0xAF )
         return TA_BAD_YAHOO_IDX_INDICATOR_AF;

      /* Allocate directly the category in the public interface.
       * No need for catDict.
       */
      category = TA_Malloc( sizeof( TA_YahooCategory ) );
      if( !category )
         return TA_ALLOC_ERR;
      memset( category, 0, sizeof( TA_YahooCategory ) );
      idx->categories[i] = category;      
      idx->nbCategory++;
      retCode = TA_StreamAccessGetString( streamAccess, &category->name );
      if( retCode != TA_SUCCESS )
         return retCode;

      /* Get the number of symbol for this category. */
      retCode = TA_StreamAccessGetInt32( streamAccess, &nbSymbol );
      if( retCode != TA_SUCCESS )
         return retCode;

      /* Verify the sanity indicator (0xEB) */
      retCode = TA_StreamAccessGetByte( streamAccess, &data );
      if( retCode != TA_SUCCESS )
         return retCode;
      if( data != 0xEB )
         return TA_BAD_YAHOO_IDX_INDICATOR_EB;

      /* Allocate the space needed for the symbols. */
      tempInt = sizeof( TA_String *) * nbSymbol;
      category->symbols = (TA_String **)TA_Malloc( tempInt );
      if( !category->symbols )
         return TA_ALLOC_ERR;

      /* Get the string for each symbol in this category. */
      retCode = TA_SUCCESS;
      for( j=0; (retCode == TA_SUCCESS) && (j < nbSymbol); j++ )
      {
         retCode = TA_StreamAccessGetString( streamAccess, &category->symbols[j] );
         if( retCode != TA_SUCCESS )
            return retCode;
         category->nbSymbol++;
      }

      /* Verify the sanity indicator (0xF2) */
      retCode = TA_StreamAccessGetByte( streamAccess, &data );
      if( retCode != TA_SUCCESS )
         return retCode;
      if( data != 0xF2 )
         return TA_BAD_YAHOO_IDX_INDICATOR_F2;
   }

   return TA_SUCCESS;
}

static TA_RetCode writeDecodingParam( 
                                      TA_Stream *stream,
                                      TA_DecodingParam *param )
{
   TA_RetCode retCode;
   TA_String *tempString;
   TA_StringCache *stringCache;

   stringCache = TA_GetGlobalStringCache();

   /* Add webSiteServer. */
   tempString= TA_StringAlloc( stringCache, param->webSiteServer );
   if( !tempString )
      return TA_ALLOC_ERR;
   retCode = TA_StreamAddString( stream, tempString );
   TA_StringFree( stringCache, tempString );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Add uirPrefix */
   tempString= TA_StringAlloc( stringCache, param->uirPrefix );
   if( !tempString )
      return TA_ALLOC_ERR;
   retCode = TA_StreamAddString( stream, tempString );
   TA_StringFree( stringCache, tempString );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Add uirSuffix */
   tempString= TA_StringAlloc( stringCache, param->uirSuffix );
   if( !tempString )
      return TA_ALLOC_ERR;
   retCode = TA_StreamAddString( stream, tempString );
   TA_StringFree( stringCache, tempString );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = TA_StreamAddInt32( stream, param->flags );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Add the rest... */
   retCode = TA_StreamAddByte( stream, param->param8_1);
   if( retCode != TA_SUCCESS )
      return retCode;
   retCode = TA_StreamAddByte( stream, param->param8_2);
   if( retCode != TA_SUCCESS )
      return retCode;
   retCode = TA_StreamAddByte( stream, param->param8_3);
   if( retCode != TA_SUCCESS )
      return retCode;
   retCode = TA_StreamAddByte( stream, param->param8_4);
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = TA_StreamAddInt16( stream, (unsigned short)param->param16_1);
   if( retCode != TA_SUCCESS )
      return retCode;
   retCode = TA_StreamAddInt16( stream, (unsigned short)param->param16_2);
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

static TA_RetCode allocDecodingParam( TA_StreamAccess *streamAccess,
                                      TA_DecodingParam **paramAllocated )
{
   TA_DecodingParam *param;
   TA_RetCode retCode;
   TA_String *tempString;

   if( !streamAccess || !paramAllocated )
      return TA_BAD_PARAM;

   *paramAllocated = NULL;

   retCode = TA_SUCCESS;

   param = (TA_DecodingParam *)TA_Malloc( sizeof( TA_DecodingParam ) );
   if( !param )
      return TA_ALLOC_ERR;

   memset( param, 0, sizeof( TA_DecodingParam ) );

   /* freeDecodingParam can be safely called from this point. */

   retCode = TA_StreamAccessGetString( streamAccess, &tempString );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   param->webSiteServer = TA_StringToChar( tempString );

   retCode = TA_StreamAccessGetString( streamAccess, &tempString );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   param->uirPrefix = TA_StringToChar( tempString );

   retCode = TA_StreamAccessGetString( streamAccess, &tempString );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   param->uirSuffix = TA_StringToChar( tempString );

   retCode = TA_StreamAccessGetInt32( streamAccess, (unsigned int *)&param->flags );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   retCode = TA_StreamAccessGetByte( streamAccess, &param->param8_1 );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   retCode = TA_StreamAccessGetByte( streamAccess, &param->param8_2 );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   retCode = TA_StreamAccessGetByte( streamAccess, &param->param8_3 );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   retCode = TA_StreamAccessGetByte( streamAccess, &param->param8_4 );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   retCode = TA_StreamAccessGetInt16( streamAccess, &param->param16_1 );
   if( retCode != TA_SUCCESS )
      goto Exit_allocDecodingParam;

   retCode = TA_StreamAccessGetInt16( streamAccess, &param->param16_2 );

Exit_allocDecodingParam:
   if( retCode != TA_SUCCESS )
      freeDecodingParam( param );
   else
      *paramAllocated = param;

   return retCode;
}

/* Free a TA_DecodingParam */
static TA_RetCode freeDecodingParam( TA_DecodingParam *param )
{
   TA_StringCache *stringCache;

   if( param )
   {
      stringCache = TA_GetGlobalStringCache();

      if( param->webSiteServer )
         TA_StringFree( stringCache, TA_StringFromChar(param->webSiteServer) );

      if( param->uirPrefix )
         TA_StringFree( stringCache, TA_StringFromChar(param->uirPrefix) );

      if( param->uirSuffix )
         TA_StringFree( stringCache, TA_StringFromChar(param->uirSuffix) );

      TA_Free(  param );
   } 

   return TA_SUCCESS;
}

static TA_RetCode buildDictFromWebSite( TA_YahooIdx *idx, TA_CountryId countryId, int buildFromScratch )
{
   TA_RetCode retCode;
   TA_WebPage *webPage;
   TA_StreamAccess *access;
   unsigned int i, nbTableToSkip;

   TA_String *catString;
   TA_String *symString;
   TA_StringCache *stringCache;

   TA_TableParseOpaqueData opaqueData;

   stringCache = TA_GetGlobalStringCache();
   catString = NULL;
   symString = NULL;

   opaqueData.countryId = countryId;
   opaqueData.idx = idx;
   opaqueData.idx_remote = NULL;

   /* Identify the server. */
   switch( countryId )
   {
   case TA_Country_ID_US:
   case TA_Country_ID_CA:
      opaqueData.serverName = "biz.yahoo.com";
      nbTableToSkip = 2;
      break;
   default:
      /* All european index are integrated together, so just
       * use the UK server.
       */
      opaqueData.serverName = "uk.biz.yahoo.com";
      nbTableToSkip = 4;
      break;
   }

   /* Identify the top page. */
   switch( countryId )
   {
   case TA_Country_ID_US:
   case TA_Country_ID_CA:
      opaqueData.topIndex = "/i/";
      break;
   case TA_Country_ID_UK:
      opaqueData.topIndex = "/p/uk/cpi/index.html";
      break;
   case TA_Country_ID_DE: /* Germany */
      opaqueData.topIndex = "/p/de/cpi/index.html";
      break;
   case TA_Country_ID_DK: /* Denmark */
      opaqueData.topIndex = "/p/dk/cpi/index.html";
      break;
   case TA_Country_ID_ES: /* Spain */
      opaqueData.topIndex = "/p/es/cpi/index.html";
      break;
   case TA_Country_ID_FR: /* France */
      opaqueData.topIndex = "/p/fr/cpi/index.html";
      break;
   case TA_Country_ID_IT: /* Italy */
      opaqueData.topIndex = "/p/it/cpi/index.html";
      break;
   case TA_Country_ID_SE: /* Sweden */
      opaqueData.topIndex = "/p/se/cpi/index.html";
      break;
   case TA_Country_ID_NO: /* Norway */
      opaqueData.topIndex = "/p/no/cpi/index.html";
      break;
   default:
      opaqueData.topIndex = "/i/";
      break;
   }

   retCode = TA_WebPageAlloc( opaqueData.serverName,
                              opaqueData.topIndex,
                              NULL, NULL,
                              &webPage, 2 );

   if( retCode != TA_SUCCESS )
      return retCode;

   /* From now on, should exit the function by jumping
    * at the end.
    */

   /* Watch-out... this is indirectly a recursive call, but it
    * will be fine as long this remains TA_USE_REMOTE_CACHE only.
    * The remote US index is used for speed optimization to find
    * category when working to build the US/CA index.
    */
   switch( countryId )
   {
   case TA_Country_ID_US:
   case TA_Country_ID_CA:
      retCode = TA_YahooIdxAlloc( TA_Country_ID_US,
                                  &opaqueData.idx_remote,
                                  TA_USE_REMOTE_CACHE,
                                  NULL, NULL, NULL );

      if( retCode != TA_SUCCESS )
         goto Exit_buildPageList;
   }

   /* Find the table at the top representing the 
    * top index.
    */
   access = TA_StreamAccessAlloc( webPage->content );

   /* Skip first the non-meaningful tables. */
   for( i=0; i < nbTableToSkip; i++ )
   {
      retCode = TA_StreamAccessSkipHTMLTable( access );
      if( retCode != TA_SUCCESS )
         goto Exit_buildPageList;
   }

   /* Now go for the table we are looking for. */   
   retCode = TA_StreamAccessGetHTMLTable( access, 100, processTopIndex, &opaqueData );
   if( retCode != TA_SUCCESS )
      goto Exit_buildPageList;

   /* Add hard-coded index. */  
   for( i=0; i < hardCodedIndiceSize; i++ )
   {
      if( hardCodedIndice[i].countryId == countryId )
      {
         symString = TA_StringAlloc( stringCache, hardCodedIndice[i].symbolLib   );

         if( !symString )
         {
            retCode = TA_ALLOC_ERR;
            goto Exit_buildPageList;
         }
                      
         catString = TA_StringAlloc( stringCache, hardCodedIndice[i].categoryLib );

         if( !catString )
         {
            retCode = TA_ALLOC_ERR;
            goto Exit_buildPageList;
         }

         retCode = addYahooSymbol( idx, catString, symString );
         if( retCode != TA_SUCCESS )
            goto Exit_buildPageList;

         if( symString )   
         {
            TA_StringFree( stringCache, symString );
            symString = NULL;
         }

         if( catString )
         {
            TA_StringFree( stringCache, catString );
            catString = NULL;
         }
      }
   }

Exit_buildPageList:

   if( symString )   
      TA_StringFree( stringCache, symString );

   if( catString )
      TA_StringFree( stringCache, catString );

   TA_StreamAccessFree( access );
   TA_WebPageFree( webPage );
   
   if( opaqueData.idx_remote )       
      TA_YahooIdxFree( opaqueData.idx_remote );

   return retCode;
}

static TA_RetCode processTopIndex( unsigned int line,
                                   unsigned int column,
                                   const char *data,
                                   const char *href,
                                   void *opaqueData)
{
   TA_TableParseOpaqueData *tableParseInfo;
   TA_RetCode retCode;
   TA_WebPage *webPage;
   TA_StreamAccess *access;
   unsigned int i, nbTableToSkip;

   (void)data; /* Get ride of compiler warning. */
   (void)column; /* Get ride of compiler warning. */
   (void)line; /* Get ride of compiler warning. */

   webPage = NULL;
   access = NULL;

   retCode = TA_SUCCESS;

   if( *href != '\0' )
   {
      tableParseInfo = (TA_TableParseOpaqueData *)opaqueData;

      printf( "************* Processing Top Index [%s]\n", href );
      retCode = TA_WebPageAlloc( tableParseInfo->serverName,
                                 href,
                                 NULL, NULL,
                                 &webPage, 2 );

      if( retCode != TA_SUCCESS )
         goto Exit_processTopIndex;

      retCode = addSymbolsFromWebPage( webPage, opaqueData );

      /* retCode = TA_FINISH_TABLE; */

      if( retCode != TA_SUCCESS )
         goto Exit_processTopIndex;

      /* Now process all the "sub index" pages */
      access = TA_StreamAccessAlloc( webPage->content );
      if( retCode != TA_SUCCESS )
         goto Exit_processTopIndex;

      /* Skip first the non-meaningful tables. */
      switch( tableParseInfo->countryId )
      {
      case TA_Country_ID_US:
      case TA_Country_ID_CA:
         nbTableToSkip = 3;
         break;
      default:
         nbTableToSkip = 8;
         break;
      }

      for( i=0; i < nbTableToSkip; i++ )
      {
         retCode = TA_StreamAccessSkipHTMLTable( access );
         if( retCode != TA_SUCCESS )
            goto Exit_processTopIndex;
      }

      /* Now go for the table we are looking for. */
      retCode = TA_StreamAccessGetHTMLTable( access, 100, processIndex, opaqueData );
      if( retCode == TA_END_OF_STREAM )
      {
         /* Do not consider as an error as it may
          * happen on certain index page.
          */
         retCode = TA_SUCCESS; 
      }

      if( retCode != TA_SUCCESS )
         goto Exit_processTopIndex;
   }

Exit_processTopIndex:
   if( access )
      TA_StreamAccessFree( access );

   if( webPage )   
      TA_WebPageFree( webPage );

   return retCode;
}

static TA_RetCode processIndex( unsigned int line,
                                unsigned int column,
                                const char *data,
                                const char *href,
                                void *opaqueData)
{
   TA_TableParseOpaqueData *tableParseInfo;
   TA_RetCode retCode;
   TA_WebPage *webPage;

   (void)data; /* Get ride of compiler warning. */
   (void)column; /* Get ride of compiler warning. */
   (void)line; /* Get ride of compiler warning. */

   webPage = NULL;

   retCode = TA_SUCCESS;

   if( *href != '\0' )
   {
      tableParseInfo = (TA_TableParseOpaqueData *)opaqueData;

      /* Trap the case where the table does not appear to be right.
       * This may happen if there is no additional index
       * index on this page (Typically the page Y,Z)
       */
      switch( tableParseInfo->countryId )
      {
      case TA_Country_ID_US:
      case TA_Country_ID_CA:
         if( strncmp( "/i/", href, 3 ) != 0 )
         {
            retCode = TA_FINISH_TABLE;
            goto Exit_processIndex;
         }
         break;
      default:         
         if( (strncmp( "/p/", href, 3 ) != 0) || (strncmp( "uk/cpi/cpi", &href[3], 10) != 0) )
         {
            retCode = TA_FINISH_TABLE;
            goto Exit_processIndex;
         }
         break;
      }

      #if 0
      /* Temporary just to debug */
      if( *(&href[3]) != 'i' )
         goto Exit_processIndex;
      #endif

      printf( "Processing web page [%s]                                 \n", &href[3] );

      retCode = TA_WebPageAlloc( tableParseInfo->serverName,
                                 href,
                                 NULL, NULL,
                                 &webPage, 2 );

      if( retCode != TA_SUCCESS )
         goto Exit_processIndex;

      retCode = addSymbolsFromWebPage( webPage, opaqueData );
      if( retCode != TA_SUCCESS )
         goto Exit_processIndex;
   }

Exit_processIndex:
   if( webPage )
      TA_WebPageFree( webPage );

   return retCode;
}

static TA_RetCode addSymbolsFromWebPage( TA_WebPage *webPage, void *opaqueData )
{
   TA_RetCode retCode;
   unsigned int i;

   TA_TableParseOpaqueData *tableParseInfo;
   TA_StreamAccess *access;

   if( !webPage )
      return TA_BAD_PARAM;

   access = TA_StreamAccessAlloc( webPage->content );
   if( !access )
      return TA_ALLOC_ERR;

   /* Position on the table containing all the symbols. */
   tableParseInfo = (TA_TableParseOpaqueData *)opaqueData;
   switch( tableParseInfo->countryId )
   {
   case TA_Country_ID_US:
   case TA_Country_ID_CA:
      retCode = TA_StreamAccessSearch( access, "value=search" );
      if( retCode != TA_SUCCESS )
      {
         TA_StreamAccessFree( access );
         return retCode;
      }
      break;

   default:
      for( i=0; i < 7; i++ )
      {
         retCode = TA_StreamAccessSkipHTMLTable( access );
         if( retCode != TA_SUCCESS )
         {
            TA_StreamAccessFree( access );
            return retCode;
         }
      }

      for( i=0; i < 2; i++ )
      {
         retCode = TA_StreamAccessSearch( access, "<p>" );
         if( retCode != TA_SUCCESS )
         {
            TA_StreamAccessFree( access );
            return retCode;
         }
      }
      break;
   }

   /* Now parse the symbol table. */
   retCode = TA_StreamAccessGetHTMLTable( access, 100, addTheSymbolFromWebPage, opaqueData );

   TA_StreamAccessFree( access );
   return retCode;
}

static TA_RetCode addTheSymbolFromWebPage( unsigned int line,
                                           unsigned int column,
                                           const char *data,
                                           const char *href,
                                           void *opaqueData)
{
   TA_TableParseOpaqueData *tableParseInfo;
   TA_RetCode retCode;
   TA_String *category;
   TA_String *symbol;
   TA_StringCache *stringCache;
   const char *abbrev;
   int allowOnlineProcessing;

   (void)line; /* Get ride of compiler warning. */
   (void)href;

   if( column == 1 )
   {
      if( data )
      {
         tableParseInfo = (TA_TableParseOpaqueData *)opaqueData;

         /* Convert from Yahoo! classification to the TA-LIB classification. */
         category = NULL;
         symbol = NULL;

         /* Go online only when processing US/CA stocks. All other
          * country have extension allowing to identify the
          * exchange so there is no need to do additional online
          * processing to identify the exchange.
          */ 
         switch( tableParseInfo->countryId )
         {
         case TA_Country_ID_US:
         case TA_Country_ID_CA:
            allowOnlineProcessing = 1;
            break;
         default:
            allowOnlineProcessing = 0;
         }

         /* When online allowed and the remote index is available,
          * cache the US index to quickly identify the category.          
          * For audit purpose, still go online for randomly selected
          * symbols. We try to not go online for ALL symbols for
          * performance reason.
          */       
         if( allowOnlineProcessing && (tableParseInfo->idx_remote != NULL) && ((rand()%100)>5) )
         {
            retCode = findStringFromCacheUsingYahooName( tableParseInfo->idx_remote,
                                                         data, &category, &symbol );
         }
         else
            retCode = TA_INVALID_SECURITY_SYMBOL;
         
         if( retCode != TA_SUCCESS )
         {
            retCode = TA_AllocStringFromYahooName( &defaultMarketDecoding,
                                                   data, &category, &symbol,
                                                   allowOnlineProcessing );

            if( retCode == TA_OBSOLETED_SYMBOL )
            {
               /* Symbol is obsolete, just ignore it with no fatal error. */
               printf( "Warning: This symbol is no longuer in use [%s]\n", data );
               return TA_SUCCESS;
            }

            if( retCode != TA_SUCCESS )
            {
               /* Just a warning for now... */
               printf( "Warning. Does not recognize exchange for [%s] (%d)\n", data, retCode );
               return TA_SUCCESS;
            }

         }

         stringCache = TA_GetGlobalStringCache();

         /* Add only if of the requested country. */
         abbrev = TA_CountryIdToAbbrev(tableParseInfo->countryId);
         if( strncmp( abbrev, TA_StringToChar(category), 2 ) == 0 )
         {
            printf( "[%s][%s]                              \r",
                    TA_StringToChar(category),
                    TA_StringToChar(symbol) );
            retCode = addYahooSymbol( tableParseInfo->idx, 
                                      category, symbol );
         }

         TA_StringFree( stringCache, category );
         TA_StringFree( stringCache, symbol );

         if( retCode != TA_SUCCESS )
         {
            printf( "Warning: Failed to add symbol [%s]\n", data );
            return retCode;
         }
      }
   }

   return TA_SUCCESS;
}

static TA_RetCode buildIndexFromLocalCache( TA_YahooIdx *idx,
                                            TA_Timestamp *cacheTimeout )
{
   const char *cachePath;
   TA_YahooIdxHidden *idxHidden;
   char *buffer;
   TA_Stream *stream;
   FILE *in;
   TA_RetCode retCode;
   int pathLength;

   idxHidden = (TA_YahooIdxHidden *)idx->hiddenData;

   cachePath = TA_GetLocalCachePath();

   if( !cachePath )
      return TA_YAHOO_IDX_EXPIRED;
   
   /* Open the file */
   pathLength = strlen( cachePath );
   buffer = TA_Malloc( pathLength + 11 );
   if( pathLength )
   {
      sprintf( buffer, "%s%cy_%c%c.dat",
               cachePath,
               TA_SeparatorASCII(),
               tolower(idx->countryAbbrev[0]),
               tolower(idx->countryAbbrev[1]) );
   }
   else
   {
      sprintf( buffer, "y_%c%c.dat",
               tolower(idx->countryAbbrev[0]),
               tolower(idx->countryAbbrev[1]) );
   }

   /* printf( "Trying to open local [%s]\n", buffer ); */

   in = fopen( buffer, "rb" );
   TA_Free(  buffer );
   if( !in )
      return TA_YAHOO_IDX_EXPIRED;

   /* Create a stream from the file. */
   stream = TA_StreamAlloc();
   if( !stream )
      return TA_YAHOO_IDX_EXPIRED;
   retCode = TA_StreamAddFile( stream, in );
   fclose(in);
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( stream );
      return TA_YAHOO_IDX_EXPIRED;
   }

   /* Call buildIndexFromStream to build the tables. */
   retCode = buildIndexFromStream( idx, stream, cacheTimeout );
   TA_StreamFree( stream );
   if( retCode != TA_SUCCESS )
      return TA_YAHOO_IDX_EXPIRED;

   /* printf( "Using local cache for [%s]\n", idx->countryAbbrev ); */

   return TA_SUCCESS;
}

static TA_RetCode findStringFromCacheUsingYahooName( TA_YahooIdx *idx,
                                                     const char *data,                                           
                                                     TA_String **category,
                                                     TA_String **symbol )
{
    TA_StringCache *stringCache;
    TA_YahooCategory *yahooCategory;
    unsigned int i, j;

    if( !idx || !data || !category || !symbol )
       return TA_BAD_PARAM;
    
    /* Do nothing if the Yahoo! string contains a dot.
     * In that case, there is better way to figure out the
     * category.
     */
    if( strchr( data, '.' ) != NULL )
       return TA_INVALID_SECURITY_SYMBOL;
    
    for( i=0; i < idx->nbCategory; i++ )
    {
       yahooCategory = idx->categories[i];
       for( j=0; j < yahooCategory->nbSymbol; j++ )
       {
          if( strcmp(data, TA_StringToChar(yahooCategory->symbols[j])) == 0 )
          {
             stringCache = TA_GetGlobalStringCache();
             if( !stringCache )
                return TA_INTERNAL_ERROR(140);;

             *category = TA_StringDup( stringCache, yahooCategory->name );
             *symbol   = TA_StringDup( stringCache, yahooCategory->symbols[j] );
             return TA_SUCCESS;
          }
       }
    }

    return TA_INVALID_SECURITY_SYMBOL;
    
}
