/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflxmll.c
    Title:      XML serialisation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/08  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/02/02

    Synopsis:   Loads XML file into memory, to/from disk files.

    Copyright:  Copyright (c) 1991-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal include file           */
#include "sfldate.h"                    /*  Date and time functions          */
#include "sflfile.h"                    /*  File access functions            */
#include "sfllist.h"                    /*  List access functions            */
#include "sflmem.h"                     /*  Memory allocation functions      */
#include "sflstr.h"                     /*  String access functions          */
#include "sflxml.h"                     /*  XML definitions                  */
#include "sflsymb.h"
#include "sflhttp.h"                    /*  Meta-char encoding/decoding      */
#include "sflxmll.h"                    /*  Include prototype data           */
#include "sflxmll.d"                    /*  Include dialog data              */

/*  Function prototypes                                                      */

static int       xml_save_file_item      (FILE *xmlfile, XML_ITEM *item,
                                          int generation);
static void      xml_save_string_item    (char *xml_string, XML_ITEM *item);
static size_t    xml_item_size           (XML_ITEM *item);
static void      init_charmaps           (void);
static void      build_charmap           (byte flag, char *chars);
static int       xml_start_dialog        (void);
static void      expect_token            (char expect);
static char      get_next_char           (void);
static char      get_next_non_white_char (void);
static int       collect_name            (void);
static char     *collect_literal         (char terminator);
static void      error_exception         (char *format, ...);


/*- Global variables used in this source file only --------------------------*/

static int
    feedback,                           /*  Feedback for calling program     */
    char_nbr,                           /*  Current read position in line    */
    line_nbr,                           /*  Input line nbr from file         */
    generation;                         /*  How many levels of children      */

static const char
    *ppath,                             /*  XML file path as specified       */
    *pname;                             /*  XML file name as specified       */

static char
    *fname,                             /*  Full file name of XML file       */
    *literal,                           /*  String of any length             */
    *xmltext,                           /*  Pointer to XML string            */
    *xmlline,                           /*  Current line w/space for EOL     */
    token         [LINE_MAX + 1],       /*  Token from input stream          */
    name          [LINE_MAX + 1],       /*  Saved name                       */
    error_message [LINE_MAX + 1];       /*  Saved name                       */

static FILE
    *xmlfile;

static XML_ITEM
    *item,                              /*  Current XML item                 */
    *top_item,                          /*  Current top-level XML item       */
    *root;                              /*  Root XML item                    */

static Bool
    extended;                           /*  Extended or normal load?         */


/*  Character classification tables and macros                               */

static byte
    cmap [256];                         /*  Character classification tables  */

#define CMAP_NAME        1              /*  Normal name character            */
#define CMAP_NAME_OPEN   2              /*  Valid character to start name    */
#define CMAP_QUOTE       4              /*  Possible string delimiters       */
#define CMAP_PRINTABLE   8              /*  Valid characters in literal      */
#define CMAP_DECIMAL    16              /*  Decimal digits                   */
#define CMAP_HEX        32              /*  Hexadecimal digits               */

                                        /*  Macros for character mapping:    */
#define is_name(ch)      (cmap [(byte) (ch)] & CMAP_NAME)
#define is_name_open(ch) (cmap [(byte) (ch)] & CMAP_NAME_OPEN)
#define is_quote(ch)     (cmap [(byte) (ch)] & CMAP_QUOTE)
#define is_printable(ch) (cmap [(byte) (ch)] & CMAP_PRINTABLE)
#define is_decimal(ch)   (cmap [(byte) (ch)] & CMAP_DECIMAL)
#define is_hex(ch)       (cmap [(byte) (ch)] & CMAP_HEX)


/*  ---------------------------------------------------------------------[<]-
    Function: xml_save_file

    Synopsis: Saves an XML tree to the specified file.  Returns XML_NOERROR
    or XML_FILEERROR.
    ---------------------------------------------------------------------[>]-*/

int
xml_save_file (
    XML_ITEM   *item,
    const char *filename)
{
    FILE
        *xmlfile;                       /*  XML output stream                */
    int
        count;                          /*  How many symbols did we save?    */

    ASSERT (item);
    ASSERT (filename);
    init_charmaps ();                   /*  Initialise character maps        */

    if ((xmlfile = file_open (filename, 'w')) == NULL)
        return XML_FILEERROR;           /*  No permission to write file      */

    /*  Write XML file header                                                */
    fprintf (xmlfile, "<?xml version=\"1.0\"?>\n");

    /*  Output XML root                                                      */
    count = xml_save_file_item (xmlfile, item, 0);

    /*  Output a final carriage return  */
    fprintf (xmlfile, "\n");

    file_close (xmlfile);
    return XML_NOERROR;
}


