/* Note: This file has been adapted for runnign within TA-LIB.
 * If you intend to use iMatix, please use their original file.
 */

/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfldate.c
    Title:      Date and time functions
    Package:    Standard Function Library (SFL)

    Written:    1996/01/06  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflprint.h"                   /*  snprintf functions               */
#include "sfldate.h"                    /*  Prototypes for functions         */
#include "ta_trace.h"

/*  ---------------------------------------------------------------------[<]-
    Function: date_now

    Synopsis: Returns the current date as a long value (CCYYMMDD).  Since
    most system clocks do not return a century, this function assumes that
    all years 80 and above are in the 20th century, and all years 00 to 79
    are in the 21st century.  For best results, consume before 1 Jan 2080.
    ---------------------------------------------------------------------[>]-*/

long
date_now (void)
{
    return (timer_to_date (time (NULL)));
}


/*  ---------------------------------------------------------------------[<]-
    Function: time_now

    Synopsis: Returns the current time as a long value (HHMMSSCC).  If the
    system clock does not return centiseconds, these are set to zero.
    ---------------------------------------------------------------------[>]-*/

long
time_now (void)
{
#if (defined (__TURBOC__))
    /*  The Turbo-C gettime() function returns just what we want             */
    struct time
        time_struct;

    gettime (&time_struct);
    return (MAKE_TIME (time_struct.ti_hour,
                       time_struct.ti_min,
                       time_struct.ti_sec,
                       time_struct.ti_hund));

#elif (defined (__UTYPE_FREEBSD__))
    return (timer_to_time (time (NULL)));

#elif (defined (__UNIX__) || defined (__VMS_XOPEN))
    /*  The BSD gettimeofday function returns seconds and microseconds       */
    struct timeval
        time_struct;

    gettimeofday (&time_struct, 0);
    return (timer_to_time (time_struct.tv_sec)
                         + time_struct.tv_usec / 10000);

#elif (defined (WIN32))
    /*  The Win32 GetLocalTime function returns just what we want            */
    SYSTEMTIME
        time_struct;

    GetLocalTime (&time_struct);
    return (MAKE_TIME (time_struct.wHour,
                       time_struct.wMinute,
                       time_struct.wSecond,
                       time_struct.wMilliseconds / 10));

#else
    /*  Otherwise, just get the time without milliseconds                    */
    return (timer_to_time (time (NULL)));
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: leap_year

    Synopsis: Returns TRUE if the year is a leap year.  You must supply a
    4-digit value for the year: 90 is taken to mean 90 ad.  Handles leap
    centuries correctly.
    ---------------------------------------------------------------------[>]-*/

Bool
leap_year (int year)
{
    return (Bool)((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: julian_date

    Synopsis: Returns the number of days since 31 December last year.  The
    Julian date of 1 January is 1.
    ---------------------------------------------------------------------[>]-*/

int
julian_date (long date)
{
    static int
        days [12] = {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };
    int
        julian;

    julian = days [GET_MONTH (date) - 1] + GET_DAY (date);
    if (GET_MONTH (date) > 2 && leap_year (GET_YEAR (date)))
        julian++;

    return (julian);
}


/*  ---------------------------------------------------------------------[<]-
    Function: day_of_week

    Synopsis: Returns the day of the week where 0 is Sunday, 1 is Monday,
    ... 6 is Saturday.  Uses Zeller's Congurence algorithm.
    ---------------------------------------------------------------------[>]-*/

int
day_of_week (long date)
{
    int
        year  = GET_CCYEAR (date),
        month = GET_MONTH  (date),
        day   = GET_DAY    (date);

    if (month > 2)
        month -= 2;
    else
      {
        month += 10;
        year--;
      }
    day = ((13 * month - 1) / 5) + day + (year % 100) +
          ((year % 100) / 4) + ((year / 100) / 4) - 2 *
           (year / 100) + 77;

    return (day - 7 * (day / 7));
}


/*  ---------------------------------------------------------------------[<]-
    Function: next_weekday

    Synopsis: Returns the date of the next weekday, skipping from Friday
    to Monday.
    ---------------------------------------------------------------------[>]-*/

long
next_weekday (long date)
{
    long
        days = date_to_days (date);

    if (day_of_week (date) == 5)        /*  Friday                           */
        days += 3;
    else
    if (day_of_week (date) == 6)        /*  Saturday                         */
        days += 2;
    else
        days += 1;                      /*  Sunday to Thursday               */

    return (days_to_date (days));
}


/*  ---------------------------------------------------------------------[<]-
    Function: prev_weekday

    Synopsis: Returns the date of the previous weekday, skipping from Monday
    to Friday.
    ---------------------------------------------------------------------[>]-*/

long
prev_weekday (long date)
{
    long
        days = date_to_days (date);

    if (day_of_week (date) == 1)        /*  Monday                           */
        days -= 3;
    else
    if (day_of_week (date) == 0)        /*  Sunday                           */
        days -= 2;
    else
        days -= 1;                      /*  Tuesday to Saturday              */

    return (days_to_date (days));
}


/*  ---------------------------------------------------------------------[<]-
    Function: week_of_year

    Synopsis: Returns the week of the year, where 1 is the first full week.
    Week 0 may or may not exist in any year.  Uses a Lillian date algorithm
    to calculate the week of the year.
    ---------------------------------------------------------------------[>]-*/

int
week_of_year (long date)
{
    long
        year = GET_CCYEAR (date) - 1501,
        day  = year * 365 + year / 4 - 29872L + 1
             - year / 100 + (year - 300) / 400;

    return ((julian_date (date) + (int) ((day + 4) % 7)) / 7);
}


/*  ---------------------------------------------------------------------[<]-
    Function: year_quarter

    Synopsis: Returns the year quarter, 1 to 4, depending on the month
    specified.
    ---------------------------------------------------------------------[>]-*/

int
year_quarter (long date)
{
    return ((GET_MONTH (date) - 1) / 3 + 1);
}


/*  ---------------------------------------------------------------------[<]-
    Function: default_century

    Synopsis: Supplies a default century for the year if necessary.  If
    the year is 51 to 99, the century is set to 19.  If the year is 0 to
    50, the century is set to 20.  Returns the adjusted date.
    ---------------------------------------------------------------------[>]-*/

long
default_century (long *date)
{
    if (GET_CENTURY (*date) == 0)
        *date += (GET_YEAR (*date) > 50? 19000000L: 20000000L);
    return (*date);
}


/*  ---------------------------------------------------------------------[<]-
    Function: pack_date

    Synopsis: Packs the date into a single unsigned short word.  Use this
    function to store dates when memory space is at a premium.  The packed
    date can be used correctly in comparisons.  Returns the packed date.
    The date must be later than 31 December 1979.
    ---------------------------------------------------------------------[>]-*/

word
pack_date (long date)
{
    return (word) (((GET_CCYEAR (date) - 1980) << 9) +
                    (GET_MONTH  (date) << 5) +
                     GET_DAY    (date));
}


/*  ---------------------------------------------------------------------[<]-
    Function: pack_time

    Synopsis: Packs the time into a single unsigned short word.  Use this
    function to store times when memory space is at a premium.  The packed
    time can be used correctly in comparisons.  Returns the packed time.
    Seconds are stored with 2-second accuracy and centiseconds are lost.
    ---------------------------------------------------------------------[>]-*/

word
pack_time (long time)
{
    return (word) ((GET_HOUR   (time) << 11) +
                   (GET_MINUTE (time) << 5)  +
                   (GET_SECOND (time) >> 1));
}


/*  ---------------------------------------------------------------------[<]-
    Function: unpack_date

    Synopsis: Converts a packed date back into a long value.
    ---------------------------------------------------------------------[>]-*/

long
unpack_date (word packdate)
{
    int year;

    year = ((word) (packdate & 0xfe00) >> 9) + 80;
    return (MAKE_DATE (year > 80? 19: 20,
                       year,
                       (word) (packdate & 0x01e0) >> 5,
                       (word) (packdate & 0x001f)));
}


/*  ---------------------------------------------------------------------[<]-
    Function: unpack_time

    Synopsis: Converts a packed time back into a long value.
    ---------------------------------------------------------------------[>]-*/

long
unpack_time (word packtime)
{
    return (MAKE_TIME ((word) (packtime & 0xf800) >> 11,
                       (word) (packtime & 0x07e0) >> 5,
                       (word) (packtime & 0x001f) << 1, 0));
}


/*  ---------------------------------------------------------------------[<]-
    Function: date_to_days

    Synopsis: Converts the date into a number of days since a distant but
    unspecified epoch.  You can use this function to calculate differences
    between dates, and forward dates.  Use days_to_date() to calculate the
    reverse function.  Author: Robert G. Tantzen, translated from the Algol
    original in Collected Algorithms of the CACM (algorithm 199).  Original
    translation into C by Nat Howard, posted to Usenet 5 Jul 1985.
    ---------------------------------------------------------------------[>]-*/

long
date_to_days (long date)
{
    long
        year    = GET_YEAR    (date),
        century = GET_CENTURY (date),
        month   = GET_MONTH   (date),
        day     = GET_DAY     (date);

    if (month > 2)
        month -= 3;
    else
      {
        month += 9;
        if (year)
            year--;
        else
          {
            year = 99;
            century--;
          }
      }
    return ((146097L * century)    / 4L +
            (1461L   * year)       / 4L +
            (153L    * month + 2L) / 5L +
                       day   + 1721119L);
}


/*  ---------------------------------------------------------------------[<]-
    Function: days_to_date

    Synopsis: Converts a number of days since some distant but unspecified
    epoch into a date.  You can use this function to calculate differences
    between dates, and forward dates.  Use date_to_days() to calculate the
    reverse function.  Author: Robert G. Tantzen, translated from the Algol
    original in Collected Algorithms of the CACM (algorithm 199).  Original
    translation into C by Nat Howard, posted to Usenet 5 Jul 1985.
    ---------------------------------------------------------------------[>]-*/

long
days_to_date (long days)
{
    long
        century,
        year,
        month,
        day;

    days   -= 1721119L;
    century = (4L * days - 1L) / 146097L;
    days    =  4L * days - 1L  - 146097L * century;
    day     =  days / 4L;

    year    = (4L * day + 3L) / 1461L;
    day     =  4L * day + 3L  - 1461L * year;
    day     = (day + 4L) / 4L;

    month   = (5L * day - 3L) / 153L;
    day     =  5L * day - 3   - 153L * month;
    day     = (day + 5L) / 5L;

    if (month < 10)
        month += 3;
    else
      {
        month -= 9;
        if (year++ == 99)
          {
            year = 0;
            century++;
          }
      }
    return (MAKE_DATE (century, year, month, day));
}


/*  ---------------------------------------------------------------------[<]-
    Function: date_to_timer

    Synopsis: Converts the supplied date and time into a time_t timer value.
    This is the number of non-leap seconds since 00:00:00 GMT Jan 1, 1970.
    Function was rewritten by Bruce Walter <walter@fortean.com>.  If the
    input date and time are invalid, returns 0.
    ---------------------------------------------------------------------[>]-*/

time_t
date_to_timer (long date, long time)
{
    struct tm
        time_struct;
    time_t
        timer;

    time_struct.tm_sec   = GET_SECOND (time);
    time_struct.tm_min   = GET_MINUTE (time);
    time_struct.tm_hour  = GET_HOUR   (time);
    time_struct.tm_mday  = GET_DAY    (date);
    time_struct.tm_mon   = GET_MONTH  (date) - 1;
    time_struct.tm_year  = GET_CCYEAR (date) - 1900;
    time_struct.tm_isdst = -1;
    timer = mktime (&time_struct);
    if (timer == -1)
        return (0);
    else
        return (timer);
}


/*  ---------------------------------------------------------------------[<]-
    Function: timer_to_date

    Synopsis: Converts the supplied timer value into a long date value.
    Dates are stored as long values: CCYYMMDD.  If the supplied value is
    zero, returns zero.  The timer value is assumed to be UTC (GMT).
    ---------------------------------------------------------------------[>]-*/

long
timer_to_date (time_t time_secs)
{
    struct tm
        *time_struct;

    if (time_secs == 0)
        return (0);
    else
      {
        /*  Convert into a long value CCYYMMDD                               */
        time_struct = safe_localtime (&time_secs);
        time_struct-> tm_year += 1900;
        return (MAKE_DATE (time_struct-> tm_year / 100,
                           time_struct-> tm_year % 100,
                           time_struct-> tm_mon + 1,
                           time_struct-> tm_mday));
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: timer_to_time

    Synopsis: Converts the supplied timer value into a long time value.
    Times are stored as long values: HHMMSS00.  Since the timer value does
    not hold centiseconds, these are set to zero.  If the supplied value
    was zero, returns zero.  The timer value is assumed to be UTC (GMT).
    ---------------------------------------------------------------------[>]-*/

long
timer_to_time (time_t time_secs)
{
    struct tm
        *time_struct;

    if (time_secs == 0)
        return (0);
    else
      {
        /*  Convert into a long value HHMMSS00                               */
        time_struct = safe_localtime (&time_secs);
        return (MAKE_TIME (time_struct-> tm_hour,
                           time_struct-> tm_min,
                           time_struct-> tm_sec,
                           0));
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: timer_to_gmdate

    Synopsis: Converts the supplied timer value into a long date value in
    Greenwich Mean Time (GMT).  Dates are stored as long values: CCYYMMDD.
    If the supplied value is zero, returns zero.
    ---------------------------------------------------------------------[>]-*/

long
timer_to_gmdate (time_t time_secs)
{
    struct tm
        *time_struct;

    if (time_secs == 0)
        return (0);
    else
      {
        /*  Convert into a long value CCYYMMDD                               */
        time_struct = safe_gmtime (&time_secs);
        if (time_struct == NULL)        /*  If gmtime is not implemented     */
            time_struct = safe_localtime (&time_secs);

        time_struct-> tm_year += 1900;
        return (MAKE_DATE (time_struct-> tm_year / 100,
                           time_struct-> tm_year % 100,
                           time_struct-> tm_mon + 1,
                           time_struct-> tm_mday));
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: timer_to_gmtime

    Synopsis: Converts the supplied timer value into a long time value in
    Greenwich Mean Time (GMT).  Times are stored as long values: HHMMSS00.
    On most systems the clock does not return centiseconds, so these are
    set to zero.  If the supplied value is zero, returns zero.
    ---------------------------------------------------------------------[>]-*/

long
timer_to_gmtime (time_t time_secs)
{
    struct tm
        *time_struct;

    if (time_secs == 0)
        return (0);
    else
      {
        /*  Convert into a long value HHMMSS00                               */
        time_struct = safe_gmtime (&time_secs);
        if (time_struct == NULL)        /*  If gmtime is not implemented     */
            time_struct = safe_localtime (&time_secs);

        return (MAKE_TIME (time_struct-> tm_hour,
                           time_struct-> tm_min,
                           time_struct-> tm_sec,
                           0));
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: time_to_csecs

    Synopsis: Converts a time (HHMMSSCC) into a number of centiseconds.
    ---------------------------------------------------------------------[>]-*/

long
time_to_csecs (long time)
{
    return ((long) (GET_HOUR   (time) * (long) INTERVAL_HOUR)
          + (long) (GET_MINUTE (time) * (long) INTERVAL_MIN)
          + (long) (GET_SECOND (time) * (long) INTERVAL_SEC)
          + (long) (GET_CENTI  (time)));
}


/*  ---------------------------------------------------------------------[<]-
    Function: csecs_to_time

    Synopsis: Converts a number of centiseconds (< INTERVAL_DAY) into a
    time value (HHMMSSCC).
    ---------------------------------------------------------------------[>]-*/

long
csecs_to_time (long csecs)
{
    long
        hour,
        min,
        sec;

    /* ASSERT(csecs < INTERVAL_DAY, 0 ); */
    if( !(csecs < INTERVAL_DAY) )
       return 0;

    hour  = csecs / INTERVAL_HOUR;
    csecs = csecs % INTERVAL_HOUR;
    min   = csecs / INTERVAL_MIN;
    csecs = csecs % INTERVAL_MIN;
    sec   = csecs / INTERVAL_SEC;
    csecs = csecs % INTERVAL_SEC;
    return (MAKE_TIME (hour, min, sec, csecs));
}


/*  ---------------------------------------------------------------------[<]-
    Function: future_date

    Synopsis: Calculates a future date and time from the date and time
    specified, plus an interval specified in days and 1/100th seconds.
    The date can be any date since some distant epoch (around 1600).
    If the date and time arguments are both zero, the current date and
    time are used.  Either date and time arguments may be null.
    ---------------------------------------------------------------------[>]-*/

void
future_date (long *date, long *time, long days, long csecs)
{
    long
        dummy_date = 0,
        dummy_time = 0;

    if (date == NULL)
        date = &dummy_date;
    if (time == NULL)
        time = &dummy_time;

    /*  Set date and time to NOW if necessary                                */
    if (*date == 0 && *time == 0)
      {
        *date = date_now ();
        *time = time_now ();
      }

    /*  Get future date in days and centiseconds                             */
    days  = date_to_days  (*date) + days;
    csecs = time_to_csecs (*time) + csecs;

    /*  Normalise overflow in centiseconds                                   */
    while (csecs >= INTERVAL_DAY)
      {
        days++;
        csecs -= INTERVAL_DAY;
      }

    /*  Convert date and time back into organised values                     */
    *date = days_to_date  (days);
    *time = csecs_to_time (csecs);
}


/*  ---------------------------------------------------------------------[<]-
    Function: past_date

    Synopsis: Calculates a past date and time from the date and time
    specified, minus an interval specified in days and 1/100th seconds.
    The date can be any date since some distant epoch (around 1600).
    If the date and time arguments are both zero, the current date and
    time are used.  Either date and time arguments may be null.
    ---------------------------------------------------------------------[>]-*/

void
past_date (long *date, long *time, long days, long csecs)
{
    long
        dummy_date = 0,
        dummy_time = 0;

    if (date == NULL)
        date = &dummy_date;
    if (time == NULL)
        time = &dummy_time;

    /*  Set date and time to NOW if necessary                                */
    if (*date == 0 && *time == 0)
      {
        *date = date_now ();
        *time = time_now ();
      }
    /*  Get past date in days and centiseconds                               */
    days  = date_to_days  (*date) - days;
    csecs = time_to_csecs (*time) - csecs;

    /*  Normalise underflow in centiseconds                                  */
    while (csecs < 0)
      {
        days--;
        csecs += INTERVAL_DAY;
      }

    /*  Convert date and time back into organised values                     */
    *date = days_to_date  (days);
    *time = csecs_to_time (csecs);
}


/*  ---------------------------------------------------------------------[<]-
    Function: date_diff

    Synopsis: Calculates the difference between two date/time values, and
    returns the difference as a number of days and a number of centiseconds.
    The date can be any date since some distant epoch (around 1600).  The
    calculation is date1:time1 - date2:time2.  The returned values may be
    negative.
    ---------------------------------------------------------------------[>]-*/

void
date_diff (
    long date1, long time1,             /*  Date and time                    */
    long date2, long time2,             /*    minus this date and time       */
    long *days, long *csecs             /*  Gives these values               */
)
{
    *days  = date_to_days  (date1) - date_to_days  (date2);
    *csecs = time_to_csecs (time1) - time_to_csecs (time2);
}


/*  ---------------------------------------------------------------------[<]-
    Function: valid_date

    Synopsis: Returns TRUE if the date is valid or zero; returns FALSE if
    the date is not valid.
    ---------------------------------------------------------------------[>]-*/

Bool
valid_date (long date)
{
    int
        month,
        day;
    Bool
        feedback;
    static byte
        month_days [] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    month = GET_MONTH (date);
    day   = GET_DAY   (date);

    if (date == 0)
        feedback = TRUE;                /*  Zero date is okay                */
    else
    if (month < 1 || month > 12)
        feedback = FALSE;               /*  Month out of range               */
    else
    if ((day < 1 || day > month_days [month - 1])
    ||  (month == 2 && day == 29 && !leap_year (GET_YEAR (date))))
        feedback = FALSE;               /*  Day out of range                 */
    else
        feedback = TRUE;                /*  Zero date is okay                */

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: valid_time

    Synopsis: Returns TRUE if the time is valid or zero; returns FALSE if
    the time is not valid.
    ---------------------------------------------------------------------[>]-*/

Bool
valid_time (long time)
{
    return (Bool)(GET_SECOND (time) < 60
        &&  GET_MINUTE (time) < 60
        &&  GET_HOUR   (time) < 24);
}


/*  ---------------------------------------------------------------------[<]-
    Function: date_is_future

    Synopsis: Returns TRUE if the specified date and time are in the future.
    Returns FALSE if the date and time are in the past, or the present (which
    will be the past by the time you've read this).  Date is specified as a
    YYYYMMDD value; time as HHMMSSCC.
    ---------------------------------------------------------------------[>]-*/

Bool
date_is_future (long date, long time)
{
    return (Bool)(date  > date_now ()
        || (date == date_now () && time > time_now ()));
}


/*  ---------------------------------------------------------------------[<]-
    Function: date_is_past

    Synopsis: Returns TRUE if the specified date and time are in the past.
    Returns FALSE if the date and time are in the future or present (which
    despite any assertion to the contrary, is not the past.  Although that
    may change soon).  Date is specified as YYYYMMDD; time as HHMMSSCC.
    ---------------------------------------------------------------------[>]-*/

Bool
date_is_past (long date, long time)
{
    return (Bool)(date  < date_now ()
        || (date == date_now () && time < time_now ()));
}


/*  ---------------------------------------------------------------------[<]-
    Function: timezone_string

    Synopsis: Returns a static string containing the time zone as a 4-digit
    number, with a leading '+' or '-' character.   GMT is represented as
    "+0000"; Central European Time is "+1000". If the system does not support
    the timezone, returns "+0000".
    ---------------------------------------------------------------------[>]-*/

char *
timezone_string (void)
{
#if (defined (TIMEZONE))
    static char
        formatted_string [10];          /*  -nnnn plus null                  */
    int
        minutes;                        /*  TIMEZONE is in seconds           */

    minutes = 0 - (((int)(TIMEZONE)) / 60);
    snprintf (formatted_string, sizeof (formatted_string), 
              "%03d%02d", minutes / 60, abs (minutes % 60));
    if (*formatted_string == '0')
        *formatted_string = '+';
    return  (formatted_string);
#else
    return ("+0000");
#endif
}


/*  ---------------------------------------------------------------------[<]-
    Function: local_to_gmt

    Synopsis: Converts the specified date and time to GMT.  Returns the GMT
    date and time in two arguments.
    ---------------------------------------------------------------------[>]-*/

void
local_to_gmt (long date, long time, long *gmt_date, long *gmt_time)
{
    time_t
        time_value;

    time_value = date_to_timer   (date, time);
    *gmt_date  = timer_to_gmdate (time_value);
    *gmt_time  = timer_to_gmtime (time_value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: gmt_to_local

    Synopsis: Converts the specified GMT date and time to the local time.
    Returns the local date and time in two arguments.  If the supplied value 
    is out of range, returns 00:00 on 1 January, 1970 (19700101).
    ---------------------------------------------------------------------[>]-*/

void
gmt_to_local (long gmt_date, long gmt_time, long *date, long *time)
{
    time_t
        time_value;
    struct tm
        *time_struct;

    /*  Convert from GMT                                                     */
    time_value  = date_to_timer (gmt_date, gmt_time) - ((int)(TIMEZONE));
    time_struct = safe_localtime (&time_value);
    if (time_struct-> tm_isdst)
      {
        time_value  += 3600;            /*  Adjust for daylight savings      */
        time_struct = localtime (&time_value);
      }
    time_struct-> tm_year += 1900;
    *date = MAKE_DATE (time_struct-> tm_year / 100,
                       time_struct-> tm_year % 100,
                       time_struct-> tm_mon + 1,
                       time_struct-> tm_mday);
    *time = MAKE_TIME (time_struct-> tm_hour,
                       time_struct-> tm_min,
                       time_struct-> tm_sec,
                       0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: safe_localtime

    Synopsis: Handles time_t values that exceed 2038.  The standard C
    localtime() function fails on most systems when the date passes 2038.
    ---------------------------------------------------------------------[>]-*/

struct tm
*safe_localtime (const time_t *time_secs)
{
    qbyte
        adjusted_time;
    struct tm
        *time_struct;
    int
        adjust_years = 0;

    adjusted_time = (qbyte) *time_secs;
    while (adjusted_time > LONG_MAX)
      {
        adjust_years  += 20;
        adjusted_time -= 631152000;     /*  Number of seconds in 20 years    */
      }
    time_struct = localtime ((const time_t *) &adjusted_time);
    /* ASSERT (time_struct); */              /*  MUST be valid now...             */
    if( !time_struct )
       return NULL;

    time_struct-> tm_year += adjust_years;

    return (time_struct);
}


/*  ---------------------------------------------------------------------[<]-
    Function: safe_gmtime

    Synopsis: Handles time_t values that exceed 2038.  The standard C
    gmtime() function fails on most systems when the date passes 2038.
    ---------------------------------------------------------------------[>]-*/

struct tm
*safe_gmtime (const time_t *time_secs)
{
    qbyte
        adjusted_time;
    struct tm
        *time_struct;
    int
        adjust_years = 0;

    adjusted_time = (qbyte) *time_secs;
    while (adjusted_time > LONG_MAX)
      {
        adjust_years  += 20;
        adjusted_time -= 631152000;     /*  Nbr seconds in 20 years          */
      }
    time_struct = gmtime ((const time_t *) &adjusted_time);
    if (time_struct)                    /*  gmtime may be unimplemented      */
        time_struct-> tm_year += adjust_years;

    return (time_struct);
}

