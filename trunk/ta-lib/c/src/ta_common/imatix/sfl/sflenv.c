/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflenv.c
    Title:      Environment variable functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/14  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/03/18

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflconv.h"                    /*  Conversion functions             */
#include "sflstr.h"                     /*  String functions                 */
#include "sflsymb.h"                    /*  Symbol-table functions           */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory-allocation functions      */
#include "sflenv.h"                     /*  Prototypes for functions         */


/*  ---------------------------------------------------------------------[<]-
    Function: env_get_string

    Synopsis: Translates the specified environment variable and returns a
    static string containing the value.  If the variable is not defined in
    the environment, returns the specified default value.  Note: if you
    want to use the value in a program you should use strdupl() to make a
    copy.  The environment variable name is always translated into upper
    case.  The default value may be NULL.

    Examples:
    config_file = strdupl (env_get_string ("config", "default.cfg"));
    ---------------------------------------------------------------------[>]-*/

char *
env_get_string (
    const char *name,
    const char *default_value)
{
    char
        *variable_name,
        *variable_value;

    variable_name = mem_strdup (name);
    variable_value = getenv (strupc (variable_name));
    mem_free (variable_name);
    return (variable_value? variable_value: (char *) default_value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: env_get_number

    Synopsis: Translates the specified environment variable and returns the
    long numeric value of the string.  If the variable is not defined in
    the environment, returns the specified default value.  The environment
    variable name is always translated into upper case.

    Examples:
    max_retries = env_get_number ("retries", 5);
    ---------------------------------------------------------------------[>]-*/

long
env_get_number (
    const char *name,
    long default_value)
{
    char
        *variable_value;

    variable_value = env_get_string (name, NULL);
    return (variable_value? atol (variable_value): default_value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: env_get_boolean

    Synopsis: Translates the specified environment variable and returns the
    Boolean value of the string.  If the variable is not defined in the
    environment, returns the specified default value. The environment
    variable name is always translated into upper case.  The environment
    variable value is interpreted irrespective to upper/lower case, and
    looking at the first letter only.  T/Y/1 are TRUE, everything else is
    FALSE.  See conv_str_bool() for the conversion rules.

    Examples:
    enforce_security = env_get_number ("security", FALSE);
    ---------------------------------------------------------------------[>]-*/

Bool
env_get_boolean (
    const char *name,
    Bool default_value)
{
    char
        *variable_value;

    variable_value = env_get_string (name, NULL);
    return (variable_value?
           (conv_str_bool (variable_value) != 0): default_value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: env2descr

    Synopsis: Returns a DESCR pointer containing the current process
    environment strings.  The descriptor is allocated using mem_alloc();
    you should use mem_free() to deallocate when you are finished.  Returns
    NULL if there was not enough memory to allocate the descriptor.
    ---------------------------------------------------------------------[>]-*/

DESCR *
env2descr (void)
{
    return (strt2descr (environ));
}


/*  ---------------------------------------------------------------------[<]-
    Function: descr2env

    Synopsis: Returns an environment block from the supplied descriptor
    data.  The returned block is an array of strings, terminated by a null
    pointer.  Each string is allocated independently using mem_alloc().
    Returns NULL if there was not enough memory to allocate the block.
    ---------------------------------------------------------------------[>]-*/

char **
descr2env (
    const DESCR *descr)
{
    return (descr2strt (descr));
}

/*  ---------------------------------------------------------------------[<]-
    Function: env2symb

    Synopsis: Creates a symbol table containing the processes environment
    variables.  Each variable is stored as a name plus value.  The names
    of the variables are converted to upper case prior to being put into
    the symbol table.  You can destroy the symbol table using
    sym_delete_table() when you are finished with it.
    ---------------------------------------------------------------------[>]-*/

SYMTAB *
env2symb (void)
{
    SYMTAB
        *symtab;                        /*  Allocated symbol table           */
    char
        *next_entry,                    /*  Environment variable + value     */
        *equals;                        /*  Position of '=' in string        */
    int
        string_nbr;                     /*  Index into string table          */

    /*  We create the table here, instead of passing through strt2symb(),
        since we have to ensure that environment variable names are stored
        in uppercase.  Some systems (NT) return mixed-case names.            */

    symtab = sym_create_table ();
    if (symtab)
      {
        for (string_nbr = 0; environ [string_nbr]; string_nbr++)
          {
            next_entry = mem_strdup (environ [string_nbr]);
            equals = strchr (next_entry, '=');
            if (equals)
              {
                *equals = '\0';         /*  Cut into two strings             */
                strupc (next_entry);
                sym_assume_symbol (symtab, next_entry, equals + 1);
              }
            mem_free (next_entry);
          }
      }
    return (symtab);
}


/*  ---------------------------------------------------------------------[<]-
    Function: symb2env

    Synopsis: Returns an environment block from the supplied symbol table.
    The returned block is an array of strings, terminated by a null
    pointer.  Each string is allocated independently using mem_alloc().
    Returns NULL if there was not enough memory to allocate the block.
    Normalises the environment variable names as follows: converts all
    letters to uppercase, and non-alphanumeric characters to underlines.
    To free the array, use strtfree().  See also symb2strt().
    ---------------------------------------------------------------------[>]-*/

char **
symb2env (
    const SYMTAB *symtab)
{
    MEMTRN
        *memtrn;                        /*  Memory transation                */
    SYMBOL
        *symbol;                        /*  Pointer to symbol                */
    char
        **strings,                      /*  Returned string array            */
        *name_and_value,                /*  Name=value string                */
        *nameptr;                       /*  Pointer into name                */
    int
        string_nbr;                     /*  Index into symbol_array          */

    if (!symtab)
        return (NULL);                  /*  Return NULL if argument is null  */

    /*  Allocate the array of pointers with one slot for the final NULL      */
    memtrn  = mem_new_trans ();
    strings = memt_alloc (memtrn, sizeof (char *) * (symtab-> size + 1));
    if (strings)
      {
        string_nbr = 0;
        for (symbol = symtab-> symbols; symbol; symbol = symbol-> next)
          {
            /*  Allocate space for "name=value" plus final null char         */
            name_and_value = memt_alloc (memtrn,
                                        (strlen (symbol-> name)
                                         + strlen (symbol-> value) + 2));
            if (!name_and_value)        /*  Quit if no memory left           */
              {
                mem_rollback (memtrn);
                return (NULL);
              }
            /*  Get symbol name in uppercase, using underlines               */
            strcpy (name_and_value, symbol-> name);
            for (nameptr = name_and_value; *nameptr; nameptr++)
                if (isalnum (*nameptr))
                    *nameptr = toupper (*nameptr);
                else
                    *nameptr = '_';
            strcat (name_and_value, "=");
            strcat (name_and_value, symbol-> value);
            strings [string_nbr++] = name_and_value;
          }
        strings [string_nbr] = NULL;    /*  Store final null pointer         */
      }
    mem_commit (memtrn);
    return (strings);
}


/*  ---------------------------------------------------------------------[<]-
    Function: env_copy

    Synopsis: Returns an environment block which is a copy of the supplied
    environment block, in all new memory.  If no environment block is
    supplied the current process environment is copied.  The returned block
    is an array of strings, terminated by a null pointer.  Each string is
    allocated independently using mem_alloc().  Returns NULL if there was
    not enough memory to allocate the block.  No changes are made to the
    strings during copying. To free the array, use strtfree().
    ---------------------------------------------------------------------[>]-*/

char **
env_copy (
    char **environment)
{
    MEMTRN
        *memtrn;                        /*  Memory transation                */
    char
        **env = environment,            /*  Environment to copy              */
        **newenv = NULL;                /*  Copy of environment              */
    int
        size  = 0,     
        pos = 0;

    if (env == NULL)
        env = environ;        /*  Default is to copy the process environment */

    /*  Count the size of the environment                                    */
    for (size = 0; env [size] != NULL; env++)
        ;  /* EMPTY BODY */

    memtrn = mem_new_trans ();
    if (!memtrn)
        return NULL;

    newenv = memt_alloc (memtrn, ((size+1) * sizeof(char *)));
    if (!newenv)
      {
      	mem_rollback (memtrn);
        return NULL;
      }

    for (pos = 0; pos < size; pos++)
      {
        newenv [pos] = memt_strdup (memtrn, env [pos]);
        if (newenv [pos] == NULL)
          {
            mem_rollback (memtrn);
            return NULL;
          }
      }  
    newenv [pos] = NULL;               /*  Terminate the array               */
    mem_commit (memtrn);               /*  Commit the memory allocations     */
   
    return newenv;
}
