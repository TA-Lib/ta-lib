/*  ----------------------------------------------------------------<Prolog>-
    Name:       testcryp.c
    Title:      Test program for encryption functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/22  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Tests the encryption and decryption functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

clock_t test_crypt (FILE *file, int crypt_type);

void handle_signal (int the_signal)
{
    exit (1);
}

int main (int argc, char *argv [])
{
    FILE
        *file;
    clock_t
        clk_idea = 0,
        clk_mdc  = 0,
        clk_des  = 0,
        clk_xor  = 0;
    long
        file_length = 0;

    signal (SIGINT,  handle_signal);
    signal (SIGSEGV, handle_signal);
    signal (SIGTERM, handle_signal);

    if (argc == 1)
        printf ("Error : no file for test\n");

    file = fopen (argv[1], "rb");
    if (file)
      {
        fseek (file, 0, SEEK_END);
        file_length = ftell (file);
        printf("Test IDEA in progress...\n");
        clk_idea = test_crypt (file, CRYPT_IDEA);
        printf("Test MDC  in progress...\n");
        clk_mdc  = test_crypt (file, CRYPT_MDC );
        printf("Test XOR  in progress...\n");
        clk_xor  = test_crypt (file, CRYPT_XOR );
        printf("Test DES  in progress...\n");
        clk_des  = test_crypt (file, CRYPT_DES );

        fclose (file);
        printf ("Test of Encryption/decryption functions\n");
        printf ("---------------------------------------\n");
        printf (" IDEA : %4.01f sec / %ld bytes\n",
                (float)clk_idea / CLOCKS_PER_SEC,
                file_length
               );
        printf (" MDC  : %4.01f sec / %ld bytes\n",
                (float)clk_mdc  / CLOCKS_PER_SEC,
                file_length
               );
        printf (" XOR  : %4.01f sec / %ld bytes\n",
                (float)clk_xor  / CLOCKS_PER_SEC,
                file_length
               );
        printf (" DES  : %4.01f sec / %ld bytes\n",
                (float)clk_des  / CLOCKS_PER_SEC,
                file_length
               );
      }
    return (EXIT_SUCCESS);
}

clock_t
test_crypt (FILE *file, int crypt_type)
{
    static char
        str_crypt_type   [10],
        read_buffer [32000+1],
        test_buffer [32000+1];
    size_t
        read_length;
    long
        nb_block;
    clock_t
        clk_begin,
        clk_end;
    char
        key [] = "SomeLongText That Can Be Used As A Key";

    switch (crypt_type)
      {
        case CRYPT_IDEA: strcpy (str_crypt_type, "IDEA"); break;
        case CRYPT_MDC : strcpy (str_crypt_type, "MDC" ); break;
        case CRYPT_XOR : strcpy (str_crypt_type, "XOR" ); break;
        case CRYPT_DES : strcpy (str_crypt_type, "DES" ); break;
      }
    memset (read_buffer, 0, sizeof (read_buffer));
    memset (test_buffer, 0, sizeof (test_buffer));
    fseek  (file, 0, SEEK_SET);

    clk_begin = clock ();
    while ((read_length = fread (read_buffer, 1, 32000, file)) != 0)
      {
        nb_block = (long)(read_length / 32);
        if (nb_block * 32 != (long)read_length)
            read_length = (size_t)(nb_block * 32);
        memcpy (test_buffer, read_buffer, 32000);
        if (crypt_encode ((byte *)test_buffer, (long) read_length,
                           crypt_type, (byte *)key))
          {
            if (crypt_decode ((byte *)test_buffer, (long) read_length,
                               crypt_type, (byte *)key))
              {
                if (memcmp (read_buffer, test_buffer, read_length) != 0)
                    printf ("Error: On %s ,the return is bad\n",
                            str_crypt_type);
              }
            else
                printf ("Error: crypt_decode failed on %s\n", str_crypt_type);
          }
        else
            printf ("Error: crypt_encode failed on %s\n", str_crypt_type);
      }
    clk_end = clock();
    return ((clock_t)(clk_end - clk_begin));
}
