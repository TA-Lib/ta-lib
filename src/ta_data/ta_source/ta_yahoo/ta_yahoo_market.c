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
 *  112400 MF   First version.
 *  062803 MF   Make parsing to find exchange/type more adaptive to 
 *              potential changes from Yahoo! market page format.
 *  102103 MF   Now recognize American Mutual Fund ticker.
 *  020104 MF   Fix mem leaks related to TA_TRACE_BEGIN/TA_TRACE_RETURN.
 */

/* Description:
 *   Extract market data from Yahoo!
 */

/**** Headers ****/
#include "ta_common.h"
#include "ta_string.h"
#include "ta_yahoo_priv.h"
#include "ta_network.h"
#include "ta_system.h"
#include "ta_trace.h"
#include "ta_country_info.h"
#include "ta_global.h"
#include "ta_magic_nb.h"
#include "sfl.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
typedef struct
{
   const char *extension;
   TA_CountryId countryId;
   const char *exchange;
   const char *type;
} TA_YahooExtension;

typedef struct
{   
   TA_YahooMarketPage      *marketPage;
   const TA_DecodingParam  *decodingParam;
} TA_MarketPageParseOpaqueData;

/**** Local functions declarations.    ****/
static TA_RetCode translateToYahooName( const TA_String *categoryName,
                                        const TA_String *symbolName,
                                        char *buffer,
                                        unsigned int maxBufferSize );

static TA_RetCode internalMarketPageAlloc( const TA_DecodingParam *decodingParam,
                                           const char *yahooName,
                                           TA_YahooMarketPage **allocatedMarketPage );

static TA_RetCode internalMarketPageFree( TA_YahooMarketPage *marketPage );

static TA_RetCode parseMarketPage( TA_StreamAccess *streamAccess,
                                   TA_YahooMarketPage *marketPage );

/**** Local variables definitions.     ****/
TA_FILE_INFO;

static TA_YahooExtension TA_YahooExtensionTable[] = 
{
   { "TO", TA_Country_ID_CA, "TSE",   "STOCK"  },  /* Toronto Stock Exchange */
   { "V",  TA_Country_ID_CA, "CDNX",  "STOCK"  },  /* Canadian Venture Exchange (Vancouver) */
   { "M",  TA_Country_ID_CA, "MSE",   "STOCK"  },  /* Montreal Stock Exchange */

   { "OB", TA_Country_ID_US, "OTCBB", "STOCK"  },  /* Over-the-counter, Bulletin Board */
   { "PK", TA_Country_ID_US, "OTC",   "STOCK"  },  /* Over-the-counter, Pink Sheet */

   { "L",  TA_Country_ID_UK, "LSE",   "STOCK"  },  /* London Stock Exchange */

   { "VI", TA_Country_ID_AT, "WBAG",  "STOCK"  },  /* Austria - Vienna Stock Exchange */

   { "CO", TA_Country_ID_DK, "XCSE",  "STOCK"  },  /* Copenhagen Stock Exchange */

   { "PA", TA_Country_ID_FR, "SBF",   "STOCK"  },  /* Bourse De Paris */
   { "P",  TA_Country_ID_FR, "SBF",   "STOCK"  },  /* Bourse De Paris */

   { "BE", TA_Country_ID_DE, "BSE",   "STOCK"  },  /* Berlin Stock Exchange */
   { "BM", TA_Country_ID_DE, "BWB",   "STOCK"  },  /* Bremen Stock Exchange */
   { "D",  TA_Country_ID_DE, "RWB",   "STOCK"  },  /* Dusseldorf Stock Exchange */
   { "F",  TA_Country_ID_DE, "FRA",   "STOCK"  },  /* Frankfurt Stock Exchange */
   { "DE", TA_Country_ID_DE, "XETRA", "STOCK"  },  /* XETRA Stock Exchange */
   { "H",  TA_Country_ID_DE, "HAM",   "STOCK"  },  /* Hamberg Stock Exchange */
   { "HA", TA_Country_ID_DE, "HAN",   "STOCK"  },  /* Hannover Stock Exchange */
   { "MU", TA_Country_ID_DE, "BBAG",  "STOCK"  },  /* Bavarian Stock Exchange (Munich) */
   { "SG", TA_Country_ID_DE, "BSAG",  "STOCK"  },  /* Stuttgart Stock Exchange */

   { "MI", TA_Country_ID_IT, "BI",    "STOCK"  },  /* Borsa Italia - Milan Stock Exchange */

   { "NL", TA_Country_ID_AT, "AEX",   "STOCK"  },  /* Amsterdam Stock Exchange */

   { "OL", TA_Country_ID_NO, "OSE",   "STOCK"  },  /* Oslo Stock Exchange */

   { "BC", TA_Country_ID_ES, "BSE",   "STOCK"  },  /* Barcelona Stock Exchange */
   { "BI", TA_Country_ID_ES, "BIL",   "STOCK"  },  /* Bilbao Stock Exchange */
   { "MF", TA_Country_ID_ES, "MEFF",  "FUTURE" },  /* Madrid Fixed Income and Derivative Market */
   { "MC", TA_Country_ID_ES, "MEFF",  "STOCK"  },  /* Madrid Fixed Income and Derivative Market */
   { "MA", TA_Country_ID_ES, "BDM",   "STOCK"  },  /* Madrid Stock Exchange */

   { "ST", TA_Country_ID_SE, "OMX",   "STOCK"  }   /* Stockholm Stock Exchange */
};

