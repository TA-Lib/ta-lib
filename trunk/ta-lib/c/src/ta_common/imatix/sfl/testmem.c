/*  ----------------------------------------------------------------<Prolog>-
    Name:       testmem.c
    Title:      Test program for bitstring functions
    Package:    Standard Function Library (SFL)

    Written:    1996/07/24  iMatix SFL project team <sfl@imatix.com>
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
    MEMTRN
        *tr1, *tr2;
    void
        *a1, *a2, *a3, *a4;

    tr1 = mem_new_trans();
    tr2 = mem_new_trans();

    a1 = mem_alloc (1);
    a2 = memt_alloc (tr1, 2);
    a3 = memt_alloc (tr1, 3);
    a4 = memt_alloc (tr2, 4);
    mem_display (stdout);

    mem_commit (tr1);
    mem_rollback (tr2);
    mem_display (stdout);

    mem_free (a1);
    mem_free (a2);
    mem_free (a3);

    mem_assert ();
    return (EXIT_SUCCESS);
}
