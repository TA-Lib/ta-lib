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
 *  101026 MF,CC  First version (#540).
 */

/* Description:
 *
 *   Rule rD2 between machines: every function flagged
 *   TA_FUNC_FLG_USES_TRANSCENDENTAL, at its defaults over the 252-bar series,
 *   against values committed once, at 1e-9 (relative above a magnitude of 1,
 *   absolute below). No machine is compared with another: each one is held to
 *   the same table.
 *
 *   A flagged function with an output that has no row fails, so a new one
 *   brings its rows:
 *   TA_TRANSCENDENTAL_REF=print writes the table, to be committed from one
 *   machine and then run on the others before it is trusted.
 *   TA_TRANSCENDENTAL_REF=measure prints the largest deviation instead of
 *   judging it.
 *
 *   A Math Transform function reads close/256, which stays inside the domain
 *   of all of them and off tanh's plateau; dividing by a power of two keeps
 *   the input the same bits everywhere.
 *
 *   SERVER_VERIFY: none. The sweep already sends these calls on this series.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "test_codegen.h"

#define TR_N        252
#define TR_MAX_OUT  4
#define TR_STEP     16

typedef struct { const char *name; int output; int bar; double value; } TrRef;
static const TrRef trRef[] = {
#include "test_transcendental_ref_data.h"
   { NULL, 0, 0, 0.0 }
};

static const TA_History *trHistory;
static double trScaled[TR_N];
static double trReal[TR_MAX_OUT][TR_N];
static int    trInt[TR_MAX_OUT][TR_N];
static int    trMode;              /* 0 check, 1 print, 2 measure */
static int    trNbFunc, trNbCmp, trNbFail;
static double trWorst;
static char   trWorstAt[96];

static void trOneFunction( const TA_FuncInfo *funcInfo, void *opaque )
{
   TA_ParamHolder *h;
   const TA_InputParameterInfo *in;
   const TA_OutputParameterInfo *out;
   const TrRef *r;
   TA_RetCode rc;
   unsigned int i;
   int beg = 0, nb = 0, bar, isReal[TR_MAX_OUT];
   unsigned int outSeen = 0;
   int math = !strcmp( funcInfo->group, "Math Transform" );

   (void)opaque;
   if( !(funcInfo->flags & TA_FUNC_FLG_USES_TRANSCENDENTAL) ) return;
   trNbFunc++;

   rc = funcInfo->nbOutput > TR_MAX_OUT ? TA_BAD_PARAM : TA_ParamHolderAlloc( funcInfo->handle, &h );
   if( rc != TA_SUCCESS ) h = NULL;
   for( i=0; i < funcInfo->nbInput && rc == TA_SUCCESS; i++ )
   {
      TA_GetInputParameterInfo( funcInfo->handle, i, &in );
      if( in->type == TA_Input_Price )
         rc = TA_SetInputParamPricePtr( h, i, trHistory->open, trHistory->high, trHistory->low,
                                        trHistory->close, trHistory->volume, trHistory->openInterest );
      else if( in->type == TA_Input_Real )
         rc = TA_SetInputParamRealPtr( h, i, math ? trScaled : trHistory->close );
      else
         rc = TA_BAD_PARAM;
   }
   for( i=0; i < funcInfo->nbOutput && rc == TA_SUCCESS; i++ )
   {
      TA_GetOutputParameterInfo( funcInfo->handle, i, &out );
      isReal[i] = out->type == TA_Output_Real;
      rc = isReal[i] ? TA_SetOutputParamRealPtr( h, i, trReal[i] )
                     : TA_SetOutputParamIntegerPtr( h, i, trInt[i] );
   }
   if( rc == TA_SUCCESS ) rc = TA_CallFunc( h, 0, TR_N-1, &beg, &nb );
   if( h ) TA_ParamHolderFree( h );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "\n  transcendental reference: %s: call failed (%d), %d bars", funcInfo->name, (int)rc, nb );
      trNbFail++;
      return;
   }

   if( trMode == 1 )
   {
      for( i=0; i < funcInfo->nbOutput; i++ )
         for( bar=beg; bar < beg+nb; bar++ )
            if( (bar-beg) % TR_STEP == 0 || bar == beg+nb-1 )
               printf( "\n   { \"%s\", %u, %d, %.17g },", funcInfo->name, i, bar,
                       isReal[i] ? trReal[i][bar-beg] : (double)trInt[i][bar-beg] );
      return;
   }

   for( r=trRef; r->name; r++ )
   {
      double got, d, limit;
      if( strcmp( r->name, funcInfo->name ) ) continue;
      if( r->output < 0 || r->output >= (int)funcInfo->nbOutput || r->bar < beg || r->bar >= beg+nb )
      {
         printf( "\n  transcendental reference: %s output %d bar %d: outside the call's range %d..%d",
                 r->name, r->output, r->bar, beg, beg+nb-1 );
         trNbFail++;
         continue;
      }
      got = isReal[r->output] ? trReal[r->output][r->bar-beg] : (double)trInt[r->output][r->bar-beg];
      outSeen |= 1u << r->output;
      limit = CODEGEN_TRANSCENDENTAL_TOL * fmax( 1.0, fabs( r->value ) );
      d = fabs( got - r->value );
      trNbCmp++;
      if( d / fmax( 1.0, fabs( r->value ) ) > trWorst )
      {
         trWorst = d / fmax( 1.0, fabs( r->value ) );
         snprintf( trWorstAt, sizeof trWorstAt, "%s output %d bar %d", r->name, r->output, r->bar );
      }
      if( trMode == 0 && !(d <= limit) )
      {
         if( trNbFail < 20 )
            printf( "\n  transcendental reference: %s output %d bar %d: %.17g, committed %.17g, apart by %.3e (limit %.3e)",
                    r->name, r->output, r->bar, got, r->value, d, limit );
         trNbFail++;
      }
   }
   if( outSeen != (1u << funcInfo->nbOutput) - 1 )
   {
      printf( "\n  transcendental reference: %s is flagged and has an output with no committed value "
              "(TA_TRANSCENDENTAL_REF=print writes the rows)", funcInfo->name );
      trNbFail++;
   }
}

