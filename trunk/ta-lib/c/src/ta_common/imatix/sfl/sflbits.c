/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflbits.c
    Title:      Large bitstring manipulation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/14  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflcomp.h"                    /*  Compression functions            */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory allocation functions      */
#include "sflbits.h"                    /*  Prototypes for functions         */


/*  Local function prototypes                                                */

static int locate_bit   (const BITS *bits, long bit, int *index_in_bits,
                         int *section_in_index, dbyte *bit_in_section);
static int get_section  (BITS *bits, int index, int section, byte *buffer,
                         Bool update);
static int put_section  (BITS *bits, int index, int section, byte *buffer);
static int alloc_block  (BITS *bits);


/*  Variables used in this source by various functions                       */

static byte
    section_data [BIT_SECTSIZE + 10],
    compressed   [(BIT_SECTSIZE * 11) / 10],
    *comp_zero = NULL,                  /*  What all zeroes looks like       */
    *comp_ones = NULL;                  /*  What all ones looks like         */
static int
    comp_ones_size,                     /*  Size of all zeroes, compressed   */
    comp_zero_size;                     /*  Size of all zeroes, compressed   */


/*  ---------------------------------------------------------------------[<]-
    Function: bits_init

    Synopsis: Initialises bitstring functions.  You must call this before
    using any other bitstring functions.  Returns 0 if okay, -1 if there
    was an error.
    ---------------------------------------------------------------------[>]-*/

