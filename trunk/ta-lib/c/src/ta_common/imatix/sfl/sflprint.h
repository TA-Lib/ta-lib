/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflprint.h
    Title:      Printing Functions
    Package:    Standard Function Library (SFL)

    Written:    1999/09/10  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/10

    Synopsis:   Provides printing functions which may be absent on some
                systems.   In particular ensures that the system has 
		snprintf()/vsnprintf() functions which can be called.  The
                functions supplied here are not as good as the vender 
		supplied ones, but are better than having none at all.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLPRINT_INCLUDED               /*  Allow multiple inclusions        */
#define SFLPRINT_INCLUDED

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

#if (! defined (DOES_SNPRINTF))
int snprintf  (char *str, size_t n, const char *format, ...);
int vsnprintf (char *str, size_t n, const char *format, va_list ap);
#endif

#ifdef __cplusplus
}
#endif

#endif
