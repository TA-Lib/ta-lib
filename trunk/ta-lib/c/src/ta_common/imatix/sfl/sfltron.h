/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfltron.h
    Title:      Tracing functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/25  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides functions for a programmer who needs to insert
                long-term tracing code in software.  The tracing code is
                activated and disactivated at run-time, for instance when
                problems are suspected.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLTRON_INCLUDED               /*  Allow multiple inclusions        */
#define SFLTRON_INCLUDED

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

void  enable_trace        (void);
void  disable_trace       (void);
void  push_trace          (Bool new_state);
void  pop_trace           (void);
void  set_trace_file      (const char *filename, char mode);
void  trace               (const char *format, ...);

#ifdef __cplusplus
}
#endif


/*  External variables                                                       */

extern Bool  TraceState;                /*  TRUE or FALSE                    */
extern FILE *TraceFile;                 /*  Current trace output file        */

#endif
