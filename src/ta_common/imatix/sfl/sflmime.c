/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflmime.c
    Title:      MIME support functions
    Package:    Standard Function Library (SFL)

    Written:    1996/03/28  iMatix SFL project team <sfl@imatix.com>
    Revised:    2000/01/19

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sfldate.h"                    /*  Date and time functions          */
#include "sflmime.h"                    /*  Prototypes for functions         */
#include "sflprint.h"                   /*  snprintf functions               */


/*  Function prototypes                                                      */

static void init_conversion_tables (void);
static int  find_month             (char *month);


/*  Global variables used in this source file only                           */

static char
    *months [] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
static char
    *days [] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

static byte char_to_base64 [128];
static char base64_to_char [64];
static Bool tables_initialised = FALSE;


/*  ---------------------------------------------------------------------[<]-
    Function: encode_base64

    Synopsis: Encodes a source buffer in Base 64 and stores the result
    in the target buffer.  The target buffer must be at least 1/3rd longer
    than the amount of data in the source buffer.  The base64 data consists
    of portable printable characters as defined in RFC 1521.  Returns the
    number of bytes output into the target buffer.
    ---------------------------------------------------------------------[>]-*/

size_t
encode_base64 (const byte *source, byte *target, size_t source_size)
{
    size_t
        target_size = 0;                /*  Length of target buffer          */
    int
        nb_block;                       /*  Total number of blocks           */
    byte
        *p_source,                      /*  Pointer to source buffer         */
        *p_target,                      /*  Pointer to target buffer         */
        value;                          /*  Value of Base64 byte             */

    ASSERT (source);
    ASSERT (target);

    if (source_size == 0)
        return (0);

    if (!tables_initialised)
        init_conversion_tables ();

    /*    Bit positions
                  | byte 1 | byte 2 | byte 3 |
    source block   87654321 87654321 87654321         -> 3 bytes of 8 bits

                  | byte 1 | byte 2 | byte 3 | byte 4 |
    Encoded block  876543   218765   432187   654321  -> 4 bytes of 6 bits
    */

    nb_block = (int) (source_size / 3);

    /*  Check if we have a partially-filled block                            */
    if (nb_block * 3 != (int) source_size)
        nb_block++;
    target_size = (size_t) nb_block * 4;
    target [target_size] = '\0';

    p_source = (byte *) source;         /*  Point to start of buffers        */
    p_target = target;

    while (nb_block--)
      {
        /*  Byte 1                                                           */
        value       = *p_source >> 2;
        *p_target++ = base64_to_char [value];

        /*  Byte 2                                                           */
        value = (*p_source++ & 0x03) << 4;
        if ((size_t) (p_source - source) < source_size)
            value |= (*p_source & 0xF0) >> 4;
        *p_target++ = base64_to_char [value];

        /*  Byte 3 - pad the buffer with '=' if block not completed          */
        if ((size_t) (p_source - source) < source_size)
          {
            value = (*p_source++ & 0x0F) << 2;
            if ((size_t) (p_source - source) < source_size)
                value |= (*p_source & 0xC0) >> 6;
            *p_target++ = base64_to_char [value];
          }
        else
            *p_target++ = '=';

        /*  Byte 4 - pad the buffer with '=' if block not completed          */
        if ((size_t) (p_source - source) < source_size)
          {
            value       = *p_source++ & 0x3F;
            *p_target++ = base64_to_char [value];
          }
        else
            *p_target++ = '=';
     }
   return (target_size);
}


/*  ---------------------------------------------------------------------[<]-
    Function: decode_base64

    Synopsis: Decodes a block of Base 64 data and stores the resulting
    binary data in a target buffer.  The target buffer must be at least
    3/4 the size of the base 64 data.  Returns the number of characters
    output into the target buffer.
    ---------------------------------------------------------------------[>]-*/

size_t
decode_base64 (const byte *source, byte *target, size_t source_size)
{
    size_t
        target_size = 0;                /*  Length of target buffer          */
    int
        nb_block;                       /*  Total number of block            */
    byte
        value,                          /*  Value of Base64 byte             */
        *p_source,                      /*  Pointer in source buffer         */
        *p_target;                      /*  Pointer in target buffer         */

    ASSERT (source);
    ASSERT (target);

    if (source_size == 0)
        return (0);

    if (!tables_initialised)
        init_conversion_tables ();

    /*  Bit positions
                  | byte 1 | byte 2 | byte 3 | byte 4 |
    Encoded block  654321   654321   654321   654321  -> 4 bytes of 6 bits
                  | byte 1 | byte 2 | byte 3 |
    Decoded block  65432165 43216543 21654321         -> 3 bytes of 8 bits
    */

    nb_block    = source_size / 4;
    target_size = (size_t) nb_block * 3;
    target [target_size] = '\0';

    p_source = (byte *) source;         /*  Point to start of buffers        */
    p_target = target;

    while (nb_block--)
      {
        /*  Byte 1                                                           */
        *p_target    = char_to_base64 [(byte) *p_source++] << 2;
        value        = char_to_base64 [(byte) *p_source++];
        *p_target++ += ((value & 0x30) >> 4);

        /*  Byte 2                                                           */
        *p_target    = ((value & 0x0F) << 4);
        value        = char_to_base64 [(byte) *p_source++];
        *p_target++ += ((value & 0x3C) >> 2);

        /*  Byte 3                                                           */
        *p_target    = (value & 0x03) << 6;
        value        = char_to_base64 [(byte) *p_source++];
        *p_target++ += value;
      }
   return (target_size);
}


