/* Note: This file has been adapted for running within TA-LIB.
 * If you intend to use iMatix, please use their original file.
 */

/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfltok.h
    Title:      String token manipulation functions.
    Package:    Standard Function Library (SFL)

    Written:    1996/09/10  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/03/31

    Synopsis:   Provides functions to break strings into tokens and handle
                symbol substitution in strings.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLTOK_INCLUDED                /*  Allow multiple inclusions        */
#define SFLTOK_INCLUDED


/*  Function prototypes                                                      */
#include "ta_common.h"

#ifdef __cplusplus
extern "C" {
#endif

char **tok_split      (TA_Libc *libHandle, const char *string);
char **tok_split_rich (TA_Libc *libHandle, const char *string, const char *delims);
void   tok_free       (TA_Libc *libHandle, char **token_list);
char **tok_push       (TA_Libc *libHandle, char **token_list, const char *string);
int    tok_size       (TA_Libc *libHandle, char **token_list);
size_t tok_text_size  (TA_Libc *libHandle, char **token_list);

char  *tok_subst      (TA_Libc *libHandle, const char *string, SYMTAB *symbols);

#ifdef __cplusplus
}
#endif

#endif
