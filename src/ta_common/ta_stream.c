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
 *  070101 MF   First version.
 *  062803 MF   Make TA_StreamAccessSearch case insensitive
 */

/* Description:
 *    Provides bit streaming functionality.
 *
 *    Streams can be merged, compress, expanded, validated with CRC, etc...
 *
 *    Provides also a portable streaming of TA_Integer, TA_Timestamp and TA_Real.
 *
 *    Although at first the implementation may seems complexe, this module
 *    is design in a way to make possible to make some very good speed
 *    optimization... eventually.
 *
 *    A stream is similar to a "buffer descriptor" mechanism provided by
 *    certain embedded OS. It is particularly needed in a protocol stack
 *    for adding header and trailer to the data while moving in the
 *    stack. The same can be done here with a TA_Stream.
 */

/**** Headers ****/
#include <stdlib.h>

#include "ta_common.h"
#include "ta_trace.h"
#include "ta_memory.h"
#include "ta_magic_nb.h"
#include "ta_global.h"

#include "sfl.h"

#ifndef TA_STREAM_H
   #include "ta_stream.h"
#endif

#include "bzip2/bzlib.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
struct TA_Stream_struct
{
  unsigned int    magicNb;

  unsigned char  *data;         /* Start of the data. */
  unsigned int    nbBitWritten; /* Number of bit in this block. */

  void           *freeData;   /* Ptr used to free the data. */
  TA_FreeFuncPtr  freeFunc;   /* TA_Free will be called when NULL. */
  void           *opaqueData; /* Transparently pass to the freeFunc. */

  unsigned int    allocSize;  /* Total nb byte allocated for 'freeData' */

  unsigned int    writePos; /* Indicate where the next bit will be written. */
  unsigned char   writeMask;

  unsigned int    nbByteToSkip; /* When decapsulating a TA_Stream, the header
                                 * bytes must be skip by the access functions.
                                 */

  struct TA_Stream_struct *next;
  struct TA_Stream_struct *lastBlock;
};

typedef struct TA_Stream_struct TA_StreamPriv;

typedef struct
{
  unsigned int     magicNb;

  unsigned int     endOfStream; /* No more bit in the stream. */

  const TA_StreamPriv *currentBlock;
  unsigned int         nbBitRead;   /* Nb bit read in the currentBlock. */

  /* Iterator within the currenBlock->freeData */
  unsigned int     readPos;
  unsigned char    readMask;
} TA_StreamAccessPriv;

/* A complete stream is a link list of block.
 * The size of the first block can be different
 * of the size of the block added (when the first block
 * is full).
 *
 * Note: These are the default size. Many blocks
 *       won't be of that size.
 */
#define TA_STREAM_FIRST_BLOCK_SIZE (8192)
#define TA_STREAM_OTHER_BLOCK_SIZE (4096)

/* There is no plan for this stream module to support more than 3 different
 * compression format (with the uncompress format, that makes a total of no
 * more than 4 types). This limit of 4 allows to code the type on two bits.
 * These two bits are append in front of the compressed information.
 * Of course, these two bits will allow the "decompresor" to use the right
 * algorithm.
 */
typedef enum
{
    TA_STREAM_NO_COMPRESSION,   /* No compression.   */
    TA_STREAM_BZIP2,            /* BZIP2 compression algorithm. */
    TA_STREAM_COMP2,            /* Undefined algorithm for the time being. */
    TA_STREAM_COMP3             /* Undefined algorithm for the time being. */
} TA_StreamType;

/**** Local functions declarations.    ****/
static TA_RetCode streamJumpToNextByteBoundary( TA_StreamPriv *stream, unsigned int *nbBitAdded );
static TA_RetCode accessJumpToNextByteBoundary( TA_StreamAccessPriv *streamAcc );
static TA_StreamPriv *streamAllocSize( unsigned int nbByte );
static TA_RetCode streamCompressBZIP2( const TA_StreamPriv *streamToCompress, TA_StreamPriv **retStream );
static TA_RetCode streamDecompressBZIP2( TA_StreamPriv *stream, TA_StreamAccessPriv *streamToDecompress );
static int accessMoveToNextBlock( TA_StreamAccessPriv *access );

static TA_RetCode streamPrivInit( TA_StreamPriv *streamPriv,
                                  unsigned char *data,
                                  unsigned int nbByte,
                                  TA_FreeFuncPtr freeFunc,
                                  void *opaqueData );

static void freeData( TA_StreamPriv *streamPriv );

static TA_RetCode calcCRC( TA_Stream *stream,
                           unsigned int start, /* First byte to include. Zero base. */
                           unsigned int stop,  /* Last byte to include. Zero base. */
                           unsigned int *crc32 );

static TA_RetCode markByteForSkip( TA_Stream *stream, unsigned int nbByteToSkip );


static void accessPrivCopy( TA_StreamAccessPriv *src, TA_StreamAccessPriv *dst );

static TA_RetCode streamGetHTMLTable( TA_StreamAccess *access,
                                      unsigned int maxElementSize,
                                      char *buffer,
                                      char *hrefBuffer,
                                      TA_HTMLTableFuncPtr funcPtr,
                                      void *opaqueData );

static TA_RetCode streamGetHREF( TA_StreamAccess *access,
                                 char *buffer,
                                 unsigned int bufferSize );
 
/**** Local variables definitions.     ****/
TA_FILE_INFO;

/**** Global functions definitions.   ****/

/* Use to alloc/dealloc a stream for the user. */
TA_Stream *TA_StreamAlloc( void )
{
   return (TA_Stream *)streamAllocSize( TA_STREAM_FIRST_BLOCK_SIZE );
}

TA_RetCode TA_StreamFree( TA_Stream *stream )
{
   TA_StreamPriv *streamPriv;
   TA_StreamPriv *tmp;

   streamPriv = (TA_StreamPriv *)stream;
   
   while( streamPriv )
   {
      if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
         return TA_BAD_OBJECT;

      freeData( streamPriv );

      tmp = streamPriv->next;
      TA_Free(  streamPriv );
      streamPriv = tmp;
   }

   return TA_SUCCESS;
}

TA_Stream *TA_StreamAllocFromBuffer( unsigned char *data,
                                     unsigned int dataSize,
                                     TA_FreeFuncPtr freeFunc,
                                     void *opaqueData )
{
   TA_StreamPriv *newStreamPriv;

   if( !data || !dataSize )
      return (TA_Stream *)NULL;

   newStreamPriv = (TA_StreamPriv *)TA_Malloc( sizeof( TA_StreamPriv ) );
   if( !newStreamPriv )
      return (TA_Stream *)NULL;

   newStreamPriv->data = data;
   newStreamPriv->freeData = data;

   /* Variables allowing to know how to free the data. */
   newStreamPriv->opaqueData = opaqueData;
   newStreamPriv->freeFunc = freeFunc; 

   /* Variables for keeping track on where the next bit is written. */
   newStreamPriv->writePos     = dataSize-1;     /* All byte written.  */
   newStreamPriv->writeMask    = 0x00;
   newStreamPriv->nbBitWritten = dataSize << 3;  /* All bit written.  */

   newStreamPriv->allocSize = dataSize; /* Nb of byte allocated. */

   /* Will allow to link this block to others. */
   newStreamPriv->next      = NULL;
   newStreamPriv->lastBlock = newStreamPriv;

   /* and the rest... */
   newStreamPriv->nbByteToSkip  = 0;
   newStreamPriv->magicNb   = TA_STREAM_MAGIC_NB;

   return (TA_Stream *)newStreamPriv;
}

