/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflhttp.c
    Title:      HTTP and CGI Support functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/31  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflstr.h"                     /*  String functions                 */
#include "sflsymb.h"                    /*  Symbol functions                 */
#include "sflconv.h"                    /*  Convertion functions             */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory functions                 */
#include "sflfind.h"                    /*  Find text functions              */
#include "sflfile.h"                    /*  Files functions                  */
#include "sfldate.h"                    /*  Date/time functions              */
#include "sflprint.h"                   /*  snprintf functions               */
#include "sflhttp.h"                    /*  Prototypes for functions         */

/*  Constants -------------------------------------------------------------- */

/*  This is the maximum size of a stream of HTTP query data coming from a
 *  file.  Used by cgi_parse_file_vars ().
 */

#define CGI_QUERY_FILE_MAX      65535U
#define MULTI_BUFFER_SIZE       16384
#define REST_BUFFER_SIZE        20000


/*- Local functions ---------------------------------------------------------*/

static size_t http_escape_char        (char code, char *result, size_t outmax,
                                       Bool hex_spaces);
static int    decode_hex              (const char **input, size_t outmax);
static void   save_multipart_header   (SYMTAB *table, SYMTAB *header,
                                       char *data, char *tmp_name,
                                       const char *local_format);
static void   multipart_decode_header (char *header, SYMTAB *table);
static DESCR *http_multipart2url      (const SYMTAB *symtab);


/*  ---------------------------------------------------------------------[<]-
    Function: http_escape

    Synopsis: Performs HTTP escaping on a string.  This works as follows:
    all characters except alphanumerics and spaces are converted into the
    3-byte sequence "%xx" where xx is the character's hexadecimal value;
    spaces are replaced by '+'.  Line breaks are stored as "%0D%0A", where
    a 'line break' is any one of: "\n", "\r", "\n\r", or "\r\n".  If the
    result buffer is NULL, calculates the required size, allocates a block
    of memory, and returns that.  Otherwise, returns result, which must be
    large enough for the escaping operation (see http_escape_size()).
    When you all http_escape() with a null target block, you must free the
    returned block using mem_free().  Returns NULL if it could not allocate
    a target block as required.  If outmax is non-zero then no more than
    outmax characters (including the NULL terminator) are stored.
    ---------------------------------------------------------------------[>]-*/

char *
http_escape (
    const char *string,
    char *result,
    size_t outmax)
{
    char
        *target;                        /*  Where we store the result        */
    size_t
        length;                         /*  of escaped character             */

    ASSERT (string);
    if (outmax == 0)                    /*  If no fixed length, get total len*/
        outmax = http_escape_size (string);

    if (result == NULL)
        if ((result = mem_alloc (outmax)) == NULL)
            return (NULL);              /*  Could not allocate a block       */

    if (outmax > 1)
        outmax -= 1;                    /*  Leave space for NULL terminator  */
    else
    if (outmax == 1)                    /*  Only room for terminator         */
      {
        *result = '\0';
        return (result);
      }
    target = result;
    while (*string)
      {
        length = http_escape_char (*string, target, outmax, FALSE);
        if (length == 0)
            break;
        
        target += length;
        outmax -= length;
        if (*string == '\n' || *string == '\r')
          {
            if ((string [1] == '\n' || string [1] == '\r')
            &&  (string [1] != *string))
                string++;
          }
        string++;
      }
    *target = '\0';                     /*  Terminate target string          */
    return (result);
}


/*---------------------------------------------------------------------------
 *  http_escape_char -- local
 *
 *  Performs HTTP escaping on a character.
 *---------------------------------------------------------------------------*/