int
bits_init (void)
{
    ASSERT (comp_zero == NULL);

    comp_zero = mem_alloc (BIT_SECTSIZE + 1);
    if (!comp_zero)
        return (-1);                    /*  Could not allocate new block     */

    memset (compressed, BIT_SECTSIZE, 0x00);
    comp_zero_size = compress_bits (compressed, comp_zero, BIT_SECTSIZE);
    comp_zero      = mem_realloc (comp_zero, comp_zero_size);

    comp_ones = mem_alloc (BIT_SECTSIZE + 1);
    if (!comp_ones)
      {
        mem_free (comp_ones);
        return (-1);                    /*  Could not allocate new block     */
      }
    memset (compressed, BIT_SECTSIZE, 0xFF);
    comp_ones_size = compress_bits (compressed, comp_ones, BIT_SECTSIZE);
    comp_ones      = mem_realloc (comp_ones, comp_ones_size);

    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_term

    Synopsis: Terminates bitstring functions.  You must call this when you
    are finished using the bitstring functions.  Returns 0 if okay, -1 if
    there was an error.
    ---------------------------------------------------------------------[>]-*/

int
bits_term (void)
{
    mem_free (comp_zero);
    mem_free (comp_ones);
    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_create

    Synopsis: Creates a new bitstring and initialises all bits to zero.
    Returns a BITS handle which you should use in all further references
    to the bitstring.
    ---------------------------------------------------------------------[>]-*/

BITS *
bits_create (void)
{
    BITS
        *bits;                          /*  Newly-created bitstring          */
    BITBLOCK
        *index;                         /*  Newly-created index block        */

    bits = mem_alloc (sizeof (BITS));
    if (bits)
      {
        memset (bits, 0, sizeof (BITS));
        index = mem_alloc (sizeof (BITBLOCK));
        if (index)
          {
            /*  Set all index fields to 0: bitstring is all zeroes           */
            memset (index, 0, sizeof (BITBLOCK));
            index-> left       = 0;
            index-> right      = 0;
            index-> size       = BIT_DATASIZE;
            bits-> block [0]   = index;
            bits-> block_count = 1;
            bits-> free_list   = 0;     /*  No blocks in free list           */
          }
        else
          {
            mem_free (bits);
            bits = NULL;
          }
      }
    return (bits);
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_destroy

    Synopsis: Releases all memory used by a bitstring and deletes the
    bitstring.  Do not refer to the bitstring after calling this function.
    ---------------------------------------------------------------------[>]-*/

void
bits_destroy (
    BITS *bits)
{
    int
        block_nbr;                      /*  Bitstring block number           */

    ASSERT (bits);

    /*  Free all blocks allocated to bitmap                                  */
    for (block_nbr = 0; block_nbr < bits-> block_count; block_nbr++)
        mem_free (bits-> block [block_nbr]);

    mem_free (bits);
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_set

    Synopsis: Sets the specified bit in the bitmap.  Returns ?
    ---------------------------------------------------------------------[>]-*/

int
bits_set (
    BITS *bits,
    long bit)
{
    int
        index,                          /*  Number of index block            */
        section;                        /*  Number of section in index       */
    dbyte
        bit_nbr;                        /*  Number of bit in section         */

    ASSERT (bits);

    locate_bit  (bits, bit, &index, &section, &bit_nbr);
    get_section (bits, index, section, section_data, TRUE);
    section_data [bit_nbr / 8] |= 1 << (bit_nbr % 8);
    put_section (bits, index, section, section_data);

    return 0;
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_clear

    Synopsis: Clears the specified bit in the bitmap.  Returns ?
    ---------------------------------------------------------------------[>]-*/

int
bits_clear (
    BITS *bits,
    long bit)
{
    int
        index,                          /*  Number of index block            */
        section;                        /*  Number of section in index       */
    dbyte
        bit_nbr;                        /*  Number of bit in section         */

    ASSERT (bits);

    locate_bit  (bits, bit, &index, &section, &bit_nbr);
    get_section (bits, index, section, section_data, TRUE);
    section_data [bit_nbr / 8] &= 255 - (1 << (bit_nbr % 8));
    put_section (bits, index, section, section_data);

    return 0;
}


/*  -------------------------------------------------------------------------
 *  locate_bit -- internal
 *
 *  For a particular bit in a bitstring, finds the index block that contains
 *  the bit, and returns the index block number, the section number within
 *  the index block, and the bit position within the section.  Returns TRUE
 *  if there was no error, FALSE if the specified bit lay outside the range
 *  currently defined by the bitstring.
 */

static int
locate_bit (
    const  BITS *bits,
    long   bit,
    int   *index_in_bits,
    int   *section_in_index,
    dbyte *bit_in_section)
{
    long
        index_base,
        relative_bit;

    ASSERT (bits);
    ASSERT (bit >= 0);
    ASSERT (bit < BIT_MAXBITS);

    *index_in_bits    = 0;              /*  Index block is always 0          */
    index_base        = *index_in_bits * (long) (BIT_SECTSIZE * BIT_INDEXSIZE);
    relative_bit      = bit - index_base;
    *section_in_index = (int)   (relative_bit / ((long) BIT_SECTSIZE * 8));
    *bit_in_section   = (dbyte) (relative_bit % ((long) BIT_SECTSIZE * 8));
    return (TRUE);
}


/*  -------------------------------------------------------------------------
 *  get_section -- internal
 *
 *  Expands the specified section into the working area specified.  If the
 *  update argument is TRUE, previously allocated blocks, if any, are put
 *  onto the free list.  Use this argument if you are changing the section
 *  and will recompress it using put_section().
 */

static int
get_section (
    BITS *bits,                         /*  Bitstring to work with           */
    int   index,                        /*  Index block number               */
    int   section,                      /*  Section within index             */
    byte *buffer,                       /*  Returned buffer                  */
    Bool  update)                       /*  If TRUE, frees section blocks    */
{
    BITBLOCK
        *index_block,                   /*  Points to index block            */
        *section_block;                 /*  Points to section block          */
    dbyte
        section_head,                   /*  Section block list head          */
        block_nbr,                      /*  Entry into block table           */
        block_next;                     /*  Next block in section list       */
    static byte
        comp [BIT_SECTSIZE + 1];        /*  Section blocks' data             */
    word
        comp_size,                      /*  Size of compressed data          */
        expand_size;                    /*  Size of expanded data            */

    ASSERT (bits);
    ASSERT (buffer);

    index_block  = bits-> block [index];
    section_head = index_block-> block.index [section];
    if (section_head == 0x0000)         /*  All 0's                          */
        memset (buffer, 0x00, BIT_SECTSIZE);
    else
    if (section_head == 0xFFFF)         /*  All 1's                          */
        memset (buffer, 0xFF, BIT_SECTSIZE);
    else
      {
        block_nbr = section_head;
        comp_size = 0;                  /*  Get compressed block             */
        while (block_nbr)               /*    from 1 or more sections        */
          {
            section_block = bits-> block [block_nbr];
            ASSERT (comp_size < BIT_SECTSIZE);
            memcpy (comp + comp_size, section_block-> block.data,
                                      section_block-> size);
            comp_size += section_block-> size;
            block_next = section_block-> right;
            if (update)
              {                         /*  Move block to free list          */
                section_block-> right = bits-> free_list;
                section_block-> size  = 0;
                bits-> free_list = block_nbr;
              }
            block_nbr = block_next;
          }
        if (update)                     /*  Wipe section block list          */
            index_block-> block.index [section] = 0;

        expand_size = expand_bits (comp, buffer, comp_size);
        ASSERT (expand_size == BIT_SECTSIZE);
      }
    return 0;
}


/*  -------------------------------------------------------------------------
 *  put_section -- internal
 *
 *  Compresses the specified buffer.  This results in zero or more blocks,
 *  stored in the bitstring at the specified index and section.  If the
 *  buffer is all zeroes, the section list head has the value zero.  If the
 *  bitstring is all ones, the list head has the value 0xFFFF.  Takes blocks
 *  off the free list if possible, or from memory if necessary.  Returns 0
 *  if the operation was successful, -1 if there was an error (usually lack
 *  of memory for new blocks).
 */

static int
put_section (
    BITS *bits,                         /*  Bitstring to work with           */
    int   index,                        /*  Index block number               */
    int   section,                      /*  Section within index             */
    byte *buffer)                       /*  Buffer to compress               */
{
    BITBLOCK
        *index_block,                   /*  Points to index block            */
        *section_block,                 /*  Points to section block          */
        *section_prev;                  /*  Points to section block          */
    dbyte
        block_nbr;                      /*  Entry into block table           */
    int
        comp_size,                      /*  Size of compressed data          */
        copy_from;                      /*  Index into compressed data       */

    ASSERT (bits);
    ASSERT (buffer);

    /*  Compress the section and get the resulting size                      */
    index_block = bits-> block [index];
    comp_size   = compress_bits (buffer, compressed, BIT_SECTSIZE);
    ASSERT (comp_size <= BIT_SECTSIZE + 1);

    if (comp_size == comp_zero_size
    &&  memcmp (compressed, comp_zero, comp_zero_size) == 0)
        index_block-> block.index [section] = 0x0000;
    else
    if (comp_size == comp_ones_size
    &&  memcmp (compressed, comp_ones, comp_ones_size) == 0)
        index_block-> block.index [section] = 0xFFFF;
    else
      {
        section_prev = NULL;
        copy_from = 0;
        while (copy_from < comp_size)   /*  Slice comp data into blocks      */
          {
            if (bits-> free_list)       /*  Get block from free-list         */
              {                         /*    if available                   */
                block_nbr        = bits-> free_list;
                bits-> free_list = bits-> block [block_nbr]-> right;
              }
            else
              {                         /*  Allocate new block                */
                block_nbr = alloc_block (bits);
                if (block_nbr == 0)     /*  If no memory left                */
                    return (-1);        /*    we give up here                */
              }
            section_block         = bits-> block [block_nbr];
            section_block-> right = 0;
            section_block-> size  = min (BIT_DATASIZE, (comp_size - copy_from));
            memcpy (section_block-> block.data, compressed + copy_from,
                    section_block-> size);

            /*  Attach block to chain                                        */
            if (section_prev)
                section_prev-> right = block_nbr;
            else
                index_block-> block.index [section] = block_nbr;

            copy_from += section_block-> size;
            section_prev = section_block;
          }
      }
    return 0;
}


/*  -------------------------------------------------------------------------
 *  alloc_block -- internal
 *
 *  Allocates a new section block (BITBLOCK bytes), and updates the bit
 *  string block table and block count accordingly.  Returns the index
 *  in the block table, or 0 if the block could not be allocated.
 */

static int
alloc_block (BITS *bits)
{
    BITBLOCK
        *the_block;                     /*  Points to allocated block        */
    int
        block_nbr = 0;

    the_block = mem_alloc (sizeof (BITBLOCK));
    if (the_block)
      {
        block_nbr = bits-> block_count++;
        bits-> block [block_nbr] = the_block;
      }
    return (block_nbr);
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_test

    Synopsis: Tests the specified bit in the bitmap.  Returns 1 or 0.
    ---------------------------------------------------------------------[>]-*/

int
bits_test (
    const BITS *bits,
    long bit)
{
    int
        index,                          /*  Number of index block            */
        section;                        /*  Number of section in index       */
    dbyte
        bit_nbr;                        /*  Number of bit in section         */

    ASSERT (bits);

    locate_bit  (bits, bit, &index, &section, &bit_nbr);
    get_section ((BITS *) bits, index, section, section_data, FALSE);
    if ((section_data [bit_nbr / 8]) & (1 << (bit_nbr % 8)))
        return (1);
    else
        return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_fput

    Synopsis: Writes the bitstring to the specified file stream.  To read
    the bitstring, use the bits_fget() function.  The structure of the
    bitstring is:
    ---------------------------------------------------------------------[>]-*/

int
bits_fput (FILE *file,
    const BITS *bits)
{
    int
        block_nbr;                      /*  Bitstring block number           */
    word
        comp_size;                      /*  Size of compressed block         */
    BITBLOCK
        *block_ptr;                     /*  Points to bitstring block        */

    ASSERT (bits);
    ASSERT (file);

    /*  Write bitstring header to file                                       */
    fwrite (&bits-> block_count, sizeof (bits-> block_count), 1, file);
    fwrite (&bits-> free_list,   sizeof (bits-> free_list),   1, file);

    /*  Write bitstring blocks to file                                       */
    for (block_nbr = 0; block_nbr < bits-> block_count; block_nbr++)
      {
        block_ptr = bits-> block [block_nbr];
        comp_size = compress_block ((byte *) block_ptr,
                                    compressed, (word) block_ptr-> size);

        fwrite (&comp_size, sizeof (comp_size), 1, file);
        fwrite (compressed, comp_size,          1, file);
      }
    return 0;
}


/*  ---------------------------------------------------------------------[<]-
    Function: bits_fget

    Synopsis: Reads a bitstring from the specified file stream.  You must
    have previously written the bitstring using bit_fput ().  Returns a
    newly-created bitmap, or NULL if there was insufficient memory.
    ---------------------------------------------------------------------[>]-*/

BITS *
bits_fget (FILE *file)
{
    int
        block_nbr;                      /*  Bitstring block number           */
    word
        comp_size;                      /*  Size of compressed block         */
    BITBLOCK
        *block_ptr;                     /*  Points to bitstring block        */
    BITS
        *bits;

    ASSERT (file);

    bits = bits_create ();              /*  Create a new, empty bitmap       */

    /*  Read bitstring header from file                                      */
    fread (&bits-> block_count, sizeof (bits-> block_count), 1, file);
    fread (&bits-> free_list,   sizeof (bits-> free_list),   1, file);

    /*  Read bitstring blocks from file                                      */
    for (block_nbr = 0; block_nbr < bits-> block_count; block_nbr++)
      {
        block_nbr = alloc_block (bits);
        if (block_nbr == 0)
          {
            bits_destroy (bits);
            return (NULL);
          }
        fread (&comp_size, sizeof (comp_size), 1, file);
        fread (compressed, comp_size,          1, file);
        block_ptr        = bits-> block [block_nbr];
        block_ptr-> size = expand_block (compressed, (byte *) block_ptr,
                                         comp_size);
      }
    return (bits);
}
