/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflfort.h
    Title:      Fortune-cookie functions
    Package:    Standard Function Library (SFL)

    Written:    1999/08/16  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/08/27

    Synopsis:   Provides functions to create compressed or simple fortune
                cookie files, and functions to read from such files.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLFORT_INCLUDED               /*  Allow multiple inclusions        */
#define SFLFORT_INCLUDED


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

int   fortune_build (const char *in, const char *out, Bool compress);
char *fortune_read  (const char *fortune_file);

#ifdef __cplusplus
}
#endif


#endif
