/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcomp.c
    Title:      Compression functions
    Package:    Standard Function Library (SFL)

    Written:    1991/05/20  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflcomp.h"                    /*  Function prototypes              */


/*  Constants                                                                */

#define FLAG_COPY             0x80      /*  For LZ/RLE compression           */
#define FLAG_COMPRESS         0x40


/*  Local function prototypes                                                */

static byte get_match (const byte *source, word ptr, word source_size,
                       short *hash, word *size, short *pos);


/*  ---------------------------------------------------------------------[<]-
    Function: compress_block

    Synopsis: Takes up to 64Kb of uncompressed data in Source, compresses
    it using a fast LZ/RLE algorithm and places the result in Dest.  The
    compression technique is comparable to that used by Zip and such tools,
    but less agressive.  It is, however, fast enough to use in realtime.
    Returns the size of the compressed data.  To decompress the data, use
    the expand_block() function.
    ---------------------------------------------------------------------[>]-*/

word
compress_block (
    const byte *src,
    byte *dst,
    word src_size)
{
    static short
          Hash [4096];
    short SymbolAddress;
    word  Key;
    word  Size;
    byte  Bit = 0;
    word  Command = 0;
    word  src_index = 0;
    word  dst_size = 3;
    word  HeaderIndex = 1;

    dst [0] = FLAG_COMPRESS;
    for (Key = 0; Key < 4096; Key++)
        Hash [Key] = -1;

    while ((src_index < src_size) && (dst_size <= src_size))
      {
        if (Bit > 15)
          {
            dst [HeaderIndex]     = (byte) ((Command >> 8) & 0x00ff);
            dst [HeaderIndex + 1] = (byte) ( Command       & 0x00ff);
            HeaderIndex = dst_size;
            dst_size += 2;
            Bit = 0;
          }
        for (Size = 1;; Size++)
            if ((word) (src_index + Size) >= src_size
            || (src [src_index] != src [src_index + Size])
            || (Size >= 0x0fff))
                break;

        if (Size >= 16)
          {
            dst [dst_size++] = 0;
            dst [dst_size++] = (byte) (((word) (Size - 16) >> 8) & 0x00ff);
            dst [dst_size++] = (byte) ((Size - 16) & 0x00ff);
            dst [dst_size++] = src [src_index];
            src_index += Size;
            Command = (Command << 1) + 1;
          }
        else
        if (get_match (src, src_index, src_size,
                       Hash, &Size, &SymbolAddress) != 0)
          {
            Key = ((src_index - SymbolAddress) << 4) + (Size - 3);
            dst [dst_size++] = (byte) ((Key >> 8) & 0x00ff);
            dst [dst_size++] = (byte) (Key & 0x00ff);
            src_index += Size;
            Command = (Command << 1) + 1;
          }
        else
          {
            dst [dst_size++] = src [src_index++];
            Command = (Command << 1);
          }
        Bit++;
      }
    Command <<= (16 - Bit);
    dst [HeaderIndex]     = (byte) ((Command >> 8) & 0x00ff);
    dst [HeaderIndex + 1] = (byte) ( Command       & 0x00ff);

     if (dst_size > src_size)
      {
         for (dst_size = 0; dst_size < src_size; dst_size++)
             dst [dst_size + 1] = src [dst_size];
         dst [0] = FLAG_COPY;
         return (src_size + 1);
       }
     return (dst_size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: expand_block

    Synopsis: Expands a block of data previously compressed using the
    compress_block() function.  The compressed block is passed in src;
    the expanded result in dst.  dst must be large enough to accomodate
    the largest possible decompressed block.  Returns the size of the
    uncompressed data.
    ---------------------------------------------------------------------[>]-*/

word
expand_block (
    const byte *src,
    byte *dst,
    word src_size)
{
    word SymbolAddress;
    word ChunkSize;
    word Counter;
    word Command = 0;
    word src_index = 1;
    word dst_size = 0;
    byte Bit = 0;

    if (src [0] == FLAG_COPY)
      {
        for (dst_size = 1; dst_size < src_size; dst_size++)
            dst [dst_size - 1] = src [dst_size];
        return (src_size - 1);
      }
    while (src_index < src_size)
      {
        if (Bit == 0)
          {
            Command  = src [src_index++] << 8;
            Command += src [src_index++];
            Bit = 16;
          }
        if (Command & 0x8000)
          {
            SymbolAddress =  (word) (src [src_index++] << 4);
            SymbolAddress += (word) (src [src_index] >> 4);
            if (SymbolAddress)
              {
                ChunkSize = (word) (src [src_index++] & 0x0f) + 3;
                SymbolAddress = dst_size - SymbolAddress;
                for (Counter = 0; Counter < ChunkSize; Counter++)
                    dst [dst_size++] = dst [SymbolAddress++];
              }
            else
              {
                ChunkSize  = (word) (src [src_index++] << 8);
                ChunkSize += (word) (src [src_index++] + 16);
                for (Counter = 0; Counter < ChunkSize; Counter++)
                    dst [dst_size++] = src [src_index];
                src_index++;
              }
          }
        else
            dst [dst_size++] = src [src_index++];

        Command <<= 1;
        Bit--;
      }
    return (dst_size);
}


/*  -------------------------------------------------------------------------
 *  get_match -- local
 *
 *  Finds a match for the bytes at the specified offset.
 */

static byte get_match (const byte *source, word ptr, word source_size,
                       short *hash, word *size, short *pos)
{
    word hash_value;

    hash_value = (word) (40543L * (long) ((((source [ptr]      << 4) ^
                                             source [ptr + 1]) << 4) ^
                                             source [ptr + 2]) >> 4) & 0xfff;
    *pos = hash [hash_value];
    hash [hash_value] = ptr;
    if ((word) ((*pos != -1) && ((ptr - *pos)) < 4096U))
      {
        for (*size = 0;; (*size)++)
            if ((*size >= 18)
            || ((word) (ptr + *size) >= source_size)
            || (source [ptr + *size] != source [*pos + *size]))
                break;

        return (byte) (*size >= 3);
      }
    return (FALSE);
}


/*  ---------------------------------------------------------------------[<]-
    Function: compress_rle

    Synopsis: Takes a block of uncompressed data in src, compresses
    it using a RLE algorithm and places the result in dst.  To decompress
    the data, use the expand_rle () function.  Returns the size of the
    compressed data.  The dst buffer should be 10% larger than the src
    buffer.  The src buffer must be at least src_size + 1 bytes long.  It
    may be modified.  The compressed data contains these strings:
    <Table>
    [01-7F][data...]     String of uncompressed data, 1 to 127 bytes.
    [83-FF][byte]        Run of 3 to 127 identical bytes.
    [80][len][byte]      Run of 128 to 255 identical bytes.
    [81][lo][hi][byte]   Run of 256 to 2^16 identical bytes.
    [82][len]            Run of 3 to 255 spaces.
    [00][len]            Run of 3 to 255 binary zeroes.
    </Table>
    ---------------------------------------------------------------------[>]-*/

word
compress_rle (
    byte *src,
    byte *dst,
    word src_size)
{
    word
        dst_size,                       /*  Size of compressed data          */
        src_scan,                       /*  Scan through source data         */
        run_end,                        /*  Points to end of run of bytes    */
        length = 0;                     /*  Size of the run or string        */
    byte
        cur_byte,                       /*  Next byte to process             */
        *header;                        /*  Header of unpacked string        */
    Bool
        have_run;                       /*  TRUE when we have a run          */

    src_scan = 0;                       /*  Start at beginning of source     */
    dst_size = 0;                       /*  No output yet                    */
    header   = NULL;                    /*  No open unpacked string          */
    while (src_scan < src_size)
      {
        cur_byte = src [src_scan++];
        have_run = FALSE;               /*  Unless we find a run             */

        /*  Three identical bytes signals the start of a run                 */
        if (cur_byte == src [src_scan]
        &&  cur_byte == src [src_scan + 1]
        && (src_scan + 1 < src_size))
          {
            /*  Stick-in a sentinel character to ensure that the run ends    */
            src [src_size] = !cur_byte;
            run_end = src_scan;         /*  src_scan <= src_size             */
            while (src [run_end] == cur_byte)
                run_end++;

            have_run = TRUE;
            if (header)                 /*  If we have a previous unpacked   */
              {                         /*    string, close it               */
                *header = (byte) length;
                header  = NULL;
              }
            length = run_end - src_scan + 1;
            src_scan = run_end;
          }
        if (have_run)
          {
            /*  We compress short runs of spaces and nulls separately        */
            if (length < 256 && cur_byte == 0)
              {
                dst [dst_size++] = 0x00;
                dst [dst_size++] = (byte) length;
              }
            else
            if (length < 256 && cur_byte == ' ')
              {
                dst [dst_size++] = 0x82;
                dst [dst_size++] = (byte) length;
              }
            else
            if (length < 128)
              {
                dst [dst_size++] = (byte) length | 0x80;
                dst [dst_size++] = cur_byte;
              }
            else
            if (length < 256)           /*  Short run 128-255 bytes          */
              {
                dst [dst_size++] = 0x80;
                dst [dst_size++] = (byte) length;
                dst [dst_size++] = cur_byte;
              }
            else                        /*  Long run 256-2^16 bytes          */
              {
                dst [dst_size++] = 0x81;
                dst [dst_size++] = (byte) (length & 0xff);
                dst [dst_size++] = (byte) (length >> 8);
                dst [dst_size++] = cur_byte;
              }
          }
        else
          {
            if (!header)                /*  Start new unpacked string if     */
              {                         /*    necessary                      */
                header = &dst [dst_size++];
                length = 0;
              }
            dst [dst_size++] = cur_byte;
            if (++length == 127)        /*  Each string can be up to 127     */
              {                         /*    bytes long (high bit cleared)  */
                *header = (byte) length;
                header  = NULL;
              }
          }
      }
    if (header)                         /*  If we have a previous unpacked   */
      {                                 /*    string, close it               */
        *header = (byte) length;
        header  = NULL;
      }
    return (dst_size);                  /*  Return compressed data size      */
}


/*  ---------------------------------------------------------------------[<]-
    Function: expand_rle

    Synopsis: Expands a block of data previously compressed using the
    compress_rle() function.  The compressed block is passed in src; the
    expanded result in dst.  Dst must be large enough to accomodate the
    largest possible decompressed block.  Returns the size of the expanded
    data.
    ---------------------------------------------------------------------[>]-*/

word
expand_rle (
    const byte *src,
    byte *dst,
    word src_size)
{
    word
        dst_size,                       /*  Size of expanded data            */
        src_scan,                       /*  Scan through source data         */
        length;                         /*  Size of the run or string        */
    byte
        cur_byte;                       /*  Next byte to process             */

    src_scan = 0;
    dst_size = 0;
    while (src_scan < src_size)
      {
        cur_byte = src [src_scan++];

        /*  1 to 127 is uncompressed string of 1 to 127 bytes                */
        if (cur_byte > 0 && cur_byte < 128)
          {
            length = (word) cur_byte;
            memcpy (dst + dst_size, src + src_scan, length);
            src_scan += length;
            dst_size += length;
          }
        else                            /*  Run of 3 or more bytes           */
          {
            switch (cur_byte)
              {
                case 0x00:              /*  Run of 3-255 zeroes              */
                    length   = src [src_scan++];
                    cur_byte = 0;
                    break;
                case 0x82:              /*  Run of 3-255 spaces              */
                    length   = src [src_scan++];
                    cur_byte = ' ';
                    break;
                case 0x80:              /*  Short run 128-255 bytes          */
                    length   = src [src_scan++];
                    cur_byte = src [src_scan++];
                    break;
                case 0x81:              /*  Long run 256-2^16 bytes          */
                    length   = src [src_scan++];
                    length  += src [src_scan++] << 8;
                    cur_byte = src [src_scan++];
                    break;
                default:                /*  Run of 3 to 127 bytes            */
                    length = cur_byte & 127;
                    cur_byte = src [src_scan++];
              }
            memset (dst + dst_size, cur_byte, length);
            dst_size += length;
          }
      }
    return (dst_size);                  /*  Return expanded data size        */
}


/*  ---------------------------------------------------------------------[<]-
    Function: compress_nulls

    Synopsis: Similar to compress_rle(), but optimised towards compression
    of binary zeroes.  Use this when you are certain that the sparse areas
    are set to binary zeroes.  You must use expand_nulls () to decompress
    a block compressed with this function.  Returns the size of the
    compressed data.  The dst buffer should be 10% larger than the src
    buffer.  The src buffer must be at least src_size + 1 bytes long.  It
    may be modified.  The compressed data contains these strings:
    <Table>
    [01-7F][data...]        String of uncompressed data, 1 to 127 bytes.
    [82-FF]                 Run of 2 to 127 binary zeroes.
    [81][80-FF]             Run of 128 to 255 binary zeroes.
    [80][lo][hi]            Run of 256 to 2^16 binary zeroes.
    [00][len][byte]         Run of 4 to 255 identical bytes.
    [00][00][lo][hi][byte]  Run of 256 to 2^16 identical bytes.
    </Table>
    ---------------------------------------------------------------------[>]-*/

word
compress_nulls (
    byte *src,
    byte *dst,
    word src_size)
{
    word
        dst_size,                       /*  Size of compressed data          */
        src_scan,                       /*  Scan through source data         */
        run_end,                        /*  Points to end of run of bytes    */
        length = 0;                     /*  Size of the run or string        */
    byte
        cur_byte,                       /*  Next byte to process             */
        *header;                        /*  Header of unpacked string        */
    Bool
        have_run;                       /*  TRUE when we have a run          */

    src_scan = 0;                       /*  Start at beginning of source     */
    dst_size = 0;                       /*  No output yet                    */
    header   = NULL;                    /*  No open unpacked string          */
    while (src_scan < src_size)
      {
        cur_byte = src [src_scan++];
        have_run = FALSE;               /*  Unless we find a run             */

        /*  Two identical bytes may signal the start of a run                */
        if (cur_byte == src [src_scan]
        &&  src_scan < src_size)
          {
            /*  Stick-in a sentinel character to ensure that the run ends    */
            src [src_size] = !cur_byte;
            run_end = src_scan;         /*  src_scan <= src_size             */
            while (src [run_end] == cur_byte)
                run_end++;

            /*  A run is 4+ identical bytes or 2+ nulls                      */
            if ((run_end - src_scan > 2) || cur_byte == 0)
              {
                have_run = TRUE;
                if (header)             /*  If we have a previous unpacked   */
                  {                     /*    string, close it               */
                    *header = (byte) length;
                    header  = NULL;
                  }
                length = run_end - src_scan + 1;
                src_scan = run_end;
              }
          }
        if (have_run)
          {
            if (cur_byte == 0)
              {
                if (length < 128)       /*  2-127 binary zeroes              */
                    dst [dst_size++] = (byte) (length | 0x80);
                else
                if (length < 256)       /*  128-256 binary zeroes            */
                  {
                    dst [dst_size++] = 0x81;
                    dst [dst_size++] = (byte) length;
                  }
                else                    /*  256-2^15 binary zeroes           */
                  {
                    dst [dst_size++] = 0x80;
                    dst [dst_size++] = (byte) (length & 0xff);
                    dst [dst_size++] = (byte) (length >> 8);
                  }
              }
            else
            if (length < 256)           /*  Short run 4-255 bytes            */
              {
                dst [dst_size++] = 0x00;
                dst [dst_size++] = (byte) length;
                dst [dst_size++] = cur_byte;
              }
            else                        /*  Long run 256-2^16 bytes          */
              {
                dst [dst_size++] = 0x00;
                dst [dst_size++] = 0x00;
                dst [dst_size++] = (byte) (length & 0xff);
                dst [dst_size++] = (byte) (length >> 8);
                dst [dst_size++] = cur_byte;
              }
          }
        else
          {
            if (!header)                /*  Start new unpacked string if     */
              {                         /*    necessary                      */
                header = &dst [dst_size++];
                length = 0;
              }
            dst [dst_size++] = cur_byte;
            if (++length == 127)        /*  Each string can be up to 127     */
              {                         /*    bytes long (high bit cleared)  */
                *header = (byte) length;
                header  = NULL;
              }
          }
      }
    if (header)                         /*  If we have a previous unpacked   */
      {                                 /*    string, close it               */
        *header = (byte) length;
        header  = NULL;
      }
    return (dst_size);                  /*  Return compressed data size      */
}


/*  ---------------------------------------------------------------------[<]-
    Function: expand_nulls

    Synopsis: Expands a block of data previously compressed using the
    compress_nulls() function. The compressed block is passed in src; the
    expanded result in dst.  Dst must be large enough to accomodate the
    largest possible decompressed block.  Returns the size of the expanded
    data.
    ---------------------------------------------------------------------[>]-*/

word
expand_nulls (
    const byte *src,
    byte *dst,
    word src_size)
{
    word
        dst_size,                       /*  Size of expanded data            */
        src_scan,                       /*  Scan through source data         */
        length;                         /*  Size of the run or string        */
    byte
        cur_byte;                       /*  Next byte to process             */

    src_scan = 0;
    dst_size = 0;
    while (src_scan < src_size)
      {
        cur_byte = src [src_scan++];

        /*  1 to 127 is uncompressed string of 1 to 127 bytes                */
        if (cur_byte > 0 && cur_byte < 128)
          {
            length = (word) cur_byte;
            memcpy (dst + dst_size, src + src_scan, length);
            src_scan += length;
            dst_size += length;
          }
        else                            /*  Run of 2 or more bytes           */
          {
            switch (cur_byte)
              {
                case 0x00:              /*  Run of non-zero bytes            */
                    length = src [src_scan++];
                    if (length == 0)    /*  Stored as double-byte            */
                      {
                        length   = src [src_scan++];
                        length  += src [src_scan++] << 8;
                      }
                    cur_byte = src [src_scan++];
                    break;
                case 0x80:              /*  256-2^16 zeroes                  */
                    length   = src [src_scan++];
                    length  += src [src_scan++] << 8;
                    cur_byte = 0;
                    break;
                case 0x81:              /*  128 to 255 zeroes                */
                    length   = src [src_scan++];
                    cur_byte = 0;
                    break;
                default:                /*  2 to 127 zeroes                  */
                    length   = cur_byte & 127;
                    cur_byte = 0;
              }
            memset (dst + dst_size, cur_byte, length);
            dst_size += length;
          }
      }
    return (dst_size);                  /*  Return expanded data size        */
}


/*  ---------------------------------------------------------------------[<]-
    Function: compress_bits

    Synopsis: Similar to compress_rle(), but optimised towards compression
    of sparse bitmaps.  Use this when you are playing with large, sparse
    bitmaps.  You must use expand_bits () to decompress a block compressed
    with this function.  Returns the size of the compressed data.  The dst
    buffer should be 10% larger than the src buffer for worst cases.  The
    src buffer must be at least src_size + 1 bytes long.  It may be
    modified.  The compressed data contains these strings:
    <Table>
    [00-07]                 Single byte containing a bit in position 0 to 7.
    [08-7F][data...]        String of uncompressed data, 1 to 120 bytes.
    [82-FF]                 Run of 1 to 126 binary zeroes.
    [81][00-FD]             Run of 127 to 380 binary zeroes.
    [81][FE][len][byte]     Run of 4 to 255 identical bytes.
    [81][FF][lo][hi][byte]  Run of 256 to 2^16 identical bytes.
    [80][lo][hi]            Run of 381 to 2^16 binary zeroes.
    </Table>
    ---------------------------------------------------------------------[>]-*/

word
compress_bits (
    byte *src,
    byte *dst,
    word src_size)
{
    word
        dst_size,                       /*  Size of compressed data          */
        src_scan,                       /*  Scan through source data         */
        run_end,                        /*  Points to end of run of bytes    */
        length = 0;                     /*  Size of the run or string        */
    byte
        cur_byte,                       /*  Next byte to process             */
        *header;                        /*  Header of unpacked string        */
    static byte
        single_bits [256];              /*  Bytes with one bit set           */
    static Bool
        initialised = FALSE;            /*  First time flag                  */

    /*  The single_bits table provides a fast lookup for bytes with          */
    /*  one bit set.  The 'interesting' bytes are non-zero in the table      */
    /*  where their value is the output code value (0-7) + 1.                */
    if (!initialised)                   /*  First time?  Initialise          */
      {
        memset (single_bits, 0, 256);
        single_bits [1]   = 1;
        single_bits [2]   = 2;
        single_bits [4]   = 3;
        single_bits [8]   = 4;
        single_bits [16]  = 5;
        single_bits [32]  = 6;
        single_bits [64]  = 7;
        single_bits [128] = 8;
      }

    src_scan = 0;                       /*  Start at beginning of source     */
    dst_size = 0;                       /*  No output yet                    */
    header   = NULL;                    /*  No open unpacked string          */
    while (src_scan < src_size)
      {
        cur_byte = src [src_scan++];

        /*- Look for 1 or more binary zeroes, and compress into a run -------*/

        if (cur_byte == 0)
          {
            src [src_size] = 0xff;      /*  Stop with a sentinel             */
            run_end = src_scan;         /*  src_scan <= src_size             */
            while (src [run_end] == 0)
                run_end++;

            if (header)                 /*  If we have a previous unpacked   */
              {                         /*    string, close it               */
                *header = (byte) length + 7;
                header  = NULL;
              }
            length = run_end - src_scan + 1;
            src_scan = run_end;
            if (length < 127)           /*  1-126 binary zeroes              */
                dst [dst_size++] = (byte) (++length | 0x80);
            else
            if (length < 381)           /*  127-380 binary zeroes            */
              {
                dst [dst_size++] = 0x81;
                dst [dst_size++] = (byte) length - 127;
              }
            else                        /*  381-2^16 binary zeroes           */
              {
                dst [dst_size++] = 0x80;
                dst [dst_size++] = (byte) (length & 0xff);
                dst [dst_size++] = (byte) (length >> 8);
              }
          }
        else

        /*- Next, look for bytes with 1 bit set; we encode these as 1 byte --*/

        if (single_bits [cur_byte])     /*  Single bit value?                */
          {
            dst [dst_size++] = single_bits [cur_byte] - 1;
            if (header)                 /*  If we have a previous unpacked   */
              {                         /*    string, close it               */
                *header = (byte) length + 7;
                header  = NULL;
              }
          }
        else

        /*- Next, look for a run of 4 or more identical (non-zero) bytes ----*/

        if (cur_byte == src [src_scan]
        &&  cur_byte == src [src_scan + 1]
        &&  cur_byte == src [src_scan + 2]
        && (src_scan < src_size - 2))
          {
            src [src_size] = !cur_byte; /*  Stick in a sentinel byte         */
            run_end = src_scan;         /*  src_scan <= src_size             */
            while (src [run_end] == cur_byte)
                run_end++;

            if (header)                 /*  If we have a previous unpacked   */
              {                         /*    string, close it               */
                *header = (byte) length + 7;
                header  = NULL;
              }
            length = run_end - src_scan + 1;
            src_scan = run_end;

            if (length < 256)           /*  Short run 4-255 bytes            */
              {
                dst [dst_size++] = 0x81;
                dst [dst_size++] = 0xFE;
                dst [dst_size++] = (byte) length;
                dst [dst_size++] = cur_byte;
              }
            else                        /*  Long run 256-2^16 bytes          */
              {
                dst [dst_size++] = 0x81;
                dst [dst_size++] = 0xFF;
                dst [dst_size++] = (byte) (length & 0xff);
                dst [dst_size++] = (byte) (length >> 8);
                dst [dst_size++] = cur_byte;
              }
          }
        else

        /*- Lastly, compress unpackable strings into chunks of 120 bytes ----*/

          {
            if (!header)                /*  Start new unpacked string if     */
              {                         /*    necessary                      */
                header = &dst [dst_size++];
                length = 0;
              }
            dst [dst_size++] = cur_byte;
            if (++length == 120)        /*  Each string can be up to 120     */
              {                         /*    bytes long (high bit cleared)  */
                *header = (byte) length + 7;
                header  = NULL;
              }
          }
      }
    if (header)                         /*  If we have a previous unpacked   */
      {                                 /*    string, close it               */
        *header = (byte) length + 7;
        header  = NULL;
      }
    return (dst_size);                  /*  Return compressed data size      */
}


/*  ---------------------------------------------------------------------[<]-
    Function: expand_bits

    Synopsis: Expands a block of data previously compressed using the
    compress_bits() function. The compressed block is passed in src; the
    expanded result in dst.  Dst must be large enough to accomodate the
    largest possible decompressed block.  Returns the size of the expanded
    data.
    ---------------------------------------------------------------------[>]-*/

word
expand_bits (
    const byte *src,
    byte *dst,
    word src_size)
{
    word
        dst_size,                       /*  Size of expanded data            */
        src_scan,                       /*  Scan through source data         */
        length;                         /*  Size of the run or string        */
    byte
        cur_byte;                       /*  Next byte to process             */

    src_scan = 0;
    dst_size = 0;
    while (src_scan < src_size)
      {
        cur_byte = src [src_scan++];

        if (cur_byte < 8)               /*  Single bit in position 0 to 7    */
            dst [dst_size++] = 1 << cur_byte;
        else
        if (cur_byte < 128)             /*  String of 1 to 120 bytes         */
          {
            length = (word) cur_byte - 7;
            memcpy (dst + dst_size, src + src_scan, length);
            src_scan += length;
            dst_size += length;
          }
        else                            /*  Run of 1 or more bytes           */
          {
            switch (cur_byte)
              {
                case 0x80:              /*  381-2^16 binary zeroes           */
                    length   = src [src_scan++];
                    length  += src [src_scan++] << 8;
                    cur_byte = 0;
                    break;
                case 0x81:
                    length = src [src_scan++];
                    if (length == 0xFE) /*  4-255 non-zero bytes             */
                      {
                        length   = src [src_scan++];
                        cur_byte = src [src_scan++];
                      }
                    else
                    if (length == 0xFF) /*  Run of 256-2^15 non-zero bytes   */
                      {
                        length   = src [src_scan++];
                        length  += src [src_scan++] << 8;
                        cur_byte = src [src_scan++];
                      }
                    else
                      {
                        length  += 127;
                        cur_byte = 0;   /*  127 to 380 zeroes                */
                      }
                    break;
                default:                /*  1 to 126 zeroes                  */
                    length   = (cur_byte - 1) & 127;
                    cur_byte = 0;
              }
            memset (dst + dst_size, cur_byte, length);
            dst_size += length;
          }
      }
    return (dst_size);                  /*  Return expanded data size        */
}
