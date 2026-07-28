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
static ErrorNumber testUnstablePeriodBounds( void );
static ErrorNumber testEnumValueContract( void );

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

   retValue = testUnstablePeriodBounds();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Unstable period bound tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testEnumValueContract();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Public enum value contract (%d)\n", retValue );
      return retValue;
   }

   return TA_TEST_PASS; /* Success. */
}

/* The published value of every enumerator that has ever shipped, pinned.
 *
 * TA_MAType and TA_FuncUnstId are ABI: wrappers record these numbers and pass
 * them back as plain integers, so renumbering one silently re-points a caller at
 * a different function. That is not hypothetical -- TA_FUNC_UNST_IMI was once
 * inserted mid-enum, shifting every id above it, and ta-lib-python still targets
 * the pre-0.6.0 numbering as a result. Nothing detected it, because both ends of
 * a set/get pair use the same wrong id and round-trip perfectly.
 *
 * The contract is APPEND-ONLY: a new indicator adds a row here and to
 * enums.yaml; an existing row must never change. The literals are deliberately
 * hardcoded -- comparing an enumerator to itself would prove nothing.
 *
 * This pins C only, which is sufficient: every other surface (the Rust crate,
 * the shipped Java enum, all four servers) is generated from the same
 * enums.yaml that generates this header, and the generator already fails if the
 * hand-maintained Rust copy drifts from it.
 */
