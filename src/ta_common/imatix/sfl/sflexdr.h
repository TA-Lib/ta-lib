/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflexdr.h
    Title:      External data representation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/25  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides functions to read and write data in a portable
                format that is suitable for transmission to other systems.
                The principle is similar to the ONC XDR standard used in
                RPC, but somewhat simpler.  The streams produced by these
                functions are not compatible with ONC XDR.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLEXDR_INCLUDED               /*  Allow multiple inclusions        */
#define SFLEXDR_INCLUDED


/*---------------------------------------------------------------------------
 *  Function prototypes
 */

#ifdef __cplusplus
extern "C" {
#endif

int    exdr_write   (byte  *buffer, const char *format, ...);
int    exdr_writed  (DESCR *buffer, const char *format, ...);
int    exdr_read    (const byte *buffer, const char *format, ...);

#ifdef __cplusplus
}
#endif


#endif                                  /*  Include sflexdr.h                */
