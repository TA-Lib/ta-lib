/*  ----------------------------------------------------------------<Prolog>-
    Name:       sflprint.h
    Title:      Printing Functions
    Package:    Standard Function Library (SFL)

    Written:    1999/09/10  iMatix SFL project team <sfl@imatix.com>
    Revised:    1999/09/10

    Synopsis:   Provides printing functions which may be absent on some
                systems.   In particular ensures that the system has 
		snprintf()/vsnprintf() functions which can be called.  The
                functions supplied here are not as good as the vender 
		supplied ones, but are better than having none at all.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "prelude.h"                    /*  Universal header file            */
#include "sflmem.h"                     /*  Memory allocation functions      */
#include "sflprint.h"                   /*  Prototypes for functions         */

#define SAFETY_FACTOR  2                /*  How much bigger to require temp  */
                                        /*  buffer to be than expected length*/
#define BUFFER_LEN     2048             /*  Length of temporary buffer       */

#if (! defined (DOES_SNPRINTF))
static char 
    shared_buffer[BUFFER_LEN];          /*  Static buffer for printing into  */
#endif

#if (! defined (DOES_SNPRINTF))
/*  ---------------------------------------------------------------------[<]-
    Function: snprintf

    Synopsis: Writes formatted output into supplied string, up to a maximum
    supplied length.  This function is provided for systems which do not have
    a snprintf() function in their C library.  It uses a temporary buffer
    to print into, and providing that temporary buffer wasn't overflowed it
    copies the data up to the supplied length into the supplied buffer.  If
    the temporary buffer was overflowed it exits immediately.  (This is a
    poor man's snprintf(), but allows other code to use snprintf() and get
    the advantages of the better library implementations where available.)
    snprintf() is implemented in terms of vsnprintf().
    Based on the GNU interface (the C9X interface is slightly different).

    Returns:  number of characters output if less than length supplied, 
              otherwise -1.

    Examples:

    char buffer [50];
    int  len;

    len = snprintf (buffer, sizeof(buffer), "Hello %s", "World");
    ---------------------------------------------------------------------[>]-*/

int snprintf  (char *str, size_t n, const char *format, ...)
{
    va_list ap;
    int     rc = 0;

    va_start (ap, format);
    rc = vsnprintf (str, n, format, ap);
    va_end   (ap);

    return rc;
}
#endif

#if (! defined (DOES_SNPRINTF))
/*  ---------------------------------------------------------------------[<]-
    Function: vsnprintf

    Synopsis: Writes formatted output into supplied string, up to a maximum
    supplied length, given a va_list of arguments with variables in it.  
    This function is provided for systems which do not have a vsnprintf() 
    function in their C library.  It uses a temporary buffer to print into, 
    and providing that temporary buffer wasn't overflowed it copies the data 
    up to the supplied length into the supplied buffer.  If the temporary 
    buffer was overflowed it exits immediately.  (This is a poor man's 
    snprintf(), but allows other code to use snprintf() and get the 
    advantages of the better library implementations where available.)
    Based on the GNU interface (the C9X interface is slightly different).

    Returns:  number of characters output if less than length supplied, 
              otherwise -1.

    Examples:

    char buffer[50];
    int len;
    va_list ap;

    va_start (ap, lastarg);
    len = snprintf (buffer, sizeof(buffer), "Hello %s", ap);
    va_end (ap);
    ---------------------------------------------------------------------[>]-*/

int vsnprintf (char *str, size_t n, const char *format, va_list ap)
{
    char *
        buffer  = shared_buffer;              /*  Temporary buffer to use    */
    int
        buflen  = sizeof(shared_buffer),      /*  Size of temporary buffer   */
        outputlen = 0;                        /*  Length of output           */
    Bool
        freebuf = FALSE;                      /*  If true, free buffer       */

    /*  Make sure we have a big enough temporary buffer                      */
    if (buflen < (SAFETY_FACTOR * n)) 
      {
	buflen = SAFETY_FACTOR * n;
	buffer = (char *)mem_alloc (buflen);

	ASSERT (buffer);
	if (! buffer) 
	  {
	    str [0] = '\0';
	    return -1;
	  }
	else
	    freebuf = TRUE;
      }

    /*  Do vsprintf() into temporary buffer                                  */
    outputlen = vsprintf (buffer, format, ap);

    /*  Check to see if we overflowed our temporary buffer: panic if we did  */
    if (outputlen > buflen) 
      {
	/*  Oh, no, buffer overflow!  It's in a static buffer, or in the     */
	/*  heap, so it is harder to exploit, but either way the system      */
	/*  is in an uncertain state, so we give up immediately.             */

	ASSERT (outputlen <= buflen);            /*  To aid debugging        */
        write (2, "vsnprintf buffer overflow\n", 26);    /*  To stderr       */

	exit (EXIT_FAILURE);
	ASSERT (FALSE);                          /*  Unreachable             */
      }

    /*  Okay, everything looks reasonable.  Copy into real buffer now.       */
    strncpy (str, buffer, n);
    str [n - 1] = '\0';                          /*  NUL terminate string    */

    if (freebuf)
	mem_free (buffer);

    return (outputlen < n ? outputlen : -1);
}
#endif
