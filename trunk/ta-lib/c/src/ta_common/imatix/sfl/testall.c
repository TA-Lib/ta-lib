/*  ----------------------------------------------------------------<Prolog>-
    Name:       testall.c
    Title:      Test program for all SFL functions (optimistic version)
    Package:    Standard Function Library (SFL)

    Written:    1997/01/06  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/11/23

    Synopsis:   Runs tests on all SFL functions.  That's the plan, anyhow.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

void handle_signal (int the_signal)
{
    exit (EXIT_FAILURE);
}


/*  ------------------------------------------------------------------------ */

void do__resolve_path (char *data)
{
    char
        *new_path;

    new_path = resolve_path (data);
    printf ("   resolve_path (%s) --> \"%s\"\n", data, new_path);
    mem_free (new_path);

    new_path = locate_path ("\\root\\", data);
    printf ("   locate_path (\\root\\ + %s) --> \"%s\"\n", data, new_path);
    mem_free (new_path);
}


void test_sfldir (void)
{
    puts ("Testing SFLDIR package");
    do__resolve_path ("");
    do__resolve_path (".");
    do__resolve_path ("..");
    do__resolve_path ("...");
    do__resolve_path ("./");
    do__resolve_path ("../");
    do__resolve_path (".../");
    do__resolve_path ("/./");
    do__resolve_path ("/../");
    do__resolve_path ("/.../");
    do__resolve_path ("a/../");
    do__resolve_path ("ab/./");
    do__resolve_path ("ab/../");
    do__resolve_path ("ab/.../");
    do__resolve_path ("/a/../");
    do__resolve_path ("/ab/./");
    do__resolve_path ("/ab/../");
    do__resolve_path ("/ab/.../");
    do__resolve_path ("/.");
    do__resolve_path ("/..");
    do__resolve_path ("/...");
    do__resolve_path ("a/..");
    do__resolve_path ("ab/.");
    do__resolve_path ("ab/..");
    do__resolve_path ("ab/...");
    do__resolve_path ("/a/..");
    do__resolve_path ("/ab/.");
    do__resolve_path ("/ab/..");
    do__resolve_path ("/ab/...");
    do__resolve_path ("/./x");
    do__resolve_path ("/../x");
    do__resolve_path ("/.../x");
    do__resolve_path ("a/../x");
    do__resolve_path ("ab/./x");
    do__resolve_path ("ab/../x");
    do__resolve_path ("ab/.../x");
    do__resolve_path ("a/b/./x");
    do__resolve_path ("a/b/../x");
    do__resolve_path ("a/b/.../x");
    do__resolve_path ("a/b/c/./x");
    do__resolve_path ("a/b/c/../x");
    do__resolve_path ("a/b/c/.../x");
    do__resolve_path ("/a/../x");
    do__resolve_path ("/ab/./x");
    do__resolve_path ("/ab/../x");
    do__resolve_path ("/ab/.../x");
    do__resolve_path (".name./..is../...okay...");
    do__resolve_path ("root/././test/dir");
    do__resolve_path ("root/../../test/dir");
    do__resolve_path ("root/.../.../test/dir");
    do__resolve_path ("a/../x/..");
    do__resolve_path ("//a//../x/../");
}


/*  ------------------------------------------------------------------------ */

void do__file_is_executable (char *filename)
{
    printf ("%s executable? %d\n", filename, file_is_executable (filename));
    printf ("%s program?    %d\n", filename, file_is_program    (filename));
}

void do__file_where (char *filename)
{
    char
        *full_name;

    full_name = file_where ('r', "PATH", filename, NULL);
    if (full_name)
        printf ("find %s => %s\n", filename, full_name);
}


void test_sflfile (void)
{
    puts ("Testing SFLFILE package");
    do__file_is_executable ("sflfile.c");
    do__file_is_executable ("lr");
    do__file_is_executable ("testall");
#if defined (__WINDOWS__)
    do__file_is_executable ("testall.exe");
    do__file_where         ("testall.exe");
    do__file_where         ("..\\sfl\\testall.exe");
    do__file_where         ("../sfl/testall.exe");
#elif defined (__UNIX__)
    do__file_is_executable ("testall");
    do__file_where         ("testall");
    do__file_where         ("..\\sfl\\testall");
    do__file_where         ("../sfl/testall");
#endif
}


