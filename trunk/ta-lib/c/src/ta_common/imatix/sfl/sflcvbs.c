/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcvbs.c
    Title:      Converts a Boolean to a string
    Package:    Standard Function Library (SFL)

    Written:    1995/12/17  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflconv.h"                    /*  Prototypes for functions         */


/*  ---------------------------------------------------------------------[<]-
    Function: conv_bool_str

    Synopsis: Converts a Bool value to a string according to the specified
    format: 0 = Yes|No; 1 = Y|N, 2 = True|False, 3 = T|F, 4 = 1|0.  Returns
    a pointer to a static string that is overwritten by each call.
    ---------------------------------------------------------------------[>]-*/

char *
conv_bool_str (
    Bool boolean,
    int  format)
{
    static char *bool_name [] =
      {
        "Yes",  "No",
        "Y",    "N",
        "True", "False",
        "T",    "F",
        "1",    "0"
      };

    conv_reason = 0;                    /*  No conversion errors so far      */
    return (bool_name [format * 2 + (boolean? 0: 1)]);
}