ErrorNumber test_func_transcendental_ref( TA_History *history )
{
   const char *mode = getenv( "TA_TRANSCENDENTAL_REF" );
   unsigned int savedUnst[TA_FUNC_UNST_COUNT];
   unsigned int u;
   int i, nbRow = 0;

   if( history->nbBars != TR_N ) return TA_TRANSCENDENTAL_REF_FAIL;

   trMode = !mode ? 0 : !strcmp( mode, "print" ) ? 1 : 2;
   trHistory = history;
   trNbFunc = trNbCmp = trNbFail = 0;
   trWorst = 0.0;
   trWorstAt[0] = 0;
   for( i=0; i < TR_N; i++ ) trScaled[i] = history->close[i] / 256.0;
   while( trRef[nbRow].name ) nbRow++;

   for( u=0; u < TA_FUNC_UNST_COUNT; u++ )
   {
      savedUnst[u] = TA_GetUnstablePeriod( (TA_FuncUnstId)u );
      TA_SetUnstablePeriod( (TA_FuncUnstId)u, 0 );
   }
   TA_ForEachFunc( trOneFunction, NULL );
   for( u=0; u < TA_FUNC_UNST_COUNT; u++ )
      TA_SetUnstablePeriod( (TA_FuncUnstId)u, savedUnst[u] );

   if( trMode == 1 ) { printf( "\n" ); return TA_TEST_PASS; }
   if( trMode == 2 )
      printf( "\n  transcendental reference: %d values, largest deviation %.3e (%s)\n", trNbCmp, trWorst, trWorstAt );
   if( trNbFail ) return TA_TRANSCENDENTAL_REF_FAIL;
   /* A row whose function is gone or lost its flag is compared by no one. */
   if( trNbFunc < 30 || trNbCmp != nbRow )
   {
      printf( "\n  transcendental reference: vacuous: %d flagged functions, %d of %d rows compared\n",
              trNbFunc, trNbCmp, nbRow );
      return TA_TRANSCENDENTAL_REF_VACUOUS;
   }
   return TA_TEST_PASS;
}
