/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcvst.c
    Title:      Converts a string to a time
    Package:    Standard Function Library (SFL)

    Written:    1996/01/05  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflconv.h"                    /*  Prototypes for functions         */

#include "sflcvst.d"                    /*  Include dialog data              */


/*- Global variables used in this source file only --------------------------*/

static long
    hour,                               /*  Calculated time components       */
    minute,
    second,
    centi,
    number,                             /*  Number picked-up from string     */
    feedback;                           /*  Feedback for calling program     */

static int
    delimiters;                         /*  How many delimiters have we had  */

static Bool
    had_pm,                             /*  TRUE = we had pm indicator       */
    had_am;                             /*  TRUE = we had am indicator       */

static char
    ch;                                 /*  Character just picked-up         */
static const char
    *string;                            /*  Input string to convert          */


/*  ---------------------------------------------------------------------[<]-
    Function: conv_str_time

    Synopsis: Converts a string to a time.  The string must have this format:

        hour[<delim>minute[<delim>second[<delim>centi]]][a[m]|p[m]]

    Any non-digit is accepted as delimiter.  Each component may be one or
    two digits.  The input string must be null-terminated.  Returns -1 in
    case of an invalid date or format.  If the string was empty (contains
    no usable digits, returns 0.  The am/pm indicator can occur one anywhere
    in the string.
    ---------------------------------------------------------------------[>]-*/

/*  We supply the dialog file as the source
long
conv_str_time (const char *p_string)
{

After-Init:
    (--) Ok                                 -> Expect-Hour
          + Get-Next-Component

Expect-Hour:
    (--) Number                             -> Expect-Minute
          + Have-Hour
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Hour
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Minute:
    (--) Number                             -> Expect-Second
          + Have-Minute
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Minute
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Second:
    (--) Number                             -> Expect-Centisecond
          + Have-Second
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Second
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Centisecond:
    (--) Number                             -> Allow-Am-Pm
          + Have-Centisecond
          + Get-Next-Component
    (--) Am-Pm                              -> Expect-Centisecond
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Allow-Am-Pm:
    (--) Am-Pm                              -> Expect-Finished
          + Have-Am-Pm-Indicator
          + Get-Next-Component

Expect-Finished:
    (--) Finished                           ->
          + Have-Complete-Time
          + Terminate-The-Program

Defaults:
    (--) Number                             ->
          + Have-Invalid-Time
          + Terminate-The-Program
    (--) Am-Pm                              ->
          + Have-Invalid-Time
          + Terminate-The-Program
    (--) Finished                           ->
          + Have-Complete-Time
          + Terminate-The-Program
    (--) Delimiter                          ->
          + Have-Delimiter
          + Get-Next-Component
    (--) Error                              ->
          + Have-Invalid-Time
          + Terminate-The-Program
}
*/

long
conv_str_time (const char *p_string)
{
    feedback = 0;                       /*  No errors so far                 */
    string = p_string;                  /*  Local copy of parameters         */

#   include "sflcvst.i"                 /*  Include dialog interpreter       */
}


/*************************   INITIALISE THE PROGRAM   ************************/

MODULE initialise_the_program (void)
{
    the_next_event = ok_event;

    hour   =
    minute =
    second =
    centi  = 0;                         /*  Time is zero so far              */
    delimiters = 0;

    had_pm =
    had_am = FALSE;

    conv_reason = 0;                    /*  No conversion errors so far      */
}


/***************************   GET NEXT COMPONENT   **************************/

MODULE get_next_component (void)
{
    ch = (char) tolower (*string++);
    if (ch == 0)
        the_next_event = finished_event;
    else
    if (isdigit (ch))
      {
        the_next_event = number_event;
        number = ch - '0';
        if (isdigit (*string))
            number = number * 10 + *string++ - '0';
      }
    else
    if (ch == 'a' || ch == 'p')
        the_next_event = am_pm_event;
    else
        the_next_event = delimiter_event;
}


/**************************   HAVE AM PM INDICATOR   *************************/

MODULE have_am_pm_indicator (void)
{
    if (had_am || had_pm)
      {
        conv_reason = CONV_ERR_MULTIPLE_AM;
        raise_exception (error_event);
      }
    else
      {
        had_pm = (ch == 'p');           /*  Save if we had pm                */
        had_am = (ch == 'a');           /*  Save if we had am                */
        if (tolower (*string == 'm'))   /*    and skip optional 'm'          */
            string++;
      }
}


/*******************************   HAVE HOUR   *******************************/

MODULE have_hour (void)
{
    hour = number;
}


/******************************   HAVE MINUTE   ******************************/

MODULE have_minute (void)
{
    minute = number;
}


/******************************   HAVE SECOND   ******************************/

MODULE have_second (void)
{
    second = number;
}


/****************************   HAVE CENTISECOND   ***************************/

MODULE have_centisecond (void)
{
    centi = number;
}


/***************************   HAVE COMPLETE TIME   **************************/

MODULE have_complete_time (void)
{
    if (had_pm && hour < 12)            /*  1 - 11 pm -> 13 - 23             */
        hour += 12;
    else
    if (had_am && hour == 12)           /*  12 am -> 0                       */
        hour = 0;

    /*  Check that time is actually valid                                    */
    if (hour > 23 || minute > 59 || second > 59)
      {
        conv_reason = CONV_ERR_OUT_OF_RANGE;
        raise_exception (error_event);
      }
    else
        feedback = hour * 1000000L + minute * 10000L + second * 100 + centi;
}


/***************************   HAVE INVALID TIME   ***************************/

MODULE have_invalid_time (void)
{
    feedback = -1;
}


/*****************************   HAVE DELIMITER   ****************************/

MODULE have_delimiter (void)
{
}


/***************************   GET EXTERNAL EVENT   **************************/

MODULE get_external_event (void)
{
}


/*************************   TERMINATE THE PROGRAM    ************************/

MODULE terminate_the_program (void)
{
    the_next_event = terminate_event;
}
