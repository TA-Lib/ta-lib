/*  ----------------------------------------------------------------<Prolog>-
    Name:       testmime.c
    Title:      Test program for base64 functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/14  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/06/01

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
        read_buffer    [LINE_MAX+1];
    byte
        decoded_buffer [LINE_MAX+1],
        encoded_buffer [LINE_MAX+1];
    size_t
        return_size;
    FILE
        *file;

    memset (read_buffer,    0, LINE_MAX+1);
    memset (encoded_buffer, 0, LINE_MAX+1);
    memset (decoded_buffer, 0, LINE_MAX+1);

    if (argc > 1)
      {
        file = file_open (argv[1], 'r');
        if (file != NULL)
          {
            while (file_read (file, read_buffer) == TRUE)
              {
                printf ("Read    : %s\n", read_buffer);

                /*  Encode the buffer                                        */
                return_size = encode_base64 ((byte *) read_buffer, 
                                             encoded_buffer,
                                             strlen (read_buffer));
                printf ("Encoded : %s\n", encoded_buffer);

                /*  Decode the buffer                                        */
                decode_base64 (encoded_buffer, decoded_buffer, return_size);
                printf ("Decoded : %s\n\n", decoded_buffer);

                /*  Test if error                                            */
                if (strcmp (read_buffer, (char *) decoded_buffer) != 0)
                  {
                    printf ("Error : decoded is not same as read\n");
                    break;
                  }
                /*  Reset the buffers                                        */
                memset (decoded_buffer, 0, LINE_MAX+1);
                memset (encoded_buffer, 0, LINE_MAX+1);
              }
            file_close (file);
          }
      }
    return (EXIT_SUCCESS);
}
