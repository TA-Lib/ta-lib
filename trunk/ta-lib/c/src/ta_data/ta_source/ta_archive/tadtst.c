#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "ta_stream.h"
#include "talibc.h"
#include "ta_error.h"
#include "ta_memory.h"

static void TA_StreamTesting( void );
static void TA_StreamTestingSize( unsigned int size );
static int  TA_StreamTestingEquality( const TA_STREAM *op1, const TA_STREAM *op2 );
static void TA_StreamTestingEquality_CheckSizeConsistency( const TA_STREAM *op1, const TA_STREAM *op2, unsigned int rv1, unsigned int rv2 );

static int isStreamRefBad( TA_STREAM_ACCESS *acc, unsigned int size );

#if 0
Mario,

Sign needs to be recorded only for the reference value in a price bar
(the open is the reference value).

1) The open is a +/- offset from the previous close (first open
determined by a 32 bit seed value).
2) The low is a negative offset from the open.
3) The High is a positive offset from the open.
4) The close is a negative offset from the high.

Consider metakit strategy for safe strategy.

\Mario




   /* This is the expected TA_DATA Library (RO) interface. */

   /* Let TA_DATA take care of allocations. */
   TA_HISTORY TA_allocHistory( id, startDate, stopDate, TA_ALL );
   TA_freeHistory ( history );

   /* Caller must provide/handle allocation. */
   TA_loadHistory( history );

   /* Get an index on all supported symbol. */
   TA_allocIndex( ... );
   TA_freeIndex( ... );

   /* Get the symbol Id. The id is used to conveniently refer
    * to this symbol in other TA_DATA call.
    */
   TA_ID TA_getSymbolIDBySymbol( const char*symbol );
   TA_ID TA_getSymbolIDByName( const char *name );

   TA_searchSymbolIdBySymbol( ... );
   TA_searchSymbolIdByNAme  ( ... );
#endif


#define TST_SIZE 2

static void readToken( FILE *in, char *str )
{
   char ch;
   unsigned int i = 0;

   ch = (char)fgetc( in );

   while( isdigit(ch) || isalpha( ch ) || ch == '.' || ch == '/' )
   {
      str[i++] = ch;
      ch = (char)fgetc( in );
   }
   str[i] = '\0';
}

void TA_loadFromASCII( TA_HISTORY *history )
{
   FILE *in;
   char token[200];
   unsigned int i;

   in = fopen( "E:/ATS/TADATA/ASCII/TLAB.TXT", "r" );

   if( in == NULL )
   {
      printf( "Cannot load from ASCCI file." );
      return;
   }

   for( i=0; i < TST_SIZE; i++ )
   {
      token[0] = '\0';
      readToken( in, token ); /* Read Symbol */
      readToken( in, token ); /* Read date. */

      readToken( in, token ); /* Open */
      history->open[i] = atof( token );

      readToken( in, token ); /* High */
      history->high[i] = atof( token );

      readToken( in, token ); /* Low */
      history->low[i] = atof( token );

      readToken( in, token ); /* Close */
      history->close[i] = atof( token );

      readToken( in, token ); /* Volume */
      history->volume[i] = atof( token );
   }

   fclose( in );
}

void TA_loadFromDB( TA_HISTORY *history )
{}

#pragma argsused
void TA_dataTesting( void )
{
TA_HISTORY history;

history.open = (TA_REAL *)malloc( sizeof(TA_REAL) * TST_SIZE );
history.high = (TA_REAL *)malloc( sizeof(TA_REAL) * TST_SIZE );
history.low  = (TA_REAL *)malloc( sizeof(TA_REAL) * TST_SIZE );
history.close = (TA_REAL *)malloc( sizeof(TA_REAL) * TST_SIZE );
history.volume = (TA_REAL *)malloc( sizeof(TA_REAL) * TST_SIZE );
history.size = TST_SIZE;

TA_loadFromASCII( &history );

printf( "(OHLCV) = (%f,%f,%f,%f,%f)\n",
          history.open[TST_SIZE-1],
          history.high[TST_SIZE-1],
          history.low[TST_SIZE-1],
          history.close[TST_SIZE-1],
          history.volume[TST_SIZE-1] );

/*TA_storeInDB( &history );*/

TA_StreamTesting();
}

