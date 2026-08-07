/* Fail-loud allocation checking for the TA-Lib developer tools (issue #177).
 *
 * ta_regtest / ta_bench are not the shipped library: nothing here promises
 * TA_ALLOC_ERR to a caller, so a dev box that runs out of memory is better
 * served by a diagnosable exit than by a NULL deref -- in a test runner a
 * mysterious segfault looks like the defect under test.
 *
 * Macros rather than a static helper, so a translation unit that includes the
 * header without allocating draws no -Wunused-function.
 */

#ifndef TA_ALLOC_CHECK_H
#define TA_ALLOC_CHECK_H

#include <stdio.h>
#include <stdlib.h>

/* Report an out-of-memory failure for `what` and exit. */
#define TA_TOOL_OOM( what )                                        \
   do {                                                            \
      fflush( stdout );                                            \
      fprintf( stderr, "\nFATAL: out of memory allocating %s"       \
                       " (%s:%d)\n", (what), __FILE__, __LINE__ );  \
      exit( EXIT_FAILURE );                                        \
   } while( 0 )

/* The common case: one pointer, named after itself in the message. */
#define TA_TOOL_CHECK_ALLOC( ptr )                                 \
   do {                                                            \
      if( (ptr) == NULL ) TA_TOOL_OOM( #ptr );                     \
   } while( 0 )

#endif
