/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmesg.h
    Title:      Message-file access functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/25  iMatix SFL project team <sfl@imatix.com>
    Revised:    1997/09/08

    Synopsis:   Provides functions to read and format messages from a message
                file.  The intention of such a file is to provide a single
                location for all error messages: you can easier translate
                these into foreign languages, and you can control the
                consistency of an application's error messages.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLMESG_INCLUDED               /*  Allow multiple inclusions        */
#define SFLMESG_INCLUDED


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

int    open_message_file   (const char *filename);
void   close_message_file  (void);
void   print_message       (int msgid, ...);
char  *message_text        (int msgid);

#ifdef __cplusplus
}
#endif

/*  Symbols, macros                                                          */

#define ERROR_ANY       0000            /*  Generic error message            */

#endif
