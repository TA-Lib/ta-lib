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
#ifndef TA_DATA_H
#define TA_DATA_H

#include <stdio.h>

#ifndef TA_COMMON_H
    #include "ta_common.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int TA_UDBase;

/* Function to create a new unified database. */
TA_RetCode TA_UDBaseAlloc( TA_UDBase **newUDBase );

/* Function to free all ressource of an unified database. */
TA_RetCode TA_UDBaseFree( TA_UDBase *toBefreed );

#define TA_SOURCELOCATION_MAX_LENGTH  2048  /* Maximum number of character. */
#define TA_SOURCEINFO_MAX_LENGTH      1024  /* Maximum number of character. */

#define TA_CATEGORY_MAX_LENGTH        64    /* Maximum number of character. */
#define TA_SYMBOL_MAX_LENGTH          64    /* Maximum number of character. */

/* A category respecting the TA-LIB guideline shall
 * respect the following limit for its 3 components.
 */
#define TA_CAT_COUNTRY_MAX_LENGTH     2  /* Max length for the country id.    */
#define TA_CAT_EXCHANGE_MAX_LENGTH    25 /* Max length for the exchange id.   */
#define TA_CAT_TYPE_MAX_LENGTH        10 /* Max length for the security type. */

/* Default category name. */
#define TA_DEFAULT_CATEGORY          "ZZ.OTHER.OTHER"
#define TA_DEFAULT_CATEGORY_COUNTRY  "ZZ"
#define TA_DEFAULT_CATEGORY_EXCHANGE "OTHER"
#define TA_DEFAULT_CATEGORY_TYPE     "OTHER"

/* The user build the "unified database" by specifying the
 * list of data source by calling TA_AddDataSource().
 * All data source, even when of different data format, are
 * transparently merged.
 *
 * See the API documentation for more info.
 */
typedef enum
{
   /* For a complete explanation on how to use these
    * data sources, see the HTML documentation.
    */
   TA_ASCII_FILE,
   TA_SIMULATOR,
   TA_YAHOO_WEB,
   TA_NUM_OF_SOURCE_ID
} TA_SourceId;

/* A group of flags can be specified to describe characteristics
 * of the data source.
 *
 * Some of these flags allow to enable/disable some functionality
 * of a data source.
 *
 * TA_AddDataSource will fail with TA_NOT_SUPPORTED if a flag request a
 * functionality not provided by the data source.
 *
 * TA_ENABLE_UPDATE_INDEX
 *    Enable the functionality that allows to add, or modify the category and
 *    symbol index.
 *
 * TA_ENABLE_UPDATE_SYMBOL
 *    Enable the functionality that allows to modify or add price data to a
 *    particular symbol.
 *
 * TA_ENABLE_CALLBACK
 *    Enable the functionality that can provides callback when a new price bar
 *    is available from this data source.
 *
 * TA_SPLIT_INCONSISTENT
 *    A data source is assumed to be completely split adjusted relatively to
 *    its last stored historical value. This will be the case with most of the
 *    data source. In the low probable case where the data source is not
 *    split adjusted, some additional processing by the TA-LIB must be done to
 *    assure consistency (through additional processing from an "inteligent
 *    split adjustment" mechanism).
 *    Note: This flag shall be rarely used.
 */

/* Watch-out... these flags are not yet fully supported. */
typedef int TA_SourceFlag;
#define TA_NO_FLAGS  0
#define TA_ENABLE_UPDATE_INDEX  (1<<0)
#define TA_ENABLE_UPDATE_SYMBOL (1<<1)
#define TA_ENABLE_CALLBACK      (1<<2)
#define TA_SPLIT_INCONSISTENT   (1<<3)

