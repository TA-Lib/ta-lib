/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflsyst.h
    Title:      System-level functions (assertions,...)
    Package:    Standard Function Library (SFL)

    Written:    1997/04/13  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/10/05

    Synopsis:   Provides miscellaneous system-level functions.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLSYST_INCLUDED               /*  Allow multiple inclusions        */
#define SFLSYST_INCLUDED


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

char *sys_name (Bool full);

#ifdef __cplusplus
}
#endif

#endif
