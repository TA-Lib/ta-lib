/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflcons.h
    Title:      Console output functions
    Package:    Standard Function Library (SFL)

    Written:    1997/05/22  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/02/08

    Synopsis:   Provides redirectable console output: use the coprintf() and
                coputs() calls instead of printf() and puts() in a real-time
                application.  Then, you can call console_send() to send all
                console output to a specified function.  This is a useful
                way to get output into -- for example -- a GUI window.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLCONS_INCLUDED               /*  Allow multiple inclusions        */
#define SFLCONS_INCLUDED


/*  Type definition for operator redirection function                        */

typedef void (CONSOLE_FCT) (const char *);


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

void  console_send        (CONSOLE_FCT *console_fct, Bool echo);
void  console_enable      (void);
void  console_disable     (void);
void  console_set_mode    (int CONSOLE_MODE);
int   console_capture     (const char *filename, char mode);
int   coputs              (const char *string);
int   coprintf            (const char *format, ...);
int   coputc              (int character);

#ifdef __cplusplus
}
#endif


/*  Constant definitions                                                     */

enum {
    CONSOLE_PLAIN,                      /*  Print as requested               */
    CONSOLE_DATETIME,                   /*  Prefix with date and time        */
    CONSOLE_TIME                        /*  Prefix with time only            */
};

#endif