/*  ------------------------------------------------------------------------ */

void test_sfltok (void)
{
    char
        **table,
        *result;
    int
        wordnbr;
    SYMTAB
        *envtab;

    puts ("Testing SFLTOK package");

    table = tok_split (" word");
    puts ("String after breaking up:");
    for (wordnbr = 0; table [wordnbr]; wordnbr++)
        printf ("%d: %s\n", wordnbr, table [wordnbr]);
    printf ("Table size=%d textsize=%ld\n",
             tok_size (table), (long) tok_text_size (table));
    tok_free (table);

    table = tok_split ("This is a string ding-a-ling-a-ding");
    puts ("String after breaking up:");
    for (wordnbr = 0; table [wordnbr]; wordnbr++)
        printf ("%d: %s\n", wordnbr, table [wordnbr]);
    printf ("Table size=%d textsize=%ld\n",
             tok_size (table), (long) tok_text_size (table));

    table = tok_push (table, "Should come before");
    puts ("String after prefixing:");
    for (wordnbr = 0; table [wordnbr]; wordnbr++)
        printf ("%d: %s\n", wordnbr, table [wordnbr]);
    printf ("Table size=%d textsize=%ld\n",
             tok_size (table), (long) tok_text_size (table));
    tok_free (table);

    table = tok_split_rich ("This is a \"string !\" ding-a-ling-a-ding", "\"");
    puts ("String after breaking up:");
    for (wordnbr = 0; table [wordnbr]; wordnbr++)
        printf ("%d: %s\n", wordnbr, table [wordnbr]);
    printf ("Table size=%d textsize=%ld\n",
             tok_size (table), (long) tok_text_size (table));
    tok_free (table);

    envtab = env2symb ();
    result = tok_subst ("$(PATH)/$(TEMP)/$(NONE)", envtab);
    puts (result);
    mem_free (result);

    result = tok_subst ("Path=$(PATH)--temp=$(TEMP)--none=$(NONE)--", envtab);
    puts (result);
    mem_free (result);
    sym_delete_table (envtab);

    mem_assert ();
}


/*  ------------------------------------------------------------------------ */

void test_sflini (void)
{
    /*  Uses testall.ini as input                                            */
    SYMTAB
        *symtab = NULL;

    symtab = ini_dyn_load (NULL, "testall.ini");
    ini_dyn_save (symtab, "testall.new");
    sym_delete_table (symtab);
    mem_assert ();
}


/*  ------------------------------------------------------------------------ */

void test_soundex (char *value)
{
    printf ("Soundex of %s = %s\n", value, soundexn (value, 5, TRUE));
}

void test_sflstr (void)
{
    test_soundex ("fabrique");
    test_soundex ("ebri");
    test_soundex ("immo");
    test_soundex ("poelman");
    test_soundex ("wijngaert");
    test_soundex ("bvba");
}

/*  ------------------------------------------------------------------------ */

void test_sflenv (void)
{
    SYMTAB
        *table;
    SYMBOL
        *symbol;                        /*  Symbol in table                  */

    table = env2symb ();
    for (symbol = table-> symbols; symbol; symbol = symbol-> next)
        printf ("%s=%s\n", symbol-> name, symbol-> value);
}


/*  ------------------------------------------------------------------------ */

