#ifndef TA_CODEC_H
#define TA_CODEC_H

#ifndef TA_FUNC_H
   #include "ta_func.h"
#endif

#ifndef TA_STREAM_H
   #include "ta_stream.h"
#endif

/* This module provides the functions for coding the most efficiently
 * possible a price history into a stream of byte.
 *
 * The goal of this ta_codec module is to encapsulate completely the
 * coding/decoding algorithm.
 */


/* On success, will return a TA_Stream. It becomes the responsibility
 * of the caller to 'free' the TA_Stream.
 *
 * Will return NULL on failure.
 */
const TA_Stream *TA_encode_history( const TA_History *inHistory );

/* On success, will return a TA_History. It becomes the responsibility
 * of the caller to 'free' the TA_History.
 *
 * Will return NULL on failure.
 */
const TA_History *TA_decode_history( const TA_Stream *inStream );

#endif

