/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflfort.c
    Title:      Fortune-cookie functions
    Package:    Standard Function Library (SFL)

    Written:    1999/08/16  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/30

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflfile.h"                    /*  File-handling functions          */
#include "sflstr.h"                     /*  String-handling functions        */
#include "sflmem.h"                     /*  Memory-handling functions        */
#include "sflcomp.h"                    /*  Compression functions            */
#include "sflfort.h"                    /*  Prototypes for functions         */


/*  These constants define the maximum sizes of blocks in the fortune file
 *  and the maxium number of blocks in such a file.  You should not change
 *  these unless you're prepared to rebuild all fortune files.
 */
 
#define BLOCK_SIZE  32000U              /*  Max. size of output block        */
#define MAX_PARAS   250                 /*  Max. paragraphs per block        */
#define MAX_BLOCKS  65000U              /*  Max. blocks in file              */

static void have_end_of_paragraph (Bool compress);
static void have_end_of_block     (Bool compress);

static FILE
    *input,                             /*  Input file stream                */
    *output,                            /*  Output file stream               */
    *scratch;                           /*  Temporary copy of input          */
static int
    *sizes;                             /*  Size of each block               */
static dbyte
    insize, 
    outsize,
    nbr_blocks,
    nbr_paras,
    output_dbyte;
static qbyte
    output_qbyte;
static byte
    *inbuf,
    *outbuf;


/*  ---------------------------------------------------------------------[<]-
    Function: fortune_build

    Synopsis: Builds an indexed fortune file from a formatted text file.
    The text file contains paragraphs separated by lines containing '%%'.
    If okay, returns 0, else returns -1, see errno for the cause.
    ---------------------------------------------------------------------[>]-*/

int
fortune_build (
    const char *infile,                 /*  Name of file to compress         */
    const char *outfile,                /*  Output file, created             */
    Bool        compress)               /*  Compress yes/no                  */
{
    char
        line [LINE_MAX + 1];            /*  Line from input file             */
    int
        block_nbr;
    long
        offset;

    inbuf   = mem_alloc (BLOCK_SIZE);
    outbuf  = mem_alloc (BLOCK_SIZE);
    sizes   = mem_alloc (MAX_BLOCKS * sizeof (int));
    input   = file_open (infile, 'r');
    scratch = ftmp_open (NULL);

    if (!inbuf || !outbuf || !sizes || !input || !scratch)
      {
        mem_free (inbuf);
        mem_free (outbuf);
        mem_free (sizes);
        return (-1);
      }

    nbr_blocks = 0;
    nbr_paras  = 0;                     /*  Number of paragraphs in block    */
    insize     = 2;                     /*  Leave room for nbr_paras         */

    while (file_read (input, line))
      {
        if (streq (line, "%%"))
            have_end_of_paragraph (compress);
        else
          {
            memcpy (inbuf + insize, line, strlen (line));
            insize += strlen (line);
            inbuf [insize++] = '\n';
          }
      }
    have_end_of_paragraph (compress);
    if (nbr_paras)
        have_end_of_block (compress);

    fclose (input);                     /*  Finished with input file         */

    output = fopen (outfile, "wb");
    fprintf (output, "IFF%s -- http://www.imatix.com/ --\n%c",
                     compress? "CMP": "TXT", 26);

    output_dbyte = htons (nbr_blocks);
    fwrite (&output_dbyte, 2, 1, output);
    offset = ftell (output) + nbr_blocks * 4;

    for (block_nbr = 0; block_nbr < nbr_blocks; block_nbr++)
      {
        output_qbyte = htonl (offset);
        fwrite (&output_qbyte, 4, 1, output);
        offset += sizes [block_nbr];
      }
    fseek (scratch, 0, SEEK_SET);       /*  Back to start of scratch file    */
    while ((outsize = fread (outbuf, 1, BLOCK_SIZE, scratch)) != 0) 
        fwrite (outbuf, 1, outsize, output);
        
    file_close (output);
    ftmp_close (scratch);
    mem_free (sizes);
    mem_free (inbuf);
    mem_free (outbuf);

    return (0);                         /*  No errors                        */
}


static void
have_end_of_paragraph (Bool compress)
{
    if (insize)                         /*  If there is a paragraph waiting  */
      {
        inbuf [insize++] = '\0';        /*  Terminate paragraph with null    */
        nbr_paras++;
        if ((nbr_paras == MAX_PARAS)
        || (insize > BLOCK_SIZE - 2000))
          {
            have_end_of_block (compress);
            nbr_paras = 0;
            insize = 2;
          }
      }
}

