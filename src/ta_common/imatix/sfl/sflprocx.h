/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflprocx.h
    Title:      Process control functions - extra functions
    Package:    Standard Function Library (SFL)

    Written:    1996/09/09  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/03/22

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/


/*  -------------------------------------------------------------------------
 *  This header file defines static variables and functions that may be used
 *  in the sflproc implementations.  We don't define function prototypes -
 *  this file must be included before the process_create_full() function
 *  definition.
 *  -------------------------------------------------------------------------*/

/*  Define default and system-specific values.
 *  The default_ext table lists extensions for all files that we can run
 *  directly; the runnable_ext table lists extensions that are directly
 *  runnable.  The script_ext table lists extensions that must be passed to
 *  a shell for execution.
 */
#if   (defined (__OS2__))
static const char *default_ext  [] = { "exe", "cmd", NULL };
static const char *runnable_ext [] = { "exe", NULL };
static const char *script_ext   [] = { "cmd", NULL };
static const char *default_shell   = "COMSPEC";
static const char *shell_run       = "/c";
#elif (defined (WIN32))
static const char *default_ext  [] = { "exe", "com", "bat", NULL };
static const char *runnable_ext [] = { "exe", "com", "bat", NULL };
static const char *script_ext   [] = { NULL };
static const char *default_shell   = "COMSPEC";
static const char *shell_run       = "/c";
#else
static const char *default_ext  [] = { NULL };
static const char *runnable_ext [] = { NULL };
static const char *script_ext   [] = { NULL };
static const char *default_shell   = "SHELL";
static const char *shell_run       = "-c";
#endif


/*  We use linked lists to create the command line with arguments            */

typedef struct _ARGLIST {
    struct _ARGLIST
        *next, *prev;
    char *value;
} ARGLIST;

#define LIST_BEFORE     0               /*  Flags for arglist calls          */
#define LIST_AFTER      1


/*  --------------------------------------------------------------------------
 *  arglist_add -- local
 *
 *  Creates a new ARGLIST node with a copy of the specified string, and adds
 *  it to the end or start of the specified argument list.
 */
 
static void
arglist_add (ARGLIST *list, int where, const char *value)
{
    /*  NOTE: There _must_not_ be a cast of the "value" parameter in the     */
    /*  macro calls list_queue or list_push, because it does not expand      */
    /*  correctly with gcc 2.7.x.  For this reason a local variable of the   */
    /*  required type is used to hold a pointer to the copy of the string.   */

    void *item = mem_strdup (value);    /*  Allocate a copy of the string    */
    
    if (where == LIST_AFTER)
        list_queue (((LIST *) list), item);
    else
        list_push  (((LIST *) list), item);
}


/*  --------------------------------------------------------------------------
 *  arglist_add_table -- local
 *
 *  Adds a token list specified to an arglist, at the start or end.  The token
 *  list is an array of char pointers, ending in a NULL pointer.  Does nothing
 *  if the token list is null.
 */
 
static void
arglist_add_table (ARGLIST *list, int where, char **tokens)
{
    int
        token_nbr;
        
    if (tokens)
      {
        for (token_nbr = 0; tokens [token_nbr]; token_nbr++)
            if (where == LIST_AFTER)
                arglist_add (list, LIST_AFTER, tokens [token_nbr]);
        if (where == LIST_BEFORE)
            for (--token_nbr; token_nbr >= 0; token_nbr--)
                arglist_add (list, LIST_BEFORE, tokens [token_nbr]);
      }
}

/*  --------------------------------------------------------------------------
 *  arglist_add_string -- local
 *
 *  Appends a string of zero or more terms onto an arglist.  Each word in the
 *  string is attached as a single node in the argument list.
 */
 
static void
arglist_add_string (ARGLIST *list, int where, const char *string)
{
    char
        **tokens;

    tokens = tok_split (string);
    arglist_add_table (list, where, tokens);
    tok_free (tokens);
}

/*  --------------------------------------------------------------------------
 *  arglist_remove_first -- local
 *
 *  Removes the first node from the argument list, assuming there is an item
 *  there.
 */
 
static void
arglist_remove_first (ARGLIST *list)
{
    ARGLIST 
        *next = list->next;

    list_unlink (next);
    mem_strfree (&next-> value);
    mem_free (next);
}

/*  --------------------------------------------------------------------------
 *  arglist_free -- local
 *
 *  Frees all memory used by the specified argument list, including the list
 *  head.  All items on the list including the head must be ARGLIST nodes.
 *  (The unix code simply leaves the process which allocates the memory,
 *  so does not call this function.)
 */

