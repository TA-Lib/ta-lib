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
 *  FI       Fernando J. Iglesias García (github @iglesias)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070726 FI,CC  First version. Regression guard for the single-precision
 *                (TA_S_) vector-arithmetic overflow reported in PR #33.
 */

/* Description:
 *
 * The single-precision variants of the vector-arithmetic operators take
 * float inputs but write a *double* output (const float in -> double out).
 * They must therefore perform the arithmetic in double: casting the first
 * operand to double before the operation widens the whole expression, so a
 * product/sum/difference/quotient that exceeds the float range (FLT_MAX ~=
 * 3.4e38) is representable in the double output instead of collapsing to
 * +/-inf.
 *
 * The pre-fix code multiplied/added in float and only widened the *result*
 * (e.g. `outReal[i] = (double)(inReal0[i]*inReal1[i]);`), so `3e38f * 10f`
 * overflowed to inf before the widening cast. PR #33 (@iglesias) reported
 * this for MULT; the same class affects ADD, SUB and DIV.
 *
 * This test pins the fix for the whole family: for each operator it feeds
 * float inputs whose exact-precision result overflows float range, and
 * requires the TA_S_ output to be finite and bit-equal to the same
 * operation carried out in double. It also confirms the double-input
 * variant (TA_ADD/TA_SUB/TA_MULT/TA_DIV) yields the same value, i.e. the
 * single-precision variant now matches the double one on this input.
 */

#include <stdio.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_libc.h"

typedef TA_RetCode (*SFloatFunc)( int, int, const float [], const float [],
                                  int *, int *, double [] );
typedef TA_RetCode (*SDoubleFunc)( int, int, const double [], const double [],
                                   int *, int *, double [] );

typedef struct
{
   const char  *name;
   SFloatFunc   sFunc;   /* TA_S_<OP>: float in,  double out */
   SDoubleFunc  dFunc;   /* TA_<OP>:   double in, double out */
   char         op;      /* '+' '-' '*' '/'                  */
   float        a;
   float        b;
} SOvfCase;

static double applyOp( char op, double a, double b )
{
   switch( op )
   {
   case '+': return a + b;
   case '-': return a - b;
   case '*': return a * b;
   case '/': return a / b;
   default:  return 0.0;
   }
}

static ErrorNumber runCase( const SOvfCase *c )
{
   float  inF0[1], inF1[1];
   double inD0[1], inD1[1];
   double outS[1], outD[1];
   int    begIdx = 0, nbElement = 0;
   double expected;
   TA_RetCode rc;

   /* Exact-precision result, computed in double from the float operands. */
   expected = applyOp( c->op, (double)c->a, (double)c->b );

   /* --- single-precision variant (the one PR #33 fixes) --- */
   inF0[0] = c->a;
   inF1[0] = c->b;
   outS[0] = -1.0;
   rc = c->sFunc( 0, 0, inF0, inF1, &begIdx, &nbElement, outS );
   if( rc != TA_SUCCESS || nbElement != 1 )
   {
      printf( "  TA_S_%s: bad retCode=%d nbElement=%d\n", c->name, rc, nbElement );
      return TA_S_OVERFLOW_BAD_RETCODE;
   }
   if( !isfinite( outS[0] ) )
   {
      printf( "  TA_S_%s: non-finite output %g for %g %c %g "
              "(float arithmetic overflowed before widening)\n",
              c->name, outS[0], (double)c->a, c->op, (double)c->b );
      return TA_S_OVERFLOW_NOT_FINITE;
   }
   if( fabs( outS[0] - expected ) > fabs( expected ) * 1e-9 )
   {
      printf( "  TA_S_%s: value %.17g != expected %.17g\n",
              c->name, outS[0], expected );
      return TA_S_OVERFLOW_WRONG_VALUE;
   }

   /* --- double-precision variant: must produce the same finite value --- */
   inD0[0] = (double)c->a;
   inD1[0] = (double)c->b;
   outD[0] = -1.0;
   rc = c->dFunc( 0, 0, inD0, inD1, &begIdx, &nbElement, outD );
   if( rc != TA_SUCCESS || nbElement != 1 )
   {
      printf( "  TA_%s: bad retCode=%d nbElement=%d\n", c->name, rc, nbElement );
      return TA_S_OVERFLOW_BAD_RETCODE;
   }
   if( !isfinite( outD[0] ) || fabs( outD[0] - expected ) > fabs( expected ) * 1e-9 )
   {
      printf( "  TA_%s: value %.17g != expected %.17g\n",
              c->name, outD[0], expected );
      return TA_S_OVERFLOW_WRONG_VALUE;
   }
   if( outS[0] != outD[0] )
   {
      printf( "  %s: single-precision %.17g != double-precision %.17g\n",
              c->name, outS[0], outD[0] );
      return TA_S_OVERFLOW_WRONG_VALUE;
   }

   return TA_TEST_PASS;
}

ErrorNumber test_func_s_overflow( TA_History *history )
{
   /* Inputs are synthetic extremes; the reference history is unused. */
   unsigned int i;
   ErrorNumber  errNb;

   /* Each operand pair is representable as float, but its exact result
    * exceeds FLT_MAX (~3.4e38), so the pre-fix float arithmetic returned
    * inf. Mirrors the values used in the red/green verification. */
   static const SOvfCase cases[] =
   {
      { "ADD",  TA_S_ADD,  TA_ADD,  '+', 3.0e38f,  3.0e38f },  /* 6e38  */
      { "SUB",  TA_S_SUB,  TA_SUB,  '-', 3.0e38f, -3.0e38f },  /* 6e38  */
      { "MULT", TA_S_MULT, TA_MULT, '*', 3.0e38f,  10.0f   },  /* 3e39  (PR #33) */
      { "DIV",  TA_S_DIV,  TA_DIV,  '/', 3.0e38f,  1.0e-3f }   /* 3e41  */
   };

   (void)history;

   printf( "Testing single-precision (TA_S_) arithmetic overflow (PR #33)\n" );

   for( i = 0; i < sizeof(cases)/sizeof(cases[0]); i++ )
   {
      errNb = runCase( &cases[i] );
      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}