/* Use to add an element at the end of the output stream. */
TA_RetCode TA_StreamAddBit( TA_Stream *stream, unsigned int bit )
{
   TA_StreamPriv *lastBlock;
   TA_StreamPriv *tempBlock;
   TA_StreamPriv *streamPriv;

   streamPriv = (TA_StreamPriv *)stream;

   if( !streamPriv )
      return TA_BAD_PARAM;

   if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
     return TA_BAD_OBJECT;

   lastBlock = streamPriv->lastBlock;

   /* Verify if a new byte is needed. */
   if( lastBlock->writeMask == 0x00 )
   {
      if( lastBlock->writePos >= (lastBlock->allocSize-1) )
      {
         /* Allocate a new block for the stream and add it at the end
          * of the link list of block.
          */
         tempBlock = streamAllocSize( TA_STREAM_OTHER_BLOCK_SIZE );
         if( !tempBlock )
            return TA_ALLOC_ERR;

         lastBlock->next       = tempBlock;
         streamPriv->lastBlock = lastBlock->next;
         lastBlock             = streamPriv->lastBlock;
      }
      else
      {
         lastBlock->writeMask = 0x80;
         lastBlock->writePos++;
      }

      lastBlock->data[lastBlock->writePos] = 0x00;
   }

   /* Write the bit and move the index to next one. */
   if( bit )
      lastBlock->data[lastBlock->writePos] |= lastBlock->writeMask;

   lastBlock->writeMask >>= 1;
   lastBlock->nbBitWritten++;

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddByte( TA_Stream *stream, unsigned char data )
{
   unsigned char mask = 0x80;
   TA_RetCode retCode;
   TA_StreamPriv *streamPriv;
   TA_StreamPriv *lastBlock;

   streamPriv = (TA_StreamPriv *)stream;

   if( !streamPriv )
      return TA_BAD_PARAM;

   if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
     return TA_BAD_OBJECT;

   lastBlock = streamPriv->lastBlock;

   /* Add byte in one shot if already on a byte boundary, else
    * add the 8 bits one by one with TA_StreamAddBit.
    */
#if 0
   !!! TO BE DEBUG

   if( (lastBlock->writeMask & 0x7F) == 0x00 )
   {
      /* Next byte written on a byte boundary. */
      if( lastBlock->writePos >= (lastBlock->allocSize-1) )
      {
         /* Allocate a new block for the stream and add it at the end
          * of the link list of block.
          */
         tempBlock = streamAllocSize( TA_STREAM_OTHER_BLOCK_SIZE );
         if( !tempBlock )
            return TA_ALLOC_ERR;

         lastBlock->next       = tempBlock;
         streamPriv->lastBlock = lastBlock->next;
         lastBlock             = streamPriv->lastBlock;
      }
      else
         lastBlock->writePos++;

      lastBlock->data[lastBlock->writePos] = data;
   }
   else
#endif

   {
      /* Add the bits one by one. */
      mask = 0x80;
      while( mask )
      {
         retCode = TA_StreamAddBit( stream, data & mask );
         if( retCode != TA_SUCCESS )
            return retCode;

         mask >>= 1;
      }
   }

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddInt16( TA_Stream *stream, unsigned short data )
{
   TA_RetCode retCode;
   unsigned char temp;

   temp = (unsigned char)(data>>8);
   retCode = TA_StreamAddByte( stream, temp );
   if( retCode != TA_SUCCESS )
      return retCode;

   temp = (unsigned char)data;
   retCode = TA_StreamAddByte( stream, temp );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddInt32( TA_Stream *stream, unsigned int data )
{
   TA_RetCode retCode;
   unsigned char temp;

   temp = (unsigned char)(data>>24);
   retCode = TA_StreamAddByte( stream, temp );
   if( retCode != TA_SUCCESS )
      return retCode;

   temp = (unsigned char)(data>>16);
   retCode = TA_StreamAddByte( stream, temp );
   if( retCode != TA_SUCCESS )
      return retCode;

   temp = (unsigned char)(data>>8);
   retCode = TA_StreamAddByte( stream, temp );
   if( retCode != TA_SUCCESS )
      return retCode;

   temp = (unsigned char)data;
   retCode = TA_StreamAddByte( stream, temp );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddInteger( TA_Stream *stream, TA_Integer data )
{
   /* Value store in a TA_Integer are assumed to not exceed 
    * a 32 bits value.
    */
   return TA_StreamAddInt32( stream, data );
}

TA_RetCode TA_StreamAddDouble( TA_Stream *stream, double data )
{
   int i;
   TA_RetCode retCode;

   /* !!! This is probably NOT portable and further
    * !!! research shall be done on this... eventually.
    */
   typedef union
   {
      double d;
      unsigned char b[8];
   } OUTDATA;

   OUTDATA outdata;
   TA_StreamPriv *streamPriv;

   streamPriv = (TA_StreamPriv *)stream;

   if( !streamPriv )
      return TA_BAD_PARAM;

   if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
     return TA_BAD_OBJECT;

   outdata.d = data;
   for( i=sizeof(double)-1; i >= 0; i-- )
   {
      retCode = TA_StreamAddByte( stream, outdata.b[i] );
      if( retCode != TA_SUCCESS )
         return retCode;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddReal( TA_Stream *stream, TA_Real data )
{
   /* Portable!? */
   return TA_StreamAddDouble( stream, data );
}

TA_RetCode TA_StreamAddBuffer( TA_Stream *stream,
                               unsigned char *data,
                               unsigned int dataSize,
                               TA_FreeFuncPtr freeFunc,
                               void *opaqueData )
{
   TA_StreamPriv *newStreamPriv;
   TA_StreamPriv *firstStreamPriv;

   if( !data || !stream || (dataSize <= 0) )
      return TA_BAD_PARAM;
   
   firstStreamPriv = (TA_StreamPriv *) stream;

   if( firstStreamPriv->magicNb != TA_STREAM_MAGIC_NB )
      return TA_BAD_OBJECT;

   newStreamPriv = (TA_StreamPriv *)TA_Malloc( sizeof( TA_StreamPriv ) );
   if( !newStreamPriv )
      return TA_ALLOC_ERR;

   newStreamPriv->data = data;
   newStreamPriv->freeData = data;

   /* Variables allowing to know how to free the data. */
   newStreamPriv->opaqueData = opaqueData;
   newStreamPriv->freeFunc = freeFunc; 

   /* Variables for keeping track on where the next bit is written. */
   newStreamPriv->writePos     = dataSize-1;     /* All byte written.  */
   newStreamPriv->writeMask    = 0x00;
   newStreamPriv->nbBitWritten = dataSize << 3;  /* All bit written.  */

   newStreamPriv->allocSize = dataSize; /* Nb of byte allocated. */

   /* Will allow to link this block to others. */
   newStreamPriv->next      = NULL;
   newStreamPriv->lastBlock = newStreamPriv;

   /* and the rest... */
   newStreamPriv->nbByteToSkip  = 0;
   newStreamPriv->magicNb   = TA_STREAM_MAGIC_NB;

   return TA_StreamMerge( stream, (TA_Stream *)newStreamPriv );
}

TA_RetCode TA_StreamAddString( TA_Stream *stream, const TA_String *string )
{
   TA_RetCode retCode;
   const char *strChar;
   unsigned int length, i;

   strChar = TA_StringToChar(string);

   length = strlen( strChar );
   if( length > 255 )
      return TA_BAD_PARAM;

   retCode = TA_StreamAddByte( stream, (unsigned char)length );
   if( retCode != TA_SUCCESS )
      return retCode;

   for( i=0; i < length; i++ )
   {
      retCode = TA_StreamAddByte( stream, strChar[i] );
      if( retCode != TA_SUCCESS )
         return retCode;
   }

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddTimestamp( TA_Stream *stream, const TA_Timestamp *timestamp )
{
   TA_RetCode retCode;

   retCode = TA_StreamAddInt32( stream, timestamp->date );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = TA_StreamAddInt32( stream, timestamp->time );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

/* Use to read the stream sequentially. */
TA_StreamAccess *TA_StreamAccessAlloc( const TA_Stream *stream )
{
   TA_StreamAccessPriv *accessPriv;
   const TA_StreamPriv *streamPriv;

   streamPriv = (const TA_StreamPriv *)stream;

   if( !streamPriv )
      return (TA_StreamAccess *)NULL;

   if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
      return (TA_StreamAccess *)NULL;
     
   accessPriv = (TA_StreamAccessPriv *) TA_Malloc( sizeof( TA_StreamAccessPriv ) );

   if( !accessPriv )
      return (TA_StreamAccess *)NULL;

   accessPriv->currentBlock = streamPriv;
   accessPriv->readPos      = 0;
   accessPriv->readMask     = 0x80;
   accessPriv->nbBitRead    = 0;
   accessPriv->endOfStream  = 0;
   accessPriv->magicNb      = TA_STREAM_ACCESS_MAGIC_NB;

   if( streamPriv->nbBitWritten == 0 )
   {
      /* The first block is totally skipped. Attempt to 
       * go to the next block having data.
       */
      accessMoveToNextBlock( accessPriv );
   }

   return (TA_StreamAccess *)accessPriv;
}

TA_RetCode TA_StreamAccessFree( TA_StreamAccess *access )
{
   TA_StreamAccessPriv *accessPriv;

   if( access )
   {
      accessPriv = (TA_StreamAccessPriv *)access;
      if( accessPriv->magicNb != TA_STREAM_ACCESS_MAGIC_NB )
         return TA_BAD_OBJECT;

      TA_Free( access );
   }

   return TA_SUCCESS;
}

TA_StreamAccess *TA_StreamAccessAllocCopy( const TA_StreamAccess *stream )
{
    TA_StreamAccessPriv *accessPriv = (TA_StreamAccessPriv *)stream;

    if( !stream )
       return NULL;
    if( accessPriv->magicNb != TA_STREAM_ACCESS_MAGIC_NB )
       return NULL;

    return TA_StreamAccessAlloc( (TA_Stream *)accessPriv->currentBlock );
}

/* Bypass the number of specified byte. */
TA_RetCode TA_StreamAccessBypass( TA_StreamAccess *access, unsigned int nbByteToBypass )
{
    unsigned int i;
    unsigned char dummy;
    TA_RetCode retCode;

    /* This function could be definitely re-written for speed optimization! */
    for( i=0; i < nbByteToBypass; i++ )
    {
        retCode = TA_StreamAccessGetByte(access, &dummy );
        if( retCode != TA_SUCCESS )
           return retCode;
    }

    return TA_SUCCESS;
}

TA_RetCode TA_StreamAccessGetBit( TA_StreamAccess *access, unsigned int nbBit, unsigned int *data )
{
    TA_StreamAccessPriv *accessPriv;
    unsigned int returnData = 0;
    unsigned int i;
    int bit;
  
    /* Note: This function could/should be easily speed optimized. */

    accessPriv = (TA_StreamAccessPriv *)access;

    /* Read only a maximum of 32 bits. */    
    if( nbBit > 32 )
       return TA_BAD_PARAM;

    if( accessPriv->endOfStream )
       return TA_END_OF_STREAM;

    /* The first bit read will be the MSB. */
    for( i=0; i < nbBit; i++ )
    {
        /* Read the bit. */
        bit = accessPriv->currentBlock->data[accessPriv->readPos] & accessPriv->readMask;
        returnData <<= 1;
        if( bit )
            returnData |= 1;

        accessPriv->nbBitRead++;

        /* Move to the next bit. */
        accessPriv->readMask >>= 1;

        /* Verify if a new byte is needed. */
        if( accessPriv->readMask == 0x00 )
        {
            accessPriv->readMask = 0x80;
            accessPriv->readPos++;
        }

        /* Verify if a new block is needed. */
        if( accessPriv->nbBitRead == accessPriv->currentBlock->nbBitWritten )
        {
            /* No more bit available in the current block.
             *
             * Move to the next block. Set endOfStream if no
             * other block available.
             */
            if( !accessMoveToNextBlock( accessPriv ) )
            {
                *data = returnData;
                accessPriv->endOfStream = 1;
                return TA_SUCCESS;
            }
        }
    }

    *data = returnData;

    return TA_SUCCESS;
}

TA_RetCode TA_StreamAccessGetByte( TA_StreamAccess *access, unsigned char *data )
{
    TA_RetCode retValue;
    unsigned int tmp;

    retValue = TA_StreamAccessGetBit( access, 8, &tmp );
    *data = (unsigned char)tmp;

    return retValue;
}

TA_RetCode TA_StreamAccessGetBuffer( TA_StreamAccess *streamAccess,
                                     const char   **theBuffer,
                                     unsigned int *theBufferSize )
{
    TA_StreamAccessPriv *accessPriv;
     
    accessPriv = (TA_StreamAccessPriv *)streamAccess;

    if( accessPriv->endOfStream )
       return TA_END_OF_STREAM;

    if( !theBuffer || !theBufferSize )
       return TA_BAD_PARAM;

    /* Return the current buffer. */
    *theBuffer = (const char *)&accessPriv->currentBlock->data[0];
    *theBufferSize = accessPriv->currentBlock->nbBitWritten>>3;

    /* Move to the next buffer. */
    if( !accessMoveToNextBlock( accessPriv ) )
       accessPriv->endOfStream = 1;

    return TA_SUCCESS;
}

/* Only the lowest 16 bits are significative. */
TA_RetCode TA_StreamAccessGetInt16( TA_StreamAccess *access, unsigned int *data )
{
    return TA_StreamAccessGetBit( access, 16, data );
}

TA_RetCode TA_StreamAccessGetInt32( TA_StreamAccess *access, unsigned int *data )
{
    return TA_StreamAccessGetBit( access, 32, data );
}

TA_RetCode TA_StreamAccessGetDouble( TA_StreamAccess *access, double *data )
{
    TA_RetCode retCode;
    unsigned int i;

    typedef union
    {
       double d;
       unsigned char b[8];
    } OUTDATA;

    OUTDATA outdata;

    outdata.d = 0; /* To get ride of warning. */

    if( !data )
       return TA_BAD_PARAM;

    for( i=0; i < sizeof(double); i++ )
    {
       retCode = TA_StreamAccessGetByte( access, &outdata.b[i] );
       if( retCode != TA_SUCCESS )
          return retCode;
    }

    *data = outdata.d;

    return TA_SUCCESS;
}

TA_RetCode TA_StreamAccessGetTimestamp( TA_StreamAccess *access,
                                        TA_Timestamp *data )
{
   TA_RetCode retCode;

   retCode = TA_StreamAccessGetInt32( access, (unsigned int *)&data->date );
   if( retCode != TA_SUCCESS )
      return retCode;

   retCode = TA_StreamAccessGetInt32( access, (unsigned int *)&data->time );
   if( retCode != TA_SUCCESS )
      return retCode;
 
   return TA_SUCCESS;
}

TA_RetCode TA_StreamAccessGetString( TA_StreamAccess *access, TA_String **string )
{
   char length;
   TA_RetCode retCode;
   unsigned char buffer[256];
   TA_String *stringPtr;
   int i;
   TA_StringCache *stringCache;
   TA_StreamAccessPriv *accessPriv;

   if( !string || !access )
      return TA_BAD_PARAM;

   accessPriv = (TA_StreamAccessPriv *)access;

   if( accessPriv->magicNb != TA_STREAM_ACCESS_MAGIC_NB )
      return TA_BAD_OBJECT;

   *string = NULL;

   retCode = TA_StreamAccessGetByte( access, (unsigned char *)&length );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Could be speed optimized, but let's keep it simple for the time being. */
   for( i=0; i < length; i++ )
   {
      retCode = TA_StreamAccessGetByte( access, &buffer[i] );
      if( retCode != TA_SUCCESS )
         return retCode;
   }

   stringCache = TA_GetGlobalStringCache();
       
   stringPtr = TA_StringAllocN( stringCache, (const char *)&buffer[0], length );
   if( !stringPtr )
      return TA_ALLOC_ERR;

   /* Success. Return the string to the caller. */
   *string = stringPtr;

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAccessSearch( TA_StreamAccess *source, const char *stringToFind )
{
   TA_RetCode retCode;
   TA_StreamAccessPriv movingPosition;
   unsigned int foundInString, stringLength;
   unsigned char data;

   /* The whole function could be speed optimized. Easily! */
   stringLength = strlen(stringToFind);
   if( stringLength == 0 )
      return TA_SUCCESS; /* Nothing to do. */

   /* Use a copy of the iterator for searching. */
   accessPrivCopy( (TA_StreamAccessPriv *)source, &movingPosition );

   foundInString = 0;
   while( foundInString != stringLength )
   {
      retCode = TA_StreamAccessGetByte( (TA_StreamAccess *)&movingPosition, &data );
      if( retCode != TA_SUCCESS )
         return retCode;
      if( toupper(stringToFind[foundInString]) == toupper(data) )
      {
         foundInString++;
      }         
      else
         foundInString = 0;         
   }
   
   /* On success, move the caller provided iterator
    * after the string.
    */
   accessPrivCopy( &movingPosition, (TA_StreamAccessPriv *)source );
  
   return TA_SUCCESS; 
}

TA_RetCode TA_StreamAccessGetHTMLTable( TA_StreamAccess *access,
                                        unsigned int maxElementSize,
                                        TA_HTMLTableFuncPtr funcPtr,
                                        void *opaqueData )
{
   TA_RetCode retCode;
   char *buffer;
   char *hrefBuffer;

   buffer = TA_Malloc( maxElementSize+1 );
   if( !buffer )
      return TA_ALLOC_ERR;

   hrefBuffer = TA_Malloc( maxElementSize+1 );
   if( !hrefBuffer )
   {
      TA_Free(  buffer );
      return TA_ALLOC_ERR;
   }

   buffer[maxElementSize] = '\0';
   hrefBuffer[maxElementSize] = '\0';

   retCode = streamGetHTMLTable( access, maxElementSize, buffer, hrefBuffer, funcPtr, opaqueData );

   TA_Free( buffer );
   TA_Free( hrefBuffer );

   return retCode;
}

/* Skip the next HTML Table.
 * (the access will point after the </table> tag)
 */
TA_RetCode TA_StreamAccessSkipHTMLTable( TA_StreamAccess *access )
{
   return TA_StreamAccessSearch( access, "</table>" );
}

unsigned int TA_StreamSizeInBit( const TA_Stream *stream )
{
    unsigned int totalSize = 0;
    TA_StreamPriv *streamPriv;

    streamPriv = (TA_StreamPriv *)stream;

    /* Add the size (in bits) found in all blocks of that stream. */
    while( streamPriv )
    {
       if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
          return 0;

       totalSize += streamPriv->nbBitWritten;
       streamPriv = streamPriv->next;
    }

    return totalSize;
}

unsigned int TA_StreamSizeInByte( const TA_Stream *stream )
{
    unsigned int tmp;
    unsigned int nbByte = 0;

    tmp = TA_StreamSizeInBit( stream );

    nbByte = tmp/8;
    if( tmp%8 )
       nbByte += 1;

    return nbByte;
}

TA_RetCode TA_StreamAppendCopy( TA_Stream *dst, const TA_Stream *src )
{
    TA_PROLOG
    TA_RetCode retCode;
    TA_StreamPriv *tmpStreamPriv;
    TA_StreamPriv *dstStreamPriv;
    TA_StreamPriv *srcStreamPriv;

    unsigned int nbByteToCopy;

    if( !dst || !src )
       return TA_BAD_PARAM;

    dstStreamPriv = (TA_StreamPriv *)dst;
    srcStreamPriv = (TA_StreamPriv *)src;

    if( (dstStreamPriv->magicNb != TA_STREAM_MAGIC_NB) ||
        (srcStreamPriv->magicNb != TA_STREAM_MAGIC_NB) )
       return TA_BAD_OBJECT;

    TA_TRACE_BEGIN( TA_StreamAppendCopy );
     
    /* The append is certainly not very efficient.
     * This function could easily be optimized in both speed
     * and memory efficiency by using a different algorithm.
     */

    /* Copy/Append block per block. */
    while( srcStreamPriv )
    {
        /* Make a copy of the 'src' block into a TA_Stream. */
        TA_ASSERT( srcStreamPriv->nbByteToSkip <= srcStreamPriv->allocSize );

        /* Do not bother to copy if there is no data in this block. */
        if( srcStreamPriv->nbBitWritten != 0 )
        {
           /* Could be more memory efficient by allocating only
            * the required bytes.
            */
           nbByteToCopy = srcStreamPriv->allocSize-srcStreamPriv->nbByteToSkip;

           tmpStreamPriv = streamAllocSize( nbByteToCopy );
           if( !tmpStreamPriv )
           {
              TA_TRACE_RETURN( TA_ALLOC_ERR );
           }

           memcpy( tmpStreamPriv->data, srcStreamPriv->data, nbByteToCopy );
  
           tmpStreamPriv->nbBitWritten = srcStreamPriv->nbBitWritten;
           tmpStreamPriv->allocSize    = nbByteToCopy;
           tmpStreamPriv->writePos     = srcStreamPriv->writePos;
           tmpStreamPriv->writeMask    = srcStreamPriv->writeMask;
           tmpStreamPriv->nbByteToSkip = 0;
           
           retCode = TA_StreamMerge( dst, (TA_Stream *)tmpStreamPriv );
           if( retCode != TA_SUCCESS )
           {
              TA_TRACE_RETURN( retCode );
           }
        }

        srcStreamPriv = srcStreamPriv->next;
    }

    TA_TRACE_RETURN( TA_SUCCESS );
}

unsigned int TA_StreamCountChar( const TA_Stream *stream, char toCount )
{
   TA_StreamAccess *streamAccess;
   TA_RetCode retCode;
   const char *theBuffer;
   unsigned int theBufferSize, total, i;

   /* !!! Could be speed optimized by avoiding a memory
    *     allocation here.
    */
   streamAccess = TA_StreamAccessAlloc( stream );

   if( !streamAccess )
      return 0;

   total = 0;

   do
   {   
      theBufferSize = 0;
      retCode = TA_StreamAccessGetBuffer( streamAccess,
                                          &theBuffer,
                                          &theBufferSize );
      if(retCode == TA_SUCCESS )
      {
         for( i=0; i < theBufferSize; i++ )
         {
            if( theBuffer[i] == toCount )
               total++;
         }
      }
   } while( retCode == TA_SUCCESS );

   TA_StreamAccessFree( streamAccess );

   return total;
}

TA_RetCode TA_StreamMerge( TA_Stream *firstStream, TA_Stream *secondStream )
{
    TA_StreamPriv *firstStreamPriv, *secondStreamPriv;

    if( !firstStream || !secondStream )
       return TA_SUCCESS;

    firstStreamPriv = (TA_StreamPriv *)firstStream;
    secondStreamPriv = (TA_StreamPriv *)secondStream;

    if( secondStreamPriv->nbBitWritten == 0 )
        TA_StreamFree( secondStream ); /* Free secondStream block and data. */
    else if( firstStreamPriv->nbBitWritten == 0 )
    {
        freeData( firstStreamPriv );
        *firstStreamPriv = *secondStreamPriv;

        /* No next block? Make sure we point to ourselve as the lastBlock. */
        if( !secondStreamPriv->next )
           firstStreamPriv->lastBlock = firstStreamPriv;

        TA_Free( secondStreamPriv ); /* Free only the block, not the data! */
    }
    else
    {
        /* Just append the second stream. */
        firstStreamPriv->lastBlock->next = secondStreamPriv;
        firstStreamPriv->lastBlock       = secondStreamPriv->lastBlock;
    }

    return TA_SUCCESS;
}

TA_RetCode TA_StreamCompress( TA_Stream *stream, const TA_Stream *streamToCompress )
{
    TA_PROLOG
    TA_RetCode retCode;
    TA_StreamPriv *bzipOutputStreamPriv;

    TA_StreamPriv *mostEfficientStreamPriv;
    TA_StreamType  mostEfficientType;
    unsigned int   mostEfficientSize;

    unsigned int nbBitWritten, nbBitAdded;

    TA_StreamPriv *streamToCompressPriv;
    TA_StreamPriv *streamPriv;

    if( !stream || !streamToCompress )
       return TA_BAD_PARAM;

    streamToCompressPriv = (TA_StreamPriv *)streamToCompress;
    streamPriv = (TA_StreamPriv *)stream;

    if( (streamToCompressPriv->magicNb != TA_STREAM_MAGIC_NB) ||
        (streamPriv->magicNb != TA_STREAM_MAGIC_NB) )
       return TA_BAD_OBJECT;

    TA_TRACE_BEGIN( TA_StreamCompress );

    /* Set variables used to keep track of the most efficient
     * compression algorithm.
     * Start by assuming the uncompress stream is the most
     * efficient.
     */
    mostEfficientType       = TA_STREAM_NO_COMPRESSION;
    mostEfficientSize       = TA_StreamSizeInBit( streamToCompress );
    mostEfficientStreamPriv = NULL;

    /* Perform BZIP2 compression. */
    retCode = streamCompressBZIP2( streamToCompressPriv, &bzipOutputStreamPriv );
    if( retCode != TA_SUCCESS )
    {
       TA_TRACE_RETURN( retCode );
    }

    /* Verify if BZIP is more efficient. */
    if( bzipOutputStreamPriv )
    {
        nbBitWritten = TA_StreamSizeInBit( (TA_Stream *)bzipOutputStreamPriv );
        if( nbBitWritten < mostEfficientSize )
        {
           mostEfficientType       = TA_STREAM_BZIP2;
           mostEfficientStreamPriv = bzipOutputStreamPriv;
           mostEfficientSize       = nbBitWritten;
        }
        else
            TA_StreamFree( (TA_Stream *)bzipOutputStreamPriv );
    }

    /* Write the compression type to the output stream. */
    retCode = TA_StreamAddBit( stream, mostEfficientType & 0x02 );
    if( retCode != TA_SUCCESS )
    {
       if( mostEfficientStreamPriv )
          TA_StreamFree( (TA_Stream *)mostEfficientStreamPriv );
       return retCode;
    }

    retCode = TA_StreamAddBit( stream, mostEfficientType & 0x01 );
    if( retCode != TA_SUCCESS )
    {
       if( mostEfficientStreamPriv )
          TA_StreamFree( (TA_Stream *)mostEfficientStreamPriv );
       return retCode;
    }

    /* Change the size to a 'byte' size if the data has been compressed
     * into another stream. This is because it is assumed the result of the
     * compression is ALWAYS on a byte boundary.
     */
    if( mostEfficientStreamPriv )
       mostEfficientSize = TA_StreamSizeInByte( (TA_Stream *)mostEfficientStreamPriv );

    /* Write the stream size (first bit indicates if the size is coded on
     * 16 or 32 bits).
     */
    if( mostEfficientSize < 65536 )
    {
        TA_StreamAddBit( stream, 0 ); /* 16 bit size */
        TA_StreamAddInt16( stream, (unsigned short)mostEfficientSize );
    }
    else
    {
        TA_StreamAddBit( stream, 1 ); /* 32 bit size */
        TA_StreamAddInt32( stream, mostEfficientSize );
    }

    /* Force the output stream on a byte boundary. */
    retCode = streamJumpToNextByteBoundary( (TA_StreamPriv *)stream, &nbBitAdded );
    if( retCode != TA_SUCCESS )
    {
       if( mostEfficientStreamPriv )
          TA_StreamFree( (TA_Stream *)mostEfficientStreamPriv );
       return retCode;
    }

    /* Add the compressed information to the output. */
    if( mostEfficientStreamPriv )
    {
        /* Merge with the compressed stream. */
        retCode = TA_StreamMerge( stream, (TA_Stream *)mostEfficientStreamPriv );
        if( retCode != TA_SUCCESS )
        {
           TA_StreamFree( (TA_Stream *)mostEfficientStreamPriv );
           return retCode;
        }
    }
    else
    {
        /* Copy the un-compress stream as-is. */
        retCode = TA_StreamAppendCopy( stream, streamToCompress );
        if( retCode != TA_SUCCESS )
           return retCode;
    }

    /* End-up the stream on a byte boundary. */
    streamJumpToNextByteBoundary( (TA_StreamPriv *)stream, &nbBitAdded );

    TA_TRACE_RETURN( TA_SUCCESS );
}

TA_RetCode TA_StreamDecompress( TA_StreamAccess *streamToDecompress,
                                TA_Stream **newAllocatedStream )
{
    TA_RetCode retCode;
    unsigned int  data;
    unsigned int  size, nbBit, nbByte;
    unsigned int i;
    TA_StreamType type;

    TA_StreamPriv *streamPriv;
    TA_Stream *stream;

    TA_StreamAccessPriv *accessPriv;

    if( !newAllocatedStream || !streamToDecompress )
       return TA_BAD_PARAM;

    *newAllocatedStream = NULL;

    accessPriv = (TA_StreamAccessPriv *)streamToDecompress;

    if( accessPriv->magicNb != TA_STREAM_ACCESS_MAGIC_NB )
       return TA_BAD_OBJECT;

    /* This function needs some re-work for being speed optimized as much
     * as possible.
     */

    /* Get the type. */
    retCode = TA_StreamAccessGetBit( streamToDecompress, 2, (unsigned int *)&type );
    if( retCode != TA_SUCCESS )
       return retCode;

    /* Get the size of the uncompress data. */
    retCode = TA_StreamAccessGetBit( streamToDecompress, 1, (unsigned int *)&data );
    if( retCode != TA_SUCCESS )
       return retCode;

    if( data == 0 )
    {
        retCode = TA_StreamAccessGetInt16( streamToDecompress, &size );
        if( retCode != TA_SUCCESS )
           return retCode;
    }
    else
    {
        retCode = TA_StreamAccessGetInt32( streamToDecompress, &size );
        if( retCode != TA_SUCCESS )
           return retCode;
    }

    if( size == 0 )
       return TA_BAD_PARAM;

    /* Translate the size in bit / byte. */
    switch( type )
    {
    case TA_STREAM_NO_COMPRESSION:
       nbBit = size;
       nbByte = nbBit/8;
       if( nbBit%8 )
          nbByte += 1;
       break;
    case TA_STREAM_BZIP2:
       nbByte = size;
       nbBit  = size << 3;
       break;
    default: /* Unrecognized compression. */
       return TA_BAD_PARAM;
    }

    /* Following the header, the data to decompress is assumed to be on
     * a byte boundary.
     */
    accessJumpToNextByteBoundary( (TA_StreamAccessPriv *)streamToDecompress );

    /* Allocate the target stream. */
    streamPriv = streamAllocSize( nbByte );
    if( !streamPriv )
       return TA_ALLOC_ERR;
    stream = (TA_Stream *)streamPriv;

    switch( type )
    {
    case TA_STREAM_NO_COMPRESSION:
        /* Make a bit per bit copy for the time being.
         * Could be easily speed optimized.
         */
        for( i=0;  i < nbBit; i++ )
        {
            retCode = TA_StreamAccessGetBit( streamToDecompress, 1, &data );
            if( retCode != TA_SUCCESS )
            {
               TA_StreamFree( stream );
               return retCode;
            }
            retCode = TA_StreamAddBit( stream, data );
            if( retCode != TA_SUCCESS )
            {
               TA_StreamFree( stream );
               return retCode;
            }
        }
        break;
    case TA_STREAM_BZIP2:
        retCode = streamDecompressBZIP2( streamPriv, accessPriv );
        if( retCode != TA_SUCCESS )
        {
           TA_StreamFree( stream );
           return retCode;
        }
        break;
    default:
        TA_StreamFree( stream );
        return TA_INTERNAL_ERROR(7);
    }

    /* Eat-up bits that could have been added at the end to keep things
     * on a byte boundary.
     */
    accessJumpToNextByteBoundary( (TA_StreamAccessPriv *)streamToDecompress );

    /* Success! Return the uncompress stream to the caller. */
    *newAllocatedStream = stream;

    return TA_SUCCESS;
}


/* Return a CRC-32 of the data in the stream. */
TA_RetCode TA_StreamCRC_32( TA_Stream *stream, unsigned int *crc32 )
{
   TA_PROLOG
   TA_RetCode retCode;
   TA_StreamPriv *streamPriv;
   qbyte runningData, temp;
   TA_StreamAccess *access;
   unsigned char data, lastData;

   temp = 0; /* To get ride of warning. */

   if( !stream || !crc32 )
      return TA_BAD_PARAM;

   streamPriv = (TA_StreamPriv *)stream;

   if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
      return TA_BAD_OBJECT;

   TA_TRACE_BEGIN(  TA_StreamCRC_32 );

   runningData = 0xFFFFFFFFL;

   access = TA_StreamAccessAlloc( stream );
   if( !access )
      return TA_SUCCESS;

   retCode = TA_StreamAccessGetByte( access, &data );
   while( retCode == TA_SUCCESS )
   {
      temp = calculate_running_crc( &runningData, &data, 1 );
      lastData = data;
      retCode = TA_StreamAccessGetByte( access, &data );
   } 

   TA_StreamAccessFree( access );

   if( retCode != TA_END_OF_STREAM  )
      return retCode;

   *crc32 = temp;
      
   TA_TRACE_RETURN( TA_SUCCESS );

   #if 0
   Possible speed optimization!?!?
   do
   {    
      if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
      {
          TA_TRACE_RETURN( TA_INTERNAL_ERROR(8) );
      }

      /* How many bytes in this block? */
      nbByteToInclude = streamPriv->allocSize-streamPriv->nbByteToSkip;
      nbByteWritten = streamPriv->nbBitWritten/8;
      if( (streamPriv->nbBitWritten)%8 )
         nbByteWritten += 1;
      nbByteToInclude = min(nbByteWritten, nbByteToInclude);

      /* Calculate the CRC for this block. */
      if( nbByteToInclude > 0 )
         temp = calculate_running_crc( &runningData, streamPriv->data, nbByteToInclude );

      /* Next block. */
      streamPriv = streamPriv->next;
   } while( streamPriv );
   #endif
}


TA_RetCode TA_StreamDecapsulate( TA_Stream *stream,
                                 TA_Timestamp *userTimestamp,
                                 TA_Integer   *userIdentifier )
{
   TA_RetCode retCode;
   TA_StreamAccess *access;   
   unsigned char data;
   unsigned int i, streamSize, streamSizeInByte;
   qbyte crc32Temp, crc32Header, crc32Content;

   if( !stream || !userTimestamp || !userIdentifier )
      return TA_BAD_PARAM;

   access = TA_StreamAccessAlloc( stream );
   if( !access )
      return TA_ALLOC_ERR;

   /* Only encoding V0.1 is currently supported. */
   retCode = TA_StreamAccessGetByte( access, &data );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamAccessFree( access );
      return retCode;
   }
   if( data != 0x01 )
   {
      TA_StreamAccessFree( access );
      return TA_UNSUPPORTED_STREAM_VERSION;
   }  

   /* Skip the unused bytes. */
   for( i=0; i < 3; i++ )
   {
      retCode = TA_StreamAccessGetByte( access, &data );
      if( retCode != TA_SUCCESS )
      {
         TA_StreamAccessFree( access );
         return retCode;
      }
   }

   /* Get the user timestamp. */
   retCode = TA_StreamAccessGetTimestamp( access, userTimestamp );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamAccessFree( access );
      return retCode;
   }

   /* Get the user identifier. */
   retCode = TA_StreamAccessGetInt32( access, (unsigned int *)userIdentifier );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamAccessFree( access );
      return retCode;
   }

   /* Get the size of the content. */
   retCode = TA_StreamAccessGetInt32( access, &streamSize );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamAccessFree( access );
      return retCode;
   }
   streamSizeInByte = streamSize / 8;
   if( streamSize % 8 )
      streamSizeInByte++;

   /* Get content CRC-32 */
   retCode = TA_StreamAccessGetInt32( access, (unsigned int *)&crc32Content );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamAccessFree( access );
      return retCode;
   }
   
   /* Get header CRC-32 */
   retCode = TA_StreamAccessGetInt32( access, (unsigned int *)&crc32Header );
   TA_StreamAccessFree( access );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* Calculate and verify the CRC of the header. */
   retCode = calcCRC( stream, 0, 23, (unsigned int *)&crc32Temp );
   if( retCode != TA_SUCCESS )
      return TA_BAD_STREAM_HEADER;

   if( crc32Temp != crc32Header )
      return TA_BAD_STREAM_HEADER_CRC;

   /* Calculate and verify the CRC of the content. */   
   retCode = calcCRC( stream, 28, 27+streamSizeInByte, (unsigned int *)&crc32Temp );
   if( retCode != TA_SUCCESS )
      return TA_BAD_STREAM_CONTENT;

   if( crc32Temp != crc32Content )
      return TA_BAD_STREAM_CRC;

   /* Mark the header for being skip from now on by all other
    * TA_StreamAccessXXXXX() functions.
    */
   retCode = markByteForSkip( stream, 28 );
   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAdjustToBoundary( TA_Stream *stream )
{
   unsigned int nbBitAdded;

   if( !stream )
      return TA_BAD_PARAM;

   return streamJumpToNextByteBoundary( (TA_StreamPriv *)stream, &nbBitAdded );
}

TA_RetCode TA_StreamEncapsulate( TA_Stream    **ptrToStream,
                                 TA_Timestamp  *userTimestamp,
                                 TA_Integer     userIdentifier )
{
   TA_RetCode retCode;
   TA_StreamPriv *streamPriv;
   unsigned int i, totalSize, nbBitAdded;
   TA_Stream *header;
   TA_Stream *stream;
   qbyte crc32;

   if( !ptrToStream || !userTimestamp )
      return TA_BAD_PARAM;

   stream = *ptrToStream;
   streamPriv = (TA_StreamPriv *)stream;

   if( !streamPriv )
      return TA_BAD_PARAM;

   if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
      return TA_BAD_OBJECT;

   /* Stream is completed to a byte boundary. */
   retCode = streamJumpToNextByteBoundary( (TA_StreamPriv *)stream, &nbBitAdded );
   if( retCode != TA_SUCCESS )
      return retCode;

   /* An header will be prefixed to the original TA_Stream:
    *     8 bits headerVersion (Version of this mechanism)
    *     8 bits nb bit appended to bring on byte boundary.
    *    16 bits RESERVED (unused for the time being)
    *    64 bits optional TA_Timestamp
    *    32 bits optional user data
    *    32 bits streamSize    (Size of the original stream in bit)
    *    32 bits streamCRC32   (CRC-32 of the original stream)
    *    32 bits headerCRC32   (CRC-32 of the header)
    */
   header = (TA_Stream *)streamAllocSize( sizeof( qbyte ) * 4 );
   if( !header )
      return TA_ALLOC_ERR;

   /* Add the version of the header encoding (V0.01) */
   retCode = TA_StreamAddByte( header, 0x01 );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Indicate the number of bits appended to bring the stream on a byte boundary. */
   retCode = TA_StreamAddByte( header, (unsigned char)nbBitAdded );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Add 16 unused bits to keep things on a 32 bit boundary. */
   for( i=0; i < 2; i++ )
   {
      retCode = TA_StreamAddByte( header, 0xCC );
      if( retCode != TA_SUCCESS )
      {
         TA_StreamFree( header );
         return retCode;
      }
   }

   /* Add the user timestamp. */
   retCode = TA_StreamAddTimestamp( header, userTimestamp );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Add the user identifier. */
   retCode = TA_StreamAddInt32( header, userIdentifier );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Add the size of the content in bits. */
   totalSize = TA_StreamSizeInBit( stream );
   if( totalSize == 0 )
   {
      TA_StreamFree( header );
      return TA_BAD_PARAM;
   }
   retCode = TA_StreamAddInt32( header, totalSize );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Add the CRC-32 of the content. */
   retCode = TA_StreamCRC_32( stream, (unsigned int *)&crc32 );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }
   retCode = TA_StreamAddInt32( header, crc32 );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Add the CRC-32 of the header at the end of the header. */
   retCode = TA_StreamCRC_32( header, (unsigned int *)&crc32 );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }
   retCode = TA_StreamAddInt32( header, crc32 );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }
   
   /* Merge the header to the content.  */
   retCode = TA_StreamMerge( header, stream );
   if( retCode != TA_SUCCESS )
   {
      TA_StreamFree( header );
      return retCode;
   }

   /* Everything succeed, the 'header' is the new stream */
   *ptrToStream = header; 
   return TA_SUCCESS;   
}

TA_RetCode TA_StreamToFile( TA_Stream *stream, FILE *out )
{
   TA_RetCode retCode;
   TA_StreamAccess *access;
   unsigned char data;

   access = TA_StreamAccessAlloc( stream );

   if( !access )
      return TA_ALLOC_ERR;

   retCode = TA_StreamAccessGetByte( access, &data );
   while( retCode == TA_SUCCESS )
   {
      fwrite(&data, 1, 1, out );
      retCode = TA_StreamAccessGetByte( access, &data );
   }

   TA_StreamAccessFree( access );

   return TA_SUCCESS;
}

TA_RetCode TA_StreamAddFile( TA_Stream *stream, FILE *in )
{
   TA_RetCode retCode;
   unsigned char data;
   unsigned int again; /* Boolean */

   /* !!! Could be speed optimized... shall we consider
    * a version using TA_FileSeq instead?
    */
   again = 1;
   do
   {
      if( !fread( &data, 1, 1, in ) )
         again = 0;
      else
      {
         retCode = TA_StreamAddByte( stream, data );
         if( retCode != TA_SUCCESS )
            return retCode;
      }
   } while( again && !feof(in) );

   return TA_SUCCESS;
}

#ifdef TA_DEBUG
TA_RetCode TA_StreamPrint( TA_Stream *stream )
{
   TA_StreamPriv *streamPriv;
   TA_StreamAccess *access;
   TA_RetCode retCode;
   FILE *out;

   unsigned char byte0,byte1,byte2,byte3,data;
   unsigned int firstIteration;

   streamPriv = (TA_StreamPriv *)stream;

   if( !stream )
      return TA_BAD_PARAM;

   out = TA_GetStdioFilePtr();

   access = TA_StreamAccessAlloc(stream);
   if( !access )
   {
      fprintf( out, "Printing stream failed: Mem alloc problem\n" );
      return TA_ALLOC_ERR;
   }

   byte0 = byte1 = byte2 = byte3 = 0xCD;

   retCode = TA_StreamAccessGetByte( access, &byte0 );
   firstIteration = 1;
   while( retCode == TA_SUCCESS )
   {      
      retCode = TA_StreamAccessGetByte( access, &data );

      if( retCode == TA_SUCCESS )
      {
         byte2 = byte3;
         byte3 = data;
      }

      if( firstIteration )
      {
         byte1 = data;
         firstIteration = 0;
      }
   }

   fprintf( out, "Stream %08lX:[%02X,%02X..%02X,%02X] NBit:%d NByte:%d Pos:%d,%02X\n",
                 (unsigned long)stream,
                 byte0, byte1, byte2, byte3,
                 TA_StreamSizeInBit ( stream ),
                 TA_StreamSizeInByte( stream ),
                 streamPriv->writePos, streamPriv->writeMask );
       
   retCode = TA_StreamAccessFree( access );

   if( retCode != TA_SUCCESS )
      return retCode;

   return TA_SUCCESS;
}
#endif

/**** Local functions definitions.     ****/
static int accessMoveToNextBlock( TA_StreamAccessPriv *accessPriv )
{
    TA_StreamPriv *nextBlock;
    int again;
    int retValue;

    retValue = 0;
    again = 1;
    while( again )
    {
       nextBlock = accessPriv->currentBlock->next;
       if( !nextBlock )
       {
          /* No next block available. Then make sure
           * the stream access point to the end of the current block,
           * and return an error.
           * From that point, that stream access should never work again.
           */
          accessPriv->nbBitRead = accessPriv->currentBlock->nbBitWritten;
          retValue = 0;
          again = 0;
       }
       else
       {
          accessPriv->currentBlock = nextBlock;
          if( nextBlock->nbBitWritten != 0 )
          {
             accessPriv->readPos      = 0;
             accessPriv->readMask     = 0x80;
             accessPriv->nbBitRead    = 0;

             retValue = 1;
             again = 0;
          }
       }
    }

    return retValue;
}

static TA_RetCode streamJumpToNextByteBoundary( TA_StreamPriv *stream, unsigned int *nbBitAdded )
{
   TA_RetCode retCode;
   int nbBitAdd;

   nbBitAdd = 0;

   while( (stream->lastBlock->writeMask) &&
          (stream->lastBlock->writeMask != 0x80))
   {
       retCode = TA_StreamAddBit( (TA_Stream *)stream, 0 );
       if( retCode != TA_SUCCESS )
          return retCode;

       nbBitAdd++;
   }

   *nbBitAdded = nbBitAdd;

   return TA_SUCCESS;
}

static TA_RetCode accessJumpToNextByteBoundary( TA_StreamAccessPriv *streamAcc )
{
    TA_RetCode retCode;
    unsigned int data;

    while( (streamAcc->readMask != 0x00) && (streamAcc->readMask != 0x80) )
    {
        retCode = TA_StreamAccessGetBit( (TA_StreamAccess *)streamAcc, 1, &data );
        if( retCode != TA_SUCCESS )
           return retCode;
    }

    return TA_SUCCESS;
}

static TA_StreamPriv *streamAllocSize( unsigned int nbByte )
{
   TA_RetCode retCode;
   TA_StreamPriv *streamPriv;
   unsigned char *data;

   streamPriv = (TA_StreamPriv *)TA_Malloc( sizeof( TA_StreamPriv ) );
   if( !streamPriv )
      return (TA_StreamPriv *)NULL;

   data = (unsigned char *)TA_Malloc( nbByte );
   if( !data )
   {
      TA_Free(  streamPriv );
      return (TA_StreamPriv *)NULL;
   }

   retCode = streamPrivInit( streamPriv, data, nbByte, NULL, NULL );
   if( retCode != TA_SUCCESS )
   {
      TA_Free(  streamPriv );
      return (TA_StreamPriv *)NULL;
   }

   return streamPriv;
}

static TA_RetCode streamPrivInit( TA_StreamPriv *streamPriv,
                                  unsigned char *data,
                                  unsigned int size,
                                  TA_FreeFuncPtr freeFunc,
                                  void *opaqueData )
{
   /* Initialize all members of a TA_Stream. */

   /* Note: freeData and data will stay the same for most of the TA_Stream.
    *       'data' will differ from 'freeData' when a TA_Stream is "decapsulated"
    *       (with TA_StreamDecapsulate). At this moment, 'data' will point 
    *       after the encapsulation header.
    *       The end-user will not be aware that the header is still kept in the
    *       block...
    */
   streamPriv->data = data;
   streamPriv->freeData = data;

   /* Variables allowing to know how to free the data. */
   streamPriv->opaqueData = opaqueData;
   streamPriv->freeFunc = freeFunc; 

   /* Variables for keeping track on where the next bit is written. */
   streamPriv->data[0]      = 0;    /* Initialize first byte to zero.      */
   streamPriv->writePos     = 0;    /* Will write in the first byte.       */
   streamPriv->writeMask    = 0x80; /* Will write in the first bit (MSB).  */
   streamPriv->nbBitWritten = 0;    /* Nb of written bit.. zero of course. */

   streamPriv->allocSize = size; /* Nb of byte that can be written. */

   /* Will allow to link this block to others. */
   streamPriv->next      = NULL;
   streamPriv->lastBlock = streamPriv;

   /* Variable allowing to skip an header once a stream is decapsulated. */
   streamPriv->nbByteToSkip  = 0;

   /* Management variables... */
   streamPriv->magicNb   = TA_STREAM_MAGIC_NB;

   return TA_SUCCESS;
}

static TA_RetCode streamCompressBZIP2( const TA_StreamPriv *streamToCompress, TA_StreamPriv **retStream )
{
    TA_PROLOG
    bz_stream bzipStream;
    unsigned int returnCode, nbByteInThisBlock;
    unsigned int compressionCompleted;
    int action;

    const TA_StreamPriv *currentInputBlock;
    TA_Stream *outputStream;
    TA_StreamPriv *currentOutputBlock;

    TA_StreamPriv *streamToCompressPriv;

    #ifdef TA_DEBUG
       FILE *out;
    #endif

    if( !streamToCompress || !retStream )
       return TA_BAD_PARAM;

    *retStream = NULL;

    streamToCompressPriv = (TA_StreamPriv *)streamToCompress;

    if( streamToCompressPriv->magicNb != TA_STREAM_MAGIC_NB )
       return TA_BAD_OBJECT;

    TA_TRACE_BEGIN(  streamCompressBZIP2 );

    /* No particular alloc/free mechanism needed. The BZIP library
     * will directly used the standard malloc/free.
     */
    bzipStream.bzalloc = NULL;
    bzipStream.bzfree  = NULL;
    bzipStream.opaque  = NULL;

    /* Initialize the BZIP2 compression library. */
    returnCode = BZ2_bzCompressInit( &bzipStream, 5, 0, 0 );
    if( returnCode != BZ_OK )
    {
        TA_FATAL(  "BZIP2 cannot compress data", returnCode, 0 );
    }

    /* Now... compress block per block. */
    action = BZ_RUN;
    compressionCompleted = 0;

    /* Initialize output stream. */
    outputStream = TA_StreamAlloc();
    if( !outputStream )
    {
       TA_TRACE_RETURN( TA_ALLOC_ERR );
    }

    currentOutputBlock = (TA_StreamPriv *)outputStream;
    bzipStream.avail_out = currentOutputBlock->allocSize;
    bzipStream.next_out  = (char *)currentOutputBlock->data;

    /* Initialize input stream. */
    currentInputBlock = streamToCompress;
    bzipStream.next_in = (char *)currentInputBlock->data;
    nbByteInThisBlock = currentInputBlock->nbBitWritten>>3; /* Divid by 8. */
    if( currentInputBlock->nbBitWritten & 0x07 )
       nbByteInThisBlock++;
    bzipStream.avail_in = nbByteInThisBlock;

    do
    {
        /* Is new input data needed? */
        if( (action == BZ_RUN) && (bzipStream.avail_in == 0) )
        {
            /* Get the next block. */
            if( currentInputBlock->next )
            {
                currentInputBlock = currentInputBlock->next;
                bzipStream.next_in = (char *)currentInputBlock->data;
                nbByteInThisBlock = currentInputBlock->nbBitWritten>>3; /* Divid by 8. */
                if( currentInputBlock->nbBitWritten & 0x07 )
                    nbByteInThisBlock++;
                bzipStream.avail_in = nbByteInThisBlock;
            }
            else
            {
                /* no further data available. */
                action = BZ_FINISH;
            }
        }

        /* Is new output buffer needed? */
        if( bzipStream.avail_out == 0 )
        {
            if( (TA_StreamPriv *)outputStream != currentOutputBlock )
                TA_StreamMerge( outputStream, (TA_Stream *)currentOutputBlock );
            currentOutputBlock = (TA_StreamPriv *)TA_StreamAlloc();
            bzipStream.avail_out = currentOutputBlock->allocSize;
            bzipStream.next_out  = (char *)currentOutputBlock->data;
        }

        /* Call the BZIP2 library to proceed with compression. */
        returnCode = BZ2_bzCompress( &bzipStream, action );

        /* Always adjust the output block for reflecting the byte added
         * by the BZIP2 library.
         */
        currentOutputBlock->writePos  = currentOutputBlock->allocSize-bzipStream.avail_out;
        currentOutputBlock->writeMask = 0x00;
        currentOutputBlock->nbBitWritten = currentOutputBlock->writePos << 3;

        switch( returnCode )
        {
        case BZ_RUN_OK:
        case BZ_FINISH_OK:
            /* Everything is fine. Keep working... */
            break;
        case BZ_STREAM_END:
            if( (TA_StreamPriv *)outputStream != currentOutputBlock )
                TA_StreamMerge( outputStream, (TA_Stream *)currentOutputBlock );
            compressionCompleted = 1;
            break;
        default:
            if( (TA_StreamPriv *)outputStream != currentOutputBlock )
                TA_StreamFree( (TA_Stream *)currentOutputBlock );
            TA_StreamFree( outputStream );
            TA_FATAL(  "BZIP2 cannot compress data", returnCode, 0 );
        }
    } while( !compressionCompleted );

    #ifdef TA_DEBUG
    out = TA_GetStdioFilePtr();
    if( out )
       fprintf( out, "Compression completed: Before: %d  After: %d\n",
                bzipStream.total_in_lo32, bzipStream.total_out_lo32 );
    #endif

    /* Free all ressources used by the BZIP2 library. */
    returnCode = BZ2_bzCompressEnd( &bzipStream );
    if( returnCode != BZ_OK )
    {
        TA_StreamFree( outputStream );
        TA_FATAL(  "BZIP2 cannot release ressources", returnCode, 0 );
    }

    *retStream = (TA_StreamPriv *)outputStream;

    TA_TRACE_RETURN( TA_SUCCESS );
}

static TA_RetCode streamDecompressBZIP2( TA_StreamPriv *stream, TA_StreamAccessPriv *streamToDecompress )
{
    TA_PROLOG
    TA_RetCode retCode;
    bz_stream bzipStream;
    unsigned int returnCode, nbByteInThisBlock;
    unsigned int decompressionCompleted;

    TA_Stream *outputStream;
    TA_StreamPriv *currentOutputBlock;
    const TA_StreamPriv *currentInputBlock;

    if( !stream || !streamToDecompress )
       return TA_BAD_PARAM;

    if( (stream->magicNb != TA_STREAM_MAGIC_NB) ||
        (streamToDecompress->magicNb != TA_STREAM_ACCESS_MAGIC_NB ) )
       return TA_BAD_OBJECT;

    TA_TRACE_BEGIN(  streamDecompressBZIP2 );
        
    /* No particular alloc/free mechanism needed. The BZIP library
     * will directly used the standard malloc/free.
     */
    bzipStream.bzalloc = NULL;
    bzipStream.bzfree  = NULL;
    bzipStream.opaque  = NULL;

    /* Initialize the BZIP2 compression library. */
    returnCode = BZ2_bzDecompressInit( &bzipStream, 0, 0 );
    if( returnCode != BZ_OK )
    {
        TA_FATAL(  "BZIP2 cannot decompress data", returnCode, 0 );
    }

    /* Now... decompress block per block. */
    decompressionCompleted = 0;

    /* Initialize output stream. */
    outputStream = TA_StreamAlloc();
    if( !outputStream )
    {
       TA_TRACE_RETURN( TA_ALLOC_ERR );
    }

    currentOutputBlock = (TA_StreamPriv *)outputStream;
    bzipStream.avail_out = currentOutputBlock->allocSize;
    bzipStream.next_out  = (char *)currentOutputBlock->data;

    /* Initialize input stream. */
    retCode = accessJumpToNextByteBoundary( streamToDecompress );
    if( retCode != TA_SUCCESS )
       return TA_BAD_PARAM;

    currentInputBlock = streamToDecompress->currentBlock;
    bzipStream.next_in = (char *)&currentInputBlock->data[streamToDecompress->readPos];
    nbByteInThisBlock  = currentInputBlock->nbBitWritten>>3; /* Divid by 8. */
    if( currentInputBlock->nbBitWritten & 0x07 )
       nbByteInThisBlock++;
    bzipStream.avail_in = nbByteInThisBlock-streamToDecompress->readPos;

    do
    {
        /* Is new input data needed? */
        if( bzipStream.avail_in == 0 )
        {
            /* Get the next block. */
            if( !accessMoveToNextBlock( streamToDecompress ) )
                TA_FATAL(  "Decompression failed. No input byte left", 0, 0 );

            currentInputBlock = streamToDecompress->currentBlock;
            bzipStream.next_in = (char *)&currentInputBlock->data[0];
            nbByteInThisBlock  = currentInputBlock->nbBitWritten>>3; /* Divid by 8. */
            if( currentInputBlock->nbBitWritten & 0x07 )
                nbByteInThisBlock++;
            bzipStream.avail_in = nbByteInThisBlock;
        }

        /* Is new output buffer needed? */
        if( bzipStream.avail_out == 0 )
        {
            if( (TA_StreamPriv *)outputStream != currentOutputBlock )
                TA_StreamMerge( outputStream, (TA_Stream *)currentOutputBlock );
            currentOutputBlock = (TA_StreamPriv *)TA_StreamAlloc();
            bzipStream.avail_out = currentOutputBlock->allocSize;
            bzipStream.next_out  = (char *)currentOutputBlock->data;
        }

        /* Call the BZIP2 library to proceed with compression. */
        returnCode = BZ2_bzDecompress( &bzipStream );

        /* Always adjust the output block for reflecting the byte added
         * by the BZIP2 library.
         */
        currentOutputBlock->writePos  = currentOutputBlock->allocSize-bzipStream.avail_out;
        currentOutputBlock->writeMask = 0x00;
        currentOutputBlock->nbBitWritten = currentOutputBlock->writePos << 3;

        switch( returnCode )
        {
        case BZ_OK:
            /* Everything is fine. Keep working... */
            break;
        case BZ_STREAM_END:
            if( (TA_StreamPriv *)outputStream != currentOutputBlock )
                TA_StreamMerge( outputStream, (TA_Stream *)currentOutputBlock );
            decompressionCompleted = 1;
            break;
        default:
            if( (TA_StreamPriv *)outputStream != currentOutputBlock )
                TA_StreamFree( (TA_Stream *)currentOutputBlock );
            TA_StreamFree( outputStream );
            TA_FATAL(  "BZIP2 cannot decompress data", returnCode, 0 );
        }
    } while( !decompressionCompleted );

    /* Adjust the ta_stream_access to reflect the bytes consume
     * by the BZIP2 library.
     */
    if( bzipStream.avail_in == 0 )
    {
        accessMoveToNextBlock( streamToDecompress );
    }
    else
    {
        TA_StreamAccessBypass( (TA_Stream *)streamToDecompress, nbByteInThisBlock-bzipStream.avail_in );
    }
    /* Free all ressources used by the BZIP2 library. */
    returnCode = BZ2_bzDecompressEnd( &bzipStream );
    if( returnCode != BZ_OK )
    {
        TA_StreamFree( outputStream );
        TA_FATAL(  "BZIP2 cannot release ressources", returnCode, 0 );
    }

    retCode = TA_StreamMerge( (TA_Stream *)stream, outputStream );

    TA_TRACE_RETURN( retCode );
}

/* The following function is defined for handling internal errors
 * from the bzip library.
 */
void bz_internal_error ( int errCode )
{
    (void)errCode; /* To get ride of warning. */
    /* TA_FATAL( "BZIP internal error occured", errCode, 0 ); */
}

static void freeData( TA_StreamPriv *streamPriv )
{   
   if( streamPriv->freeData )
   {
      if( streamPriv->freeFunc )
         streamPriv->freeFunc(  streamPriv->freeData, streamPriv->opaqueData );
      else
         TA_Free(  streamPriv->freeData );
   }
}

static TA_RetCode markByteForSkip( TA_Stream *stream, unsigned int nbByteToSkip )
{
   TA_StreamPriv *streamPriv;
   unsigned int nbByteToSkipInThisBlock, nbByte;

   if( !stream || !nbByteToSkip )
      return TA_BAD_PARAM;

   streamPriv = (TA_StreamPriv *)stream;

   while( nbByteToSkip && streamPriv )
   {
      if( streamPriv->magicNb != TA_STREAM_MAGIC_NB )
         return TA_BAD_OBJECT;


      nbByte = streamPriv->nbBitWritten/8;
      if( streamPriv->nbBitWritten%8 )
        nbByte += 1;

      nbByteToSkipInThisBlock = min( nbByte, nbByteToSkip );

      streamPriv->data += nbByteToSkipInThisBlock;
      streamPriv->nbBitWritten -= nbByteToSkipInThisBlock*8;
      streamPriv->nbByteToSkip = nbByteToSkipInThisBlock;

      nbByteToSkip -= nbByteToSkipInThisBlock;
      streamPriv = streamPriv->next;
   }

   if( nbByteToSkip != 0 )
      return TA_INTERNAL_ERROR(10);

   return TA_SUCCESS;
}

static TA_RetCode calcCRC( TA_Stream *stream,
                           unsigned int start, /* First byte to include. Zero base. */
                           unsigned int stop,  /* Last byte to include. Zero base. */
                           unsigned int *crc32 )
{
   TA_RetCode retCode;
   TA_StreamAccess *access;
   unsigned int temp, i;
   unsigned char data, lastData;
   qbyte runningData;

   temp = 0; /* To get ride of warning. */

   if( !stream )
      return TA_BAD_PARAM;

   if( stop < start )
      return TA_BAD_PARAM;

   access = TA_StreamAccessAlloc( stream );
   if( !access )
      return TA_ALLOC_ERR;

   if( start != 0 )
   {
      retCode = TA_StreamAccessBypass( access, start );
      if( retCode != TA_SUCCESS )
      {
         TA_StreamAccessFree( access );
         return retCode;
      }
   }

   /* The following can be speed optimized. Big time! */
   runningData = 0xFFFFFFFFL;
   for( i=0; i <= (stop-start); i++ )
   {
      retCode = TA_StreamAccessGetByte( access, &data );
      if( retCode != TA_SUCCESS )
      {
         TA_StreamAccessFree( access );
         return retCode;
      }

      temp = calculate_running_crc( &runningData, &data, 1 );
      lastData = data;
   }

   TA_StreamAccessFree( access );

   *crc32 = temp;
   return TA_SUCCESS;   
}

static void accessPrivCopy( TA_StreamAccessPriv *src, TA_StreamAccessPriv *dst )
{
  /* Really simple... */
  *dst = *src;
}

static TA_RetCode streamGetHTMLTable( TA_StreamAccess *accessStart,
                                      unsigned int maxElementSize,
                                      char *buffer,
                                      char *hrefBuffer,
                                      TA_HTMLTableFuncPtr funcPtr,
                                      void *opaqueData )
{
   TA_RetCode retCode;
   TA_StreamAccessPriv accessPriv;
   TA_StreamAccess *access;
   unsigned int endOfTag, lineTag, columnTag, tableTag, skipTag;
   unsigned int line, column;
   unsigned int bufPos, again;
   unsigned char data;
   int tableLevel;
   int skipRestOfTable;

   accessPrivCopy( (TA_StreamAccessPriv *)accessStart,  &accessPriv );
 
   access = (TA_StreamAccess *)&accessPriv;

   line = column = bufPos = 0;
   
   *hrefBuffer = '\0';

   /* Find the start of the table. 
    * Position 'access' immediatly after 
    * the '>' of the <table ...> tag
    */
   retCode = TA_StreamAccessSearch( access, "<table" );
   if( retCode != TA_SUCCESS ) return retCode;
   do
   {
      retCode = TA_StreamAccessGetByte( access, &data );
      if( retCode != TA_SUCCESS ) return retCode;
   } while( data != '>' );

   /* Ok... not the best parsing approach on earth... but it
    * works.
    * Parse only HTML with good syntax (matching tags),
    * else an error is returned.
    */
   tableLevel = 1;
   again = 1;
   skipRestOfTable = 0;
   do
   {
      retCode = TA_StreamAccessGetByte( access, &data );
      if( retCode != TA_SUCCESS ) return retCode;

      /* Process tags. */
      endOfTag = 0;
      columnTag = 0;
      lineTag = 0;
      tableTag = 0;
      endOfTag = 0;
      skipTag = 0;

      if( data == '<' )
      {
         retCode = TA_StreamAccessGetByte( access, &data );
         if( retCode != TA_SUCCESS ) return retCode;

         /* Start or end of a tag? */
         if( data == '/' )
         {
            endOfTag = 1;
            retCode = TA_StreamAccessGetByte( access, &data );
            if( retCode != TA_SUCCESS ) return retCode;
         }
         else
            endOfTag = 0;

         /* Identify the tag. */                              
         switch( toupper(data) )
         {
         case 'T':
            retCode = TA_StreamAccessGetByte( access, &data );
            if( retCode != TA_SUCCESS ) return retCode;
            switch( toupper(data) )
            {
            case 'A':
               tableTag = 1;
               break;
            case 'D':
               columnTag = 1;
               break;
            case 'R':
               lineTag = 1;
               break;
            }
            break;
         case 'A':
            retCode = TA_StreamAccessGetByte( access, &data );
            if( retCode != TA_SUCCESS ) return retCode;
            if( isspace( toupper(data) ) )
            {
               /* Extract a "href=x>" where 'x' is returned in
                * hrefBuffer as a NULL terminated string.
                */
               retCode = streamGetHREF( access, hrefBuffer, maxElementSize );
               if( retCode != TA_SUCCESS )
                  return retCode;
               data = '>'; /* Because streamGetHREF eat the ending '>' */
            }
         }


         /* Confirm if yes or no this is a tableTag.
          * If not it will be skip.
          */
         if( tableTag )
         {
            tableTag = 0;
            retCode = TA_StreamAccessGetByte( access, &data );
            if( retCode != TA_SUCCESS ) return retCode;
            if( toupper(data) == 'B' )
            {
               retCode = TA_StreamAccessGetByte( access, &data );
               if( retCode != TA_SUCCESS ) return retCode;
               if( toupper(data) == 'L' )
               {
                  retCode = TA_StreamAccessGetByte( access, &data );
                  if( retCode != TA_SUCCESS ) return retCode;
                  if( toupper(data) == 'E' )
                     tableTag = 1; /* Confirmed. */
               }
            }
         }

            /* Handle the processing of the <TD>, </TD>, <TR>, </TR>,
             * <TABLE> and </TABLE> tag.
             */
            if( lineTag )
            {
               if( endOfTag )
               {  /* This is a new line. */
                  line++;
                  column = 0;
                  bufPos = 0;
                  *hrefBuffer = '\0';
               }
            }
            else if( tableTag )
            {
               if( !endOfTag )
               {
                  bufPos = 0; /* Begining of a table. */
                  *hrefBuffer = '\0';
                  tableLevel++;
               }
               else
               {
                  tableLevel--;
                  if( tableLevel <= 0 )
                     again = 0; /* End of table. Processing completed. */
               }
            }
            else if( columnTag )
            {
               if( endOfTag )
               {
                  /* Process one element of the table. */                 
                  if( !skipRestOfTable )
                  {
                     buffer[bufPos] = '\0';
                     retCode = funcPtr( line, column, buffer, hrefBuffer, opaqueData );
                     if( retCode == TA_FINISH_TABLE )
                        skipRestOfTable = 1;
                     else if( retCode != TA_SUCCESS )
                        return retCode;
                  }
                  column++;
                  bufPos = 0;
                  *hrefBuffer = '\0';
               }
            }

         /* Skip the rest of the tag. */
         while( data != '>' )
         {
            retCode = TA_StreamAccessGetByte( access, &data );
            if( retCode != TA_SUCCESS ) return retCode;
            if( data == '<' )
               return TA_BAD_HTML_SYNTAX;
         }
      }
      else
      {
         /* This is not within a tag... just accumulate the text. */
         if( bufPos < maxElementSize )
         {
            /* Do some replacement */
            if( isspace( data ) )
               data = ' ';

            buffer[bufPos] = data;
            bufPos++;
         }
      }

   } while( again );
   
   return TA_SUCCESS;
}

static TA_RetCode streamGetHREF( TA_StreamAccess *access,
                                 char *buffer, unsigned int bufferSize )
{
   TA_RetCode retCode;
   unsigned int i, again;
   unsigned char data;

   /* Extract a "href=x>" where 'x' is returned in
    * buffer as a NULL terminated string.
    * Remove potentialy double quotes surrounding
    * the x like this: "x"
    */
   retCode = TA_StreamAccessSearch( access, "href=" );
   if( retCode != TA_SUCCESS )
      return retCode;

   i = 0;
   again = 1;
   do
   { 
      retCode = TA_StreamAccessGetByte( access, &data );
      if( retCode != TA_SUCCESS )
      {
         *buffer = '\0';
         return retCode;
      }

      if( data != '"' )
      {   
         if( data == '>' )
            again = 0;
         else if( i < (bufferSize-1) )
            buffer[i++] = data;
      }
   } while( again );

   buffer[i] = '\0';
   
   return TA_SUCCESS;
}