typedef struct
{
   /* The whole structure should be first initialize
    * with zero, and only the relevant parameter
    * to your application needs to be set.
    *
    * The safest way is to ALWAYS do something like:
    *    TA_AddDataSourceParam param;
    *    memset( &param, 0, sizeof( TA_AddDataSourceParam ) );
    *    ... set only the parameter you need...
    *    retCode = TA_AddDataSource( &param ); 
    *
    * Initializing the whole structure to zero assure
    * that the actual (or future) unused parameters
    * won't interfere.
    */

   /* Identify the data source. */
   TA_SourceId   id;

   /* Indicates the type of functionality that
    * must be enabled from this data source.
    */
   TA_SourceFlag flags;

   /* The usage of the following two parameter are
    * data source dependent. See documentation.
    */
   const char *location;
   const char *info;

   /* For data source requiring authentication. */
   const char *username;
   const char *password;

   /* The category can be specified in two way.
    *
    * By setting "category" or by setting
    * individual category component following
    * the TA-LIB guideline.
    * Example:
    *    param.category = "My Category";
    *
    *     or
    *
    *    param.country  = "US";
    *    param.exchange = "NASDAQ";
    *    param.type     = "STOCK";
    *
    * Some data source will ignore the category 
    * provided here.
    */
   const char *category;
   const char *country;
   const char *exchange;
   const char *type;

   /* The following should be set when requesting to add
    * one and only one specific symbol.
    * May fail if the data source does not contain the symbol.
    */
   const char *symbol;

} TA_AddDataSourceParam;

TA_RetCode TA_AddDataSource( TA_UDBase *unifiedDatabase,
                             const TA_AddDataSourceParam *param );


/* The following are some pre-defined ASCII file format.
 * These can be used for the 'sourceInfo' for TA_ASCII_FILE format.
 *
 * Note: If these file format are not convenient, you can
 *       specify your own. See documentation.
 */
#define TA_DOHLCV "[Y][M][D][O][H][L][C][V]"
#define TA_DOCHLV "[Y][M][D][O][C][H][L][V]"
#define TA_DOCV   "[Y][M][D][O][C][V]"
#define TA_DCV    "[Y][M][D][C][V]"

/* The following functions are used to obtain the list of
 * category found in an unified database.
 *
 * On success, it becomes the responsibility of the caller to
 * call TA_CategoryTableFree once the 'table' is no longuer needed.
 *
 * Simple example printing out all the category:
 *
 *   TA_StringTable *table;
 *   TA_RetCode retCode;
 *   int i;
 *
 *   retCode = TA_CategoryTableAlloc( udBase, &table );
 *
 *   if( retCode == TA_SUCCESS )
 *   {
 *      for( i=0; i < table->size; i++ )
 *         printf( "%s\n", table->string[i] );
 *
 *      TA_CategoryTableFree( table );
 *   }
 */
TA_RetCode TA_CategoryTableAlloc( TA_UDBase *unifiedDatabase,
                                  TA_StringTable **table );

TA_RetCode TA_CategoryTableFree ( TA_StringTable *table );

/* The following functions are used to obtain the list of
 * symbols provided by the unified database.
 *
 * Note: To obtain the list of symbol not classify under
 *       any particular category, just pass NULL for the
 *       category string.
 *
 * On success, it becomes the responsibility of the caller to
 * call TA_SymbolTableFree once the 'table' is no longuer needed.
 *
 * Example:
 * This code snippet will print out all the supported
 * symbol for the "US.NASDAQ.STOCK" category:
 *
 *   TA_StringTable *table;
 *   TA_RetCode retCode;
 *   int i;
 *
 *   retCode = TA_SymbolTableAlloc( udBase, "US.NASDAQ.STOCK", &table );
 *
 *   if( retCode == TA_SUCCESS )
 *   {
 *      for( i=0; i < table->size; i++ )
 *         printf( "%s\n", table->string[i] );
 *
 *      TA_SymbolTableFree( table );
 *   }
 */
TA_RetCode TA_SymbolTableAlloc( TA_UDBase *unifiedDatabase,
                                const char *category,
                                TA_StringTable **table );

TA_RetCode TA_SymbolTableFree ( TA_StringTable *table );


