/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcvsd.c
    Title:      Converts a string to a date
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
#include "sfldate.h"                    /*  Date library functions           */
#include "sflstr.h"                     /*  String functions                 */


/*  ---------------------------------------------------------------------[<]-
    Function: conv_str_date

    Synopsis: Converts a string to a date.  The supposed format of the
    date is defined by the format argument, which can be one of:
    <TABLE>
        DATE_YMD_COMPACT    Year month day.
        DATE_YMD_DELIM      Year month day.
        DATE_YMD_SPACE      Year month day.
        DATE_YMD_COMMA      Year month day.
        DATE_YM_COMPACT     Year and month only; day is zero.
        DATE_YM_DELIM       Year and month only; day is zero.
        DATE_YM_SPACE       Year and month only; day is zero.
        DATE_MD_COMPACT     Month and day only; year is zero.
        DATE_MD_DELIM       Month and day only; year is zero.
        DATE_MD_SPACE       Month and day only; year is zero.
    </TABLE>

    The date order must be one of:
    <TABLE>
        DATE_ORDER_YMD      Year month day.
        DATE_ORDER_DMY      Day month year.
        DATE_ORDER_MDY      Month day year.
    </TABLE>

    You can override the order using these flags:
    <TABLE>
        FLAG_D_ORDER_YMD    Year month day.
        FLAG_D_ORDER_DMY    Day month year.
        FLAG_D_ORDER_MDY    Month day year.
    </TABLE>

    Returns the date as a long integer, YYYYMMDD.  The conversion is pretty
    relaxed and allows strings like: 010195, 1-1-95, 1-Jan-95, 1jan95,
    01jan95, 1 1 95, etc.  The input string must be null-terminated.
    Returns -1 in case of an invalid date or format. If the short formats
    (DATE_DM_..., DATE_YM_...) are used, the missing field is always set
    to 0 and cannot be supplied in the string.  If the date was empty, i.e.
    contains no usable digits, returns 0.
    ---------------------------------------------------------------------[>]-*/