#if (! defined (__UNIX__))
static void
arglist_free (ARGLIST *list)
{
    ARGLIST
        *next;

    while (list-> next != list)
      {
        next = list-> next;
        list_unlink (next);
        mem_strfree (&next-> value);
        mem_free (next);
      }
    mem_free (list);
}
#endif

/*  --------------------------------------------------------------------------
 *  arglist_value -- local
 *
 *  Returns a string consisting of the concatenation of the argument list
 *  values, separated by spaces.  The returned string is provided in a freshly
 *  allocated buffer.  Returns NULL if there was insufficient memory.
 */

#if ((! defined (__UNIX__)) && (! defined (__OS2__)))
static char *
arglist_value (ARGLIST *list)
{
    ARGLIST
        *node;                          /*  Each node in the list            */
    int
        value_size;                     /*  Total size of value              */
    char
        *value;                         /*  Full concatenated value          */

    value_size = 0;
    for (node = list-> next; node != list; node = node-> next)
        if (node-> value)               /*  Count size of value + space      */
            value_size += strlen (node-> value) + 1;

    value = mem_alloc (value_size + 1);
    if (value)
      {
        /*  Append each value; null values will not be followed by a space   */
        value [0] = '\0';
        for (node = list-> next; node != list; node = node-> next)
            xstrcat (value, node-> value, " ", NULL);
        strcrop (value);                /*  Drop trailing space(s)           */
      }
    return (value);
}
#endif

/*  --------------------------------------------------------------------------
 *  arglist_to_table -- local
 *
 *  Returns a argv style array of strings which can be used as arguments
 *  to exec*(), or spawn*() (under unix and OS/2 respectively).  The 
 *  array of strings is in freshly allocated memory, but the the strings
 *  themselves are simply the ones in the arglist directly.  Take care
 *  not to free them prematurely, or twice, but ensure that the array is
 *  freed.  Returns NULL if there was insufficient memory.
 */

#if (defined (__UNIX__) || defined (__OS2__))
static char **
arglist_to_table (ARGLIST *list)
{
    ARGLIST
        *node;                          /*  Each node in the list            */
    int
        length = 0;                     /*  Length of arglist                */
    char **
        array = NULL;

    /*  Figure out number of items in the list                               */
    for (node = list-> next; node != list; node = node-> next)
         ++length;

    array = mem_alloc ((length + 1) * sizeof (char *));
    if (array)
      {
        for (node = list-> next, length = 0; 
             node != list; 
             node = node-> next, ++length)

             array [length] = node-> value;

        array [length] = NULL;

        return array;
      }
    else
        return NULL;
}
#endif


/*  --------------------------------------------------------------------------
 *  merge_environment -- local
 *
 *  Creates a new environment variable bundle which contains the entries from
 *  curenv, with the entries from envadd put in (overwriting any existing
 *  entries of that name), and the entries for the keys in envrm removed.
 *
 *  curenv may be NULL, in which case the processes current environment is
 *  used.  If envadd or envrm are NULL then no change is made for that part.
 *
 *  Returns the new environment variable bundle, or NULL if errors encountered.
 *
 *  When the environment is no longer required it should be freed with
 *  strtfree().
 *
 *  NOTE: strt2symb() is used to convert the environment to a symbol table,
 *  rather than env2symb(), so no conversions are applied to the strings.
 *  The environment should be "ready to go" prior to using this function.
 */

static char **
merge_environment (char **cur_env, SYMTAB *envadd, SYMTAB *envrm)
{
    int
        rc = 0;
    SYMTAB
        *envtable = NULL;
    char
        **new_env = NULL,
        **base_env;
    SYMBOL
        *symbol,                        /*  Next symbol in table             */
        *symbol_found;                  /*  Symbol to remove from table      */

    base_env = cur_env? cur_env: environ;
    ASSERT (base_env);
    if (!base_env)
        return NULL;

    ASSERT (envadd || envrm);     /*  Should be doing some translation       */

    /*  If no changes are required, just return copy of base environment     */
    if (!envadd && !envrm)
        return (env_copy (base_env));

    envtable = strt2symb (cur_env);
    if (envtable == NULL)
        return (NULL);

    if (envadd)
      {
        rc = sym_merge_tables (envtable, envadd);
        ASSERT (rc > 0);                /*  0 = nothing imported; +ve okay   */
        if (rc < 0)                     /*  -ve means error during import    */
          {
            sym_delete_table (envtable);
            return (NULL);
          }
      }
    if (envrm)
      {
        /*  To remove the symbols we process the whole envrm table,          */
        /*  removing symbols from the main table                             */
        for (symbol = envrm-> symbols; symbol; symbol = symbol-> next)
          {
            symbol_found = sym_lookup_symbol (envtable, symbol-> name);
            while (symbol_found)
                symbol_found = sym_delete_symbol (envtable, symbol_found);
          }
    }
    /*  Now turn the symbol table back into a set of environment variables   */
    new_env = symb2env (envtable);

    /*  Free up the symbol table, and exit.                                  */
    sym_delete_table (envtable);

    return (new_env);
}