/* An alternate way to get the index of all symbols is
 * through the use of the TA_ForEachSymbol(). You can
 * setup a function to be called for each symbols currently
 * in the unified database.
 *
 * Example:
 *  This code will print the name and category of all symbols
 *  actually in the unified database.
 *
 *  void printSymbolInfo( TA_UDBase *udBase,
 *                        const char *category,
 *                        const char *symbol,
 *                        void *opaqueData )
 *  {
 *     printf( "Category=%s Symbol=%s\n", category, symbol );
 *  }
 *
 *  void printAll( TA_UDBase *udBase )
 *  {
 *     TA_ForEachSymbol( udBase, printSymbolInfo, NULL );
 *  }
 */
typedef void (*TA_ForEachSymbolFunc)( TA_UDBase *unifiedDatabase,
                                      const char *category,
                                      const char *symbol,
                                      void *opaqueData );

TA_RetCode TA_ForEachSymbol( TA_UDBase *unifiedDatabase,
                             TA_ForEachSymbolFunc functionToCall,
                             void *opaqueData );

/* If a modification have been done to the index of a data source,
 * you may want to refresh the information in the unified database.
 *
 * There is no need to call this funciton if only the financial data
 * did change. Only index changes are meaningful.
 * 
 * Note: This can be time-consuming, particularly if the number or
 *       size of the data source is large.
 */

/* !!!Not yet implemented.
TA_RetCode TA_RefreshAllDataSource( TA_UDBase *unifiedDatabase );
*/

/* The following functions allows to access historic data.
 * On success, it becomes the responsibility of the caller to
 * call TA_HistoryFree once the 'history' is no longuer needed.
 *
 * Example:
 *   This code will print out all the available daily close values of
 *   the company "LNUX". It is assume that this symbol is part of
 *   the unified database under the "US.NASDAQ.STOCK" category.
 *
 *   TA_History *history;
 *   TA_RetCode retCode;
 *   int i;
 *
 *   retCode = TA_HistoryAlloc( udBase, "US.NASDAQ.STOCK", 
 *                              "LNUX", TA_DAILY, NULL, NULL,
 *                              TA_ALL, &history );
 *
 *   if( retCode == TA_SUCCESS )
 *   {
 *      for( i=0; i < history->nbBars; i++ )
 *         printf( "Close = %f\n", history->close[i] );
 *
 *      TA_HistoryFree( history );
 *   }
 */
typedef int TA_Field;
#define TA_ALL           0 /* Includes all fields available from database. */
#define TA_OPEN         (1<<0)
#define TA_HIGH         (1<<1)
#define TA_LOW          (1<<2)
#define TA_CLOSE        (1<<3)
#define TA_VOLUME       (1<<4)
#define TA_OPENINTEREST (1<<5)
#define TA_TIMESTAMP    (1<<6)

TA_RetCode TA_HistoryAlloc( TA_UDBase           *unifiedDatabase,
                            const char          *category,
                            const char          *symbol,
                            TA_Period            period,
                            const TA_Timestamp  *start,
                            const TA_Timestamp  *end,
                            TA_Field             fieldToAlloc,
                            TA_History         **history );

TA_RetCode TA_HistoryFree( TA_History *history );

/* TA_HistoryEqual
 *  Return 1 if two TA_History are equivalent (same content).
 *  Return 0 if a difference is found.
 */
int TA_HistoryEqual( const TA_History *history1, const TA_History *history2 );

/* TA_Report is provided for printing out the
 * contents of the unified database.
 */

/* Flags that can be used for TA_Report. */
typedef int TA_ReportFlag;
#define TA_REPORT_SYMBOL    (1<<0)
#define TA_REPORT_CATEGORY  (1<<1)
#define TA_REPORT_SOURCE    (1<<2)
#define TA_REPORT_TOTAL     (1<<3)


TA_RetCode TA_Report( TA_UDBase *unifiedDatabase,
                      FILE *out,
                      TA_ReportFlag flags );

/* A little utility to fetch a web page and send the raw data
 * to the provided FILE pointer. This ptr could be "stdout" to
 * display on the console.
 *
 * Example:
 *        TA_WebFetch( "www.yahoo.com", stdout );
 *           or
 *        TA_WebFetch( "http://www.yahoo.com//mt", myFile );
 */
TA_RetCode TA_WebFetch( const char *url, FILE *out );

