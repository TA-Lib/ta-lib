/* Note: This file has been adapted for running within TA-LIB.
 * If you intend to use iMatix, please use their original file.
 */

/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflstr.c
    Title:      String-handling functions
    Package:    Standard Function Library (SFL)

    Written:    1992/10/28  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/02

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory handling functions        */
#include "sflsymb.h"                    /*  Symbol-table functions           */
#include "sfltok.h"                     /*  Token-handling functions         */
#include "sflstr.h"                     /*  Prototypes for functions         */

#include "ta_memory.h"
#include "ta_trace.h"

TA_FILE_INFO;

#ifdef WIN32
#pragma warning( disable : 4127 ) /* Disable 'conditional expression is constant' */
#endif

/*  Globals                                                                  */
char *xstrcpy_file = "";                /*  Source file calling xstrcpy      */
word  xstrcpy_line = 0;                 /*  Source line for call             */

#if 0
!!! Not used in TA-LIB
/*  ---------------------------------------------------------------------[<]-
    Function: strdupl

    Synopsis:
    Makes a duplicate of string, obtaining space with a call to malloc().
    The allocated space is strlen (string) + 1 bytes long.  The caller is
    responsible for freeing the space allocated by strdup when it is no
    longer needed.  Returns a pointer to the allocated string, which holds
    a copy of the parameter string.  Returns NULL if there was insufficient
    heap storage available to allocate the string, or if the original string
    was itself NULL.

    Use this function in place of the non-portable strdup() function.  You
    may also want to use the more robust _mem_strdup () function.
    ---------------------------------------------------------------------[>]-*/

