#ifndef TA_YAHOO_PRIV_H
#define TA_YAHOO_PRIV_H

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_COUNTRY_INFO_H
   #include "ta_country_info.h"
#endif

#ifndef TA_NETWORK_H
   #include "ta_network.h"
#endif

#ifndef TA_GLOBAL_H
   #include "ta_global.h"
#endif

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_SOURCE_H
   #include "ta_source.h"
#endif

/* There is an attempts to make the decoding
 * a little more solid against change to
 * the format of the page providing the data.
 *
 * Some parametric information is stored in 
 * the index. Assuming the user is using an index
 * created with the latest software (like when read
 * from ta-lib.org), the user may be able to continue
 * to interpret the web page without updating its
 * software.
 *
 * Example: If Yahoo! decide to change the URL to access
 *          the data, 'webSiteServer' can reflect the
 *          change.
 */
typedef enum
{
  TA_NO_FLAG = 0x00
} TA_DecodeFlag;

typedef struct
{
   const char *webSiteServer;
   const char *uirPrefix;
   const char *uirSuffix;
   TA_DecodeFlag  flags;

   unsigned char  param8_1;
   unsigned char  param8_2;
   unsigned char  param8_3;
   unsigned char  param8_4;

   unsigned int param16_1; /* Only first 16 bits used. */
   unsigned int param16_2; /* Only first 16 bits used. */
} TA_DecodingParam;

typedef struct
{
   /* Basic parameters for extracting historical data. */
   TA_DecodingParam *historical;

   /* Basic parameters for extracting current market data. */
   TA_DecodingParam *market;

   /* Basic parameters for extracting security info. */
   TA_DecodingParam *info;
} TA_YahooDecodingParam;

/* Reflect the information that can be extracted from the
 * Quote page from Yahoo.
 * Contains mainly the Market data + some.
 *
 * This function is in "ta_yahoo_market.c"
 */
typedef struct
{
   unsigned int magicNb;
   TA_Libc *libHandle;

   TA_Real lastTrade;
   TA_Real bid;
   TA_Real ask;

   TA_Real open;
   TA_Real dayLow;
   TA_Real dayHigh;

   TA_Real previousClose; 

   TA_Integer volume;

   TA_String *name; /* Company name (long version). */
   TA_CountryId countryId;
   const char *exchange;
   const char *type;
} TA_YahooMarketPage;

TA_RetCode TA_YahooMarketPageAlloc( TA_Libc *libHandle,
                                    const TA_DecodingParam *marketDecodingParam,
                                    const TA_String *categoryName,
                                    const TA_String *symbolName,
                                    TA_YahooMarketPage **allocatedMarketPage );

TA_RetCode TA_YahooMarketPageFree( TA_YahooMarketPage *quotePage );

/* Allocate strings used by TA-LIB to classify each stock.
 *
 * On success, both strings must be freed with TA_StringFree.
 * On failure, no string are allocated.
 *
 * When the Yahoo! symbol is a known extension, this call
 * is quite fast, but in the case that there is no extension
 * further online investigation is required (fetching of webPage).
 *
 * Because this online investigation can be quite costly,
 * in terms of time, the parameter allowOnlineProcessing
 * must be true for allowing such investigation. 
 *
 * In practice, only US symbols needs further investigation
 * because many do not have an extension (by default I guess).
 *
 * On success, the allocated strings must be eventually freed
 * with TA_StringFree.
 *
 * These allocated strings are the one that must be used
 * when calling TA_YahooQuotePageAlloc.
 *
 * Example 1:
 *     if yahooSymbolString is "NT.TO", the allocated strings
 *     will be:
 *           *allocatedCategoryName = "CAN.TSE.STOCK"
 *           *allocatedSymbolName   = "NT"
 *
 *     CAN is the official abbreviation for Canada.
 *     TSE is the official abbreviation for the Toronto Stock Exchange.
 *     STOCK is the security type.
 *     NT  is the symbol representing Nortel.
 *
 *     More information on how categoy works can be found
 *     in the "Category Guideline" document.
 *
 * See "ta_yahoo_market.c"
 */
TA_RetCode TA_AllocStringFromYahooName( TA_Libc *libHandle,
                                        TA_DecodingParam *info,
                                        const char *yahooSymbol,
                                        TA_String **allocatedCategoryName,
                                        TA_String **allocatedSymbolName,
                                        unsigned int allowOnlineProcessing );


/* Providing a Category and Symbol name, the Yahoo! Symbol is rebuild. 
 * TA_AllocStringFromLibName() is the complement of the function 
 * TA_AllocStringFromYahooName(). Rebuilding from the TA-Lib Name is
 * speed efficient.
 *
 * See "ta_yahoo_market.c"
 */
TA_RetCode TA_AllocStringFromLibName( TA_Libc *libHandle,
                                      const TA_String *category,
                                      const TA_String *symbol,
                                      TA_String **allocatedYahooName );

/* Retreive a web page corresponding to the provided symbol.
 *
 * See "ta_yahoo_market.c"
 */
TA_RetCode TA_WebPageAllocFromYahooName( TA_Libc *libHandle,
                                         const TA_DecodingParam *info,
                                         const char *yahooName,
                                         TA_WebPage **allocatedWebPage );


/* Retreive the historical data corresponding to a 
 * categoryHandle, symbolHandle.
 * This is the functions that return back the data
 * to TA_LIB by calling TA_HistoryAddData.
 *
 * See "ta_yahoo_historical.c"
 */
TA_RetCode TA_GetHistoryDataFromWeb( TA_Libc *libHandle,
                                     TA_DataSourceHandle *handle,
                                     TA_CategoryHandle   *categoryHandle,
                                     TA_SymbolHandle     *symbolHandle,
                                     TA_Period            period,
                                     const TA_Timestamp  *start,
                                     const TA_Timestamp  *end,
                                     TA_Field             fieldToAlloc,
                                     TA_ParamForAddData  *paramForAddData );

/* Table of Hard coded indices for Yahoo! 
 * The table allows to translate from TA-LIB category/symbol to the
 * strng used by Yahoo! (and vice versa).
 */
typedef struct
{
   TA_CountryId countryId;
   const char *symbolYahoo;
   const char *symbolLib;
   const char *categoryLib;
} TA_HardCodedIndice;

extern TA_HardCodedIndice hardCodedIndice[];
extern const unsigned int hardCodedIndiceSize;

#endif