#define NB_YAHOO_EXCHANGE_EXTENSION (sizeof(TA_YahooExtensionTable)/sizeof(TA_YahooExtension))

/**** Global functions definitions.   ****/
TA_RetCode TA_AllocStringFromLibName( const TA_String *category,
                                      const TA_String *symbol,
                                      TA_String **allocatedYahooName )
{
   TA_PROLOG

   TA_RetCode retCode;
   TA_StringCache *stringCache;
   char buffer[200];

   TA_TRACE_BEGIN(  TA_AllocStringFromLibName );
   buffer[199] = '\0'; /* Just to be safe. */

   /* Translate the category/symbol into the yahoo! name. */
   retCode = translateToYahooName( category, symbol, &buffer[0], 199 );
   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   stringCache = TA_GetGlobalStringCache();
   *allocatedYahooName = TA_StringAlloc( stringCache, buffer );
   if( !*allocatedYahooName )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }
   
   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_AllocStringFromYahooName( TA_DecodingParam *marketDecodingParam,
                                        const char *yahooSymbol,
                                        TA_String **allocatedCategoryName,
                                        TA_String **allocatedSymbolName,
                                        unsigned int allowOnlineProcessing )
{
   TA_PROLOG

   TA_RetCode retCode;

   const char   *symbol;
   unsigned int symbolLength;
   const char   *ext;

   const char *countryAbbrev;
   const char *exchangeString;
   const char *typeString;

   unsigned int i;
   char *tempBuffer;
   TA_String *allocCategory;
   TA_String *allocSymbol;
   TA_StringCache *stringCache;
   TA_CountryId countryId;

   TA_YahooMarketPage *allocatedMarketPage;

   TA_TRACE_BEGIN( TA_AllocStringFromYahooName );

   /* Validate parameter */
   if( !marketDecodingParam || !yahooSymbol ||
       !allocatedCategoryName || !allocatedSymbolName )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   stringCache = TA_GetGlobalStringCache();

   /* Set a pointer on where the symbol start. */
   symbol = yahooSymbol;
  
   /* Size of the symbol. */
   symbolLength = getstrfldlen( symbol, 0, 0, "." );
   if( symbolLength < 2 )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }
   else
      symbolLength--;

   /* The 3 strings forming the final Category string. */
   countryAbbrev  = NULL;
   exchangeString = NULL;
   typeString     = NULL;

   /* Identify if there is an extension. */
   ext = strchr( symbol, '.' );
   if( ext )
   {
      ext++;
      if( *ext == '\0' )
         ext = NULL;
   }

   /* If ext != NULL, ext points on first char of the extension. */
   
   if( ext )
   {
      /* Identify known USA/CAN extension. */
      for( i=0; i < NB_YAHOO_EXCHANGE_EXTENSION; i++ )
      {
         if( lexcmp( TA_YahooExtensionTable[i].extension, ext ) == 0 )
         {
            countryId = TA_YahooExtensionTable[i].countryId;           
            countryAbbrev  = TA_CountryIdToAbbrev( countryId );
            exchangeString = TA_YahooExtensionTable[i].exchange;
            typeString     = TA_YahooExtensionTable[i].type;
            break; /* Exit the loop */
         }
      }

      /* Unknown extension, let's use the whole thing
       * as the symbol and keep going as if nothing
       * happened.
       */
      if( !countryAbbrev )
         ext = NULL; /* No known extension. */
   }

   /* Trap the american mutual fund who are always
    * 5 characters (with the last character being a 'X').
    */
   if( (strlen(yahooSymbol) == 5) && (yahooSymbol[4] == 'X') )
   {
      countryId      = TA_Country_ID_US;           
      countryAbbrev  = TA_CountryIdToAbbrev( countryId );
      exchangeString = "NASDAQ";
      typeString     = "FUND";
   }

   if( !exchangeString )
   {
      /* If online access is not allowed, and the 
       * symbol does not have a known extension, it
       * is not possible to identify the exchange.
       *
       * With online access, the exchange can be
       * found by doing further investigation on the
       * Yahoo! web sites.
       */
      if( !allowOnlineProcessing )
      {
         TA_TRACE_RETURN(TA_INVALID_SECURITY_EXCHANGE);
      }

      /* OK, we need to proceed by extracting the info
       * from Yahoo! web sites.
       */
      retCode = internalMarketPageAlloc( marketDecodingParam,
                                         yahooSymbol,
                                         &allocatedMarketPage );

      if( retCode != TA_SUCCESS )
      {
         TA_TRACE_RETURN(retCode);
      }

      TA_ASSERT_DEBUG( allocatedMarketPage->exchange != NULL );
      TA_ASSERT_DEBUG( allocatedMarketPage->type != NULL );

      /* All these string pointer are globals. So the allocatedMarketPage
       * can be freed and the member-poitners are still valid.
       */
      countryAbbrev  = TA_CountryIdToAbbrev( allocatedMarketPage->countryId );
      exchangeString = allocatedMarketPage->exchange;
      typeString     = allocatedMarketPage->type;

      internalMarketPageFree( allocatedMarketPage );
   }   
   
   TA_ASSERT_DEBUG( typeString     != NULL );
   TA_ASSERT_DEBUG( exchangeString != NULL );
   TA_ASSERT_DEBUG( countryAbbrev  != NULL );

   /* Build the Category string into a buffer. */
   tempBuffer = TA_Malloc( strlen( countryAbbrev  )  +
                           strlen( exchangeString ) +
                           strlen( typeString )     + 3 );

   sprintf( tempBuffer, "%s.%s.%s", countryAbbrev, exchangeString, typeString );

   /* Allocate the Category string. */
   allocCategory = TA_StringAlloc_UC( stringCache, tempBuffer );

   TA_Free( tempBuffer );

   if( !allocCategory )
   {
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Allocate the symbol string. */
   allocSymbol = TA_StringAllocN_UC( stringCache, symbol, symbolLength );

   if( !allocSymbol )
   {
      TA_StringFree( stringCache, allocCategory );
      TA_TRACE_RETURN( TA_ALLOC_ERR );
   }

   /* Everything went fine, return the info to the caller. */
   *allocatedCategoryName = allocCategory;
   *allocatedSymbolName   = allocSymbol;

   TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_YahooMarketPageAlloc( const TA_DecodingParam *decodingParam,
                                    const TA_String *categoryName,
                                    const TA_String *symbolName,
                                    TA_YahooMarketPage **allocatedMarketPage )
{
   TA_RetCode retCode;
   char buffer[200];

   buffer[199] = '\0'; /* Just to be safe. */

   /* Translate the category/symbol into the yahoo! name. */
   retCode = translateToYahooName( categoryName, symbolName, &buffer[0], 199 );
   if( retCode != TA_SUCCESS )
      return retCode;

   return internalMarketPageAlloc( decodingParam,
                                   buffer,
                                   allocatedMarketPage );
}


TA_RetCode TA_YahooMarketPageFree( TA_YahooMarketPage *marketPage )
{
   return internalMarketPageFree( marketPage );
}

TA_RetCode TA_WebPageAllocFromYahooName( const TA_DecodingParam *decodingParam,
                                         const char *yahooName,
                                         TA_WebPage **allocatedWebPage )
{
   TA_PROLOG
   TA_RetCode retCode;
   char webSitePage[300];
   unsigned int prefixLength, suffixLength, symbolLength, i;
   const char *webSiteAddr;
   const char *uirPrefix, *uirSuffix;
   TA_WebPage *webPage;

   TA_TRACE_BEGIN( TA_WebPageAllocFromYahooName );

   retCode = TA_INTERNAL_ERROR(117);

   if( !decodingParam || !yahooName || !allocatedWebPage )
   {
      TA_TRACE_RETURN( TA_BAD_PARAM );
   }

   webSiteAddr  = decodingParam->webSiteServer;
   TA_ASSERT( webSiteAddr != NULL );

   uirPrefix = decodingParam->uirPrefix;
   TA_ASSERT( uirPrefix != NULL );
   prefixLength = strlen( uirPrefix );

   uirSuffix = decodingParam->uirSuffix;
   TA_ASSERT( uirSuffix != NULL );
   suffixLength = strlen( uirSuffix );
   
   symbolLength = strlen( yahooName );
   if( (symbolLength + suffixLength + prefixLength) >= 299 )
   {
      TA_TRACE_RETURN( TA_INVALID_SECURITY_EXCHANGE );
   }

   sprintf( webSitePage, "%s%s%s", uirPrefix, yahooName, uirSuffix );

   /* Get the Web Page */   
   for( i=0; i < 10; i++ )
   {
      retCode = TA_WebPageAlloc( webSiteAddr,
                                 webSitePage,
                                 NULL, NULL, &webPage, 10 );

      if( retCode == TA_SUCCESS )
         break;
      else
      {         
         /* Yahoo! is may be slow, let's sleep 1 minute */
         TA_Sleep( 60 ); 
      }
   }

   if( retCode != TA_SUCCESS )
   {
      TA_TRACE_RETURN( retCode );
   }

   *allocatedWebPage = webPage;
   TA_TRACE_RETURN( TA_SUCCESS );
}

/**** Local functions definitions.     ****/
static TA_RetCode translateToYahooName( const TA_String *categoryName,
                                        const TA_String *symbolName,
                                        char *buffer,
                                        unsigned int maxBufferSize )
{
   TA_CountryId countryId;
   unsigned int i, symbolLength, xchangeLength, extLength, typeLength;
   const char *symbol;
   const char *xchange;
   const char *type;
   const char *category;
   const char *ext;

   category = TA_StringToChar(categoryName);

   /* Identify the symbol and its length. */
   symbol = TA_StringToChar(symbolName);

   symbolLength = strlen( symbol );
   if( symbolLength >= maxBufferSize )
      return TA_INVALID_SECURITY_SYMBOL;

   strcpy( buffer, symbol );

   /* Trap special case where the symbol/category is for
    * one of the hard coded indice.
    */

   for( i=0; i < hardCodedIndiceSize; i++ )
   {
      if( (strcmp( hardCodedIndice[i].symbolLib, TA_StringToChar( symbolName ) ) == 0) &&
          (strcmp( hardCodedIndice[i].categoryLib, TA_StringToChar( categoryName ) ) == 0) )
      {
         strncpy( buffer, hardCodedIndice[i].symbolYahoo, maxBufferSize );
         return TA_SUCCESS;
      }
   }

   /* Identify if an extenstion must be appended
    * to the symbol.
    * At the same time, verify the country. 
    */
   countryId = TA_CountryAbbrevToId( TA_StringToChar(categoryName) );
   switch( countryId )
   {
   case TA_Country_ID_CA: /* Canada */
   case TA_Country_ID_US: /* United States */
   case TA_Country_ID_UK: /* United Kingdom */
   case TA_Country_ID_DE: /* Germany */
   case TA_Country_ID_DK: /* Denmark */
   case TA_Country_ID_ES: /* Spain */
   case TA_Country_ID_FR: /* France */
   case TA_Country_ID_IT: /* Italy */
   case TA_Country_ID_SE: /* Sweden */
   case TA_Country_ID_NO: /* Norway */
      /* Get the length of the second field of the category.
       * This field must exist, else an error is returned.
       */
      xchangeLength = getstrfldlen( category, 1, 0, "." ) -1;
      if( xchangeLength < 2 )
         return TA_INVALID_SECURITY_EXCHANGE;
      xchange = strchr( category, '.' );
      if( !xchange || (*xchange == '\0') )
         return TA_INVALID_SECURITY_EXCHANGE;
      xchange++;
      if( *xchange == '\0' )
         return TA_INVALID_SECURITY_EXCHANGE;

      /* Get the length of the third field of the category.
       * This field must exist, else an error is returned.
       */
      type = strchr( xchange, '.' );
      if( !type || (type == xchange) || (*type == '\0') )
         return TA_INVALID_SECURITY_TYPE;
      type++;
      if( *type == '\0' )
         return TA_INVALID_SECURITY_TYPE;
      typeLength = strlen( type );

      /* Look for the possible extension corresponding to
       * this exchange. If found, append it to the symbol.
       */
      for( i=0; i < NB_YAHOO_EXCHANGE_EXTENSION; i++ )
      {         
         if( (TA_YahooExtensionTable[i].countryId == countryId) &&
             (lexncmp( TA_YahooExtensionTable[i].exchange, xchange, xchangeLength ) == 0) &&
             (lexncmp( TA_YahooExtensionTable[i].type, type, typeLength ) == 0) )
         {
            ext = TA_YahooExtensionTable[i].extension;
            extLength = strlen( ext );
            if( extLength+symbolLength+1 > maxBufferSize-1 )
               return TA_INVALID_SECURITY_EXCHANGE;
            buffer[symbolLength] = '.';
            symbolLength++;
            strcpy( &buffer[symbolLength], ext );
            break; /* Exit the loop */
         }
      }
      break;
   default:
      return TA_INVALID_SECURITY_COUNTRY;
   }

   /* At this point, buffer contains the name as used by Yahoo! web site. */

   return TA_SUCCESS;
}

                                  
static TA_RetCode internalMarketPageAlloc( const TA_DecodingParam *decodingParam,
                                           const char *yahooName,
                                           TA_YahooMarketPage **allocatedMarketPage )
{
   TA_RetCode retCode;
   TA_WebPage *webPage;
   TA_YahooMarketPage *marketPage;
   TA_StreamAccess *streamAccess;

   retCode = TA_WebPageAllocFromYahooName( decodingParam,
                                           yahooName,
                                           &webPage );
                                              
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Allocate the structure who will contained the extracted data */
   marketPage = (TA_YahooMarketPage *)TA_Malloc( sizeof( TA_YahooMarketPage ) );
   if( !marketPage )
   {
      TA_WebPageFree( webPage );
      return TA_ALLOC_ERR;
   }
   memset( marketPage, 0, sizeof( TA_YahooMarketPage ) );

   marketPage->magicNb = TA_MARKET_PAGE_MAGIC_NB;

   /* From this point, internalMarketPageFree can be safely called. */
    
   /* Extract the data by parsing the Web Page. */
   streamAccess = TA_StreamAccessAlloc( webPage->content );
   retCode = parseMarketPage( streamAccess, marketPage );

   if( retCode != TA_SUCCESS )
   {
      TA_StreamAccessFree( streamAccess );
      TA_WebPageFree( webPage );
      internalMarketPageFree( marketPage );
      return retCode;
   }

   retCode = TA_StreamAccessFree( streamAccess );
   if( retCode != TA_SUCCESS )
   {
      TA_WebPageFree( webPage );
      internalMarketPageFree( marketPage );
      return retCode;
   }

   retCode = TA_WebPageFree( webPage );
   if( retCode != TA_SUCCESS )
   {
      internalMarketPageFree( marketPage );
      return retCode;
   }
   
   /* Success. Return the result to the caller. */  
   *allocatedMarketPage = marketPage;

   return TA_SUCCESS;
}

static TA_RetCode internalMarketPageFree( TA_YahooMarketPage *marketPage )
{
   TA_StringCache *stringCache;

   if( marketPage )
   {
      if( marketPage->magicNb != TA_MARKET_PAGE_MAGIC_NB )
         return TA_BAD_OBJECT;

      if( marketPage->name )
      {
         stringCache = TA_GetGlobalStringCache();
         TA_StringFree( stringCache, marketPage->name );
      }

      TA_Free(  marketPage );
   }

   return TA_SUCCESS;
}

static TA_RetCode parseMarketPage( TA_StreamAccess *streamAccess,
                                   TA_YahooMarketPage *marketPage )
{
   TA_RetCode retCode;
   TA_StreamAccess *searchStartPoint;
   int i;
   #define NB_EXCHANGE_STRING 5
   static const char *exchangeString[NB_EXCHANGE_STRING][3] = 
   {
      {"(NASDAQNM", "NASDAQ", "STOCK"},
      {"(NASDAQSC", "NASDAQ", "FUND" },
      {"(NASDAQ",   "NASDAQ", "FUND" },
      {"(NYSE",     "NYSE",   "STOCK"},
      {"(AMEX",     "AMEX",   "STOCK"},
   };

   /* This parsing handle only US pages */
   marketPage->countryId = TA_Country_ID_US;

   /* Identify the exchange and type */
   searchStartPoint = TA_StreamAccessAllocCopy( streamAccess );
   for( i=0; i < NB_EXCHANGE_STRING; i++ )
   {
      retCode = TA_StreamAccessSearch( searchStartPoint, exchangeString[i][0] );
      if( retCode == TA_SUCCESS )
      { 
         marketPage->exchange  = exchangeString[i][1];
         marketPage->type      = exchangeString[i][2];
         break; /* Exchange identified... exit loop */
      }
   }
   TA_StreamAccessFree(searchStartPoint);

   if( i == NB_EXCHANGE_STRING )
   {
      /* Default unknown exchange */
      marketPage->exchange  = "OTHER";
      marketPage->type      = "OTHER";

      /* Detect case where the symbol is no longer valid. */
      searchStartPoint = TA_StreamAccessAllocCopy( streamAccess );
      retCode = TA_StreamAccessSearch( searchStartPoint, "is no longer valid" );
      TA_StreamAccessFree(searchStartPoint);
      if( retCode == TA_SUCCESS )
         return TA_OBSOLETED_SYMBOL;

      /* Detect case where the symbol is no longer valid. */
      searchStartPoint = TA_StreamAccessAllocCopy( streamAccess );
      retCode = TA_StreamAccessSearch( searchStartPoint, "Changed Ticker Symbol" );
      TA_StreamAccessFree(searchStartPoint);
      if( retCode == TA_SUCCESS )
         return TA_OBSOLETED_SYMBOL;

      /* Detect case where the symbol is no longer valid. */
      searchStartPoint = TA_StreamAccessAllocCopy( streamAccess );
      retCode = TA_StreamAccessSearch( searchStartPoint, "is not a valid ticker symbol" );
      TA_StreamAccessFree(searchStartPoint);
      if( retCode == TA_SUCCESS )
         return TA_OBSOLETED_SYMBOL;

      /* Detect case where the symbol is no longer valid. */
      searchStartPoint = TA_StreamAccessAllocCopy( streamAccess );
      retCode = TA_StreamAccessSearch( searchStartPoint, "Invalid Ticker Symbol" );
      TA_StreamAccessFree(searchStartPoint);
      if( retCode == TA_SUCCESS )
         return TA_OBSOLETED_SYMBOL;

      return TA_INVALID_SECURITY_EXCHANGE;
   }
   
   return TA_SUCCESS;
}