char *
strdupl (
    const char *string)
{
    char *copy;
    size_t length;

    if (string)
      {
        length = strlen (string) + 1;
        copy = TA_Malloc ( length);
        if (copy)
            strncpy (copy, string, length);
      }
    else
        copy = NULL;

    return (copy);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strfree

    Synopsis:
    Releases memory occupied by a string.  Call this function only when you
    previously allocated the string using malloc or strdupl().  You pass the
    address of a char pointer; this function sets the pointer to NULL.  This
    is a safety measure meant to make it safe to try to free non-allocated
    strings.  In your code, initialise all such pointers to NULL.  Returns
    the address of the modified pointer.
    ---------------------------------------------------------------------[>]-*/

char **
strfree (char **string)
{
    TA_ASSERT_RET ( string, NULL );
    if (string && *string)
      {
        TA_Free ( *string);
        *string = NULL;
      }
    return (string);
}
#endif


/*  ---------------------------------------------------------------------[<]-
    Function: strskp

    Synopsis: Skips leading spaces in string, and returns a pointer to the
    first non-blank character.  If this is a null, the end of the string
    was reached.
    ---------------------------------------------------------------------[>]-*/

char *
strskp (
    const char *string)
{
    char
        *skip = (char *) string;

    if( !string )
       return NULL;

    while (*skip == ' ')
        skip++;
    return (skip);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strcset

    Synopsis: Sets all characters in string up to but not including the
    final null character to ch.  Returns string.  Use this function instead
    of the equivalent but non-portable strset() function.
    ---------------------------------------------------------------------[>]-*/

char *
strcset (
    char *string,
    char ch)
{
    char *scan;

    if( !string )
       return string;

    scan = string;
    while (*scan)
        *scan++ = ch;
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strpad

    Synopsis: Returns string of length characters, padding with ch or
    truncating if necessary.  String must be at least length + 1 long.
    ---------------------------------------------------------------------[>]-*/

char *
strpad (
    char *string,
    char ch,
    int  length)
{
    int cursize;

    if( !string )
       return string;

    cursize = strlen (string);          /*  Get current length of string     */
    while (cursize < length)            /*  Pad until at desired length      */
        string [cursize++] = ch;

    string [cursize++] = '\0';          /*  Add terminating null             */
    return (string);                    /*    and return to caller           */
}


/*  ---------------------------------------------------------------------[<]-
    Function: strlwc

    Synopsis: Converts all alphabetic characters in string to lowercase,
    stopping at the final null character.  Returns string.  If string is
    null, returns null.  We do not call this function strlwr because that
    is already provided by some systems (but not by ANSI C).
    ---------------------------------------------------------------------[>]-*/

char *
strlwc (
    char *string)
{
    char *scan;

    if (string)
      {
        scan = string;
        while (*scan)
          {
            *scan = (char) tolower (*scan);
            scan++;
          }
      }
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strupc

    Synopsis: Converts all alphabetic characters in string to uppercase,
    stopping at the final null character.  Returns string.  If string is
    null, returns null.  We do not call this function strupr because that
    is already provided by some systems (but not by ANSI C).
    ---------------------------------------------------------------------[>]-*/

char *
strupc (
    char *string)
{
    char *scan;

    if (string)
      {
        scan = string;
        while (*scan)
          {
            *scan = (char) toupper (*scan);
            scan++;
          }
      }
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strcrop

    Synopsis: Drops trailing whitespace from string by truncating string
    to the last non-whitespace character.  Returns string.  If string is
    null, returns null.
    ---------------------------------------------------------------------[>]-*/

char *
strcrop (
    char *string)
{
    char *last;

    if (string)
      {
        last = string + strlen (string);
        while (last > string)
          {
            if (!isspace (*(last - 1)))
                break;
            last--;
          }
        *last = 0;
      }
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: stropen

    Synopsis: Inserts a character at string, and places a blank in the gap.
    If align is TRUE, makes room by reducing the size of the next gap of 2
    or more spaces.  If align is FALSE, extends the size of the string.
    Returns string.
    ---------------------------------------------------------------------[>]-*/

char *
stropen (
    char *string,
    Bool  align)
{
    char *gap;
    int  length;

    if( !string )
       return string;

    length = strlen (string) + 1;       /*  By default, move string + NULL   */
    if (align)                          /*  If align is TRUE, find gap       */
      {
        gap = strstr (string, "  ");
        if (gap)
            length = (int) (gap - string);
      }
    memmove (string + 1, string, length);
    string [0] = ' ';                   /*  Stick a space into string        */
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strclose

    Synopsis: Removes the character at string, and shifts the remainder
    down by one.  If align is TRUE, only shifts up to the next gap of 2 or
    more spaces.  Returns string.
    ---------------------------------------------------------------------[>]-*/

char *
strclose (
    char *string,
    Bool align)
{
    char *gap;
    int  length;

    if( !string )
       return string;

    length = strlen (string);           /*  By default, move string + NULL   */
    if (align) {                        /*  If align is TRUE, find gap       */
        gap = strstr (string, "  ");
        if (gap && gap != string)
            length = (int) (gap - string);
    }
    memmove (string, string + 1, length);
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strunique

    Synopsis: Reduces chains of some character to a single instances.
    For example: replace multiple spaces by one space.  Returns string.
    ---------------------------------------------------------------------[>]-*/

char *
strunique (
    char *string,
    char  unique)
{
    char
        *from,
        *to;

    if (strnull (string))
        return (string);                /*  Get rid of special cases         */

    from = string + 1;
    to   = string;
    while (*from)
      {
        if (*from == unique && *to == unique)
            from++;
        else
            *++to = *from++;
      }
    *++to = '\0';
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strmatch

    Synopsis: Calculates a similarity index for the two strings.  This
    is a value from 0 to 32767 with higher values indicating a closer match.
    The two strings are compared without regard for case.  The algorithm was
    designed by Leif Svalgaard <leif@ibm.net>.
    ---------------------------------------------------------------------[>]-*/

int
strmatch (
    const char *string1,
    const char *string2)
{
    static int
        name_weight [30] = {
            20, 15, 13, 11, 10, 9, 8, 8, 7, 7, 7, 6, 6, 6, 6,
             6,  5,  5,  5,  5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4
        };
    int
        comp_index,
        name_index,
        start_of_string,
        longest_so_far,
        substring_contribution,
        substring_length,
        compare_length,
        longest_length,
        length_difference,
        name_length,
        char_index,
        similarity_index,
        similarity_weight;
    char
        cur_name_char;

    if( !string1 || !string2 )
       return 0;

    name_length    = strlen (string1);
    compare_length = strlen (string2);
    if (name_length > compare_length)
      {
        length_difference = name_length - compare_length;
        longest_length    = name_length;
      }
    else
      {
        length_difference = compare_length - name_length;
        longest_length    = compare_length;
      }
    if (compare_length)
      {
        similarity_weight = 0;
        substring_contribution = 0;

        for (char_index = 0; char_index < name_length; char_index++)
          {
            start_of_string = char_index;
            cur_name_char   = (char) tolower (string1 [char_index]);
            longest_so_far  = 0;
            comp_index      = 0;

            while (comp_index < compare_length)
              {
                while ((comp_index < compare_length)
                &&     (tolower (string2 [comp_index]) != cur_name_char))
                    comp_index++;

                substring_length = 0;
                name_index = start_of_string;

                while ((comp_index < compare_length)
                &&     (tolower (string2 [comp_index])
                     == tolower (string1 [name_index])))
                  {
                    if (comp_index == name_index)
                        substring_contribution++;
                    comp_index++;
                    if (name_index < name_length)
                      {
                        name_index++;
                        substring_length++;
                      }
                  }
                substring_contribution += (substring_length + 1) * 3;
                if (longest_so_far < substring_length)
                    longest_so_far = substring_length;
              }
            similarity_weight += (substring_contribution
                                  + longest_so_far + 1) * 2;
            similarity_weight /= name_length + 1;
          }
        similarity_index  = (name_length < 30? name_weight [name_length]: 3)
                          * longest_length;
        similarity_index /= 10;
        similarity_index += 2 * length_difference / longest_length;
        similarity_index  = 100 * similarity_weight / similarity_index;
      }
    else
        similarity_index = 0;

    return (similarity_index);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strprefixed

    Synopsis: If string starts with specified prefix, returns TRUE.  If
    string does not start with specified prefix, returns FALSE.
    ---------------------------------------------------------------------[>]-*/

Bool
strprefixed (
    const char *string,
    const char *prefix)
{
    if( !string || !prefix )
       return FALSE;

    if (*string == *prefix              /*  Check that first letters match   */
    &&  strlen (string) >= strlen (prefix)
    &&  memcmp (string, prefix, strlen (prefix)) == 0)
        return (TRUE);
    else
        return (FALSE);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strprefix

    Synopsis: Looks for one of the delimiter characters in the string; if
    found, returns a string that contains the text up to that delimiter.
    If not found, returns NULL.  The returned string can be zero or more
    characters long followed by a null byte.  It is allocated using the
    mem_alloc() function; you should free it using TA_Free ( () when finished.
    ---------------------------------------------------------------------[>]-*/

char *
strprefix (
    const char *string,
    const char *delims)
{
    const char
        *nextch;
    char
        *token;
    int
        token_size;

    if( !string || !delims )
       return NULL;

    for (nextch = string; *nextch; nextch++)
      {
        if (strchr (delims, *string))   /*  Is next character a delimiter    */
          {
            token_size = (int) (nextch - string);
            token = TA_Malloc( token_size + 1);
            if (token == NULL)
                return (NULL);          /*  Not enough memory - fail         */
            memcpy (token, string, token_size);
            token [token_size] = 0;
            return (token);
          }
      }
    return (NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strdefix

    Synopsis: If string starts with specified prefix, returns pointer to
    character after prefix.  Null character is not considered part of the
    prefix.  If string does not start with specified prefix, returns NULL.
    ---------------------------------------------------------------------[>]-*/

char *
strdefix (
    const char *string,
    const char *prefix)
{
    if( !string || !prefix )
       return NULL;

    if (strlen (string) >= strlen (prefix)
    &&  memcmp (string, prefix, strlen (prefix)) == 0)
        return ((char *) string + strlen (prefix));
    else
        return (NULL);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strhash

    Synopsis:
    Calculates a 32-bit hash value for the string.  The string must end in
    a null.  To use the result as a hash key, take the modulo over the hash
    table size.  The algorithm was designed by Peter Weinberger.  This
    version was adapted from Dr Dobb's Journal April 1996 page 26.

    Examples:
    int index;
    index = (int) strhash (name) % TABLE_SIZE;
    ---------------------------------------------------------------------[>]-*/

qbyte
strhash (
    const char *string)
{
    qbyte
        high_bits,
        hash_value = 0;

    if( !string )
       return 0;

    while (*string)
      {
        hash_value = (hash_value << 4) + *string++;
        if ((high_bits = hash_value & 0xF0000000L) != 0)
            hash_value ^= high_bits >> 24;
        hash_value &= ~high_bits;
      }
    return (hash_value);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strconvch

    Synopsis: Converts all instances of one character in a string to some
    other character.  Returns string.  Does nothing if the string is NULL.
    ---------------------------------------------------------------------[>]-*/

char *
strconvch (
    char *string,
    char from,
    char to)
{
    char *scan;

    if (string)
      {
        scan = string;
        while (*scan)
          {
            if (*scan == from)
               *scan = to;
            scan++;
          }
      }
    return (string);
}


/*  ---------------------------------------------------------------------[<]-
    Function: xstrcat

    Synopsis: Concatenates multiple strings into a single result.  Eg.
    xstrcat (buffer, "A", "B", NULL) stores "AB" in buffer.  Returns dest.
    Append the string to any existing contents of dest.
    From DDJ Nov 1992 p. 155, with adaptions.
    ---------------------------------------------------------------------[>]-*/

char *
xstrcat (
    char *dest,
    const char *src, ...)
{
    char
        *feedback = dest;
    va_list
        va;
    if( !dest || !src )
       return dest;

    while (*dest)                       /*  Find end of dest string          */
        dest++;

    va_start (va, src);
    while (src)
      {
        while (*src)
            *dest++ = *src++;
        src = va_arg (va, char *);
      }
    *dest = '\0';                       /*  Append a null character          */
    va_end (va);
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: xstrcpy

    Synopsis: Concatenates multiple strings into a single result.  Eg.
    xstrcpy (buffer, "A", "B", NULL) stores "AB" in buffer.  Returns dest.
    Any existing contents of dest are cleared.  If the dest buffer is NULL,
    allocates a new buffer with the required length and returns that.  The
    buffer is allocated using mem_alloc(), and should eventually be freed
    using TA_Free ( () or mem_strfree().  Returns NULL if there was too little
    memory to allocate the new string.  We can't define macros with variable
    argument lists, so we pass the file and line number through two globals,
    xstrcpy_file and xstrcpy_line, which are reset to empty values after
    each call to xstrcpy.
    ---------------------------------------------------------------------[>]-*/

char *
xstrcpy (
    char *dest,
    const char *src, ...)
{
    const char
        *src_ptr;
    va_list
        va;
    size_t
        dest_size;                      /*  Size of concatenated strings     */

    /*  Allocate new buffer if necessary                                     */
    if (dest == NULL)
      {
        va_start (va, src);             /*  Start variable args processing   */
        src_ptr   = src;
        dest_size = 1;                  /*  Allow for trailing null char     */
        while (src_ptr)
          {
            dest_size += strlen (src_ptr);
            src_ptr = va_arg (va, char *);
          }
        va_end (va);                    /*  End variable args processing     */

        /*  Allocate by going directly to mem_alloc_ function                */
        dest = TA_Malloc( dest_size );
        xstrcpy_file = "";
        xstrcpy_line = 0;
        if (dest == NULL)
            return (NULL);              /*  Not enough memory                */
      }

    /*  Now copy strings into destination buffer                             */
    va_start (va, src);                 /*  Start variable args processing   */
    src_ptr  = src;
    dest [0] = '\0';
    while (src_ptr)
      {
        strcat (dest, src_ptr);
        src_ptr = va_arg (va, char *);
      }
    va_end (va);                        /*  End variable args processing     */
    return (dest);
}


/*  ---------------------------------------------------------------------[<]-
    Function: lexcmp

    Synopsis: Performs an unsigned comparison of two strings without regard
    to the case of any letters in the strings.  Returns a value that is
    <TABLE>
        <_0     if string1 is less than string2
        ==_0    if string1 is equal to string2
        >_0     if string1 is greater than string2
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

int
lexcmp (
    const char *string1,
    const char *string2)
{
    int cmp;

    if( !string1 || !string2 )
       return 0;

    do
      {
        cmp = (byte) tolower (*string1) - (byte) tolower (*string2);
      }
    while (*string1++ && *string2++ && cmp == 0);
    return (cmp);
}


/*  ---------------------------------------------------------------------[<]-
    Function: lexncmp

    Synopsis: Performs an unsigned comparison of two strings without regard
    to the case of specified number of letters in the strings.
    Returns a value that is
    <TABLE>
        <_0     if string1 is less than string2
        ==_0    if string1 is equal to string2
        >_0     if string1 is greater than string2
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

int
lexncmp (
    const char *string1,
    const char *string2,
    const int   count)
{
    int
        cmp;
    char
        *end;

    if( !string1 || !string2 )
       return 0;

    end = (char *)string1 + count;
    do
      {
        cmp = (byte) tolower (*string1) - (byte) tolower (*string2);
      }
    while (*string1++ && *string2++ && cmp == 0 && string1 < end);
    return (cmp);
}


/*  ---------------------------------------------------------------------[<]-
    Function: lexwcmp

    Synopsis: Compares two strings ignoring case, and allowing wildcards
    in the second string (the pattern).  Two special characters are
    recognised in the pattern: '?' matches any character in the string,
    and '*' matches the remainder of the string.
    Returns a value that is:
    <TABLE>
        <_0     if string1 is less than pattern
        ==_0    if string1 is equal to pattern
        >_0     if string1 is greater than pattern
    </TABLE>
    ---------------------------------------------------------------------[>]-*/

int
lexwcmp (
    const char *string1,
    const char *pattern)
{
    int cmp = 0;

    if( !string1 || !pattern )
       return 0;

    do
      {
        if (*pattern != '?' && *pattern != '*')
            cmp = (byte) tolower (*string1) - (byte) tolower (*pattern);
      }
    while (*string1++ && *pattern++ && cmp == 0 && *pattern != '*');
    return (cmp);
}

#if 0
!!! NOT NEEDED for TA-LIB
/*  ---------------------------------------------------------------------[<]-
    Function: soundex

    Synopsis: Calculates the SOUNDEX code for the string.  Returns the
    address of a static area that holds the code.  This area is overwritten
    by each call to the soundex function.  The SOUNDEX encoding converts
    letters to uppercase, and translates each letter according to this
    table: A0 B1 C2 D3 E0 F1 G2 H0 I0 J2 K2 L4 M5 N5 O0 P1 Q2 R6 S2 T3
    U0 V1 W0 X2 Y0 Z2.  Non-letters are ignored, letters that translate
    to zero, and multiple occurences of the same value are also ignored.
    This function always returns a 4-letter encoding: the first letter of
    the string followed by the first three significant digits.

    Examples:
    printf ("Soundex of %s = %s\n", argv [1], soundex (argv [1]));
    ---------------------------------------------------------------------[>]-*/

char *
soundex (const char *string)
{
    TA_ASSERT_RET ( string, NULL );
    return (soundexn (string, 4, FALSE));
}
#endif


#if 0
!!! NOT NEEDED for TA-LIB

/*  ---------------------------------------------------------------------[<]-
    Function: soundexn

    Synopsis: Calculates the SOUNDEX code for the string.  Returns the
    address of a static area that holds the code.  This area is overwritten
    by each call to the soundex function.  The SOUNDEX encoding converts
    letters to uppercase, and translates each letter according to this
    table: A0 B1 C2 D3 E0 F1 G2 H0 I0 J2 K2 L4 M5 N5 O0 P1 Q2 R6 S2 T3
    U0 V1 W0 X2 Y0 Z2.  Non-letters are ignored, letters that translate
    to zero, and multiple occurences of the same value are also ignored.
    This function returns a N-letter encoding: the first letter of the
    string followed by the first N-1 significant digits.  N may not be
    greater than SOUNDEX_MAX (100).  If the fold argument is true, includes
    the first letter in the calculated digits, else starts with the second
    letter.
    ---------------------------------------------------------------------[>]-*/

char *
soundexn (
    const char *string, int size, Bool fold)
{
#   define SOUNDEX_MAX  100
#   define SOUNDEX_TABLE                   \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "00123012002245501262301020200000" \
        "00123012002245501262301020200000" \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000" \
        "00000000000000000000000000000000"

    static char
       *soundex_table = SOUNDEX_TABLE,  /*  ASCII-SOUNDEX conversion         */
        soundex_code [SOUNDEX_MAX + 1]; /*  Letter + 3 digits                */
    int
        index;
    char
        last_value = 0,
        this_value;

    TA_ASSERT_RET ( string, NULL );
    TA_ASSERT_RET ( size > 0 && size <= SOUNDEX_MAX, NULL );

    /*  Initialise the soundex code to a string of zeroes                    */
    memset (soundex_code, '0', size);
    soundex_code [size] = '\0';

    soundex_code [0] = toupper (*string);
    last_value = fold? 0: soundex_table [(byte) *string];
    index = 1;                          /*  Store results at [index]         */
    while (*string)
      {
        this_value = soundex_table [(byte) *string++];
        if (this_value == last_value    /*  Ignore doubles                   */
        ||  this_value == '0')          /*    and 'quiet' letters            */
          {
            last_value = this_value;
            continue;
          }
        last_value = this_value;
        soundex_code [index++] = this_value;
        if (index == size)              /*  Up to size result characters     */
            break;
      }
    return (soundex_code);
}
#endif

#if 0
!!! NOT NEEDED for TA-LIB
/*  ---------------------------------------------------------------------[<]-
    Function: strt2descr

    Synopsis: Converts a table of strings into a single block of memory.
    The input table consists of an array of null-terminated strings,
    terminated in a null pointer.  Returns the address of a DESCR block
    defined as: "typedef struct {size_t size; byte *data} DESCR;".
    Allocates the descriptor block using the mem_alloc() function; you must
    free it using TA_Free ( () when you are finished with it. The strings are
    packed into the descriptor data field, each terminated by a null byte.
    The final string is terminated by two nulls.  The total size of the
    descriptor is descr-> size + sizeof (DESCR).  Note that if you omit the
    last null pointer in the input table, you will probably get an addressing
    error.  Returns NULL if there was insufficient memory to allocate the
    descriptor block.
    ---------------------------------------------------------------------[>]-*/

DESCR *
strt2descr (char **table)
{
    DESCR
        *descr;                         /*  Allocated descriptor             */
    char
        *descr_ptr;                     /*  Pointer into block               */
    size_t
        descr_size;                     /*  Size of table                    */
    int
        string_nbr;                     /*  Index into string table          */

    TA_ASSERT_RET ( table, NULL);

    /*  Calculate the size of the descriptor                                 */
    descr_size = 1;                     /*  Allow for final null byte        */
    for (string_nbr = 0; table [string_nbr]; string_nbr++)
        descr_size += strlen (table [string_nbr]) + 1;

    /*  Allocate a descriptor and fill it with the strings                   */
    descr = TA_Malloc ( descr_size + sizeof (DESCR));
    if (descr)
      {
        descr-> size = descr_size;
        descr-> data = (byte *) descr + sizeof (DESCR);
        descr_ptr    = (char *) descr-> data;

        for (string_nbr = 0; table [string_nbr]; string_nbr++)
          {
            size_t descr_len = strlen (table [string_nbr]) + 1;
            strncpy (descr_ptr, table [string_nbr], descr_len);
            descr_ptr += descr_len;
          }
        *descr_ptr = '\0';              /*  Add a null string                */
      }
    return (descr);
}


/*  ---------------------------------------------------------------------[<]-
    Function: descr2strt

    Synopsis: Takes a descriptor prepared by strt2descr() and returns an
    array of strings pointers, terminated in a null pointer.  The array is
    allocated using the mem_alloc() function.  Each string is individually
    allocated.  Thus, to free the string table you must call TA_Free ( () for
    each entry in the table, except the last one, and then for the table.
    You can also call strtfree() to destroy the table in a single operation.
    Returns NULL if there was insufficient memory to allocate the table of
    strings.
    ---------------------------------------------------------------------[>]-*/

char **
descr2strt ( const DESCR *descr)
{
    char
        **table;
    int
        string_count,
        string_nbr;                     /*  Index into string table          */
    char
        *descr_ptr;                     /*  Pointer into block               */

    TA_ASSERT_RET ( descr, NULL);

    /*  Count the number of strings in the table                             */
    descr_ptr = (char *) descr-> data;
    string_count = 0;
    while (*descr_ptr)                  /*  Loop until we hit null string    */
      {
        string_count++;
        descr_ptr += strlen (descr_ptr) + 1;
      }

    /*  Allocate a table and fill it with the strings                        */
    table = TA_Malloc ( (string_count + 1) * sizeof (char *));
    if (table)
      {
        descr_ptr = (char *) descr-> data;
        for (string_nbr = 0; string_nbr < string_count; string_nbr++)
          {
            table [string_nbr] = mem_strdup (descr_ptr);
            descr_ptr += strlen (descr_ptr) + 1;
          }
        table [string_count] = NULL;    /*  Store final null pointer         */
      }
    return (table);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strtfree

    Synopsis: Releases a table of strings as created by descr2strt() or a
    similar function.  If the argument is null, does nothing.
    ---------------------------------------------------------------------[>]-*/

void
strtfree (char **table)
{
    int
        string_nbr;                     /*  Index into string array          */

    if (table)
      {
        for (string_nbr = 0; table [string_nbr]; string_nbr++)
            TA_Free (  table [string_nbr]);
        TA_Free (  table);
      }
}
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: strcntch

    Synopsis: Returns number of instances of a character in a string.
    ---------------------------------------------------------------------[>]-*/

int
strcntch (    
    const char *string,
    char value)
{
    int
        count = 0;

    if( !string )
       return 0;

    while (*string)
        if (*string++ == value)
            count++;

    return (count);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strlookup

    Synopsis: Searches the specified lookup table, defined as an array of
    LOOKUP items, for the specified string key, and returns a lookup value.
    You are REQUIRED to terminate the table with a null key: if the key is
    not found in the table, returns the value for the last, null key.
    ---------------------------------------------------------------------[>]-*/

int
strlookup (
    const LOOKUP *lookup, const char *key)
{
    int
        index;

    if( !lookup || !key )
       return 0;

    for (index = 0; lookup [index].key; index++)
        if (streq (lookup [index].key, key))
            break;
    
    return (lookup [index].value);
}

/*  ---------------------------------------------------------------------[<]-
    Function: strreformat

    Synopsis: Reformats a string to fit within lines of the specified width.
    Prefixes each line with some optional text (included in the width).
    Allocates a fresh block of memory to contain the newly formatted line.
    ---------------------------------------------------------------------[>]-*/

char *
strreformat ( const char *source, size_t width, const char *prefix)
{
    size_t
        total_size,                     /*  Total size of buffer             */
        prefix_len,                     /*  Size of prefix string            */
        token_len;                      /*  Size of current token            */
    char
        **tokens,                       /*  String broken into words         */
        *token,                         /*  Current token                    */
        *buffer,                        /*  Target multiline buffer          */
        *bufptr;                        /*  Next position in buffer          */
    int
        cur_width,                      /*  Current line width incl. prefix  */
        token_nbr;                      /*  Token number, 0..n               */

    TA_ASSERT_RET ( source, NULL );
    if (source == NULL)
        return NULL;
    
    /*  Ignore prefix if NULL                                                */
    if (prefix == NULL)
        prefix = "";

    /*  Calculate maximum size of resulting buffer, which is difficult to
     *  predict accurately.  We allow for 8 wasted characters on each line
     *  plus the line ending.
     */
    prefix_len = strlen (prefix);
    total_size = strlen (source) / (width - prefix_len) + 1;
    total_size = total_size * (width + 9);
    buffer = TA_Malloc (total_size);
    tokens = tok_split (source);

    TA_ASSERT_RET ( strlen (prefix) < width, NULL);
    TA_ASSERT_RET ( total_size > tok_text_size ( tokens), NULL);

    cur_width = 0;
    bufptr = buffer;
    for (token_nbr = 0; tokens [token_nbr]; token_nbr++)
      {
        token = tokens [token_nbr];
        token_len = strlen (token);

        /*  First decide if next token will fit on line or not               */
        if (token_len + cur_width > width)
          {
            *bufptr++ = '\n';           /*  Start new line                   */
            cur_width = 0;
          }
        /*  Put prefix at at start of line if necessary                      */
        if (cur_width == 0)
          {
            /*  If prefix would overflow buffer, we quit                     */
            if ((bufptr - buffer) + prefix_len >= total_size)
                break;
            memcpy (bufptr, prefix, prefix_len);
            bufptr   += prefix_len;
            cur_width = prefix_len;
          }
        /*  If token would overflow buffer, we quit                          */
        if ((bufptr - buffer) + token_len + 1 >= total_size)
            break;

        /*  Now append token to line and add a space                         */
        memcpy (bufptr, token, token_len);
        bufptr    += token_len;
        cur_width += token_len + 1;
        *bufptr++ = ' ';
      }
    *bufptr = '\0';                     /*  Terminate the last line          */
    tok_free (tokens);
    return (buffer);
}


/*  ---------------------------------------------------------------------[<]-
    Function: removechars

    Synopsis: Removes known chars from a string. Returns pointer to head of
    the buffer.  Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
removechars (
    char *strbuf,
    char *chrstorm)
{
   char *offset;

   if( !strbuf || !chrstorm )
      return strbuf;

   offset = (char *)NULL;

   while (*strbuf)
      {
         offset = strpbrk (strbuf, chrstorm);
         if (offset)
             strcpy (offset, (offset + 1));                    /* NO OVERRUN */
         else
             break;
      }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: replacechrswith

    Synopsis: Subsitutes known char(s)in a string with another. Returns
    pointer to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
replacechrswith (
    char *strbuf,
    char *chrstorm,
    char chartorlcwith)
{
   char *offset;

   if( !strbuf || !chrstorm )
      return strbuf;

   offset = (char *)NULL;

   while (*strbuf)
      {
         offset = strpbrk (strbuf, chrstorm);
         if (offset)
           {
             *(offset)= chartorlcwith;
           }

         else
             break;
      }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: insertstring

    Synopsis: Inserts a string into another string.  Returns a pointer
    to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
insertstring (
    char *strbuf,
    char *chrstoins,
    int pos)
{
   if( !strbuf || !chrstoins )
      return strbuf;

   memmove (((strbuf + pos) + strlen (chrstoins)),
            (strbuf + pos), (strlen ((strbuf + pos)) + 1));
   memcpy ((strbuf + pos), chrstoins, strlen (chrstoins));

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: insertchar

    Synopsis: Inserts a char into a string.  Returns a pointer to head of
    the buffer.  Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
insertchar (
    char *strbuf,
    char chrtoins,
    int pos)
{
   if( !strbuf )
      return strbuf;

   memmove ((strbuf + pos)+ 1, (strbuf + pos), strlen ((strbuf + pos))+ 1);
   *(strbuf + pos)= chrtoins;

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: leftfill

    Synopsis: Pads a string to the left, to a know length, with the
    given char value. Returns a pointer to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
leftfill (
    char *strbuf,
    char chrtofill,
    unsigned len)
{
   if( !strbuf )
      return strbuf;

   while (strlen (strbuf)< len)
     {
       insertchar (strbuf, chrtofill, 0);
     }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: rightfill

    Synopsis: Pads a string to the right, to a known length, with the
    given char value. Returns a pointer to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
rightfill (
    char *strbuf,
    char chrtofill,
    unsigned len)
{
   if( !strbuf )
      return strbuf;

   while (strlen (strbuf)< len)
     {
       insertchar (strbuf, chrtofill, strlen (strbuf));
     }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: trim

    Synopsis: Eats the whitespace's from the left and right side of a
    string.  This function maintains a proper pointer head.  Returns a
    pointer to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
trim (
    char *strin)
{
    ltrim (strin);
    strcrop (strin);

    return strin;
}

/*  ---------------------------------------------------------------------[<]-
    Function: ltrim

    Synopsis: Deletes leading white spaces in string, and returns a
    pointer to the first non-blank character.  If this is a null, the
    end of the string was reached.
    ---------------------------------------------------------------------[>]-*/

char *
ltrim (
    char *string)
{
   if( !string )
      return string;

   while (isspace(*string))
       deletechar(string,0);

   return string;
}

/*  ---------------------------------------------------------------------[<]-
    Function: searchreplace

    Synopsis: A case insensitive search and replace. Searches for all
    occurances of a string, and replaces it with another string.
    Returns a pointer
    to head of the buffer.  Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
searchreplace (
    char *strbuf,
    char *strtofnd,
    char *strtoins)
{
   char *offset, *strbase;

   if( !strbuf || !strtofnd || !strtoins )
      return strbuf;

   offset = strbase = (char *)NULL;

   while (*strbuf)
     {
       offset = stricstr (!offset ? strbuf : strbase, strtofnd);
       if (offset)
         {
           strbase = (offset + strlen (strtoins));
           strcpy (offset, (offset + strlen (strtofnd)));
           memmove (offset + strlen (strtoins),
                     offset, strlen (offset) + 1);
           memcpy (offset, strtoins, strlen (strtoins));
         }

      else
           break;
     }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: deletestring

    Synopsis: Deletes all occurances of one string, in another string.
    Returns a pointer to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
deletestring (
    char *strbuf,
    char *strtodel,
    int ignorecase)
{
   char *offset;

   if( !strbuf || !strtodel )
      return strbuf;

   offset = (char *)NULL;

   while (*strbuf)
     {
        if (!ignorecase)
            offset = stricstr (strbuf, strtodel);
        else
            offset = strstr (strbuf, strtodel);
        if (offset)
          {
            strcpy (offset, (offset + strlen (strtodel)));    /* NO OVERRUN */
          }
        else
            break;
     }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: getstrfld

    Synopsis: Gets a sub-string from a formated string. nice strtok
    replacement.

    usage:
      char strarray[] = { "123,456,789,abc" };
      char strretbuff[4];
      getstrfld (strarray, 2, 0, ",", strretbuff);

    This would return the string "789" and place it also in strretbuff.
    Returns a NULL if fldno is out of range, else returns a pointer to
    head of the buffer.  Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
getstrfld (
    char *strbuf,
    int fldno,
    int ofset,
    char *sep,
    char *retstr)
{
   char *offset, *strptr;
   int curfld;

   if( !strbuf || !sep || !retstr )
      return strbuf;

   offset = strptr = (char *)NULL;
   curfld = 0;

   strbuf += ofset;

   while (*strbuf)
     {
       strptr = !offset ? strbuf : offset;
       offset = strpbrk ((!offset ? strbuf : offset), sep);

       if (offset)
          offset++;
       else if (curfld != fldno)
         {
           *retstr = (char)0;
           break;
         }

       if (curfld == fldno)
         {
           strncpy (retstr, strptr,
              (!offset ? strlen (strptr)+ 1 :
              (size_t)(offset - strptr)));
           if (offset)
              retstr[offset - strptr - 1] = 0;

           break;
         }
       curfld++;
     }
   return retstr;
}

/*  ---------------------------------------------------------------------[<]-
    Function: setstrfld

    Synopsis: Inserts a string into a fomated string.

    usage:
       char strsrray[26] = { "this is a test." };
       setstrfld (strsrray, 2, 0, " ", "big ");

       result: this is a big test.

    Does nothing if fldno is out of range, else returns pointer to head
    of the buffer.  Returns a pointer to head of the buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
setstrfld (
    char *strbuf,
    int fldno,
    int ofset,
    char *sep,
    char *strtoins)
{
   char *offset, *strptr, *strhead;
   int curfld;

   if( !strbuf || !sep || !strtoins )
      return strbuf;

   offset = strptr = (char *)NULL;
   curfld = 0;

   strhead = strbuf;
   strbuf += ofset;

   while (*strbuf)
     {
       strptr = !offset ? strbuf : offset;
       offset = strpbrk ((!offset ? strbuf : offset), sep);

       if (offset)
          offset++;

       if (curfld == fldno)
          {
            insertstring (strptr, strtoins,
               (!offset ? (int)strlen (strptr):
               (offset - strptr)));
            break;
          }
       curfld++;
     }

   return strhead;
}

/*  ---------------------------------------------------------------------[<]-
    Function: getstrfldlen

    Synopsis: Get the length of as a field in a string.  Used mainly
    for getting the len to malloc mem to call getstrfld with.  Returns
    the length of the field.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
getstrfldlen (
    const char *strbuf,
    int fldno,
    int ofset,
    char *sep)
{
   const char *offset, *strptr;
   int curfld, retlen = 0;

   if( !strbuf || !sep )
      return 0;

   offset = strptr = (char *)NULL;
   curfld = 0;

   strbuf += ofset;

   while (*strbuf)
     {
       strptr = !offset ? strbuf : offset;
       offset = strpbrk ((!offset ? strbuf : offset), sep);

       if (offset)
          offset++;
       else if (curfld != fldno)
         {
           retlen = 0;
           break;
         }
       if (curfld == fldno)
         {
           retlen = (!offset ? (int)strlen (strptr) + 1 : (offset - strptr));
           break;
         }
       curfld++;
     }
   return retlen;
}

#if 0
!!! Not used within TA-Lib
/*  ---------------------------------------------------------------------[<]-
    Function: findstrinfile

    Synopsis:
    Find's a string inside a text file and reads the line in and sets the
    file pointer to the beginning of that line.  Assumes the line length
    to be <= 1024 bytes.  Returns a pointer to head of the return buffer,
    and the file postion will be at the start of the found string. If
    the strretstr param is != NULL then strretstr will contain the line
    that the search string was found in.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
findstrinfile (
    FILE *fp,
    char *strtofind,
    char *strretstr,
    int *iLnNo)
{
   char strline[1025];
   int nfnd = 0;
   long lfpos;

   if( !fp || !strtofind || !strretstr )
      strretstr;

   if (strretstr)
       *strretstr = 0;

   while (1)
     {
       lfpos = ftell (fp);
       fgets (strline, 1024, fp);
       trim (strline);

       if (!*strline)
          continue;

       if (iLnNo)
           (*iLnNo)++;

       if (stricstr (strline, strtofind))
         {
           if (strretstr)
             {
               strcpy (strretstr, strline);
             }

           fseek (fp, lfpos, SEEK_SET);
           nfnd = 1;
           break;
         }

       if (feof (fp))
           break;
     }

   if (strretstr)
       return strretstr;
   else
       return (char *)nfnd;
}
#endif

/*  ---------------------------------------------------------------------[<]-
    Function: getequval

    Synopsis:
    get the everything on a line past a '='.

    Examples:
       char strtest[] = { "progpath=c:\sfl");
       char strret[256];
       getequval (strtest, strret);

    This would return: "c:\sfl".  Returns a pointer to head of the
    return buffer.  Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
getequval (
    char *strline,
    char *strretstr)
{
   char *stroffset;

   if( !strline || !strretstr )
      return strline;

   stroffset = strstr (strline, "=");

   if (stroffset)
       ltrim (stroffset + 1);
   else
       return (char *)NULL;

   return strcpy (strretstr,
                  (stroffset && *(stroffset + 1))? (stroffset + 1): "");
}

/*  ---------------------------------------------------------------------[<]-
    Function: matchtable

    Synopsis:
    Function to compare a string with a set of strings.

    Examples:
       iret = matchtable (strname, "bill|william|billy", "|", IGNORECASE);

    If strname == "william", then matchtable would return 1.
    Returns >= 0 if match is found.  a -1 if no match is found.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
matchtable (
    char *strbuf,
    char *strmatch,
    char *strsept,
    int ncase)
{
   int nstate = -1, cnt = 0, icmpres;
   int ilen;
   char *strtemp;

   if( !strbuf || !strmatch || !strsept )
      return -1;

   while (1)
     {
       ilen = getstrfldlen (strmatch, cnt, 0, strsept);
       strtemp = (char *) TA_Malloc (sizeof (char) * ilen + 1);
       TA_ASSERT_RET ( strtemp, 0 );
       getstrfld (strmatch, cnt, 0, strsept, strtemp);
       if (*strtemp)
         {
           trim (strtemp);
           if (!ncase)
             {
               icmpres = lexcmp (strbuf, strtemp);
             }
           else
             {
               icmpres = strcmp (strbuf, strtemp);
             }

           if (!icmpres)
             {
               nstate = cnt;
               break;
             }
           else
             {
               if (!strcmp (strbuf, strtemp))
                 {
                   nstate = cnt;
                   break;
                 }
             }
         }
       else
         {
           nstate = -1;
           break;
         }
       cnt++;
       TA_Free (strtemp);
     }

   return nstate;
}

/*  ---------------------------------------------------------------------[<]-
    Function: stringreplace

    Synopsis:
    This function searches for known strings, and replaces them with
    another string.

    Examples:
       stringreplace (strfilename, "sqv|sqr,ruv|run,h_v|h");

    This example would replace all occurences of sqv, with sqr, ruv with
    run and h_v with h. Returns pointer to head of the return buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
stringreplace (
    char *strbuf,
    char *strpattern)
{
   int ilen, ifld = 0;
   char *strsrch, *strrpl, *strpat;

   TA_ASSERT_RET ( strbuf, NULL);
   TA_ASSERT_RET ( strpattern, NULL);

   if (!strpattern)
       return strbuf;

   while (1)
     {
       ilen = getstrfldlen (strpattern, ifld, 0, ",");
       if (!ilen)
           break;
       strpat = (char *)TA_Malloc ( ilen + 1);
       getstrfld (strpattern, ifld, 0, ",", strpat);
       ifld++;

       ilen = getstrfldlen (strpat, 0, 0, "|");
       strsrch = (char *)TA_Malloc( ilen + 1);
       getstrfld (strpat, 0, 0, "|", strsrch);

       ilen = getstrfldlen (strpat, 1, 0, "|");
       strrpl = (char *)TA_Malloc ( ilen + 1);
       getstrfld (strpat, 1, 0, "|", strrpl);

       searchreplace (strbuf, strsrch, strrpl);

       TA_Free ( strsrch);
       TA_Free ( strrpl);
       TA_Free ( strpat);
     }

   return strbuf;
}

/*  ---------------------------------------------------------------------[<]-
    Function: wordwrapstr

    Synopsis:
    Function that does word wraping of a string at or less than iwid.
    Breaks up a string on word boundaries by placing '\n' in the string.
    Returns a pointer to head of the return buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
wordwrapstr (
    char *strbuff,
    int iwid)
{
   char *strtmp = strbuff;
   int icnt = 0;

   if( !strbuff )
      return strbuff;

   replacechrswith (strbuff, "\n", ' ');
   while (*strtmp)
     {
       if ((int)strlen (strtmp) > (int)iwid)
         {
           icnt = iwid;
           while (*(strtmp + icnt))
             {
               if (strchr (" .?;!,", *(strtmp + icnt)))
                 {
                   ltrim ((strtmp + icnt));
                   insertchar (strtmp, '\n', icnt);
                   strtmp += icnt + 1;
                   break;
                 }
               icnt--;

               if (!icnt)
                 {
                   if (strchr (" .?;!,", *(strtmp + icnt)))
                     {
                       ltrim ((strtmp + iwid));
                       insertchar (strtmp, '\n', iwid);
                       strtmp += iwid + 1;
                       break;
                     }
                 }
             }
         }
       else
           break;
   }

   return strbuff;
}

/*  ---------------------------------------------------------------------[<]-
    Function: stricstr

    Synopsis:
    A case insensitive strstr.  Returns a pointer to head of the str1.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
stricstr (
    const char *str1,
    const char *str2)
{
   char *strtmp = (char *)str1;
   int iret = 1;

   if( !str1 || !str2 )
      return NULL;

   while (*strtmp)
     {
       if (strlen (strtmp)>= strlen (str2))
         {
           iret = lexncmp (strtmp, str2, strlen (str2));
         }
       else
         {
           break;
         }

       if (!iret)
         {
           break;
         }

       strtmp++;
     }

   return !iret ? strtmp : (char *)NULL;
}

/*  ---------------------------------------------------------------------[<]-
    Function: strtempcmp

    Synopsis:
    Compares a string to a template.
    Template chars and there functions:
      # or 9 = Number.
      A or _ = Alpha.
      @      = Alphanumeric
      \char  = Literal.  Char would be the literal to use. ie: "\%" -
               looks for a % in that postion
    Returns 0 if == to the template and 1 if != to the template.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
strtempcmp (
    const char *str1,
    const char *strPat)
{
   int ires = 1;

   if( !str1 || !strPat )
      return 0;

   while (*str1 && *strPat)
     {
       switch ((int)*strPat)
         {
           case '#':
           case '9':
              ires = isdigit ((int)*str1);
              break;

           case 'A':
           case '_':
              ires = isalpha ((int)*str1);
              break;

           case '@':
              ires = isalnum ((int)*str1);
              break;

           case ' ':
              ires = isspace ((int)*str1);
              break;

           case '\\':
              strPat++;
              if (*str1 != *strPat)
                 {
                   ires = 1;
                 }
              break;

           default:
              break;
         }

       if (!ires)
         {
           break;
         }

       str1++;
       strPat++;
     }

   return ires ? 0 : 1;
}

/*  ---------------------------------------------------------------------[<]-
    Function: istoken

    Synopsis:
    Eats strToEat from strBuff only if it begins with contents of
    strToEat, and returns a 0 or 1 to tell what it did.

    Examples:
       char strBuff[] = { "select * from mytbl;" };
       int iWasToken;
       istoken (&strBuff, "SELECT", &iWasToken);

    On return here iWasToken would == 1, and strBuff would be:
    " * from mytbl;"
    If the token is not found, then strBuff will not be affected, and
    a 0 will be returned.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
istoken (
    char **strLine,
    const char *strtoken,
    int *iWasToken)
{
   int iRet;
   char cChar;

   if( !strLine || !strtoken || !iWasToken )
      return 0;
   
   iRet = lexncmp (*strLine, strtoken, strlen (strtoken));

   if (!iRet)
     {
       cChar = *(*strLine + strlen (strtoken));
       if (!isalpha ((int)cChar)&& cChar != '_')
         {
           iRet = *iWasToken = 1;
           strcpy (*strLine, (*strLine + strlen (strtoken)));
         }
       else
           iRet = *iWasToken = 0;
     }

   else
       iRet = *iWasToken = 0;

   return iRet;
}

/*  ---------------------------------------------------------------------[<]-
    Function: eatstr

    Synopsis:
    Eats strToEat from strBuff only if it begins with contents of
    strToEat.

    Examples:
       char strBuff[] = { "select * from mytbl;" };
       eatstr (&strBuff, "SELECT");

       On return here strBuff would be: " * from mytbl;"

    If the token is not found, then strBuff will not be affected and
    a NULL char * will be returned, but any white spaces on the left
    of strBuff would be trimed.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
eatstr (
    char **strBuff,
    char *strToEat)
{
   int iWasToken;

   if( !strBuff || !strToEat )
      return NULL;      

   ltrim (*strBuff);
   istoken (strBuff, strToEat, &iWasToken);

   return iWasToken ? *strBuff : (char *)NULL;
}

/*  ---------------------------------------------------------------------[<]-
    Function: eatstrpast

    Synopsis:
    Eats chars past first occurrence of one of the chars contained in
    strCharsToEatPast.

    Examples:
       char strBuff[] = { " , 456, 789" };
       eatstrpast (&strBuff, ",");

    On return here strBuff would be: " 456, 789".
    Returns a pointer to head of the input buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
eatstrpast (
    char **strBuff,
    char *strCharsToEatPast)
{
   if( !strBuff || !strCharsToEatPast )
      return NULL;
         
   ltrim (*strBuff);
   while (**strBuff && strchr (strCharsToEatPast, **strBuff))
       deletechar (*strBuff, 0);

   return *strBuff;
}

/*  ---------------------------------------------------------------------[<]-
    Function: movestrpast

    Synopsis:
    Eats chars past first occurrence of one of the chars contained in
    strCharsToEatPast.

    Examples:
       char strBuff[] = { "123, 456, 789" };
       movestrpast (&strBuff, ",");

    On return here strBuff would be: " 456, 789".
    Returns a pointer to head of the input buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
movestrpast (
    char **strBuff,
    char cCharToEatPast)
{
   if( !strBuff )
      return NULL;

   ltrim (*strBuff);
   while (**strBuff && **strBuff != cCharToEatPast)
       deletechar (*strBuff, 0);

   if (**strBuff && **strBuff == cCharToEatPast)
       deletechar (*strBuff, 0);

   return *strBuff;
}

/*  ---------------------------------------------------------------------[<]-
    Function: eatchar

    Synopsis:
    Trims white spaces and eats just past occurrence of cChar.  If
    contents of cChar is not found then only white spaces are trimmed.

    Examples:
       char strBuff[] = { "('test', 5)" };
       eatchar (&strBuff, '(');
    On return here strBuff would be: "'test', 5)".
    Returns a pointer to head of the input buffer.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

char *
eatchar (
    char **strBuff,
    char cChar)
{
   if( !strBuff )
      return NULL;

   ltrim (*strBuff);
   if (**strBuff && **strBuff == cChar)
       deletechar (*strBuff, 0);

   return *strBuff;
}

/*  ---------------------------------------------------------------------[<]-
    Function: isoneoftokens

    Synopsis:
    Eats strToEat from strBuff only if it begins with contents of
    strToEat, and returns a 0 or 1 to tell what it did. Returns 0
    if nothing found, and >= 1 which is an index of the one found.

    Examples:
       char strBuff[] = { "select * from mytbl;" };
       int iWasToken;
       isoneoftokens (&strBuff, "INSERT|SELECT|DELETE", "|", &iWasToken);

       On return here iWasToken would == 1, and strBuff would be:
       " * from mytbl;" and the return value would be 2.

    If the token is not found, then strBuff will not be affected, and
    a 0 will be returned.
    Submitted by Scott Beasley <jscottb@infoave.com>
    ---------------------------------------------------------------------[>]-*/

int
isoneoftokens (
    char **strbuf,
    char *strmat,
    char *strsep,
    int *iWasToken)
{
   int nstate = 0, cnt = 0, icmpres;
   int iLen;
   char *strtemp, cChar;

   TA_ASSERT_RET ( strbuf, 0);
   TA_ASSERT_RET ( strmat, 0 );
   TA_ASSERT_RET ( strsep, 0 );
   TA_ASSERT_RET ( iWasToken, 0 );

   while (1)
     {
       iLen = getstrfldlen (strmat, cnt, 0, strsep);
       strtemp = (char *) TA_Malloc ( iLen + 1);
       getstrfld (strmat, cnt, 0, strsep, strtemp);
       if (*strtemp)
         {
           trim (strtemp);
           icmpres = lexncmp (*strbuf, strtemp, strlen (strtemp));

           if (!icmpres)
             {
               cChar = *(*strbuf + strlen (strtemp));
               if (!isalpha ((int)cChar)&& cChar != '_')
                 {
                   *iWasToken = cnt + 1;
                   strcpy (*strbuf, (*strbuf + strlen (strtemp)));

                   nstate = cnt + 1;
                 }
               break;
             }

            else
              {
                if (!lexncmp (*strbuf, strtemp, strlen (strtemp)))
                  {
                    cChar = *(*strbuf + strlen (strtemp));
                    if (!isalpha ((int)cChar)&& cChar != '_')
                      {
                        *iWasToken = cnt + 1;
                        strcpy (*strbuf, (*strbuf + strlen (strtemp)));

                        nstate = cnt + 1;
                      }
                    break;
                  }
              }
         }

       else
         {
           *iWasToken = 0;
           nstate = 0;
           break;
         }

       cnt++;
       TA_Free ( strtemp);
     }

   return nstate;
}

#ifdef WIN32
#pragma warning( default : 4127 ) /* Restore warnings. */
#endif

