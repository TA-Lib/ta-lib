/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcvns.c
    Title:      Converts a number to a string
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
#include "sflstr.h"                     /*  String functions                 */
#include "sflconv.h"                    /*  Prototypes for functions         */


/*  ---------------------------------------------------------------------[<]-
    Function: conv_number_str

    Synopsis: Converts a number to a string.  The number format is defined
    largely by the flags argument, which can specify various values defined
    in sflconv.h:
    <TABLE>
        FLAG_N_SIGNED       Show signed number, using sign_format argument.
        FLAG_N_DECIMALS     Show decimals, using dec_format argument.
        FLAG_N_LEFT         Left-justify number; no effect if width is 0.
        FLAG_N_ZERO_FILL    Right-justfified, with leading zeroes.
        FLAG_N_ZERO_BLANK   Show zero as empty string or spaces (width > 0).
        FLAG_N_THOUSANDS    Show number with thousands separators.
    </TABLE>

    Sign formats:
    <TABLE>
        SIGN_NEG_TRAIL          Negative numbers only: 123-
        SIGN_ALL_TRAIL          All non-zero numbers:  123- 123+
        SIGN_NEG_LEAD           Negative numbers only: -123
        SIGN_ALL_LEAD           All non-zero numbers:  -123 +123
        SIGN_FINANCIAL          Negative numbers only: (123)
    </TABLE>

    Decimal formats:
    <TABLE>
        DECS_SHOW_ALL           123.10, 123.00, 0.95
        DECS_DROP_ZEROS         123.1, 123, 0.95
        DECS_HIDE_ALL           123, 123, 0
        DECS_PERCENTAGE         12300, 12300, 95
        DECS_SCIENTIFIC         1.231e2, 1.23e2, 9.5e-1
    </TABLE>

    The input number string may contain leading zeros and a leading sign
    character (space, '+', '-') if signed.  These are examples of valid
    8-digit numbers: "1234" "00001234" "12345678" "+12345678".

    If the flag FLAG_N_DECIMALS is set, the last X digits are taken to be
    decimals, where X is the value of the decimals argument.  If the number
    contains a decimal point (always '.'), this is taken to indicate the
    start of the decimal part.

    The formatted number is placed within a field of specified width.  If
    the number is right-justfied, this means it may have leading spaces.
    If the field width is 0, the number will never have leading spaces.
    Returns a pointer to the formatted string, or NULL if the specified
    width is too small for formatted number or the supplied number does
    not contain enough digits.
    ---------------------------------------------------------------------[>]-*/

