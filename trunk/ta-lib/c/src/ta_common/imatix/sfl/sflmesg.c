/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmesg.c
    Title:      Error-message access functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/28  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflfile.h"                    /*  File-access functions            */
#include "sflmesg.h"                    /*  Prototypes for functions         */
#include "sflprint.h"                   /*  snprintf functions               */


local read_msg (int msgid);
static FILE
    *msgfile = NULL;
static char
    msgline [LINE_MAX + 1];

/*  ---------------------------------------------------------------------[<]-
    Function: open_message_file

    Synopsis: Opens the specified error message file for reading.  Returns
    0 if the file exists and is readable, otherwise returns -1.  Use this
    function before calling print_message().  You can keep just one message
    file open at once; this function closes any previously-opened message
    file.  This was done on purpose: it is common to open a message file for
    an entire application in the main function, then refer to it at other
    points in the code.  It is a pain to pass file handles around the entire
    application, and global variables are generally a bad idea.
    ---------------------------------------------------------------------[>]-*/

int
open_message_file (const char *filename)
{
    int feedback;

    close_message_file ();
    msgfile = file_open (filename, 'r');

    if (msgfile)
        feedback = 0;
    else
        feedback = -1;

    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: close_message_file

    Synopsis: Closes the currently open message file, if any.  Does not
    return anything.
    ---------------------------------------------------------------------[>]-*/

void
close_message_file (void)
{
    if (msgfile)
      {
        file_close (msgfile);
        msgfile = NULL;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: print_message

    Synopsis: Scans the message file for a message with the specified id.
    Each line in the message file should start with a four-digit id, then a
    space, then the message to print.  The message can include format
    specifiers using '%'.  Values for each format are passed after the msgid.
    Returns nothing.  The message file must be sorted by ascending message
    id's.  Make sure you call open_message_file () before this function.
    Prints the message on stderr.
    ---------------------------------------------------------------------[>]-*/

void
print_message (int msgid, ...)
{
    va_list argptr;                     /*  Argument list pointer            */

    read_msg (msgid);                   /*  Retrieve message into msgline    */
    va_start (argptr, msgid);           /*  Start variable arguments list    */
    vfprintf (stderr, msgline, argptr);
    va_end   (argptr);                  /*  End variable arguments list      */
    fprintf  (stderr, "\n");
    fflush   (stderr);
}


/*  ---------------------------------------------------------------------[<]-
    Function: message_text

    Synopsis: Works like print_message(), but returns a pointer to the raw
    message rather than printing it.  The message text is stored in a static
    area that is overwritten by each call.

    If msgid is -1, retrieves the next message sequentially, ignoring any
    numbering.  This is only valid after previously reading a message.
    Places "." in the message if no more are found.
    ---------------------------------------------------------------------[>]-*/

char *
message_text (int msgid)
{
    read_msg (msgid);                   /*  Retrieve message into msgline    */
    return (msgline);
}


/*  Read message into msgline, expand '$' if found at end of message         */

local read_msg (int msgid)
{
    static int
        lastid = 32767;                 /*  Last message we read             */

    if (msgfile == NULL)
      {
        snprintf (msgline, sizeof (msgline), 
		           "** Message %d not found - file not open **\n",
                           msgid);
        return;                         /*  Message not found in file        */
      }
    if (msgid == -1)                    /*  Get next ignoring numbers?       */
      {
        if (!file_read (msgfile, msgline))
            strncpy (msgline, "0000 .", sizeof (msgline)); /*  "." signals   */
	                                                   /* end of file    */
      }
    else
      {
        if (msgid <= lastid)            /*  If necessary, back to start      */
            rewind (msgfile);

        for (;;)
          {
            if (!file_read (msgfile, msgline))
              {
                snprintf (msgline, sizeof (msgline), 
			           "0000 ** Message %d not found **\n", msgid);
                break;                  /*  Message not found in file        */
              }
            if ((isdigit (*msgline))
            && (atoi (msgline) == msgid))
              {
                lastid = msgid;
                break;
              }
          }
      }
    /*  Remove first four digits of message                                  */
    memmove (msgline, msgline + 4, strlen (msgline) - 3);
    /*  Remove leading space, if any                                         */
    if (msgline [0])
        memmove (msgline, msgline + 1, strlen (msgline));

    /*  Append system message if reqd                                        */
    if (*msgline && strlast (msgline) == '$')
      {
        strlast (msgline) = 0;
        strcat  (msgline, ": ");
        strcat  (msgline, strerror (errno));
      }
    /*  Kill newline at end of line                                          */
    if (*msgline && strlast (msgline) == '\n')
        strlast (msgline) = 0;
}
