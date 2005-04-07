#ifndef TA_YAHOO_IDX_H
#define TA_YAHOO_IDX_H

#ifndef TA_COMMON_H
   #include "ta_common.h"
#endif

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

#ifndef TA_STREAM_H
   #include "ta_stream.h"
#endif

#ifndef TA_COUNTRY_INFO
   #include "ta_country_info.h"
#endif

#ifndef TA_YAHOO_PRIV_H
   #include "ta_yahoo_priv.h"
#endif

typedef struct
{
   TA_String *name; /* Name of this category. */

   unsigned int nbSymbol; /* Nb of symbols in this cateogry. */
   TA_String **symbols;   /* Name of symbols in this category. */

   /* Do not modify the following! */
   void *hiddenData;
} TA_YahooCategory;

typedef struct
{
   TA_CountryId countryId;        /* The country of this index. */

   unsigned char countryAbbrev[3]; /* A 2 letters abbreviation for this
                                    * country. NULL terminated.
                                    */

   unsigned int nbCategory;
   TA_YahooCategory **categories;

   TA_Timestamp creationDate;

   /* Do not modify the following! */
   void *hiddenData;
} TA_YahooIdx;

/* Build an index using the online Yahoo! web site.
 *
 * The strategy indicates how the index could be build.
 *
 * TA_USE_STREAM: Use the provided stream for building
 * the index.
 *
 * TA_USE_LOCAL_CACHE: Use the local cache to retreive
 * the index. An optional timeout can be provided.
 *
 * TA_USE_REMOTE_CACHE: Give the possibility to avoid to 
 * bother Yahoo! by using a pre-assembled index stored
 * at ta-lib.org. An optional timeout can be provided.
 *
 * TA_USE_YAHOO_SITE: Build the index by accessing
 * directly the Yahoo! web site as the source. This is
 * requiring a lot of bandwidth and patience (can take
 * many hours).
 *
 * TA_USE_YAHOO_AND_REMOTE_MERGE
 * Use remote Yahoo! web site and TA-Lib.org to update
 * more quickly a local index.
 *
 * Note 1: The strategy can be mixed and are going to be attempted
 *         in the order state above. Example:
 *            TA_USE_YAHOO_SITE|TA_USE_REMOTE_CACHE will first
 *            try the remote cache, if it fails it will then
 *            rebuild the index directly from Yahoo!.
 *
 * Note 2: if 'stream' is provided, the index is inconditionaly
 *         build from this stream. No other strategy are
 *         attempted.
 */
typedef enum
{
   TA_USE_STREAM                 = 0x02,
   TA_USE_LOCAL_CACHE            = 0x04,
   TA_USE_REMOTE_CACHE           = 0x08,
   TA_USE_YAHOO_SITE             = 0x10,
   TA_USE_YAHOO_AND_REMOTE_MERGE = 0x20
} TA_YahooIdxStrategy;

TA_RetCode TA_YahooIdxAlloc( TA_CountryId           countryId,
                             TA_YahooIdx          **yahooIdxAllocated,
                             TA_YahooIdxStrategy    strategy,
                             TA_Stream             *stream,
                             TA_Timestamp          *localCacheTimeout,
                             TA_Timestamp          *remoteCacheTimeout );

TA_RetCode TA_YahooIdxFree( TA_YahooIdx *idxToBefreed );

/* Compress an index and transform it into a stream of bytes. This stream
 * can be used to rebuild at a later time the index by using the TA_USE_STREAM
 * strategy.
 *
 * Error detection are included in the stream, allowing to transmit/store that
 * stream with confidence. Using that stream, you are guaranteed that the index
 * will be identical when TA_YahooIdxAlloc succeed.
 *
 * The exact same stream is used for the local and remote cache.
 */
TA_RetCode TA_YahooIdxStream( const TA_YahooIdx *idx, TA_Stream **streamAllocated );

/* Get the decoding parameters. 
 *
 * Inside a yahoo index, some decoding information are stored. This
 * information will allow to potentially adapt to a new parsing for
 * the Yahoo! web pages (without having to change the library).
 * 
 * The specific web site address and page location is also encoded.
 */
typedef enum {TA_YAHOOIDX_CSV_PAGE,
              TA_YAHOOIDX_MARKET_PAGE,
              TA_YAHOOIDX_INFO,
              TA_YAHOOIDX_ADJUSTMENT} TA_DecodeType;

TA_DecodingParam *TA_YahooIdxDecodingParam( TA_YahooIdx *idx, TA_DecodeType type );

/* Get the decoding parameters to get the market data from 
 * a specific Yahoo! web site.
 */ 
TA_RetCode TA_YahooIdxDataDecoding( TA_CountryId id,
                                    TA_DecodeType type,
                                    TA_DecodingParam *param );

#endif
