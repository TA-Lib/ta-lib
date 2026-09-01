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
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  082326 MF,CC  First version. DIV's documented zero-divisor result
 *                (issue #249).
 */

/* Description:
 *
 * DIV's published Notes say: "Zero divided by zero gives NaN; anything else
 * divided by zero gives positive or negative infinity. Neither is reported as an
 * error." That is true by construction in every backend, and nothing else
 * asserts the VALUE: the abstract sweep skips its zero dataset for the Math
 * group, the fuzz generators map a generic second real input to volume (never
 * zero), and the finite-output gate EXEMPTS DIV via TA_FUNC_FLG_NAN_INF_OUT --
 * a permission, not an assertion.
 *
 * test_variants.c does feed TA_DIV and TA_S_DIV a +0.0 divisor on every bare
 * ta_regtest run, but all it takes from that is `rcS == rcD` plus a memcmp of
 * the two variants against EACH OTHER: never TA_SUCCESS, never a value, and two
 * variants wrong the same way pass it.
 *
 * What this group pins, on one table covering every sign combination:
 *
 *   (a) TA_DIV returns TA_SUCCESS with the full range of output -- a zero
 *       divisor is not an error and does not truncate outNBElement.
 *   (b) The value is the IEEE-754 one, SIGN INCLUDED: NaN for either zero over
 *       either zero, +/-Inf per the sign rule for a non-zero over a zero.
 *   (c) The controls: a zero NUMERATOR over a non-zero divisor stays a signed
 *       zero, and ordinary quotients are untouched. A guard added to "fix" (b)
 *       would most plausibly over-fire on one of these.
 *   (d) TA_S_DIV agrees BIT FOR BIT with TA_DIV, over every row and against a
 *       stated value rather than against the other variant.
 *   (e) The streaming tier agrees, on BOTH of the loops it emits. `_OpenImpl`
 *       carries its own transcription of the batch body (the warm-up fill) and
 *       `_StepImpl` the per-bar one, so a guard added to one is invisible to the
 *       other -- measured: an `_OpenImpl`-only guard survives every other
 *       assertion in this file. The whole table therefore goes through
 *       OpenAndFill, and again through Peek/Update on a second handle, and the
 *       handle must report having committed every bar. Neither entry point may
 *       reject the bar: both guard their INPUTS with TA_IS_FINITE and a zero
 *       divisor is finite, so a guard mistakenly widened to the OUTPUT would
 *       turn every case here into TA_BAD_PARAM while the batch assertions above
 *       stayed green.
 *   (f) DIV declares TA_FUNC_FLG_NAN_INF_OUT. (a)-(e) are what makes that
 *       declaration true, so the carve-out cannot be silently deleted or spread.
 *
 * Cross-language: each batch call is re-issued through server_verify, which
 * feeds the identical arrays to every running server as lossless
 * hex-of-IEEE-bits and compares a full-precision hash of the raw output bytes
 * against C's. Bit-for-bit, so it covers the NaN PAYLOAD as well as the value.
 * It runs under --codegen only; the C-only assertions above stand alone in a
 * bare run.
 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "server_verify.h"
#include "ta_libc.h"

/* One row per sign combination that IEEE-754 division distinguishes, plus the
 * controls. Every literal is exactly representable as a float, so the TA_S_
 * leg divides the same two numbers. */
typedef struct
{
   double      num;
   double      den;
   int         isNaN;   /* 1 -> the only defined answer is NaN */
   double      value;   /* used when isNaN == 0; its SIGN is part of the claim */
   const char *what;
} DivZeroCase;

#define DZ_N 12

/* Literal, and deliberately not derived from DZ_N -- see the floor check in
 * test_func_div_zero(). */
#define DZ_FLOOR_CASES  12
#define DZ_FLOOR_CHECKS 100

static const DivZeroCase DZ_CASES[DZ_N] =
{
   {  0.0,  0.0, 1,  0.0,                 "+0 / +0 -> NaN"  },
   {  0.0, -0.0, 1,  0.0,                 "+0 / -0 -> NaN"  },
   { -0.0,  0.0, 1,  0.0,                 "-0 / +0 -> NaN"  },
   { -0.0, -0.0, 1,  0.0,                 "-0 / -0 -> NaN"  },
   {  1.5,  0.0, 0,  (double)INFINITY,    "+x / +0 -> +Inf" },
   { -1.5,  0.0, 0, -(double)INFINITY,    "-x / +0 -> -Inf" },
   {  1.5, -0.0, 0, -(double)INFINITY,    "+x / -0 -> -Inf" },
   { -1.5, -0.0, 0,  (double)INFINITY,    "-x / -0 -> +Inf" },
   {  0.0,  4.0, 0,  0.0,                 "+0 / +x -> +0"   },
   { -0.0,  4.0, 0, -0.0,                 "-0 / +x -> -0"   },
   {  6.0,  3.0, 0,  2.0,                 "ordinary quotient" },
   { -6.0,  3.0, 0, -2.0,                 "ordinary quotient" }
};

/* Every value comparison this group takes, counted AT the comparison. Printed
 * and required to match the expected total: "nothing failed" and "nothing was
 * compared" are otherwise the same observation. */
static int g_dzChecked;

/* Six rows need more than `==`: the four NaN rows, which are not comparable at
 * all, and the two signed-zero rows, where +0.0 == -0.0. (`==` does separate
 * +Inf from -Inf on its own; the signbit test is there for the zeros.)
 * Comparing raw bits instead would over-assert -- the NaN payload is the
 * host's, not the library's. */
static int dz_matches( double got, const DivZeroCase *c )
{
   g_dzChecked++;
   if( c->isNaN )
      return got != got;                        /* true only for NaN */
   if( got != c->value )
      return 0;
   return signbit( got ) == signbit( c->value );
}

/* Counted verdicts for the two comparisons that are not a table lookup. Both
 * increment INSIDE the helper, so deleting a call site drops the total rather
 * than leaving the summary printing the same number over fewer checks. */
static int dz_same_bits( double a, double b )
{
   g_dzChecked++;
   return memcmp( &a, &b, sizeof(double) ) == 0;
}

static int dz_rejected( TA_RetCode retCode )
{
   g_dzChecked++;
   return retCode == TA_BAD_PARAM;
}

static ErrorNumber dz_report( const char *tier, int i, double got )
{
   printf( "\nDIV %s: case %d (%s) gave %.17g\n",
           tier, i, DZ_CASES[i].what, got );
   return TA_DIVZERO_BAD_VALUE;
}

/* One TA_DIV call over DZ_CASES[startIdx..endIdx], checked and then re-issued
 * to every active language server. */
static ErrorNumber dz_batch_range( int startIdx, int endIdx )
{
   double     num[DZ_N], den[DZ_N], out[DZ_N];
   int        outBegIdx, outNbElement, i;
   TA_RetCode retCode;

   for( i = 0; i < DZ_N; i++ )
   {
      num[i] = DZ_CASES[i].num;
      den[i] = DZ_CASES[i].den;
      out[i] = 0.0;
   }

   retCode = TA_DIV( startIdx, endIdx, num, den, &outBegIdx, &outNbElement, out );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nDIV batch [%d..%d]: a zero divisor must not be an error, got retCode=%d\n",
              startIdx, endIdx, retCode );
      return TA_DIVZERO_BAD_RETCODE;
   }
   if( outBegIdx != startIdx || outNbElement != endIdx - startIdx + 1 )
   {
      printf( "\nDIV batch [%d..%d]: outBegIdx=%d outNBElement=%d\n",
              startIdx, endIdx, outBegIdx, outNbElement );
      return TA_DIVZERO_BAD_SHAPE;
   }

   for( i = 0; i < outNbElement; i++ )
   {
      if( !dz_matches( out[i], &DZ_CASES[startIdx + i] ) )
         return dz_report( "batch", startIdx + i, out[i] );
   }

   if( server_verify_active() )
   {
      int         cmpBefore = server_verify_comparisons();
      ErrorNumber errNb = server_verify( "DIV", startIdx, endIdx, DZ_N,
                             retCode, outBegIdx, outNbElement,
                             (const TA_Real*[]){ num, den, NULL },
                             NULL, 0,
                             (const TA_Real*[]){ out, NULL }, NULL );
      if( errNb != TA_TEST_PASS ) return errNb;
      if( server_verify_comparisons() == cmpBefore )
      {
         printf( "\nDIV batch [%d..%d]: compared no server despite live pipes\n",
                 startIdx, endIdx );
         return TA_DIVZERO_VACUOUS;
      }
   }

   return TA_TEST_PASS;
}

