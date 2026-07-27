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
 *  072626 MF,CC  First version. Targeted MAVP grouping-path legs with a
 *                per-bar TA_MA oracle and end-truncation pins (#143).
 *  072726 MF,CC  #145. Iterate the generated MAType list instead of a
 *                hand-kept one, and verify the unstable legs cross-language.
 */

/* Description:
 *
 * Targeted MAVP tests. MAVP's implementation groups outputs by their
 * clamped per-bar period and runs one bounded MA pass per distinct
 * period, so its internal paths depend on the SHAPE of the period
 * series (how many distinct periods, whether each period's outputs are
 * contiguous, where a period is last used). The generic sweeps do not
 * control that shape: the codegen sweep's volume-like period input
 * clamps every bar to maxPeriod (one distinct period) and the variant
 * gate's 2+(k%25) ramp interleaves everything (no contiguous runs).
 * This test drives each shape deliberately:
 *
 *  - Per-bar oracle: MAVP output at bar k must bit-equal TA_MA of that
 *    bar's clamped period at bar k. This is MAVP's definition, so it
 *    holds for ANY implementation, and it fails loudly if a bounded
 *    pass is cut short, a group is mis-assembled, or a copy is missed.
 *    Along the way it asserts TA_MA(outBegIdx) == MAVP(outBegIdx) for
 *    every period used — the ma_lookback monotonicity contract MAVP's
 *    output indexing relies on.
 *  - End-truncation pins: shrinking endIdx must not change any earlier
 *    output (every MA fills forward). MAVP's bounded per-period passes
 *    are only bit-safe because of this property, so it is pinned here
 *    as a public-API contract, for every MAType routed through ma().
 *  - Shapes: single distinct period (via data, via clamping from both
 *    sides, and via minPeriod==maxPeriod), contiguous blocks, strictly
 *    interleaved periods, ascending/descending staircases (descending
 *    puts the largest period's last use earliest), sawtooth, fractional
 *    period values (the (int) cast), period=1 legs (minPeriod=1), a
 *    wide sparse period range, subrange and single-bar calls, and the
 *    empty-output case (lookback beyond the data).
 *  - Each shape also runs in-place (outReal == inReal) and, at a
 *    default unstable period and a non-zero one, plus a TA_S_MAVP
 *    spot-check against a per-bar TA_S_MA oracle.
 *
 * When --codegen is active every successful full-range call is also
 * verified against the language servers (C, Rust, Java, .NET) through
 * server_verify, extending every shape cross-language bitwise — at the
 * default unstable period and at a non-zero one, which server_verify
 * forwards to each server.
 */

/**** Headers ****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "server_verify.h"
#include "ta_func_unguarded.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#define MV_DATA_SIZE 252   /* Daily reference data size. */
#define MV_BUF_SIZE  280

/* Every MAType routed through ma(), read at run time from ta_abstract's
 * MAType list rather than spelled out here: that list is generated from
 * ta_codegen/input/enums.yaml (backends/ta_abstract_c.rs emits
 * TA_MA_TypeDataPair straight from the enum variants), so an appended
 * MAType is exercised by this file without editing it (#145).
 *
 * Today the list covers window-slide (SMA/WMA/TRIMA), recursive
 * (EMA/DEMA/TEMA/KAMA), the period-ignoring path (MAMA), the
 * long-lookback cascades (T3/HMA — T3 goes empty on the wide-range
 * shapes, exercising the empty contract) and the period-independent
 * identity copy (DISABLED).
 */
#define MV_MAX_TYPES 32
#define MV_MIN_TYPES 11   /* non-vacuity floor: the count when this was written */
static TA_MAType mvTypes[MV_MAX_TYPES];
static int       mvNbTypes;

static TA_Real mvPeriods[MV_BUF_SIZE];
static TA_Real mvOut[MV_BUF_SIZE];
static TA_Real mvOutCut[MV_BUF_SIZE];
static TA_Real mvInplace[MV_BUF_SIZE];
static TA_Real mvMA[MV_BUF_SIZE];
static float   mvPriceS[MV_BUF_SIZE];
static float   mvPeriodsS[MV_BUF_SIZE];

/**** Local functions declarations.    ****/
static ErrorNumber mvLoadTypes( void );
static ErrorNumber mvUnguardedBoundCheck( void );
static ErrorNumber mvOracleCheck( const char *label,
                                  const TA_History *history,
                                  const TA_Real *periods,
                                  int minPeriod, int maxPeriod,
                                  TA_MAType maType, int unstable,
                                  int startIdx, int endIdx );
static ErrorNumber mvTruncationCheck( const char *label,
                                      const TA_History *history,
                                      const TA_Real *periods,
                                      int minPeriod, int maxPeriod,
                                      TA_MAType maType );
static ErrorNumber mvSinglePrecisionCheck( const char *label,
                                           const TA_History *history,
                                           const TA_Real *periods,
                                           int minPeriod, int maxPeriod,
                                           TA_MAType maType );
static ErrorNumber mvRunShapeMatrix( const char *label,
                                     const TA_History *history,
                                     int minPeriod, int maxPeriod );

/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
ErrorNumber test_func_mavp( TA_History *history )
{
   ErrorNumber errNb;
   TA_RetCode retCode;
   TA_Integer outBegIdx, outNbElement;
   int i, endIdx;

   if( history->nbBars < MV_DATA_SIZE )
   {
      printf( "\nFail: MAVP tests need %d bars\n", MV_DATA_SIZE );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   endIdx = MV_DATA_SIZE - 1;

   errNb = mvLoadTypes();
   if( errNb != TA_TEST_PASS ) return errNb;

   /* These tests assume pristine global settings. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   /* Shape: one distinct period, straight from the data. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = 12.0;
   errNb = mvRunShapeMatrix( "const-mid", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: one distinct period reached by clamping up (data below min). */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = 0.25;
   errNb = mvRunShapeMatrix( "const-clamp-up", history, 5, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: one distinct period reached by clamping down (data above max,
    * the layout the generic codegen sweep always produces). */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = 500.0;
   errNb = mvRunShapeMatrix( "const-clamp-down", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: two contiguous blocks (block copies, and a hole of unused
    * periods between the two). The boundary sits at bar 200 so it stays
    * past every type's lookback at maxPeriod 30 — a boundary below the
    * lookback would leave only one block visible and silently degrade
    * the shape to the single-period fast path for that type. (The copy
    * branches are the same generated code for every MAType, so one
    * degraded type would not lose branch coverage, only shape variety.) */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = ( i < 200 ) ? 8.0 : 21.0;
   errNb = mvRunShapeMatrix( "two-blocks", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: two strictly interleaved periods (nothing contiguous). */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = ( i % 2 ) ? 9.0 : 17.0;
   errNb = mvRunShapeMatrix( "alternating", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: ascending staircase (every period one contiguous block). */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 2 + i/21 );
   errNb = mvRunShapeMatrix( "staircase-up", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: descending staircase — the largest period's last use is the
    * EARLIEST bar, so its bounded pass is the shortest. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 13 - i/21 );
   errNb = mvRunShapeMatrix( "staircase-down", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: sawtooth — every period recurs to the end of the range. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 2 + i % 12 );
   errNb = mvRunShapeMatrix( "sawtooth", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: fractional period values — the (int) cast must truncate. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = 3.0 + (TA_Real)( i % 9 ) + ( ( i % 2 ) ? 0.99 : 0.25 );
   errNb = mvRunShapeMatrix( "fractional", history, 2, 30 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: minPeriod == maxPeriod forces one period whatever the data. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = 7.3 + (TA_Real)( i % 40 );
   errNb = mvRunShapeMatrix( "min-eq-max", history, 15, 15 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: period=1 legs inside the grouping (minPeriod=1). */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 1 + i % 13 );
   errNb = mvRunShapeMatrix( "period-one", history, 1, 13 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Shape: wide sparse range — many distinct periods, few bars each. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 2 + ( i * 7 ) % 99 );
   errNb = mvRunShapeMatrix( "wide-100", history, 2, 100 );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Boundary calls (SMA): single-bar range, subrange, empty output. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 2 + i/21 );
   errNb = mvOracleCheck( "single-bar", history, mvPeriods, 2, 30,
                          TA_MAType_SMA, 0, endIdx, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;

   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = ( i % 2 ) ? 9.0 : 17.0;
   errNb = mvOracleCheck( "subrange", history, mvPeriods, 2, 30,
                          TA_MAType_SMA, 0, 100, 200 );
   if( errNb != TA_TEST_PASS ) return errNb;

   errNb = mvOracleCheck( "empty-output", history, mvPeriods, 2, 5000,
                          TA_MAType_SMA, 0, 0, endIdx );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* An inverted window (minPeriod > maxPeriod) must be rejected cleanly. */
   outBegIdx = outNbElement = -1;
   retCode = TA_MAVP( 0, endIdx, history->close, mvPeriods, 30, 2,
                      TA_MAType_SMA, &outBegIdx, &outNbElement, mvOut );
   if( retCode != TA_BAD_PARAM || outBegIdx != 0 || outNbElement != 0 )
   {
      printf( "\nFail: MAVP(min=30,max=2): rc=%d beg=%d nb=%d, expected clean TA_BAD_PARAM\n",
              (int)retCode, (int)outBegIdx, (int)outNbElement );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* End-truncation pins for every MA type (sawtooth shape). */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 2 + i % 12 );
   for( i = 0; i < mvNbTypes; i++ )
   {
      char label[64];
      snprintf( label, sizeof(label), "truncation maType=%d", (int)mvTypes[i] );
      errNb = mvTruncationCheck( label, history, mvPeriods, 2, 30, mvTypes[i] );
      if( errNb != TA_TEST_PASS ) return errNb;
   }

   /* TA_S_ spot-checks on the two shapes with the most distinct paths. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = ( i < 200 ) ? 8.0 : 21.0;
   errNb = mvSinglePrecisionCheck( "TA_S two-blocks SMA", history, mvPeriods,
                                   2, 30, TA_MAType_SMA );
   if( errNb != TA_TEST_PASS ) return errNb;
   errNb = mvSinglePrecisionCheck( "TA_S two-blocks EMA", history, mvPeriods,
                                   2, 30, TA_MAType_EMA );
   if( errNb != TA_TEST_PASS ) return errNb;

   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = 12.0;
   errNb = mvSinglePrecisionCheck( "TA_S const SMA", history, mvPeriods,
                                   2, 30, TA_MAType_SMA );
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Off-contract unguarded periods must stay bounded (#145). */
   errNb = mvUnguardedBoundCheck();
   if( errNb != TA_TEST_PASS ) return errNb;

   /* Leave globals as found. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );

   printf( "\n  MAVP grouping: %d MAType(s) from the generated ta_abstract list,"
           " each shape at unstable 0 and 7%s\n", mvNbTypes,
           server_verify_active() ? " (both cross-language)" : "" );

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* Fill mvTypes from ta_abstract's generated MAType list, found by name on
 * MAVP's own optInMAType parameter (not by a hardcoded parameter index).
 */
static ErrorNumber mvLoadTypes( void )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo   *funcInfo;
   const TA_OptInputParameterInfo *optInfo;
   const TA_IntegerList *list = NULL;
   unsigned int i;

   if( TA_GetFuncHandle( "MAVP", &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "\nFail: MAVP: cannot reach ta_abstract metadata\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   for( i = 0; i < funcInfo->nbOptInput; i++ )
   {
      if( TA_GetOptInputParameterInfo( handle, i, &optInfo ) != TA_SUCCESS )
         continue;
      if( optInfo->type == TA_OptInput_IntegerList &&
          strcmp( optInfo->paramName, "optInMAType" ) == 0 )
      {
         list = (const TA_IntegerList *)optInfo->dataSet;
         break;
      }
   }
   if( list == NULL || list->data == NULL )
   {
      printf( "\nFail: MAVP: no optInMAType integer list in ta_abstract\n" );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* Floor, not an equality: the count must never SHRINK (that would make
    * every matrix below quietly thinner), but a newly appended MAType is
    * meant to be picked up here with no edit. */
   if( list->nbElement < MV_MIN_TYPES || list->nbElement > MV_MAX_TYPES )
   {
      printf( "\nFail: MAVP: MAType list has %u entries, expected %d..%d\n",
              list->nbElement, MV_MIN_TYPES, MV_MAX_TYPES );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   for( i = 0; i < list->nbElement; i++ )
      mvTypes[i] = (TA_MAType)list->data[i].value;
   mvNbTypes = (int)list->nbElement;

   return TA_TEST_PASS;
}

/* Issue #145: the bucket table must stay bounded on an OFF-CONTRACT unguarded
 * call carrying a near-INT_MAX period.
 *
 * ma_lookback() returns 0 for TA_MAType_DISABLED at any period, so a huge
 * optInMaxPeriod does NOT push startIdx past endIdx — a handful of bars is
 * enough to reach the allocation. That is the one path where the pre-#145
 * `malloc((maxUsed+2) * sizeof(int))` evaluated 2147483646+2 in signed int
 * (UBSan: "signed integer overflow ... cannot be represented in type 'int'"),
 * and where merely-huge periods asked for a multi-GB table.
 */
static ErrorNumber mvUnguardedBoundCheck( void )
{
   TA_RetCode retCode;
   TA_Integer outBegIdx, outNbElement;
   int i;
   const int huge = 2147483646;   /* INT_MAX-1 */

   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvInplace[i] = 100.0 + (TA_Real)i;

   /* Widest possible spread: the table is refused, not overflowed. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = ( i % 2 ) ? 1.0 : (TA_Real)huge;
   outBegIdx = outNbElement = -1;
   retCode = TA_MAVP_Unguarded( 0, MV_DATA_SIZE-1, mvInplace, mvPeriods,
                                1, huge, TA_MAType_DISABLED,
                                &outBegIdx, &outNbElement, mvOut );
   if( retCode != TA_BAD_PARAM || outBegIdx != 0 || outNbElement != 0 )
   {
      printf( "\nFail: MAVP unguarded wide spread: rc=%d beg=%d nb=%d,"
              " expected clean TA_BAD_PARAM\n",
              (int)retCode, (int)outBegIdx, (int)outNbElement );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* Same huge period on every bar: the spread is 0, so relative indexing
    * makes this a two-int table and an ordinary success — the absolute
    * indexing it replaced would have asked for ~8GB here. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)huge;
   outBegIdx = outNbElement = -1;
   retCode = TA_MAVP_Unguarded( 0, MV_DATA_SIZE-1, mvInplace, mvPeriods,
                                huge, huge, TA_MAType_DISABLED,
                                &outBegIdx, &outNbElement, mvOut );
   if( retCode != TA_SUCCESS || outBegIdx != 0 || outNbElement != MV_DATA_SIZE )
   {
      printf( "\nFail: MAVP unguarded narrow huge band: rc=%d beg=%d nb=%d,"
              " expected the identity copy over %d bars\n",
              (int)retCode, (int)outBegIdx, (int)outNbElement, MV_DATA_SIZE );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   for( i = 0; i < MV_DATA_SIZE; i++ )
   {
      if( !( mvOut[i] == mvInplace[i] ) )
      {
         printf( "\nFail: MAVP unguarded narrow huge band [%d]: got %.17g,"
                 " expected %.17g\n", i, mvOut[i], mvInplace[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   /* Control: the same path at in-contract periods is untouched. The bound is
    * inert for anything the guarded API can express. */
   for( i = 0; i < MV_DATA_SIZE; i++ )
      mvPeriods[i] = (TA_Real)( 2 + i % 50 );
   outBegIdx = outNbElement = -1;
   retCode = TA_MAVP_Unguarded( 0, MV_DATA_SIZE-1, mvInplace, mvPeriods,
                                1, 100000, TA_MAType_DISABLED,
                                &outBegIdx, &outNbElement, mvOut );
   if( retCode != TA_SUCCESS || outNbElement != MV_DATA_SIZE )
   {
      printf( "\nFail: MAVP unguarded in-contract control: rc=%d nb=%d\n",
              (int)retCode, (int)outNbElement );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   return TA_TEST_PASS;
}

/* Clamp a raw period value exactly like the implementation does. */
static int mvClamp( TA_Real raw, int minPeriod, int maxPeriod )
{
   int p = (int)raw;
   if( p < minPeriod )
      p = minPeriod;
   else if( p > maxPeriod )
      p = maxPeriod;
   return p;
}

/* One shape x {unstable 0, 7}, all MA types, with server verification. */
static ErrorNumber mvRunShapeMatrix( const char *label,
                                     const TA_History *history,
                                     int minPeriod, int maxPeriod )
{
   ErrorNumber errNb;
   int t, u;
   char subLabel[96];

   for( t = 0; t < mvNbTypes; t++ )
   {
      for( u = 0; u <= 1; u++ )
      {
         snprintf( subLabel, sizeof(subLabel), "%s maType=%d unstable=%d",
                   label, (int)mvTypes[t], u ? 7 : 0 );
         errNb = mvOracleCheck( subLabel, history, mvPeriods,
                                minPeriod, maxPeriod, mvTypes[t], u ? 7 : 0,
                                0, MV_DATA_SIZE - 1 );
         if( errNb != TA_TEST_PASS )
            return errNb;
      }
   }
   return TA_TEST_PASS;
}

/* The definitional oracle: MAVP at bar k == TA_MA of that bar's clamped
 * period at bar k, bitwise, plus the shape/in-place/server checks.
 */
static ErrorNumber mvOracleCheck( const char *label,
                                  const TA_History *history,
                                  const TA_Real *periods,
                                  int minPeriod, int maxPeriod,
                                  TA_MAType maType, int unstable,
                                  int startIdx, int endIdx )
{
   TA_RetCode retCode;
   TA_Integer outBegIdx, outNbElement;
   TA_Integer maBegIdx, maNbElement;
   int i, p, lookbackTotal, expectedBegIdx, used;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, unstable );

   for( i = 0; i < MV_BUF_SIZE; i++ )
      mvOut[i] = TA_REAL_MIN;   /* poison: a stale slot cannot pass the oracle */

   retCode = TA_MAVP( startIdx, endIdx, history->close, periods,
                      minPeriod, maxPeriod, maType,
                      &outBegIdx, &outNbElement, mvOut );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: MAVP %s: retCode %d\n", label, (int)retCode );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* Shape contract, including the empty-output case. */
   lookbackTotal = TA_MAVP_Lookback( minPeriod, maxPeriod, maType );
   expectedBegIdx = ( startIdx > lookbackTotal ) ? startIdx : lookbackTotal;
   if( expectedBegIdx > endIdx )
   {
      if( outNbElement != 0 || outBegIdx != 0 )
      {
         printf( "\nFail: MAVP %s: expected empty output, got beg=%d nb=%d\n",
                 label, (int)outBegIdx, (int)outNbElement );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      return TA_TEST_PASS;
   }
   if( outBegIdx != expectedBegIdx ||
       outBegIdx + outNbElement - 1 != endIdx )
   {
      printf( "\nFail: MAVP %s: beg=%d nb=%d, expected beg=%d through %d\n",
              label, (int)outBegIdx, (int)outNbElement, expectedBegIdx, endIdx );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   /* Nothing may be written past outNBElement (the poison must survive). */
   for( i = outNbElement; i < MV_BUF_SIZE; i++ )
   {
      if( mvOut[i] != TA_REAL_MIN )
      {
         printf( "\nFail: MAVP %s: wrote %.17g past outNBElement at [%d]\n",
                 label, mvOut[i], i );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   /* Per-bar oracle, one full-range TA_MA call per period in use. */
   for( p = minPeriod; p <= maxPeriod; p++ )
   {
      used = 0;
      for( i = outBegIdx; i <= endIdx && !used; i++ )
         used = ( mvClamp( periods[i], minPeriod, maxPeriod ) == p );
      if( !used )
         continue;

      retCode = TA_MA( outBegIdx, endIdx, history->close, p, maType,
                       &maBegIdx, &maNbElement, mvMA );
      if( retCode != TA_SUCCESS )
      {
         printf( "\nFail: %s: oracle TA_MA(%d) retCode %d\n", label, p, (int)retCode );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      /* The alignment MAVP's output indexing relies on: a smaller period
       * never has a larger lookback (ma_lookback monotone in the period).
       */
      if( maBegIdx != outBegIdx || maNbElement != outNbElement )
      {
         printf( "\nFail: %s: TA_MA(%d) beg=%d nb=%d misaligned with MAVP beg=%d nb=%d"
                 " (ma_lookback monotonicity contract)\n",
                 label, p, (int)maBegIdx, (int)maNbElement,
                 (int)outBegIdx, (int)outNbElement );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      for( i = outBegIdx; i <= endIdx; i++ )
      {
         if( mvClamp( periods[i], minPeriod, maxPeriod ) != p )
            continue;
         if( !( mvOut[i - outBegIdx] == mvMA[i - outBegIdx] ) )
         {
            printf( "\nFail: %s: bar %d (period %d): got %.17g, expected %.17g\n",
                    label, i, p, mvOut[i - outBegIdx], mvMA[i - outBegIdx] );
            return TA_REGTEST_OPTIMIZATION_REF_ERROR;
         }
      }
   }

   /* In-place call (outReal == inReal) must produce the same bits. */
   memcpy( mvInplace, history->close, MV_DATA_SIZE * sizeof(TA_Real) );
   retCode = TA_MAVP( startIdx, endIdx, mvInplace, periods,
                      minPeriod, maxPeriod, maType,
                      &maBegIdx, &maNbElement, mvInplace );
   if( retCode != TA_SUCCESS ||
       maBegIdx != outBegIdx || maNbElement != outNbElement )
   {
      printf( "\nFail: %s: in-place rc=%d beg=%d nb=%d\n",
              label, (int)retCode, (int)maBegIdx, (int)maNbElement );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }
   for( i = 0; i < outNbElement; i++ )
   {
      if( !( mvInplace[i] == mvOut[i] ) )
      {
         printf( "\nFail: %s: in-place [%d] got %.17g, expected %.17g\n",
                 label, i, mvInplace[i], mvOut[i] );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
   }

   /* Cross-language, on every leg. server_verify forwards the current
    * TA_SetUnstablePeriod state to each server before the call
    * (sync_unstable_periods), so the unstable legs verify too: with the
    * forwarding disabled these fail on the EMA-family types, where the
    * unstable period moves outBegIdx (#145).
    */
   if( server_verify_active() )
   {
      ErrorNumber errNb;
      errNb = server_verify( "MAVP", startIdx, endIdx, MV_DATA_SIZE,
                             TA_SUCCESS, outBegIdx, outNbElement,
                             (const TA_Real*[]){ history->close, periods, NULL },
                             (const double[]){ (double)minPeriod, (double)maxPeriod,
                                               (double)maType }, 3,
                             (const TA_Real*[]){ mvOut, NULL }, NULL );
      if( errNb != TA_TEST_PASS )
      {
         printf( "Fail: %s: server verification\n", label );
         return errNb;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   return TA_TEST_PASS;
}

/* Shrinking endIdx must leave every earlier output bit-unchanged: the
 * forward-fill (causality) property MAVP's bounded per-period passes
 * rely on, pinned at the public API for each MAType.
 */
static ErrorNumber mvTruncationCheck( const char *label,
                                      const TA_History *history,
                                      const TA_Real *periods,
                                      int minPeriod, int maxPeriod,
                                      TA_MAType maType )
{
   TA_RetCode retCode;
   TA_Integer begFull, nbFull, begCut, nbCut;
   int i, c, cuts[3];

   retCode = TA_MAVP( 0, MV_DATA_SIZE - 1, history->close, periods,
                      minPeriod, maxPeriod, maType,
                      &begFull, &nbFull, mvOut );
   if( retCode != TA_SUCCESS || nbFull < 3 )
   {
      printf( "\nFail: %s: full call rc=%d nb=%d\n", label, (int)retCode, (int)nbFull );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   cuts[0] = begFull;                          /* single output      */
   cuts[1] = begFull + nbFull / 2;             /* mid-range          */
   cuts[2] = MV_DATA_SIZE - 2;                 /* one bar short      */
   for( c = 0; c < 3; c++ )
   {
      for( i = 0; i < MV_BUF_SIZE; i++ )
         mvOutCut[i] = TA_REAL_MIN;   /* poison: a shorter-than-reported write must not
                                       * compare equal against a previous cut's data */
      retCode = TA_MAVP( 0, cuts[c], history->close, periods,
                         minPeriod, maxPeriod, maType,
                         &begCut, &nbCut, mvOutCut );
      if( retCode != TA_SUCCESS || begCut != begFull ||
          begCut + nbCut - 1 != cuts[c] )
      {
         printf( "\nFail: %s: cut=%d rc=%d beg=%d nb=%d\n",
                 label, cuts[c], (int)retCode, (int)begCut, (int)nbCut );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      for( i = 0; i < nbCut; i++ )
      {
         if( !( mvOutCut[i] == mvOut[i] ) )
         {
            printf( "\nFail: %s: cut=%d [%d] got %.17g, full-range %.17g\n",
                    label, cuts[c], i, mvOutCut[i], mvOut[i] );
            return TA_REGTEST_OPTIMIZATION_REF_ERROR;
         }
      }
   }
   return TA_TEST_PASS;
}

/* TA_S_MAVP against a per-bar TA_S_MA oracle (float inputs, double out). */
static ErrorNumber mvSinglePrecisionCheck( const char *label,
                                           const TA_History *history,
                                           const TA_Real *periods,
                                           int minPeriod, int maxPeriod,
                                           TA_MAType maType )
{
   TA_RetCode retCode;
   TA_Integer outBegIdx, outNbElement;
   TA_Integer maBegIdx, maNbElement;
   int i, p, used;

   for( i = 0; i < MV_DATA_SIZE; i++ )
   {
      mvPriceS[i] = (float)history->close[i];
      mvPeriodsS[i] = (float)periods[i];
   }
   for( i = 0; i < MV_BUF_SIZE; i++ )
      mvOut[i] = TA_REAL_MIN;

   retCode = TA_S_MAVP( 0, MV_DATA_SIZE - 1, mvPriceS, mvPeriodsS,
                        minPeriod, maxPeriod, maType,
                        &outBegIdx, &outNbElement, mvOut );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFail: %s: retCode %d\n", label, (int)retCode );
      return TA_REGTEST_OPTIMIZATION_REF_ERROR;
   }

   for( p = minPeriod; p <= maxPeriod; p++ )
   {
      used = 0;
      for( i = outBegIdx; i < MV_DATA_SIZE && !used; i++ )
         used = ( mvClamp( (TA_Real)mvPeriodsS[i], minPeriod, maxPeriod ) == p );
      if( !used )
         continue;

      retCode = TA_S_MA( outBegIdx, MV_DATA_SIZE - 1, mvPriceS, p, maType,
                         &maBegIdx, &maNbElement, mvMA );
      if( retCode != TA_SUCCESS || maBegIdx != outBegIdx )
      {
         printf( "\nFail: %s: oracle TA_S_MA(%d) rc=%d beg=%d\n",
                 label, p, (int)retCode, (int)maBegIdx );
         return TA_REGTEST_OPTIMIZATION_REF_ERROR;
      }
      for( i = outBegIdx; i < MV_DATA_SIZE; i++ )
      {
         if( mvClamp( (TA_Real)mvPeriodsS[i], minPeriod, maxPeriod ) != p )
            continue;
         if( !( mvOut[i - outBegIdx] == mvMA[i - outBegIdx] ) )
         {
            printf( "\nFail: %s: bar %d (period %d): got %.17g, expected %.17g\n",
                    label, i, p, mvOut[i - outBegIdx], mvMA[i - outBegIdx] );
            return TA_REGTEST_OPTIMIZATION_REF_ERROR;
         }
      }
   }
   return TA_TEST_PASS;
}
