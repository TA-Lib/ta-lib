/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflexdr.c
    Title:      External data representation functions
    Package:    Standard Function Library (SFL)

    Written:    1996/06/25  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/05/03

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflstr.h"                     /*  String functions                 */
#include "sfllist.h"                    /*  Linked-list functions            */
#include "sflmem.h"                     /*  Memory allocation functions      */
#include "sflexdr.h"                    /*  Prototypes for functions         */
#include "sflsock.h"                    /*  Required for ntohl, etc -rj      */

/*  Internal function prototypes                                             */

void mem_free_list (byte **ptr_list, dbyte ptr_list_size);


/*  ---------------------------------------------------------------------[<]-
    Function: exdr_write

    Synopsis: Accepts a list of data items, prepares them according to the
    format string, and stores the result in the buffer.  The buffer may be
    transmitted to another system, then decoded using exdr_read.  Assumes
    nothing about system word sizes, etc.  However, does assume that both
    systems use ASCII.  If the buffer address is NULL, does not store the
    data items, but counts the effective size and returns that.
    The null-terminated format string can contain these sequences:
    <Table>
    c                   Single character value (may be multibyte)
    b                   Single byte value
    w,d                 Double byte value (16-bit short integer)
    q,l                 Quad-byte value (32-bit long integer)
    s                   Null-terminated string (address of string), or NULL
    B                   Bool value (16-bit short integer)
    m                   Memory descriptor size (16-bit integer), >= 0
    M                   Memory descriptor body (pointer to block), or NULL
    h                   Huge memory descriptor size (31-bit integer), >= 0
    H                   Huge memory descriptor body (pointer), or NULL
    </Table>
    Each format sequence corresponds to one item in the list.
    The buffer must be large enough to hold the formatted result.  Returns
    the size of the formatted data.  Ignores invalid format characters;
    you can insert hyphens or spaces freely.  Strings may be specified as
    (void *) NULL - they are stored as empty strings.  Memory blocks may be
    specified as 0 and (void *) NULL together.  Note that if you do not use
    the (void *) typecast when calling exdr_write(), your code will fail on
    systems where an int is not the same size as a void *.  Huge memory
    blocks cannot be more than 2^31 bytes large (2Gb) or 2^16 bytes if
    size_t is 16 bits large.
    ---------------------------------------------------------------------[>]-*/