void test_sflhttp (void)
{

#if (defined (__UNIX__) || defined (__WINDOWS__))
#   define TEST_STRING    \
        "  aacute  \341 "   \
        "  acirc   \342 "   \
        "  aelig   \346 "   \
        "  AElig   \306 "   \
        "  agrave  \340 "   \
        "  aring   \345 "   \
        "  Aring   \305 "   \
        "  auml    \344 "   \
        "  Auml    \304 "   \
        "  ccedil  \347 "   \
        "  eacute  \351 "   \
        "  ecirc   \352 "   \
        "  egrave  \350 "   \
        "  euml    \353 "   \
        "  iacute  \355 "   \
        "  icirc   \356 "   \
        "  igrave  \354 "   \
        "  iuml    \357 "   \
        "  oacute  \363 "   \
        "  ocirc   \364 "   \
        "  ograve  \362 "   \
        "  ouml    \366 "   \
        "  Ouml    \326 "   \
        "  uacute  \372 "   \
        "  ucirc   \373 "   \
        "  ugrave  \371 "   \
        "  uuml    \374 "   \
        "  Uuml    \334 "   \
        "  yuml    \375 "
#elif (defined (__MSDOS__))
#   define TEST_STRING      \
        "  aacute  \240 "   \
        "  acirc   \203 "   \
        "  aelig   \221 "   \
        "  AElig   \222 "   \
        "  agrave  \205 "   \
        "  aring   \206 "   \
        "  Aring   \217 "   \
        "  auml    \204 "   \
        "  Auml    \216 "   \
        "  ccedil  \207 "   \
        "  eacute  \202 "   \
        "  ecirc   \210 "   \
        "  egrave  \212 "   \
        "  euml    \211 "   \
        "  iacute  \241 "   \
        "  icirc   \214 "   \
        "  igrave  \215 "   \
        "  iuml    \213 "   \
        "  oacute  \242 "   \
        "  ocirc   \223 "   \
        "  ograve  \225 "   \
        "  ouml    \224 "   \
        "  Ouml    \231 "   \
        "  uacute  \243 "   \
        "  ucirc   \226 "   \
        "  ugrave  \227 "   \
        "  uuml    \201 "   \
        "  Uuml    \232 "   \
        "  yuml    \230 "
#else
#   define TEST_STRING  "No test string defined"
#endif

    static char
        buffer [2001];
    char 
        *test_ptr;

    test_ptr = TEST_STRING;
    http_encode_meta (buffer, &test_ptr, 2000, TRUE);
    printf ("Full length: %s\n", buffer);

    test_ptr = TEST_STRING;
    http_encode_meta (buffer, &test_ptr, 100, TRUE);
    printf ("Truncated to 100: %s\n", buffer);

    test_ptr = TEST_STRING;
    http_encode_meta (buffer, &test_ptr, 10, TRUE);
    printf ("Truncated to 10: %s\n", buffer);

    test_ptr = TEST_STRING;
    http_encode_meta (buffer, &test_ptr, 1, TRUE);
    printf ("Truncated to 1: %s\n", buffer);

    test_ptr = TEST_STRING;
    http_encode_meta (buffer, &test_ptr, 0, TRUE);
    printf ("Truncated to 0: %s\n", buffer);
}


/*  ------------------------------------------------------------------------ */

void test_mask (char *address, char *mask, char *ok)
{
    printf ("Socket=%-10s mask=%-30s [%s]--> ", address, mask, ok);
    if (socket_is_permitted (address, mask))
        printf ("--\n");
    else
        printf ("XX\n");
}

void test_sflsock (void)
{
    test_mask ("127.0.0.1", "",                 "XX");
    test_mask ("127.0.0.1", "*",                "--");
    test_mask ("127.0.0.1", "127",              "XX");
    test_mask ("127.0.0.1", "127.*",            "--");
    test_mask ("127.0.0.1", "127.0.0.1",        "--");
    test_mask ("127.0.0.1", "128.*",            "XX");
    test_mask ("127.0.0.1", "128.*,",           "XX");
    test_mask ("127.0.0.1", "128.*,*",          "--");
    test_mask ("127.0.0.1", "128.*,127",        "XX");
    test_mask ("127.0.0.1", "128.*,127.*",      "--");
    test_mask ("127.0.0.1", "128.*,127.0.0.1",  "--");
    test_mask ("127.0.0.1", "!",                "--");
    test_mask ("127.0.0.1", "!127.0.0.1",       "XX");
    test_mask ("127.0.0.1", "!*",               "XX");
    test_mask ("127.0.0.1", "!127",             "--");
    test_mask ("127.0.0.1", "!127.*",           "XX");
    test_mask ("127.0.0.1", "!128.*",           "XX");
    test_mask ("127.0.0.1", "!128.*,!",         "--");
    test_mask ("127.0.0.1", "!128.*,!*",        "XX");
    test_mask ("127.0.0.1", "!128.*,!127",      "--");
    test_mask ("127.0.0.1", "!128.*,!127.*",    "XX");
    test_mask ("127.0.0.1", "127.0.0.123",      "XX");
    test_mask ("127.0.0.1", "127.0.0.123,*",    "--");
    test_mask ("127.0.0.1", "!127.0.0.123",     "XX");
    test_mask ("127.0.0.1", "!127.0.0.123,!*",  "XX");
}