/*  --------------------------------------------------------------------------
 *  redirect_via_interpreter -- local
 *
 *  If the specified file is an executable script, extracts the name of the
 *  script interpreter plus arguments from the first line of the file, plus
 *  the full name of the script file to be executed, and returns this string
 *  in a static buffer.  If the specified file is not an executable script,
 *  or is not found, returns NULL.
 *
 *  Searches for the file on the path if necessary and searchpath is TRUE.
 *  The first line of the file should contain "#! interpreter" or "/ *!
 *  interpreter" (without the space), and under OS/2, accepts a line starting 
 *  with "EXTPROC".
 *
 *  Handles an interpreter name like '/usr/bin/perl' as follows: if the
 *  full filename exists, returns that.  Else strips off the leading path
 *  and looks for the program name on the PATH.  If that exists, returns
 *  just the program name (plus any arguments), else returns NULL.
 *
 *  If the file does not contain a magic line, but has an extension listed in
 *  the script_ext table, returns the name of the shell plus options.
 *
 *  To allow filenames with spaces, the specified filename should have been
 *  passed through process_escape() before calling this function.
 *
 *  We implement this function on all OSes, so that features like the "/ *!"
 *  (without the space) invocation for ReXX scripts are portable.  Note that 
 *  under Unix, the test 'file_is_program()' should be done before calling 
 *  this function, and so take care of normal executable scripts.
 */