/*  -------------------------------------------------------------------------
 *  init_charmaps
 *
 *  Initialise character map bit tables.  These are used to speed-up
 *  token recognition and collection.
 */

static void
init_charmaps (void)
{
    memset (cmap, 0, sizeof (cmap));    /*  Clear all bitmaps                */

    /*  Name     ::= (Letter | '_' | ':') (NameChar)*                        */
    /*  NameChar ::= Letter | Digit | MiscName                               */

    /*  Map fixed character sets to various bitmaps                          */
    build_charmap (CMAP_NAME, "abcdefghijklmnopqrstuvwxyz");
    build_charmap (CMAP_NAME, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    build_charmap (CMAP_NAME, "0123456789");
    build_charmap (CMAP_NAME, "_:.-");

    build_charmap (CMAP_NAME_OPEN, "abcdefghijklmnopqrstuvwxyz");
    build_charmap (CMAP_NAME_OPEN, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    build_charmap (CMAP_NAME_OPEN, "_:");

    build_charmap (CMAP_QUOTE, "\"'");

    /*  Printable characters.  ???                                           */
    build_charmap (CMAP_PRINTABLE, "abcdefghijklmnopqrstuvwxyz");
    build_charmap (CMAP_PRINTABLE, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    build_charmap (CMAP_PRINTABLE, "0123456789");
    build_charmap (CMAP_PRINTABLE, "!@#$%^&*()-_=+[]{}\\|;:'\"<>,./?`~ ");

    build_charmap (CMAP_DECIMAL, "0123456789");

    build_charmap (CMAP_HEX, "abcdefghijklmnopqrstuvwxyz");
    build_charmap (CMAP_HEX, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    build_charmap (CMAP_HEX, "0123456789");
}


/*  -------------------------------------------------------------------------
 *  build_charmap
 *
 *  Encode character string and flag into character map table.  Flag should
 *  be a 1-bit value from 1 to 128 (character map is 8 bits wide).
 */

static void
build_charmap (byte flag, char *string)
{
    for (; *string; string++)
        cmap [(byte) *string] |= flag;
}


static int
xml_save_file_item (FILE *xmlfile, XML_ITEM *item, int generation)
{
    int
        count = 1;                      /*  Count 1 for current item         */
    XML_ITEM
        *child,
        *sibling;
    XML_ATTR
        *attr;
    char
        *item_name,
        *attr_name,
        *ptr;
    Bool
        pretty;

    /*  First output item name and attributes                                */
    item_name  = xml_item_name  (item);
    if (item_name)
      {
        fprintf (xmlfile, "<%s", item_name);
        FORATTRIBUTES (attr, item)
          {
            attr_name  = xml_attr_name  (attr);
            ptr        = xml_attr_value (attr);
            http_encode_meta (token, &ptr, LINE_MAX, FALSE);
            fprintf (xmlfile, "\n%*s%s = \"%s", (generation + 1) * 4, "",
                                              attr_name, token);
            while (*ptr)
              {
                http_encode_meta (token, &ptr, LINE_MAX, FALSE);
                fprintf (xmlfile, "\n%s", token);
              }
            fprintf (xmlfile, "\"");
          }

        /*  If value or children exist, use long form, otherwise short form  */
        if ((child = xml_first_child (item)))
          {
            fprintf (xmlfile, ">");

            pretty = TRUE;
            for (sibling = child ;
                 sibling != NULL ;
                 sibling = xml_next_sibling (sibling))
                if (! xml_item_name (sibling))
                    pretty = FALSE;
                else
                    break;

            for ( ; child != NULL; child  = xml_next_sibling (child))
              {
                if (pretty)
                    fprintf (xmlfile, "\n%*s", (generation + 1) * 4, "");

                count += xml_save_file_item (xmlfile, child,
                                             generation + 1);

                if (xml_item_name (child))
                  {
                    pretty = TRUE;
                    for (sibling = xml_next_sibling (child) ;
                         sibling != NULL ;
                         sibling = xml_next_sibling (sibling))
                        if (! xml_item_name (sibling))
                            pretty = FALSE;
                        else
                            break;
                  }
              }
            
            if (pretty)
                fprintf (xmlfile, "\n%*s", generation * 4, "");

            fprintf (xmlfile, "</%s>", item_name);
          }
        else
            fprintf (xmlfile, "/>");
      }
    else            /*  Not name => this is a value node  */
      {
        ptr = xml_item_value (item);
        if (ptr)
            while (*ptr)
              {
                http_encode_meta (token, &ptr, LINE_MAX, FALSE);
                fprintf (xmlfile, "%s", token);
              }
      }
    return (count);
}


/*  ---------------------------------------------------------------------[<]-
    Function: xml_save_string

    Synopsis: Saves an XML tree to a string.  Returns a freshly-allocated
    string containing the XML tree, or NULL if there was insufficient memory
    to allocate a new string.
    ---------------------------------------------------------------------[>]-*/

char *
xml_save_string (
    XML_ITEM *item)
{
    size_t
        xml_size;
    char
        *xml_string;
        
    ASSERT (item);
    init_charmaps ();                   /*  Initialise character maps        */
    xml_size = xml_item_size (item);
    xml_string = mem_alloc (xml_size + 1000);
    if (xml_string)
        xml_save_string_item (xml_string, item);

    return (xml_string);
}


/*  Return string size of XML item and all its children                      */

static size_t
xml_item_size (XML_ITEM *item)
{
    size_t
        item_size = 0;
    XML_ITEM
        *child = NULL;
    XML_ATTR
        *attr;
    char
        *item_name,
        *attr_name,
        *ptr;

    /*  First output item name and attributes                                */
    item_name = xml_item_name (item);
    if (item_name)
      {
        /*  Count '<name'                                                    */
        item_size += strlen (item_name) + 1;
        FORATTRIBUTES (attr, item)
          {
            attr_name  = xml_attr_name  (attr);
            ptr        = xml_attr_value (attr);
            /*  Count ' name="value"'                                        */
            item_size += strlen (attr_name) + 4;
            while (*ptr)
              {
                http_encode_meta (token, &ptr, LINE_MAX, FALSE);
                item_size += strlen (token);
              }
          }
        /*  If value or children exist, use long form, otherwise short form  */
        if ((child = xml_first_child (item)))
          {
            /*  Count '>' and '</name>'                                      */
            item_size += strlen (name) + 4;
            for (child  = xml_first_child (item);
                 child != NULL;
                 child  = xml_next_sibling (child))
                item_size += xml_item_size (child);
          }
        else
            /*  Count '/>'                                                   */
            item_size += 2;
      }
    else            /*  No name => this is a value node                      */
      {
        ptr = xml_item_value (item);
        while (*ptr)
          {
            http_encode_meta (token, &ptr, LINE_MAX, FALSE);
            item_size += strlen (token);
          }
      }
    return (item_size);
}


static void
xml_save_string_item (char *xml_string, XML_ITEM *item)
{
    XML_ITEM
        *child = NULL;
    XML_ATTR
        *attr;
    char
        *item_name,
        *attr_name,
        *ptr;

    /*  First output item name and attributes                                */
    xml_string [0] = 0;
    item_name = xml_item_name (item);
    if (item_name)
      {
        xstrcat (xml_string, "<", item_name, NULL);
        FORATTRIBUTES (attr, item)
          {
            attr_name  = xml_attr_name  (attr);
            ptr        = xml_attr_value (attr);
            xstrcat (xml_string, " ", attr_name, "=\"", NULL);
            while (*ptr)
              {
                http_encode_meta (token, &ptr, LINE_MAX, FALSE);
                strcat (xml_string, token);
              }
            strcat (xml_string, "\"");
          }

        /*  If value or children exist, use long form, otherwise short form  */
        if ((child = xml_first_child (item)))
          {
            strcat (xml_string, ">");
            for (child  = xml_first_child (item);
                 child != NULL;
                 child  = xml_next_sibling (child))
              {
                xml_string += strlen (xml_string);
                xml_save_string_item (xml_string, child);
              }
            xstrcat (xml_string, "</", item_name, ">", NULL);
          }
        else
            strcat (xml_string, "/>");
      }
    else            /*  No name => this is a value node                      */
      {
        ptr = xml_item_value (item);
        while (*ptr)
          {
            xml_string += strlen (xml_string);
            http_encode_meta (token, &ptr, LINE_MAX, FALSE);
            strcat (xml_string, token);
          }
      }
}


/*  ---------------------------------------------------------------------[<]-
    Function: xml_error

    Synopsis: Returns the last error message generated by xml_load.
    ---------------------------------------------------------------------[>]-*/

char *
xml_error (void)
{
    return (error_message);
}


/*  ---------------------------------------------------------------------[<]-
    Function: xml_seems_to_be

    Synopsis: Reads the first line of a file.  Returns XML_NOERROR if file
    is found and has a valid XML header, XML_FILEERROR if the file could not
    be opened and XML_LOADERROR if the first line is not an XML header.
    ---------------------------------------------------------------------[>]-*/

int
xml_seems_to_be (const char *path,
                 const char *filename)
{
    ASSERT (filename);
    pname = filename;
    ppath = path;

    /*  Current line w/space for EOL                                         */
    xmlline = mem_alloc (LINE_MAX + 2);

    the_next_event = looks_like_xml_event;
    xml_start_dialog ();
    mem_free (xmlline);
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: xml_load_file

    Synopsis: Loads the contents of an XML file into a given XML tree,
    or a new XML tree if none was specified.  The XML data is not checked
    against a DTD.  Returns one of XML_NOERROR, XML_FILEERROR,
    XML_LOADERROR.  If the parameter `allow_extended' is TRUE then the file
    may contain more than one XML item.  The following attributes are
    defined in the root item of the XML tree.
    <TABLE>
    filename        Name of the XML input file
    filetime        Modification time of the file, "HHMMSSCC"
    filedate        Modification date of input file, "YYYYMMDD"
    </TABLE>
    Looks for the XML file on the specified path symbol, or in the current
    directory if the path argument is null.  Adds the extension ".xml"
    to the file name if there is none already included.
    ---------------------------------------------------------------------[>]-*/

int
xml_load_file (XML_ITEM  **item,
               const char *path,
               const char *filename,
               Bool allow_extended)
{
    ASSERT (filename);
    pname = filename;
    ppath = path;

    extended = allow_extended;

    if (! *item)
      {
        *item = xml_new (NULL, "root", "");
        ASSERT (*item);
      }
    root = *item;

    /*  Current line w/space for EOL                                         */
    xmlline = mem_alloc (LINE_MAX + 2);

    line_nbr = 0;

    the_next_event = file_event;

    xml_start_dialog ();
    mem_free (xmlline);
    return (feedback);
}


/*  ---------------------------------------------------------------------[<]-
    Function: xml_load_string

    Synopsis: Loads an XML string into a new XML tree.  The XML data is not
    checked against a DTD.  See xml_load() for details.
    ---------------------------------------------------------------------[>]-*/

int
xml_load_string (XML_ITEM  **item,
                 const char *xmlstring,
                 Bool allow_extended)
{
    ASSERT (xmlstring);
    pname = NULL;
    ppath = NULL;

    xmltext  = (char *) xmlstring;
    xmlline  = xmltext;

    extended = allow_extended;

    if (! *item)
      {
        *item = xml_new (NULL, "root", "");
        ASSERT (*item);
      }
    root = *item;
    
    char_nbr = 0;                       /*  Use string as input line         */
    line_nbr = 1;

    the_next_event = string_event;

    xml_start_dialog ();
    return (feedback);
}


int
xml_start_dialog (void)
{
    feedback = XML_NOERROR;             /*  Reset return feedback            */
#   include "sflxmll.i"                 /*  Include dialog interpreter       */
}


/*************************   INITIALISE THE PROGRAM   ************************/

MODULE initialise_the_program (void)
{
    xmlfile    = NULL;                  /*  No files open yet                */
    generation = 0;                     /*  No items yet                     */
    top_item   = NULL;

    init_charmaps ();                   /*  Initialise character maps        */
}


/*****************************   OPEN XML FILE   *****************************/

MODULE open_xml_file (void)
{
    fname = file_where ('r', ppath, pname, "xml");
    if (!fname)
      {
        error_exception ("Could not find XML file: %s", pname);
        feedback = XML_FILEERROR;
        return;
      }
    if ((xmlfile = file_open (fname, 'r')) == NULL)
      {
        error_exception ("Could not open XML file: %s", pname);
        feedback = XML_FILEERROR;
        return;
      }

    char_nbr    = 0;                    /*  Clear input line                 */
    xmlline [0] = 0;
}


/**************************   INITIALISE XML TREE   **************************/

MODULE initialise_xml_tree (void)
{
    static char
        buffer [60];

    if (pname && (! extended))
      {
        xml_put_attr (root, "filename", fname);
        sprintf (buffer, "%ld", timer_to_date (get_file_time (fname)));
        xml_put_attr (root, "filedate", buffer);
        sprintf (buffer, "%ld", timer_to_time (get_file_time (fname)));
        xml_put_attr (root, "filetime", buffer);
      }
    item = root;
}


/***************************   GET CONTENT TOKEN   ***************************/

MODULE get_content_token (void)
{
    char thisch;                        /*  Next char in token               */

    thisch = get_next_char ();
    if (thisch == '<')
      {
        if (xmlline [char_nbr] == '?')
          {
            char_nbr ++;
            the_next_event = processing_event;
          }
        else
        if (xmlline [char_nbr] == '/')
          {
            char_nbr ++;
            the_next_event = close_event;
          }
        else
        if (xmlline [char_nbr] == '!')
            if ((xmlline [char_nbr + 1] == '-')
            &&  (xmlline [char_nbr + 2] == '-'))
              {
                char_nbr += 3;
                the_next_event = comment_event;
              }
            else
              {
                char_nbr ++;
                the_next_event = ignore_event;
              }
        else
            the_next_event = open_event;
      }
    else
    if (thisch == '\0')
        the_next_event = end_of_file_event;
    else
    if (strchr (" \t\r\n", thisch))
        the_next_event = space_event;
    else
        the_next_event = char_event;
}


static char
get_next_char (void)
{
    if (xmlfile)
        while (xmlline [char_nbr] == 0)
          {
            char_nbr = 0;
            if (file_read (xmlfile, xmlline))
              {
                strcat (xmlline, "\n");
                line_nbr++;
              }
            else
              {
                xmlline [0] = '\0';
                the_next_event = end_of_file_event;
                return 0;
              }
          }
    else
      {
        if ((char_nbr > 0)
        &&  (xmlline [char_nbr - 1] == '\n'))
          {
            line_nbr++;
            xmlline += char_nbr;
            char_nbr = 0;
          }
        if (xmlline [char_nbr] == 0)
          {
            the_next_event = end_of_file_event;
            return 0;
          }
      }
    return (xmlline [char_nbr++]);
}


/*****************************   GET TAG TOKEN   *****************************/

MODULE get_tag_token (void)
{
    char thisch;                        /*  Next char in token               */

    thisch = get_next_non_white_char ();

    if (!thisch)
        return;

    if (thisch == '/')
        the_next_event = slash_event;
    else
    if (thisch == '>')
        the_next_event = close_event;
    else
    if is_name_open (thisch)
      {
        name [0] = thisch;
        collect_name ();
        strcpy (name + 1, token);
        the_next_event = name_event;
      }
    else
    if ((thisch == '<')
    &&  (xmlline [char_nbr] == '!')
    &&  (xmlline [char_nbr + 1] == '-')
    &&  (xmlline [char_nbr + 2] == '-'))
      {
        char_nbr += 3;
        the_next_event = comment_event;
      }
    else
        error_exception ("Unrecognised token: %c", thisch);
}


static char
get_next_non_white_char (void)
{
    char thisch;                        /*  Next char in token               */

    thisch = get_next_char ();
    while (strchr (" \t\r\n", thisch) && thisch)
        thisch = get_next_char ();

    return thisch;
}


static int
collect_name (void)
{
    int  size;                          /*  Size of token                    */
    char thisch;                        /*  Next char                        */

    /*  Name ::= (Letter | '_' | ':') (NameChar)*                            */

    size = 0;
    while (is_name (thisch = get_next_char ()))
        token [size++] = thisch;

    char_nbr--;

    token [size] = 0;                   /*  Terminate token string           */
    return size;
}


/***************************   COLLECT ITEM VALUE   **************************/

MODULE collect_item_value (void)
{
    char
        *value;

    char_nbr --;                        /*  Back up to starting char         */
    value = collect_literal ('<');
    char_nbr --;                        /*  Back up to terminating char      */

    xml_new (item, NULL, value);

    mem_free (value);
}


static char *
collect_literal (char terminator)
{
    char
        thisch,                         /*  Next character                   */
        *literal = NULL,                /*  The result                       */
        *bufptr;                        /*  Pointer to input buffer          */
    int
        length = 0,                     /*  Total length of literal          */
        chunk = 0,                      /*  Length of line in literal        */
        snippet;                        /*  Length of decoded line           */
    int
        start_line_nbr;                 /*  Where literal started            */

    thisch = get_next_char ();
    start_line_nbr = line_nbr;

    FOREVER
      {
        while (thisch && thisch != terminator && chunk < LINE_MAX)
          {
            token [chunk++] = thisch;
            thisch = get_next_char ();
          }
        if (!thisch)
          {
            line_nbr = start_line_nbr;
            error_exception ("File end inside literal.");
            mem_free (literal);
            return NULL;
          }

        token [chunk] = 0;
    
        bufptr = token;

        snippet = http_decode_meta (token, &bufptr, LINE_MAX + 1);
        if (literal)
            literal = mem_realloc (literal, length + snippet + 1);
        else
            literal = mem_alloc   (         length + snippet + 1);

        ASSERT (literal);
        memcpy (&literal [length], token, snippet);

        length += snippet;
        literal [length] = '\0';

        /*  If we zapped the & in http_decode_meta(), put it back            */
        if (bufptr - token < chunk)
            *bufptr = '&';

        strcpy (token, bufptr);
        chunk = strlen (token);
        if (thisch == terminator)
            return literal;
      }
}


/******************************   SKIP SPACES   ******************************/

MODULE skip_spaces (void)
{
    char thisch;                        /*  Next char in token               */

    thisch = get_next_char ();
    while (strchr (" \t\r\n", thisch) && thisch)
        thisch = get_next_char ();

    if (thisch)
        char_nbr--;
}


/****************************   ATTACH NEW ITEM   ****************************/

MODULE attach_new_item (void)
{
    item = xml_new (item, name, NULL);
    ASSERT (item);

    generation++;
}


/**************************   NOTE TOP LEVEL ITEM   **************************/

MODULE note_top_level_item (void)
{
    top_item = item;
}


/******************************   EXPECT NAME   ******************************/

MODULE expect_name (void)
{
    if (get_next_non_white_char ())
      {
        char_nbr--;
        if (collect_name ())
            strcpy (name, token);

        return;
      }
    error_exception ("Did not find a name.");
}


/***************************   CONFIRM ITEM NAME   ***************************/

MODULE confirm_item_name (void)
{
    if (strneq (name, xml_item_name (item)))
        error_exception ("Incorrect closing tag name: %s", name);
}


/********************   REMOVE VALUES IF ALL WHITE SPACE   *******************/

MODULE remove_values_if_all_white_space (void)
{
    XML_ITEM
        *child,
        *trash;
    char
        *value;

    for (child  = xml_first_child (item); child != NULL; )
      {
         if (!xml_item_name (child))
           {
             value = xml_item_value (child);
             while (*value)
               if (! isspace (*value))
                   break;
               else
                   value++;

             if (! *value)              /*  End of string with all spaces    */
               {
                 trash = child;
                 child  = xml_next_sibling (child);
                 xml_free (trash);
                 continue;
               }
           }
         child  = xml_next_sibling (child);
      }
}


/*****************************   CLOSE THE ITEM   ****************************/

MODULE close_the_item (void)
{
    item = xml_parent (item);
    if (!item)
        error_exception ("Incorrect closing tag name: %s", name);

    generation--;
}


/******************************   EXPECT CLOSE   *****************************/

MODULE expect_close (void)
{
    expect_token ('>');
}


static void
expect_token (char expect)
{
    char thisch;                        /*  Next char in token               */

    thisch = get_next_non_white_char ();
    if (thisch != expect)
        error_exception ("Unexpected token: %c", thisch);
}


/************************   SKIP REST OF PROCESSING   ************************/

MODULE skip_rest_of_processing (void)
{
    char
        thisch;                         /*  Next character                   */
    int
        start_line_nbr;                 /*  Where tag started                */

    start_line_nbr = line_nbr;
    FOREVER
      {
        thisch = get_next_non_white_char ();
        if (thisch == 0)
          {
            line_nbr = start_line_nbr;
            error_exception ("File end inside tag.");
            return;
          }

        if ((            thisch == '?')
        &&  (xmlline [char_nbr] == '>'))
          {
            char_nbr++;
            return;
          }
      }
}


/**************************   SKIP REST OF COMMENT   *************************/

MODULE skip_rest_of_comment (void)
{
    char
        thisch;                         /*  Next character                   */
    int
        start_line_nbr;                 /*  Where comment started            */

    start_line_nbr = line_nbr;
    FOREVER
      {
        thisch = get_next_non_white_char ();
        if (thisch == 0)
          {
            line_nbr = start_line_nbr;
            error_exception ("File end inside comment.");
            return;
          }

        if ((              thisch == '-')
        &&  (xmlline [char_nbr]   == '-')
        &&  (xmlline [char_nbr+1] == '>'))
          {
            char_nbr+= 2;
            return;
          }
      }
}


/**************************   SKIP REST OF SECTION   *************************/

MODULE skip_rest_of_section (void)
{
    char
        thisch;                         /*  Next character                   */
    int
        start_line_nbr;                 /*  Where tag started                */

    start_line_nbr = line_nbr;
    FOREVER
      {
        thisch = get_next_non_white_char ();
        if (thisch == 0)
          {
            line_nbr = start_line_nbr;
            error_exception ("File end inside tag.");
            return;
          }

        if (thisch == '>')
            return;
      }
}



/**************************   CHECK FOR OPEN ITEM   **************************/

MODULE check_for_open_item (void)
{
    if (generation > 0)
        the_next_event = ok_event;
    else
        if (extended)
            the_next_event = optional_items_event;
        else
            the_next_event = no_more_items_event;
}


/***************************   EXPECT END OF FILE   **************************/

MODULE expect_end_of_file (void)
{
    if (get_next_non_white_char ())
        error_exception ("End of file expected.");
}


/*****************************   REPORT NO XML   *****************************/

MODULE report_no_xml (void)
{
    error_exception ("File contains no XML items.");
}


/*****************************   CLOSE XML FILE   ****************************/

MODULE close_xml_file (void)
{
    if (xmlfile)
        if (file_close (xmlfile))
            error_exception ("Error closing file.");
}


/**************************   EXPECT EQUALS TOKEN   **************************/

MODULE expect_equals_token (void)
{
    expect_token ('=');
}


/*****************************   EXPECT LITERAL   ****************************/

MODULE expect_literal (void)
{
    char
        thisch;
 
    thisch = get_next_non_white_char ();
    if (thisch == 0)                    /* End-of-file was reached           */
        return;

    if (!is_quote (thisch))
      {
        error_exception ("Invalid literal opening quote: %c",
                         thisch);
        return;
      }

    literal = collect_literal (thisch);
}


/**************************   ATTACH NEW ATTRIBUTE   *************************/

MODULE attach_new_attribute (void)
{
    if (xml_put_attr (item, name, literal) != 1)
        error_exception ("Duplicate attribute name: %s", name);

    mem_strfree (&literal);
}


/*************************   FREE PARTIAL XML TREE   *************************/

MODULE free_partial_xml_tree (void)
{
    if (top_item)
        xml_free (top_item);
}


/*************************   RETURN ERROR FEEDBACK   *************************/

MODULE return_error_feedback (void)
{
    feedback = XML_LOADERROR;
}


/*************************   UNEXPECTED TOKEN ERROR   ************************/

MODULE unexpected_token_error (void)
{
    error_exception ("Unexpected character: %c", xmlline [char_nbr - 1]);
}


/***************************   GET EXTERNAL EVENT   **************************/

MODULE get_external_event (void)
{
}


/*************************   TERMINATE THE PROGRAM    ************************/

MODULE terminate_the_program (void)
{
    the_next_event = terminate_event;
}


/*****************************************************************************/

local
error_exception (char *format, ...)
{
    char
        *ch,
        *mess_ptr;
    int
        offset;
    va_list
        argptr;

    mess_ptr = error_message;
    if (line_nbr > 0)
      {
        ch = strchr (xmlline, '\n');
        if (ch)
            *ch = '\0';
        if (pname)
          {
            offset = sprintf (mess_ptr, "%s ", pname);
            mess_ptr += offset;
          }
        offset = sprintf (mess_ptr, "%d: %s\n", line_nbr, xmlline);
        mess_ptr += offset;
        if (ch)
            *ch = '\n';
      }

    va_start (argptr, format);          /*  Start variable arguments list    */
    offset = vsprintf (mess_ptr, format, argptr);
    va_end   (argptr);                  /*  End variable arguments list      */
    mess_ptr += offset;

    sprintf (mess_ptr, "\n");
    raise_exception (error_event);
}
