/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflenv.h
    Title:      Environment variable functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/14  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/10/22

    Synopsis:   Provides functions to read environment variables (also called
                shell variables or logical variables.)  Provides translation
                into numeric and Boolean values.  Provides functions to work
                with the environment block.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef _SFLENV_INCLUDED                /*  Allow multiple inclusions        */
#define _SFLENV_INCLUDED


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

char    *env_get_string   (const char *name, const char *default_value);
long     env_get_number   (const char *name, long default_value);
Bool     env_get_boolean  (const char *name, Bool default_value);
DESCR   *env2descr        (void);
char   **descr2env        (const DESCR *descr);
SYMTAB  *env2symb         (void);
char   **symb2env         (const SYMTAB *symtab);
char   **env_copy         (char **environment);

#ifdef __cplusplus
}
#endif

#endif