/* (d) The single-precision variant on the same numbers. Compared by RAW BITS,
 * because that is what test_variants.c claims for the pair and because NaN is
 * not comparable with ==. */
static ErrorNumber dz_single_precision( void )
{
   float      num[DZ_N], den[DZ_N];
   double     outS[DZ_N], outD[DZ_N];
   int        begIdx = -1, nbElement = -1, i;
   TA_RetCode retCode;

   for( i = 0; i < DZ_N; i++ )
   {
      num[i] = (float)DZ_CASES[i].num;
      den[i] = (float)DZ_CASES[i].den;
      outS[i] = outD[i] = 0.0;
   }

   retCode = TA_S_DIV( 0, DZ_N - 1, num, den, &begIdx, &nbElement, outS );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != DZ_N )
   {
      printf( "\nTA_S_DIV: retCode=%d outBegIdx=%d outNBElement=%d\n",
              retCode, begIdx, nbElement );
      return TA_DIVZERO_BAD_RETCODE;
   }

   {
      double dNum[DZ_N], dDen[DZ_N];
      for( i = 0; i < DZ_N; i++ ) { dNum[i] = (double)num[i]; dDen[i] = (double)den[i]; }
      retCode = TA_DIV( 0, DZ_N - 1, dNum, dDen, &begIdx, &nbElement, outD );
      if( retCode != TA_SUCCESS )
      {
         printf( "\nTA_DIV (widened control): retCode=%d\n", retCode );
         return TA_DIVZERO_BAD_RETCODE;
      }
   }

   for( i = 0; i < DZ_N; i++ )
   {
      if( !dz_matches( outS[i], &DZ_CASES[i] ) )
         return dz_report( "TA_S_", i, outS[i] );
      if( !dz_same_bits( outS[i], outD[i] ) )
      {
         printf( "\nDIV case %d (%s): TA_S_ %.17g differs bitwise from TA_ %.17g\n",
                 i, DZ_CASES[i].what, outS[i], outD[i] );
         return TA_DIVZERO_BAD_VALUE;
      }
   }

   return TA_TEST_PASS;
}

