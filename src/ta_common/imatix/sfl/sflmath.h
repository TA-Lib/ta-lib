/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmath.h
    Title:      Mathematic functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/12  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides miscellaneous mathematical functions, including
                calculation of points within areas.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLMATH_INCLUDED               /*  Allow multiple inclusions        */
#define SFLMATH_INCLUDED

/*  Structure declaration                                                    */

typedef struct
  {
    double x;
    double y;
  } FPOINT;

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

int  point_in_rect   (const FPOINT *point, const FPOINT *coords);
int  point_in_circle (const FPOINT *point, const FPOINT *coords);
int  point_in_poly   (const FPOINT *point, const FPOINT *coords, int nb_point);

#ifdef __cplusplus
}
#endif

#endif
