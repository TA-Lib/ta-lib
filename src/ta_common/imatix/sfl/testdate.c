/*  ----------------------------------------------------------------<Prolog>-
    Name:       testdate.c
    Title:      Test program for date & time functions
    Package:    Standard Function Library (SFL)

    Written:    1996/07/10  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

int main (int argc, char *argv [])
{
    long date, time;

    date = date_now ();
    time = time_now ();
    printf ("Date=%ld time=%ld\n", date, time);

    date = days_to_date  (date_to_days (date));
    time = csecs_to_time (time_to_csecs (time));
    printf ("Date=%ld time=%ld\n", date, time);

    date = date_now ();
    time = time_now ();
    date = days_to_date  (date_to_days (date));
    time = csecs_to_time (time_to_csecs (time));

    future_date (&date, &time, 0, INTERVAL_HOUR);
    printf ("Date in one hour = %ld, %ld\n", date, time);

    date = date_now ();
    time = time_now ();
    future_date (&date, &time, 0, INTERVAL_DAY);
    printf ("Date in one day  = %ld, %ld\n", date, time);

    date = date_now ();
    time = time_now ();
    future_date (&date, &time, 1, 0);
    printf ("Date in one day  = %ld, %ld\n", date, time);

    date = date_now ();
    time = time_now ();
    future_date (&date, &time, 7, 0);
    printf ("Date in one week = %ld, %ld\n", date, time);

    date = date_now ();
    time = time_now ();
    future_date (&date, &time, 365, 0);
    printf ("Date in one year = %ld, %ld\n", date, time);

    return (EXIT_SUCCESS);
}
