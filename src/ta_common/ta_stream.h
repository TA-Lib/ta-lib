#ifndef TA_STREAM_H
#define TA_STREAM_H

#ifndef TA_MEMORY_H
   #include "ta_memory.h"
#endif

#ifndef TA_STRING_H
   #include "ta_string.h"
#endif

/* Important Note: This stream module is absolutly not protected for
 *                 multithread access. Make sure you use mutex around the 
 *                 access of a particular stream.
 *
 * Also, it is strongly suggested to keep everything on a BYTE boundary.
 *
 * Two reason for this:
 *   1) It is simply more efficient.
 *   2) Mixing non-boundary data with compressed/encapsulate stream
 *      can become quite complexe and error prone.
 *
 * So when using directly TA_StreamAddBit(), make sure you complete to 
 * a byte boundary eventually before mixing with other type of data. All
 * other TA_StreamAddXXXXX() functions can be used without worrying about
 * this.
 * If you are not sure if you are on a byte boundary, call
 * TA_StreamAdjustToBoundary() and the stream will be adjusted if needed.
 */
typedef unsigned int TA_Stream;
typedef unsigned int TA_StreamAccess;

/* Use to alloc/dealloc a stream. */
TA_Stream *TA_StreamAlloc( TA_Libc *libHandle );
TA_RetCode TA_StreamFree( TA_Stream *stream );

/* Alloc a TA_Stream from an existing buffer. 
 * On succcess, the buffer belongs to the TA_Stream and 
 * will be freed by the calller provided freeFunc when
 * the TA_StreamFree gets called.
 */
TA_Stream *TA_StreamAllocFromBuffer( TA_Libc *libHandle,
                                     unsigned char *data,
                                     unsigned int dataSize,
                                     TA_FreeFuncPtr funcPtr,
                                     void *opaqueData );


/* Use to append an element to the stream. */

/* Fundamental types. */
TA_RetCode TA_StreamAddBit  ( TA_Stream *stream, unsigned int   data );
TA_RetCode TA_StreamAddByte ( TA_Stream *stream, unsigned char  data );
TA_RetCode TA_StreamAddInt16( TA_Stream *stream, unsigned short data );
TA_RetCode TA_StreamAddInt32( TA_Stream *stream, unsigned int   data );

/* TA-LIB types. */
TA_RetCode TA_StreamAddInteger  ( TA_Stream *stream, TA_Integer data );
TA_RetCode TA_StreamAddReal     ( TA_Stream *stream, TA_Real    data );
TA_RetCode TA_StreamAddString   ( TA_Stream *stream, const TA_String *string );
TA_RetCode TA_StreamAddTimestamp( TA_Stream *stream, const TA_Timestamp *timestamp );

/* Append the content of a file to the stream. */
TA_RetCode TA_StreamAddFile( TA_Stream *steam, FILE *in );

/* Append a buffer to the stream.
 * On success, the buffer belongs to the stream.
 * By default the data is freed by TA_Free unless
 * a TA_FreeFuncPtr is specified.
 */
TA_RetCode TA_StreamAddBuffer( TA_Stream *stream,
                               unsigned char *data,
                               unsigned int dataSize,
                               TA_FreeFuncPtr funcPtr,
                               void *opaqueData );

/* Bring the stream on a byte boundary. */
TA_RetCode TA_StreamAdjustToBoundary( TA_Stream *stream );

/* Use to read the stream sequentially.
 * The TA_StreamAccessGetXXXXX() functions return TA_END_OF_STREAM 
 * when there is no more data to read.
 *
 * A stream can safely be read by multiple TA_StreamAccess simultaneously.
 *
 * It is impossible to read backward in a stream (an alternative is
 * to re-allocate another TA_StreamAccess and start over!)
 */
TA_StreamAccess *TA_StreamAccessAlloc( const TA_Stream *stream );
TA_StreamAccess *TA_StreamAccessAllocCopy( const TA_StreamAccess *stream );

TA_RetCode TA_StreamAccessFree( TA_StreamAccess *streamAccess );

TA_RetCode TA_StreamAccessGetBit( TA_StreamAccess *streamAccess,
                                  unsigned int nbBit,
                                  unsigned int *data );

TA_RetCode TA_StreamAccessGetByte( TA_StreamAccess *streamAccess,
                                   unsigned char *data );

TA_RetCode TA_StreamAccessGetInt16( TA_StreamAccess *streamAccess,
                                    unsigned int  *data );

TA_RetCode TA_StreamAccessGetInt32( TA_StreamAccess *streamAccess,
                                    unsigned int  *data );

TA_RetCode TA_StreamAccessGetInteger( TA_StreamAccess *streamAccess,
                                      TA_Integer *data );

TA_RetCode TA_StreamAccessGetReal( TA_StreamAccess *streamAccess,
                                   TA_Real *data );

TA_RetCode TA_StreamAccessGetTimestamp( TA_StreamAccess *streamAccess,
                                        TA_Timestamp *data );


/* Get a string. This one is slightly different because
 * a TA_String will be allocated from this call. This is
 * why the address of a pointer is passed instead.
 */
TA_RetCode TA_StreamAccessGetString( TA_StreamAccess *streamAccess,
                                     TA_String **string );

/* Get the data chunk by chunk for speed efficiency.
 * Shall be used when it is known that the data is on
 * byte boundaries. MUST NOT be mix with any other
 * TA_StreamAccessGetXXXXXX() function.
 */
