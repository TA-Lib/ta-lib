/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfllbuf.h
    Title:      Line buffering functions
    Package:    Standard Function Library (SFL)

    Written:    1997/09/07  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides circular line buffering functions.  A line buffer
                is a data structure that holds a fixed amount of data in a
                serial fashion; the oldest data gets discarded as new data
                is added.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLLBUF_INCLUDED               /*  Allow multiple inclusions        */
#define SFLLBUF_INCLUDED


/*- Type definitions --------------------------------------------------------*/

typedef struct {
    char  *data;                        /*  Buffer contents                  */
    char  *head;                        /*  Where we add new data            */
    char  *tail;                        /*  Oldest data is here              */
    size_t size;                        /*  Actual size of buffer            */
    char  *top;                         /*  Address of top of buffer         */
} LINEBUF;                              /*  Empty when tail == head          */


/*- Function Prototypes -----------------------------------------------------*/

LINEBUF *linebuf_create  (size_t maxsize);
void     linebuf_destroy (LINEBUF *buffer);
void     linebuf_reset   (LINEBUF *buffer);
void     linebuf_append  (LINEBUF *buffer, const char *line);
char    *linebuf_first   (LINEBUF *buffer, DESCR *line);
char    *linebuf_next    (LINEBUF *buffer, DESCR *line, const char *cur);
char    *linebuf_last    (LINEBUF *buffer, DESCR *line);
char    *linebuf_prev    (LINEBUF *buffer, DESCR *line, const char *cur);

#endif
