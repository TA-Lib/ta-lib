/*  ----------------------------------------------------------------<Prolog>-
    Name:       testtron.c
    Title:      Test program for output trace functions
    Package:    Standard Function Library (SFL)

    Written:    1996/05/04  iMatix SFL project team <sfl@imatix.com>
    Revised:    1998/05/04

    Synopsis:   Outputs some test messages to stdout, and file.  Because
                the date is included in the messages they are not checked
                internally, but can be checked against an external reference
                by stripping the date off the start of both files.

    Copyright:  Copyright (c) 1996-2000 iMatix Corporation
    License:    This is free software; you can redistribute it and/or modify
                it under the terms of the SFL License Agreement as provided
                in the file LICENSE.TXT.  This software is distributed in
                the hope that it will be useful, but without any warranty.
 ------------------------------------------------------------------</Prolog>-*/

#include "sfl.h"

#define  TRACE_TEST_FILE "trace.tst"

int main (int argc, char *argv [])
{
  int c = 0;

  printf("Testing sfltron.c functions\n\n");

  /* Trace is initially disabled */
  trace("Initial test, not output (%d)", c++);

  enable_trace();

  trace("First line after enabling tracing (%d)", c++);

  disable_trace();

  trace("A line after disabling tracing again (%d)", c++);

  push_trace(TRUE);
  trace("Trace enabled, but saved disabled (%d)", c++);
  pop_trace();

  trace("Trace disabled, not shown (%d", c++);

  enable_trace();
  trace("Trace enabled again, to save enabled (%d)", c++);
  push_trace(FALSE);
  trace("Trace disabled, but saved enabled (%d)", c++);
  pop_trace();
  trace("Trace enabled again, from saved state (%d)", c++);

  trace("Multiple strings passed in: %s, %s, %s (%d)", 
        "one", "two", "three", c++);

  set_trace_file(TRACE_TEST_FILE, 'w');
  trace("First line traced to the output file (%d)", c++);
  disable_trace();
  trace("Trace disabled, this line goes to ether (%d)", c++);
  push_trace(TRUE);
  trace("Trace enabled, but saved disabled (%d)", c++);
  pop_trace();

  trace("Trace disabled, not shown (%d", c++);

  enable_trace();
  trace("Trace enabled again, to save enabled (%d)", c++);
  push_trace(FALSE);
  trace("Trace disabled, but saved enabled (%d)", c++);
  pop_trace();
  trace("Trace enabled again, from saved state (%d)", c++);

  trace("Multiple strings passed in: %s, %s, %s (%d)", 
        "one", "two", "three", c++);

  set_trace_file(NULL, 'w');
  trace("Tracing back to the console, not file (%d)", c++);

  return (0);
}