static size_t
http_escape_char (
    char   code,
    char  *result,
    size_t outmax,
    Bool   hex_spaces)
{
    static char
        hex_char [] = "0123456789ABCDEF";

    if (isalnum (code))                 /*  Don't escape letters or digits   */
      {
        if (outmax < 1)
            return (0);
        *result = code;
        return 1;
      }
    else
    if (code == ' ' && !hex_spaces)     /*  Spaces are replaced by '+'       */
      {
        if (outmax < 1)
            return (0);
        *result = '+';
        return 1;
      }
    if (code == '\n' || code == '\r')
      {
        if (outmax < 6)
            return 0;
        *result++ = '%';                /*  New line becomes %0A%0D          */
        *result++ = '0';
        *result++ = 'A';
        *result++ = '%';
        *result++ = '0';
        *result++ = 'D';
        return (6);
      }
    else
      {
        if (outmax < 3)
            return 0;
        *result++ = '%';                /*  Some other escaped character     */
        *result++ = hex_char [(byte) code >> 4];
        *result++ = hex_char [(byte) code & 15];
        return (3);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_escape_size

    Synopsis: Returns the size of a string after HTTP escaping.  See the
    http_escape() function for details of the escaping algorithm.  Includes
    the null terminator in the returned size.
    ---------------------------------------------------------------------[>]-*/

size_t
http_escape_size (
    const char *string)
{
    size_t
        result_size = 1;                /*  Allow for null terminator        */

    ASSERT (string);
    while (*string)
      {
        if (isalnum (*string))          /*  Don't escape letters or digits   */
            result_size++;
        else
        if (*string == '\n' || *string == '\r')
          {
            if ((string [1] == '\n' || string [1] == '\r')
            &&  (string [1] != *string))
                string++;
            result_size += 6;           /*  Line ending becomes %0D%0A       */
          }
        else
            result_size += 3;           /*  Some other escaped character     */

        string++;
      }
    return (result_size);
}


/*  -------------------------------------------------------------------------
    Function: decode_hex

    Synopsis: Decodes a hexadecimal string.  Stops after outmax characters
    or when an invalid hex character is reached.  Sets the input pointer
    to the first unprocessed character.  Returns the result.
    -------------------------------------------------------------------------*/

int
decode_hex (
    const char   **input,
    size_t         outmax)
{
    static char
        hex_to_bin [128] = {
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,    /*   0..9     */
           -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*   A..F     */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*            */
           -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,    /*   a..f     */
           -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };  /*            */
    int
        nextch;
    size_t
        index,
        result;

    ASSERT (input);
    ASSERT (*input);

    index  = 0;
    result = 0;
    while (outmax == 0 || index < outmax)
      {
        nextch = (*input) [index] & 127;
        if (nextch && hex_to_bin [nextch] != -1)
          {
            result = result * 16 + hex_to_bin [nextch];
            index++;
          }
        else
            break;
      }
    (*input) += index;
    return (result);
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_escape_hex

    Synopsis: Works as http_escape(), but converts spaces to %20 instead of
    '+'.  This encoding is standard for filenames and URL requests.
    ---------------------------------------------------------------------[>]-*/

char *
http_escape_hex (
    const char *string,
    char  *result,
    size_t outmax)
{
    char
        *target;                        /*  Where we store the result        */
    size_t
        length;                         /*  of escaped character             */

    ASSERT (string);
    if (outmax == 0)                    /*  If no fixed length, get total len*/
        outmax = http_escape_size (string);

    if (result == NULL)
        if ((result = mem_alloc (outmax)) == NULL)
            return (NULL);              /*  Could not allocate a block       */

    if (outmax > 1)
        outmax -= 1;                    /*  Leave space for NULL terminator  */
    else
    if (outmax == 1)                    /*  Only room for terminator         */
      {
        *result = '\0';
        return (result);
      }
    target = result;
    while (*string)
      {
        length = http_escape_char (*string, target, outmax, TRUE);
        if (length == 0)
            break;
        
        target += length;
        outmax -= length;
        if (*string == '\n' || *string == '\r')
          {
            if ((string [1] == '\n' || string [1] == '\r')
            &&  (string [1] != *string))
                string++;
          }
        string++;
      }
    *target = '\0';                     /*  Terminate target string          */
    return (result);
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_unescape

    Synopsis: Removes HTTP escaping from a string.  See http_escape() for
    details of the escaping algorithm.  If the result string is NULL,
    modifies the source string in place, else fills-in the result string.
    Returns the resulting string.  End-of-line sequences (%0A%0D) are
    stored as a single new-line character, i.e. carriage-returns (%0D) are
    not stored.
    ---------------------------------------------------------------------[>]-*/

char *
http_unescape (
    char *string,
    char *result)
{
    char
        *target;                        /*  Where we store the result        */

    ASSERT (string);
    if (!result)                        /*  If result string is null,        */
        result = string;                /*    modify in place                */
    target = result;

    while (*string)
      {
        if (*string == '%'              /*  Unescape %xx sequence            */
        &&   string [1] && string [2])
          {
            string++;
            *target = decode_hex ((const char **) &string, 2);
            if (*target != '\r')
                target++;               /*  We do not store CRs              */
          }
        else
          {
            if (*string == '+')         /*  Spaces are escaped as '+'        */
                *target++ = ' ';
            else
                *target++ = *string;    /*  Otherwise just copy              */

            string++;
          }
      }
    *target = '\0';                     /*  Terminate target string          */
    return (result);
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_unescape_hex

    Synopsis: Removes HTTP hex escaping from a URL string, by expanding any
    sequences of characters %xx.
    ---------------------------------------------------------------------[>]-*/

char *
http_unescape_hex (
    char *string,
    char *result)
{
    char
        *target;                        /*  Where we store the result        */

    ASSERT (string);
    if (!result)                        /*  If result string is null,        */
        result = string;                /*    modify in place                */
    target = result;

    while (*string)
      {
        if (*string == '%'              /*  Unescape %xx sequence            */
        &&   string [1] && string [2])
          {
            string++;
            *target = decode_hex ((const char **) &string, 2);
            target++;             
          }
        else
          {
            *target++ = *string;        /*  Otherwise just copy              */
            string++;
          }
      }
    *target = '\0';                     /*  Terminate target string          */
    return (result);
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_query2strt

    Synopsis: Parses a HTTP query string, building an array of strings of
    the format "name=value".  The query string is assumed to be in escaped
    format, so http_unescape() is always applied to the query string.
    Within the query string, field=value pairs are delimited by & or ;.
    Returns a pointer to the array.  The array is allocated dynamically.
    The array ends with a NULL string.  To free the table, call strtfree().
    If there was not enough memory to allocate the table, returns NULL.
    ---------------------------------------------------------------------[>]-*/

char **
http_query2strt (
    const char *original_query)
{
    char
        *query,                         /*  Local copy of query string       */
        *query_ptr,                     /*  Pointer into query string        */
        *query_next,                    /*  Pointer to next query chunk      */
        **strings;                      /*  Returned string array            */
    int
        char_nbr,                       /*  Index into query string          */
        string_count,                   /*  Size of string table             */
        string_nbr;                     /*  Index into string table          */

    ASSERT (original_query);

    if (*original_query == '&')         /*  Skip leading & if present        */
        original_query++;

    if ((query = mem_strdup (original_query)) == NULL)
        return (NULL);                  /*  Could not allocate memory        */

    /*  Break query string at & and ; delimiters and count strt size         */
    string_count = 1;                   /*  Last string has no delimiter     */
    for (char_nbr = 0; original_query [char_nbr]; char_nbr++)
        if (query [char_nbr] == '&' || query [char_nbr] == ';')
          {
            query [char_nbr] = '\0';
            string_count++;
          }

    /*  Allocate the array of pointers with one slot for the final NULL      */
    if ((strings = mem_alloc (sizeof (char *) * (string_count + 1))) == NULL)
      {
        mem_free (query);
        return (NULL);                  /*  Could not allocate memory        */
      }

    /*  Query string now consists of a series of substrings, each ending in
     *  a null character.  We have to unescape each substring, which we do
     *  in-place: the unescaped string is never larger than the original
     *  string.
     */
    query_ptr = query;
    for (string_nbr = 0; string_nbr < string_count; string_nbr++)
      {
        /*  Unescape next query string component                             */
        query_next = query_ptr + strlen (query_ptr) + 1;
        http_unescape (query_ptr, NULL);

        /*  Allocate space for "name=value" plus final null char             */
        strings [string_nbr] = mem_strdup (query_ptr);
        query_ptr = query_next;
      }
    strings [string_nbr] = NULL;        /*  Store final null pointer         */
    mem_free (query);                   /*  Release temporary memory         */
    return (strings);
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_query2symb

    Synopsis: Parses a HTTP query string, and populates a symbol table with
    the resulting field values.  The query string is assumed to be escaped,
    so http_unescape() is always applied to the query string.  Within the
    query string, field=value pairs are delimited by & or ;.  Returns a
    SYMTAB pointer to the new table.  If there was not enough memory to
    allocate the table, returns NULL.
    ---------------------------------------------------------------------[>]-*/

SYMTAB *
http_query2symb (
    const char *query)
{
    char
        **strings;                      /*  Formatted string array           */
    SYMTAB
        *symtab;                        /*  Returned symbol table            */

    strings = http_query2strt (query);
    if (strings)
      {
        symtab = strt2symb (strings);
        strtfree (strings);
        return (symtab);
      }
    else
        return (NULL);                  /*  Couldn't create string table     */
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_query2descr

    Synopsis: Parses a HTTP query string, and returns the values as a DESCR
    block, composed of null-delimited strings with an empty string at the
    end.  See strt2descr() and http_query2symb() for more details.  Returns
    the address of the allocated descriptor, or NULL if there was not
    enough memory.
    ---------------------------------------------------------------------[>]-*/

DESCR *
http_query2descr (
    const char *query)
{
    char
        **strings;                      /*  Formatted string array           */
    DESCR
        *descr;                         /*  Returned descriptor              */

    strings = http_query2strt (query);
    if (strings)
      {
        descr = strt2descr (strings);
        strtfree (strings);
        return (descr);
      }
    else
        return (NULL);                  /*  Couldn't create string table     */
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_encode_meta

    Synopsis: Translates special characters into HTML/SGML metacharacters.
    The input buffer is not modified; you supply an output buffer and specify
    the maximum size of this buffer.  The input buffer must end in a null.
    Aftern calling, the input pointer is set to the character after the last
    encoded character.  Returns the final size of the translated data 
    excluding the final null byte.  If the resulting output data would be
    too long, translations stops.  If html is TRUE then the metacharacters
    amp, lt, gt and quot are not translated - this allows you to encode
    markup characters within HTML; otherwise they are translated and the
    output doesn't look like HTML.
    ---------------------------------------------------------------------[>]-*/

size_t
http_encode_meta (
    char    *output,
    char    **input,
    size_t  outmax,
    Bool    html)
{
    size_t
        space_left,                     /*  Space left in destination        */
        length;
    char
        *dest;                          /*  Pointer to result string         */

    ASSERT (input);
    ASSERT (*input);
    ASSERT (output);

    if (outmax == 0)                    /*  Special case for zero space      */
        return (0);

    space_left = outmax - 1;            /*  Allow for final null byte        */
    dest = output;
    while (**input && space_left > 0)
      {
        length = encode_meta_char (dest, **input, space_left, html);
        if (length)
          {
            space_left -= length;
            dest += length;
            (*input) ++;
          }
        else
            break;
      }
    *dest = '\0';
    return ((size_t) (dest - output));
}


/*  ---------------------------------------------------------------------[<]-
    Function: encode_meta_char

    Synopsis: Translates one character into HTML/SGML metacharacters.  You
    supply an output buffer and specify the maximum size of this buffer.
    Returns the final size of the translated data excluding the final null
    byte.  If the resulting data is too long, translation does not occur
    and the returned value is zero.  If html is TRUE then the metacharacters
    cr, amp, lt, gt and quot are not translated - this allows you to encode
    accented characters within HTML; otherwise they are translated and the
    output doesn't look like HTML.
    ---------------------------------------------------------------------[>]-*/

size_t
encode_meta_char (
    char  *output,
    char   code,
    size_t outmax,
    Bool   html)
{
    static char
        *meta [256];                    /*  Metacharacter translation table  */
    static Bool
        first_time = TRUE;              /*  First time flag                  */
    size_t
        length;                         /*  Length of translation            */
    char
        *meta_char,                     /*  Pointer through metachar string  */
        buffer [10];                    /*  Buffer for conversions           */

    /*  Initialise translation table first time through                      */
    if (first_time)
      {
        first_time = FALSE;
        memset (meta, 0, sizeof (meta));

#if (defined (__UNIX__) || defined (__WINDOWS__))
        /*  UNIX and Windows generally use ISO-8859-1 (Latin-1)              */
        meta [0x91] = "lsquo";
        meta [0x92] = "rsquo";
        meta [0xA1] = "iexcl";
        meta [0xA2] = "cent";
        meta [0xA3] = "pound";
        meta [0xA4] = "curren";
        meta [0xA5] = "yen";
        meta [0xA6] = "brvbar";
        meta [0xA7] = "sect";
        meta [0xA8] = "uml";
        meta [0xA9] = "copy";
        meta [0xAA] = "ordf";
        meta [0xAB] = "laquo";
        meta [0xAC] = "not";
        meta [0xAD] = "shy";
        meta [0xAE] = "reg";
        meta [0xAF] = "macr";
        meta [0xB0] = "deg";
        meta [0xB1] = "plusmn";
        meta [0xB2] = "sup2";
        meta [0xB3] = "sup3";
        meta [0xB4] = "acute";
        meta [0xB5] = "micro";
        meta [0xB6] = "para";
        meta [0xB7] = "middot";
        meta [0xB8] = "cedil";
        meta [0xB9] = "sup1";
        meta [0xBA] = "ordm";
        meta [0xBB] = "raquo";
        meta [0xBC] = "frac14";
        meta [0xBD] = "frac12";
        meta [0xBE] = "frac34";
        meta [0xBF] = "iquest";
        meta [0xC0] = "Agrave";
        meta [0xC1] = "Aacute";
        meta [0xC2] = "Acirc";
        meta [0xC3] = "Atilde";
        meta [0xC4] = "Auml";
        meta [0xC5] = "Aring";
        meta [0xC6] = "AElig";
        meta [0xC7] = "Ccedil";
        meta [0xC8] = "Egrave";
        meta [0xC9] = "Eacute";
        meta [0xCA] = "Ecirc";
        meta [0xCB] = "Euml";
        meta [0xCC] = "Igrave";
        meta [0xCD] = "Iacute";
        meta [0xCE] = "Icirc";
        meta [0xCF] = "Iuml";
        meta [0xD0] = "ETH";
        meta [0xD1] = "Ntilde";
        meta [0xD2] = "Ograve";
        meta [0xD3] = "Oacute";
        meta [0xD4] = "Ocirc";
        meta [0xD5] = "Otilde";
        meta [0xD6] = "Ouml";
        meta [0xD7] = "times";
        meta [0xD8] = "Oslash";
        meta [0xD9] = "Ugrave";
        meta [0xDA] = "Uacute";
        meta [0xDB] = "Ucirc";
        meta [0xDC] = "Uuml";
        meta [0xDD] = "Yacute";
        meta [0xDE] = "THORN";
        meta [0xDF] = "szlig";
        meta [0xE0] = "agrave";
        meta [0xE1] = "aacute";
        meta [0xE2] = "acirc";
        meta [0xE3] = "atilde";
        meta [0xE4] = "auml";
        meta [0xE5] = "aring";
        meta [0xE6] = "aelig";
        meta [0xE7] = "ccedil";
        meta [0xE8] = "egrave";
        meta [0xE9] = "eacute";
        meta [0xEA] = "ecirc";
        meta [0xEB] = "euml";
        meta [0xEC] = "igrave";
        meta [0xED] = "iacute";
        meta [0xEE] = "icirc";
        meta [0xEF] = "iuml";
        meta [0xF0] = "eth";
        meta [0xF1] = "ntilde";
        meta [0xF2] = "ograve";
        meta [0xF3] = "oacute";
        meta [0xF4] = "ocirc";
        meta [0xF5] = "otilde";
        meta [0xF6] = "ouml";
        meta [0xF7] = "divide";
        meta [0xF8] = "oslash";
        meta [0xF9] = "ugrave";
        meta [0xFA] = "uacute";
        meta [0xFB] = "ucirc";
        meta [0xFC] = "uuml";
        meta [0xFD] = "yacute";
        meta [0xFE] = "thorn";
        meta [0xFF] = "yuml";

#elif (defined (__MSDOS__))
        /*  DOS generally uses the PC-1 alphabet                             */
        meta [0x80] = "Ccedil";
        meta [0x81] = "uuml";
        meta [0x82] = "eacute";
        meta [0x83] = "acirc";
        meta [0x84] = "auml";
        meta [0x85] = "agrave";
        meta [0x86] = "aring";
        meta [0x87] = "ccedil";
        meta [0x88] = "ecirc";
        meta [0x89] = "euml";
        meta [0x8B] = "iuml";
        meta [0x8C] = "icirc";
        meta [0x8D] = "igrave";
        meta [0x8E] = "Auml";
        meta [0x8F] = "Aring";
        meta [0x90] = "Eacute";
        meta [0x91] = "aelig";
        meta [0x92] = "AElig";
        meta [0x93] = "ocirc";
        meta [0x94] = "ouml";
        meta [0x95] = "ograve";
        meta [0x96] = "ucirc";
        meta [0x97] = "ugrave";
        meta [0x98] = "yuml";
        meta [0x99] = "Ouml";
        meta [0x9A] = "Uuml";
        meta [0x9B] = "oslash";
        meta [0x9C] = "pound";
        meta [0x9D] = "Oslash";
        meta [0x9E] = "times";
        meta [0xA0] = "aacute";
        meta [0xA1] = "iacute";
        meta [0xA2] = "otilde";
        meta [0xA3] = "uacute";
        meta [0xA4] = "ntilde";
        meta [0xA5] = "Ntilde";
        meta [0xA6] = "ordf";
        meta [0xA7] = "ordm";
        meta [0xA8] = "iquest";
        meta [0xA9] = "reg";
        meta [0xAA] = "not";
        meta [0xAB] = "frac14";
        meta [0xAC] = "frac12";
        meta [0xAD] = "iexcl";
        meta [0xAE] = "laquo";
        meta [0xAF] = "raquo";
        meta [0xB0] = "shy";
        meta [0xB5] = "Aacute";
        meta [0xB6] = "Acirc";
        meta [0xB7] = "Agrave";
        meta [0xB8] = "copy";
        meta [0xC6] = "atilde";
        meta [0xC7] = "Atilde";
        meta [0xCA] = "egrave";
        meta [0xCF] = "curren";
        meta [0xD1] = "ETH";
        meta [0xD2] = "Ecirc";
        meta [0xD3] = "Euml";
        meta [0xD4] = "Egrave";
        meta [0xD6] = "Iacute";
        meta [0xD7] = "Icirc";
        meta [0xD8] = "Iuml";
        meta [0xDD] = "brvbar";
        meta [0xDE] = "Igrave";
        meta [0xE0] = "Oacute";
        meta [0xE1] = "szlig";
        meta [0xE2] = "Ocirc";
        meta [0xE3] = "Ograve";
        meta [0xE5] = "Otilde";
        meta [0xE6] = "THORN";
        meta [0xE9] = "Uacute";
        meta [0xEA] = "Ucirc";
        meta [0xEB] = "Ugrave";
        meta [0xEC] = "yacute";
        meta [0xED] = "Yacute";
        meta [0xEE] = "macr";
        meta [0xEF] = "acute";
        meta [0xF1] = "plusmn";
        meta [0xF3] = "frac34";
        meta [0xF4] = "para";
        meta [0xF5] = "sect";
        meta [0xF6] = "divide";
        meta [0xF8] = "deg";
        meta [0xFA] = "middot";
        meta [0xFB] = "sup1";
        meta [0xFC] = "sup3";
        meta [0xFD] = "sup2";
#endif
      }
   
    meta_char = meta [(int) code & 255];
    if (meta_char == 0)
      {
        if (html)
          {
            output [0] = code;
            return (1);
          } 
        else
          {
            switch (code)
              {
                case  '&':
                    meta_char = "amp";
                    break;
                case  '<':
                    meta_char = "lt";
                    break;
                case  '>':
                    meta_char = "gt";
                    break;
                case  '"':
                    meta_char = "quot";
                    break;
                default:
                    output [0] = code;
                    return (1);
              }
          }
      }
    snprintf (buffer, sizeof (buffer), "&%s;", meta_char);
    length = strlen (buffer);
    if (length < outmax)
      {
        strncpy (output, buffer, outmax);
        return length;
      }
    else
      {
        output [0] = 0;
        return (0);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_decode_meta

    Synopsis: Translates special characters from HTML/SGML
    metacharacters.  The input buffer is not modified; you supply an
    output buffer and specify the maximum size of this buffer.  The input
    buffer must end in a null.  The two buffers may be the same, since
    decoding reduces the size of the data.  Returns the final size of the
    translated data excluding the final null byte.  If the resulting data
    is too long, it is brutally truncated.  Invalid meta-characters are
    left as-is.  Normally the input pointer points to the final null;
    however if the input string runs out in the middle of a
    meta-character, the input pointer is left pointing at the start of
    that meta-character (the '&').  If the two buffers are the same, it
    may be the case that this character has been overwritten with the
    terminating null.
    ---------------------------------------------------------------------[>]-*/

size_t
http_decode_meta (
    char  *output,
    char **input,
    size_t outmax)
{
    char
        *dest,                          /*  Pointer to result string         */
        *end,                           /*  Next character after meta-char   */
        code;                           /*  Decoded metachar string          */
    size_t
        space_left;                     /*  Space left in destination        */

    ASSERT (input);
    ASSERT (*input);
    ASSERT (output);

    if (outmax == 0)                    /*  Special case for zero space      */
        return (0);

    space_left = outmax - 1;            /*  Allow for final null byte        */
    dest = output;
    for (; **input; (*input)++)
      {
        if (**input == '&')
          {
            end = strchr (*input, ';');
            if (end)
              {
                code = decode_meta_charn ((*input) + 1, end - (*input) - 1);
                if (code)
                    *input = end;       /*  Skip past decoded metachar       */
                else
                    code = **input;     /*  Ignore the &, no valid metachar  */
              }
            else                        /*  The string ends before the ';'   */
            if (strlen (*input) > 10)
                code = **input;         /*  Ignore the &, no valid metachar  */
            else
                break;
          }
        else
            code = **input;

        if (space_left > 0)
          {
            *dest++ = code;
            space_left --;
          }
      }
    *dest = '\0';
    return ((size_t) (dest - output));
}


/*  ---------------------------------------------------------------------[<]-
    Function: decode_meta_charn

    Synopsis: Decodes a single meta-character (the piece from the character
    after the '&' up to but not including the ';'.  If the meta-character
    is valid, returns the character; otherwise returns zero.  Decodes both
    named and numeric meta-characters.  Use the macro decode_meta_char if
    the input has a terminating zero.
    ---------------------------------------------------------------------[>]-*/

char
decode_meta_charn (const char *input, size_t length)
{
#define META_COUNT 102

    static char
        *meta [META_COUNT];             /*  Metacharacter translation tables */
    static byte
        code [META_COUNT];
    static Bool
        first_time = TRUE;              /*  First time flag                  */
    int
        char_index,                     /*  Index into translation table     */
        min,                            /*  Pointers for binary chop         */
        max,
        cmp,
        num;

    /*  Initialise translation table first time through                      */
    if (first_time)
      {
        first_time = FALSE;

        /*  The meta-characters must be in alphabetical order so we can use  */
        /*  a binary chop.                                                   */
#if (defined (__UNIX__) || defined (__WINDOWS__))
        /*  UNIX and Windows generally use ISO-8859-1 (Latin-1)              */
        code [  0] = 0xC6;  meta [  0] = "AElig";
        code [  1] = 0xC1;  meta [  1] = "Aacute";
        code [  2] = 0xC2;  meta [  2] = "Acirc";
        code [  3] = 0xC0;  meta [  3] = "Agrave";
        code [  4] = 0xC5;  meta [  4] = "Aring";
        code [  5] = 0xC3;  meta [  5] = "Atilde";
        code [  6] = 0xC4;  meta [  6] = "Auml";
        code [  7] = 0xC7;  meta [  7] = "Ccedil";
        code [  8] = 0xD0;  meta [  8] = "ETH";
        code [  9] = 0xC9;  meta [  9] = "Eacute";
        code [ 10] = 0xCA;  meta [ 10] = "Ecirc";
        code [ 11] = 0xC8;  meta [ 11] = "Egrave";
        code [ 12] = 0xCB;  meta [ 12] = "Euml";
        code [ 13] = 0xCD;  meta [ 13] = "Iacute";
        code [ 14] = 0xCE;  meta [ 14] = "Icirc";
        code [ 15] = 0xCC;  meta [ 15] = "Igrave";
        code [ 16] = 0xCF;  meta [ 16] = "Iuml";
        code [ 17] = 0xD1;  meta [ 17] = "Ntilde";
        code [ 18] = 0xD3;  meta [ 18] = "Oacute";
        code [ 19] = 0xD4;  meta [ 19] = "Ocirc";
        code [ 20] = 0xD2;  meta [ 20] = "Ograve";
        code [ 21] = 0xD8;  meta [ 21] = "Oslash";
        code [ 22] = 0xD5;  meta [ 22] = "Otilde";
        code [ 23] = 0xD6;  meta [ 23] = "Ouml";
        code [ 24] = 0xDE;  meta [ 24] = "THORN";
        code [ 25] = 0xDA;  meta [ 25] = "Uacute";
        code [ 26] = 0xDB;  meta [ 26] = "Ucirc";
        code [ 27] = 0xD9;  meta [ 27] = "Ugrave";
        code [ 28] = 0xDC;  meta [ 28] = "Uuml";
        code [ 29] = 0xDD;  meta [ 29] = "Yacute";
        code [ 30] = 0xE1;  meta [ 30] = "aacute";
        code [ 31] = 0xE2;  meta [ 31] = "acirc";
        code [ 32] = 0xB4;  meta [ 32] = "acute";
        code [ 33] = 0xE6;  meta [ 33] = "aelig";
        code [ 34] = 0xE0;  meta [ 34] = "agrave";
        code [ 35] = '&';   meta [ 35] = "amp";
        code [ 36] = 0xE5;  meta [ 36] = "aring";
        code [ 37] = 0xE3;  meta [ 37] = "atilde";
        code [ 38] = 0xE4;  meta [ 38] = "auml";
        code [ 39] = 0xA6;  meta [ 39] = "brvbar";
        code [ 40] = 0xE7;  meta [ 40] = "ccedil";
        code [ 41] = 0xB8;  meta [ 41] = "cedil";
        code [ 42] = 0xA2;  meta [ 42] = "cent";
        code [ 43] = 0xA9;  meta [ 43] = "copy";
        code [ 44] = 0xA4;  meta [ 44] = "curren";
        code [ 45] = 0xB0;  meta [ 45] = "deg";
        code [ 46] = 0xF7;  meta [ 46] = "divide";
        code [ 47] = 0xE9;  meta [ 47] = "eacute";
        code [ 48] = 0xEA;  meta [ 48] = "ecirc";
        code [ 49] = 0xE8;  meta [ 49] = "egrave";
        code [ 50] = 0xF0;  meta [ 50] = "eth";
        code [ 51] = 0xEB;  meta [ 51] = "euml";
        code [ 52] = 0xBD;  meta [ 52] = "frac12";
        code [ 53] = 0xBC;  meta [ 53] = "frac14";
        code [ 54] = 0xBE;  meta [ 54] = "frac34";
        code [ 55] = '>';   meta [ 55] = "gt";
        code [ 56] = 0xED;  meta [ 56] = "iacute";
        code [ 57] = 0xEE;  meta [ 57] = "icirc";
        code [ 58] = 0xA1;  meta [ 58] = "iexcl";
        code [ 59] = 0xEC;  meta [ 59] = "igrave";
        code [ 60] = 0xBF;  meta [ 60] = "iquest";
        code [ 61] = 0xEF;  meta [ 61] = "iuml";
        code [ 62] = 0xAB;  meta [ 62] = "laquo";
        code [ 63] = 0x91;  meta [ 63] = "lsquo";
        code [ 64] = '<';   meta [ 64] = "lt";
        code [ 65] = 0xAF;  meta [ 65] = "macr";
        code [ 66] = 0xB5;  meta [ 66] = "micro";
        code [ 67] = 0xB7;  meta [ 67] = "middot";
        code [ 68] = ' ';   meta [ 68] = "nbsp";
        code [ 69] = 0xAC;  meta [ 69] = "not";
        code [ 70] = 0xF1;  meta [ 70] = "ntilde";
        code [ 71] = 0xF3;  meta [ 71] = "oacute";
        code [ 72] = 0xF4;  meta [ 72] = "ocirc";
        code [ 73] = 0xF2;  meta [ 73] = "ograve";
        code [ 74] = 0xAA;  meta [ 74] = "ordf";
        code [ 75] = 0xBA;  meta [ 75] = "ordm";
        code [ 76] = 0xF8;  meta [ 76] = "oslash";
        code [ 77] = 0xF5;  meta [ 77] = "otilde";
        code [ 78] = 0xF6;  meta [ 78] = "ouml";
        code [ 79] = 0xB6;  meta [ 79] = "para";
        code [ 80] = 0xB1;  meta [ 80] = "plusmn";
        code [ 81] = 0xA3;  meta [ 81] = "pound";
        code [ 82] = '"';   meta [ 82] = "quot";
        code [ 83] = 0xBB;  meta [ 83] = "raquo";
        code [ 84] = 0xAE;  meta [ 84] = "reg";
        code [ 85] = 0x92;  meta [ 85] = "rsquo";
        code [ 86] = 0xA7;  meta [ 86] = "sect";
        code [ 87] = 0xAD;  meta [ 87] = "shy";
        code [ 88] = 0xB9;  meta [ 88] = "sup1";
        code [ 89] = 0xB2;  meta [ 89] = "sup2";
        code [ 90] = 0xB3;  meta [ 90] = "sup3";
        code [ 91] = 0xDF;  meta [ 91] = "szlig";
        code [ 92] = 0xFE;  meta [ 92] = "thorn";
        code [ 93] = 0xD7;  meta [ 93] = "times";
        code [ 94] = 0xFA;  meta [ 94] = "uacute";
        code [ 95] = 0xFB;  meta [ 95] = "ucirc";
        code [ 96] = 0xF9;  meta [ 96] = "ugrave";
        code [ 97] = 0xA8;  meta [ 97] = "uml";
        code [ 98] = 0xFC;  meta [ 98] = "uuml";
        code [ 99] = 0xFD;  meta [ 99] = "yacute";
        code [100] = 0xA5;  meta [100] = "yen";
        code [101] = 0xFF;  meta [101] = "yuml";
#elif (defined (__MSDOS__))
        code [  0] = 0x92;  meta [  0] = "AElig";
        code [  1] = 0xB5;  meta [  1] = "Aacute";
        code [  2] = 0xB6;  meta [  2] = "Acirc";
        code [  3] = 0xB7;  meta [  3] = "Agrave";
        code [  4] = 0x8F;  meta [  4] = "Aring";
        code [  5] = 0xC7;  meta [  5] = "Atilde";
        code [  6] = 0x8E;  meta [  6] = "Auml";
        code [  7] = 0x80;  meta [  7] = "Ccedil";
        code [  8] = 0xD1;  meta [  8] = "ETH";
        code [  9] = 0x90;  meta [  9] = "Eacute";
        code [ 10] = 0xD2;  meta [ 10] = "Ecirc";
        code [ 11] = 0xD4;  meta [ 11] = "Egrave";
        code [ 12] = 0xD3;  meta [ 12] = "Euml";
        code [ 13] = 0xD6;  meta [ 13] = "Iacute";
        code [ 14] = 0xD7;  meta [ 14] = "Icirc";
        code [ 15] = 0xDE;  meta [ 15] = "Igrave";
        code [ 16] = 0xD8;  meta [ 16] = "Iuml";
        code [ 17] = 0xA5;  meta [ 17] = "Ntilde";
        code [ 18] = 0xE0;  meta [ 18] = "Oacute";
        code [ 19] = 0xE2;  meta [ 19] = "Ocirc";
        code [ 20] = 0xE3;  meta [ 20] = "Ograve";
        code [ 21] = 0x9D;  meta [ 21] = "Oslash";
        code [ 22] = 0xE5;  meta [ 22] = "Otilde";
        code [ 23] = 0x99;  meta [ 23] = "Ouml";
        code [ 24] = 0xE6;  meta [ 24] = "THORN";
        code [ 25] = 0xE9;  meta [ 25] = "Uacute";
        code [ 26] = 0xEA;  meta [ 26] = "Ucirc";
        code [ 27] = 0xEB;  meta [ 27] = "Ugrave";
        code [ 28] = 0x9A;  meta [ 28] = "Uuml";
        code [ 29] = 0xED;  meta [ 29] = "Yacute";
        code [ 30] = 0xA0;  meta [ 30] = "aacute";
        code [ 31] = 0x83;  meta [ 31] = "acirc";
        code [ 32] = 0xEF;  meta [ 32] = "acute";
        code [ 33] = 0x91;  meta [ 33] = "aelig";
        code [ 34] = 0x85;  meta [ 34] = "agrave";
        code [ 35] = '&';   meta [ 35] = "amp";
        code [ 36] = 0x86;  meta [ 36] = "aring";
        code [ 37] = 0xC6;  meta [ 37] = "atilde";
        code [ 38] = 0x84;  meta [ 38] = "auml";
        code [ 39] = 0xDD;  meta [ 39] = "brvbar";
        code [ 40] = 0x87;  meta [ 40] = "ccedil";
        code [ 41] = 0xB8;  meta [ 41] = "cedil";
        code [ 42] = 0xA2;  meta [ 42] = "cent";
        code [ 43] = 0xB8;  meta [ 43] = "copy";
        code [ 44] = 0xCF;  meta [ 44] = "curren";
        code [ 45] = 0xF8;  meta [ 45] = "deg";
        code [ 46] = 0xF6;  meta [ 46] = "divide";
        code [ 47] = 0x82;  meta [ 47] = "eacute";
        code [ 48] = 0x88;  meta [ 48] = "ecirc";
        code [ 49] = 0xCA;  meta [ 49] = "egrave";
        code [ 50] = 0xF0;  meta [ 50] = "eth";
        code [ 51] = 0x89;  meta [ 51] = "euml";
        code [ 52] = 0xAC;  meta [ 52] = "frac12";
        code [ 53] = 0xAB;  meta [ 53] = "frac14";
        code [ 54] = 0xF3;  meta [ 54] = "frac34";
        code [ 55] = '>';   meta [ 55] = "gt";
        code [ 56] = 0xA1;  meta [ 56] = "iacute";
        code [ 57] = 0x8C;  meta [ 57] = "icirc";
        code [ 58] = 0xAD;  meta [ 58] = "iexcl";
        code [ 59] = 0x8D;  meta [ 59] = "igrave";
        code [ 60] = 0xA8;  meta [ 60] = "iquest";
        code [ 61] = 0x8B;  meta [ 61] = "iuml";
        code [ 62] = 0xAE;  meta [ 62] = "laquo";
        code [ 63] = 0x91;  meta [ 63] = "lsquo";
        code [ 64] = '<';   meta [ 64] = "lt";
        code [ 65] = 0xEE;  meta [ 65] = "macr";
        code [ 66] = 0xB5;  meta [ 66] = "micro";
        code [ 67] = 0xFA;  meta [ 67] = "middot";
        code [ 68] = ' ';   meta [ 68] = "nbsp";
        code [ 69] = 0xAA;  meta [ 69] = "not";
        code [ 70] = 0xA4;  meta [ 70] = "ntilde";
        code [ 71] = 0xF3;  meta [ 71] = "oacute";
        code [ 72] = 0x93;  meta [ 72] = "ocirc";
        code [ 73] = 0x95;  meta [ 73] = "ograve";
        code [ 74] = 0xA6;  meta [ 74] = "ordf";
        code [ 75] = 0xA7;  meta [ 75] = "ordm";
        code [ 76] = 0x9B;  meta [ 76] = "oslash";
        code [ 77] = 0xA2;  meta [ 77] = "otilde";
        code [ 78] = 0x94;  meta [ 78] = "ouml";
        code [ 79] = 0xF4;  meta [ 79] = "para";
        code [ 80] = 0xF1;  meta [ 80] = "plusmn";
        code [ 81] = 0x9C;  meta [ 81] = "pound";
        code [ 82] = '"';   meta [ 82] = "quot";
        code [ 83] = 0xAF;  meta [ 83] = "raquo";
        code [ 84] = 0xA9;  meta [ 84] = "reg";
        code [ 85] = 0x92;  meta [ 85] = "rsquo";
        code [ 86] = 0xF5;  meta [ 86] = "sect";
        code [ 87] = 0xB0;  meta [ 87] = "shy";
        code [ 88] = 0xFB;  meta [ 88] = "sup1";
        code [ 89] = 0xFD;  meta [ 89] = "sup2";
        code [ 90] = 0xFC;  meta [ 90] = "sup3";
        code [ 91] = 0xE1;  meta [ 91] = "szlig";
        code [ 92] = 0xFE;  meta [ 92] = "thorn";
        code [ 93] = 0x9E;  meta [ 93] = "times";
        code [ 94] = 0xA3;  meta [ 94] = "uacute";
        code [ 95] = 0x96;  meta [ 95] = "ucirc";
        code [ 96] = 0x97;  meta [ 96] = "ugrave";
        code [ 97] = 0xA8;  meta [ 97] = "uml";
        code [ 98] = 0x81;  meta [ 98] = "uuml";
        code [ 99] = 0xEC;  meta [ 99] = "yacute";
        code [100] = 0xA5;  meta [100] = "yen";
        code [101] = 0x98;  meta [101] = "yuml";
#endif
      }

    if (*input == '#')    /*  Numeric translation  */
      {
        input++;
        num = 0;
        if (*input == 'x')  /*  Hex  */
          {
            input++;
            num = decode_hex (&input, 0);
            input++;
          }
        else
            FOREVER
              {
                if ((*input >= '0') && (*input <= '9'))
                    num = (num * 10) + *input - '0';
                else
                    break;

                input++;
              }

        if (*input != ';')
            num = 0;

        return num;
      }
    else  /*  Lookup meta-character  */
      {
        min = 0;
        max = META_COUNT;
        while (max > min)
          {
            char_index = (max + min) / 2;
            cmp = strncmp (input, meta [char_index], length);
            if (cmp == 0)
                return code [char_index];

            if (cmp > 0)
                min = char_index + 1;
            else
                max = char_index;
          }

        return 0;
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: cgi_parse_query_vars

    Synopsis: Parses a CGI query string and loads the resulting variables
    into an existing symbol table, optionally prefixing each name with a
    string.  Returns the number of variables loaded.  The prefix can be
    NULL or empty if not required.
    ---------------------------------------------------------------------[>]-*/

int
cgi_parse_query_vars (
    SYMTAB *symtab,
    const char *query,
    const char *prefix)
{
    char
        *query_var,                     /*  Query variable name              */
        **query_vars,                   /*  Query as string table            */
        *equals;                        /*  Equal sign in variable           */
    int
        string_nbr,                     /*  Index into string table          */
        variables = 0;                  /*  Number of variables loaded       */

    ASSERT (symtab);
    if ((query_vars = http_query2strt (query)) == NULL)
        return (0);                     /*  Not enough memory                */

    for (string_nbr = 0; query_vars [string_nbr]; string_nbr++)
      {
        equals = strchr (query_vars [string_nbr], '=');
        if (equals)
          {
            *equals = '\0';             /*  Cut into two strings             */
            if (prefix == NULL)
                prefix = "";            /*  Make safe for xstrcpy()          */
            query_var = xstrcpy (NULL, prefix, query_vars [string_nbr], NULL);
            sym_assume_symbol (symtab, query_var, equals + 1);
            mem_strfree (&query_var);
            *equals = '=';              /*  Restore previous state           */
            variables++;                /*  Count this variable              */
          }
      }
    strtfree (query_vars);
    return (variables);
}


/*  ---------------------------------------------------------------------[<]-
    Function: cgi_parse_file_vars

    Synopsis: Parses a CGI query string stored in a file, and loads the
    resulting variables into an existing symbol table, optionally
    prefixing each name with a string.  Returns the number of variables
    loaded.  The prefix can be NULL or empty if not required.  The
    file data is assumed to be escaped (see http_escape()); the data
    should not contain line breaks, spaces, or other unescaped chars.
    The file should already have been opened: a typical use for this
    function is to parse the values supplied in stdin.  The maximum size
    for the file is CGI_QUERY_FILE_MAX characters.
    ---------------------------------------------------------------------[>]-*/

int
cgi_parse_file_vars (
    SYMTAB *symtab,
    FILE   *file,
    const char *prefix,
    size_t size)
{
    /*  Maximum size of a stream of HTTP query data coming from a file       */
#   define CGI_QUERY_FILE_MAX  65535U
    char
        *query;                         /*  Data loaded from file            */
    size_t
        read_size;                      /*  Amount of data read from file    */
    int
        variables = 0;                  /*  Number of variables loaded       */

    ASSERT (file);
    ASSERT (symtab);
    ASSERT (size <= CGI_QUERY_FILE_MAX);

    if ((query = mem_alloc (size + 1)) == NULL)
        return (0);

    read_size = fread (query, 1, size, file);
    query [read_size] = '\0';
    variables = cgi_parse_query_vars (symtab, query, prefix);
    mem_free (query);
    return (variables);
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_multipart_decode

    Synopsis: Parses a multipart-encoded file (as received by a web server as
    POST data) and returns a HTTP-encoded string containing the field data,
    in the format: "name=value&name=value&name=value...".  For each field that
    refers to an uploaded file (INPUT field with type FILE), creates a
    temporary file holding the data.  The name of this temporary file is put
    into a generated variable, whose name is built by using the local format
    string (ex: '%s_tmp').  The actual uploaded file is stored in
    a temporary file whose name is generated by the SFL get_tmp_file_name()
    function.  So, data for a file upload field called "doc" will be stored in
    a temporary file called (eg) "temp1234.tmp", and a field "doc_tmp" will be
    added, with the value "temp1234.tmp".  The HTTP-encoded string is returned
    as a DESCR block, which you can decode using http_query2strt(), passing the
    descriptor data.  You must free the descriptor using mem_free() when you're
    finished with it.
    ---------------------------------------------------------------------[>]-*/

DESCR *
http_multipart_decode (const char *mime_file, const char *store_path,
                       const char *local_format)
{
    FILE
        *f_source,
        *f_tmp = NULL;
    char
        *tmp_name = NULL,
        *p_head,
        *p_data,
        *p_next,
        *buffer;
    int
        offset,
        read_size,
        rest_read_size;
    static char
        separator [80 + 1];
    static int
        sep_size;
    SYMTAB
        *table,
        *header_tab;
    qbyte
        tmp_index = 1;
    DESCR
        *descr = NULL;

    ASSERT (local_format);

    if (strstr (local_format, "%s") == NULL)
        return (NULL);

    buffer = mem_alloc (MULTI_BUFFER_SIZE + REST_BUFFER_SIZE + 1);
    if (buffer == NULL)
        return (NULL);

    table = sym_create_table ();
    if (table == NULL)
      {
        mem_free (buffer);
        return (NULL);
      }

    header_tab = sym_create_table ();
    if (header_tab == NULL)
      {
        mem_free (buffer);
        sym_delete_table (table);
        return (NULL);
      }

    f_source = fopen (mime_file, "rb");
    if (f_source == NULL)
      {
        mem_free (buffer);
        sym_delete_table (table);
        sym_delete_table (header_tab);
        return (NULL);
      }

    memset (separator, 0, sizeof (separator));
    separator [0] = 0x0D;
    separator [1] = 0x0A;
    fgets (&separator [2], 78, f_source);
    strconvch (&separator [2] , '\r', '\0');
    strconvch (&separator [2] , '\n', '\0');
    sep_size  = strlen (separator);

    read_size = fread (buffer, 1, MULTI_BUFFER_SIZE, f_source);
    p_next = buffer;
    while (read_size > 0)
      {
        sym_empty_table (header_tab);
        p_head = p_next;
        p_data = (char *) memfind ((byte *) p_head,
                          MULTI_BUFFER_SIZE + REST_BUFFER_SIZE - (p_head - buffer),
                          (byte *) "\r\n\r\n", 4, FALSE);
        if (p_data)
          {
            *p_data = '\0';
            p_data += 4;
          }
        if (p_head)
          {
            multipart_decode_header (p_head, header_tab);
            if (sym_lookup_symbol (header_tab, "filename") != NULL)
              {
                if (f_tmp != NULL)
                  {
                    ASSERT (tmp_name != NULL);
                    fclose (f_tmp);
                    f_tmp = NULL;
                    if (get_file_size (tmp_name) == 0)
                        file_delete (tmp_name);
                  }
                tmp_name = get_tmp_file_name (store_path, &tmp_index, "tmp");
                f_tmp = fopen (tmp_name, "wb");
              }
          }
        p_next = (char *) memfind ((byte *) p_data,
                          /*  read_size                                      */
                          MULTI_BUFFER_SIZE + REST_BUFFER_SIZE - (p_data - buffer),
                          (byte *) separator, sep_size, FALSE);

        if (p_next != NULL)
          {
            /*  The next attribute-value pair was found                      */
            *p_next = '\0';
            save_multipart_header (table, header_tab, p_data, tmp_name,
                                   local_format);
            if (f_tmp)
              {
                /* Special case:
                   The next attribute value is found AND "f_tmp" is opened.
                   That means that the download file is completely in the
                   buffer and so we can save it to disk.
                 */
                fwrite (p_data, p_next - p_data, 1, f_tmp);
                fclose (f_tmp);
                f_tmp = NULL;
                if (get_file_size (tmp_name) == 0)
                    file_delete (tmp_name);

                /* Now we need to check the buffer.  If the buffer is full
                   then we must read the rest of "f_source". The rest will
                   be written into the the buffer at REST_BUFFER_SIZE.
                 */
                if (read_size == MULTI_BUFFER_SIZE)
                    rest_read_size = fread (&buffer [MULTI_BUFFER_SIZE], 1,
                                            REST_BUFFER_SIZE, f_source);
              }
            p_next += sep_size;

            /*  Found end of file marker                                     */
            if (*p_next == '-' && *(p_next + 1) == '-')
              {
                if (f_tmp)
                  {
                    fclose (f_tmp);
                    f_tmp = NULL;
                    if (get_file_size (tmp_name) == 0)
                        file_delete (tmp_name);
                  }
                break;
              }
            else
                while (*p_next == '\r' || *p_next == '\n')
                    p_next++;
          }
        else
          {
            if (f_tmp)
                fwrite (p_data, &buffer [read_size - sep_size ] - p_data,
                        1, f_tmp);
            offset = 0;
            while (read_size > 0 && p_next == NULL)
              {
                memmove (buffer, &buffer [read_size - sep_size + offset ],
                                 sep_size);
                read_size = fread (&buffer [sep_size], 1,
                                   MULTI_BUFFER_SIZE - sep_size, f_source);
                p_next = (char *) memfind ((byte *) buffer,
                                  read_size + sep_size,
                                  (byte *) separator, sep_size, FALSE);               
                if (p_next != NULL)
                  {
                    /*  We need to test if the buffer is large enough.  If
                        not, we use the additional buffer size starting at
                        position REST_BUFFER_SIZE.
                     */
                    if (read_size == (MULTI_BUFFER_SIZE - sep_size))
                      {
                        /* Attach the rest of the file */
                        rest_read_size = fread (&buffer [MULTI_BUFFER_SIZE], 1,
                                                REST_BUFFER_SIZE, f_source);
                      }
   
                    *p_next = '\0';
                    save_multipart_header (table, header_tab,
                                           p_data, tmp_name, local_format);
                    if (f_tmp)
                      {
                        fwrite (buffer, p_next - buffer, 1, f_tmp);
                        fclose (f_tmp);
                        f_tmp = NULL;
                        if (get_file_size (tmp_name) == 0)
                            file_delete (tmp_name);
                      }
                    p_next += sep_size;

                   /*  Found end of file marker                              */
                   if (*p_next == '-' && *(p_next + 1) == '-')
                     {
                       read_size = 0;
                       break;
                     }
                   else
                       while (*p_next == '\r' || *p_next == '\n')
                           p_next++;
                   read_size += sep_size;
                  }
                else
                  {
                    /*  We did not find the next separator, so we write the
                        file and continue.
                     */
                    if (f_tmp)
                        fwrite (buffer, read_size, 1, f_tmp);
                    offset = sep_size;
                  }

              }
          }
      }
    if (f_tmp)
      {
        fclose (f_tmp);
        if (get_file_size (tmp_name) == 0)
            file_delete (tmp_name);
      }
    sym_delete_table (header_tab);
    fclose (f_source);
    mem_free (buffer);

    descr = http_multipart2url (table);
    sym_delete_table (table);

    return (descr);
}


/*  -------------------------------------------------------------------------
    Function: http_multipart2url

    Synopsis: Convert a symbol table to a string in format :
              name=value&name=value&name=value ....
    -------------------------------------------------------------------------*/

static DESCR *
http_multipart2url  (const SYMTAB *symtab)
{
    DESCR
        *descr;                         /*  Formatted descriptor             */
    SYMBOL
        *symbol;                        /*  Pointer to symbol                */
    char
        *p_char,
        *p_val;
    qbyte
        buffer_length = 0;

    if (!symtab)
        return (NULL);                  /*  Return NULL if argument is null  */

    /*  Calculate length of buffer                                           */
    for (symbol = symtab-> symbols; symbol; symbol = symbol-> next)
        buffer_length += strlen (symbol-> name) + strlen (symbol-> value) + 2;

    /*  On 16-bit systems, check that buffer will fit into max. block size   */
#   if (UINT_MAX == 0xFFFFU)
    if (buffer_length > UINT_MAX - 32)
        return (NULL);
#   endif

    /*  Now allocate the descriptor                                          */
    descr = mem_descr (NULL, (size_t) buffer_length + 1);
    if (descr == NULL)
        return (NULL);

    p_char = (char *) descr-> data;
    for (symbol = symtab-> symbols; symbol; symbol = symbol-> next)
      {
        if (symbol != symtab-> symbols)
            *p_char++ = '&';
        p_val = symbol-> name;
        while (*p_val)
            *p_char++ = *p_val++;
        *p_char++ = '=';
        p_val = symbol-> value;
        while (*p_val)
            *p_char++ = *p_val++;
      }
    *p_char = '\0';

    return (descr);
}


/*  -------------------------------------------------------------------------
    Function: save_multipart_header

    Synopsis: Store field name and value in symbol table.
    -------------------------------------------------------------------------*/

static void
save_multipart_header (SYMTAB *table, SYMTAB *header, char *data,
                       char *tmp_name, const char *local_format)
{
    SYMBOL
        *sym_filename,
        *symbol;
    char
        *copyfmt,
        *percent,
        *tmp_val,
        *value;

    /* Check if is a file uploaded or form field                             */
    if ((sym_filename = sym_lookup_symbol (header, "filename")) != NULL)
      {
        symbol = sym_lookup_symbol (header, "name");
        if (symbol)
          {
            value = http_escape (sym_filename-> value, NULL, 0);
            if (value)
              {
                sym_assume_symbol (table, symbol-> value, value);
                mem_free (value);
              }
            /*  Construct temporary name by replacing %s in the format string
             *  by the variable name.  We don't use sprintf because it's too
             *  dangerous here: the format string can cause havoc.
             */
            copyfmt = mem_strdup (local_format);
            percent = strchr (copyfmt, '%');
            if (percent && percent [1] == 's')
              {
                percent [0] = '\0';
                tmp_val = xstrcpy (NULL, copyfmt, symbol-> value, percent + 2, NULL);
              }
            else
                tmp_val = xstrcpy (NULL, symbol-> value, "_tmp", NULL);

            value = http_escape (tmp_name, NULL, 0);
            if (value)
              {
                sym_assume_symbol (table, tmp_val, value);
                mem_free (value);
              }
            mem_strfree (&tmp_val);
            mem_strfree (&copyfmt);
          }
      }
    else
      {
        symbol = sym_lookup_symbol (header, "name");
        if (symbol)
          {
            value = http_escape (data, NULL, 0);
            if (value)
              {
                sym_assume_symbol (table, symbol-> value, value);
                mem_free (value);
              }
          }
      }
}


/*  -------------------------------------------------------------------------
    Function: multipart_decode_header

    Synopsis: Decode mime header of a part.
    -------------------------------------------------------------------------*/

static void
multipart_decode_header (char *header, SYMTAB *table)
{
    char
        *p,
        *variable,
        *value = "";

    ASSERT (header);
    ASSERT (table);

    p        = header;
    variable = header;
    while (*p)
      {
        if ((*p == ':' && *(p + 1) == ' ')
        ||   *p == '=')
          {
            *p++ = '\0';
            if (*p == ' ' || *p == '"')
                p++;
            value = p;
          }
        else
        if (*p == ';' || *p == '\r'||  *p == '\n')
          {
            if (*(p - 1) == '"')
                *(p - 1) = '\0';
            else
                *p = '\0';
            http_unescape (value, NULL);
            sym_assume_symbol (table, variable, value);
            p++;
            while (*p == ' ' || *p == '\r' || *p == '\n')
                p++;
            variable = p;
          }
        p++;
      }
    if (p != header && *(p - 1) == '"')
        *(p - 1) = '\0';
    http_unescape (value, NULL);
    sym_assume_symbol (table, variable, value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: is_full_url

    Synopsis: If the specified URI string starts with a URL scheme, returns
    TRUE, else returns FALSE.  A schema is one or more of [A-Za-z0-9+-.]
    followed by a ':'.
    ---------------------------------------------------------------------[>]-*/

Bool
is_full_url (const char *string)
{
    Bool
        scheme_size = 0;

    ASSERT (string);
    while (*string)
      {
        if (isalpha (*string)
        ||  isdigit (*string)
        ||  *string == '+'
        ||  *string == '-'
        ||  *string == '.')
            scheme_size++;              /*  So far, a valid scheme name      */
        else
        if (*string == ':')
            return (scheme_size > 0);   /*  Okay if ':' was not first char   */
        else
            return (FALSE);             /*  No scheme name found             */
        string++;
      }
    return (FALSE);                     /*  End of string but no scheme      */
}


/*  ---------------------------------------------------------------------[<]-
    Function: build_full_url

    Synopsis: If the specified URI string does not start with a URL schema
    or a directory delimiter, we add the path of the base URI, and return
    the resulting full URI.  This is provided as a freshly-allocated string
    which the caller must free using mem_free() when finished.  If the
    specified URI is already a full URI, returns a fresh copy of that URI.
    ---------------------------------------------------------------------[>]-*/

char *
build_full_url (const char *uri, const char *base_uri)
{
    char
        *full_uri,                      /*  Formatted full URI               */
        *slash;                         /*  Find delimiter in path           */
    int
        base_len,                       /*  Length of base part of URI       */
        uri_len;                        /*     and the rest of uri           */

    if (is_full_url (uri) || uri [0] == '/')
        return (mem_strdup (uri));
    else
      {
        /*  Find last slash in base URI                                      */
        slash = strrchr (base_uri, '/');
        if (slash)
          {
            uri_len  = strlen (uri);
            base_len = ++slash - base_uri;
            full_uri = mem_alloc (base_len + uri_len + 1);
            if (full_uri) 
              {
                memcpy (full_uri, base_uri, base_len);
                strncpy (full_uri + base_len, uri, (uri_len + 1));
              }
          }
        else
            full_uri = xstrcpy (NULL, "/", uri, NULL);

        return (full_uri);
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: http_time_str

    Synopsis: Returns the current date and time formatted using the HTTP
    standard format for log files: "DD/Mon/YYYY:hh:mm:ss".  The formatted
    time is in a static string that each call overwrites.
    ---------------------------------------------------------------------[>]-*/

char *
http_time_str (void)
{
    static char
        formatted_time [30],
        *months = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
    time_t
        time_secs;
    struct tm
        *time_struct;

    time_secs   = time (NULL);
    time_struct = safe_localtime (&time_secs);

    snprintf (formatted_time, sizeof (formatted_time), 
	                      "%02d/%s/%04d:%02d:%02d:%02d %s",
                              time_struct-> tm_mday,
                              months + time_struct-> tm_mon * 4,
                              time_struct-> tm_year + 1900,
                              time_struct-> tm_hour,
                              time_struct-> tm_min,
                              time_struct-> tm_sec,
                              timezone_string ());
    return (formatted_time);
}


/*  ---------------------------------------------------------------------[<]-
    Function: cgi_get_input

    Synopsis: Gets CGI data input from stdin or the enviorment vairable
    QUERY_STRING,  to form a stream to be used by cgi_fld_by_name,
    cgi_fld_by_index and cgi_fld_len_by_index functions.  After you
    have finshed with the input stream from this function you should call,
    cgi_free_input to free it up.

    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
cgi_get_input (
    int iMethod)
{
    int iStdinLen = 0, iMethodWas = 0;
    char *strHead, *strRetBuf;

    if (iMethod == CGIPOST
    ||  iMethod == CGIETHER)
      {
        if (getenv ("CONTENT_LENGTH"))
          {
            iStdinLen = atoi (getenv ("CONTENT_LENGTH"));
            iMethodWas = 1;
          }
      }
    if (iMethod == CGIGET
    || (iMethod == CGIETHER && !iStdinLen))
      {
        if (getenv ("QUERY_STRING"))
          {
            iStdinLen = strlen (getenv ("QUERY_STRING"));
            iMethodWas = 0;
          }
      }
    if (!iStdinLen)
        return (NULL);

    strHead = strRetBuf = (char *) malloc (sizeof (char) *
                                           iStdinLen + 1);
    if (strHead == NULL)
        return (NULL);

    memset (strRetBuf, 0, iStdinLen + 1);
    if (iMethodWas == CGIPOST)
        fread (strRetBuf, sizeof (char), iStdinLen, stdin);
    else
        strncpy (strRetBuf, getenv ("QUERY_STRING"), (iStdinLen + 1));

    return (*strHead? strHead: NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: cgi_fld_by_name

    Synopsis: Gets field data from a CGI input stream based off a HTML
    form field name.  This function returns the value of the form field.

    example:

      Form snippet:
         Product <INPUT TYPE="text" NAME="SENDERID" SIZE=80 VALUE="mailto">
         Version <INPUT TYPE="text" NAME="VERSION" SIZE=80 VALUE="v1.4">
         E-mail  <INPUT TYPE="text" NAME="Email" SIZE=80>

      CGI code to get form field:
         cgi_fld_by_name ("VERSION", strStdin, strVersion);

      On return from the call:
         strVersion would = "VERSION"

    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
cgi_fld_by_name (
    char *strFld,
    char *strIn,
    char *strRetBuf)
{
    int iCnt = 0, iLen;
    char *strTmp;

    ASSERT (strFld);
    ASSERT (strIn);

    trim (strFld);
    *strRetBuf = (char) NULL;
    FOREVER
      {
        iLen = getstrfldlen (strIn, iCnt, 0, "&");
        if (!iLen)
            break;

        strTmp = (char *) malloc (sizeof (char) * iLen + 1);
        getstrfld (strIn, iCnt++, 0, "&", strTmp);
        if (!lexncmp (strFld, strTmp, strlen (strFld)))
          {
            getequval (strTmp, strRetBuf);
            http_unescape (strRetBuf, NULL);
          }
        if (strTmp)
            free (strTmp);
        strTmp = NULL;
      }
    return strRetBuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: cgi_fld_by_index

    Synopsis: Gets field data from a CGI input stream based off a field
    index.  The index is from 0 to n.  Where 0 would be the first field
    in the stream, and n would be the last.  This function returns the
    HTML form field name and the value of the field.

    example:

      Form snippet:
         Product <INPUT TYPE="text" NAME="SENDERID" SIZE=80 VALUE="mailto">
         Version <INPUT TYPE="text" NAME="VERSION" SIZE=80 VALUE="v1.4">
         E-mail  <INPUT TYPE="text" NAME="Email" SIZE=80>

      CGI code to get form field:
         cgi_fld_by_index (1, strStdin, strFldValue, strFldName);

      On return from the call:
         strFldValue would = "v1.4"
         and would strFldName be = "VERSION"

    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
cgi_fld_by_index (
    int iPos,
    char *strIn,
    char *strRetBuf,
    char *strFldName)
{
    int iLen;
    char *strTmp;

    ASSERT (strIn);

    *strRetBuf = (char) NULL;
    iLen = getstrfldlen (strIn, iPos, 0, "&");
    if (iLen)
      {
        strTmp = (char *) malloc (sizeof (char) * iLen + 1);

        getstrfld (strIn, iPos, 0, "&", strTmp);
        getequval (strTmp, strRetBuf);
        http_unescape (strRetBuf, NULL);
        if (strFldName)
            getstrfld (strTmp, 0, 0, "=", strFldName);

        free (strTmp);
      }
    return strRetBuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: cgi_fld_len_by_index

    Synopsis: Gets the length of the field data from a CGI input stream
    based off a field index.  The index is from 0 to n.  Where 0 would
    be the first field in the stream, and n would be the last.

    example:

      Form snippet:
         Product <INPUT TYPE="text" NAME="SENDERID" SIZE=80 VALUE="mailto">
         Version <INPUT TYPE="text" NAME="VERSION" SIZE=80 VALUE="v1.4">
         E-mail  <INPUT TYPE="text" NAME="Email" SIZE=80>

      CGI code to get form field:
         cgi_fld_len_by_index (1, strStdin, &iDataLen, &iNameLen);

      On return from the call:
         iDataLen would = 4
         and iNameLen would be = 7

    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
cgi_fld_len_by_index (
    int iPos,
    char *strIn,
    int *iDataLen,
    int *iNameLen)
{
    int iLen;
    char *strTmp;

    ASSERT (strIn);

    iLen = getstrfldlen (strIn, iPos, 0, "&");

    if (iLen)
      {
        strTmp = (char *) malloc (sizeof (char) * iLen + 1);

        getstrfld (strIn, iPos, 0, "&", strTmp);
        http_unescape (strTmp, NULL);
        iLen = getstrfldlen (strTmp, 1, 0, "=");
        if (iDataLen)
            *iDataLen = iLen;

        if (iNameLen)
            *iNameLen = getstrfldlen (strTmp, 0, 0, "=");

        free (strTmp);
      }

    return iLen;
}

/*  ---------------------------------------------------------------------[<]-
    Function: displayform

    Synopsis: Writes a HTML file out to the web browser or another file.
    Gives the option to have replaceable parameters in your HTML.

    Form snippet:
       Product <INPUT TYPE="text" NAME="SENDERID" SIZE=80 VALUE="%PRONAME%">
       Version <INPUT TYPE="text" NAME="TOIDS" SIZE=80 VALUE="%PROVER%">
       E-mail  <INPUT TYPE="text" NAME="Email" SIZE=80>

    This would/could be a string you build at runtime:
      char *strformparms = "%PRONAME%|mailto,%PROVER%|v1.4";
      char *strfilename = The full path to the HTML file to display.

    This will read the file and do any string replaces it needs to
    and then send it out to the browser, or file with the changes.
      displayform (strfilename, strformparms);

    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
displayform (
    char *strformfile,
    char *strvalues)
{
    FILE *fp;
    char strline [1025];

    ASSERT (strformfile);

    fp = fopen (strformfile, "r");
    if (!fp)
        return 1;

    while (!feof (fp))
      {
        fgets (strline, 1024, fp);
        strcrop (strline);
        if (*strvalues)
            stringreplace (strline, strvalues);
        puts (strline);
      }

    fclose (fp);
    return 0;
}


