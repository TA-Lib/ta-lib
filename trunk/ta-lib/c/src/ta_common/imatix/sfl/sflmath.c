/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmath.c
    Title:      Mathematic functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/13  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
                This code was adapted from the NCSA HTTPd Server.
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflmath.h"                    /*  Function prototypes              */


/*  ---------------------------------------------------------------------[<]-
    Function: point_in_rect

    Synopsis: Checks if the requested FPOINT is within the specified
              rectangle.  Returns TRUE or FALSE appropriately.
    ---------------------------------------------------------------------[>]-*/

int
point_in_rect (const FPOINT *point, const FPOINT *coords)
{
    return ((point-> x >= coords [0].x && point-> x <= coords [1].x)
         && (point-> y >= coords [0].y && point-> y <= coords [1].y));
}


/*  ---------------------------------------------------------------------[<]-
    Function: point_in_circle

    Synopsis: Checks if the requested FPOINT is within the specified
              circle.  Returns TRUE or FALSE appropriately.
    ---------------------------------------------------------------------[>]-*/

int
point_in_circle (const FPOINT *point, const FPOINT *coords)
{
    double
        circle_radius,
        distance_from_centre;

    circle_radius        = ((coords [0].y - coords [1].y) *
                            (coords [0].y - coords [1].y)) +
                           ((coords [0].x - coords [1].x)  *
                            (coords [0].x - coords [1].x));

    distance_from_centre = ((coords [0].y - point-> y) *
                            (coords [0].y - point-> y)) +
                           ((coords [0].x - point-> x) *
                            (coords [0].x - point-> x));
    return (distance_from_centre <= circle_radius);
}


/*  ---------------------------------------------------------------------[<]-
    Function: point_in_poly

    Synopsis: Checks if the requested FPOINT is within the specified
              polygon.  Returns TRUE or FALSE.
    ---------------------------------------------------------------------[>]-*/

int
point_in_poly (const FPOINT *point, const FPOINT *pgon, int nbpoints)
{
    int
        inside_flag,
        xflag0,
        crossings;
    const double
        *stop;
    double
        *p,
        tx,
        ty,
        y;

    crossings = 0;

    tx = point-> x;
    ty = point-> y;
    y  = pgon [nbpoints - 1].y;

    p = (double *) pgon + 1;
    if ((y >= ty) != (*p >= ty))
      {
        if ((xflag0 = (pgon [nbpoints - 1].x >= tx)) ==
                (*(double *) pgon >= tx))
          {
            if (xflag0)
               crossings++;
          }
        else
            crossings += (pgon [nbpoints - 1].x - (y - ty) *
                         (*(double *) pgon - pgon [nbpoints - 1].x) /
                         (*p - y)) >= tx;
      }
     stop = &pgon [nbpoints].y;
     for (y = *p, p += 2; p <= stop; y = *p, p += 2)
       {
         if (y >= ty)
           {
             while ((p < stop) && (*p >= ty))
                 p += 2;
             if (p >= stop)
                break;
             if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
               {
                 if (xflag0)
                    crossings++;
               }
             else
                 crossings += (*(p - 3) - (*(p - 2) - ty) *
                              (*(p - 1) - *(p - 3)) /
                              (*p - *(p - 2))) >= tx;
           }
         else
           {
             while ((p < stop) && (*p < ty))
                p += 2;
             if (p >= stop)
                break;
             if ((xflag0 = (*(p - 3) >= tx)) == (*(p - 1) >= tx))
               {
                 if (xflag0)
                    crossings++;
               }
             else
                 crossings += (*(p - 3) - (*(p - 2) - ty) *
                              (*(p - 1) -  *(p - 3)) /
                              (*p - *(p - 2))) >= tx;
           }
       }
     inside_flag = crossings & 0x01;
     return (inside_flag);
}