static char *
redirect_via_interpreter (
    char  *filename,                    /*  Name of file we want to execute  */
    Bool   searchpath,                  /*  Search on path?                  */
    const char  *suppliedpath,          /*  If so, what path symbol          */
    const char **searchext,             /*  Executable extensions to use     */
    const char  *shell)                 /*  Shell to use for script_ext's    */
{
    static char
        curline [LINE_MAX + 1];         /*  First line from file             */
    const char
        *path,                          /*  Path to search along             */
        **extensions;                   /*  Extensions to try on command     */
    char
        *full_filename,                 /*  Filename with path               */
        *arguments,                     /*  Program arguments if any         */
        *shell_command = NULL,          /*  Name of shell interpreter        */
        *extension;                     /*  File extension                   */
    FILE
        *stream = NULL;                 /*  File input stream                */
    Bool
        redirected = FALSE;             /*  Did we redirect the filename?    */
    int
        ext_index;                      /*  Index into script_ext table      */

    ASSERT (filename);
    if (!filename)
        return (NULL);

    extensions = searchext? searchext: default_ext;
    if (searchpath == FALSE)
        path = NULL;  
    else
        path = suppliedpath? suppliedpath: "PATH";

    if (strlen (filename) > LINE_MAX)
        return (NULL);                  /*  Filename is too long for us      */
    process_unesc (curline, filename);  /*  Unescape any spaces in filename  */

    /*  We look for the file on the path, if searchpath is true.  We first
     *  try the filename just as we got it, possibly without any extensions,
     *  and then if that didn't work, we try our default extensions (and
     *  optionally mandatory extensions).                
     */
    full_filename = file_where_ext ('r', path, curline, NULL);
    if (!full_filename)
        full_filename = file_where_ext ('r', path, curline, extensions);

    /*  Quick exit if we did not find the file on the path as specified      */
    if (!full_filename)
        return (NULL);

    /*  Save full filename, since it's in a static buffer in sflfile         */
    full_filename = mem_strdup (full_filename);
    redirected = FALSE;

    /*  Open the file, and look for an interpreter name                      */
    stream = fopen (full_filename, "r");
    if (stream)
      {
        if (file_read (stream, curline))
          {
            strconvch (curline, '\\', '/');
            if (memcmp (curline, "#!",  2) == 0)
              {
                redirected = TRUE;
                shell_command = curline + 2;
              }
            else
            if (memcmp (curline, "/*!", 3) == 0)
              {
                /*  Remove closing OS/2 style comment if present             */
                char
                    *close_comments = strstr (curline, "*/");
                if (close_comments)
                    *close_comments = '\0';
                redirected = TRUE;
                shell_command = curline + 3;
              }
#   if (defined (__OS2__))
            /*  Look for EXTPROC line in both capitals and lower case.
             *  NOTE: If the EXTPROC line happens to specify a command
             *  processor that understands EXTPROC lines then if it is
             *  poorly written (eg 4OS/2) it may attempt to run itself
             *  over the script repeatedly until running out of memory.  
             */
            else
            if (lexncmp (curline, "EXTPROC", 7) == 0)
              {
                redirected = TRUE;
                shell_command = curline + 7;
              }
#   endif
          }
        file_close (stream);
      }
    if (redirected)
      {
        /*  Skip spaces and pick-up the interpreter name                     */
        strcrop (curline);
        while (*shell_command == ' ')
            shell_command++;

        /*  Separate shell name from arguments, if any                       */
        arguments = strchr (shell_command, ' ');
        if (arguments)
            *arguments = '\0';

        /*  If shell name is not empty, find full executable filename        */
        if (strnull (shell_command))
            redirected = FALSE;         /*  Empty line - can't redirect      */
      }
    if (redirected)
      {
        /*  Now check we can find shell as specified or on path              */
        if (!file_is_program (shell_command))
          {
            /*  Strip leading path and search on PATH                        */
            shell_command = strrchr (shell_command, '/');
            if (shell_command && !file_is_program (++shell_command))
                redirected = FALSE;
          }
      }
    if (redirected)
      {
        /*  Shell name is still just before arguments; we put the space back
         *  to turn this into a nice string again.
         */
        if (arguments)
            *arguments = ' '; 
      }
    else
      {
        /*  Now, check if filename matches the script_ext table              */
        if (full_filename)
            extension = strrchr (full_filename, '.');
        else
            extension = strrchr (filename, '.');
        if (extension == NULL
        ||  strchr (extension, '/')     /*  Last '.' is part of path         */
        ||  strchr (extension, '\\'))   /*    => filename has no ext         */
            extension = NULL;

        shell_command = NULL;              /*  Nothing executable found         */
        if (extension)
          {
            extension++;                /*  Bump past dot                    */
            for (ext_index = 0; script_ext [ext_index]; ext_index++)
                if (lexcmp (extension, script_ext [ext_index]) == 0)
                  {
                    xstrcpy (curline, shell, " ", shell_run, NULL);
                    shell_command = curline;
                    break;
                  }
          }
      }

    /* If we found the interpreter, append the translated full filename      */
    /* onto the end of the string, to use to replace the existing one.       */
    if (shell_command && full_filename)
        xstrcat (shell_command, " ", full_filename, NULL);
    
    mem_strfree (&full_filename);
    return (shell_command);
}


/*  --------------------------------------------------------------------------
 *  restore_redirection -- local
 *
 *  If the file handles for old_stdin, old_stdout, old_stderr, are zero or
 *  greater then duplicate those file handles over stdin, stdout, stderr
 *  respectively, and close the old file handles.  Each of the file handles
 *  is considered seperately.
 *  This function is used to restore the file handles after IO redirection.
 */

static void
restore_redirection (int old_stdin, int old_stdout, int old_stderr)
{
    file_fhrestore (old_stdin,  STDIN_FILENO);
    file_fhrestore (old_stdout, STDOUT_FILENO);
    file_fhrestore (old_stderr, STDERR_FILENO);
}


#if (defined (__VMS__))
/*  --------------------------------------------------------------------------
 *  translate_to_vms -- local
 *
 *  Translates POSIX style filename /top/path2/path2/filename into OpenVMS
 *  style filename top:[path1.path2]filename, which is always the same size.
 *  Does nothing if the filename is not valid, i.e. with at least a top,
 *  one path component, and a filename.
 */

static void
translate_to_vms (char *filename)
{
    char
        *path_start,
        *path_end;

    /*  Filename must start with '/'                                         */
    if (*filename != '/')
        return;

    /*  Find start and end of file path                                      */
    path_start = strchr  (filename + 1, '/');
    path_end   = strrchr (filename, '/');
    if (path_start == NULL || path_start == path_end)
        return;                         /*  Badly-formed filename            */

    path_start--;
    memmove (filename, filename + 1, path_start - filename);
    *path_start++ = ':';
    *path_start++ = '[';
    *path_end     = '\0';               /*  Cut string before filename       */
    strconvch (path_start, '/', '.');   /*    and replace slashes by dots    */
    *path_end     = ']';                /*  Finally, add ']' after path      */
}
#endif