/*  -------------------------------------------------------------------------
    init_conversion_tables function -- internal
    initialise the tables conversion for BASE64 coding
    -----------------------------------------------------------------------*/

static void
init_conversion_tables (void)
{
    byte
        value,                          /*  Value to store in table          */
        offset,
        index;                          /*  Index in table                   */

    /*  Reset the tables                                                     */
    memset (char_to_base64, 0, sizeof (char_to_base64));
    memset (base64_to_char, 0, sizeof (base64_to_char));

    value  = 'A';
    offset = 0;

    for (index = 0; index < 62; index++)
      {
        if (index == 26)
          {
            value  = 'a';
            offset = 26;
          }
        else
        if (index == 52)
          {
            value  = '0';
            offset = 52;
          }
        base64_to_char [index] = value + index - offset;
        char_to_base64 [value + index - offset] = index;
      }
    base64_to_char [62]  = '+';
    base64_to_char [63]  = '/';
    char_to_base64 ['+'] = 62;
    char_to_base64 ['/'] = 63;

    tables_initialised = TRUE;
}


/*  ---------------------------------------------------------------------[<]-
    Function: decode_mime_time

    Synopsis: Takes a MIME date and time string in various formats and
    converts to a date and time (both long values).  Returns TRUE if it
    could convert the date and time okay, else returns FALSE.  Accepts
    these formats:
    <TABLE>
    Mon_Jan_12_12:05:01_1995            ctime format
    Monday,_12-Jan-95_12:05:01_GMT      RFC 850
    Monday,_12-Jan-1995_12:05:01_GMT    RFC 850 iMatix extension
    Mon,_12_Jan_1995_12:05:01_GMT       RFC 1123
    </TABLE>
    The returned date and time are in local time, not GMT.
    ---------------------------------------------------------------------[>]-*/

Bool
decode_mime_time (const char *mime_string, long *date, long *time)
{
    int
        cent  = 0,
        year  = 0,
        month = 0,
        day   = 0,
        hour  = 0,
        min   = 0,
        sec   = 0;
    char
        month_name [20],
        buffer     [50],
        *p_char;

    ASSERT (mime_string);
    ASSERT (date);
    ASSERT (time);

    /*  Whatever format we're looking at, it will start with weekday.        */
    /*  Skip to first space.                                                 */
    if (!(p_char = strchr (mime_string, ' ')))
        return FALSE;
    else
        while (isspace (*p_char))
            ++p_char;

    if (isalpha (*p_char))
        /*  ctime                                                            */
        sscanf (p_char, "%s %d %d:%d:%d %d",
                month_name, &day, &hour, &min, &sec, &year);
    else
    if (p_char [2] == '-')
      {
        /*  RFC 850                                                          */
        sscanf (p_char, "%s %d:%d:%d",
                buffer, &hour, &min, &sec);
        buffer [2] = '\0';
        day        = atoi (buffer);
        buffer [6] = '\0';
        strcpy (month_name, &buffer [3]);
        year = atoi (&buffer [7]);

        /*  Use windowing at 1970 if century is missing                      */
        if (year < 70)
            cent = 20;
        else
            cent = 19;
      }
    else
        /*  RFC 1123                                                         */
        sscanf (p_char, "%d %s %d %d:%d:%d",
                &day, month_name, &year, &hour, &min, &sec);

    if (year > 100)
      {
        cent = (int) year / 100;
        year -= cent * 100;
      }
    month = find_month (month_name);
    *date = MAKE_DATE (cent, year, month, day);
    *time = MAKE_TIME (hour, min,  sec,   0  );

    gmt_to_local (*date, *time, date, time);
    return (TRUE);
}


/*  -------------------------------------------------------------------------
    find_month function -- internal
    Converts a 3-letter month into a value 0 to 11, or -1 if the month
    name is not valid.
    -----------------------------------------------------------------------*/

static int
find_month (char *month)
{
    int
        index;

    for (index = 0; index < 12; index++)
        if (!strcmp (months [index], month))
            return (index + 1);

    return (-1);
}


/*  ---------------------------------------------------------------------[<]-
    Function: encode_mime_time

    Synopsis: Encode date and time (in long format) in Mime RFC1123 date
    format, e.g. Mon, 12 Jan 1995 12:05:01 GMT.  The supplied date and time
    are in local time.  Returns the date/time string if the date was legal,
    else returns "?".  Returned string is in a static buffer.
    ---------------------------------------------------------------------[>]-*/

char *
encode_mime_time (long date, long time)
{
    int
        day_week,                       /*  Day of week number (0 is sunday) */
        month;                          /*  Month number                     */
    static char
        buffer [50];

    local_to_gmt (date, time, &date, &time);
    day_week = day_of_week (date);
    month    = GET_MONTH   (date);
    if (day_week >= 0 && day_week < 7 && month > 0 && month < 13)
      {
        snprintf (buffer, sizeof (buffer), 
                          "%s, %02d %s %04d %02d:%02d:%02d GMT",
                          days       [day_week],
                          GET_DAY    (date),
                          months     [month - 1],
                          GET_CCYEAR (date),
                          GET_HOUR   (time),
                          GET_MINUTE (time),
                          GET_SECOND (time)
                 );
        return (buffer);
      }
    else
        return ("?");
}