int
exdr_write (byte *buffer, const char *format, ...)
{
    va_list
        argptr;                         /*  Argument list pointer            */
    byte
        byte_value,                     /*  Byte value from arguments        */
        *target,                        /*  Pointer into target buffer       */
        *block;                         /*  Source block for 'M' type        */
    char
        *string;                        /*  Source string for 's' type       */
    dbyte
        dbyte_value;                    /*  Network format dbyte value       */
    qbyte
        memory_size = 0,                /*  Memory descriptor size value     */
        qbyte_value;                    /*  Network format qbyte value       */

    ASSERT (format);
    va_start (argptr, format);          /*  Start variable arguments list    */
    target = buffer;

    while (*format)
      {
        switch (*format++)
          {
            case 'c':                   /*  Character                        */
            case 'b':                   /*  Single byte                      */
                byte_value = (byte) va_arg (argptr, int);
                if (buffer)
                    *(byte *) target = byte_value;
                target += 1;
                break;

            case 'd':                   /*  Signed short integer             */
            case 'w':                   /*  Unsigned short integer           */
            case 'B':                   /*  Bool                             */
                dbyte_value = htons ((short) va_arg (argptr, int));
                if (buffer)
                  {
                    *(byte *) target++ = *((byte *) &dbyte_value);
                    *(byte *) target++ = *((byte *) &dbyte_value + 1);
                  }
                else
                    target += 2;
                break;

            case 'l':                   /*  Signed long (32-bit)             */
            case 'q':                   /*  4-byte unsigned value            */
                qbyte_value = htonl (va_arg (argptr, qbyte));
                if (buffer)
                  {
                    *(byte *) target++ = *((byte *) &qbyte_value);
                    *(byte *) target++ = *((byte *) &qbyte_value + 1);
                    *(byte *) target++ = *((byte *) &qbyte_value + 2);
                    *(byte *) target++ = *((byte *) &qbyte_value + 3);
                  }
                else
                    target += 4;
                break;

            case 's':                   /*  Null-terminated string           */
                string = va_arg (argptr, char *);
                if (string)
                  {
                    if (buffer)
                        strcpy ((char *) target, string);
                    target += strlen (string) + 1;
                  }
                else                    /*  Store NULL as single null byte   */
                  {
                    if (buffer)
                        *(byte *) target++ = 0;
                    else
                        target += 1;
                  }
                break;

            case 'm':                   /*  Memory descriptor size           */
                memory_size = va_arg (argptr, int);
                dbyte_value = htons ((dbyte) memory_size);
                if (buffer)
                  {
                    *(byte *) target++ = *((byte *) &dbyte_value);
                    *(byte *) target++ = *((byte *) &dbyte_value + 1);
                  }
                else
                    target += 2;
                break;
            case 'M':                   /*  Memory descriptor body           */
                block = va_arg (argptr, byte *);
                if (block)
                  {
                    if (buffer)
                        memcpy (target, block, (size_t) memory_size);
                    target += (size_t) memory_size;
                  }
                else
                    ASSERT (memory_size == 0);
                break;

            case 'h':                   /*  Huge memory descriptor size       */
                memory_size = va_arg (argptr, qbyte);
                qbyte_value = htonl (memory_size);
                if (buffer)
                  {
                    *(byte *) target++ = *((byte *) &qbyte_value);
                    *(byte *) target++ = *((byte *) &qbyte_value + 1);
                    *(byte *) target++ = *((byte *) &qbyte_value + 2);
                    *(byte *) target++ = *((byte *) &qbyte_value + 3);
                  }
                else
                    target += 4;
                break;
            case 'H':                   /*  Huge memory descriptor body       */
                block = va_arg (argptr, byte *);
                if (block)
                  {
                    if (buffer)
                        memcpy (target, block, (size_t) memory_size);
                    target += (size_t) memory_size;
                  }
                else
                    ASSERT (memory_size == 0);
                break;
          }
      }
    va_end (argptr);                    /*  End variable arguments list      */
    return ((int) (target - buffer));
}


/*  ---------------------------------------------------------------------[<]-
    Function: exdr_writed

    Synopsis: As exdr_write(), but accepts a DESCR buffer.  This is more
    secure.  Aborts with an error if the formatted data would be too long for
    the buffer, if compiled with DEBUG.  The buffer address cannot be NULL.
    ---------------------------------------------------------------------[>]-*/