static ErrorNumber testEnumValueContract( void )
{
   typedef struct { const char *name; int shipped; int current; } EnumPin;

   /* Retired ids keep their slot forever (removing one renumbers the rest). */
   static const EnumPin unstPins[] = {
      { "TA_FUNC_UNST_ADX",           0, TA_FUNC_UNST_ADX },
      { "TA_FUNC_UNST_UNUSED_1",      1, TA_FUNC_UNST_UNUSED_1 },
      { "TA_FUNC_UNST_ATR",           2, TA_FUNC_UNST_ATR },
      { "TA_FUNC_UNST_CMO",           3, TA_FUNC_UNST_CMO },
      { "TA_FUNC_UNST_DX",            4, TA_FUNC_UNST_DX },
      { "TA_FUNC_UNST_EMA",           5, TA_FUNC_UNST_EMA },
      { "TA_FUNC_UNST_HT_DCPERIOD",   6, TA_FUNC_UNST_HT_DCPERIOD },
      { "TA_FUNC_UNST_HT_DCPHASE",    7, TA_FUNC_UNST_HT_DCPHASE },
      { "TA_FUNC_UNST_HT_PHASOR",     8, TA_FUNC_UNST_HT_PHASOR },
      { "TA_FUNC_UNST_HT_SINE",       9, TA_FUNC_UNST_HT_SINE },
      { "TA_FUNC_UNST_HT_TRENDLINE", 10, TA_FUNC_UNST_HT_TRENDLINE },
      { "TA_FUNC_UNST_HT_TRENDMODE", 11, TA_FUNC_UNST_HT_TRENDMODE },
      { "TA_FUNC_UNST_UNUSED_12",    12, TA_FUNC_UNST_UNUSED_12 },
      { "TA_FUNC_UNST_KAMA",         13, TA_FUNC_UNST_KAMA },
      { "TA_FUNC_UNST_MAMA",         14, TA_FUNC_UNST_MAMA },
      { "TA_FUNC_UNST_UNUSED_15",    15, TA_FUNC_UNST_UNUSED_15 },
      { "TA_FUNC_UNST_MINUS_DI",     16, TA_FUNC_UNST_MINUS_DI },
      { "TA_FUNC_UNST_MINUS_DM",     17, TA_FUNC_UNST_MINUS_DM },
      { "TA_FUNC_UNST_NATR",         18, TA_FUNC_UNST_NATR },
      { "TA_FUNC_UNST_PLUS_DI",      19, TA_FUNC_UNST_PLUS_DI },
      { "TA_FUNC_UNST_PLUS_DM",      20, TA_FUNC_UNST_PLUS_DM },
      { "TA_FUNC_UNST_RSI",          21, TA_FUNC_UNST_RSI },
      { "TA_FUNC_UNST_UNUSED_22",    22, TA_FUNC_UNST_UNUSED_22 },
      { "TA_FUNC_UNST_T3",           23, TA_FUNC_UNST_T3 },
      /* Pinned so adding an indicator can never move it (#144). */
      { "TA_FUNC_UNST_ALL",       65535, TA_FUNC_UNST_ALL }
   };

   static const EnumPin maPins[] = {
      { "TA_MAType_SMA",       0, TA_MAType_SMA },
      { "TA_MAType_EMA",       1, TA_MAType_EMA },
      { "TA_MAType_WMA",       2, TA_MAType_WMA },
      { "TA_MAType_DEMA",      3, TA_MAType_DEMA },
      { "TA_MAType_TEMA",      4, TA_MAType_TEMA },
      { "TA_MAType_TRIMA",     5, TA_MAType_TRIMA },
      { "TA_MAType_KAMA",      6, TA_MAType_KAMA },
      { "TA_MAType_MAMA",      7, TA_MAType_MAMA },
      { "TA_MAType_T3",        8, TA_MAType_T3 },
      { "TA_MAType_HMA",       9, TA_MAType_HMA },
      { "TA_MAType_DISABLED", 10, TA_MAType_DISABLED }
   };

   /* Returned to every caller and mapped by name in the wrappers (ta-lib-python
    * hardcodes this whole list in _ta_check_success), so it is ABI too.
    */
   static const EnumPin retCodePins[] = {
      { "TA_SUCCESS",                       0, TA_SUCCESS },
      { "TA_LIB_NOT_INITIALIZE",            1, TA_LIB_NOT_INITIALIZE },
      { "TA_BAD_PARAM",                     2, TA_BAD_PARAM },
      { "TA_ALLOC_ERR",                     3, TA_ALLOC_ERR },
      { "TA_GROUP_NOT_FOUND",               4, TA_GROUP_NOT_FOUND },
      { "TA_FUNC_NOT_FOUND",                5, TA_FUNC_NOT_FOUND },
      { "TA_INVALID_HANDLE",                6, TA_INVALID_HANDLE },
      { "TA_INVALID_PARAM_HOLDER",          7, TA_INVALID_PARAM_HOLDER },
      { "TA_INVALID_PARAM_HOLDER_TYPE",     8, TA_INVALID_PARAM_HOLDER_TYPE },
      { "TA_INVALID_PARAM_FUNCTION",        9, TA_INVALID_PARAM_FUNCTION },
      { "TA_INPUT_NOT_ALL_INITIALIZE",     10, TA_INPUT_NOT_ALL_INITIALIZE },
      { "TA_OUTPUT_NOT_ALL_INITIALIZE",    11, TA_OUTPUT_NOT_ALL_INITIALIZE },
      { "TA_OUT_OF_RANGE_START_INDEX",     12, TA_OUT_OF_RANGE_START_INDEX },
      { "TA_OUT_OF_RANGE_END_INDEX",       13, TA_OUT_OF_RANGE_END_INDEX },
      { "TA_INVALID_LIST_TYPE",            14, TA_INVALID_LIST_TYPE },
      { "TA_BAD_OBJECT",                   15, TA_BAD_OBJECT },
      { "TA_NOT_SUPPORTED",                16, TA_NOT_SUPPORTED },
      { "TA_INTERNAL_ERROR",             5000, TA_INTERNAL_ERROR },
      { "TA_UNKNOWN_ERR",              0xFFFF, TA_UNKNOWN_ERR }
   };

   /* Every pinned unstable id except the trailing ALL wildcard. */
   const int nbUnstIds = (int)(sizeof(unstPins)/sizeof(unstPins[0])) - 1;
   unsigned int i;

   for( i=0; i < sizeof(retCodePins)/sizeof(retCodePins[0]); i++ )
   {
      if( retCodePins[i].current != retCodePins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        every wrapper maps them by number. Append, never renumber.\n",
                 retCodePins[i].name, retCodePins[i].current, retCodePins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_3;
      }
   }

   for( i=0; i < sizeof(unstPins)/sizeof(unstPins[0]); i++ )
   {
      if( unstPins[i].current != unstPins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        wrappers pass them back as integers. Append new ids, never renumber.\n",
                 unstPins[i].name, unstPins[i].current, unstPins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_0;
      }
   }

   for( i=0; i < sizeof(maPins)/sizeof(maPins[0]); i++ )
   {
      if( maPins[i].current != maPins[i].shipped )
      {
         printf( "\nFailed: %s is %d but shipped as %d. These values are ABI --\n"
                 "        they are the optInMAType a caller passes. Append, never renumber.\n",
                 maPins[i].name, maPins[i].current, maPins[i].shipped );
         return TA_INTERNAL_ENUM_CONTRACT_FAIL_1;
      }
   }

   /* The count must equal the number of ids pinned above -- not merely be no
    * smaller. A ">=" test would let a newly added indicator sit unpinned
    * indefinitely, which is exactly the protection this table exists to give:
    * adding an id has to fail here until its row is added, or the contract only
    * covers whatever someone remembered to write down.
    */
   if( TA_FUNC_UNST_COUNT != nbUnstIds )
   {
      printf( "\nFailed: TA_FUNC_UNST_COUNT is %d but %d ids are pinned. Add the new\n"
              "        id's row to unstPins[] (append only -- never renumber).\n",
              TA_FUNC_UNST_COUNT, nbUnstIds );
      return TA_INTERNAL_ENUM_CONTRACT_FAIL_2;
   }

   /* Ids grow upward and the wildcard is fixed, so they must never meet. */
   if( TA_FUNC_UNST_COUNT >= TA_FUNC_UNST_ALL )
   {
      printf( "\nFailed: TA_FUNC_UNST_COUNT (%d) has reached TA_FUNC_UNST_ALL (%d)\n",
              TA_FUNC_UNST_COUNT, (int)TA_FUNC_UNST_ALL );
      return TA_INTERNAL_ENUM_CONTRACT_FAIL_2;
   }

   return TA_TEST_PASS;
}

