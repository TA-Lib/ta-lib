/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflhttp.h
    Title:      HTTP and CGI Support functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/31  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/06/16

    Synopsis:   Provides various functions that support HTTP and CGI
                programming, including escaping/unescaping, and CGI data
                manipulation.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#ifndef SFLHTTP_INCLUDED               /*  Allow multiple inclusions        */
#define SFLHTTP_INCLUDED

/*  Macro's and defines                                                      */

/* Macro to free up the input line that GetCgiInput created. */
#define cgi_free_input(strBuf) free((strBuf))

/* Defines for input methods. */
#define CGIGET   0
#define CGIPOST  1
#define CGIETHER 2

/*  Function prototypes                                                      */

#ifdef __cplusplus
extern "C" {
#endif

char   *http_escape           (const char *string, char *result, size_t outmax);
char   *http_escape_hex       (const char *string, char *result, size_t outmax);
size_t  http_escape_size      (const char *string);
char   *http_unescape         (char *string, char *result);
char   *http_unescape_hex     (char *string, char *result);
char  **http_query2strt       (const char *query);
SYMTAB *http_query2symb       (const char *query);
DESCR  *http_query2descr      (const char *query);
size_t  http_encode_meta      (char  *output, char **input,
                               size_t outmax, Bool html);
size_t  encode_meta_char      (char  *output, char meta_char,
                               size_t outmax, Bool html);
size_t  http_decode_meta      (char  *output, char **input, size_t outmax);
char    decode_meta_charn     (const char *meta_char, size_t length);
int     cgi_parse_query_vars  (SYMTAB *symtab, const char *query,
                               const char *prefix);
int     cgi_parse_file_vars   (SYMTAB *symtab, FILE *file, const char *prefix,
                               size_t size);
DESCR  *http_multipart_decode (const char *mime_file, const char *store_path,
                               const char *local_format);
Bool    is_full_url           (const char *string);
char   *build_full_url        (const char *uri, const char *base_uri);
char   *http_time_str         (void);
char   *cgi_get_input         (int iMethod);
char   *cgi_fld_by_name       (char *strFld, char *strIn, char *strRetBuf);
char   *cgi_fld_by_index      (int iPos, char *strIn, char *strRetBuf,
                               char *strFldName);
int     cgi_fld_len_by_index  (int iPos, char *strIn, int *iDataLen,
                               int *iNameLen);
int     displayform           (char *strformfile, char *strvalues);

#ifdef __cplusplus
}
#endif

#define decode_meta_char(string) decode_meta_charn (string, sizeof (string))

#endif