int
exdr_writed (DESCR *buffer, const char *format, ...)
{
    va_list
        argptr;                         /*  Argument list pointer            */
    byte
        *target,                        /*  Pointer into target buffer       */
        *block;                         /*  Source block for 'M' type        */
    char
        *string;                        /*  Source string for 's' type       */
    dbyte
        dbyte_value;                    /*  Network format dbyte value       */
    qbyte
        memory_size = 0,                /*  Memory descriptor size value     */
        qbyte_value;                    /*  Network format qbyte value       */
    size_t
        used_size;                      /*  Current buffer data size         */

    ASSERT (buffer);
    ASSERT (format);
    va_start (argptr, format);          /*  Start variable arguments list    */
    target = buffer-> data;

    while (*format)
      {
        used_size = (size_t) (target - buffer-> data);
        switch (*format++)
          {
            case 'c':                   /*  Character                        */
            case 'b':                   /*  Single byte                      */
                *(byte *) target = (byte) va_arg (argptr, int);
                ASSERT (used_size + 1 <= buffer-> size);
                target += 1;
                break;

            case 'd':                   /*  Signed short integer             */
            case 'w':                   /*  Unsigned short integer           */
            case 'B':                   /*  Bool                             */
                dbyte_value = htons ((short) va_arg (argptr, int));
                ASSERT (used_size + 2 <= buffer-> size);
                *(byte *) target++ = *((byte *) &dbyte_value);
                *(byte *) target++ = *((byte *) &dbyte_value + 1);
                break;

            case 'l':                   /*  Signed long (32-bit)             */
            case 'q':                   /*  4-byte unsigned value            */
                qbyte_value = htonl (va_arg (argptr, qbyte));
                ASSERT (used_size + 4 <= buffer-> size);
                *(byte *) target++ = *((byte *) &qbyte_value);
                *(byte *) target++ = *((byte *) &qbyte_value + 1);
                *(byte *) target++ = *((byte *) &qbyte_value + 2);
                *(byte *) target++ = *((byte *) &qbyte_value + 3);
                break;

            case 's':                   /*  Null-terminated string           */
                string = va_arg (argptr, char *);
                if (string)
                  {
                    ASSERT (used_size + strlen (string) + 1 <= buffer-> size);
                    strcpy ((char *) target, string);
                    target += strlen (string) + 1;
                  }
                else                    /*  Store NULL as single null byte   */
                  {
                    ASSERT (used_size + 1 <= buffer-> size);
                    *(byte *) target++ = 0;
                  }
                break;

            case 'm':                   /*  Memory descriptor size           */
                memory_size = va_arg (argptr, int);
                ASSERT (used_size + 2 + memory_size <= buffer-> size);
                dbyte_value = htons ((dbyte) memory_size);
                *(byte *) target++ = *((byte *) &dbyte_value);
                *(byte *) target++ = *((byte *) &dbyte_value + 1);
                break;
            case 'M':                   /*  Memory descriptor body           */
                block = va_arg (argptr, byte *);
                if (block)
                  {
                    memcpy (target, block, (size_t) memory_size);
                    target += (size_t) memory_size;
                  }
                else
                    ASSERT (memory_size == 0);
                break;

            case 'h':                   /*  Huge memory descriptor size      */
                memory_size = va_arg (argptr, qbyte);
                ASSERT (used_size + 4 + memory_size <= buffer-> size);
                qbyte_value = htonl (memory_size);
                *(byte *) target++ = *((byte *) &qbyte_value);
                *(byte *) target++ = *((byte *) &qbyte_value + 1);
                *(byte *) target++ = *((byte *) &qbyte_value + 2);
                *(byte *) target++ = *((byte *) &qbyte_value + 3);
                break;
            case 'H':                   /*  Huge memory descriptor body      */
                block = va_arg (argptr, byte *);
                if (block)
                  {
                    memcpy (target, block, (size_t) memory_size);
                    target += (size_t) memory_size;
                  }
                else
                    ASSERT (memory_size == 0);
                break;
          }
      }
    va_end (argptr);                    /*  End variable arguments list      */
    return ((int) (target - buffer-> data));
}


/*  ---------------------------------------------------------------------[<]-
    Function: exdr_read

    Synopsis: Unpacks a buffer prepared by exdr_write() into as set of data
    items as specified by a format string.  See exdr_write() for the syntax
    of the format string.  Each format sequence corresponds to one item in
    in the list which must be specified as an address.  Target strings and
    memory blocks must be large enough to hold the returned data: target
    strings and blocks can also be null, in which case the function calls
    mem_alloc to allocate heap memory.  Note that you must supply a pointer
    to the string or memory block address, not the address itself.  It is a
    common error to pass the address of a static block - see the example
    below for the *right* way to do it.  Any of the argument addresses can
    be NULL, in which case that field is ignored.  This is useful to get a
    few selected fields out of a message.  Errors in the argument list can
    cause memory corruption and unpredictable program results.

    If a memory allocation fails, all previous memory allocations are "rolled
    back" and the function returns the value -1.

    Return codes: 0  - normal
                  -1 - memory allocation failed

    Examples:
    char *string = NULL;
    byte buffer [1000];
    byte *buffaddr = buffer;
    int value, length;
    exdr_read (buffer, "qdsmM", NULL, &value, &string, &length, &buffaddr);
    ---------------------------------------------------------------------[>]-*/

