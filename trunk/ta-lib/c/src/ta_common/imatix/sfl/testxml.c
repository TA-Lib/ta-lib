/*  ----------------------------------------------------------------<Prolog>-
    Name:       testxml.c
    Title:      Test program for XML functions
    Package:    Standard Function Library (SFL)

    Written:    1998/03/23  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/06/21

    Synopsis:   Test XML loading and saving.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

int main (int argc, char *argv [])
{
    XML_ITEM
        *root;

    root = xml_new (NULL, "root", "");
    if (xml_load_file (&root, ".", argv[1], TRUE))
      {
        xml_save_file (root, "testxml.txt");
        xml_free      (root);
      }
    else
        printf ("Load error: %s\n", xml_error ());

    mem_assert ();
    return (EXIT_SUCCESS);
}