char *
conv_number_str (
    const char *number,                 /*  Number to convert                */
    int   flags,                        /*  Number formatting flags          */
    char  dec_point,                    /*  Decimal point: '.' or ','        */
    int   decimals,                     /*  Number of decimals, or 0         */
    int   dec_format,                   /*  How are decimals shown?          */
    int   width,                        /*  Output field width, or 0         */
    int   sign_format                   /*  How are negatives shown?         */
)
{
    static char
        formatted [FORMAT_MAX + 1],     /*  Formatted return string          */
        zero [CONV_MAX_DECS + 2];       /*  Default value if needed          */
    int
        sep_stop,                       /*  Where we put next sep_char       */
        dec_stop,                       /*  Where we put decimal point       */
        decs_wanted = decimals,         /*  Number of decimals wanted        */
        decs_seen,                      /*  Number of decimals output        */
        sign_pos,                       /*  Where we put sign, if any        */
        digits;                         /*  Number of digits read so far     */
    char
       *dest,                           /*  Store formatted number here      */
        sign_char,                      /*  Number's sign: ' ', '+', '-'     */
        sep_char,                       /*  Thousands separator '.' or ','   */
        drop_zero,                      /*  We suppress this char            */
        ch;                             /*  Next character in picture        */
    Bool
        have_zero;                      /*  TRUE if whole number is zero     */

    ASSERT (width <= FORMAT_MAX);
    ASSERT (dec_point == '.' || dec_point == ',');

    conv_reason = 0;                    /*  No conversion errors so far      */

    /*  ---------------------------------   Prepare to copy digits  ---------*/

    if (decs_wanted > CONV_MAX_DECS)
      {
        conv_reason = CONV_ERR_DECS_OVERFLOW;
        return (NULL);                  /*  Error - too many decimals        */
      }
    /*  If value is empty, use "0" with enough decimals as default value     */
    /*  We allow one whole digit and as many decimals as needed.             */
    if (strnull (number))
      {
        strpad (zero, '0', decs_wanted + 1);
        number = zero;
      }

    /*  Pick-up sign character if present                                    */
    if (*number == ' ' || *number == '+' || *number == '-')
        sign_char = *number++;
    else
        sign_char = ' ';

    /*  While leading zero is '0' we blank-out zeros in the number           */
    drop_zero = (char) (flags & FLAG_N_ZERO_FILL? ' ': '0');

    /*  Prepare for decimals                                                 */
    if ((flags & FLAG_N_DECIMALS) == 0)
        decs_wanted = 0;

    if (strchr (number, '.'))
        dec_stop = (int) (strchr (number, '.') - (char *) number);
    else
        dec_stop = strlen (number) - decs_wanted;

    if (dec_stop < 1)
      {
        conv_reason = CONV_ERR_DECS_MISSING;
        return (NULL);                  /*  Error - too few decimals         */
      }

    /*  Prepare for thousands-separators if FLAG_N_THOUSANDS                 */
    if ((flags & FLAG_N_THOUSANDS) && !(flags & FLAG_N_ZERO_FILL))
      {
        /*  Get number of whole digits, allowing for decimals & dec sign     */
        sep_char = (char) (dec_point == '.'? ',': '.');
        sep_stop = (dec_stop - (decs_wanted? decs_wanted + 1: 0)) % 3;
        if (sep_stop == 0)
            sep_stop = 3;               /*  Get into range 1..3              */
      }
    else
      {
        sep_char = ' ';
        sep_stop = 0;                   /*  No thousands separators          */
      }

    /*  ---------------------------------   Copy the digits  ----------------*/

    digits    = 0;                      /*  No digits loaded yet             */
    decs_seen = 0;                      /*  No decimals output yet           */
    have_zero = TRUE;                   /*  Assume number is zero            */
    dest      = formatted;              /*  Format number                    */
    while (*number)                     /*    until we hit the terminator    */
      {
        ch = *number++;
        if (ch == '.')
            continue;                   /*  Ignore '.' in number             */

        digits++;

        if (ch == drop_zero && digits < dec_stop)
            ch = ' ';
        else
        if (isdigit (ch))
          {
            drop_zero = ' ';
            if (ch > '0')
                have_zero = FALSE;
          }
        if (ch != ' ' || (width > 0 && !(flags & FLAG_N_LEFT)))
          {
            *dest++ = ch;               /*  Output this digit                */
            if (digits > dec_stop)
                decs_seen++;            /*  Count the decimal digit          */
            else
            if (digits == dec_stop)     /*  Handle decimal stop              */
              {                         /*    with optional point            */
                if (flags & FLAG_N_DECIMALS)
                    *dest++ = dec_point;
                sep_stop = 0;           /*  And kill further thousand seps   */
              }
          }
        /*  Output thousands separator unless we are in blank area           */
        if (digits == sep_stop)
          {
            if (ch != ' ')
                *dest++ = sep_char;
            sep_stop += 3;
          }
      }
    *dest = 0;                          /*  Terminate the string nicely      */
    /*  ---------------------------------   Post-format the result  ---------*/

    if (decs_wanted > 0)
      {
        /*  Output trailing decimal zeroes if not supplied                   */
        if (decs_seen == 0)
            *dest++ = dec_point;
        while (decs_seen < decs_wanted)
          {
            *dest++ = '0';
            decs_seen++;
          }
        /*  Drop all decimals if format is DEC_HIDE_ALL                      */
        if (dec_format == DECS_HIDE_ALL)
            while (*dest != dec_point)
                dest--;                 /*  Drop-off trailing zero           */
        else
        /*  Drop trailing decimal zeroes if format is DEC_DROP_ZEROS         */
        if (dec_format == DECS_DROP_ZEROS)
            while (*dest != dec_point)
              {
                if (*(dest - 1) > '0')
                    break;
                else
                    dest--;             /*  Drop-off trailing zero           */
              }
        *dest = 0;                      /*  Terminate the string nicely      */
      }

    /*  Justify within width if width > 0                                    */
    sign_pos = 0;                       /*  Sign normally comes at start     */
    digits   = strlen (formatted);
    if (flags & FLAG_N_SIGNED)
      {
        digits++;                       /*  Allow for eventual sign          */
        if (sign_format == SIGN_FINANCIAL)
            digits++;                   /*  Sign shown like (123)            */
      }
    while (digits < width)
      {
        if (flags & FLAG_N_LEFT && !(flags & FLAG_N_ZERO_FILL))
            strcat (formatted, " ");
        else
          {
            stropen (formatted, FALSE); /*  Insert blank at start of string  */
            if (flags & FLAG_N_ZERO_FILL)
                formatted [0] = '0';
            else
                sign_pos++;             /*  Skip leading space               */
          }
        digits++;
      }

    /*  Format sign if FLAG_N_SIGNED                                         */
    if (flags & FLAG_N_SIGNED)
      {
        if (sign_format == SIGN_NEG_LEAD
        ||  sign_format == SIGN_ALL_LEAD
        ||  sign_format == SIGN_FINANCIAL)
            stropen (formatted, FALSE);

        if (sign_format == SIGN_NEG_TRAIL
        ||  sign_format == SIGN_ALL_TRAIL
        ||  sign_format == SIGN_FINANCIAL)
            strcat (formatted, " ");

        if (!have_zero)                 /*  Zero has no sign                 */
            switch (sign_format)
              {
                case SIGN_NEG_LEAD:
                    if (sign_char != '-')
                        break;          /*  Fall through if negative sign    */
                case SIGN_ALL_LEAD:
                    formatted [sign_pos] = sign_char;
                    break;

                case SIGN_NEG_TRAIL:
                    if (sign_char != '-')
                        break;          /*  Fall through if negative sign    */
                case SIGN_ALL_TRAIL:
                    strlast (formatted) = sign_char;
                    break;

                case SIGN_FINANCIAL:
                    if (sign_char == '-')
                      {
                        formatted [0]       = '(';
                        strlast (formatted) = ')';
                      }
                    break;
              }
      }

    /*  If all zeroes, return a blank string if FLAG_N_ZERO_BLANK            */
    if ((flags & FLAG_N_ZERO_BLANK) && have_zero)
      {
        memset (formatted, ' ', width);
        formatted [width] = 0;
      }

    if (width > 0 && (strlen (formatted) > (size_t) width))
      {
        conv_reason = CONV_ERR_NUM_OVERFLOW;
        return (NULL);                  /*  Overflow -- number too large     */
      }
    else
        return (formatted);
}
