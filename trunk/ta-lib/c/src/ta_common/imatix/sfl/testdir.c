/*  ----------------------------------------------------------------<Prolog>-
    Name:       testdir.c
    Title:      Test program for directory functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/02  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Testdir runs the specified or current directory through
                the open_dir and read_dir functions, formatting the output
                using format_dir.  Use this program if you think that the
                directory functions are not working correctly.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

void handle_signal (int the_signal)
{
    exit (EXIT_FAILURE);
}

int main (int argc, char *argv [])
{
    NODE
        * file_list;
    FILEINFO
        * file_info;
    char
        *sort_type = NULL;

    signal (SIGINT,  handle_signal);
    signal (SIGSEGV, handle_signal);
    signal (SIGTERM, handle_signal);

    if (argc > 2)
        sort_type = argv[2];

    file_list = load_dir_list (argv [1], sort_type);
    if (file_list)
      {
        for (file_info  = file_list-> next;
             file_info != (FILEINFO *) file_list;
             file_info  = file_info-> next
            )
            puts (format_dir (&file_info-> dir, TRUE));
        free_dir_list (file_list);
      }
    mem_assert ();

    return (EXIT_SUCCESS);
}