static void TA_StreamTesting( void )
{
    unsigned int nbTest;
    time_t t;

    TA_StreamTestingSize( 1 );
    TA_StreamTestingSize( 2 );
    TA_StreamTestingSize( 7 );
    TA_StreamTestingSize( 8 );
    TA_StreamTestingSize( 9 );
    TA_StreamTestingSize( 31 );
    TA_StreamTestingSize( 32 );
    TA_StreamTestingSize( 33 );
    TA_StreamTestingSize( 254 );
    TA_StreamTestingSize( 255 );
    TA_StreamTestingSize( 256 );
    TA_StreamTestingSize( 65534 );
    TA_StreamTestingSize( 65535 );
    TA_StreamTestingSize( 65536 );
    TA_StreamTestingSize( 0x19C );
    TA_StreamTestingSize( 0x39C );
    srand((unsigned) time(&t));
    /* Perform the test 100 times with random data size each time. */
    for( nbTest = 0; nbTest < 100; nbTest++ )
        TA_StreamTestingSize( (rand() % 10000) + 1 );

    TA_StreamTestingSize( 1024*8*300 );  /* Test 300K  */
}

static void TA_StreamTestingSize( unsigned int size )
{
    TA_STREAM *stream1, *stream2, *streamRef, *streamRefCopy;
    TA_STREAM_ACCESS *acc, *acc2;
    unsigned int i;
    unsigned int tmpInt1, originalMemSize;

    originalMemSize = TA_MemUsed();

    /* Allocate/fill the reference stream. */
    streamRef = TA_StreamAlloc();

    for( i=0; i < size; i++ )
       TA_StreamAddBit( streamRef, (i % 3 + size)%2 );

    /* Verify that the streamRef size is correct. */
    tmpInt1 = TA_StreamSizeInBit( streamRef );
    if( size != tmpInt1 )
    {
        printf( "TA_StreamTestingSize test failed (size, TA_StreamSize) = (%d,%d)\n", size, tmpInt1 );
        return;
    }

    /* Verify that the reference is good. */
    acc = TA_StreamAccessAlloc( streamRef );
    if( isStreamRefBad( acc, size ) )
       return;
    TA_StreamAccessFree( acc );

    /* Test merge with an empty stream to merge. */
    stream1 = TA_StreamAlloc();
    TA_StreamMerge( streamRef, stream1 );

    /* Verify that the reference is still good. */
    acc = TA_StreamAccessAlloc( streamRef );
    if( isStreamRefBad( acc, size ) )
       return;
    TA_StreamAccessFree( acc );

    /* Size should be the same at this point. */
    tmpInt1 = TA_StreamSizeInBit( streamRef );
    if( size != tmpInt1 )
    {
        printf( "TA_StreamTestingSize test #1 failed after merge (size, TA_StreamSize) = (%d,%d)\n", size, tmpInt1 );
        return;
    }

    /* Make a copy of the streamRef. */
    streamRefCopy = TA_StreamAlloc();
    TA_StreamAppendCopy( streamRefCopy, streamRef );

    /* Verify that the reference copy has the correct data. */
    acc = TA_StreamAccessAlloc( streamRefCopy );
    if( isStreamRefBad( acc, size ) )
       return;
    TA_StreamAccessFree( acc );

    /* From this point, we continue to fill up stream1
     * with the following series of data:
     *      - 3 x 1 bit data                  (all ones)
     *      - 1 x streamRef data              (...)
     *      - 1 x 32 bits integer             (0xA5A5A5A5)
     *      - 1 x Compressed streamRef data   (...)
     *      - 3 x 1 byte                      (0x5A5AFE)
     *      - 1 x 1 bit data                  (all ones)
     *      - 1 x Compressed streamRef data   (...)
     *      - 1 x 8 bit data                  (all ones)
     *      - 1 x 64 bits Real data           (value -1)
     *      - 1 x 64 bits Real data           (value  0)
     *      - 1 x 64 bits Real data           (value  1)
     *
     * Once fill up, the stream is completely reviewed to verify the
     * validity (the comrpessed streamRef are going to be decompressed
     * for verifying the validity).
     */
    stream1 = TA_StreamAlloc();

    for( i=0; i < 3; i++ )
        TA_StreamAddBit( stream1, 1 );

    TA_StreamMerge( stream1, streamRefCopy );

    /* Size should be the same at this point. */
    tmpInt1 = TA_StreamSizeInBit( stream1 );
    if( size != (tmpInt1-3) )
    {
        printf( "TA_StreamTestingSize test #2 failed after merge (size, TA_StreamSize) = (%d,%d)\n", size, tmpInt1 );
        return;
    }

    TA_StreamAddInt32( stream1, 0xA5A5A5A5 );

    TA_StreamCompress( stream1, streamRef );

    TA_StreamAddByte ( stream1, 0x5A );
    TA_StreamAddByte ( stream1, 0x5A );
    TA_StreamAddByte ( stream1, 0xFE );
    TA_StreamAddBit  ( stream1, 1 );
    TA_StreamCompress( stream1, streamRef );

    for( i=0; i < 8; i++ )
        TA_StreamAddBit( stream1, 1 );

    TA_StreamAddDouble( stream1, -1.0 );
    TA_StreamAddDouble( stream1,  0.0 );
    TA_StreamAddDouble( stream1,  1.0 );

    /* Now re-verify completely the stream1 by reading it sequentially. */
    acc = TA_StreamAccessAlloc( stream1 );

   TA_StreamAccessGetBit( acc, 3, &tmpInt1 );
   if( tmpInt1 != 7 )
      TA_FATAL( "Out of sequence problem #1", tmpInt1, 0 );

    if( isStreamRefBad( acc, size ) )
      TA_FATAL( "Out of sequence problem #2", size, 0 );

    TA_StreamAccessGetInt32( acc, &tmpInt1 );
    if( tmpInt1 != 0xA5A5A5A5 )
        TA_FATAL( "Out of sequence problem #3", tmpInt1, 0 );

    stream2 = TA_StreamAlloc();
    TA_StreamDecompress( stream2, acc );

    acc2 = TA_StreamAccessAlloc( stream2 );
    if( isStreamRefBad( acc2, size ) )
       TA_FATAL( "Out of sequence problem #4", size, 0 );

    TA_StreamAccessFree( acc2 );
    TA_StreamFree( stream2 );

    /* Clean-up and exit. */
    TA_StreamFree( stream1 );

    /* Verify that the reference is still intact. */
    TA_StreamAccessFree( acc );
    acc = TA_StreamAccessAlloc( streamRef );
    if( isStreamRefBad( acc, size ) )
       return;
    TA_StreamAccessFree( acc );

    TA_StreamFree( streamRef );

    tmpInt1 = TA_MemUsed();
    if( tmpInt1 != originalMemSize )
    {
        TA_MemDisplay(stdout);
        TA_FATAL( "Memory leakage problem", tmpInt1, originalMemSize );
    }
}

