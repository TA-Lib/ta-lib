/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflslot.h
    Title:      Time-slot functions
    Package:    Standard Function Library (SFL)

    Written:    1996/01/01  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   The time-slot functions provide long-running programs with
                a means to 'switch-on' and 'switch-off' depending on the time
                of day, and day of year.  The intention is that the user can
                configure such programs to be active only between certain
                hours, on certain days, etc.  The time-slot functions work
                with 'range' bitmaps for a day (in seconds) and a year (in
                days), and provide functions to set, clear, and test these
                ranges.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLSLOT_INCLUDED               /*  Allow multiple inclusions        */
#define SFLSLOT_INCLUDED

#define MAX_DAY          366            /*  Max. days in a normal year       */
#define MAX_MIN         1440            /*  Max. minutes in a normal day     */
typedef byte year_range [46];           /*  366 bits (1 per day)             */
typedef byte day_range  [180];          /*  1440 bits (1 per minute)         */

#ifdef __cplusplus
extern "C" {
#endif

void  year_range_empty      (byte *range);
void  year_range_fill       (byte *range);
int   year_slot_set         (byte *range, int day_from, int day_to);
int   year_slot_clear       (byte *range, int day_from, int day_to);
Bool  year_slot_filled      (const byte *range, int day);

void  day_range_empty       (byte *range);
void  day_range_fill        (byte *range);
int   day_slot_set          (byte *range, int min_from, int min_to);
int   day_slot_clear        (byte *range, int min_from, int min_to);
Bool  day_slot_filled       (const byte *range, int minute);

int   date_to_day           (long date);
int   time_to_min           (long time);

#ifdef __cplusplus
}
#endif

#endif