void test_sfllbuf (void)
{
    LINEBUF
        *buffer;
    static byte
        line [100];
    char
        *ptr;
    DESCR
        descr = { 100, line };

    buffer = linebuf_create (4000);
    linebuf_append (buffer, "line 1 line 1 line 1 line 1 line 1 line 1 line 1");
    linebuf_append (buffer, "line 2 line 2 line 2 line 2 line 2 line 2 line 2");
    linebuf_append (buffer, "line 3 line 3 line 3 line 3 line 3 line 3 line 3");
    linebuf_append (buffer, "line 4 line 4 line 4 line 4 line 4 line 4 line 4");
    linebuf_append (buffer, "line 5 line 5 line 5 line 5 line 5 line 5 line 5");
    linebuf_append (buffer, "line 6 line 6 line 6 line 6 line 6 line 6 line 6");
    linebuf_append (buffer, "line 7 line 7 line 7 line 7 line 7 line 7 line 7");
    linebuf_append (buffer, "line 8 line 8 line 8 line 8 line 8 line 8 line 8");
    linebuf_append (buffer, "line 9 line 9 line 9 line 9 line 9 line 9 line 9");
    linebuf_append (buffer, "line A line A line A line A line A line A line A");
    linebuf_append (buffer, "line B line B line B line B line B line B line B");
    linebuf_append (buffer, "line C line C line C line C line C line C line C");
    ptr = linebuf_first (buffer, &descr);
    while (ptr)
      {
        printf ("=>%s\n", (char *) line);
        ptr = linebuf_next (buffer, &descr, ptr);
      }
    puts ("--");
    ptr = linebuf_last (buffer, &descr);
    while (ptr)
      {
        printf ("=>%s\n", (char *) line);
        ptr = linebuf_prev (buffer, &descr, ptr);
      }
    linebuf_destroy (buffer);
}


/*  ------------------------------------------------------------------------ */

int main (int argc, char *argv [])
{
    signal (SIGINT,  handle_signal);
    signal (SIGSEGV, handle_signal);
    signal (SIGTERM, handle_signal);

    if (argc == 1)
      {
        puts ("Syntax: testall option");
        puts ("  -dir            test functions in sfldir");
        puts ("  -file           test functions in sflfile");
        puts ("  -tok            test functions in sfltok");
        puts ("  -ini            test functions in sflini");
        puts ("  -str            test functions in sflstr");
        puts ("  -env            test functions in sflenv");
        puts ("  -http           test functions in sflhttp");
        puts ("  -sock           test functions in sflsock");
        puts ("  -all            test all functions");
        exit (1);
      }
    if (streq (argv [1], "-dir")  || streq (argv [1], "-all"))
        test_sfldir  ();
    if (streq (argv [1], "-file") || streq (argv [1], "-all"))
        test_sflfile ();
    if (streq (argv [1], "-tok") || streq (argv [1], "-all"))
        test_sfltok  ();
    if (streq (argv [1], "-ini") || streq (argv [1], "-all"))
        test_sflini  ();
    if (streq (argv [1], "-str")  || streq (argv [1], "-all"))
        test_sflstr  ();
    if (streq (argv [1], "-env")  || streq (argv [1], "-all"))
        test_sflenv  ();
    if (streq (argv [1], "-http") || streq (argv [1], "-all"))
        test_sflhttp ();
    if (streq (argv [1], "-sock") || streq (argv [1], "-all"))
        test_sflsock ();
    if (streq (argv [1], "-lbuf") || streq (argv [1], "-all"))
        test_sfllbuf ();

    return (EXIT_SUCCESS);
}