static void
have_end_of_block (Bool compress)
{
    output_dbyte = htons (nbr_paras);
    *(dbyte *) inbuf = output_dbyte;

    if (compress)
      {
        outsize = compress_block (inbuf, outbuf, insize);    
        output_dbyte = htons (outsize);
        fwrite (&output_dbyte, 2, 1, scratch);
        fwrite (outbuf, 1, outsize, scratch);                
        sizes [nbr_blocks] = outsize + 2;
      }
    else
      {
        output_dbyte = htons (insize);
        fwrite (&output_dbyte, 2, 1, scratch);
        fwrite (inbuf, 1, insize, scratch);
        sizes [nbr_blocks] = insize + 2;
      }
    insize = 2;                         /*  Clear input buffer               */
    nbr_paras = 0;
    nbr_blocks++;
}


/*  ---------------------------------------------------------------------[<]-
    Function: fortune_read

    Synopsis: Reads a random paragraph from the specified fortune file.  The
    fortune file is located in the current directory or on the current path.
    The paragraph of text is returned in a freshly-allocated block of memory
    that you should free afterwards using mem_free().  Returns NULL if the
    fortune file could not be opened.
    ---------------------------------------------------------------------[>]-*/

char *
fortune_read (const char *fortune_file)
{
    static Bool
        first_time = TRUE;
    FILE
        *fortunes;                      /*  FFF input stream                 */
    byte
        *inbuf,                         /*  Block read from file             */
        *outbuf;                        /*  And after decompression          */
    int
        nbr_blocks,                     /*  Number of blocks in data file    */
        nbr_paras,                      /*  Number of paragraphs in block    */
        paragraph;
    dbyte
        input_dbyte,
        block_size;                     /*  Size of current block            */
    qbyte
        input_qbyte,
        block_offset;                   /*  Offset of block in file          */
    char
        *para_start;                    /*  Start of paragraph               */
    Bool
        compressed;                     /*  Compressed fortune data?         */

    /*  Initialise random-number generator if necessary                      */
    if (first_time)
      {
        randomize ();
        first_time = FALSE;
      }

    /*  Look for fortunes file                                               */
    fortunes = file_locate ("PATH", fortune_file, NULL);
    if (fortunes == NULL)
        return (NULL);

    /*  Allocate working buffers                                             */
    inbuf  = mem_alloc (BLOCK_SIZE);
    outbuf = mem_alloc (BLOCK_SIZE);
    if (inbuf == NULL || outbuf == NULL)
      {
        mem_free (inbuf);
        mem_free (outbuf);
        return (NULL);
      }

    /*  Indexed Fortune File format starts with IFFCMP or IFFTXT             */
    file_read (fortunes, (char *) inbuf);
    if (memcpy (inbuf, "IFFTXT", 6) == 0)
        compressed = FALSE;
    else
        compressed = TRUE; 

    while (fgetc (fortunes) != 26);     /*  Skip past file header            */

    /*  Get total number of blocks in file                                   */
    fread (&input_dbyte, 2, 1, fortunes);
    nbr_blocks = ntohs (input_dbyte);

    /*  Look at random block address in Toc                                  */
    fseek (fortunes, random (nbr_blocks) * 4, SEEK_CUR);
    fread (&input_qbyte, 4, 1, fortunes);
    block_offset = ntohl (input_qbyte);

    /*  Go read, then decompress the block                                   */
    fseek (fortunes, block_offset, SEEK_SET);
    fread (&input_dbyte, 2, 1, fortunes);
    block_size = ntohs (input_dbyte);

    if (compressed)
      {
        fread (inbuf, 1, block_size, fortunes);
        expand_block (inbuf, outbuf, block_size);
      }
    else
        fread (outbuf, 1, block_size, fortunes);

    /*  Chose random paragraph from block                                    */
    input_dbyte = *(dbyte *) outbuf;
    nbr_paras   = ntohs (input_dbyte);
    paragraph   = random (nbr_paras);
    para_start  = (char *) outbuf + 2;
    while (paragraph--)
        para_start = strchr (para_start, '\0') + 1;

    para_start = mem_strdup (para_start);

    /*  Release allocated memory                                             */
    fclose (fortunes);
    mem_free (inbuf);
    mem_free (outbuf);

    return (para_start);
}