/* Some data source provides information on the current
 * market.
 *
 * When multiple data source provides market data for the same
 * symbol, TA-LIB will select and return the most recent
 * information. This mechanism allow to have multiple simultaneous
 * data source with a "fall back" if one of the source fails.
 */

#if 0

!!! We are currently putting more effort on
!!! historical and technical analysis functionality.
!!! Retreiving market data is interesting, but
!!! not yet implemented.

typedef struct
{
   /* Reflects the market at a certain moment. */
   TA_Timestamp timestamp;

   TA_Real    lastTrade;
   TA_Real    bid;
   TA_Real    ask;
   TA_Integer volume;

   /* Hidden data for internal use by the TA-LIB. Do not modify. */
   void *hiddenData;
} TA_MarketData;

TA_RetCode TA_MarketDataAlloc( TA_UDBase *unifiedDatabase, TA_MarketData **marketData );
TA_RetCode TA_MarketDataFree( TA_MarketData *data );

#endif

/* Functions for "realtime" feedback.
 *
 * First of all, with TA_AddDataSource, some online data source can
 * be added. Consequently, repetitive call to TA_HistoryAlloc and
 * TA_HistoryFree allows to obtain the "latest" available data if that
 * data source is "real time". This method is functional and is probably
 * the most simple approach.
 *
 * Demanding user may want to consider one of the two more dynamic
 * method:
 *
 *   (Note: The following is in development)
 *
 *  1) Periodical Refresh
 *     TA_HistoryUpdate allows to get the latest data from the data source
 *     added with the flag TA_ENABLE_REALTIME_FEED.
 *
 *     Simplified example:
 *        TA_History data;
 *        TA_RetCode retCode;
 *        TA_Timestamp today;
 *
 *        ...
 *
 *        TA_AddDataSource( TA_ASCII_FILE, ... );
 *        TA_AddDataSource( TA_RT_QUOTE, with TA_ENABLE_REALTIME_FEED );
 *
 *        TA_TimestampNow( &today );
 *
 *        TA_HistoryAlloc( "NASDAQ", "LNUX", TA_15MIN, today, NULL, &data );
 *
 *        while( 1 )
 *        {
 *            ... sleep for a while ...
 *            retCode = TA_HistoryUpdate( &data, 30 );
 *            if( retCode == TA_SUCCESS )
 *               DisplayData( data );
 *         }
 *
 *      In this example, all available intraday 15 Min bars are obtained
 *      with TA_HistoryAlloc (TA_RT_QUOTE and the ASCII files are merged).
 *
 *      Following that, the TA_History can be refreshed. Only the data
 *      coming from data source with TA_ENABLE_REALTIME_FEED are going to
 *      be used for the update.
 *
 *      If there is new data available, TA_SUCCESS is returned and the
 *      new and old data is returned in the provided TA_History pointer (the
 *      TA-LIB will take care of all the possibly needed re-allocation).
 *      TA_NO_NEW_DATA indicates that no new data was available at this time.
 *      For all return code different than TA_SUCCESS, the TA_History pointer
 *      is still pointing on the same old data.
 *
 *      The timeout parameter indicate how longs (in seconds) the caller is
 *      willing to wait for the update. If a timeout occured, TA_TIMEOUT is
 *      returned. If zero is specified, the function return immediatly with
 *      TA_NO_NEW_DATA if no new data is immediatly available.
 *
 *  2) Callback method
 *     With this method, the user can setup a callback function whenever a
 *     change occured for a particular symbol. The design of this functionality
 *     is under development and is not yet available.
 */


/**** Functions TA_HistoryUpdate ****/
/* <<UNDER INITIAL DEVELOPMENT, COME BACK LATER! >> */
/*TA_RetCode TA_HistoryUpdate( TA_UDBase *unifiedDatabase,
                             TA_History **history,
                             unsigned int timeoutInSec ); */

/**** Functions for adding "real time" callback for price bars update. *****/
/* <<UNDER INITIAL DEVELOPMENT, COME BACK LATER! >> */

/**** Functions for modifying the index and data of a data source.  *****/
/* <<UNDER INITIAL DEVELOPMENT, COME BACK LATER! >> */

#ifdef __cplusplus
}
#endif

#endif

