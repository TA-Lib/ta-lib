/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcons.c
    Title:      Console output functions
    Package:    Standard Function Library (SFL)

    Written:    1997/05/22  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflfile.h"                    /*  File-handling functions          */
#include "sflstr.h"                     /*  String-handling functions        */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory-management functions      */
#include "sfldate.h"                    /*  Date/time functions              */
#include "sflprint.h"                   /*  snprintf functions               */
#include "sflcons.h"                    /*  Prototypes for functions         */


static Bool
    console_active = TRUE,              /*  Allow console output?            */
    console_echo   = TRUE;              /*  Copy to stdout                   */
static CONSOLE_FCT
    *console_fct = NULL;                /*  Redirector function              */
static int
    console_mode = CONSOLE_PLAIN;       /*  Output display mode              */
static FILE
    *console_file = NULL;               /*  Capture file, if any             */

static char *date_str (void);
static char *time_str (void);


/*  ---------------------------------------------------------------------[<]-
    Function: console_send

    Synopsis: Redirects console output to a specified CONSOLE_FCT function.
    If the specified address is NULL, redirects back to the stdout stream.
    This is independent of any console capturing in progress.  If the echo
    argument is TRUE, console output is also sent to stdout.
    ---------------------------------------------------------------------[>]-*/

void
console_send (CONSOLE_FCT *new_console_fct, Bool echo)
{
    console_fct  = new_console_fct;
    console_echo = echo;                /*  Copy to stdout                   */
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_enable

    Synopsis: Enables console output.  Use together with console_disable()
    to stop and start console output.
    ---------------------------------------------------------------------[>]-*/

void
console_enable (void)
{
    console_active = TRUE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_disable

    Synopsis: Disables console output. Use together with console_enable()
    to stop and start console output.
    ---------------------------------------------------------------------[>]-*/

void
console_disable (void)
{
    console_active = FALSE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_set_mode

    Synopsis: Sets console display mode; the argument can be one of:
    <TABLE>
    CONSOLE_PLAIN       Output text exactly as specified
    CONSOLE_DATETIME    Prefix text by "yy/mm/dd hh:mm:ss "
    CONSOLE_TIME        Prefix text by "hh:mm:ss "
    </TABLE>
    The default is plain output.
    ---------------------------------------------------------------------[>]-*/

void
console_set_mode (int mode)
{
    ASSERT (mode == CONSOLE_PLAIN
         || mode == CONSOLE_DATETIME
         || mode == CONSOLE_TIME);

    console_mode = mode;
}


/*  ---------------------------------------------------------------------[<]-
    Function: console_capture

    Synopsis: Starts capturing console output to the specified file.  If the
    mode is 'w', creates an empty capture file.  If the mode is 'a', appends
    to any existing data.  Returns 0 if okay, -1 if there was an error - in
    this case you can test the value of errno.  If the filename is NULL or
    an empty string, closes any current capture file.
    ---------------------------------------------------------------------[>]-*/

int
console_capture (const char *filename, char mode)
{
    if (console_file)
      {
        file_close (console_file);
        console_file = NULL;
      }
    if (filename && *filename)
      {
        ASSERT (mode == 'w' || mode == 'a');
        console_file = file_open (filename, mode);
        if (console_file == NULL)
            return (-1);
      }
    return (0);
}


/*  ---------------------------------------------------------------------[<]-
    Function: coprintf

    Synopsis: As printf() but sends output to the current console.  This is
    by default the stdout device, unless you used console_send() to direct
    console output to some function.  A newline is added automatically.
    ---------------------------------------------------------------------[>]-*/

int
coprintf (const char *format, ...)
{
    va_list
        argptr;                         /*  Argument list pointer            */
    int
        fmtsize = 0;                    /*  Size of formatted line           */
    char
        *formatted = NULL,              /*  Formatted line                   */
        *prefixed = NULL;               /*  Prefixed formatted line          */

    if (console_active)
      {
        formatted = mem_alloc (LINE_MAX + 1);
        if (!formatted)
            return (0);
        va_start (argptr, format);      /*  Start variable args processing   */
        vsnprintf (formatted, LINE_MAX, format, argptr);
        va_end (argptr);                /*  End variable args processing     */
        switch (console_mode)
          {
            case CONSOLE_DATETIME:
                xstrcpy_debug ();
                prefixed = xstrcpy (NULL, date_str (), " ", time_str (), ": ",
                                    formatted, NULL);
                break;
            case CONSOLE_TIME:
                xstrcpy_debug ();
                prefixed = xstrcpy (NULL, time_str (), ": ", formatted, NULL);
                break;
          }
        if (console_file)
          {
            file_write (console_file, prefixed? prefixed: formatted);
            fflush (console_file);
          }
        if (console_fct)
            (console_fct) (prefixed? prefixed: formatted);

        if (console_echo)
          {
            fprintf (stdout, "%s", prefixed? prefixed: formatted);
            fprintf (stdout, "\n");
            fflush  (stdout);
          }
        if (prefixed)
          {
            fmtsize = strlen (prefixed);
            mem_free (prefixed);
          }
        else
            fmtsize = strlen (formatted);

        mem_free (formatted);
      }
    return (fmtsize);
}


/*  -------------------------------------------------------------------------
 *  date_str
 *
 *  Returns the current date formatted as: "yyyy/mm/dd".
 */

static char *
date_str (void)
{
    static char
        formatted_date [11];
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    snprintf (formatted_date, sizeof (formatted_date),
	                      "%4d/%02d/%02d",
                              time_struct-> tm_year + 1900,
                              time_struct-> tm_mon + 1,
                              time_struct-> tm_mday);
                            
    return (formatted_date);
}


/*  -------------------------------------------------------------------------
 *  time_str
 *
 *  Returns the current time formatted as: "hh:mm:ss".
 */

static char *
time_str (void)
{
    static char
        formatted_time [9];
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    snprintf (formatted_time, sizeof (formatted_time),
	                      "%02d:%02d:%02d",
                              time_struct-> tm_hour,
                              time_struct-> tm_min,
                              time_struct-> tm_sec);
    return (formatted_time);
}


/*  ---------------------------------------------------------------------[<]-
    Function: coputs

    Synopsis: As puts() but sends output to the current console.  This is
    by default the stdout device, unless you used console_send() to direct
    console output to some function.
    ---------------------------------------------------------------------[>]-*/

int
coputs (const char *string)
{
    coprintf (string);
    return (1);
}


/*  ---------------------------------------------------------------------[<]-
    Function: coputc

    Synopsis: As putc() but sends output to the current console.  This is
    by default the stdout device, unless you used console_send() to direct
    console output to some function.
    ---------------------------------------------------------------------[>]-*/

int
coputc (int character)
{
    char
        buffer [2];

    if (console_active)
      {
        if (console_file)
          {
            putc (character, console_file);
            fflush (console_file);
          }
        if (console_fct)
          {
            buffer [0] = (char) character;
            buffer [1] = '\0';
            (console_fct) (buffer);
          }
        if (console_echo)
          {
            putc (character, stdout);
            fflush  (stdout);
          }
      }
    return (character);
}
