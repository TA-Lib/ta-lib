/*  ----------------------------------------------------------------<Prolog>-
    Name:       testbits.c
    Title:      Test program for bitstring functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/14  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Runs various tests on bit manipulation functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

int main (int argc, char *argv [])
{
    BITS
        *bits;
    FILE
        *bitfile;                       /*  Where we store the bits          */
    long
        count,
        bit_no;

    if (argc < 2)
        count = 1;
    else
        count = atol (argv [1]);

    bits_init ();
    bits = bits_create ();
    while (count--)
      {
        bit_no = (long) random (4000) * (long) random (4000);
        bits_set (bits, bit_no);

        if ((count % 100) == 0)
          {
            printf (".");
            fflush (stdout);            /*  Show output now!                 */
          }
      }
    bitfile = fopen ("testbits.out", "w");
    bits_fput (bitfile, bits);
    fclose (bitfile);

    bits_destroy (bits);
    bits_term ();

    mem_assert ();
    return (EXIT_SUCCESS);
}
