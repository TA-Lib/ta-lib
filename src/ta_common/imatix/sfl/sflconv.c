/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflconv.c
    Title:      Global variables for conversion functions
    Package:    Standard Function Library (SFL)

    Written:    1996/01/07  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Defines various global variables used by the conversion
                functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflconv.h"                    /*  Prototypes for functions         */

/*  The conv_reason details the last error returned by a conv_ function.     */
/*  Constants for the various possible errors are defined in ifconv.h.       */

int conv_reason = 0;

/*  The conv_reason_text array provides an error message text for each       */
/*  value of conv_reason.                                                    */

char *conv_reason_text [] =
  {
    "No errors",
    "Unrecognised char in input",
    "Value out of valid range",
    "Not a yes/no or true/false value",
    "More than one 'am' or 'pm'",
    "Result too large for output",
    "Too few or too many digits",
    "Too many delimiters",
    "Unknown month name",
    "3/5 digits in a row not allowed",
    "More than one month name",
    "Not enough decimals supplied",
    "Result too large for output",
    "More than one sign character",
    "Sign not allowed if unsigned",
    "Malformed financial negative",
    "More than one decimal point",
    "Decimals not allowed if integer",
    "Decimals not allowed if hidden",
    "Too many decimal positions",
    "Too many digits for number"
  };
