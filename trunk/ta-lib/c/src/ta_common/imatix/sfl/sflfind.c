/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflfind.c
    Title:      Fast string searching functions
    Package:    Standard Function Library (SFL)

    Written:    1996/04/24  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/05/04

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflfind.h"                    /*  Prototypes for functions         */


/*  ---------------------------------------------------------------------[<]-
    Function: strfind

    Synopsis: Searches for a pattern in a string using the Boyer-Moore-
    Horspool-Sunday algorithm.  The string and pattern are null-terminated
    strings.  Returns a pointer to the pattern if found within the string,
    or NULL if the pattern was not found.  If you repeatedly scan for the
    same pattern, use the repeat_find argument.  If this is TRUE, the
    function does not re-parse the pattern.  You must of course call the
    function with repeat_find equal to FALSE the first time.  This function
    is meant to handle  character data, and is most effective when you work
    with large strings.  To search binary data use memfind().  Will not work
    on multibyte characters.

    Examples:
    char *result;

    result = strfind ("abracadabra", "cad", FALSE);
    if (result)
        puts (result);
    ---------------------------------------------------------------------[>]-*/

char *
strfind (const char *string,            /*  String containing data           */
         const char *pattern,           /*  Pattern to search for            */
         Bool repeat_find)              /*  Same pattern as last time        */
{
    static size_t
        searchbuf [256];                /*  Fixed search buffer              */

    ASSERT (string);                    /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  fall through if not debugging    */

    return (char *) memfind_rb (string,    strlen (string),
                                pattern,   strlen (pattern),
                                searchbuf, &repeat_find);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strfind_r

    Synopsis: Searches for a pattern in a string using the Boyer-Moore-
    Horspool-Sunday algorithm.  The string and pattern are null-terminated
    strings.  Returns a pointer to the pattern if found within the string,
    or NULL if the pattern was not found.  This function is meant to handle
    character data, and is most effective when you work with large strings.
    To search binary data use memfind().  Will not work on multibyte
    characters.  Reentrant.

    Examples:
    char *result;

    result = strfind_r ("abracadabra", "cad");
    if (result)
        puts (result);
    ---------------------------------------------------------------------[>]-*/

char *
strfind_r (const char *string,          /*  String containing data           */
           const char *pattern)         /*  Pattern to search for            */
{
    size_t
        searchbuf [256];                /*  One-time search buffer           */
    Bool
        secondtime = FALSE;             /*  Search buffer init needed        */

    ASSERT (string);                    /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  fall through if not debugging    */

    return (char *) memfind_rb (string,    strlen (string),
                                pattern,   strlen (pattern),
                                searchbuf, &secondtime);
}


/*  ---------------------------------------------------------------------[<]-
    Function: strfind_rb

    Synopsis: Searches for a pattern in a string using the Boyer-Moore-
    Horspool-Sunday algorithm.  The string and pattern are null-terminated
    strings.  Returns a pointer to the pattern if found within the string,
    or NULL if the pattern was not found.  Supports more efficient repeat
    searches (for the same pattern), through a supplied search buffer.  The
    search buffer must be long enough to contain 256 (2**8) size_t entries.
    On the first call repeat_find must be set to FALSE.  After the search
    buffer has been initialised, repeat_find will be set to TRUE by the
    function, avoiding the search buffer initialisation on later calls.

    This function is most effective when repeated searches are made for
    the same pattern in one or more strings.  This function is meant to
    handle  character data, and is most effective when you work with
    large strings.  To search binary data use memfind().  Will not work
    on multibyte characters.  Reentrant.

    Examples:
    char   *result;
    Bool   repeat_search = FALSE;
    size_t searchbuf[256];

    result = strfind_rb ("abracadabra", "cad", searchbuf, &repeat_search);
    if (result)
      {
        puts (result);
        result = strfind_rb ("cad/cam", "cad", searchbuf, &repeat_search);
        if (result)
            puts (result);
      }
    ---------------------------------------------------------------------[>]-*/

char *
strfind_rb (const char *string,         /*  String containing data           */
            const char *pattern,        /*  Pattern to search for            */
            size_t     *shift,          /*  Working buffer between searches  */
            Bool       *repeat_find)    /*  Flag for first/later search      */
{
    ASSERT (string);                    /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  fall through if not debugging    */
    ASSERT (shift);
    ASSERT (repeat_find);

    return (char *) memfind_rb (string,    strlen (string),
                                pattern,   strlen (pattern),
                                shift,     repeat_find);
}


/*  ---------------------------------------------------------------------[<]-
    Function: memfind

    Synopsis: Searches for a pattern in a block of memory using the Boyer-
    Moore-Horspool-Sunday algorithm.  The block and pattern may contain any
    values; you must explicitly provide their lengths.  Returns a pointer to
    the pattern if found within the block, or NULL if the pattern was not
    found.  If you repeatedly scan for the same pattern, use the repeat_find
    argument.  If this is TRUE, the function does not re-parse the pattern.
    This function is meant to handle binary data.  If you need to search
    strings, use the strfind_r or strfind_rb() functions.  Non-Reentrant.
    ---------------------------------------------------------------------[>]-*/

void *
memfind (const void  *block,            /*  Block containing data            */
         size_t       block_size,       /*  Size of block in bytes           */
         const void  *pattern,          /*  Pattern to search for            */
         size_t       pattern_size,     /*  Size of pattern block            */
         Bool         repeat_find)      /*  Same pattern as last time        */
{
    static size_t
        searchbuf [256];                /*  Static shared search buffer      */

    ASSERT (block);                     /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  full through if not debugging    */

    return memfind_rb (block, block_size, pattern, pattern_size,
                       searchbuf, &repeat_find);
}

/*  ---------------------------------------------------------------------[<]-
    Function: memfind_r

    Synopsis: Searches for a pattern in a block of memory using the Boyer-
    Moore-Horspool-Sunday algorithm.  The block and pattern may contain any
    values; you must explicitly provide their lengths.  Returns a pointer to
    the pattern if found within the block, or NULL if the pattern was not
    found.

    This function is meant to handle binary data, for a single search for
    a given pattern.  If you need to search strings, use the strfind_r()
    or strfind_rb() functions.  If you want to do efficient repeated searches
    for one pattern, use memfind_rb().  Reentrant.
    ---------------------------------------------------------------------[>]-*/

void *
memfind_r (const void  *block,          /*  Block containing data            */
           size_t       block_size,     /*  Size of block in bytes           */
           const void  *pattern,        /*  Pattern to search for            */
           size_t       pattern_size)   /*  Size of pattern block            */
{
    size_t
        searchbuf [256];                /*  One-time search buffer           */
    Bool
        secondtime  = FALSE;

    ASSERT (block);                     /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  full through if not debugging    */

    return memfind_rb (block, block_size, pattern, pattern_size,
                       searchbuf, &secondtime);
}


/*  ---------------------------------------------------------------------[<]-
    Function: memfind_rb

    Synopsis: Searches for a pattern in a block of memory using the Boyer-
    Moore-Horspool-Sunday algorithm.  The block and pattern may contain any
    values; you must explicitly provide their lengths.  Returns a pointer to
    the pattern if found within the block, or NULL if the pattern was not
    found.  On the first search with a given pattern, *repeat_find should
    be FALSE.  It will be set to TRUE after the shift table is initialised,
    allowing the initialisation phase to be skipped on subsequent searches.
    shift must point to an array big enough to hold 256 (8**2) size_t values.

    This function is meant to handle binary data, for repeated searches
    for the same pattern.  If you need to search strings, use the
    strfind_r() or strfind_rb() functions.  If you wish to search for a
    pattern only once consider using memfind_r(). Reentrant.
    ---------------------------------------------------------------------[>]-*/

void *
memfind_rb (const void  *in_block,      /*  Block containing data            */
            size_t       block_size,    /*  Size of block in bytes           */
            const void  *in_pattern,    /*  Pattern to search for            */
            size_t       pattern_size,  /*  Size of pattern block            */
            size_t      *shift,         /*  Shift table (search buffer)      */
            Bool        *repeat_find)   /*  TRUE: search buffer already init */
{
    size_t
        byte_nbr,                       /*  Distance through block           */
        match_size;                     /*  Size of matched part             */
    const byte
        *match_base = NULL,             /*  Base of match of pattern         */
        *match_ptr  = NULL,             /*  Point within current match       */
        *limit      = NULL;             /*  Last potiental match point       */
    const byte
        *block   = (byte *) in_block,   /*  Concrete pointer to block data   */
        *pattern = (byte *) in_pattern; /*  Concrete pointer to search value */

    ASSERT (block);                     /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  fail gracefully if not debugging */
    ASSERT (shift);                     /*  NULL repeat_find => is false     */
    if (block == NULL || pattern == NULL || shift == NULL)
        return (NULL);

    /*  Pattern must be smaller or equal in size to string                   */
    if (block_size < pattern_size)
        return (NULL);                  /*  Otherwise it's not found         */

    if (pattern_size == 0)              /*  Empty patterns match at start    */
        return ((void *)block);

    /*  Build the shift table unless we're continuing a previous search      */

    /*  The shift table determines how far to shift before trying to match   */
    /*  again, if a match at this point fails.  If the byte after where the  */
    /*  end of our pattern falls is not in our pattern, then we start to     */
    /*  match again after that byte; otherwise we line up the last occurence */
    /*  of that byte in our pattern under that byte, and try match again.    */

    if (!repeat_find || !*repeat_find)
      {
        for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
            shift [byte_nbr] = pattern_size + 1;
        for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
            shift [(byte) pattern [byte_nbr]] = pattern_size - byte_nbr;

        if (repeat_find)
            *repeat_find = TRUE;
      }

    /*  Search for the block, each time jumping up by the amount             */
    /*  computed in the shift table                                          */

    limit = block + (block_size - pattern_size + 1);
    ASSERT (limit > block);

    for (match_base = block;
         match_base < limit;
         match_base += shift [*(match_base + pattern_size)])
      {
        match_ptr  = match_base;
        match_size = 0;

        /*  Compare pattern until it all matches, or we find a difference    */
        while (*match_ptr++ == pattern [match_size++])
          {
            ASSERT (match_size <= pattern_size && 
                    match_ptr == (match_base + match_size));

            /*  If we found a match, return the start address                */
            if (match_size >= pattern_size)
              return ((void*)(match_base));

          }
      }
    return (NULL);                      /*  Found nothing                    */
}


/*  ---------------------------------------------------------------------[<]-
    Function: txtfind

    Synopsis: Searches for a case-insensitive text pattern in a string
    using the Boyer-Moore-Horspool-Sunday algorithm.  The string and
    pattern are null-terminated strings.  Returns a pointer to the pattern
    if found within the string, or NULL if the pattern was not found.
    Will match strings irrespective of case.  To match exact strings, use
    strfind().  Will not work on multibyte characters.

    Examples:
    char *result;

    result = txtfind ("AbracaDabra", "cad");
    if (result)
        puts (result);
    ---------------------------------------------------------------------[>]-*/

char *
txtfind (const char *string,            /*  String containing data           */
         const char *pattern)           /*  Pattern to search for            */
{
    size_t
        shift [256];                    /*  Shift distance for each value    */
    size_t
        string_size,
        pattern_size,
        byte_nbr,                       /*  Index into byte array            */
        match_size;                     /*  Size of matched part             */
    const char
        *match_base = NULL,             /*  Base of match of pattern         */
        *match_ptr  = NULL,             /*  Point within current match       */
        *limit      = NULL;             /*  Last potiental match point       */

    ASSERT (string);                    /*  Expect non-NULL pointers, but    */
    ASSERT (pattern);                   /*  fail gracefully if not debugging */
    if (string == NULL || pattern == NULL)
        return (NULL);

    string_size  = strlen (string);
    pattern_size = strlen (pattern);

    /*  Pattern must be smaller or equal in size to string                   */
    if (string_size < pattern_size)
        return (NULL);                  /*  Otherwise it cannot be found     */

    if (pattern_size == 0)              /*  Empty string matches at start    */
        return (char *) string;

    /*  Build the shift table                                                */

    /*  The shift table determines how far to shift before trying to match   */
    /*  again, if a match at this point fails.  If the byte after where the  */
    /*  end of our pattern falls is not in our pattern, then we start to     */
    /*  match again after that byte; otherwise we line up the last occurence */
    /*  of that byte in our pattern under that byte, and try match again.    */

    for (byte_nbr = 0; byte_nbr < 256; byte_nbr++)
        shift [byte_nbr] = pattern_size + 1;

    for (byte_nbr = 0; byte_nbr < pattern_size; byte_nbr++)
        shift [(byte) tolower (pattern [byte_nbr])] = pattern_size - byte_nbr;

    /*  Search for the string.  If we don't find a match, move up by the     */
    /*  amount we computed in the shift table above, to find location of     */
    /*  the next potiental match.                                            */

    limit = string + (string_size - pattern_size + 1);
    ASSERT (limit > string);

    for (match_base = string;
         match_base < limit;
         match_base += shift [(byte) tolower (*(match_base + pattern_size))])
      {
        match_ptr  = match_base;
        match_size = 0;

        /*  Compare pattern until it all matches, or we find a difference    */
        while (tolower (*match_ptr++) == tolower (pattern [match_size++]))
          {
            ASSERT (match_size <= pattern_size && 
                    match_ptr == (match_base + match_size));

            /*  If we found a match, return the start address                */
            if (match_size >= pattern_size)
                return ((char *)(match_base));
          }
      }
    return (NULL);                      /*  Found nothing                    */
}

