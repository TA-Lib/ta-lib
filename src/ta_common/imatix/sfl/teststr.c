/*  ----------------------------------------------------------------<Prolog>-
    Name:       teststr.c
    Title:      Test program for string functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/24  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/03/04

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

int main (int argc, char *argv [])
{
    char
        *alloc,
        dest [128],
        *table [10] = { "One", "Two", "Three", "Four", "Five",
                       "Six", "Seven", "Eight", "Nine", NULL },
        **new_table;
    DESCR
        *descr;
    int
        string;

    puts ("Testing xstrcpy():");
    xstrcpy (dest, "This ", "Is ", "A ", "String", NULL);
    puts (dest);

    puts ("Testing xstrcpy():");
    alloc = xstrcpy (NULL, "This ", "Is ", "A ", "String", NULL);
    puts (alloc);

    puts ("Testing xstrcat():");
    xstrcat (dest, "1", "2", "3", NULL);
    puts (dest);

    puts ("Testing strt2descr():");
    descr     = strt2descr (table);
    printf ("Descriptor size=%ld\n", (long) descr-> size);

    new_table = descr2strt (descr);
    printf ("Contents of table: ");
    for (string = 0; new_table [string]; string++)
        printf ("[%s] ", new_table [string]);
    puts ("");

    return (EXIT_SUCCESS);
}
