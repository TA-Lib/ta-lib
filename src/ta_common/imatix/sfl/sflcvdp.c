/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcvdp.c
    Title:      Converts a date to a string using a picture
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
#include "sfldate.h"                    /*  Date/time library functions      */
#include "sflstr.h"                     /*  String-handling functions        */


/*  ---------------------------------------------------------------------[<]-
    Function: conv_date_pict

    Synopsis: Converts a date to a string using a picture.  The picture is
    composed of any combination of these formats:
    <TABLE>
        cc        century 2 digits, 01-99
        y         day of year, 1-366
        yy        year 2 digits, 00-99
        yyyy      year 4 digits, 100-9999
        m         month, 1-12
        mm        month, 01-12
        mmm       month, 3 letters
        mmmm      month, full name
        MMM       month, 3 letters, ucase
        MMMM      month, full name, ucase
        d         day, 1-31
        dd        day, 01-31
        ddd       day of week, Sun-Sat
        dddd      day of week, Sunday-Saturday
        DDD       day of week, SUN-SAT
        DDDD      day of week, SUNDAY-SATURDAY
        w         day of week, 1-7 (1=Sunday)
        ww        week of year, 1-53
        q         year quarter, 1-4
        \x        literal character x
        other     literal character
    </TABLE>

    Returns the formatted result.  This is a static string, of at most 80
    characters, that is overwritten by each call.  If date is zero, returns
    an empty string.  The 'm' and 'd' formats output a leading space when
    used at the start of the picture.  This is to improve alignment of
    columns of dates.  The 'm' and 'd' formats also output a space when the
    previous character was a digit; otherwise the date components stick
    together and are illegible.

    Examples:
    puts (conv_date_pict (19951202, "mm d, yy"));
        Dec 2, 95
    puts (conv_date_pict (19951202, "d mmm, yy"));
        2 Dec, 95
    ---------------------------------------------------------------------[>]-*/

char *
conv_date_pict (
    long date,
    const char *picture)
{
    static char
        *month_name [] =
          {
            "January", "February", "March", "April", "May", "June", "July",
            "August", "September", "October", "November", "December"
          },
        *day_name [] =
          {
            "Sunday", "Monday", "Tuesday", "Wednesday",
            "Thursday", "Friday", "Saturday"
          },
        formatted [FORMAT_MAX + 1];     /*  Formatted return string          */
    int
        century,                        /*  Century component of date        */
        year,                           /*  Year component of date           */
        month,                          /*  Month component of date          */
        day,                            /*  Day component of date            */
        cursize;                        /*  Size of current component        */
    char
       *dest,                           /*  Store formatted data here        */
        ch,                             /*  Next character in picture        */
        lastch = '0';                   /*  Last character we output         */
    long
        full_date = date;

    conv_reason = 0;                    /*  No conversion errors so far      */

    /*  Zero date is returned as empty string                                */
    if (date == 0)
      {
        strclr (formatted);
        return (formatted);
      }

    default_century (&full_date);
    century = GET_CENTURY (full_date);
    year    = GET_YEAR    (full_date);
    month   = GET_MONTH   (full_date);
    day     = GET_DAY     (full_date);

    ASSERT (month > 0 && month <= 12);
    ASSERT (day   > 0 && day   <= 31);

    /*  Scan through picture, converting each component                      */
    dest = formatted;
    *dest = 0;                          /*  string is empty                  */
    while (*picture)
      {
        /*  Get character and count number of occurences                     */
        ch = *picture++;
        for (cursize = 1; *picture == ch; cursize++)
            picture++;

        switch (ch)
          {
            /*  cc        century 2 digits, 01-99                            */
            case 'c':
                if (cursize == 2)
                    sprintf (dest, "%02d", century);
                break;

            /*  y         day of year, 1-366                                 */
            /*  yy        year 2 digits, 00-99                               */
            /*  yyyy      year 4 digits, 0100-9999                           */
            case 'y':                   /*  y = day of year                  */
                if (cursize == 1)
                    sprintf (dest, "%d", julian_date (full_date));
                else
                if (cursize == 2)
                    sprintf (dest, "%02d", year);
                else
                if (cursize == 4)
                    sprintf (dest, "%02d%02d", century, year);
                break;

            /*  m         month, 1-12                                        */
            /*  mm        month, 01-12                                       */
            /*  mmm       month, 3 letters                                   */
            /*  mmmm      month, full name                                   */
            case 'm':
                if (cursize == 1)
                    sprintf (dest, (isdigit (lastch)? "%2d": "%d"), month);
                else
                if (cursize == 2)
                    sprintf (dest, "%02d", month);
                else
                if (cursize == 3)
                  {
                    memcpy (dest, month_name [month - 1], 3);
                    dest [3] = 0;
                  }
                else
                if (cursize == 4)
                    strcpy (dest, month_name [month - 1]);
                break;

            /*  MMM       month, 3-letters, ucase                            */
            /*  MMMM      month, full name, ucase                            */
            case 'M':
                if (cursize == 3)
                  {
                    memcpy (dest, month_name [month - 1], 3);
                    dest [3] = 0;
                    strupc (dest);
                  }
                else
                if (cursize == 4)
                  {
                    strcpy (dest, month_name [month - 1]);
                    strupc (dest);
                  }
                break;

            /*  d         day, 1-31                                          */
            /*  dd        day, 01-31                                         */
            /*  ddd       day of week, Sun-Sat                               */
            /*  dddd      day of week, Sunday-Saturday                       */
            case 'd':
                if (cursize == 1)
                    sprintf (dest, (isdigit (lastch)? "%2d": "%d"), day);
                else
                if (cursize == 2)
                    sprintf (dest, "%02d", day);
                else
                if (cursize == 3)
                  {
                    memcpy (dest, day_name [day_of_week (full_date)], 3);
                    dest [3] = 0;
                  }
                else
                if (cursize == 4)
                    strcpy (dest, day_name [day_of_week (full_date)]);
                break;

            /*  DDD       day of week, SUN-SAT                               */
            /*  DDDD      day of week, SUNDAY-SATURDAY                       */
            case 'D':
                if (cursize == 3)
                  {
                    memcpy (dest, day_name [day_of_week (full_date)], 3);
                    dest [3] = 0;
                    strupc (dest);
                  }
                else
                if (cursize == 4)
                  {
                    strcpy (dest, day_name [day_of_week (full_date)]);
                    strupc (dest);
                  }
                break;

            /*  w         day of week, 1-7 (1=Sunday)                        */
            /*  ww        week of year, 1-53                                 */
            case 'w':
                if (cursize == 1)
                    sprintf (dest, "%d", day_of_week (full_date) + 1);
                else
                if (cursize == 2)
                    sprintf (dest, "%d", week_of_year (full_date));
                break;

            /*  q         year quarter, 1-4                                  */
            case 'q':
                if (cursize == 1)
                    sprintf (dest, "%d", year_quarter (full_date));
                break;

            /*  \x        literal character x                                */
            case '\\':
                ch = *picture++;
        }
        if (*dest)                      /*  If something was output,         */
            while (*dest)               /*    skip to end of string          */
                dest++;
        else
            while (cursize--)           /*  Else output ch once or more      */
                *dest++ = ch;           /*    and bump dest pointer          */

        lastch = *(dest - 1);           /*  Get previous character           */
        *dest = 0;                      /*  Terminate the string nicely      */
    }
    return (formatted);
}
