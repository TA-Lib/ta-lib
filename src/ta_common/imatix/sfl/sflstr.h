/* Note: This file has been adapted for running within TA-LIB.
 * If you intend to use iMatix, please use their original file.
 */

/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflstr.h
    Title:      String-handling functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/25  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/06/22

    Synopsis:   Provides various string-handling functions.  Some of these
                functions are available on some but not all platforms; others
                are useful tools for string handling.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLSTR_INCLUDED                /*  Allow multiple inclusions        */
#define SFLSTR_INCLUDED

#include "ta_common.h"

/*  Macro to prepare call to xstrcpy                                         */
#if (defined (DEBUG))
#  define xstrcpy_debug() { xstrcpy_file = __FILE__; xstrcpy_line = __LINE__;}
#else
#  define xstrcpy_debug()
#endif

extern char *xstrcpy_file;
extern word  xstrcpy_line;

/*  Structure defines simple unsorted lookup table                           */
typedef struct {
    char *key;
    int   value;
} LOOKUP;

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

#if 0
char   *strdupl            (const char *string);
char  **strfree            (char **string);
#endif

char   *strskp             (const char *string);
char   *strcset            (char *string, char ch);
char   *strpad             (char *string, char ch, int length);
char   *strlwc             (char *string);
char   *strupc             (char *string);
char   *strcrop            (char *string);
char   *stropen            (char *string, Bool align);
char   *strclose           (char *string, Bool align);
char   *strunique          (char *string, char unique);
int     strmatch           (const char *string1, const char *string2);
qbyte   strhash            (const char *string);
Bool    strprefixed        (const char *string, const char *prefix);
char   *strprefix          (const char *string, const char *delims);
char   *strdefix           (const char *string, const char *prefix);
char   *strconvch          (char *string, char from, char to);
char   *xstrcat            (char *dest, const char *src, ...);
char   *xstrcpy            (char *dest, const char *src, ...);
int     lexcmp             (const char *string1, const char *string2);
int     lexncmp            (const char *string1, const char *string2,
                            const int count);
int     lexwcmp            (const char *string1, const char *string2);

#if 0
char   *soundex            (const char *string);
char   *soundexn           (const char *string, int size, Bool fold);
#endif

DESCR  *strt2descr         (char **strings);
char  **descr2strt         (const DESCR *descr);
void    strtfree           (char **strings);
int     strcntch           (const char *string, char value);
int     strlookup          (const LOOKUP *lookup, const char *key);
char   *strreformat        (const char *source, size_t width, const char *prefix);

char   *removechars        (char *str, char *chrs);
char   *replacechrswith    (char *str, char *chrs, char ctorlcwth);
char   *insertstring       (char *str, char *chrstoins, int pos);
char   *insertchar         (char *str, char chrtoins, int pos);
char   *leftfill           (char *str, char chrtofill, unsigned len);
char   *rightfill          (char *str, char chrtofill, unsigned len);
char   *trim               (char *strin);
char   *ltrim              (char *string);
char   *searchreplace      (char *str, char *strtofnd, char *strtoins);
char   *deletestring       (char *str, char *strtodel, int ignorecase);
char   *getstrfld          (char *str, int fln, int ofset,
                            char *sep, char *ret);
char   *setstrfld          (char *str, int fln, int ofset,
                            char *sep, char *ins);
int     getstrfldlen       (const char *str, int fln, int ofset, char *sep);
char   *findstrinfile      (FILE *fp, char *find, char *retstr, int *iLnNo);
char   *getequval          (char *strline, char *strretstr);
int     matchtable         (char *str, char *strmatch,
                            char *strsept, int ncse);
char   *stringreplace      (char *strbuf, char *strpattern);
char   *wordwrapstr        (char *strbuff, int iwid);
char   *stricstr           (const char *str1, const char *str2);
int     strtempcmp         (const char *str1, const char *strPat);
int     istoken            (char **strLine, const char *strtoken,
                            int *iWasToken);
int     isoneoftokens      (char **str, char *strmat,
                            char *strsep, int *iWasTk);
char   *eatstr             (char **strBuff, char *strToEat);
char   *eatstrpast         (char **strBuff, char *strCharsToEatPast);
char   *movestrpast        (char **strBuff, char cCharToEatPast);
char   *eatchar            (char **strBuff, char cChar);

#ifdef __cplusplus
}
#endif

/*  Macros & defines                                                         */
#define IGNORECASE 0
#define SENSECASE  1

#define mstrncpy(dest,src,len) {strncpy(dest,src,len);*(dest+len)='\0';}
#define cstrcpy(dest,src) {*dest=src;*(dest+1)='\0';}
#define deletechar(strbuf,pos) strcpy((strbuf+pos),(strbuf+pos+1))
#define deletechars(strbuf,pos,cnt) strcpy((strbuf+pos),(strbuf+pos+cnt))
#define getcommandlinearg(argnum) ((argnum)<=(argc-1))?argv[argnum]:""
#define numofcmdargs() ((argc-1)>0?argc-1:0)
#define checkargcnt(reqnum) ((argc-1)>=(reqnum)?1:0)

#endif
