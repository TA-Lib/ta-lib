/* Note: This file has been adapted for running within TA-LIB.
 * If you intend to use iMatix, please use their original file.
 */

/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflenv.c
    Title:      String token manipulation functions.
    Package:    Standard Function Library (SFL)

    Written:    1996/09/10  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/11

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflstr.h"                     /*  Conversion functions             */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory allocation functions      */
#include "sflsymb.h"                    /*  Symbol table functions           */
#include "sfltok.h"                     /*  Prototypes for functions         */

#include "ta_memory.h"
#include "ta_trace.h"

/*  ---------------------------------------------------------------------[<]-
    Function: tok_split

    Synopsis: Accepts a string and breaks it into words, delimited by white
    space (spaces, tabs, newlines).  Returns an array of strings which ends
    in a NULL string.  If the string is empty or NULL, returns an array
    containing a single NULL value.  The array is allocated dynamically by
    this function, and you must call tok_free() to release it when you have
    finished.  Returns NULL if there was insufficient memory to complete the
    split operation.
    ---------------------------------------------------------------------[>]-*/

char **
tok_split (
    
    const char *string)
{
    char
        *buffer,
        *bufptr,
        **token_list,                   /*  Returned token list              */
        last_char = '\0';               /*  Last character parsed            */
    int
        word_count = 0,                 /*  Number of words in string        */
        word_nbr;

    /*  Allocate space for work string, up to the size of the input string   */
    if (string == NULL)
        string = "";

    buffer = TA_Malloc( strlen(string)+1 );
    if (buffer == NULL)
        return (NULL);

    /*  Now copy string and eliminate whitespace                             */
    bufptr = buffer;                    /*  Move to start of target buffer   */
    while (*string)                     /*  Copy entire string               */
        if (isspace (*string))          /*  Collapse whitespace to           */
          {                             /*    a single null byte             */
            while (isspace (*string))
                string++;
            if (bufptr > buffer)
              {
                word_count++;           /*  We have a new word               */
                last_char = *bufptr++ = '\0';
              }
          }
        else
            last_char = *bufptr++ = *string++;

    /*  Count last word if it was not terminated in a white space            */
    if (last_char > 0)
        word_count++;
    *bufptr = '\0';                     /*  End with final NULL              */

    /*  The token list starts with a pointer to the buffer, then has one     */
    /*  pointer to each string in the buffer.  It ends with a null pointer.  */
    /*  We return the address of the first string pointer, i.e. the caller   */
    /*  does not see the pointer to the buffer.  We can thus get away with   */
    /*  just two allocs; one for the buffer and one for the token list.      */
    token_list = TA_Malloc( sizeof (char *) * (word_count + 2));
    if (token_list == NULL)
        return (NULL);

    token_list [0] = buffer;            /*  Store buffer address             */
    token_list++;                       /*    and bump starting address      */

    bufptr = buffer;
    for (word_nbr = 0; word_nbr < word_count; word_nbr++)
      {
        token_list [word_nbr] = bufptr;
        bufptr += strlen (bufptr) + 1;
      }
    token_list [word_count] = NULL;     /*  Store final null pointer         */
    return (token_list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tok_split_rich

    Synopsis: Works as the tok_split() function, but handles tokens that are
    delimited by user-specified characters.  A typical use of this function
    is to handle quoted strings.  Each character in the delims argument is a
    potential token delimiter.  For instance, to handle strings defined with
    single or double quotes, you could pass "\"'" as the delims argument.
    Note that this function always splits on spaces outside delimited tokens.
    ---------------------------------------------------------------------[>]-*/

char **
tok_split_rich (
    
    const char *string,
    const char *delims)
{
    char
        *buffer,
        *bufptr,
        **token_list,                   /*  Returned token list              */
        delim,                          /*  Current delimiter character      */
        last_char = '\0';               /*  Last character parsed            */
    int
        word_count = 0,                 /*  Number of words in string        */
        word_nbr;

    /*  Allocate space for work string, up to the size of the input string   */
    buffer = TA_Malloc( strlen (string) + 1 );
    if (buffer == NULL)
        return (NULL);

    /*  Now copy string and eliminate spaces and cut on delimiters           */
    bufptr = buffer;                    /*  Move to start of target buffer   */
    if (string)                         /*  Allow string to be NULL          */
      {
        while (*string)                 /*  Copy entire string               */
            if (strchr (delims, *string))
              {                         /*  User-specified delimiter         */
                word_count++;           /*  Count the word                   */
                delim = *string++;      /*    and skip the delimiter         */
                while (*string != delim)
                  {
                    if (*string)
                        *bufptr++ = *string++;
                    else
                        break;          /*  Unfinished token, but allright   */
                  }
                last_char = *bufptr++ = '\0';
                if (*string == delim)
                    string++;           /*  Skip final delimiter             */
                while (isspace (*string))
                    string++;           /*    and any trailing spaces        */
              }
            else
            if (isspace (*string))      /*  Collapse whitespace to           */
              {                         /*    a single null byte             */
                word_count++;           /*    unless at the start            */
                while (isspace (*string))
                    string++;
                if (bufptr > buffer)
                    last_char = *bufptr++ = '\0';
              }
            else
                last_char = *bufptr++ = *string++;
      }
    /*  Count last word if it was not terminated in a white space            */
    if (last_char > 0)
        word_count++;

    *bufptr = '\0';                     /*  End with final NULL              */

    /*  The token list starts with a pointer to the buffer, then has one     */
    /*  pointer to each string in the buffer.  It ends with a null pointer.  */
    /*  We return the address of the first string pointer, i.e. the caller   */
    /*  does not see the pointer to the buffer.  We can thus get away with   */
    /*  just two allocs; one for the buffer and one for the token list.     */
    token_list = TA_Malloc( sizeof(char *) * (word_count + 2));
    if (token_list == NULL)
        return (NULL);

    token_list [0] = buffer;            /*  Store buffer address             */
    token_list++;                       /*    and bump starting address      */

    bufptr = buffer;
    for (word_nbr = 0; word_nbr < word_count; word_nbr++)
      {
        token_list [word_nbr] = bufptr;
        bufptr += strlen (bufptr) + 1;
      }
    token_list [word_count] = NULL;     /*  Store final null pointer         */
    return (token_list);

}

/*  ---------------------------------------------------------------------[<]-
    Function: tok_free

    Synopsis: Frees the memory allocated by a tok_split() call.  You should
    call this function for each call to tok_split(), to avoid memory leaks.
    Do not try to free the allocated memory yourself, as the structure of a
    token list is not documented and may change over time.
    ---------------------------------------------------------------------[>]-*/

void
tok_free (
    
    char **token_list)
{
    if (token_list)
      {
        token_list--;                   /*  Back-up to get right address     */
        TA_Free( token_list [0] );      /*  Free buffer                      */
        TA_Free( token_list );          /*    and free token list            */
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: tok_push

    Synopsis: Prepends a string to the specified token table.  Reallocates
    the table as required to make room for the new string.  Returns the new
    token table -- the supplied token table is automatically freed by a call
    to tok_free().
    ---------------------------------------------------------------------[>]-*/

char **
tok_push (
    
    char **token_list,
    const char *string)
{
    char
        *new_buffer,                    /*  Newly-allocated buffer           */
        **new_list,                     /*  Returned token list              */
        *new_bufptr;                    /*  Pointer into new buffer          */
    int
        word_count,                     /*  Number of words in string        */
        word_nbr;
    size_t
        buffer_size,
        string_size,
        new_buffer_size;

    buffer_size = tok_text_size (token_list);
    word_count  = tok_size      (token_list);

    string_size = strlen (string);
    new_buffer_size = buffer_size + string_size + 1;

    /*  New list has one entry for each in old list, plus header, plus null
     *  entry at end, plus one for the new string -- makes 3 more.
     */
    new_list   = TA_Malloc( (sizeof (char *) * (word_count + 3)));
    new_buffer = TA_Malloc( (new_buffer_size));
    if (new_list == NULL || new_buffer == NULL) 
      {
        TA_Free( new_list );
        TA_Free( new_buffer );
        return (NULL);
      }

    word_count++;                       /*  We add one word                  */
    strncpy (new_buffer, string, new_buffer_size);
    memcpy (new_buffer + string_size + 1, token_list [-1], buffer_size);
    new_list [0] = new_buffer;          /*  Store buffer address             */
    new_list++;                         /*    and bump starting address      */

    new_bufptr = new_buffer;
    for (word_nbr = 0; word_nbr < word_count; word_nbr++)
      {
        new_list [word_nbr] = new_bufptr;
        new_bufptr += strlen (new_bufptr) + 1;
      }
    new_list [word_count] = NULL;       /*  Store final null pointer         */
    tok_free (token_list);              /*  Free old list                    */
    return (new_list);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tok_size

    Synopsis: Returns size of specified token list, in entries.  Stops at
    the empty terminating token.  Thus the table "This", "Is", "A", "String"
    will return a size of 4.
    ---------------------------------------------------------------------[>]-*/

int
tok_size (
    
    char **token_list)
{
    int
        word_nbr;

    for (word_nbr = 0; token_list [word_nbr]; word_nbr++);
    return (word_nbr);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tok_text_size

    Synopsis: Returns size of specified token list, in characters.  Counts
    the size of each token, plus one terminator for each token.  Thus the
    table "This", "Is", "A", "String" will return a size of 17.
    ---------------------------------------------------------------------[>]-*/

size_t
tok_text_size (
    
    char **token_list)
{
    size_t
        text_size;
    int
        word_nbr;

    text_size = 0;
    for (word_nbr = 0; token_list [word_nbr]; word_nbr++)
        text_size += strlen (token_list [word_nbr]) + 1;

    return (text_size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: tok_subst

    Synopsis: Performs symbol substitution into the specified string.
    Returns a newly-allocated string containing the result, or NULL if there
    was not enough memory.  The symbol table holds the set of symbols that
    may be inserted.  Undefined symbols are inserted as an empty value.
    Symbols are specified in the string using this syntax: $(name), where
    'name' is case-sensitive.  This version does not allow embedded symbols.
    ---------------------------------------------------------------------[>]-*/
#if 0
!!! Unused
char *
tok_subst ( 
            const char *string,
            SYMTAB *symbols)
{
    char
        *sym_start,
        *sym_end,
        *sym_name,                      /*  Symbol name to look for          */
        *copied_to,                     /*  Up to where we copied so far     */
        *cur_result,                    /*  Current result buffer            */
        *new_result;                    /*  After adding next symbol         */
    int
        sym_length;                     /*  Length of symbol name            */
    SYMBOL
        *symbol;                        /*  Symbol in symbol table           */

    TA_ASSERT_RET ( string, NULL );
    TA_ASSERT_RET ( symbols, NULL );

    /*  Parse from left to right, looking for $(...) patterns                */
    cur_result = mem_strdup ("");       /*  Result buffer is empty           */
    sym_start  = strchr (string, '$');
    copied_to  = (char *) string;
    while (sym_start)
      {
        sym_end = strchr (++sym_start, ')');
        if (*sym_start == '('
        &&   sym_end
        &&  *sym_end   == ')')
          {
            /*  First, copy string up to sym_start                           */
            size_t cur_len = strlen (cur_result);
            TA_ASSERT_RET ( (sym_start - copied_to) >= 0);

            new_result = TA_Malloc(  (cur_len + (sym_start - copied_to)));
            if (new_result == NULL)
              {
                TA_Free(   (cur_result));
                return (NULL);
              }

            strncpy (new_result, cur_result, (cur_len + 1));
            strncat (new_result, copied_to, sym_start - 1 - copied_to);
            TA_Free(   (cur_result));
            cur_result = new_result;
            copied_to  = sym_end + 1;

            /*  Now translate and insert symbol                              */
            sym_length = sym_end - sym_start - 1;
            sym_name   = TA_Malloc(  (sym_length + 1) );
            if (sym_name == NULL)
              {
                TA_Free(   (cur_result));
                return (NULL);
              }

            memcpy (sym_name, sym_start + 1, sym_length);
            sym_name [sym_length] = 0;
            symbol = sym_lookup_symbol (symbols, sym_name);
            TA_Free(   (sym_name));
            if (symbol)
              {
                xstrcpy_debug ();
                new_result = xstrcpy (NULL, cur_result, symbol-> value, NULL);
                TA_Free(   (cur_result));
                cur_result = new_result;
              }
          }
        else
            sym_end = sym_start + 1;

        sym_start = strchr (sym_end, '$');
      }
    /*  Copy anything remaining in the string                                */
    xstrcpy_debug ();
    new_result = xstrcpy (NULL, cur_result, copied_to, NULL);
    TA_Free(   (cur_result));
    return (new_result);
}

#endif
