/*  ----------------------------------------------------------------<Prolog>-
    Name:       sfllang.h
    Title:      Multilanguage support
    Package:    Standard Function Library (SFL)

    Written:    1997/06/04  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/05/31

    Synopsis:   Provides hard-coded multilanguage dictionaries for dates and
                numbers,  The hard-coded dictionaries work with most European
                languages.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLLANG_INCLUDED               /*  Allow multiple inclusions        */
#define SFLLANG_INCLUDED


/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

int   set_userlang        (int language);
int   set_userlang_str    (const char *language);
int   get_userlang        (void);
char *get_userlang_str    (void);
int   set_accents         (Bool accents);
Bool  get_accents         (void);
char *get_units_name      (int units);
char *get_tens_name       (int tens);
char *get_day_name        (int day);
char *get_day_abbrev      (int day, Bool upper);
char *get_month_name      (int month);
char *get_month_abbrev    (int month, Bool upper);
char *timestamp_string    (char *buffer, const char *pattern);

#ifdef __cplusplus
}
#endif


/*  Constant definitions                                                     */

enum {
    USERLANG_DEFAULT = 0,               /*  Default language                 */
    USERLANG_DA,                        /*  Danish                           */
    USERLANG_DE,                        /*  German                           */
    USERLANG_EN,                        /*  English                          */
    USERLANG_ES,                        /*  Castillian Spanish               */
    USERLANG_FB,                        /*  Belgian or Swiss French          */
    USERLANG_FR,                        /*  French                           */
    USERLANG_IS,                        /*  Icelandic                        */
    USERLANG_IT,                        /*  Italian                          */
    USERLANG_NL,                        /*  Dutch                            */
    USERLANG_NO,                        /*  Norwegian                        */
    USERLANG_PO,                        /*  Portuguese                       */
    USERLANG_SV                         /*  Swedish                          */
};

#define USERLANG_TOP     USERLANG_SV + 1

#endif
