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
 *  071626 MF,CC  First version. Composite-function test category.
 *  072026 MF,CC  Oracle check via checkOracleValue (near-zero absolute floor).
 *  072126 MF,CC  CMF differential leg, at tolerance rather than bitwise (#134).
 */

/* Description:
 *
 *   Regression test category for COMPOSITE functions: indicators defined as an
 *   exact arithmetic composition of functions TA-Lib already ships. Each such
 *   function is verified two independent ways:
 *
 *   The DIFFERENTIAL leg comes in two flavours. Most members are BIT-EXACT: the
 *   fused loop and the compose-over-primitives reference perform the same
 *   operations in the same order, so anything but equality is a defect. CMF is
 *   the other flavour, ALGEBRAIC-IDENTITY-AT-TOLERANCE: its reference differences
 *   a cumulative accumulator (TA_AD) where the shipped loop sums a sliding
 *   window, so the two sum different term sets in different orders and IEEE
 *   addition is not associative. Bit-equality is unreachable there by
 *   construction, not by sloppiness -- do not "fix" it into a memcmp.
 *
 *   1. DIFFERENTIAL (bit-exact). The shipped implementation is compared,
 *      bit-for-bit, against a test-only reference built by calling the shipped
 *      sub-function(s) it composes. The reference contains ZERO new numerical
 *      logic - only calls to primitives already proven by the cross-language
 *      bitwise gate (--xlang-hash), the differential fuzz (--fuzz-064) and the
 *      hard-coded expected values - so it is an honest second implementation.
 *      Two independently-maintained code paths that must agree exactly; this
 *      catches any drift a future optimization of the fused path could
 *      introduce. It proves OPTIMIZATION correctness, not formula correctness.
 *
 *   2. EXTERNAL-ORACLE (formula correctness). A handful of outputs are checked
 *      against golden values produced by an independent implementation. This
 *      proves the composition is the RIGHT formula, which (1) cannot: both sides
 *      of (1) could share the same wrong formula. Golden values, their source
 *      library + version, and the tolerance are documented at each call site.
 *
 *   First member: PVO (Percentage Volume Oscillator), which is defined as the
 *   PPO (Percentage Price Oscillator) applied to the volume series. Its
 *   differential reference is therefore a single call to the shipped TA_PPO on
 *   volume. Reference:
 *   https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/percentage-volume-oscillator-pvo
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define OUT_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* PVO differential grid: every MA type, plus a fast>slow pair to exercise the
 * internal fast/slow swap. PVO(volume) must equal PPO(volume) bit-for-bit for
 * ALL of them (identical composition, different function). */
static const struct { int fast; int slow; TA_MAType maType; } pvoGrid[] =
{
   { 12, 26, TA_MAType_SMA   },
   { 12, 26, TA_MAType_EMA   },
   { 12, 26, TA_MAType_WMA   },
   { 12, 26, TA_MAType_DEMA  },
   { 12, 26, TA_MAType_TEMA  },
   { 12, 26, TA_MAType_TRIMA },
   { 12, 26, TA_MAType_KAMA  },
   { 12, 26, TA_MAType_MAMA  },
   { 12, 26, TA_MAType_T3    },
   { 26, 12, TA_MAType_SMA   },  /* fast>slow: internal swap path */
   { 26, 12, TA_MAType_EMA   },  /* fast>slow: internal swap path */
   {  5, 10, TA_MAType_EMA   },  /* shorter periods */
};
#define NB_PVO_GRID (sizeof(pvoGrid)/sizeof(pvoGrid[0]))

/* PVO external-oracle golden values.
 *
 * Source: pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1), column
 * PVO_12_26_9, on the standard 252-bar volume series
 * (TA_SREF_volume_daily_ref_0_PRIV), fast=12, slow=26, EXPONENTIAL form
 * (optInMAType = TA_MAType_EMA), unstable period 0. outBegIdx=25, nb=227.
 * These values are identical to pandas-ta-classic v0.6.52's PVO line; TA-Lib's
 * EMA-form PVO reproduces them to a measured max relative error of ~1.4e-14
 * (max absolute ~6e-14) - the ULP-scale difference between the two EMA
 * recurrences, not a formula difference (a wrong formula diverges by whole
 * percent).
 *
 * idx is the OUTPUT-array index (0 == global bar 25). */
static const struct { int idx; double value; } pvoOracle[] =
{
   {   0,   2.591161493249625   },
   {   1,   1.0831790429048995  },
   {  56,   9.65424368702839    },
   { 113,  12.314326908257636   },
   { 170,  12.146354938349695   },
   { 226, -28.68370548643097    },
};
#define NB_PVO_ORACLE (sizeof(pvoOracle)/sizeof(pvoOracle[0]))

#define PVO_ORACLE_EXPECTED_BEG 25
#define PVO_ORACLE_EXPECTED_NB  227
/* Relative tolerance for the oracle check: 1e-12. The measured agreement at
 * these six samples is ~1.4e-14 (max abs ~6e-14), and the EMA-recurrence
 * difference is bounded by ~n*eps ~ 1e-13 across all 252 bars, so 1e-12 keeps a
 * ~10x-70x margin against cross-platform FP-rounding variance while staying far
 * tighter than any real formula error (SMA-vs-EMA, wrong scalar/periods all
 * diverge by >1%). */
#define PVO_ORACLE_TOL 1e-12
/* Near-zero absolute floor. PVO is an oscillator: it crosses zero, where a
 * relative test is meaningless. The six goldens above were deliberately sampled
 * where |value| >= 1, so this floor never governs them (rel term at |want|=1 is
 * 1e-12, a decade above it) -- it is here so that adding a golden near a
 * crossing later cannot silently turn the check into a spurious failure. Sized
 * ~10x the measured absolute agreement (~6e-14). See checkOracleValue(). */
#define PVO_ORACLE_ABS 1e-12

/**** Local functions declarations. ****/
static ErrorNumber test_pvo_differential( const TA_History *history );
static ErrorNumber test_pvo_oracle( const TA_History *history );
static ErrorNumber test_pvo_default_is_ema( const TA_History *history );
static ErrorNumber test_vwma_differential( const TA_History *history );
static ErrorNumber test_vwma_oracle( const TA_History *history );
static ErrorNumber test_vwma_inplace( const TA_History *history );
static ErrorNumber test_vwma_tulip_vectors( void );
static ErrorNumber test_vwma_flat_price( void );
static ErrorNumber test_vwma_all_zero_volume( void );
static ErrorNumber test_cmf_differential( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_composite( TA_History *history )
{
   ErrorNumber retValue;

   /* PVO composes PPO, which uses TA_MA / EMA; pin the EMA unstable period to 0
    * so the EMA-form comparisons are deterministic and match the oracle. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );

   retValue = test_pvo_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_pvo_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_pvo_default_is_ema( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_vwma_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_vwma_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_vwma_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_vwma_tulip_vectors();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_vwma_flat_price();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_vwma_all_zero_volume();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_cmf_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) DIFFERENTIAL: PVO(volume) == PPO(volume), bit-for-bit, across the grid. */
static ErrorNumber test_pvo_differential( const TA_History *history )
{
   unsigned int g;
   TA_RetCode rcPVO, rcPPO;
   TA_Integer begPVO, nbPVO, begPPO, nbPPO;
   static TA_Real outPVO[OUT_CAP];
   static TA_Real outPPO[OUT_CAP];

   for( g = 0; g < NB_PVO_GRID; g++ )
   {
      int fast = pvoGrid[g].fast;
      int slow = pvoGrid[g].slow;
      TA_MAType mt = pvoGrid[g].maType;

      /* Shipped composite. */
      rcPVO = TA_PVO( 0, (int)history->nbBars - 1, history->volume,
                      fast, slow, mt, &begPVO, &nbPVO, outPVO );

      /* Test-only reference: the shipped sub-function it composes, on volume. */
      rcPPO = TA_PPO( 0, (int)history->nbBars - 1, history->volume,
                      fast, slow, mt, &begPPO, &nbPPO, outPPO );

      if( rcPVO != rcPPO )
      {
         printf( "PVO differential Fail [grid %u f=%d s=%d mt=%d]: retCode PVO=%d PPO=%d\n",
                 g, fast, slow, (int)mt, (int)rcPVO, (int)rcPPO );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( rcPVO != TA_SUCCESS )
         continue;   /* both agreed on the same error; nothing to compare */

      if( begPVO != begPPO || nbPVO != nbPPO )
      {
         printf( "PVO differential Fail [grid %u f=%d s=%d mt=%d]: shape PVO(%d,%d) PPO(%d,%d)\n",
                 g, fast, slow, (int)mt, begPVO, nbPVO, begPPO, nbPPO );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      /* Bit-exact: same composition, same inputs => identical bits. */
      if( nbPVO > 0 && memcmp( outPVO, outPPO, (size_t)nbPVO * sizeof(TA_Real) ) != 0 )
      {
         int i;
         for( i = 0; i < nbPVO; i++ )
            if( outPVO[i] != outPPO[i] )
            {
               printf( "PVO differential Fail [grid %u f=%d s=%d mt=%d]: bit mismatch at "
                       "out[%d] PVO=%.17g PPO=%.17g\n",
                       g, fast, slow, (int)mt, i, outPVO[i], outPPO[i] );
               break;
            }
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* Cross-language: PVO must also be bit-identical on every language server. */
      if( server_verify_active() )
      {
         ErrorNumber e = server_verify( "PVO", 0, (int)history->nbBars - 1, history->nbBars,
                                        rcPVO, begPVO, nbPVO,
                                        (const TA_Real*[]){ history->volume, NULL },
                                        (double[]){ (double)fast, (double)slow, (double)mt }, 3,
                                        (const TA_Real*[]){ outPVO, NULL }, NULL );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (2) EXTERNAL-ORACLE: PVO(volume, EMA, 12, 26) vs the pandas-ta-classic golden. */
static ErrorNumber test_pvo_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int k;

   retCode = TA_PVO( 0, (int)history->nbBars - 1, history->volume,
                     12, 26, TA_MAType_EMA, &begIdx, &nbElement, out );

   if( retCode != TA_SUCCESS )
   {
      printf( "PVO oracle Fail: retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != PVO_ORACLE_EXPECTED_BEG || nbElement != PVO_ORACLE_EXPECTED_NB )
   {
      printf( "PVO oracle Fail: shape got (%d,%d) expected (%d,%d)\n",
              begIdx, nbElement, PVO_ORACLE_EXPECTED_BEG, PVO_ORACLE_EXPECTED_NB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( k = 0; k < NB_PVO_ORACLE; k++ )
   {
      int idx = pvoOracle[k].idx;
      double want = pvoOracle[k].value;
      double got  = out[idx];
      double err; const char *mode;

      if( !checkOracleValue( got, want, PVO_ORACLE_TOL, PVO_ORACLE_ABS, &err, &mode ) )
      {
         printf( "PVO oracle Fail at out[%d]: got %.17g expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                 idx, got, want, mode, err, PVO_ORACLE_TOL, PVO_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The default optInMAType must be EMA (Gerald Appel's PPO form), not SMA. The
 * C entry point takes optInMAType positionally, so the DEFAULT only exists in the
 * ta_abstract layer. This checks BOTH the declared default value AND that the
 * abstract call path (MAType left unset) actually computes the EMA result. */
static ErrorNumber test_pvo_default_is_ema( const TA_History *history )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo   *funcInfo;
   TA_ParamHolder      *paramHolder;
   TA_RetCode           r1, r2, r3, r4, rc;
   TA_Integer           emaBeg, emaNb, smaBeg, smaNb, defBeg, defNb;
   static TA_Real       emaOut[OUT_CAP], smaOut[OUT_CAP], defOut[OUT_CAP];
   int                  endIdx = (int)history->nbBars - 1;
   int                  maTypeFound = 0;
   unsigned int         i;

   /* (a) Declared default: PVO's MAType optional input defaults to EMA. */
   if( TA_GetFuncHandle( "PVO", &handle ) != TA_SUCCESS ||
       TA_GetFuncInfo( handle, &funcInfo ) != TA_SUCCESS )
   {
      printf( "PVO default Fail: cannot get PVO func handle/info\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   for( i = 0; i < funcInfo->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *optInfo;
      TA_GetOptInputParameterInfo( handle, i, &optInfo );
      if( optInfo->paramName && strstr( optInfo->paramName, "MAType" ) )
      {
         maTypeFound = 1;
         if( (int)optInfo->defaultValue != (int)TA_MAType_EMA )
         {
            printf( "PVO default Fail: optInMAType default = %d, expected EMA (%d)\n",
                    (int)optInfo->defaultValue, (int)TA_MAType_EMA );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }
   if( !maTypeFound )
   {
      printf( "PVO default Fail: no MAType optional input found\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   /* Explicit EMA and SMA references. They MUST differ, or (b) proves nothing. */
   if( TA_PVO( 0, endIdx, history->volume, 12, 26, TA_MAType_EMA, &emaBeg, &emaNb, emaOut ) != TA_SUCCESS ||
       TA_PVO( 0, endIdx, history->volume, 12, 26, TA_MAType_SMA, &smaBeg, &smaNb, smaOut ) != TA_SUCCESS )
   {
      printf( "PVO default Fail: explicit TA_PVO call failed\n" );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( emaNb != smaNb ||
       memcmp( emaOut, smaOut, (size_t)emaNb * sizeof(TA_Real) ) == 0 )
   {
      printf( "PVO default Fail: EMA and SMA outputs identical — test would be vacuous\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* (b) Behavioural: drive PVO through ta_abstract setting only fast+slow, leaving
    * optInMAType at its allocator-initialized default; the result must be the EMA
    * one (bit-exact) — hence NOT the SMA one. */
   if( TA_ParamHolderAlloc( handle, &paramHolder ) != TA_SUCCESS )
   {
      printf( "PVO default Fail: TA_ParamHolderAlloc failed\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   r1 = TA_SetInputParamPricePtr( paramHolder, 0, history->open, history->high,
                                  history->low, history->close, history->volume,
                                  history->openInterest );
   r2 = TA_SetOptInputParamInteger( paramHolder, 0, 12 );  /* optInFastPeriod */
   r3 = TA_SetOptInputParamInteger( paramHolder, 1, 26 );  /* optInSlowPeriod */
   /* optInMAType (index 2) is deliberately NOT set -> uses the default. */
   r4 = TA_SetOutputParamRealPtr( paramHolder, 0, defOut );
   if( r1 != TA_SUCCESS || r2 != TA_SUCCESS || r3 != TA_SUCCESS || r4 != TA_SUCCESS )
   {
      printf( "PVO default Fail: abstract param setup failed (%d,%d,%d,%d)\n",
              (int)r1, (int)r2, (int)r3, (int)r4 );
      TA_ParamHolderFree( paramHolder );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   rc = TA_CallFunc( paramHolder, 0, endIdx, &defBeg, &defNb );
   TA_ParamHolderFree( paramHolder );
   if( rc != TA_SUCCESS )
   {
      printf( "PVO default Fail: TA_CallFunc (default MAType) rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( defBeg != emaBeg || defNb != emaNb ||
       memcmp( defOut, emaOut, (size_t)defNb * sizeof(TA_Real) ) != 0 )
   {
      printf( "PVO default Fail: default-MAType output != explicit EMA "
              "(the default is not EMA)\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* ==================== VWMA ====================
 *
 * VWMA is Sum(P*V)/Sum(V) over a trailing window. TradingView documents the
 * equivalence ta.vwma(src,len) == ta.sma(src*volume,len)/ta.sma(volume,len),
 * and the shipped implementation keeps the redundant /n on both sums (and
 * TA_SMA's add-new / snapshot / subtract-old order) precisely so that
 * equivalence holds BIT-FOR-BIT rather than approximately. If the differential
 * below ever needs a tolerance, the implementation has drifted from the spec --
 * do not paper over it with an epsilon.
 */

/* VWMA period grid: the boundary period, the two oracle-vector periods, the
 * shipped default, and a long window. */
static const int vwmaGrid[] = { 1, 2, 3, 4, 5, 14, 30, 100 };
#define NB_VWMA_GRID (sizeof(vwmaGrid)/sizeof(vwmaGrid[0]))

/* VWMA external-oracle golden values.
 *
 * Source: pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1), on the standard
 * 252-bar close/volume series (TA_SREF_close_daily_ref_0_PRIV /
 * TA_SREF_volume_daily_ref_0_PRIV), optInTimePeriod = 30. outBegIdx = 29,
 * nb = 223. pandas agrees on all six to a measured max relative error of
 * 4.7e-16; Tulip's fused Sum(pv)/Sum(v) form deviates by 2.2e-16 on this series
 * (it is a tolerance oracle, not a bitwise one -- see the header note).
 *
 * idx is the OUTPUT-array index (0 == global bar 29). */
static const struct { int idx; double value; } vwmaOracle[] =
{
   {   0,  90.3104571585586  },
   {   1,  90.1627270288419  },
   {   2,  89.9679352432261  },
   { 111, 127.6634203180634  },
   { 221, 107.8003329935021  },
   { 222, 108.2790680119618  },
};
#define NB_VWMA_ORACLE (sizeof(vwmaOracle)/sizeof(vwmaOracle[0]))

#define VWMA_ORACLE_EXPECTED_BEG 29
#define VWMA_ORACLE_EXPECTED_NB  223
#define VWMA_ORACLE_PERIOD       30
/* Relative tolerance 1e-12 against a measured 4.7e-16 agreement -- a ~1000x
 * margin for cross-platform rounding, still far tighter than any formula error
 * (an unweighted mean, or a fused-form mix-up, diverges by whole percent).
 * Absolute floor 1e-9: VWMA tracks price, so it never approaches zero on any
 * realistic series, but the floor keeps a future near-zero golden from turning
 * the check into a spurious failure. See checkOracleValue(). */
#define VWMA_ORACLE_TOL 1e-12
#define VWMA_ORACLE_ABS 1e-9

/* (1) DIFFERENTIAL: VWMA == SMA(inReal*inVolume)/SMA(inVolume), bit-for-bit. */
static ErrorNumber test_vwma_differential( const TA_History *history )
{
   unsigned int g;
   int i, nbBars;
   TA_RetCode rcV, rcA, rcB;
   TA_Integer begV, nbV, begA, nbA, begB, nbB;
   static TA_Real product[OUT_CAP];
   static TA_Real outVWMA[OUT_CAP];
   static TA_Real outSmaPV[OUT_CAP];
   static TA_Real outSmaV[OUT_CAP];

   nbBars = (int)history->nbBars;

   /* Form the price*volume series exactly as vwma.c does: the multiply is its
    * own statement so neither side can contract it into an FMA. */
   for( i = 0; i < nbBars; i++ )
      product[i] = history->close[i] * history->volume[i];

   for( g = 0; g < NB_VWMA_GRID; g++ )
   {
      int period = vwmaGrid[g];

      rcV = TA_VWMA( 0, nbBars - 1, history->close, history->volume,
                     period, &begV, &nbV, outVWMA );

      /* Test-only reference: two calls to the shipped TA_SMA. No new numerical
       * logic -- only primitives already proven by the bitwise cross-language
       * gate, the differential fuzz and the hardcoded expected values. */
      rcA = TA_SMA( 0, nbBars - 1, product,          period, &begA, &nbA, outSmaPV );
      rcB = TA_SMA( 0, nbBars - 1, history->volume,  period, &begB, &nbB, outSmaV );

      if( rcV != rcA || rcA != rcB )
      {
         printf( "VWMA differential Fail [period %d]: retCode VWMA=%d SMA=%d/%d\n",
                 period, (int)rcV, (int)rcA, (int)rcB );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( rcV != TA_SUCCESS )
         continue;

      if( begV != begA || nbV != nbA || begA != begB || nbA != nbB )
      {
         printf( "VWMA differential Fail [period %d]: range VWMA(%d,%d) SMA(%d,%d)/(%d,%d)\n",
                 period, (int)begV, (int)nbV, (int)begA, (int)nbA, (int)begB, (int)nbB );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbV; i++ )
      {
         double want = outSmaPV[i] / outSmaV[i];
         if( memcmp( &outVWMA[i], &want, sizeof(double) ) != 0 )
         {
            printf( "VWMA differential Fail [period %d] at out[%d]: "
                    "fused %.17g != compose %.17g (must be BIT-exact)\n",
                    period, i, outVWMA[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) EXTERNAL ORACLE: the composition is the RIGHT formula, which (1) cannot
 * show -- both sides of (1) could share the same wrong formula. */
static ErrorNumber test_vwma_oracle( const TA_History *history )
{
   unsigned int k;
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real out[OUT_CAP];

   rc = TA_VWMA( 0, (int)history->nbBars - 1, history->close, history->volume,
                 VWMA_ORACLE_PERIOD, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "VWMA oracle Fail: retCode %d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != VWMA_ORACLE_EXPECTED_BEG || nb != VWMA_ORACLE_EXPECTED_NB )
   {
      printf( "VWMA oracle Fail: got beg=%d nb=%d expected %d/%d\n",
              (int)beg, (int)nb, VWMA_ORACLE_EXPECTED_BEG, VWMA_ORACLE_EXPECTED_NB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( k = 0; k < NB_VWMA_ORACLE; k++ )
   {
      int idx = vwmaOracle[k].idx;
      double want = vwmaOracle[k].value;
      double got  = out[idx];
      double err; const char *mode;

      if( !checkOracleValue( got, want, VWMA_ORACLE_TOL, VWMA_ORACLE_ABS, &err, &mode ) )
      {
         printf( "VWMA oracle Fail at out[%d]: got %.17g expected %.17g "
                 "(%s=%.3e > rel %.3e / abs %.3e)\n",
                 idx, got, want, mode, err, VWMA_ORACLE_TOL, VWMA_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) IN-PLACE ALIASING. The C API lets the caller pass the same buffer as an
 * input and the output. VWMA reads both trailing values before storing, so this
 * must hold for the output aliased over EITHER input -- and each is a distinct
 * risk, since the two reads are separate statements. See issue #130 for the
 * functions where this invariant is currently broken. */
static ErrorNumber test_vwma_inplace( const TA_History *history )
{
   int i, nbBars, period = 30;
   TA_RetCode rc;
   TA_Integer begR, nbR, beg, nb;
   static TA_Real ref[OUT_CAP];
   static TA_Real work[OUT_CAP];
   static TA_Real vol[OUT_CAP];

   nbBars = (int)history->nbBars;

   rc = TA_VWMA( 0, nbBars - 1, history->close, history->volume, period, &begR, &nbR, ref );
   if( rc != TA_SUCCESS )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   /* (a) output aliased over the price input. */
   for( i = 0; i < nbBars; i++ ) work[i] = history->close[i];
   rc = TA_VWMA( 0, nbBars - 1, work, history->volume, period, &beg, &nb, work );
   if( rc != TA_SUCCESS || beg != begR || nb != nbR )
   {
      printf( "VWMA in-place(price) Fail: rc=%d range(%d,%d) vs (%d,%d)\n",
              (int)rc, (int)beg, (int)nb, (int)begR, (int)nbR );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nb; i++ )
      if( memcmp( &work[i], &ref[i], sizeof(double) ) != 0 )
      {
         printf( "VWMA in-place(price) Fail at out[%d]: got %.17g expected %.17g\n",
                 i, work[i], ref[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* (b) output aliased over the volume input. */
   for( i = 0; i < nbBars; i++ ) vol[i] = history->volume[i];
   rc = TA_VWMA( 0, nbBars - 1, history->close, vol, period, &beg, &nb, vol );
   if( rc != TA_SUCCESS || beg != begR || nb != nbR )
   {
      printf( "VWMA in-place(volume) Fail: rc=%d range(%d,%d) vs (%d,%d)\n",
              (int)rc, (int)beg, (int)nb, (int)begR, (int)nbR );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nb; i++ )
      if( memcmp( &vol[i], &ref[i], sizeof(double) ) != 0 )
      {
         printf( "VWMA in-place(volume) Fail at out[%d]: got %.17g expected %.17g\n",
                 i, vol[i], ref[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   return TA_TEST_PASS;
}

/* (4) SECOND EXTERNAL ORACLE: Tulip Indicators 0.9.2 checked-in vectors.
 *
 * The pandas goldens above and these are independent of each other, which
 * matters because the composite differential cannot catch a shared wrong
 * formula. Note the two vectors are NOT of equal standing:
 *
 *   - tests/extra.txt:320 states its values are "calculated by hand or in a
 *     spreadsheet" -- genuinely independent of any implementation.
 *   - tests/untest.txt:466 is regenerated from Tulip's own output; its header
 *     says "failing a test here doesn't necessarily indicate a fault". It is a
 *     cross-implementation check, not an independent derivation.
 *
 * Tolerance is absolute at half a unit of the last printed decimal, since that
 * is the only precision the vectors carry (4dp for extra, 3dp for untest).
 * Tulip computes the fused Sum(pv)/Sum(v); we compute the /n form, so the two
 * agree to ~1e-16 relative -- far inside the printed precision. */
static const double tulipVwma5In[]  = { 81.59,81.06,82.87,83.00,83.61,83.15,82.84,83.99,
                                        84.55,84.36,85.53,86.54,86.89,87.77,87.29 };
static const double tulipVwma5Vol[] = { 5653100,6447400,7690900,3831400,4455100,3798000,
                                        3936200,4732000,4841300,3915300,6830800,6694100,
                                        5293600,7985800,4807900 };
static const double tulipVwma5Exp[] = { 82.332,82.610,83.070,83.354,83.682,83.822,
                                        84.409,85.165,85.698,86.418,86.805 };

static const double tulipVwma4In[]  = { 50.25,50.55,52.5,54.5,54.1,54.12,55.5,50.2,50.45,50.24 };
static const double tulipVwma4Vol[] = { 12412,12458,15874,12354,12456,12542,15421,19510,12521,12041 };
static const double tulipVwma4Exp[] = { 51.9819,52.8828,53.7204,54.6075,53.1948,52.4340,51.6345 };

static ErrorNumber vwma_check_vector( const char *tag,
                                      const double *in, const double *vol, int n,
                                      int period, const double *exp, int nbExp,
                                      double absTol )
{
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i;
   static TA_Real out[OUT_CAP];
   double err; const char *mode;

   rc = TA_VWMA( 0, n - 1, in, vol, period, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "VWMA %s Fail: retCode %d\n", tag, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != period - 1 || nb != nbExp )
   {
      printf( "VWMA %s Fail: beg=%d nb=%d expected beg=%d nb=%d\n",
              tag, (int)beg, (int)nb, period - 1, nbExp );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbExp; i++ )
   {
      if( !checkOracleValue( out[i], exp[i], 0.0, absTol, &err, &mode ) )
      {
         printf( "VWMA %s Fail at out[%d]: got %.17g expected %.17g (%s=%.3e > abs %.3e)\n",
                 tag, i, out[i], exp[i], mode, err, absTol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

static ErrorNumber test_vwma_tulip_vectors( void )
{
   ErrorNumber r;

   /* extra.txt:320 -- hand/spreadsheet derived, 4 decimals. */
   r = vwma_check_vector( "tulip extra.txt:320 (vwma 4)",
                          tulipVwma4In, tulipVwma4Vol,
                          (int)(sizeof(tulipVwma4In)/sizeof(double)), 4,
                          tulipVwma4Exp,
                          (int)(sizeof(tulipVwma4Exp)/sizeof(double)), 5e-5 );
   if( r != TA_TEST_PASS ) return r;

   /* untest.txt:466 -- regenerated from Tulip's own output, 3 decimals. */
   r = vwma_check_vector( "tulip untest.txt:466 (vwma 5)",
                          tulipVwma5In, tulipVwma5Vol,
                          (int)(sizeof(tulipVwma5In)/sizeof(double)), 5,
                          tulipVwma5Exp,
                          (int)(sizeof(tulipVwma5Exp)/sizeof(double)), 5e-4 );
   return r;
}

/* (5) FLAT PRICE SERIES. With every price equal to C the weighted mean is C for
 * any volumes. The running sum of C*v does not round to C*sum(v), so agreement
 * is to a few ULP rather than bitwise -- as it is everywhere: on the same flat
 * series pandas-ta is out by up to 9.1e-12 and Tulip by 6.5e-11 (ours 8.0e-11).
 * Hence a relative bound. */
static ErrorNumber test_vwma_flat_price( void )
{
#define FLAT_N 120
   static const double flatPrice[] = { 0.1, 93.75, 100.0, 12345.678 };
   static const int    flatPeriod[] = { 2, 5, 14, 30, 100 };
   static TA_Real in[FLAT_N], vol[FLAT_N], out[OUT_CAP];
   unsigned int pi, ki;
   int i;
   TA_RetCode rc;
   TA_Integer beg, nb;
   double err; const char *mode;

   for( i = 0; i < FLAT_N; i++ )
      vol[i] = 1000.0 + (double)((i * 37) % 911) * 13.0;   /* wildly varying */

   for( pi = 0; pi < sizeof(flatPrice)/sizeof(flatPrice[0]); pi++ )
   {
      double C = flatPrice[pi];
      for( i = 0; i < FLAT_N; i++ ) in[i] = C;

      for( ki = 0; ki < sizeof(flatPeriod)/sizeof(flatPeriod[0]); ki++ )
      {
         int period = flatPeriod[ki];
         rc = TA_VWMA( 0, FLAT_N - 1, in, vol, period, &beg, &nb, out );
         if( rc != TA_SUCCESS )
         {
            printf( "VWMA flat-price Fail [C=%g period=%d]: retCode %d\n", C, period, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         for( i = 0; i < nb; i++ )
         {
            if( !checkOracleValue( out[i], C, 1e-13, 0.0, &err, &mode ) )
            {
               printf( "VWMA flat-price Fail [C=%g period=%d] at out[%d]: got %.17g (%s=%.3e > rel 1e-13)\n",
                       C, period, i, out[i], mode, err );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
#undef FLAT_N
}

/* (6) ALL-ZERO VOLUME. The window has no weights, so the weighted mean is
 * undefined and the element is NaN -- same as Tulip, pandas-ta and numpy's
 * zero-weight average. There is deliberately no guard; this pins that, so a
 * future guard cannot silently contradict the VWMA page. */
static ErrorNumber test_vwma_all_zero_volume( void )
{
#define ZV_N 60
   static TA_Real in[ZV_N], vol[ZV_N], out[OUT_CAP];
   int i;
   TA_RetCode rc;
   TA_Integer beg, nb;

   for( i = 0; i < ZV_N; i++ ) { in[i] = 90.0 + (double)(i % 5); vol[i] = 0.0; }

   rc = TA_VWMA( 0, ZV_N - 1, in, vol, 30, &beg, &nb, out );
   if( rc != TA_SUCCESS || nb <= 0 )
   {
      printf( "VWMA all-zero-volume Fail: retCode %d nb %d (a successful call is expected)\n",
              (int)rc, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nb; i++ )
   {
      if( out[i] == out[i] )   /* NaN is the only value failing self-equality */
      {
         printf( "VWMA all-zero-volume Fail at out[%d]: got %.17g, expected NaN "
                 "(no guard is intended)\n", i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
#undef ZV_N
}

/* ------------------------------------------------------------------------- */
/* CMF -- ALGEBRAIC-IDENTITY-AT-TOLERANCE differential.
 *
 * CMF[i] == ( AD[i] - AD[i-n] ) / SUM(volume,n)[i]
 *
 * This is an algebraic identity, not an operational one: TA_AD is a cumulative
 * start-dependent accumulator, so differencing two of its running totals sums a
 * different term set, in a different order, than the shipped sliding-window sum.
 * Measured against exact rational arithmetic the two are equally accurate
 * (~1e-16 absolute on this corpus); they simply round differently. Hence the
 * tolerance, and hence an ABSOLUTE one -- CMF crosses zero, so a relative bound
 * explodes at the crossings while the absolute error stays at the last bit.
 *
 * The value of the leg is twofold: it states at a high level that the fused loop
 * computes what it claims, and it is a cross-function integrity net -- if a
 * future change silently breaks TA_AD or TA_SUM, this composed expectation
 * breaks too, so the damage surfaces here as well as in those functions' own
 * tests. It introduces NO new numerical logic: only calls to shipped primitives.
 */
#define CMF_DIFF_TOL 1e-13
#define CMF_DIFF_ABS 1e-13

static const int cmfDiffGrid[] = { 2, 14, 20, 21, 50, 100 };  /* 100 > CIRCBUF static size */
#define NB_CMF_DIFF_GRID (sizeof(cmfDiffGrid)/sizeof(cmfDiffGrid[0]))

static ErrorNumber test_cmf_differential( const TA_History *history )
{
   unsigned int g;
   int i, nbBars;
   TA_RetCode rcC, rcAD, rcS;
   TA_Integer begC, nbC, begAD, nbAD, begS, nbS;
   static TA_Real outCMF[OUT_CAP];
   static TA_Real outAD[OUT_CAP];
   static TA_Real outSumV[OUT_CAP];

   nbBars = (int)history->nbBars;

   /* AD over the whole series: outAD[i] is the accumulator at bar i. */
   rcAD = TA_AD( 0, nbBars - 1, history->high, history->low,
                 history->close, history->volume, &begAD, &nbAD, outAD );
   if( rcAD != TA_SUCCESS || begAD != 0 || nbAD != nbBars )
   {
      printf( "CMF differential Fail: TA_AD rc=%d shape (%d,%d) expected (0,%d)\n",
              (int)rcAD, (int)begAD, (int)nbAD, nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( g = 0; g < NB_CMF_DIFF_GRID; g++ )
   {
      int period = cmfDiffGrid[g];

      rcC = TA_CMF( 0, nbBars - 1, history->high, history->low,
                    history->close, history->volume,
                    period, &begC, &nbC, outCMF );
      rcS = TA_SUM( 0, nbBars - 1, history->volume, period, &begS, &nbS, outSumV );

      if( rcC != rcS )
      {
         printf( "CMF differential Fail [period %d]: retCode CMF=%d SUM=%d\n",
                 period, (int)rcC, (int)rcS );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( rcC != TA_SUCCESS )
         continue;

      /* Both are plain (n-1) window lookbacks, so the ranges must coincide. */
      if( begC != begS || nbC != nbS )
      {
         printf( "CMF differential Fail [period %d]: range CMF(%d,%d) SUM(%d,%d)\n",
                 period, (int)begC, (int)nbC, (int)begS, (int)nbS );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbC; i++ )
      {
         int bar = begC + i;                 /* absolute bar for output i */
         double adNow  = outAD[bar];
         double adThen = ( bar - period >= 0 ) ? outAD[bar - period] : 0.0;
         double want   = ( adNow - adThen ) / outSumV[i];
         double err;
         const char *mode;

         if( !checkOracleValue( outCMF[i], want, CMF_DIFF_TOL, CMF_DIFF_ABS, &err, &mode ) )
         {
            printf( "CMF differential Fail [period %d] at out[%d] (bar %d): "
                    "fused %.17g vs AD-difference %.17g (%s err=%.3e > %.3e/%.3e)\n",
                    period, i, bar, outCMF[i], want,
                    mode, err, CMF_DIFF_TOL, CMF_DIFF_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}
