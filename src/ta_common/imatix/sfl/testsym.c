/*  ----------------------------------------------------------------<Prolog>-
    Name:       testsym.c
    Title:      Test program for symbol-table functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/24  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

static Bool
dump_symbol (SYMBOL *symbol, ...)
{
    printf ("%s = %s\n", symbol-> name, symbol-> value);
    return (TRUE);
}


int
compare (const void *symb1, const void* symb2)
{
    int
        val1,
        val2;

    val1 = atoi ((*(SYMBOL **) symb1)-> value);
    val2 = atoi ((*(SYMBOL **) symb2)-> value);
    if (val1 < val2)
        return (-1);
    else
    if (val1 > val2)
        return (1);

    return (0);
}

int main (int argc, char *argv [])
{
    SYMTAB *
        table;

    table = sym_create_table ();
    sym_create_symbol (table, "a Symbol 1", "1");
    sym_create_symbol (table, "d Symbol 2", "2");
    sym_create_symbol (table, "e Symbol 4", "4");
    sym_create_symbol (table, "b Symbol 3", "3");
    sym_exec_all (table, dump_symbol);

    puts ("\nSorted by symbol value:");
    sym_sort_table (table, compare);
    sym_exec_all (table, dump_symbol);

    puts ("\nSorted by symbol name:");
    sym_sort_table (table, NULL);
    sym_exec_all (table, dump_symbol);

    sym_delete_table (table);

    return (EXIT_SUCCESS);
}