long
conv_str_date (
    const char *string,
    int  flags,
    int  format,
    int  order)
{
    static
        char *month_name [] =
          {
            "jan", "feb", "mar", "apr", "may", "jun",
            "jul", "aug", "sep", "oct", "nov", "dec"
          };

    static byte
        month_days [] =
          {
            31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
          },
        order_ptr [][3] =               /*  Year, Month, Day pointers        */
          {                             /*    for various orders & formats   */
          /*  YY MM DD   YYYY MM DD     YY MM       YYYY MM            MM DD */
            { 0, 2, 4 }, { 0, 4, 6 }, { 0, 2, 99 }, { 0, 4, 99 }, { 99, 0, 2 },
            { 4, 2, 0 }, { 4, 2, 0 }, { 2, 0, 99 }, { 2, 0, 99 }, { 99, 2, 0 },
            { 4, 0, 2 }, { 4, 0, 2 }, { 2, 0, 99 }, { 2, 0, 99 }, { 99, 0, 2 },
          };

    char
        date_digits   [9],              /*  8 digits of date                 */
        month_letters [4] = "???",      /*  3 characters of month            */
        ch;                             /*  Next character in date           */
    long
        feedback;                       /*  Returned date; -1 = error        */
    int
        digits,                         /*  Number of digits in string       */
        delimiters,                     /*  Number of delimiters in date     */
        digitseq,                       /*  Number of digits in sequence     */
        count,                          /*  Number of month letters          */
        year,                           /*  Date year value                  */
        month,                          /*  Date month value                 */
        day,                            /*  Date day value                   */
        order_index,                    /*  Index into order table           */
        y_ptr,                          /*  Where is year in date?           */
        m_ptr,                          /*  Where is month in date?          */
        d_ptr,                          /*  Where is day in date?            */
        date_order = order;             /*  Actual date order                */
    Bool
        had_month;                      /*  Did we already get a month?      */

    ASSERT (format >= DATE_FORMAT_FIRST && format <= DATE_FORMAT_LAST);
    ASSERT (order  >= DATE_ORDER_FIRST  && order  <= DATE_ORDER_LAST);

    conv_reason = 0;                    /*  No conversion errors so far      */
    if (flags & FLAG_D_ORDER_YMD)
        date_order = DATE_ORDER_YMD;
    else
    if (flags & FLAG_D_ORDER_DMY)
        date_order = DATE_ORDER_DMY;
    else
    if (flags & FLAG_D_ORDER_MDY)
        date_order = DATE_ORDER_MDY;

    /*  Collect date digits                                                  */
    digits     = 0;                     /*  Nothing collected so far         */
    digitseq   = 0;                     /*  No digits in sequence            */
    feedback   = 0;                     /*  No errors so far                 */
    delimiters = 0;                     /*  We allow up to 2 delimiters      */
    had_month  = FALSE;                 /*  True after 3-letter month seen   */

    do
      {
        ch = *string++;
        if (isdigit (ch))
          {
            if (digits < 8)
              {
                digitseq++;
                date_digits [digits++] = ch;
              }
            else
              {
                conv_reason = CONV_ERR_DATE_SIZE;
                feedback = -1;          /*  Too many digits                  */
              }
          }
        else
          {
            /*  Fill-up to even number of digits                             */
            if (digits > (digits / 2) * 2)
              {
                date_digits [digits] = date_digits [digits - 1];
                date_digits [digits - 1] = '0';
                digits++;
              }
            /*  3 or 5 in a row is not allowed                               */
            if (digitseq == 3 || digitseq == 5)
              {
                conv_reason = CONV_ERR_REJECT_3_5;
                feedback = -1;
              }
            digitseq = 0;

            /*  If a letter, try to match against a month                    */
            if (isalpha (ch))
              {
                if (had_month)
                  {
                    conv_reason = CONV_ERR_MULTIPLE_MONTH;
                    feedback = -1;
                  }
                else
                  {
                    for (count = 0; isalpha (ch); )
                      {
                        if (count < 3)
                            month_letters [count++] = (char) tolower (ch);
                        ch = *string++;
                      }
                    string--;           /*  Move back to char after month    */
                    if (count < 3)
                      {
                        conv_reason = CONV_ERR_BAD_MONTH;
                        feedback = -1;  /*  Too few letters                  */
                      }
                    month_letters [3] = 0;
                    for (count = 0; count < 12; count++)
                        if (streq (month_letters, month_name [count]))
                          {
                            count++;
                            date_digits [digits++] = (char) (count / 10 + '0');
                            date_digits [digits++] = (char) (count % 10 + '0');
                            had_month = TRUE;
                            break;
                          }
                    if (!had_month)
                      {
                        conv_reason = CONV_ERR_BAD_MONTH;
                        feedback = -1;  /*  Month not found                  */
                      }
                  }
              }
            else
            if (ispunct (ch))           /*  Skip any delimiter               */
                if ((++delimiters > 2)
                ||  (format > DATE_YMD_LAST && delimiters > 1))
                  {
                    conv_reason = CONV_ERR_MULTIPLE_DELIM;
                    feedback = -1;      /*  Multiple delimiters              */
                  }
          }
      }
    until (ch == 0);

    /*  Return zero date if empty                                            */
    if (digits == 0)
        return (feedback);

    /*  Calculate offset in date_digits for various date order & formats     */
    order_index = (date_order - 1) * 5; /*  Each row has 5 items             */

    if (format <= DATE_YMD_LAST)
      {
        if (digits == 6)
            ;   /* nothing */
        else
        if (digits == 8)
            order_index += 1;
      }
    else
    if (format <= DATE_YM_LAST)
      {
        date_digits [digits++] = '0';
        date_digits [digits++] = '0';
        if (digits == 6)
            order_index += 2;
        else
        if (digits == 8)
            order_index += 3;
        else
          {
            conv_reason = CONV_ERR_DATE_SIZE;
            return (-1);                /*  Error - bad date size            */
          }
      }
    else
    if (format <= DATE_MD_LAST)
      {
        date_digits [digits++] = '0';
        date_digits [digits++] = '0';
        if (digits == 6)
            order_index += 4;
        else
          {
            conv_reason = CONV_ERR_DATE_SIZE;
            return (-1);                /*  Error - bad date size            */
          }
      }

    /*  Decode order to pick-up offset of day/month/year in date_digits      */
    y_ptr = order_ptr [order_index][0];
    m_ptr = order_ptr [order_index][1];
    d_ptr = order_ptr [order_index][2];

#   define DIGIT(x) (date_digits [(x)] - '0')
    if (y_ptr != 99)
      {
        year = DIGIT (y_ptr) * 10 + DIGIT (y_ptr + 1);
        if (digits == 8)
            year = DIGIT (y_ptr + 2) * 10 + DIGIT (y_ptr + 3) + year * 100;

        if (year < 50)
            year += 2000;
        else
        if (year < 100)
            year += 1900;
      }
    else
        year = 0;

    if (m_ptr != 99)
      {
        month = DIGIT (m_ptr) * 10 + DIGIT (m_ptr + 1);
        if (month == 0 || month > 12)
          {
            conv_reason = CONV_ERR_OUT_OF_RANGE;
            feedback = -1;
          }
      }
    else
        month = 0;

    if (d_ptr != 99)
      {
        day = DIGIT (d_ptr) * 10 + DIGIT (d_ptr + 1);
        if ((day == 0 || day > (int) month_days [month - 1])
        ||  (month == 2 && day == 29 && !leap_year (year)))
          {
            conv_reason = CONV_ERR_OUT_OF_RANGE;
            feedback = -1;
          }
      }
    else
        day = 0;

    if (feedback == 0)
        feedback = year * 10000L + month * 100 + day;

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: conv_str_day

    Synopsis: Converts a string to a day.  The string contains a day name
    in English.  Only the first three letters of the name are significant.
    Returns a value 0..6 for Sunday to Saturday, or -1 if the name is not
    a valid day.
    ---------------------------------------------------------------------[>]-*/

int
conv_str_day (
    const char *string)
{
    static
        char *day_name [] =
          { "sun", "mon", "tue", "wed", "thu", "fri", "sat" };
    int
        day;
    char
        day_comp [4];

    /*  Get up to three letters of day name and convert to lower case        */
    strncpy (day_comp, string, 3);
    day_comp [3] = '\0';
    strlwc (day_comp);

    /*  I don't like magic constants, but "7" is pretty safe for now         */
    for (day = 0; day < 7; day++)
        if (streq (day_name [day], day_comp))
            break;

    return (day >= 7? -1: day);
}