/* (e) The streaming tier, on both loops the generator emits for it.
 *
 *  - `_OpenImpl` is the warm-up fill: its own transcription of the batch body.
 *    OpenAndFill runs the WHOLE table through it, and TA_DIV_Open takes the
 *    same history through the stride-0 sink and must hand back the last row.
 *  - `_StepImpl` is the per-bar loop. A second handle opens on bar 0 and walks
 *    the rest a bar at a time.
 *
 * Keeping both is not belt and braces: an `_OpenImpl`-only guard on a zero
 * divisor was measured to survive every other assertion in this file.
 */
static ErrorNumber dz_stream( void )
{
   double           num[DZ_N], den[DZ_N], filled[DZ_N];
   double           value, peeked;
   TA_DIV_Stream   *stream = NULL;
   TA_RetCode       retCode;
   int              outBegIdx, outNbElement, i;
   ErrorNumber      errNb = TA_TEST_PASS;

   for( i = 0; i < DZ_N; i++ )
   {
      num[i] = DZ_CASES[i].num;
      den[i] = DZ_CASES[i].den;
      filled[i] = 0.0;
   }

   /* --- the fill loop, every row --- */
   retCode = TA_DIV_OpenAndFill( &stream, num, den, DZ_N,
                                 &outBegIdx, &outNbElement, filled );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTA_DIV_OpenAndFill: a zero divisor must not be an error, got retCode=%d\n",
              retCode );
      return TA_DIVZERO_BAD_RETCODE;
   }
   if( outBegIdx != 0 || outNbElement != DZ_N )
   {
      printf( "\nTA_DIV_OpenAndFill: outBegIdx=%d outNBElement=%d\n",
              outBegIdx, outNbElement );
      errNb = TA_DIVZERO_BAD_SHAPE;
      goto done;
   }
   for( i = 0; i < DZ_N; i++ )
   {
      if( !dz_matches( filled[i], &DZ_CASES[i] ) )
      {
         errNb = dz_report( "stream OpenAndFill", i, filled[i] );
         goto done;
      }
   }
   TA_DIV_Close( stream );
   stream = NULL;

   /* Same loop, stride 0: only the last row survives, and it must be that row
    * rather than whatever the sink was initialised to. */
   value = 0.0;
   retCode = TA_DIV_Open( &stream, num, den, DZ_N, &value );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTA_DIV_Open: a zero divisor must not be an error, got retCode=%d\n", retCode );
      return TA_DIVZERO_BAD_RETCODE;
   }
   if( !dz_matches( value, &DZ_CASES[DZ_N - 1] ) )
   {
      errNb = dz_report( "stream Open", DZ_N - 1, value );
      goto done;
   }
   TA_DIV_Close( stream );
   stream = NULL;

   /* --- the step loop, every row --- */
   value = 0.0;
   retCode = TA_DIV_Open( &stream, num, den, 1, &value );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTA_DIV_Open(1 bar): retCode=%d\n", retCode );
      return TA_DIVZERO_BAD_RETCODE;
   }
   if( !dz_matches( value, &DZ_CASES[0] ) )
   {
      errNb = dz_report( "stream Open(1 bar)", 0, value );
      goto done;
   }

   for( i = 1; i < DZ_N; i++ )
   {
      peeked = 0.0;
      retCode = TA_DIV_Peek( stream, num[i], den[i], &peeked );
      if( retCode != TA_SUCCESS )
      {
         printf( "\nTA_DIV_Peek case %d (%s): retCode=%d\n", i, DZ_CASES[i].what, retCode );
         errNb = TA_DIVZERO_BAD_RETCODE;
         goto done;
      }
      if( !dz_matches( peeked, &DZ_CASES[i] ) )
      {
         errNb = dz_report( "stream Peek", i, peeked );
         goto done;
      }

      value = 0.0;
      retCode = TA_DIV_Update( stream, num[i], den[i], &value );
      if( retCode != TA_SUCCESS )
      {
         printf( "\nTA_DIV_Update case %d (%s): retCode=%d\n", i, DZ_CASES[i].what, retCode );
         errNb = TA_DIVZERO_BAD_RETCODE;
         goto done;
      }
      if( !dz_matches( value, &DZ_CASES[i] ) )
      {
         errNb = dz_report( "stream Update", i, value );
         goto done;
      }

      /* Same tier, run twice: strictly bitwise, so the NaN rows assert a
       * payload the table deliberately does not name. */
      if( !dz_same_bits( peeked, value ) )
      {
         printf( "\nDIV case %d (%s): Peek %.17g differs bitwise from Update %.17g\n",
                 i, DZ_CASES[i].what, peeked, value );
         errNb = TA_DIVZERO_BAD_VALUE;
         goto done;
      }
   }

   /* The handle knows how many bars it has consumed -- the one
    * thing no value comparison can see. Rust, Java and C# assert it too. */
   g_dzChecked++;
   outBegIdx = outNbElement = -1;
   retCode = TA_StreamOutRange( stream, &outBegIdx, &outNbElement );
   if( retCode != TA_SUCCESS || outBegIdx != 0 || outNbElement != DZ_N )
   {
      printf( "\nTA_DIV stream OutRange: retCode=%d begIdx=%d count=%d, expected 0/%d\n",
              retCode, outBegIdx, outNbElement, DZ_N );
      errNb = TA_DIVZERO_BAD_SHAPE;
      goto done;
   }

   /* The control: the tier's own contract is unchanged, so "a zero divisor is
    * not rejected" cannot be read as "nothing is rejected". */
   {
      static const double bad[3] = { (double)NAN, (double)INFINITY, -(double)INFINITY };
      int b, slot;
      for( b = 0; b < 3; b++ )
      {
         for( slot = 0; slot < 2; slot++ )
         {
            double a0 = slot == 0 ? bad[b] : 1.0;
            double a1 = slot == 0 ? 1.0 : bad[b];

            /* Peek as well as Update: they carry the guard separately, so a
             * control on one of them leaves the other's removable. */
            value = 0.0;
            retCode = TA_DIV_Peek( stream, a0, a1, &value );
            if( !dz_rejected( retCode ) )
            {
               printf( "\nTA_DIV_Peek accepted a non-finite bar (%g, slot %d): retCode=%d\n",
                       bad[b], slot, retCode );
               errNb = TA_DIVZERO_BAD_RETCODE;
               goto done;
            }

            value = 0.0;
            retCode = TA_DIV_Update( stream, a0, a1, &value );
            if( !dz_rejected( retCode ) )
            {
               printf( "\nTA_DIV_Update accepted a non-finite bar (%g, slot %d): retCode=%d\n",
                       bad[b], slot, retCode );
               errNb = TA_DIVZERO_BAD_RETCODE;
               goto done;
            }
         }
      }
   }