/* TA_Set/GetUnstablePeriod index TA_Globals->unstablePeriod[id] after a bound
 * check that used to test only the upper end. TA_TEST_UNST_NONE is -1 and makes
 * the enum signed, so every negative id slipped past and read/wrote off the
 * front of the array -- onto TA_Globals->compatibility, which sits immediately
 * before it. The setter still returned TA_SUCCESS while silently corrupting the
 * global (issue #144).
 *
 * Asserted here: both sentinels and an arbitrary negative are rejected, the
 * wildcard still sets every function, and a normal id still round-trips.
 * Non-vacuity: the setter half is caught by the returned TA_BAD_PARAM, and the
 * getter half only because compatibility is parked at a non-zero value first --
 * otherwise an out-of-bounds read of it returns 0 and looks correct.
 */
static ErrorNumber testUnstablePeriodBounds( void )
{
   ErrorNumber retValue;
   TA_RetCode retCode;
   int id;

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   /* Park a non-zero value in the field that unstablePeriod[-1] aliases, so the
    * assertions below can tell a real guard from an accidental zero. Without
    * this the getter checks pass even with the guard reverted, because a fresh
    * TA_Initialize leaves compatibility == 0 and an out-of-bounds read of it
    * looks exactly like the correct answer.
    */
   TA_SetCompatibility( TA_COMPATIBILITY_METASTOCK );

   /* Out-of-range ids must be refused, not indexed. */
   if( TA_SetUnstablePeriod( TA_TEST_UNST_NONE, 99 ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( (TA_FuncUnstId)-1000000, 99 ) != TA_BAD_PARAM ||
       TA_SetUnstablePeriod( (TA_FuncUnstId)TA_FUNC_UNST_COUNT, 99 ) != TA_BAD_PARAM )
   {
      printf( "\nFailed: out-of-range TA_SetUnstablePeriod id not rejected\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_0;
   }

   /* ...and must not have written anything. TA_Globals->compatibility is the
    * field the id == -1 write landed on.
    */
   if( TA_GetCompatibility() != TA_COMPATIBILITY_METASTOCK )
   {
      printf( "\nFailed: rejected TA_SetUnstablePeriod id corrupted compatibility\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_1;
   }

   /* Reads of the sentinels are defined as 0, never an out-of-bounds load. With
    * compatibility == METASTOCK (1) above, an unguarded read of [-1] yields 1
    * and fails here.
    */
   if( TA_GetUnstablePeriod( TA_TEST_UNST_NONE ) != 0 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_ALL ) != 0 ||
       TA_GetUnstablePeriod( (TA_FuncUnstId)-1000000 ) != 0 )
   {
      printf( "\nFailed: sentinel TA_GetUnstablePeriod did not read as 0\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_2;
   }

   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   /* The valid range still works: the wildcard sets every function, a single
    * id round-trips, and neither disturbs compatibility.
    */
   retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 7 );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed: TA_SetUnstablePeriod wildcard RetCode = %d\n", retCode );
      return TA_INTERNAL_UNST_BOUND_FAIL_3;
   }
   for( id=0; id < TA_FUNC_UNST_COUNT; id++ )
   {
      if( TA_GetUnstablePeriod( (TA_FuncUnstId)id ) != 7 )
      {
         printf( "\nFailed: TA_SetUnstablePeriod wildcard missed id %d\n", id );
         return TA_INTERNAL_UNST_BOUND_FAIL_3;
      }
   }

   retCode = TA_SetUnstablePeriod( TA_FUNC_UNST_RSI, 3 );
   if( retCode != TA_SUCCESS ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_RSI ) != 3 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_EMA ) != 7 ||
       TA_GetUnstablePeriod( TA_FUNC_UNST_ADX ) != 7 ||
       TA_GetCompatibility() != TA_COMPATIBILITY_DEFAULT )
   {
      printf( "\nFailed: single-id TA_SetUnstablePeriod round-trip\n" );
      return TA_INTERNAL_UNST_BOUND_FAIL_3;
   }

   /* Pairs with the allocLib() above (as testCircularBuffer does) -- shutting
    * down zeroes TA_Globals, so the periods set here cannot leak into any
    * later test.
    */
   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
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
