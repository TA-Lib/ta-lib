/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfltron.c
    Title:      Tracing functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/28  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/12/04  iMatix SFL project team <sfl@imatix.com>

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflfile.h"                    /*  File access functions            */
#include "sfldate.h"                    /*  Date/time functions              */
#include "sfltron.h"                    /*  Prototypes for functions         */


Bool
    trace_state = FALSE,                /*  Initial default                  */
    pushed_state = FALSE;               /*  Saved trace state                */
FILE
    *trace_file = NULL;                 /*  Trace file stream                */


/*  ---------------------------------------------------------------------[<]-
    Function: enable_trace

    Synopsis: Enables tracing.  All calls to trace() send a line of text to
    stdout or the trace file specified with set_trace_file().
    ---------------------------------------------------------------------[>]-*/

void
enable_trace (void)
{
    trace_state = TRUE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: disable_trace

    Synopsis: Ends tracing.  Following a call to this functions, calls to
    trace() have no effect.
    ---------------------------------------------------------------------[>]-*/

void
disable_trace (void)
{
    trace_state = FALSE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: push_trace

    Synopsis: Saves the current trace state.  Restore with pop_trace().
    The current implementation only saves one level of tracing.
    ---------------------------------------------------------------------[>]-*/

void
push_trace (Bool new_state)
{
    pushed_state = trace_state;
    trace_state = new_state;
}


/*  ---------------------------------------------------------------------[<]-
    Function: pop_trace

    Synopsis: Restores a trace state saved by push_trace().
    ---------------------------------------------------------------------[>]-*/

void
pop_trace (void)
{
    trace_state = pushed_state;
}


/*  ---------------------------------------------------------------------[<]-
    Function: set_trace_file

    Synopsis:
    Sends trace output to the specified file.  If set_trace_file is not used,
    traces go to the console.  If filename is null, any open trace file is
    close.  Only one trace file can be open at any time.  If mode is 'a',
    output is appended to the trace file; if 'w' the trace file is reset at
    open time.  The caller can check for errors in this function by looking
    at the value of trace_file, which is left null if errors occur.
    ---------------------------------------------------------------------[>]-*/

void
set_trace_file (const char *filename, char mode)
{
    if (trace_file)
      {
        file_close (trace_file);
        trace_file = NULL;
      }
    if (filename)
      {
        ASSERT (mode == 'w' || mode == 'a');
        trace_file = file_open (filename, mode);
      }
}


/*  -------------------------------------------------------------------------
 *  print_time_str
 *
 *  Prints the current date and time formatted as: "yyyy/mm/dd hh:mm:ss".
 *  out to the specified FILE stream.  (This might be better situated as
 *  a public function in sfldate.c.)
 *
 *  Reentrant.
 */

static void
print_time_str (FILE *out)
{
    time_t
        time_secs;
    struct tm
        *time_struct;

    ASSERT (out);                 /*  Expect handle for output, fail nicely  */
    if (!out)
        return;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    fprintf (out, "%2d/%02d/%02d %2d:%02d:%02d",
                  time_struct-> tm_year + 1900,
                  time_struct-> tm_mon + 1,
                  time_struct-> tm_mday,
                  time_struct-> tm_hour,
                  time_struct-> tm_min,
                  time_struct-> tm_sec);
}


/*  ---------------------------------------------------------------------[<]-
    Function: trace

    Synopsis:
    If the global variable trace_state is TRUE, this function formats the
    parameters (using printf() conventions) and sends these to stdout, or
    the trace_file if opened using set_trace_file(). The trace output is
    given a newline automatically.

    Reentrant.  Uses globals.
    ---------------------------------------------------------------------[>]-*/

void
trace (const char *format, ...)
{
    va_list
        argptr;                         /*  Argument list pointer            */
    FILE
        *out = stdout;                  /*  Output by default to stdout      */

    if (trace_state)
      {
        va_start (argptr, format);      /*  Start variable args processing   */
        if (trace_file)
            out = trace_file;

        /*  Output message:  date: message text\n                            */
        print_time_str (out);
        fputs    (": ", out);
        vfprintf (out, format, argptr);
        fputs    ("\n", out);
        fflush   (out);

        va_end (argptr);                /*  End variable args processing     */
      }
}
