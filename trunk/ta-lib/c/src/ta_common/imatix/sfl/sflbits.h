/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflbits.h
    Title:      Large bitstring manipulation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/14  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides operations to manipulate large bitstrings.  The
                bitstrings are compressed.  Intended for bit-based index
                techniques, where bitstrings can be millions of bits long.
                These functions are still in development; this is an early
                version that provides basic functionality.  Simple tests
                on large bitmaps with random filling show a cost of about
                3 bytes per bit, after compression.  This includes all the
                indexing information.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLBITS_INCLUDED               /*  Allow multiple inclusions        */
#define SFLBITS_INCLUDED

/*  Definitions                                                              */

#define BIT_DATASIZE    500             /*  Size of block data part          */
#define BIT_INDEXSIZE   BIT_DATASIZE/2  /*  Size of block index part         */
#define BIT_SECTSIZE    8192            /*  Size of one bitstring section    */
#define BIT_MAXBLOCKS   1024            /*  Max. size of bitstring           */
#define BIT_MAXBITS     16384000L       /*  Max. possible bit number         */

typedef struct {                        /*  Bitstring block                  */
    union {
        byte  data  [BIT_DATASIZE];     /*    Data record part               */
        dbyte index [BIT_INDEXSIZE];    /*    Index record part              */
    } block;
    dbyte left,                         /*    Pointer to left (index only)   */
          right;                        /*    Pointer to right (data too)    */
    int   size;                         /*    Size of data part              */
} BITBLOCK;

typedef struct {                        /*  Bitstring object                 */
    BITBLOCK
       *block [BIT_MAXBLOCKS];          /*  Table of allocated blocks        */
    int
        block_count;                    /*  How many allocated blocks        */
    dbyte
        free_list;                      /*  Block free list                  */
} BITS;

extern long bits_free_count;            /*  We count free() and malloc()     */
extern long bits_alloc_count;

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

int   bits_init    (void);
int   bits_term    (void);
BITS *bits_create  (void);
void  bits_destroy (BITS *bits);
int   bits_set     (BITS *bits, long bit);
int   bits_clear   (BITS *bits, long bit);
int   bits_test    (const BITS *bits, long bit);
int   bits_fput    (FILE *file, const BITS *bits);
BITS *bits_fget    (FILE *file);

#ifdef __cplusplus
}
#endif

#endif