TA_RetCode TA_StreamAccessGetBuffer( TA_StreamAccess *streamAccess,
                                     const char   **theBuffer,
                                     unsigned int *theBufferSize );

/* Bypass the specified number of byte. */
TA_RetCode TA_StreamAccessBypass( TA_StreamAccess *streamAccess, unsigned int nbByteToBypass );

/* Search for a string.
 * On success, the stream will be positioned AFTER the requested string.
 * If the string does not exist, TA_END_OF_STREAM will be returned and the
 * TA_StreamAccess is NOT affected (in other word, on all failure, the position
 * in the source is preserved).
 */
TA_RetCode TA_StreamAccessSearch( TA_StreamAccess *source, const char *stringToFind );

/* Extract the next HTML table from the stream.
 * For each element of the table, the provided function is called.
 * All HTML tag are strip off from the element.
 *
 * The href is the first hyperlink within the cell. Additional href in the
 * same cell are ignored.
 *
 * If a table is embedded within a table, the whole elements are
 * returned as if they were belonging to the same global table.
 */
typedef TA_RetCode (*TA_HTMLTableFuncPtr)( TA_Libc *libHandle, 
                                           unsigned int line,
                                           unsigned int column,
                                           const char *data,
                                           const char *href,
                                           void *opaqueData);

TA_RetCode TA_StreamAccessGetHTMLTable( TA_StreamAccess *streamAccess,
                                        unsigned int maxElementSize,
                                        TA_HTMLTableFuncPtr funcPtr,
                                        void *opaqueData );

/* Skip the next HTML Table.
 * (the streamAccess will point after the </table> tag)
 */
TA_RetCode TA_StreamAccessSkipHTMLTable( TA_StreamAccess *streamAccess );

/* Use to get the total size of byte/bit in the stream. */
unsigned int TA_StreamSizeInBit ( const TA_Stream *stream );
unsigned int TA_StreamSizeInByte( const TA_Stream *stream );

/* Use to write a stream to a file. */
TA_RetCode TA_StreamToFile( TA_Stream *stream, FILE *out );

/* Use to append a copy of a stream to another one.
 * The 'src' stream is copied at the end of the 'dest' stream.
 * The caller still have to properly free the 'dest' and 'src' TA_Stream.
 *
 * If the 'src' stream is not needed anymore by the caller, consider to use
 * instead the TA_StreamMerge function.
 */
TA_RetCode TA_StreamAppendCopy( TA_Stream *dest, const TA_Stream *src );

/* Take two streams and merge these into one stream.
 * The 'secondStream' will be concatenated to the firstStream.
 * On TA_SUCCESS, the caller must NEVER free the 'secondStream' (the
 * data will be freed when the firstStream will be freed).
 *
 * The TA_StreamMerge is a LOT more speed efficient than the
 * TA_StreamAppendCopy.
 */
TA_RetCode TA_StreamMerge( TA_Stream *firstStream, TA_Stream *secondStream );

/* A stream can contain compress or uncompress information.
 *
 * All the compression/decompression functionality is
 * encapsulated within this module.
 */

/* Compress a stream and append it to the 'stream'. */
TA_RetCode TA_StreamCompress( TA_Stream *stream, const TA_Stream *streamToCompress );

/* Un-compress a stream
 * On success, a new un-compress stream is returned.
 *
 * Following this call the 'streamToDecompress' access will be set
 * on the next bit to read from the stream. In other words, only the
 * bits needed by the compression algorithm are consumed.
 *
 * It is VERY IMPORTANT that the TA_StreamAccess is really pointing to a
 * position where the stream is compressed.
 */
TA_RetCode TA_StreamDecompress( TA_StreamAccess *streamToDecompress,
                                TA_Stream **newAllocatedStream );

/* Append CRC-32 information to the stream to allow to safely
 * rebuild it by a remote entity (or later re-read from a file).
 *
 * Following a call to TA_StreamEncapsulate, the stream is expected to
 * be written to a file or sent across a network. On reading/receiving
 * the data, a call to TA_StreamDecapsulate must be performed.
 *
 * On success, these two functions GUARANTEE the integrity of the
 * TA_Stream. In other word, TA_StreamDecapsulate will succeed ONLY
 * if the TA_Stream is successfully rebuild and IDENTICAL to the original.
 *
 * The caller can provide a timestamp and an identifier to be embedded
 * within the header of the encapsulated stream. If the caller does not
 * care, just pass dummy value for these. The exactitude of this timestamp
 * and integer is also guaranteed when TA_StreamDecapsulate succeed.
 *
 * Warning: Notice that TA_StreamEncapsulate may change the pointer on
 *          the 'stream'. Do not assume that the stream pointer
 *          did not changed. Free that stream as usual once you are done.
 */

TA_RetCode TA_StreamEncapsulate( TA_Stream    **stream, /* See Warning. */
                                 TA_Timestamp  *userTimestamp,
                                 TA_Integer     userIdentifier );

TA_RetCode TA_StreamDecapsulate( TA_Stream    *stream,
                                 TA_Timestamp *userTimestamp,
                                 TA_Integer   *userIdentifier ); 

/* Simply return the number of time a certain character exist within
 * the stream.
 * Check only on 8 bits boundary.
 *
 * Example: TA_StreamCountChar( stream, '\n' ) will return the
 *          number of lines in the stream.
 *     
 */
unsigned int TA_StreamCountChar( const TA_Stream *stream, char toCount );

#ifdef TA_DEBUG
TA_RetCode TA_StreamPrint( TA_Stream *stream );
#endif

#endif
