/*  ----------------------------------------------------------------<Prolog>-
    Name:       testuid.c
    Title:      Test program for user id functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/03  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/11/02  Rob Judd   Unused variable warning in Win32

    Synopsis:   Link and set uid bit on.  Run from another account.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"
#include "sfluid.h"

int main (int argc, char *argv [])
{
    printf ("Login name = %s\n", get_login ());
    return (EXIT_SUCCESS);
}