static int isStreamRefBad( TA_STREAM_ACCESS *acc, unsigned int size )
{
    unsigned int i;
    unsigned int tmpInt1;

    for( i=0; i < size; i++ )
    {
        TA_StreamAccessGetBit( acc, 1, &tmpInt1 );

        if( (i % 3 + size)%2 )
        {
            if( tmpInt1 == 0 )
            {
                TA_FATAL( "Test failed for reading bit reference", size, i );
                return 1;
            }
        }
        else
        {
            if( tmpInt1 == 1 )
            {
                TA_FATAL( "Test failed for reading bit reference", size, i );
                return 1;
            }
        }
    }

    return 0;
}

/* Return 1/0 depending if both stream are identical. */
static int TA_StreamTestingEquality( const TA_STREAM *op1, const TA_STREAM *op2 )
{
    unsigned int equalityBit    = 1;
    unsigned int equalityByte   = 1;
    unsigned int equalityInt    = 1;
    unsigned int data1, data2;
    unsigned char byte1, byte2;
    unsigned int rv1, rv2;

    TA_STREAM_ACCESS *acc1, *acc2;

    /* Test equality with different method. All method must agree at the
     * very end.
     */

    /* Compare bit per bit. */
    acc1 = TA_StreamAccessAlloc( op1 );
    acc2 = TA_StreamAccessAlloc( op2 );

    rv1 = TA_StreamAccessGetBit( acc1, 1, &data1 );
    rv2 = TA_StreamAccessGetBit( acc2, 1, &data2 );
    while( (rv1 != NULL) && (rv2 != NULL ) && equalityBit )
    {
        if( data1 != data2 )
            equalityBit = 0;
        rv1 = TA_StreamAccessGetBit( acc1, 1, &data1 );
        rv2 = TA_StreamAccessGetBit( acc2, 1, &data2 );
    }
    TA_StreamTestingEquality_CheckSizeConsistency( op1, op2, rv1, rv2 );
    TA_StreamAccessFree( acc1 );
    TA_StreamAccessFree( acc2 );

    /* Compare byte per byte. */
    acc1 = TA_StreamAccessAlloc( op1 );
    acc2 = TA_StreamAccessAlloc( op2 );

    rv1 = TA_StreamAccessGetByte( acc1, &byte1 );
    rv2 = TA_StreamAccessGetByte( acc2, &byte2 );
    while( (rv1 != NULL) && (rv2 != NULL ) && equalityByte )
    {
        if( byte1 != byte2 )
            equalityByte = 0;
        rv1 = TA_StreamAccessGetByte( acc1, &byte1 );
        rv2 = TA_StreamAccessGetByte( acc2, &byte2 );
    }
    TA_StreamTestingEquality_CheckSizeConsistency( op1, op2, rv1, rv2 );
    TA_StreamAccessFree( acc1 );
    TA_StreamAccessFree( acc2 );

    /* Compare integer per integer. */
    acc1 = TA_StreamAccessAlloc( op1 );
    acc2 = TA_StreamAccessAlloc( op2 );

    rv1 = TA_StreamAccessGetInt32( acc1, &data1 );
    rv2 = TA_StreamAccessGetInt32( acc2, &data2 );
    while( (rv1 != NULL) && (rv2 != NULL ) && equalityInt )
    {
        if( data1 != data2 )
            equalityBit = 0;
        rv1 = TA_StreamAccessGetBit( acc1, 1, &data1 );
        rv2 = TA_StreamAccessGetBit( acc2, 1, &data2 );
    }
    TA_StreamTestingEquality_CheckSizeConsistency( op1, op2, rv1, rv2 );
    TA_StreamAccessFree( acc1 );
    TA_StreamAccessFree( acc2 );

    /* Verify the agreements. */
    if( equalityBit && equalityInt && equalityByte && (op1->size == op2->size) )
      return 1;
    else if ( (equalityBit == 0) && (equalityByte == 0) && (equalityInt == 0) )
      return 0;

    TA_FATAL( "TA_StreamTestingEquality failed", op1->size, op2->size );
    return 0;
}


static void TA_StreamTestingEquality_CheckSizeConsistency( const TA_STREAM *op1, const TA_STREAM *op2, unsigned int rv1, unsigned int rv2 )
{
    if( (rv1 == NULL) && (rv2 == NULL) )
    {
        /* Should be the same size. */
        if( op1->size != op2->size )
        {
            TA_FATAL( "TA_StreamTestingEquality failed.", op1->size, op2->size );
            return;
        }
    }
    else if( (rv1 == NULL) && (rv2 != NULL) )
    {
        /* Then op2 size should be higher than op1 size. */
        if( op2->size <= op1->size )
        {
            TA_FATAL( "TA_StreamTestingEquality failed.", op1->size, op2->size );
            return;
        }
    }
    else if( (rv2 == NULL) && (rv1 != NULL) )
    {
        /* Then op1 size should be higher than op2 size. */
        if( op1->size <= op2->size )
        {
            TA_FATAL( "TA_StreamTestingEquality failed.", op1->size, op2->size );
            return;
        }
    }
    else
    {
        TA_FATAL( "TA_StreamTestingEquality failed.", op1->size, op2->size );
        return;
    }
}