int
exdr_read (const byte *buffer, const char *format, ...)
{
    MEMTRN
        *memtrn;                        /*  Memory transaction               */
    va_list
        argptr;                         /*  Argument list pointer            */
    void
        *target;                        /*  Source data item address         */
    dbyte
        string_size,                    /*  String size                      */
        dbyte_value;                    /*  Network format dbyte value       */
    qbyte
        memory_size = 0,                /*  Memory descriptor size value     */
        qbyte_value;                    /*  Network format qbyte value       */

    ASSERT (buffer);
    ASSERT (format);
    memtrn = mem_new_trans ();

    va_start (argptr, format);          /*  Start variable arguments list    */
    while (*format)
      {
        target = va_arg (argptr, void *);
        switch (*format++)
          {
            case 'c':                   /*  Character                        */
            case 'b':                   /*  Single byte                      */
                if (target)
                    *(byte *) target = *(byte *) buffer;
                buffer += 1;
                break;

            case 'd':                   /*  Signed short integer             */
            case 'w':                   /*  Unsigned short integer           */
            case 'B':                   /*  Bool                             */
                *((byte *) &dbyte_value)     = *(byte *) buffer++;
                *((byte *) &dbyte_value + 1) = *(byte *) buffer++;
                if (target)
                    *(dbyte *) target = ntohs (dbyte_value);
                break;

            case 'l':                   /*  Signed long (32-bit)             */
            case 'q':                   /*  4-byte unsigned value            */
                *((byte *) &qbyte_value)     = *(byte *) buffer++;
                *((byte *) &qbyte_value + 1) = *(byte *) buffer++;
                *((byte *) &qbyte_value + 2) = *(byte *) buffer++;
                *((byte *) &qbyte_value + 3) = *(byte *) buffer++;
                if (target)
                    *(qbyte *) target = ntohl (qbyte_value);
                break;

            case 's':                   /*  Null-terminated string           */
                string_size = strlen ((char *) buffer) + 1;
                if (target)
                  {
                    if (*(byte **) target == NULL)
                        *(byte **) target = mem_alloc (string_size);
                    if (*(byte **) target)
                        memcpy (*(byte **) target, buffer, string_size);
                    else
                      {
                        mem_rollback (memtrn);
                        return (-1);
                      }
                  }
                buffer += string_size;
                break;

            case 'm':                   /*  Memory descriptor size           */
                *((byte *) &dbyte_value)     = *(byte *) buffer++;
                *((byte *) &dbyte_value + 1) = *(byte *) buffer++;
                memory_size = ntohs (dbyte_value);
                if (target)
                    *(dbyte *) target = (dbyte) memory_size;
                break;
            case 'M':                   /*  Memory descriptor body           */
                if (target && memory_size > 0)
                  {
                    if (*(byte **) target == NULL)
                        *(byte **) target = mem_alloc ((size_t) memory_size);
                    if (*(byte **) target)
                        memcpy (*(byte **) target, buffer,
                                 (size_t) memory_size);
                    else
                      {
                        mem_rollback (memtrn);
                        return (-1);
                      }
                  }
                buffer += (size_t) memory_size;
                break;

            case 'h':                   /*  Huge memory descriptor size      */
                *((byte *) &qbyte_value)     = *(byte *) buffer++;
                *((byte *) &qbyte_value + 1) = *(byte *) buffer++;
                *((byte *) &qbyte_value + 2) = *(byte *) buffer++;
                *((byte *) &qbyte_value + 3) = *(byte *) buffer++;
                memory_size = ntohl (qbyte_value);
                if (target)
                    *(qbyte *) target = memory_size;
                break;
            case 'H':                   /*  Huge memory descriptor body      */
                if (target && memory_size > 0)
                  {
                    if (*(byte **) target == NULL)
                        *(byte **) target = mem_alloc ((size_t) memory_size);
                    if (*(byte **) target)
                        memcpy (*(byte **) target, buffer,
                                 (size_t) memory_size);
                    else
                      {
                        mem_rollback (memtrn);
                        return (-1);
                      }
                  }
                buffer += (size_t) memory_size;
                break;
          }
      }
    va_end (argptr);                    /*  End variable arguments list      */
    mem_commit (memtrn);
    return (0);
}


/*  Function: mem_free_list -- internal
 *
 *  Synopsis: exdr_rollback accepts a list of byte * of a given size. It
 *            calls mem_free for all non-zero pointrs in that list
 */

void
mem_free_list (byte **ptr_list, dbyte ptr_list_size)
{
    while (ptr_list_size--)
      {
        if (*ptr_list)
            mem_free (*ptr_list);
        ptr_list++;
      }
}
