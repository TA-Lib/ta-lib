/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflini.h
    Title:      Initialisation file access functions
    Package:    Standard Function Library (SFL)

    Written:    1994/01/08  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/10/26

    Synopsis:   Provides functions to read an initialisation file that follows
                the MS-Windows style, i.e. consists of [Sections] followed by
                keyword = value lines.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SLFINI_INCLUDED                /*  Allow multiple inclusions        */
#define SLFINI_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

Bool    ini_find_section  (FILE *inifile, char *section, Bool top);
Bool    ini_scan_section  (FILE *inifile, char **keyword, char **value);
SYMTAB *ini_dyn_load      (SYMTAB *symtab, const char *filename);
SYMTAB *ini_dyn_loade     (SYMTAB *symtab, const char *filename);
int     ini_dyn_save      (SYMTAB *symtab, const char *filename);
Bool    ini_dyn_changed   (SYMTAB *symtab);
Bool    ini_dyn_refresh   (SYMTAB *symtab);
char   *ini_dyn_value     (SYMTAB *symtab, const char *section,
                           const char *keyword, const char *default_value);
char  **ini_dyn_values    (SYMTAB *symtab, const char *section,
                           const char *keyword, const char *default_value);
char   *ini_dyn_assume    (SYMTAB *symtab, const char *section,
                           const char *keyword, const char *default_value);

#ifdef __cplusplus
}
#endif

#endif
