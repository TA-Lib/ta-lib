#include <stddef.h>

#include "ta_data.h"
#include "ta_codec.h"
#include "ta_memory.h"
#include "ta_stream.h"

/* Keep in mind, the functionality embedded in this file is complexe and
 * not speed optimized for the encoding. The decoding should stay fast though.
 */

/* The following should be changed only if a major re-design of the
 * encoding is needed.
 * If this version number change, the code will have to automatically provide
 * a mechanism for updating from one version to the other.
 * Hopefully, this should not happen.
 *
 * (Note: This version number is actually encoded on 1 bit! Meaning the next
 *        version will need to set that first bit to one and upgrade the
 *        version bit size)
 */
#define STREAM_ENCODE_VER          0

/* The following will be set to '1' whenever an error occured in this file.
 * Take note that this is a private variable used only to 'exit' properly
 * when an error did occured.
 */
static unsigned int errorOccured = 0;

/* List of different encoding strategy. A new strategy MUST always be added
 * to the end of this list. All other strategy MUST stay at the same
 * position for keeping backward compatibility.
 *
 * Different encoding are seperatly considered for the following
 * grouping of data:
 *      1) All the integer parts of the price bar.
 *      2) All the fractional parts of the price bar.
 *      3) All the volume of the price bar.
 *      4) All the open interest of the price bar.
 *      5) All the date related information.
 *
 * Note: The current version support a maximum of 8 different encoding
 *       for each group. This should be more than enough.
 */
#define NB_MAX_CODING_PER_GROUP 8

typedef struct
{
   void (*encode)( const TA_History *inHistory, TA_Stream  *stream );
   void (*decode)( TA_StreamAccess *inStream,  TA_History *outHistory );
} TA_CodingFunc;

/* List of coding functions. */
static void TA_encodeIntRaw( const TA_History *inHistory, TA_Stream *stream );
static void TA_decodeIntRaw( TA_StreamAccess *inStream, TA_History *outHistory );