done:
   TA_DIV_Close( stream );
   return errNb;
}

/* (f) The other half of the #191 exemption: DIV must SAY it can write a
 * non-finite value. Everything above is why that is true. */
static ErrorNumber dz_declares_nan_inf_out( void )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo   *funcInfo;
   TA_RetCode           retCode;

   retCode = TA_GetFuncHandle( "DIV", &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTA_GetFuncHandle(DIV) failed [%d]\n", retCode );
      return TA_DIVZERO_BAD_RETCODE;
   }
   retCode = TA_GetFuncInfo( handle, &funcInfo );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nTA_GetFuncInfo(DIV) failed [%d]\n", retCode );
      return TA_DIVZERO_BAD_RETCODE;
   }
   if( !(funcInfo->flags & TA_FUNC_FLG_NAN_INF_OUT) )
   {
      printf( "\nDIV does not declare TA_FUNC_FLG_NAN_INF_OUT, yet writes NaN "
              "and +/-Inf on a zero divisor (issue #191/#249)\n" );
      return TA_DIVZERO_FLAG_MISSING;
   }
   return TA_TEST_PASS;
}

ErrorNumber test_func_div_zero( TA_History *history )
{
   /* Inputs are the synthetic sign table; the reference history is unused. */
   ErrorNumber errNb;
   int         expected;

   (void)history;

   g_dzChecked = 0;

   errNb = dz_declares_nan_inf_out();
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Full range, then a sub-range that starts inside the +/-Inf block: the
    * outputs must shift with startIdx rather than being recomputed from 0. */
   errNb = dz_batch_range( 0, DZ_N - 1 );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = dz_batch_range( 4, 7 );
   if( errNb != TA_TEST_PASS ) return errNb;

   errNb = dz_single_precision();
   if( errNb != TA_TEST_PASS ) return errNb;

   errNb = dz_stream();
   if( errNb != TA_TEST_PASS ) return errNb;

   expected = DZ_N                /* batch, full range                       */
            + 4                   /* batch, sub-range                        */
            + 2 * DZ_N            /* TA_S_: value, then bits vs TA_          */
            + DZ_N + 1            /* stream OpenAndFill, then Open's last    */
            + 1                   /* the step handle's own open              */
            + 3 * (DZ_N - 1)      /* Peek, Update, and the two bitwise equal */
            + 1                   /* the handle's committed range            */
            + 12;                 /* Peek and Update reject 3 values x 2 slots*/
   if( g_dzChecked != expected )
   {
      printf( "\nDIV zero-divisor: compared %d value(s), expected %d\n",
              g_dzChecked, expected );
      return TA_DIVZERO_VACUOUS;
   }
   /* `expected` moves with DZ_N, so on its own it would keep passing over a
    * table half its size -- the derived-floor trap ta_test_legacy.c's literal
    * LEGACY_FLOOR_* exists to avoid. These two do not move. */
   if( DZ_N < DZ_FLOOR_CASES || g_dzChecked < DZ_FLOOR_CHECKS )
   {
      printf( "\nDIV zero-divisor: %d case(s) / %d check(s), floor is %d / %d\n",
              DZ_N, g_dzChecked, DZ_FLOOR_CASES, DZ_FLOOR_CHECKS );
      return TA_DIVZERO_VACUOUS;
   }

   return TA_TEST_PASS;
}
