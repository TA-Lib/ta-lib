/*  ----------------------------------------------------------------<Prolog>-
    Name:       testexdr.c
    Title:      Test program for EXDR functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/26  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

void dump (byte *buffer, int buffer_size);

int main (int argc, char *argv [])
{
    static byte
            buffer [1024];
    static char
            string_value [1024];
    char    *string = string_value;
    int     buffer_size;
    Bool    Bool_value;
    byte    byte_value;
    dbyte   dbyte_value,
            size_value;
    qbyte   qbyte_value;

    puts ("--- bwq ---");
    buffer_size = exdr_write (buffer, "bwq", (byte) 0x12,
                             (dbyte) 0x12, (qbyte) 0x12L);
    dump (buffer, buffer_size);
    exdr_read (buffer, "bwq", &byte_value, &dbyte_value, &qbyte_value);
    printf ("%02x %04x %08lx\n", byte_value, dbyte_value, qbyte_value);

    puts ("--- cdl ---");
    buffer_size = exdr_write (buffer, "cdl", (byte) 0x12,
                             (dbyte) 0x1234, (qbyte) 0x1234L);
    dump (buffer, buffer_size);
    exdr_read (buffer, "cdl", &byte_value, &dbyte_value, &qbyte_value);
    printf ("%02x %04x %08lx\n", byte_value, dbyte_value, qbyte_value);

    puts ("--- bwq ---");
    buffer_size = exdr_write (buffer, "bwq", (byte) 0x12,
                             (dbyte) 0x1234, (qbyte) 0x123456L);
    dump (buffer, buffer_size);
    exdr_read (buffer, "bwq", &byte_value, &dbyte_value, &qbyte_value);
    printf ("%02x %04x %08lx\n", byte_value, dbyte_value, qbyte_value);

    puts ("--- cdl ---");
    buffer_size = exdr_write (buffer, "cdl", (byte) 0x12,
                             (dbyte) 0x1234, (qbyte) 0x12345678L);
    dump (buffer, buffer_size);
    exdr_read (buffer, "cdl", &byte_value, &dbyte_value, &qbyte_value);
    printf ("%02x %04x %08lx\n", byte_value, dbyte_value, qbyte_value);

    puts ("--- Bs ---");
    buffer_size = exdr_write (buffer, "Bs", TRUE, "AZaz");
    dump (buffer, buffer_size);
    exdr_read (buffer, "Bs", &Bool_value, &string);
    printf ("%d %s\n", Bool_value, string_value);

    puts ("--- mMc ---");
    buffer_size = exdr_write (buffer, "mMc", (dbyte) 4, "AZaz", (byte) 0xff);
    dump (buffer, buffer_size);
    exdr_read (buffer, "mMc", &dbyte_value, &string, &byte_value);
    printf ("%d %c%c%c%c %x\n", dbyte_value, string_value [0],
            string_value [1], string_value [2], string_value [3], byte_value);

    puts ("--- dmMq ---");
    dbyte_value = 0x1234;
    qbyte_value = 0x12345678L;
    buffer_size = exdr_write (buffer, "dmMq", dbyte_value,
                              (dbyte) strlen ("AZaz"), "AZaz", qbyte_value);
    dump (buffer, buffer_size);
    exdr_read (buffer, "dmMq", &dbyte_value,
               &size_value, &string, &qbyte_value);
    printf ("%04x %d %c%c%c%c %08lx\n", dbyte_value, size_value,
             string_value [0], string_value [1],
             string_value [2], string_value [3], qbyte_value);

    return (EXIT_SUCCESS);
}


void dump (byte *buffer, int buffer_size)
{
    int
        byte_nbr;

    for (byte_nbr = 0; byte_nbr < buffer_size; byte_nbr++)
        printf ("%0x ", buffer [byte_nbr]);
    printf ("\n");
}
