/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfllbuf.c
    Title:      Line-buffering functions
    Package:    Standard Function Library (SFL)

    Written:    1995/10/28  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/10/06

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory handling functions        */
#include "sflstr.h"                     /*  String handling functions        */

#include "sfllbuf.h"                    /*  Prototypes for functions         */


static char *start_next_line (LINEBUF *buffer, const char *lineptr);
static char *get_line        (LINEBUF *buffer, DESCR *descr, const char *line);

#define buffer_inc(p) if (++(p) == buffer-> top) p = buffer-> data
#define buffer_dec(p) if (p == buffer-> data) p = buffer-> top - 1; else (p)--


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_create

    Synopsis: Creates a new line buffer with the specified size.  The size
    must be at least LINE_MAX + 1 characters long.  Returns the address of
    the newly-created buffer, or NULL if there was insufficient memory.
    The fresh line buffer is set to empty (tail == head).
    ---------------------------------------------------------------------[>]-*/

LINEBUF *
linebuf_create (size_t maxsize)
{
    LINEBUF
        *buffer;

    ASSERT (maxsize > LINE_MAX);

    buffer = mem_alloc (sizeof (LINEBUF));
    if (!buffer)
        return (NULL);

    buffer-> data = mem_alloc (maxsize);
    if (!buffer-> data)
      {
        free (buffer);
        return (NULL);
      }

    buffer-> head = buffer-> data;
    buffer-> tail = buffer-> data;
    buffer-> top  = buffer-> data + maxsize;
    buffer-> size = maxsize;
    return (buffer);
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_destroy

    Synopsis: Destroys a line buffer and frees its memory.
    ---------------------------------------------------------------------[>]-*/

void
linebuf_destroy (LINEBUF *buffer)
{
    ASSERT (buffer);

    mem_free (buffer-> data);
    mem_free (buffer);
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_reset

    Synopsis: Resets a line buffer; i.e. empties it of all data.  This is
    done simply by setting the tail and head to the start of the buffer.
    ---------------------------------------------------------------------[>]-*/

void
linebuf_reset (LINEBUF *buffer)
{
    ASSERT (buffer);
    buffer-> head = buffer-> data;
    buffer-> tail = buffer-> data;
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_append

    Synopsis: Appends a line to the line buffer.  If the buffer was full,
    the oldest line is lost.  Updates the buffer head and tail as needed.
    ---------------------------------------------------------------------[>]-*/

void
linebuf_append (LINEBUF *buffer, const char *line)
{
    int
        length,                         /*  Size of line to insert           */
        room_left,                      /*  Space left between head and top  */
        tail_old,                       /*  Offset of tail into buffer       */
        head_old,                       /*  Offset of head before insert     */
        head_new;                       /*  Offset of head after insert      */
    char
        *linedata;                      /*  Address of data to store         */

    ASSERT (buffer);
    ASSERT (line);

    linedata  = (char *) line;
    length    = strlen (line) + 1;      /*  Include trailing null            */
    room_left = (int) (buffer-> top - buffer-> head);

    /*  We need to make space for the new line; we calculate the new head
     *  and if the tail falls between the old and new head, it must be moved
     *  up to the next line start.  We compare 'ints' not 'char *' because
     *  they can be negative.
     */
    tail_old = (int) (buffer-> tail - buffer-> data);
    head_old = (int) (buffer-> head - buffer-> data);
    if (head_old > tail_old)            /*  Shift head_old down to get it    */
        head_old -= buffer-> size;      /*    somewhere before tail_old      */
    head_new = head_old + length;       /*  And calculate head_new           */

    /*  If the line is too large for the remaining space, copy what we can   */
    if (length > room_left)
      {
        memcpy (buffer-> head, linedata, room_left);
        linedata += room_left;
        length   -= room_left;
        buffer-> head = buffer-> data;  /*  Bump head to start of buffer     */
      }
    /*  Copy rest of line to buffer                                          */
    memcpy (buffer-> head, linedata, length);
    buffer-> head += length;            /*  Bump head past string            */
    if (buffer-> head == buffer-> top)  /*    and maybe wrap-around          */
        buffer-> head = buffer-> data;

    ASSERT (buffer-> head <= buffer-> top);

    if (head_old <  tail_old            /*  If tail falls between head_old   */
    &&  tail_old <= head_new)           /*    and/on head_new, bump it up    */
        buffer-> tail = start_next_line (buffer, buffer-> head);
}


/*  -------------------------------------------------------------------------
 *  start_next_line -- local
 *
 *  Returns the address of the character following the first null byte after
 *  lineptr.  Wraps-around the end-start of the buffer if it has to.
 */

static char *
start_next_line (LINEBUF *buffer, const char *lineptr)
{
    char
        *next;

    next = (char *) memchr (lineptr, 0,
                           (size_t) (buffer-> top - (char *) lineptr));
    if (next == NULL)
        next = (char *) memchr (buffer-> data, 0, buffer-> size);

    ASSERT (next != NULL);              /*  If not there, abandon ship!      */
    buffer_inc (next);
    return (next);
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_first

    Synopsis: Fetches the oldest line in the buffer.  Returns a pointer that
    may be used in calls to linebuf_next().  Returns NULL if the buffer is
    empty.  The line is stored in the supplied descriptor, and is truncated
    if the descriptor is too small.
    ---------------------------------------------------------------------[>]-*/

char *
linebuf_first (LINEBUF *buffer, DESCR *descr)
{
    ASSERT (buffer);
    ASSERT (descr);

    return (linebuf_next (buffer, descr, buffer-> tail));
}


/*  -------------------------------------------------------------------------
 *  get_line -- local
 *
 *  Picks-up the line at the specified address and stores it in the descr
 *  data, truncating to the descr size if necessary.  Returns address of
 *  next line in the buffer.
 */

static char *
get_line (LINEBUF *buffer, DESCR *descr, const char *line)
{
    char
        *dest = (char *) descr-> data;
    size_t
        size = 0;

    while (*line && (size < descr-> size - 1))
      {
        *dest++ = *line;
        buffer_inc (line);
      }
    *dest = '\0';                       /*  Terminate with a null            */
    buffer_inc (line);                  /*  Bump past null                   */
    return ((char *) line);
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_next

    Synopsis: Fetches the next line in the buffer, using the pointer that
    was returned by linebuf_first().  Returns NULL if there are no more lines
    in the buffer, or a pointer for further calls. The line is stored in the
    supplied descriptor, and is truncated if the descriptor is too small.
    ---------------------------------------------------------------------[>]-*/

char *
linebuf_next (LINEBUF *buffer, DESCR *descr, const char *curline)
{
    ASSERT (buffer);
    ASSERT (descr);
    ASSERT (curline);

    if (curline == buffer-> head)
        return (NULL);                  /*  We're at the end                 */
    else
        return (get_line (buffer, descr, curline));
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_last

    Synopsis: Fetches the newest line in the buffer. Returns a pointer that
    may be used in calls to linebuf_next().  Returns NULL if the buffer is
    empty.  The line is stored in the supplied descriptor, and is truncated
    if the descriptor is too small.
    ---------------------------------------------------------------------[>]-*/

char *
linebuf_last (LINEBUF *buffer, DESCR *descr)
{
    ASSERT (buffer);
    ASSERT (descr);

    return (linebuf_prev (buffer, descr, buffer-> head));
}


/*  ---------------------------------------------------------------------[<]-
    Function: linebuf_prev

    Synopsis: Fetches the previous line in the buffer, using the pointer that
    was returned by linebuf_last().  Returns NULL if there are no more lines
    in the buffer, or a pointer for further calls. The line is stored in the
    supplied descriptor, and is truncated if the descriptor is too small.
    ---------------------------------------------------------------------[>]-*/

char *
linebuf_prev (LINEBUF *buffer, DESCR *descr, const char *curline)
{
    ASSERT (buffer);
    ASSERT (descr);
    ASSERT (curline);

    if (curline == buffer-> tail)
        return (NULL);                  /*  We're at the start               */
    else
      {
        /*  We're pointing to the byte after the line's null byte            */
        buffer_dec (curline);           /*  Bump down to null                */
        ASSERT (*curline == '\0');

        do
          {
            buffer_dec (curline);       /*  And now look for previous null   */
            if (*curline == '\0')
              {
                buffer_inc (curline);   /*  Bump up to start of string       */
                break;
              }
          }
        until (curline == buffer-> tail);

        get_line (buffer, descr, curline);
        return ((char *) curline);
      }
}
