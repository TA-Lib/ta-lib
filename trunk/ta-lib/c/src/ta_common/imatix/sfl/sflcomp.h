/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcomp.h
    Title:      Compression functions
    Package:    Standard Function Library (SFL)

    Written:    1991/05/20  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Various compression/decompression functions.  The LZ-type
                algorith (LZRW1/KH) was originally written by Kurt Haenen
                <ghgaea8@blekul11> and made portable by P. Hintjens. This
                is a reasonable LZ/RLE algorithm, very fast, but about 30%
                less efficient than a ZIP-type algorithm in terms of space.
                The RLE algorithms are better suited to compressing sparse
                data.  The nulls variant is specifically tuned to data that
                consists mostly of binary zeroes.  The bits variant is
                tuned for compressing sparse bitmaps.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLCOMP_INCLUDED               /*  Allow multiple inclusions        */
#define SFLCOMP_INCLUDED

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

word compress_block (const byte *source, byte *dest, word source_size);
word expand_block   (const byte *source, byte *dest, word source_size);
word compress_rle   (      byte *source, byte *dest, word source_size);
word expand_rle     (const byte *source, byte *dest, word source_size);
word compress_nulls (      byte *source, byte *dest, word source_size);
word expand_nulls   (const byte *source, byte *dest, word source_size);
word compress_bits  (      byte *source, byte *dest, word source_size);
word expand_bits    (const byte *source, byte *dest, word source_size);

#ifdef __cplusplus
}
#endif

#endif
