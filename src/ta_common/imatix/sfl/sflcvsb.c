/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcvsb.c
    Title:      Converts a Boolean to a string
    Package:    Standard Function Library (SFL)

    Written:    1996/01/05  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/29

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflconv.h"                    /*  Prototypes for functions         */


/*  ---------------------------------------------------------------------[<]-
    Function: conv_str_bool

    Synopsis: Converts a string to a Bool.  Accepts T/Y/1 as TRUE, F/N/0
    as FALSE, ignoring case.  Looks only at the first letter of the string.
    Returns 1 for TRUE, 0 for FALSE, -1 if the string was not valid.
    ---------------------------------------------------------------------[>]-*/

int
conv_str_bool (
    const char *string)
{
    char
        ch;

    ch = tolower (string [0]);
    conv_reason = 0;                    /*  No conversion errors so far      */
    if (ch == 'y' || ch == 't' || ch == '1')
        return (1);
    else
    if (ch == 'n' || ch == 'f' || ch == '0')
        return (0);
    else
      {
        conv_reason = CONV_ERR_NOT_BOOLEAN;
        return (-1);
      }
}
