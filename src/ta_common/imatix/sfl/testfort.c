/*  ----------------------------------------------------------------<Prolog>-
    Name:       testfort.c
    Title:      Test program for fortune-cookie functions
    Package:    Standard Function Library (SFL)

    Written:    1999/08/18
    Revised:    1999/08/18

    Synopsis:   testfort -b filename outfile  builds fortune file
                testfort filename             displays random fortune
                use testfort.txt as test input file

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"
#include "sflfort.h"

int main (int argc, char *argv [])
{
    int
        rc;
    char
        *textfile,
        *fortunefile,
        *fortune;
        
    if (argc < 2)
      {
        puts ("syntax: testfort [-b textfile] fortunefile");
        return (EXIT_SUCCESS);
      }
    if (streq (argv [1], "-b"))
      {
        if (argc < 4)
          {
            puts ("syntax: testfort [-b textfile] fortunefile");
            return (EXIT_SUCCESS);
          }
        textfile    = argv [2];
        fortunefile = argv [3];
        printf ("Compressing %s to %s...\n", textfile, fortunefile);
        rc = fortune_build (textfile, fortunefile, TRUE);
        if (rc)
            printf ("Failed: %s\n", strerror (errno));
        else
            printf ("Okay\n");
      }
    else
      {
        fortunefile = argv [1];
        fortune = fortune_read (fortunefile);
        if (fortune)
            printf (fortune);
        else
            printf ("Failed: %s\n", strerror (errno));
      }
    return (EXIT_SUCCESS);
}