/* Each of the coding function is put into a table. */
static TA_CodingFunc codingIntTable[NB_MAX_CODING_PER_GROUP] =
{ {TA_encodeIntRaw, TA_decodeIntRaw},
  {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
  {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL} };

static TA_CodingFunc codingFraTable[NB_MAX_CODING_PER_GROUP] =
{ {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
  {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL} };

static TA_CodingFunc codingVolTable[NB_MAX_CODING_PER_GROUP] =
{ {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
  {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL} };

static TA_CodingFunc codingOpiTable[NB_MAX_CODING_PER_GROUP] =
{ {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
  {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL} };

static TA_CodingFunc codingDatTable[NB_MAX_CODING_PER_GROUP] =
{ {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
  {NULL, NULL}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL} };

static TA_Stream *encodeUsingTable( const TA_History *inHistory,
                                    const TA_CodingFunc *codingTable );

const TA_Stream *TA_encode_history( const TA_History *inHistory )
{
    unsigned int i;
    TA_Stream *finalStream;
    TA_Stream *intStream, *fraStream, *volStream, *opiStream, *datStream;

    /* Allocate the stream that is going to be returned. */
    finalStream = TA_StreamAlloc();

    if( !finalStream )
       return NULL;

    /* Encode the different group of data individually. */
    intStream = encodeUsingTable( inHistory, codingIntTable );
    fraStream = encodeUsingTable( inHistory, codingFraTable );;
    volStream = encodeUsingTable( inHistory, codingVolTable );;
    opiStream = encodeUsingTable( inHistory, codingOpiTable );;
    datStream = encodeUsingTable( inHistory, codingDatTable );;

    /* If we get up to here without error, start to merge all the
     * encoding into the finalStream.
     */
    if( errorOccured )
    {
       TA_StreamFree( finalStream );
       return NULL;
    }
    else
    {
        /* Write the version number (only one bit for the time being) */
        TA_StreamAddBit( finalStream, STREAM_ENCODE_VER );

        /* Indicate the amount of price bar. */
        TA_StreamAddInt32( finalStream, inHistory->priceBars.nbTick );

        /* Indicate which group of data are part of the file. */
        TA_StreamAddBit( finalStream, intStream? 1:0 );
        TA_StreamAddBit( finalStream, fraStream? 1:0 );
        TA_StreamAddBit( finalStream, volStream? 1:0 );
        TA_StreamAddBit( finalStream, opiStream? 1:0 );
        TA_StreamAddBit( finalStream, datStream? 1:0 );

        /* Merge the streams all together. */
        TA_StreamMerge( finalStream, intStream );
        TA_StreamMerge( finalStream, fraStream );
        TA_StreamMerge( finalStream, volStream );
        TA_StreamMerge( finalStream, opiStream );
        TA_StreamMerge( finalStream, datStream );
    }

    if( !errorOccured )
        return finalStream;
    else
    {
        TA_StreamFree( finalStream );
        return NULL;
    }
}

const TA_History *TA_decode_history( const TA_Stream *inStream )
{
    TA_History *history;
    TA_StreamAccess *acc;
    unsigned int tmpInt;
    unsigned int errorOccured;

    acc = TA_StreamAccessAlloc( inStream );
    if( !acc )
        return NULL;

    /* Verify the version. */
    if( !TA_StreamAccessGetBit( acc, 1, &tmpInt ) )
    {
        TA_StreamAccess_Free( acc );
        return NULL;
    }

    /* Allocate the history structure. */
    history = TA_HistoryAlloc();

    if( !history )
    {
        TA_StreamAccessFree( acc );
        return NULL;
    }

    /* Get the number of price bar. */
    if( !TA_StreamAccessGetInt32( acc, &history->priceBars.nbTick ) )
    {
        TA_HistoryFree( history );
        TA_StreamAccessFree( acc );
        return NULL;
    }

    /* Identify which data is provided from the stream and allocate the
     * corresponding memory needed for the history.
     */
    if( !TA_StreamAccessGetBit( acc, 5, &tmpInt1 ) )
    {
        TA_HistoryFree( history );
        TA_StreamAccessFree( acc );
        return NULL;
    }

    errorOccured = 0;
    if( (tmpInt1 && 0x10) || (tmpInt1 && 0x08) )
    {
        history->priceBars.open  = TA_Malloc( sizeof( TA_Real ) * history->priceBars.nbTick );
        history->priceBars.close = TA_Malloc( sizeof( TA_Real ) * history->priceBars.nbTick );
        history->priceBars.high  = TA_Malloc( sizeof( TA_Real ) * history->priceBars.nbTick );
        history->priceBars.low   = TA_Malloc( sizeof( TA_Real ) * history->priceBars.nbTick );

        if( (!history->priceBars.open) && (!history->priceBars.close) &&
            (!history->priceBars.high) && (!history->priceBars.low) )
           errorOccured = 1;
    }

    if( (!errorOccured) && (tmpInt1 & 0x04) )
    {
        history->priceBars.volume = TA_Malloc( sizeof( TA_Integer ) * history->priceBars.nbTick );

        if( !history->priceBars.volume )
            errorOccured = 1;
    }

    if( (!errorOccured) && (tmpInt1 & 0x02) )
    {
        history->priceBars.openInterest = TA_Malloc( sizeof( TA_Integer ) * history->priceBars.nbTick );

        if( !history->priceBars.openInterest )
            errorOccured = 1;
    }

    /* Allocate the private info.
    history->privateInfo = TA_Malloc( sizeof( TA_HISTORY_PRIV ) ); */

    if( errorOccured )
    {
        /* Something was not allocated correctly, free everything and
         * return.
         */
        TA_HistoryFree( history );
        TA_StreamAccessFree( acc );
        return NULL;
    }

    /* Decode each group of data individually. */
    if( ((history->priceBars.open)   && (!decodeUsingTable( acc, history, codingIntTable )) ||
        ((history->priceBars.open)   && (!decodeUsingTable( acc, history, codingFraTable )) ||
        ((history->priceBars.volume) && (!decodeUsingTable( acc, history, codingVolTable )) ||
        ((history->priceBars.openInterest) && (!decodeUsingTable( acc, history, codingOpiTable )))
    {
        /* Something wrong happened. Free everything and return. */
        TA_HistoryFree( history );
        TA_StreamAccessFree( acc );
        return NULL;
    }

    return history;
}

static TA_Stream *encodeUsingTable( const TA_History *inHistory,
                                    const TA_CodingFunc *codingTable )
{
    unsigned int i;
    unsigned int bestEncoding;
    TA_Stream *stream = NULL;
    TA_Stream *bestStream = NULL;
    TA_Stream *headerStream;

    for( i=0; (i < NB_MAX_CODING_PER_GROUP) && !errorOccured; i++ )
    {
        if( codingTable[i].encode != NULL )
        {
            stream = TA_StreamAlloc();

            /* Do the encoding. */
            codingTable[i].encode( inHistory, stream );

            /* Keep the best one and get ride of the other. */
            if( bestStream == NULL )
            {
                bestStream = stream;
                bestEncoding = i;
            }
            else
            {
                if( TA_StreamSize( stream ) < TA_StreamSize( bestStream ) )
                {
                    TA_StreamFree( bestStream );
                    bestStream = stream;
                    bestEncoding = i;
                }
                else
                    TA_StreamFree( stream );
            }
        }
    }

    if( bestStream == NULL )
        return NULL;

    if( errorOccured )
    {
        /* Free everything and return. */
        TA_StreamFree( bestStream );
        return NULL;
    }
    else
    {
        /* Append the best encoding index in front of the stream. */
        headerStream = TA_StreamAlloc();
        if( !headerStream )
        {
            TA_StreamFree( bestStream );
            return NULL;
        }

        TA_StreamAddBit( headerStream, bestEncoding & 0x04 );
        TA_StreamAddBit( headerStream, bestEncoding & 0x02 );
        TA_StreamAddBit( headerStream, bestEncoding & 0x01 );
        TA_StreamMerge ( headerStream, bestStream );

        return headerStream;
    }
}

/***************************************************************************
 * The following functions in this file are the different encoding/decoding
 * strategy for: the integer part of the price bar.
 ***************************************************************************/
static void TA_encodeIntRaw( const TA_History *inHistory, TA_Stream *outStream )
{
    unsigned int i;

    /* This encoding is simply saving the price history
     * by using 32 bits per integer. There is absolutly no
     * compression performed.
     */
    for( i=0; i < inHistory->size; i++ )
        TA_StreamAddInt32( outStream, inHistory->open[i] );

    for( i=0; i < inHistory->size; i++ )
        TA_StreamAddInt32( outStream, inHistory->high[i] );

    for( i=0; i < inHistory->size; i++ )
        TA_StreamAddInt32( outStream, inHistory->low[i] );

    for( i=0; i < inHistory->size; i++ )
        TA_StreamAddInt32( outStream, inHistory->close[i] );
}

static void TA_decodeIntRaw( const TA_Stream *inStream, TA_History *outHistory )
{
    unsigned int i;
    unsigned int size;
    TA_StreamAccess *acc;
    int data;

    /* See TA_encodeIntRaw for more info. */
    acc = TA_StreamAccessAlloc( inStream );

    for( i=0; i < size; i++ )
    {
        TA_StreamAccessGetInt32( acc, &data );
        outHistory->open[i] = data;
    }

    for( i=0; i < size; i++ )
    {
        TA_StreamAccessGetInt32( acc, &data );
        outHistory->high[i] = data;
    }

    for( i=0; i < size; i++ )
    {
        TA_StreamAccessGetInt32( acc, &data );
        outHistory->low[i] = data;
    }

    for( i=0; i < size; i++ )
    {
        TA_StreamAccessGetInt32( acc, &data );
        outHistory->close[i] = data;
    }

    TA_StreamAccessFree( acc );
}

/***************************************************************************
 * The following functions in this file are the different encoding/decoding
 * strategy for: the fractional part of the price bar.
 ***************************************************************************/

/***************************************************************************
 * The following functions in this file are the different encoding/decoding
 * strategy for: the volume.
 ***************************************************************************/

/***************************************************************************
 * The following functions in this file are the different encoding/decoding
 * strategy for: the openInterest.
 ***************************************************************************/

/***************************************************************************
 * The following functions in this file are the different encoding/decoding
 * strategy for: the date.
 ***************************************************************************/

 #if 0
 #if 0
void TA_encodeSimple( TA_History *history, TA_Stream *stream )
{
   unsigned int i, size;

   size = history->size;
   for( i=0; i < size; i++ )
   {
      TA_StreamAddDouble( stream, history->open[i]   );
      TA_StreamAddDouble( stream, history->high[i]   );
      TA_StreamAddDouble( stream, history->low[i]    );
      TA_StreamAddDouble( stream, history->close[i]  );
      TA_StreamAddDouble( stream, history->volume[i] );
   }
}
#endif

static unsigned int decToInt( double data )
{
   unsigned char tmp[32];
   unsigned int i;

   /* Instead of calculation, use a sprintf.
    * Although not really speed performant, at least
    * it should works in most environment.
    */
   sprintf( tmp, "%#031.15f", data );

   /* Verify some assumptions. */
   if( tmp[31] != 0 )
   {
      printf( "Unexpected non-null terminated string\n" );
      return 0;
   }

   if( tmp[15] != '.' )
   {
      printf( "Unexpected sprintf behavior\n" );
      return 0;
   }

   /* Remove trailing zero. */
   i = 30;
   while( tmp[i] == '0' )
      tmp[i--] = '\0';

   tmp[15] = '1';
   return (unsigned int)atoi( &tmp[15] );
}

static void writeRangeInfo( TA_Stream *stream, int minValue, int maxValue )
{
   unsigned int range, tmp, size, mask, zeroOffset;
   unsigned int zeroIncludedCoding;

   /* Calculate the range. */
   range = abs(maxValue - minValue) ;

   /* Find the bitsize required for the range. */
   tmp = range;
   size = 0;
   while( tmp != 0 )
   {
      size++;
      tmp >>= 1;
   }

   /* Output the bitsize on 5 bits. */
   mask = 1 << 4;
   while( mask )
   {
      TA_StreamAddBit( stream, size & mask );
      mask >>= 1;
   }

   /* If the bitsize is zero, no further processing needed.
    * (that will happen when the range is zero...)
    */
   if( size == 0 ) return;

   /* Coding is different if zero is not included in the range. */
   if( (minValue <= 0) && (maxValue >= 0) )
      zeroIncludedCoding = 1;
   else
      zeroIncludedCoding = 0;
   TA_StreamAddBit( stream, zeroIncludedCoding );

   if( zeroIncludedCoding )
   {
      /* Output the zero offset by using 'size' bits. */
      zeroOffset = 0 - minValue;
      mask = 1 << (size-1);
      while( mask )
      {
         TA_StreamAddBit( stream, zeroOffset & mask );
         mask >>= 1;
      }
   }
   else
   {
      /* Zero not included in range or no range, consequently,
       * save the signed min value for making possible to offset
       * correctly the delta.
       */
      TA_StreamAddInt( stream, minValue );
   }
}


void TA_encodeDelta( TA_History *inHistory, TA_Stream *stream )
{
    unsigned int i;

    unsigned int tmp_uint;
    int tmp_int;

    int priceIntMin, priceIntMax;
    int priceDecMin, priceDecMax;
    int volMin, volMax, volPrecMin;

    unsigned int prevPriceOpenInt;
    unsigned int prevPriceHighInt;
    unsigned int prevPriceLowInt;
    unsigned int prevPriceCloseInt;

    unsigned int prevPriceOpenDec;
    unsigned int prevPriceHighDec;
    unsigned int prevPriceLowDec;
    unsigned int prevPriceCloseDec;

    unsigned int prevVol;

    priceIntMin = INT_MAX;
    priceDecMin = INT_MAX;
    volMin      = INT_MAX;

    priceIntMax = INT_MIN;
    priceDecMax = INT_MIN;
    volMax      = INT_MIN;

    volPrecMin  = 3;

    prevPriceOpenInt  = (unsigned int)floor(inHistory->open[0]);
    prevPriceHighInt  = (unsigned int)floor(inHistory->high[0]);
    prevPriceLowInt   = (unsigned int)floor(inHistory->low[0]);
    prevPriceCloseInt = (unsigned int)floor(inHistory->close[0]);

    prevPriceOpenDec  = decToInt( inHistory->open[0]  );
    prevPriceHighDec  = decToInt( inHistory->high[0]  );
    prevPriceLowDec   = decToInt( inHistory->low[0]   );
    prevPriceCloseDec = decToInt( inHistory->close[0] );

    prevVol = (unsigned int)inHistory->volume[0];

    /* Calculate min/max delta values. */
    for( i=1; i < inHistory->size; i++ )
    {
        /* Calculate delta values for the integer part of the price. */
        if( inHistory->open )
        {
            tmp_uint = (unsigned int)floor(inHistory->open[i]);
            tmp_int  = tmp_uint - prevPriceCloseInt; /* Open is relative to previous close. */
            if( tmp_int > priceIntMax ) priceIntMax = tmp_int;
            if( tmp_int < priceIntMin ) priceIntMin = tmp_int;
            prevPriceOpenInt = tmp_uint;
        }

        if( inHistory->high )
        {
            tmp_uint = (unsigned int)floor(inHistory->high[i]);
            tmp_int  = tmp_uint - prevPriceHighInt;
            if( tmp_int > priceIntMax ) priceIntMax = tmp_int;
            if( tmp_int < priceIntMin ) priceIntMin = tmp_int;
            prevPriceHighInt = tmp_uint;
        }

        if( inHistory->low )
        {
            tmp_uint = (unsigned int)floor(inHistory->low[i]);
            tmp_int  = tmp_uint - prevPriceLowInt;
            if( tmp_int > priceIntMax ) priceIntMax = tmp_int;
            if( tmp_int < priceIntMin ) priceIntMin = tmp_int;
            prevPriceLowInt = tmp_uint;
        }

        if( inHistory->close )
        {
            tmp_uint = (unsigned int)floor(inHistory->close[i]);
            tmp_int  = tmp_uint - prevPriceOpenInt; /* Close is relative to "current" open. */
            if( tmp_int > priceIntMax ) priceIntMax = tmp_int;
            if( tmp_int < priceIntMin ) priceIntMin = tmp_int;
            prevPriceCloseInt = tmp_uint;
        }

        /* Keep track of min, max delta value among the decimal
         * part of the price.
         */
        if( inHistory->open )
        {
            tmp_uint = decToInt( inHistory->open[i] );
            tmp_int  = tmp_uint - prevPriceCloseDec; /* Open is relative to previous close. */
            if( tmp_int > priceDecMax ) priceDecMax = tmp_int;
            if( tmp_int < priceDecMin ) priceDecMin = tmp_int;
            prevPriceOpenDec = tmp_uint;
        }

        if( inHistory->high )
        {
            tmp_uint = decToInt( inHistory->high[i] );
            tmp_int  = tmp_uint - prevPriceHighDec;
            if( tmp_int > priceDecMax ) priceDecMax = tmp_int;
            if( tmp_int < priceDecMin ) priceDecMin = tmp_int;
            prevPriceHighDec = tmp_uint;
        }

        if( inHistory->low )
        {
            tmp_uint = decToInt( inHistory->low[i] );
            tmp_int  = tmp_uint - prevPriceLowDec;
            if( tmp_int > priceDecMax ) priceDecMax = tmp_int;
            if( tmp_int < priceDecMin ) priceDecMin = tmp_int;
            prevPriceLowDec = tmp_uint;
        }

        if( inHistory->close )
        {
            tmp_uint = decToInt( inHistory->close[i] );
            tmp_int  = tmp_uint - prevPriceOpenDec; /* Close is relative to "current" open. */
            if( tmp_int > priceDecMax ) priceDecMax = tmp_int;
            if( tmp_int < priceDecMin ) priceDecMin = tmp_int;
            prevPriceCloseDec = tmp_uint;
        }

        /* Keep track of min, max delta value among the volume. */
        if( inHistory->volume )
        {
            tmp_uint = (unsigned int)inHistory->volume[i];
            tmp_int  = tmp_uint - prevVol;
            if( tmp_int > volMax ) volMax = tmp_int;
            if( tmp_int < volMin ) volMin = tmp_int;
            prevVol = tmp_uint;

            /* Keep track min precision needed for the volume. */
            switch( volPrecMin )
            {
            case 2: /* 100's */
                if( (abs(tmp_int) % 100) != 0 )
                volPrecMin = 1;
                break;
            case 3: /* 1000's */
                if( (abs(tmp_int) % 1000) != 0 )
                volPrecMin = 2;
                break;
            case 1: /* 10's */
                if( (abs(tmp_int) % 10) != 0 )
                    volPrecMin = 0;
                break;
            case 0: /* 1's */
                break;
            }
        }
    }

    /* Adjust volume min/max considering the precision. */
    if( volPrecMin != 0 )
    {
        volMax /= pow( 10, volPrecMin );
        volMin /= pow( 10, volPrecMin );
    }

    /* Calculate/write the bit size and the median value for each element type. */
    writeRangeInfo( stream, priceIntMin, priceIntMax );
    writeRangeInfo( stream, priceDecMin, priceDecMax );
    writeRangeInfo( stream, volMin, volMax );

   /* Write the volume precision (on 2 bits). */
   TA_StreamAddBit( stream, volPrecMin & 0x1 );
   TA_StreamAddBit( stream, volPrecMin & 0x2 );

   /* Write all the deltas. */
   for( i=1; i < inHistory->size; i++ )
   {
   }

   /* Encode the history. */
}

void TA_decodeDelta( TA_History *inHistory, TA_History *outHistory )
{
}

void TA_storeInDB( TA_History *history )
{
   TA_Stream *simpleEncode;
   TA_Stream *deltaEncode;

   printf( "\nProcessing TLAB: " );

   /* Try many encoding strategy and keep the stream of the
    * most efficient one.
    */
   simpleEncode = TA_StreamAlloc();
   TA_encodeSimple( history, simpleEncode );
   printf( "\nSimple encoding: %d", simpleEncode->size );
   TA_StreamFree( simpleEncode );

   deltaEncode = TA_StreamAlloc();
   TA_encodeDelta( history, deltaEncode );
   printf( "\nDelta encoding: %d", deltaEncode->size );
   TA_StreamFree( deltaEncode );
}
#endif
