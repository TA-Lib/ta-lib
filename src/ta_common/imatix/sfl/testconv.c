/*  ----------------------------------------------------------------<Prolog>-
    Name:       testconv.c
    Title:      Test program for conversion functions
    Package:    Standard Function Library (SFL)

    Written:    1995/12/18  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   testconv rams a set of test data through the SFL conversion
                functions, with the intention of quickly running a wide set
                of tests.  The input is supplied by a file (testconv.dat)
                whose format you should study & respect if you want to add
                specific tests.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"                        /*  SFL library header file          */

static char *get_token    (char *line, int *offset);
static void  test_btos    (char *line);
static void  test_stob    (char *line);
static void  test_dtop    (char *line);
static void  test_dtos    (char *line);
static void  test_stod    (char *line);
static void  test_ttop    (char *line);
static void  test_ttos    (char *line);
static void  test_stot    (char *line);
static void  test_ntos    (char *line);
static void  test_ston    (char *line);

int main (int argc, char *argv [])
{
    FILE
        *testfile;                      /*  Read input from driver file      */
    char
        curline [LINE_MAX + 1],
        *token,
        *filter;                        /*  Points to filter or NULL         */
    int
        offset;

    if (argc == 1)
      {
        puts ("Usage: 'testconv filename [filter]' where filename contains");
        puts ("lines following this format\n");
        puts ("! comment");
        puts ("> echo text");
        puts ("@ btos 1/0  YESNO|TRUEFALSE|YN|TF|10 ");
        puts ("@ stob string");
        puts ("@ dtop yyyymmdd picture");
        puts ("@ dtos yyyymmdd YMD|DMY|MDY format flags width");
        puts ("  Format: YMD YMD_ YMD/ YMD, YM YM_ YM/ MD MD_ MD/");
        puts ("  Flags: D=dd-d M=mm-m a=Month c=cent u=upper y m d");
        puts ("@ stod datestr YMD|DMY|MDY format flags");
        puts ("@ ttos hhmmsscc picture");
        puts ("@ stot timestr flags width");
        puts ("  Flags: h=hh-h m=mm-m s=ss-s c=cc-s /=compact p=12-hr");
        puts ("@ ntos +00012345 flags decs ./, decfmt signfmt width");
        puts ("  Flags: -=sign .=dec <=left 0=0pad B=0blank ,=thousands");
        puts ("  Decfmt: 1.10 1.1 1  Signfmt: n- n+ -n +n (n)");
        puts ("@ ston string flags decs ./, decfmt width");
        puts ("\nfilter is @ command, eg. dtos");
        exit (0);
      }
    testfile = file_open (argv [1], 'r');
    if (testfile == NULL)
      {
        printf ("Input file '%s' not found\n", argv [1]);
        exit (1);
      }
    if (argc > 2)
        filter = argv [2];
    else
        filter = NULL;

    while (file_read (testfile, curline))
      {
        switch (*curline)
          {
            case ' ':                   /*  Comment - ignore                 */
            case 0:
            case '!':
                break;

            case '>':                   /*  Echo line to output              */
                puts (curline);
                break;

            case '@':                   /*  Call test function               */
                offset = 1;
                token = get_token (curline, &offset);
                offset++;
                if (streq (token, "btos"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_btos (curline + offset);
                  }
                else
                if (streq (token, "stob"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_stob (curline + offset);
                  }
                else
                if (streq (token, "dtop"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_dtop (curline + offset);
                  }
                else
                if (streq (token, "dtos"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_dtos (curline + offset);
                  }
                else
                if (streq (token, "stod"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_stod (curline + offset);
                  }
                else
                if (streq (token, "ttop"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_ttop (curline + offset);
                  }
                else
                if (streq (token, "ttos"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_ttos (curline + offset);
                  }
                else
                if (streq (token, "stot"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_stot (curline + offset);
                  }
                else
                if (streq (token, "ntos"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_ntos (curline + offset);
                  }
                else
                if (streq (token, "ston"))
                  {
                    if (filter == NULL || streq (filter, token))
                        test_ston (curline + offset);
                  }
                else
                    printf ("Invalid @ action: %s\n", token);
                break;

            default:
                printf ("Invalid line: %s\n", curline);
          }
      }
    file_close (testfile);
    mem_assert ();

    return (EXIT_SUCCESS);
}


/*---------------------------------------------------------------------------
 *  Finds next token in line, blank delimited.  Returns pointer to token,
 *  which is in a static area.  Updates offset into line to point to char
 *  after token (space or null).
 */

static char *
get_token (char *line, int *offset)
{
    static char
        token [LINE_MAX + 1];
    int
        token_size;

    while (line [*offset] == ' ')       /*  Skip leading spaces              */
        (*offset)++;

    for (token_size = 0; line [*offset] > ' '; (*offset)++)
        if (line [*offset] == '_')
            token [token_size++] = ' ';
        else
            token [token_size++] = line [*offset];

    token [token_size] = 0;             /*  Delimit token nicely             */
    return (token);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_bool_str; format is:
 *
 *  bool format
 */

static void
test_btos (char *line)
{
    Bool
        bool;
    int
        offset = 0,
        format_val;
    char
        *format,
        *scan,
        *result;

    bool    = atoi       (get_token (line, &offset));
    format  = mem_strdup (get_token (line, &offset));

    format_val = streq (format, "YESNO")?     BOOL_YES_NO:
                 streq (format, "YN")?        BOOL_Y_N:
                 streq (format, "TRUEFALSE")? BOOL_TRUE_FALSE:
                 streq (format, "TF")?        BOOL_T_F:
                 streq (format, "10")?        BOOL_1_0:
                 /*  else  */                 0;

    result = conv_bool_str (bool, format_val);
    if (result == NULL)
        result = "<Error>";

    /*  Replace spaces by underlines for visibility                          */
    for (scan = result; *scan; scan++)
        if (*scan == ' ')
            *scan = '_';

    printf (" btos: %-55s", line);
    printf ("   => %s\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (format);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_str_bool; format is:
 *
 *  boolstr
 */

static void
test_stob (char *line)
{
    int
        result;
    int
        offset = 0;
    char
        *string;

    string = get_token (line, &offset);

    result = conv_str_bool (string);
    printf (" stob: %-55s", line);
    printf ("   => %d\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_date_pict; format is:
 *
 *  yyyymmdd picture
 */

static void
test_dtop (char *line)
{
    long
        date;
    int
        offset = 0;
    char
        *picture,
        *scan,
        *result;

    date    = atol       (get_token (line, &offset));
    picture = mem_strdup (get_token (line, &offset));
    result  = conv_date_pict (date, picture);
    if (result == NULL)
        result = "<Error>";

    /*  Replace spaces by underlines for visibility                          */
    for (scan = result; *scan; scan++)
        if (*scan == ' ')
            *scan = '_';

    printf (" dtop: %-55s", line);
    printf ("   => %s\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (picture);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_date_str; format is:
 *
 *  yyyymmdd order format flags width
 */

static void
test_dtos (char *line)
{
    long
        date;
    int
        offset = 0,
        width,
        order_val,
        format_val;
    char
        *order,
        *format,
        *flags,
        *flags_ptr,
        *scan,
        *result;
    word
        flags_val;

    date    = atol       (get_token (line, &offset));
    order   = mem_strdup (get_token (line, &offset));
    format  = mem_strdup (get_token (line, &offset));
    flags   = mem_strdup (get_token (line, &offset));
    width   = atoi       (get_token (line, &offset));

    order_val = streq (order, "YMD")? DATE_ORDER_YMD:
                streq (order, "DMY")? DATE_ORDER_DMY:
                streq (order, "MDY")? DATE_ORDER_MDY:
                /*  else  */          0;

    flags_val = 0;
    for (flags_ptr = flags; *flags_ptr; flags_ptr++)
      {
        switch (*flags_ptr)
          {
            case 'D': flags_val |= FLAG_D_DD_AS_D;   break;
            case 'M': flags_val |= FLAG_D_MM_AS_M;   break;
            case 'a': flags_val |= FLAG_D_MONTH_ABC; break;
            case 'c': flags_val |= FLAG_D_CENTURY;   break;
            case 'u': flags_val |= FLAG_D_UPPER;     break;
            case 'y': flags_val |= FLAG_D_ORDER_YMD; break;
            case 'm': flags_val |= FLAG_D_ORDER_MDY; break;
            case 'd': flags_val |= FLAG_D_ORDER_DMY; break;
          }
      }
    format_val = streq (format, "YMD")?  DATE_YMD_COMPACT:
                 streq (format, "YMD/")? DATE_YMD_DELIM:
                 streq (format, "YMDb")? DATE_YMD_SPACE:
                 streq (format, "YMD,")? DATE_YMD_COMMA:
                 streq (format, "YM")?   DATE_YM_COMPACT:
                 streq (format, "YM/")?  DATE_YM_DELIM:
                 streq (format, "YMb")?  DATE_YM_SPACE:
                 streq (format, "MD")?   DATE_MD_COMPACT:
                 streq (format, "MD/")?  DATE_MD_DELIM:
                 streq (format, "MDb")?  DATE_MD_SPACE:
                 /*  else  */            0;

    result = conv_date_str (date, flags_val, format_val,
                            order_val, '-', width);
    if (result == NULL)
        result = "<Error>";

    /*  Replace spaces by underlines for visibility                          */
    for (scan = result; *scan; scan++)
        if (*scan == ' ')
            *scan = '_';

    printf (" dtos: %-55s", line);
    printf ("   => %s\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (order);
    mem_free (format);
    mem_free (flags);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_str_date; format is:
 *
 *  datestr order format flags
 */

static void
test_stod (char *line)
{
    long
        result;
    int
        offset = 0,
        order_val,
        format_val;
    char
        *string,
        *order,
        *format,
        *flags,
        *flags_ptr;
    word
        flags_val;

    string  = mem_strdup (get_token (line, &offset));
    order   = mem_strdup (get_token (line, &offset));
    format  = mem_strdup (get_token (line, &offset));
    flags   = mem_strdup (get_token (line, &offset));

    order_val = streq (order, "YMD")? DATE_ORDER_YMD:
                streq (order, "DMY")? DATE_ORDER_DMY:
                streq (order, "MDY")? DATE_ORDER_MDY:
                /*  else  */          0;

    flags_val = 0;
    for (flags_ptr = flags; *flags_ptr; flags_ptr++)
      {
        switch (*flags_ptr)
          {
            case 'D': flags_val |= FLAG_D_DD_AS_D;   break;
            case 'M': flags_val |= FLAG_D_MM_AS_M;   break;
            case 'a': flags_val |= FLAG_D_MONTH_ABC; break;
            case 'c': flags_val |= FLAG_D_CENTURY;   break;
            case 'u': flags_val |= FLAG_D_UPPER;     break;
            case 'y': flags_val |= FLAG_D_ORDER_YMD; break;
            case 'm': flags_val |= FLAG_D_ORDER_MDY; break;
            case 'd': flags_val |= FLAG_D_ORDER_DMY; break;
          }
      }
    format_val = streq (format, "YMD")?  DATE_YMD_COMPACT:
                 streq (format, "YMD/")? DATE_YMD_DELIM:
                 streq (format, "YMDb")? DATE_YMD_SPACE:
                 streq (format, "YMD,")? DATE_YMD_COMMA:
                 streq (format, "YM")?   DATE_YM_COMPACT:
                 streq (format, "YM/")?  DATE_YM_DELIM:
                 streq (format, "YMb")?  DATE_YM_SPACE:
                 streq (format, "MD")?   DATE_MD_COMPACT:
                 streq (format, "MD/")?  DATE_MD_DELIM:
                 streq (format, "MDb")?  DATE_MD_SPACE:
                 /*  else  */            0;

    result = conv_str_date (string, flags_val, format_val, order_val);
    printf (" stod: %-55s", line);
    printf ("   => %ld\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (string);
    mem_free (order);
    mem_free (format);
    mem_free (flags);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_time_pict; format is:
 *
 *  hhmmsscc picture
 */

static void
test_ttop (char *line)
{
    long
        time;
    int
        offset = 0;
    char
        *picture,
        *scan,
        *result;

    time    = atol       (get_token (line, &offset));
    picture = mem_strdup (get_token (line, &offset));
    result  = conv_time_pict (time, picture);
    if (result == NULL)
        result = "<Error>";

    /*  Replace spaces by underlines for visibility                          */
    for (scan = result; *scan; scan++)
        if (*scan == ' ')
            *scan = '_';

    printf (" dtop: %-55s", line);
    printf ("   => %s\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (picture);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_time_str; format is:
 *
 *  hhmmsscc flags width
 */

static void
test_ttos (char *line)
{
    long
        time;
    int
        offset = 0,
        width;
    char
        *flags,
        *flags_ptr,
        *scan,
        *result;
    word
        flags_val;

    time  = atol       (get_token (line, &offset));
    flags = mem_strdup (get_token (line, &offset));
    width = atoi       (get_token (line, &offset));

    flags_val = 0;
    for (flags_ptr = flags; *flags_ptr; flags_ptr++)
      {
        switch (*flags_ptr)
          {
            case 'h': flags_val |= FLAG_T_HH_AS_H;   break;
            case 'm': flags_val |= FLAG_T_MM_AS_M;   break;
            case 's': flags_val |= FLAG_T_SS_AS_S;   break;
            case 'c': flags_val |= FLAG_T_CC_AS_C;   break;
            case '/': flags_val |= FLAG_T_COMPACT;   break;
            case 'p': flags_val |= FLAG_T_12_HOUR;   break;
          }
      }
    result = conv_time_str (time, flags_val, ':', width);
    if (result == NULL)
        result = "<Error>";

    /*  Replace spaces by underlines for visibility                          */
    for (scan = result; *scan; scan++)
        if (*scan == ' ')
            *scan = '_';

    printf (" ttos: %-55s", line);
    printf ("   => %s\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (flags);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_str_time; format is:
 *
 *  timestr
 */

static void
test_stot (char *line)
{
    long
        result;
    int
        offset = 0;
    char
        *string;

    string = get_token (line, &offset);

    result = conv_str_time (string);
    printf (" stot: %-55s", line);
    printf ("   => %ld\n", result);

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_number_str; format is:
 *
 *  +00012345 {-.<0B,} decs ./, 1.10|1.1|1 n-|n+|-n|+n|(n) width
 */

static void
test_ntos (char *line)
{
    int
        offset = 0,
        decimals,
        decfmt_val,
        negfmt_val,
        width;
    word
        flags_val;
    char
        *number,
        *result,
        *flags,
        *flags_ptr,
        decpoint,
        *scan,
        *decfmt,
        *negfmt;

    number    = mem_strdup (get_token (line, &offset));
    flags     = mem_strdup (get_token (line, &offset));
    decimals  = atoi       (get_token (line, &offset));
    decpoint  =            *get_token (line, &offset);
    decfmt    = mem_strdup (get_token (line, &offset));
    negfmt    = mem_strdup (get_token (line, &offset));
    width     = atoi       (get_token (line, &offset));

    flags_val = 0;
    for (flags_ptr = flags; *flags_ptr; flags_ptr++)
      {
        switch (*flags_ptr)
          {
            case '-': flags_val += FLAG_N_SIGNED;     break;
            case '.': flags_val += FLAG_N_DECIMALS;   break;
            case '<': flags_val += FLAG_N_LEFT;       break;
            case '0': flags_val += FLAG_N_ZERO_FILL;  break;
            case 'B': flags_val += FLAG_N_ZERO_BLANK; break;
            case ',': flags_val += FLAG_N_THOUSANDS;  break;
          }
      }
    negfmt_val = streq (negfmt, "n-")  ? SIGN_NEG_TRAIL:
                 streq (negfmt, "n+")  ? SIGN_ALL_TRAIL:
                 streq (negfmt, "-n")  ? SIGN_NEG_LEAD:
                 streq (negfmt, "+n")  ? SIGN_ALL_LEAD:
                 streq (negfmt, "(n)") ? SIGN_FINANCIAL:
                 /*  else  */          0;

    decfmt_val = streq (decfmt, "1.10") ? DECS_SHOW_ALL:
                 streq (decfmt, "1.1")  ? DECS_DROP_ZEROS:
                 streq (decfmt, "1")    ? DECS_HIDE_ALL:
                 /*  else  */           0;

    printf (" ntos: %-55s", line);
    result = conv_number_str (number, flags_val, decpoint,
                              decimals, decfmt_val, width, negfmt_val);

    /*  Replace spaces by underlines for visibility                          */
    if (result)
      {
        for (scan = result; *scan; scan++)
            if (*scan == ' ')
                *scan = '_';

        printf ("   => %s\n", result);
      }
    else
        printf ("   => (null)\n");

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (number);
    mem_free (flags);
    mem_free (decfmt);
    mem_free (negfmt);
}


/*---------------------------------------------------------------------------
 *  Run test through conv_str_number; format is:
 *
 *  +00012345 {-.<0B,} decs ./, 1.10|1.1|1 width
 */

static void
test_ston (char *line)
{
    int
        offset = 0,
        decimals,
        decfmt_val,
        width;
    word
        flags_val;
    char
        *result,
        *scan,
        *string,
        *flags,
        *flags_ptr,
        decpoint,
        *decfmt;

    string    = mem_strdup (get_token (line, &offset));
    flags     = mem_strdup (get_token (line, &offset));
    decimals  = atoi       (get_token (line, &offset));
    decpoint  =            *get_token (line, &offset);
    decfmt    = mem_strdup (get_token (line, &offset));
    width     = atoi       (get_token (line, &offset));

    flags_val = 0;
    for (flags_ptr = flags; *flags_ptr; flags_ptr++)
      {
        switch (*flags_ptr)
          {
            case '-': flags_val += FLAG_N_SIGNED;     break;
            case '.': flags_val += FLAG_N_DECIMALS;   break;
            case '<': flags_val += FLAG_N_LEFT;       break;
            case '0': flags_val += FLAG_N_ZERO_FILL;  break;
            case 'B': flags_val += FLAG_N_ZERO_BLANK; break;
            case ',': flags_val += FLAG_N_THOUSANDS;  break;
          }
      }
    decfmt_val = streq (decfmt, "1.10") ? DECS_SHOW_ALL:
                 streq (decfmt, "1.1")  ? DECS_DROP_ZEROS:
                 streq (decfmt, "1")    ? DECS_HIDE_ALL:
                 /*  else  */           0;

    printf (" ston: %-55s", line);
    result = conv_str_number (string, flags_val, decpoint,
                              decimals, decfmt_val, width);

    /*  Replace spaces by underlines for visibility                          */
    if (result)
      {
        for (scan = result; *scan; scan++)
            if (*scan == ' ')
                *scan = '_';

        printf ("   => %s\n", result);
      }
    else
        printf ("   => (null)\n");

    if (conv_reason)
        printf ("Error: %s\n", conv_reason_text [conv_reason]);

    mem_free (string);
    mem_free (flags);
    mem_free (decfmt);
}
