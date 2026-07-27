/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  070401 MF   First version.
 *  050104 MF   Add TA_RegressionTest calls.
 *  080605 MF   Add tests for pseudo-random generator.
 *  091705 MF   Add tests for TA_AddTimeToTimestamp (Fix#1293953).
 *  110906 MF   Remove pseudo-random to eliminate dependencies.
 */

/* Description:
 *         Regression testing of some internal utility like:
 *            - collections: List/Stack/Circular buffer.
 *            - Memory allocation mechanism.
 *            etc...
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_memory.h"
#include "ta_defs.h"
#include "ta_common.h"
#include "codegen_pipe.h"


/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static ErrorNumber testCircularBuffer( void );
static ErrorNumber testBoundedAppend( void );

static TA_RetCode circBufferFillFrom0ToSize( int size, int *buffer );


/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
/* None */

/**** Local functions definitions.     ****/
ErrorNumber test_internals( void )
{
   ErrorNumber retValue;

   printf( "Testing utility functions\n" );

   retValue = testCircularBuffer();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Circular buffer tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testBoundedAppend();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Bounded append tests (%d)\n", retValue );
      return retValue;
   }

   return TA_TEST_PASS; /* Success. */
}

/* codegen_appendf/codegen_appendc replaced the
 *    pos += snprintf(buf + pos, buf_size - pos, ...)
 * idiom used to build every JSON-RPC request/response in the test tools.
 * That idiom lets `pos` run past `buf_size` on the first truncation; the next
 * call then passes a negative int that converts to a huge size_t and writes
 * past the buffer (CodeQL cpp/overflowing-snprintf, CWE-190/CWE-253).
 *
 * The contract asserted here: `pos` NEVER exceeds `buf_size - 1`, the buffer
 * stays NUL-terminated, and nothing is ever written at or past `buf_size`.
 * A guard region filled with 0xA5 detects the latter directly, so the test
 * fails on the actual overflow rather than on a symptom.
 *
 * Non-vacuity: ARENA is much larger than BUFSZ and truncation is forced
 * (TA_INTERNAL_APPEND_VACUOUS fires if the overflowing loop never saturates). */
#define APPEND_ARENA  256
#define APPEND_BUFSZ   16
#define APPEND_GUARD  0xA5

static int appendGuardIntact( const char *arena, int bufSize )
{
   int i;
   for( i = bufSize; i < APPEND_ARENA; i++ )
      if( (unsigned char)arena[i] != APPEND_GUARD )
         return 0;
   return 1;
}

static ErrorNumber testBoundedAppend( void )
{
   char arena[APPEND_ARENA];
   int pos, i, saturatedAt;

   /* 1. Plain accumulation stays exact while it fits. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   pos = codegen_appendf( arena, APPEND_BUFSZ, 0, "ab" );
   pos = codegen_appendc( arena, APPEND_BUFSZ, pos, 'c' );
   pos = codegen_appendf( arena, APPEND_BUFSZ, pos, "%d", 42 );
   if( pos != 5 || strcmp( arena, "abc42" ) != 0 )
      return TA_INTERNAL_APPEND_FAIL_0;
   if( !appendGuardIntact( arena, APPEND_BUFSZ ) )
      return TA_INTERNAL_APPEND_FAIL_1;

   /* 2. A single oversized append truncates, saturates and NUL-terminates. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   pos = codegen_appendf( arena, APPEND_BUFSZ, 0, "%s",
                          "0123456789ABCDEF0123456789ABCDEF" );
   if( pos != APPEND_BUFSZ - 1 )
      return TA_INTERNAL_APPEND_FAIL_2;
   if( arena[APPEND_BUFSZ - 1] != '\0' || strlen( arena ) != APPEND_BUFSZ - 1u )
      return TA_INTERNAL_APPEND_FAIL_3;
   if( !appendGuardIntact( arena, APPEND_BUFSZ ) )
      return TA_INTERNAL_APPEND_FAIL_4;

   /* 3. The overflow loop: keep appending well past capacity. This is the
    *    shape that used to walk off the end. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   pos = 0;
   saturatedAt = -1;
   for( i = 0; i < 64; i++ )
   {
      pos = codegen_appendf( arena, APPEND_BUFSZ, pos, ",%d", i );
      pos = codegen_appendc( arena, APPEND_BUFSZ, pos, 'x' );
      if( pos < 0 || pos > APPEND_BUFSZ - 1 )
         return TA_INTERNAL_APPEND_FAIL_5;
      if( saturatedAt < 0 && pos == APPEND_BUFSZ - 1 )
         saturatedAt = i;
   }
   if( !appendGuardIntact( arena, APPEND_BUFSZ ) )
      return TA_INTERNAL_APPEND_FAIL_6;
   if( saturatedAt < 0 )
      return TA_INTERNAL_APPEND_VACUOUS;   /* never truncated: test proved nothing */

   /* 4. Degenerate sizes must not write at all. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   if( codegen_appendf( arena, 0, 0, "x" ) != 0 ||
       codegen_appendc( arena, 0, 0, 'x' ) != 0 ||
       !appendGuardIntact( arena, 0 ) )
      return TA_INTERNAL_APPEND_FAIL_7;

   /* buf_size 1 holds only the terminator; both helpers must stay at 0. */
   memset( arena, APPEND_GUARD, sizeof(arena) );
   if( codegen_appendf( arena, 1, 0, "x" ) != 0 ||
       codegen_appendc( arena, 1, 0, 'x' ) != 0 ||
       !appendGuardIntact( arena, 1 ) )
      return TA_INTERNAL_APPEND_FAIL_8;

   return TA_TEST_PASS; /* Success. */
}

static ErrorNumber testCircularBuffer( void )
{
   TA_RetCode retCode;
   int i;
   int buffer[20];
   ErrorNumber retValue;

   /* Initialize the library. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   /* The following function is supose to fill
    * the buffer with the value 0 to 8 sequentialy,
    * if somehow it is not 0 to 8, there is a bug!
    */
   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 1, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (1+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_1;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 2, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (2+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_2;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 3, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (3+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_3;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 4, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (4+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_4;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 5, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (5+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_5;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 6, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (6+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_6;
      }
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS; /* Success. */
}

/* This function is suppose to fill the buffer
 * with values going from 0 to 'size'.
 * The filling is done using the CIRCBUF macros.
 */
static TA_RetCode circBufferFillFrom0ToSize( int size, int *buffer )
{
   CIRCBUF_PROLOG(MyBuf,int,4);
   int i, value;
   int outIdx;

   CIRCBUF_INIT(MyBuf,int,size);

   outIdx = 0;

   // 1st Loop: Fill MyBuf with initial values
   //           (must be done).
   value = 0;
   for( i=0; i < size; i++ )
   {
      MyBuf[MyBuf_Idx] = value++;
      CIRCBUF_NEXT(MyBuf);
   }

   // 2nd Loop: Get and Add subsequent values
   //           in MyBuf (optional)
   for( i=0; i < 3; i++ )
   {
      buffer[outIdx++] = MyBuf[MyBuf_Idx];
      MyBuf[MyBuf_Idx] = value++;
      CIRCBUF_NEXT(MyBuf);
   }

   // 3rd Loop: Empty MyBuf (optional)
   for( i=0; i < size; i++ )
   {
      buffer[outIdx++] = MyBuf[MyBuf_Idx];
      CIRCBUF_NEXT(MyBuf);
   }

   CIRCBUF_DESTROY(MyBuf);

   return TA_SUCCESS;
}
