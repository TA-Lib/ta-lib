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
 *  KL       Kevin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  071626 MF,CC  First version. Composite-function test category.
 *  072026 MF,CC  Oracle check via checkOracleValue (near-zero absolute floor).
 *  072126 MF,CC  CMF differential leg, at tolerance rather than bitwise (#134).
 *  072226 MF,CC  HMA legs, incl. TA_MAType_HMA dispatch parity (#139).
 *  081226 KL     EFI legs: EMA-of-force differential (#206).
 *  081226 KL     QSTICK legs: SMA-of-body differential plus two book vectors.
 *  081626 MF,CC  AO legs: two-SMA-of-median differential plus two oracles (#227).
 *  081726 MF,CC  AC legs: AO-less-its-own-SMA differential plus one oracle (#228).
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
   { 12, 26, TA_MAType_HMA   },
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
static ErrorNumber test_vwma_period_one( void );
static ErrorNumber test_vwma_oracle( const TA_History *history );
static ErrorNumber test_vwma_inplace( const TA_History *history );
static ErrorNumber test_vwma_tulip_vectors( void );
static ErrorNumber test_vwma_flat_price( void );
static ErrorNumber test_vwma_all_zero_volume( void );
static ErrorNumber test_cmf_differential( const TA_History *history );
static ErrorNumber test_hma_differential( const TA_History *history );
static ErrorNumber test_hma_oracle( const TA_History *history );
static ErrorNumber test_hma_tulip_vector( void );
static ErrorNumber test_hma_inplace( const TA_History *history );
static ErrorNumber test_hma_matype( const TA_History *history );
static ErrorNumber test_hma_single_element( const TA_History *history );
static ErrorNumber test_hma_param_reject( const TA_History *history );
static ErrorNumber test_hma_large_period( void );
static ErrorNumber test_efi_differential( const TA_History *history );
static ErrorNumber test_efi_oracle( const TA_History *history );
static ErrorNumber test_efi_degenerate( void );
static ErrorNumber test_efi_inplace( const TA_History *history );
static ErrorNumber test_qstick_differential( const TA_History *history );
static ErrorNumber test_qstick_oracle( void );
static ErrorNumber test_qstick_period_one( const TA_History *history );
static ErrorNumber test_qstick_flat( void );
static ErrorNumber test_qstick_inplace( const TA_History *history );
static ErrorNumber test_qstick_rounding_corpus( void );
static ErrorNumber test_ao_differential( const TA_History *history );
static ErrorNumber test_ao_oracle( const TA_History *history );
static ErrorNumber test_ao_inverted_pair( const TA_History *history );
static ErrorNumber test_ao_inplace( const TA_History *history );
static ErrorNumber test_ao_param_reject( const TA_History *history );
static ErrorNumber test_ao_reference_anchoring( const TA_History *history );
static ErrorNumber test_ac_differential( const TA_History *history );
static ErrorNumber test_ac_oracle( const TA_History *history );
static ErrorNumber test_ac_inverted_pair( const TA_History *history );
static ErrorNumber test_ac_inplace( const TA_History *history );
static ErrorNumber test_ac_param_reject( const TA_History *history );
static ErrorNumber test_ac_reference_anchoring( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_composite1( TA_History *history )
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

   retValue = test_vwma_period_one();
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

   retValue = test_hma_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_tulip_vector();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_matype( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_single_element( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_param_reject( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_hma_large_period();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_efi_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_efi_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_efi_degenerate();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_efi_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_qstick_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_qstick_oracle();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_qstick_period_one( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_qstick_flat();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_qstick_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_qstick_rounding_corpus();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ao_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ao_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ao_inverted_pair( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ao_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ao_param_reject( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ao_reference_anchoring( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ac_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ac_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ac_inverted_pair( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ac_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ac_param_reject( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_ac_reference_anchoring( history );
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
 *
 * The equivalence holds for every period EXCEPT 1, which is not governed by
 * this formula at all: a moving average of period 1 is the no-smoothing copy
 * of its input, the rule the whole TA_MAType family obeys. (P*V)/V is that
 * value in real arithmetic but round-trips only ~97% of the time in IEEE
 * double, so the copy is explicit in vwma.c and the composition would disagree
 * with it. Period 1 is out of the grid below and pinned by leg (1b) instead --
 * in THIS test group, because `--function=VWMA` matches on the DO_TEST tag and
 * would otherwise reach no period-1 value assertion at all (the enum-wide gate
 * in test_period_boundary.c is tagged PERIOD1/BOUNDARY; it is the broader net,
 * not a substitute for VWMA's own).
 */

/* VWMA period grid: the boundary period, the two oracle-vector periods, the
 * shipped default, and a long window. Starts at 2 -- see (1b) for 1. */
static const int vwmaGrid[] = { 2, 3, 4, 5, 14, 30, 100 };
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

/* (1b) PERIOD 1: the no-smoothing copy, on data (1) cannot use.
 *
 * Two properties the composition above cannot state, both bit-exact:
 *   - the output IS the input, on a series where (P*V)/V is demonstrably not
 *     (the assertion below fails if that stops being true of this data), and
 *   - a zero-volume bar carries the price, not the NaN a period >= 2 window of
 *     zero volumes gives -- one bar's weight cancels.
 */
static ErrorNumber test_vwma_period_one( void )
{
#define P1_N 120
   static TA_Real in[P1_N], vol[P1_N], out[OUT_CAP];
   int i, nbNaive = 0;
   TA_RetCode rc;
   TA_Integer beg, nb;

   /* Two-decimal prices are not dyadic, so P spends a full mantissa; a
    * six-digit integer volume pushes the product past it and the division
    * cannot come back. Bar 7 has no volume at all. */
   for( i = 0; i < P1_N; i++ )
   {
      in[i]  = (TA_Real)(10000 + (i * 7919) % 20000) / 100.0;
      vol[i] = (TA_Real)(100003 + (i * 104729) % 899993);
      if( (in[i] * vol[i]) / vol[i] != in[i] )
         nbNaive++;
   }
   vol[7] = 0.0;

   if( nbNaive < 3 )
   {
      printf( "VWMA period-1 Fail: this data no longer discriminates -- (P*V)/V "
              "returns P on all but %d of %d bars\n", nbNaive, P1_N );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( TA_VWMA_Lookback( 1 ) != 0 )
   {
      printf( "VWMA period-1 Fail: TA_VWMA_Lookback(1) = %d, expected 0\n",
              TA_VWMA_Lookback( 1 ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   rc = TA_VWMA( 0, P1_N - 1, in, vol, 1, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 0 || nb != P1_N )
   {
      printf( "VWMA period-1 Fail: rc=%d beg=%d nb=%d expected 0/0/%d\n",
              (int)rc, (int)beg, (int)nb, P1_N );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nb; i++ )
      if( memcmp( &out[i], &in[i], sizeof(double) ) != 0 )
      {
         printf( "VWMA period-1 Fail at out[%d]: got %.17g expected %.17g%s\n",
                 i, out[i], in[i], i == 7 ? " (the zero-volume bar)" : "" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   return TA_TEST_PASS;
#undef P1_N
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
 * functions where this invariant is currently broken.
 *
 * Run at period 1 as well as the default: the period-1 copy is its own loop,
 * writing the slot it just read, so it is a second aliasing surface and not
 * covered by the windowed path above it. */
static const int vwmaInPlaceGrid[] = { 1, 30 };

static ErrorNumber test_vwma_inplace( const TA_History *history )
{
   int i, nbBars;
   unsigned int g;
   TA_RetCode rc;
   TA_Integer begR, nbR, beg, nb;
   static TA_Real ref[OUT_CAP];
   static TA_Real work[OUT_CAP];
   static TA_Real vol[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < sizeof(vwmaInPlaceGrid)/sizeof(vwmaInPlaceGrid[0]); g++ )
   {
      int period = vwmaInPlaceGrid[g];

      rc = TA_VWMA( 0, nbBars - 1, history->close, history->volume, period, &begR, &nbR, ref );
      if( rc != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      /* (a) output aliased over the price input. */
      for( i = 0; i < nbBars; i++ ) work[i] = history->close[i];
      rc = TA_VWMA( 0, nbBars - 1, work, history->volume, period, &beg, &nb, work );
      if( rc != TA_SUCCESS || beg != begR || nb != nbR )
      {
         printf( "VWMA in-place(price) Fail [period %d]: rc=%d range(%d,%d) vs (%d,%d)\n",
                 period, (int)rc, (int)beg, (int)nb, (int)begR, (int)nbR );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
         if( memcmp( &work[i], &ref[i], sizeof(double) ) != 0 )
         {
            printf( "VWMA in-place(price) Fail [period %d] at out[%d]: got %.17g expected %.17g\n",
                    period, i, work[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

      /* (b) output aliased over the volume input. Not at period 1: that path
       * never reads inVolume, so the check would be true by construction. */
      if( period == 1 )
         continue;
      for( i = 0; i < nbBars; i++ ) vol[i] = history->volume[i];
      rc = TA_VWMA( 0, nbBars - 1, history->close, vol, period, &beg, &nb, vol );
      if( rc != TA_SUCCESS || beg != begR || nb != nbR )
      {
         printf( "VWMA in-place(volume) Fail [period %d]: rc=%d range(%d,%d) vs (%d,%d)\n",
                 period, (int)rc, (int)beg, (int)nb, (int)begR, (int)nbR );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
         if( memcmp( &vol[i], &ref[i], sizeof(double) ) != 0 )
         {
            printf( "VWMA in-place(volume) Fail [period %d] at out[%d]: got %.17g expected %.17g\n",
                    period, i, vol[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
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

/* ==================== HMA ====================
 *
 * HMA(n) = WMA( 2*WMA(close, n/2) - WMA(close, n), (int)sqrt(n) ), both derived
 * periods TRUNCATED -- the author's Integer(); see hma.c and issue #139.
 *
 * The shipped hma() is a FUSED single pass (three interleaved WMA rolling
 * sums + a sqrt(n)-sized ring, issue #139), while the differential below
 * re-composes the formula through the PUBLIC TA_WMA at the fused loop's own
 * anchor -- two genuinely independent code paths that must agree BIT-FOR-BIT,
 * because the fused loop reproduces TA_WMA's exact accumulation order per
 * stage. Formula correctness additionally comes from the external oracle (2)
 * and the Tulip vector (3), which a differential cannot provide (both sides
 * could share the same wrong formula).
 *
 * The sub-ranges matter: TA_WMA carries periodSum/periodSub accumulators from
 * its (clamped) startIdx, so its output at a given bar depends at ULP scale on
 * where the call started (TA_STABLE_EPSILON class, ~1e-13 relative -- MEASURED
 * 233/233 bars differ bitwise for WMA(10) started at bar 19 vs bar 9). The
 * composition below therefore anchors the two inner WMAs at period-1, exactly
 * where a full-range hma() anchors them internally. If this differential ever
 * needs a tolerance, the implementation has drifted from the composition --
 * do not paper over it with an epsilon.
 */

/* HMA period grid: 2 and 3 drive halfPeriod/sqrtPeriod to 1 (TA_WMA's
 * period==1 identity short-circuit); 4 is the smallest fully-weighted case;
 * 5 matches the Tulip vector; 9, 16, 25 and 100 have exact integer square
 * roots (the truncation boundary); 20 is the shipped default (author's); 30
 * is the TA-Lib MA-family default. */
static const int hmaGrid[] = { 2, 3, 4, 5, 9, 16, 20, 25, 30, 100 };
#define NB_HMA_GRID (sizeof(hmaGrid)/sizeof(hmaGrid[0]))

/* HMA external-oracle golden values.
 *
 * Source: pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1), on the
 * standard 252-bar close series (TA_SREF_close_daily_ref_0_PRIV),
 * optInTimePeriod = 20. outBegIdx = 22, nb = 230. Independently reproduced
 * from scratch (fresh rolling dot-product WMAs, no TA-Lib code) to a measured
 * max relative error of 2.7e-16 across all six -- ULP-level agreement of two
 * structurally different implementations.
 *
 * idx is the OUTPUT-array index (0 == global bar 22). */
static const struct { int idx; double value; } hmaOracle[] =
{
   {   0,  87.94363051948052  },
   {   1,  86.58896103896105  },
   {  57,  98.18782943722945  },
   { 115, 136.34847445887448  },
   { 172, 115.81672034632035  },
   { 229, 108.51976017316020  },
};
#define NB_HMA_ORACLE (sizeof(hmaOracle)/sizeof(hmaOracle[0]))

#define HMA_ORACLE_EXPECTED_BEG 22
#define HMA_ORACLE_EXPECTED_NB  230
#define HMA_ORACLE_PERIOD       20
/* Relative tolerance 1e-12 against a measured 2.7e-16 agreement -- a ~3000x
 * margin for cross-platform rounding, still far tighter than any formula error
 * (a rounding-convention mix-up on the derived periods moves values by 1e-3
 * relative and shifts the lookback). Absolute floor 1e-9: HMA tracks price and
 * never approaches zero on this series; the floor keeps a future near-zero
 * golden from becoming a spurious failure. See checkOracleValue(). */
#define HMA_ORACLE_TOL 1e-12
#define HMA_ORACLE_ABS 1e-9

/* (1) DIFFERENTIAL: hma() == WMA(2*WMA(h) - WMA(n), s) composed through the
 * public TA_WMA at hma()'s own internal sub-ranges, bit-for-bit. */
static ErrorNumber test_hma_differential( const TA_History *history )
{
   unsigned int g;
   int i, nbBars, halfPeriod, sqrtPeriod, wmaStart;
   TA_RetCode rcH, rcF, rcHalf, rcS;
   TA_Integer begH, nbH, begF, nbF, begHalf, nbHalf, begS, nbS;
   static TA_Real outHMA[OUT_CAP];
   static TA_Real outFull[OUT_CAP];
   static TA_Real outHalf[OUT_CAP];
   static TA_Real diffSeries[OUT_CAP];
   static TA_Real outSmooth[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < NB_HMA_GRID; g++ )
   {
      int period = hmaGrid[g];
      halfPeriod = period / 2;
      sqrtPeriod = (int)sqrt((double)period);
      /* A full-range hma() clamps startIdx to its lookback (period+sqrt-2) and
       * anchors both inner WMAs sqrt-1 bars earlier: at period-1 exactly. */
      wmaStart = period - 1;

      rcH = TA_HMA( 0, nbBars - 1, history->close, period, &begH, &nbH, outHMA );

      /* Test-only reference: three calls to the shipped TA_WMA plus the de-lag
       * combine. No new numerical logic -- only primitives already proven by
       * the bitwise cross-language gate, the differential fuzz and the
       * hardcoded expected values. */
      rcF    = TA_WMA( wmaStart, nbBars - 1, history->close, period,
                       &begF, &nbF, outFull );
      rcHalf = TA_WMA( wmaStart, nbBars - 1, history->close, halfPeriod,
                       &begHalf, &nbHalf, outHalf );

      if( rcH != TA_SUCCESS || rcF != TA_SUCCESS || rcHalf != TA_SUCCESS )
      {
         printf( "HMA differential Fail [period %d]: retCode HMA=%d WMA=%d/%d\n",
                 period, (int)rcH, (int)rcF, (int)rcHalf );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* wmaStart >= both inner lookbacks, so both calls start exactly there. */
      if( begF != wmaStart || begHalf != wmaStart || nbF != nbHalf )
      {
         printf( "HMA differential Fail [period %d]: inner ranges (%d,%d)/(%d,%d) "
                 "expected beg %d\n",
                 period, (int)begF, (int)nbF, (int)begHalf, (int)nbHalf, wmaStart );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      /* De-lag combine, the same expression hma() fuses. */
      for( i = 0; i < nbF; i++ )
         diffSeries[i] = 2.0*outHalf[i] - outFull[i];

      rcS = TA_WMA( 0, nbF - 1, diffSeries, sqrtPeriod, &begS, &nbS, outSmooth );
      if( rcS != TA_SUCCESS )
      {
         printf( "HMA differential Fail [period %d]: smoothing retCode %d\n",
                 period, (int)rcS );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      /* Global first output = wmaStart + (sqrt-1) == the HMA lookback. */
      if( begH != wmaStart + begS || nbH != nbS )
      {
         printf( "HMA differential Fail [period %d]: range HMA(%d,%d) vs "
                 "compose(%d,%d)\n",
                 period, (int)begH, (int)nbH, (int)(wmaStart + begS), (int)nbS );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nbH; i++ )
      {
         if( memcmp( &outHMA[i], &outSmooth[i], sizeof(double) ) != 0 )
         {
            printf( "HMA differential Fail [period %d] at out[%d]: "
                    "fused %.17g != compose %.17g (must be BIT-exact)\n",
                    period, i, outHMA[i], outSmooth[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) EXTERNAL ORACLE: the composition is the RIGHT formula -- including the
 * author's truncation of both derived periods -- which (1) cannot show. */
static ErrorNumber test_hma_oracle( const TA_History *history )
{
   unsigned int k;
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real out[OUT_CAP];

   rc = TA_HMA( 0, (int)history->nbBars - 1, history->close,
                HMA_ORACLE_PERIOD, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "HMA oracle Fail: retCode %d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != HMA_ORACLE_EXPECTED_BEG || nb != HMA_ORACLE_EXPECTED_NB )
   {
      printf( "HMA oracle Fail: got beg=%d nb=%d expected %d/%d\n",
              (int)beg, (int)nb, HMA_ORACLE_EXPECTED_BEG, HMA_ORACLE_EXPECTED_NB );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( k = 0; k < NB_HMA_ORACLE; k++ )
   {
      int idx = hmaOracle[k].idx;
      double want = hmaOracle[k].value;
      double got  = out[idx];
      double err; const char *mode;

      if( !checkOracleValue( got, want, HMA_ORACLE_TOL, HMA_ORACLE_ABS, &err, &mode ) )
      {
         printf( "HMA oracle Fail at out[%d]: got %.17g expected %.17g "
                 "(%s=%.3e > rel %.3e / abs %.3e)\n",
                 idx, got, want, mode, err, HMA_ORACLE_TOL, HMA_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) TULIP VECTOR: Tulip Indicators 0.9.2 tests/untest.txt:203, "hma 5" on
 * its 15-bar sample series, published at 3 decimals. Tulip truncates both
 * derived periods exactly as the author does, so this is a second, structurally
 * independent implementation agreeing on the convention. Measured max
 * |TA_HMA - printed| = 4.5e-4, inside the 5e-4 half-ulp of the 3-decimal
 * print. A rounding-convention mix-up moves these values by ~1e-1. */
static ErrorNumber test_hma_tulip_vector( void )
{
   static const double tulipHma5In[] =
   {
      81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99, 84.55, 84.36,
      85.53, 86.54, 86.89, 87.77, 87.29
   };
   static const double tulipHma5Exp[] =
   {
      83.690, 83.038, 83.472, 84.550, 84.835, 85.360, 86.552, 87.346, 87.965,
      87.916
   };
   const int nIn  = (int)(sizeof(tulipHma5In)/sizeof(double));
   const int nExp = (int)(sizeof(tulipHma5Exp)/sizeof(double));
   int i;
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real out[OUT_CAP];

   rc = TA_HMA( 0, nIn - 1, tulipHma5In, 5, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "HMA tulip vector Fail: retCode %d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != TA_HMA_Lookback( 5 ) || nb != nExp )
   {
      printf( "HMA tulip vector Fail: got beg=%d nb=%d expected %d/%d\n",
              (int)beg, (int)nb, TA_HMA_Lookback( 5 ), nExp );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nExp; i++ )
   {
      if( fabs( out[i] - tulipHma5Exp[i] ) > 5e-4 )
      {
         printf( "HMA tulip vector Fail at out[%d]: got %.6f expected %.3f\n",
                 i, out[i], tulipHma5Exp[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Rounding-convention pins. Neither the n=20 pandas oracle nor the n=5
    * vector above can SEE the sqrt-rounding convention (their sqrt truncates
    * and rounds to the same integer), so a round-nearest sabotage would slip
    * past both external legs. n=7 (sqrt 2.65) and n=8 (sqrt 2.83) round UP:
    * the trunc/round last-bar values differ by ~0.1 (88.109365 vs 88.015536
    * at n=7; 88.013870 vs 87.860130 at n=8). Expected values from an
    * independent from-scratch implementation (fresh rolling dot products),
    * which reproduces Tulip's published 3-decimal output exactly; tolerance
    * 1e-9 abs against a measured cross-implementation drift of ~1e-12. */
   {
      static const struct { int period; double lastWant; } convPin[] =
      {
         { 7, 88.10936507936505 },
         { 8, 88.01387037037038 },
      };
      unsigned int k;
      for( k = 0; k < sizeof(convPin)/sizeof(convPin[0]); k++ )
      {
         rc = TA_HMA( 0, nIn - 1, tulipHma5In, convPin[k].period, &beg, &nb, out );
         if( rc != TA_SUCCESS || nb < 1 )
         {
            printf( "HMA convention pin Fail [period %d]: rc=%d nb=%d\n",
                    convPin[k].period, (int)rc, (int)nb );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( fabs( out[nb-1] - convPin[k].lastWant ) > 1e-9 )
         {
            printf( "HMA convention pin Fail [period %d]: last got %.17g "
                    "expected %.17g (truncate, not round, the derived periods)\n",
                    convPin[k].period, out[nb-1], convPin[k].lastWant );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) IN-PLACE ALIASING: outReal == inReal must hold (#130). In the composed
 * form safety comes from the final WMA reading only the scratch buffer; in a
 * fused rolling-sum form it comes from the write cursor trailing every read
 * by the lookback clamp. That margin is exactly ZERO at sqrt(n) == 1 (n = 2
 * and 3): the output store lands on the same slot as the same iteration's
 * trailing read, so ordering (read trailing BEFORE store) is load-bearing --
 * pin those periods alongside the default. */
static ErrorNumber test_hma_inplace( const TA_History *history )
{
   static const int ipGrid[] = { 2, 3, 20 };
   unsigned int g;
   int i, nbBars;
   TA_RetCode rc;
   TA_Integer begR, nbR, beg, nb;
   static TA_Real ref[OUT_CAP];
   static TA_Real work[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < sizeof(ipGrid)/sizeof(ipGrid[0]); g++ )
   {
      int period = ipGrid[g];

      rc = TA_HMA( 0, nbBars - 1, history->close, period, &begR, &nbR, ref );
      if( rc != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      for( i = 0; i < nbBars; i++ ) work[i] = history->close[i];
      rc = TA_HMA( 0, nbBars - 1, work, period, &beg, &nb, work );
      if( rc != TA_SUCCESS || beg != begR || nb != nbR )
      {
         printf( "HMA in-place Fail [period %d]: rc=%d range(%d,%d) vs (%d,%d)\n",
                 period, (int)rc, (int)beg, (int)nb, (int)begR, (int)nbR );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nb; i++ )
         if( memcmp( &work[i], &ref[i], sizeof(double) ) != 0 )
         {
            printf( "HMA in-place Fail [period %d] at out[%d]: got %.17g expected %.17g\n",
                    period, i, work[i], ref[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   return TA_TEST_PASS;
}

/* (5) MATYPE ARM (#139): TA_MAType_HMA == 9 dispatches to the same code.
 * (a) MA(period, HMA) == TA_HMA(period), bit-for-bit, across the grid;
 * (b) MA(1, HMA) takes the documented identity path (copy of the input),
 *     BEFORE the dispatch -- so hma()'s own period floor of 2 is never hit;
 * (c) BBANDS middle band with TA_MAType_HMA == TA_HMA, bit-for-bit, through
 *     the abstract-visible param surface (smoke for every other MAType taker).
 */
static ErrorNumber test_hma_matype( const TA_History *history )
{
   unsigned int g;
   int i, nbBars;
   TA_RetCode rcM, rcH;
   TA_Integer begM, nbM, begH, nbH;
   static TA_Real outMA[OUT_CAP];
   static TA_Real outHMA[OUT_CAP];
   static TA_Real outUpper[OUT_CAP];
   static TA_Real outMiddle[OUT_CAP];
   static TA_Real outLower[OUT_CAP];

   nbBars = (int)history->nbBars;

   /* (a) dispatch parity across the period grid. */
   for( g = 0; g < NB_HMA_GRID; g++ )
   {
      int period = hmaGrid[g];

      rcM = TA_MA( 0, nbBars - 1, history->close, period, TA_MAType_HMA,
                   &begM, &nbM, outMA );
      rcH = TA_HMA( 0, nbBars - 1, history->close, period, &begH, &nbH, outHMA );

      if( rcM != TA_SUCCESS || rcH != TA_SUCCESS )
      {
         printf( "HMA matype Fail [period %d]: retCode MA=%d HMA=%d\n",
                 period, (int)rcM, (int)rcH );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begM != begH || nbM != nbH )
      {
         printf( "HMA matype Fail [period %d]: range MA(%d,%d) HMA(%d,%d)\n",
                 period, (int)begM, (int)nbM, (int)begH, (int)nbH );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbM; i++ )
         if( memcmp( &outMA[i], &outHMA[i], sizeof(double) ) != 0 )
         {
            printf( "HMA matype Fail [period %d] at out[%d]: "
                    "MA %.17g != HMA %.17g (must be BIT-exact)\n",
                    period, i, outMA[i], outHMA[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   /* (b) MA(1, HMA): the identity path runs before the dispatch. */
   rcM = TA_MA( 0, nbBars - 1, history->close, 1, TA_MAType_HMA,
                &begM, &nbM, outMA );
   if( rcM != TA_SUCCESS || begM != 0 || nbM != nbBars )
   {
      printf( "HMA matype Fail: MA(1,HMA) rc=%d beg=%d nb=%d expected 0/%d\n",
              (int)rcM, (int)begM, (int)nbM, nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nbBars; i++ )
      if( memcmp( &outMA[i], &history->close[i], sizeof(double) ) != 0 )
      {
         printf( "HMA matype Fail: MA(1,HMA) at out[%d] not an input copy\n", i );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   /* (c) BBANDS(20, 2, 2, HMA): the middle band IS MA(close, 20, HMA). */
   rcM = TA_BBANDS( 0, nbBars - 1, history->close, 20, 2.0, 2.0, TA_MAType_HMA,
                    &begM, &nbM, outUpper, outMiddle, outLower );
   rcH = TA_HMA( 0, nbBars - 1, history->close, 20, &begH, &nbH, outHMA );
   if( rcM != TA_SUCCESS || rcH != TA_SUCCESS || begM != begH || nbM != nbH )
   {
      printf( "HMA matype Fail: BBANDS(HMA) rc=%d range(%d,%d) vs HMA rc=%d (%d,%d)\n",
              (int)rcM, (int)begM, (int)nbM, (int)rcH, (int)begH, (int)nbH );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbM; i++ )
      if( memcmp( &outMiddle[i], &outHMA[i], sizeof(double) ) != 0 )
      {
         printf( "HMA matype Fail: BBANDS(HMA) middle[%d] %.17g != HMA %.17g\n",
                 i, outMiddle[i], outHMA[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

   return TA_TEST_PASS;
}

/* (6) SINGLE-ELEMENT RANGE: startIdx == endIdx == last bar. TA_WMA carries
 * running accumulators from its clamped startIdx, so a late-start hma() is
 * NOT bit-identical to the full-range call at the same bar (TA_STABLE_EPSILON
 * class) -- compare at the class tolerance, not memcmp. */
static ErrorNumber test_hma_single_element( const TA_History *history )
{
   /* 20 exercises the general regime; 2 the degenerate (n<=3) arm, whose
    * late-start indexing is otherwise untested (every other degenerate leg
    * calls with startIdx 0). */
   static const int seGrid[] = { 2, 20 };
   unsigned int g;
   int nbBars;
   TA_RetCode rc;
   TA_Integer begR, nbR, beg, nb;
   double want, got, relErr;
   static TA_Real ref[OUT_CAP];
   static TA_Real out[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < sizeof(seGrid)/sizeof(seGrid[0]); g++ )
   {
      int period = seGrid[g];

      rc = TA_HMA( 0, nbBars - 1, history->close, period, &begR, &nbR, ref );
      if( rc != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;

      rc = TA_HMA( nbBars - 1, nbBars - 1, history->close, period, &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != nbBars - 1 || nb != 1 )
      {
         printf( "HMA single-element Fail [period %d]: rc=%d beg=%d nb=%d expected %d/1\n",
                 period, (int)rc, (int)beg, (int)nb, nbBars - 1 );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      want = ref[nbR - 1];
      got  = out[0];
      relErr = fabs( got - want ) / fabs( want );
      if( relErr > 1e-9 )
      {
         printf( "HMA single-element Fail [period %d]: got %.17g expected %.17g (rel %.3e)\n",
                 period, got, want, relErr );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (7) PARAMETER FLOOR (#139): the smallest legal period is 1, where HMA is
 * the same no-smoothing copy every TA-Lib moving average performs. 0 is out
 * of every published definition and must be a guarded reject, never a
 * computed answer; the default sentinel must resolve to the documented 20. */
static ErrorNumber test_hma_param_reject( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer beg, nb, i;
   static TA_Real out[OUT_CAP];
   static const int badPeriod[] = { 0, -1, 100001 };
   unsigned int k;

   /* Period 1: bit-exact copy of the requested range, lookback 0. */
   if( TA_HMA_Lookback( 1 ) != 0 )
   {
      printf( "HMA period-1 Fail: TA_HMA_Lookback(1) = %d, expected 0\n",
              TA_HMA_Lookback( 1 ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   rc = TA_HMA( 0, (int)history->nbBars - 1, history->close, 1, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 0 || nb != (TA_Integer)history->nbBars )
   {
      printf( "HMA period-1 Fail: rc=%d beg=%d nb=%d expected 0/0/%d\n",
              (int)rc, (int)beg, (int)nb, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nb; i++ )
   {
      if( out[i] != history->close[i] )
      {
         printf( "HMA period-1 Fail: [%d] got %.17g, expected %.17g\n",
                 (int)i, out[i], history->close[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   for( k = 0; k < sizeof(badPeriod)/sizeof(badPeriod[0]); k++ )
   {
      rc = TA_HMA( 0, (int)history->nbBars - 1, history->close,
                   badPeriod[k], &beg, &nb, out );
      if( rc != TA_BAD_PARAM )
      {
         printf( "HMA param reject Fail: period %d returned %d, expected TA_BAD_PARAM\n",
                 badPeriod[k], (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( TA_HMA_Lookback( badPeriod[k] ) != -1 )
      {
         printf( "HMA param reject Fail: TA_HMA_Lookback(%d) != -1\n", badPeriod[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* The TA_INTEGER_DEFAULT sentinel resolves to the documented default 20. */
   if( TA_HMA_Lookback( TA_INTEGER_DEFAULT ) != TA_HMA_Lookback( 20 ) )
   {
      printf( "HMA param reject Fail: default sentinel lookback %d != lookback(20) %d\n",
              TA_HMA_Lookback( TA_INTEGER_DEFAULT ), TA_HMA_Lookback( 20 ) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   rc = TA_HMA( 0, (int)history->nbBars - 1, history->close,
                TA_INTEGER_DEFAULT, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != TA_HMA_Lookback( 20 ) )
   {
      printf( "HMA param reject Fail: default sentinel rc=%d beg=%d\n", (int)rc, (int)beg );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   return TA_TEST_PASS;
}

/* (8) LARGE PERIODS -- the scratch-allocation regimes (#139).
 *
 * The fused implementation's scratch is the de-lag ring of sqrt(n)-1 slots
 * (CIRCBUF): STACK while that fits the 50-slot prolog, i.e. every
 * n <= 2703, and TA_Malloc from n = 2704 (sqrt = 52, ring = 51) up --
 * verified against the macro's `Size > capacity` test, NOT assumed from
 * sqrt(n) alone (an off-by-one here once made this leg silently stack-only).
 * Pin BOTH sides of the true boundary with the same bitwise WMA-composition
 * differential used at small periods, plus an in-place run per period.
 *
 * Synthetic deterministic series: 2860 bars of a trending sine, enough for
 * n=2704 (lookback 2754) to emit 106 outputs. */
#define HMA_LP_BARS 2860
static ErrorNumber test_hma_large_period( void )
{
   static TA_Real lpIn[HMA_LP_BARS];
   static TA_Real lpHMA[HMA_LP_BARS];
   static TA_Real lpFull[HMA_LP_BARS];
   static TA_Real lpHalf[HMA_LP_BARS];
   static TA_Real lpDiff[HMA_LP_BARS];
   static TA_Real lpSmooth[HMA_LP_BARS];
   static TA_Real lpWork[HMA_LP_BARS];
   /* 2703: sqrt = 51, ring = 50 -- the LAST stack-CIRCBUF period;
    * 2704: sqrt = 52, ring = 51 -- the FIRST TA_Malloc period. */
   static const int lpGrid[] = { 2703, 2704 };
   unsigned int g;
   int i, halfPeriod, sqrtPeriod, wmaStart;
   TA_RetCode rc, rcF, rcH, rcS;
   TA_Integer begV, nbV, begF, nbF, begH, nbH, begS, nbS;

   for( i = 0; i < HMA_LP_BARS; i++ )
      lpIn[i] = 100.0 + 25.0 * sin( 0.05 * (double)i ) + 0.01 * (double)i;

   for( g = 0; g < sizeof(lpGrid)/sizeof(lpGrid[0]); g++ )
   {
      int period = lpGrid[g];
      halfPeriod = period / 2;
      sqrtPeriod = (int)sqrt((double)period);
      wmaStart   = period - 1;

      rc = TA_HMA( 0, HMA_LP_BARS - 1, lpIn, period, &begV, &nbV, lpHMA );
      if( rc != TA_SUCCESS || nbV < 1 )
      {
         printf( "HMA large-period Fail [n=%d]: rc=%d nb=%d\n", period, (int)rc, (int)nbV );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begV != period + sqrtPeriod - 2 )
      {
         printf( "HMA large-period Fail [n=%d]: beg=%d expected %d\n",
                 period, (int)begV, period + sqrtPeriod - 2 );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      /* Same three-TA_WMA composition as the small-period differential. */
      rcF = TA_WMA( wmaStart, HMA_LP_BARS - 1, lpIn, period, &begF, &nbF, lpFull );
      rcH = TA_WMA( wmaStart, HMA_LP_BARS - 1, lpIn, halfPeriod, &begH, &nbH, lpHalf );
      if( rcF != TA_SUCCESS || rcH != TA_SUCCESS || begF != begH || nbF != nbH )
      {
         printf( "HMA large-period Fail [n=%d]: inner WMA rc=%d/%d\n",
                 period, (int)rcF, (int)rcH );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nbF; i++ )
         lpDiff[i] = 2.0*lpHalf[i] - lpFull[i];
      rcS = TA_WMA( 0, nbF - 1, lpDiff, sqrtPeriod, &begS, &nbS, lpSmooth );
      if( rcS != TA_SUCCESS || begV != wmaStart + begS || nbV != nbS )
      {
         printf( "HMA large-period Fail [n=%d]: compose range (%d,%d) vs (%d,%d)\n",
                 period, (int)(wmaStart + begS), (int)nbS, (int)begV, (int)nbV );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbV; i++ )
         if( memcmp( &lpHMA[i], &lpSmooth[i], sizeof(double) ) != 0 )
         {
            printf( "HMA large-period Fail [n=%d] at out[%d]: %.17g != %.17g "
                    "(must be BIT-exact)\n", period, i, lpHMA[i], lpSmooth[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

      /* In-place over the same period: the scratch-allocation path must not
       * change the aliasing contract. */
      for( i = 0; i < HMA_LP_BARS; i++ ) lpWork[i] = lpIn[i];
      rc = TA_HMA( 0, HMA_LP_BARS - 1, lpWork, period, &begF, &nbF, lpWork );
      if( rc != TA_SUCCESS || begF != begV || nbF != nbV )
      {
         printf( "HMA large-period in-place Fail [n=%d]: rc=%d range(%d,%d)\n",
                 period, (int)rc, (int)begF, (int)nbF );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbV; i++ )
         if( memcmp( &lpWork[i], &lpHMA[i], sizeof(double) ) != 0 )
         {
            printf( "HMA large-period in-place Fail [n=%d] at out[%d]\n", period, i );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
   }

   return TA_TEST_PASS;
}

/* ==========================================================================
 * EFI -- Elder's Force Index, EMA( (close - prevClose) * volume ).
 *
 * Composed of three shipped functions: TA_MOM at period 1 for the close
 * change, TA_MULT against the volume of the same bar, TA_EMA for the
 * smoothing. The fused loop performs exactly those operations in exactly that
 * order, so leg (1) is a memcmp.
 *
 * Index bookkeeping, since it is the easiest thing to get wrong here: the
 * force series is one shorter than the bar series and force[k] belongs to bar
 * k+1, there being no close change on the first bar. Every comparison below
 * therefore offsets the reference call by one bar, and the ranges are checked
 * as well as the values -- a reference silently one bar out would otherwise
 * agree on a shifted series that is nearly as smooth.
 * ========================================================================== */

/* Periods across the branches: 1 (the raw Force Index, its own copy loop), 2
 * (Elder's short-term reading and the shortest smoothing that recurses), 13
 * (his intermediate-term default) and 30. */
static const int efiGrid[] = { 1, 2, 13, 30 };
#define NB_EFI_GRID (sizeof(efiGrid)/sizeof(efiGrid[0]))

/* The EMA unstable period shifts the lookback of both sides; sweeping it
 * proves EFI routes TA_FUNC_UNST_EMA the same way the composition does rather
 * than ignoring it. */
static const unsigned int efiUnstGrid[] = { 0, 5 };
#define NB_EFI_UNST (sizeof(efiUnstGrid)/sizeof(efiUnstGrid[0]))

static const int efiStartGrid[] = { 0, 1, 40, 200 };
#define NB_EFI_START (sizeof(efiStartGrid)/sizeof(efiStartGrid[0]))

/* External oracle: pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1),
 * efi(close, volume, length=N, mamode="ema") -- the identical call the
 * ta_pandas_serve arm makes -- on the standard 252-bar close/volume series,
 * TA_FUNC_UNST_EMA = 0. idx below is the BAR index.
 *
 * These are the oracle's OWN doubles, transcribed at the 17 digits that
 * round-trip. They are NOT what TA_EFI returns: pandas' ewm computes
 * alpha*x + (1-alpha)*prev where TA-Lib computes fma(x-prev, alpha, prev), and
 * the seed sums differ in order (pandas .mean() sums pairwise), so the two
 * trajectories separate in the last bits. Measured divergence at these bars is
 * 1.4e-16 .. 7.0e-16 relative, and 1.4e-14 over all 239 values at period 13.
 * Never make this leg bitwise -- pinning TA-Lib's own arithmetic here would
 * turn the only formula-constraining leg into a restatement of leg (1).
 *
 * Single-arm, and the arm is not fully independent: Tulip ships no force index
 * at all, and pandas-ta-classic's EMA carries an explicit SMA-seed patch to
 * reproduce TA-Lib's warm-up anchor (overlap/ema.py). So agreement on WHERE the
 * output starts is by construction. What this leg does corroborate
 * independently is the algebra -- close.diff(1) * volume smoothed over the
 * whole trajectory -- which is the part leg (1) cannot see, since both of its
 * sides would share any wrong formula.
 *
 * Two periods: 13 (Elder's intermediate-term default) and 2 (his short-term
 * reading, and the shortest period that still recurses). */
static const struct { int period; int beg; int nb; } efiOracleCase[] =
{
   { 13, 13, 239 },
   {  2,  2, 250 },
};
#define NB_EFI_ORACLE_CASE (sizeof(efiOracleCase)/sizeof(efiOracleCase[0]))

static const struct { int period; int idx; double value; } efiOracle[] =
{
   { 13,  13,  -9561925.384615384    },
   { 13,  14,  -6316864.615384616    },
   { 13,  50,    728376.35804008192  },
   { 13, 125,   8077682.3251350606   },
   { 13, 200, -10621261.764120152    },
   { 13, 251,   -823984.84225067939  },

   {  2,   2,   7163838.25           },
   {  2,   3,   4382490.0833333302   },
   {  2,  50,  -3434299.4893575865   },
   {  2, 125,   8329032.2864905726   },
   {  2, 200,   -282589.81305174041  },
   {  2, 251,  -1994114.3091605683   },
};
#define NB_EFI_ORACLE (sizeof(efiOracle)/sizeof(efiOracle[0]))
#define EFI_ORACLE_TOL    1e-12
/* Values run to ~1e7; the absolute floor only guards a crossing that lands on
 * an exact zero, which this corpus does not produce. */
#define EFI_ORACLE_ABS    1e-9

/* Build the force series with shipped primitives only: TA_MOM for the change,
 * TA_MULT against the volume of the bar the change lands on. Returns the
 * number of force values, or -1 on any failure (already reported). */
static int efi_build_force( const TA_History *history, TA_Real *force )
{
   static TA_Real mom[OUT_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb, begX, nbX;
   int nbBars = (int)history->nbBars;

   rc = TA_MOM( 0, nbBars - 1, history->close, 1, &beg, &nb, mom );
   if( rc != TA_SUCCESS || beg != 1 || nb != nbBars - 1 )
   {
      printf( "EFI reference Fail: TA_MOM rc=%d beg=%d nb=%d (expected 1/%d)\n",
              (int)rc, (int)beg, (int)nb, nbBars - 1 );
      return -1;
   }

   /* mom[k] is the change into bar k+1, so it pairs with volume[k+1]. */
   rc = TA_MULT( 0, nbBars - 2, mom, &history->volume[1], &begX, &nbX, force );
   if( rc != TA_SUCCESS || begX != 0 || nbX != nbBars - 1 )
   {
      printf( "EFI reference Fail: TA_MULT rc=%d beg=%d nb=%d (expected 0/%d)\n",
              (int)rc, (int)begX, (int)nbX, nbBars - 1 );
      return -1;
   }

   return nbBars - 1;
}

/* (1) DIFFERENTIAL: EFI == EMA( MOM(close,1) * volume ), bit-for-bit, across
 * the period grid, the unstable-period grid and several startIdx values. */
static ErrorNumber test_efi_differential( const TA_History *history )
{
   unsigned int g, u, s;
   int i, nbBars, nbForce;
   TA_RetCode rcF, rcE;
   TA_Integer begF, nbF, begE, nbE;
   static TA_Real force[OUT_CAP];
   static TA_Real outEfi[OUT_CAP];
   static TA_Real outEma[OUT_CAP];

   nbBars  = (int)history->nbBars;
   nbForce = efi_build_force( history, force );
   if( nbForce < 0 )
      return TA_TESTUTIL_TFRR_BAD_RETCODE;

   for( u = 0; u < NB_EFI_UNST; u++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, efiUnstGrid[u] );

      for( s = 0; s < NB_EFI_START; s++ )
      {
         int startIdx = efiStartGrid[s];
         /* Bar startIdx is force index startIdx-1; bar 0 has no force value,
          * so the reference starts at force index 0 either way. */
         int startForce = startIdx > 0 ? startIdx - 1 : 0;

         for( g = 0; g < NB_EFI_GRID; g++ )
         {
            int period = efiGrid[g];

            rcF = TA_EFI( startIdx, nbBars - 1, history->close, history->volume,
                          period, &begF, &nbF, outEfi );
            rcE = TA_EMA( startForce, nbForce - 1, force,
                          period, &begE, &nbE, outEma );

            if( rcF != rcE )
            {
               printf( "EFI differential Fail [unst %u start %d period %d]: "
                       "retCode EFI=%d EMA=%d\n",
                       efiUnstGrid[u], startIdx, period, (int)rcF, (int)rcE );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            if( rcF != TA_SUCCESS )
               continue;

            /* begF is a bar index, begE a force index: they must differ by
             * exactly the one bar the first close change consumes. */
            if( begF != begE + 1 || nbF != nbE )
            {
               printf( "EFI differential Fail [unst %u start %d period %d]: "
                       "range EFI(%d,%d) EMA(%d,%d) -- expected beg %d\n",
                       efiUnstGrid[u], startIdx, period,
                       (int)begF, (int)nbF, (int)begE, (int)nbE, (int)begE + 1 );
               TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            for( i = 0; i < nbF; i++ )
            {
               if( memcmp( &outEfi[i], &outEma[i], sizeof(double) ) != 0 )
               {
                  printf( "EFI differential Fail [unst %u start %d period %d] "
                          "at out[%d]: fused %.17g != compose %.17g "
                          "(must be BIT-exact)\n",
                          efiUnstGrid[u], startIdx, period, i,
                          outEfi[i], outEma[i] );
                  TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
   return TA_TEST_PASS;
}

/* (2) EXTERNAL ORACLE. The only leg that constrains the formula. */
static ErrorNumber test_efi_oracle( const TA_History *history )
{
   unsigned int c, k;
   int nbCompared = 0;
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real out[OUT_CAP];

   for( c = 0; c < NB_EFI_ORACLE_CASE; c++ )
   {
      int period = efiOracleCase[c].period;

      rc = TA_EFI( 0, (int)history->nbBars - 1, history->close, history->volume,
                   period, &beg, &nb, out );
      if( rc != TA_SUCCESS )
      {
         printf( "EFI oracle Fail [period %d]: retCode %d\n", period, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( beg != efiOracleCase[c].beg || nb != efiOracleCase[c].nb )
      {
         printf( "EFI oracle Fail [period %d]: got beg=%d nb=%d expected %d/%d\n",
                 period, (int)beg, (int)nb,
                 efiOracleCase[c].beg, efiOracleCase[c].nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < NB_EFI_ORACLE; k++ )
      {
         int idx;
         double want, got, err;
         const char *mode;

         if( efiOracle[k].period != period )
            continue;

         idx  = efiOracle[k].idx - beg;   /* bar -> output index */
         want = efiOracle[k].value;
         got  = out[idx];

         if( !checkOracleValue( got, want, EFI_ORACLE_TOL, EFI_ORACLE_ABS,
                                &err, &mode ) )
         {
            printf( "EFI oracle Fail [period %d] at bar %d (out[%d]): got %.17g "
                    "expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                    period, efiOracle[k].idx, idx, got, want, mode, err,
                    EFI_ORACLE_TOL, EFI_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         nbCompared++;
      }
   }

   /* Every pinned row must have been reached: a period typo in the table would
    * otherwise silently drop its rows and leave the leg passing on fewer. */
   if( nbCompared != (int)NB_EFI_ORACLE )
   {
      printf( "EFI oracle Fail: compared %d value(s) but the table carries %d\n",
              nbCompared, (int)NB_EFI_ORACLE );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (3) DEGENERATE INPUTS, both exactly zero rather than merely small.
 *
 * There is no division by anything derived from the data -- the only divisor
 * is the period, a positive integer parameter -- so issue #112 holds
 * structurally here and the assertions can be exact equality:
 *   - a flat close makes every force exactly 0.0, and an EMA of exact zeros
 *     stays exactly 0.0 (the seed sum is 0.0 and (0-0)*k+0 is 0);
 *   - zero volume does the same through the multiply, on prices that move. */
static ErrorNumber test_efi_degenerate( void )
{
#define EFI_DEG_N 80
   static TA_Real close[EFI_DEG_N], volume[EFI_DEG_N], out[OUT_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i, c;
   const int period = 13;

   for( c = 0; c < 2; c++ )
   {
      const char *tag = c == 0 ? "flat close" : "zero volume";

      for( i = 0; i < EFI_DEG_N; i++ )
      {
         if( c == 0 )
         {
            close[i]  = 42.5;                        /* never moves */
            volume[i] = 1000.0 + (double)i * 13.0;   /* volume does */
         }
         else
         {
            close[i]  = 42.5 + (double)i * 0.75;     /* price moves */
            volume[i] = 0.0;                         /* nobody traded */
         }
      }

      rc = TA_EFI( 0, EFI_DEG_N - 1, close, volume, period, &beg, &nb, out );
      if( rc != TA_SUCCESS )
      {
         printf( "EFI %s Fail: retCode %d\n", tag, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( beg != period || nb != EFI_DEG_N - period )
      {
         printf( "EFI %s Fail: got beg=%d nb=%d expected %d/%d\n",
                 tag, (int)beg, (int)nb, period, EFI_DEG_N - period );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( i = 0; i < nb; i++ )
      {
         if( out[i] != 0.0 )
         {
            printf( "EFI %s Fail at out[%d]: got %.17g, expected exactly 0\n",
                    tag, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
#undef EFI_DEG_N
}

/* (4) IN-PLACE ALIASING over both inputs.
 *
 * The reason this needs its own leg rather than an argument: at bar t the loop
 * needs close[t-1], and when the output aliases inClose that slot was
 * overwritten one iteration earlier -- reachable at startIdx 1, which period 1
 * allows. The implementation carries prevClose in a scalar so the stale array
 * is never read; this is what proves it, and it would fail loudly if a future
 * rewrite went back to indexing. */
static const int efiInPlaceGrid[] = { 1, 2, 13 };
#define NB_EFI_INPLACE (sizeof(efiInPlaceGrid)/sizeof(efiInPlaceGrid[0]))

static ErrorNumber test_efi_inplace( const TA_History *history )
{
   unsigned int g;
   int i, nbBars;
   TA_RetCode rc;
   TA_Integer begRef, nbRef, begAlias, nbAlias;
   static TA_Real outRef[OUT_CAP];
   static TA_Real work[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < NB_EFI_INPLACE; g++ )
   {
      int period = efiInPlaceGrid[g];
      int startIdx, which;

      /* startIdx 1 puts the first write at the slot holding the close the very
       * next bar needs; startIdx 0 is the ordinary case. */
      for( startIdx = 0; startIdx <= 1; startIdx++ )
      {
         rc = TA_EFI( startIdx, nbBars - 1, history->close, history->volume,
                      period, &begRef, &nbRef, outRef );
         if( rc != TA_SUCCESS )
         {
            printf( "EFI in-place Fail [period %d start %d]: reference rc %d\n",
                    period, startIdx, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( which = 0; which < 2; which++ )
         {
            const char *tag = which == 0 ? "outReal==inClose"
                                         : "outReal==inVolume";

            for( i = 0; i < nbBars; i++ )
               work[i] = which == 0 ? history->close[i] : history->volume[i];

            if( which == 0 )
               rc = TA_EFI( startIdx, nbBars - 1, work, history->volume,
                            period, &begAlias, &nbAlias, work );
            else
               rc = TA_EFI( startIdx, nbBars - 1, history->close, work,
                            period, &begAlias, &nbAlias, work );

            if( rc != TA_SUCCESS )
            {
               printf( "EFI in-place Fail [period %d start %d, %s]: rc %d\n",
                       period, startIdx, tag, (int)rc );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            if( begAlias != begRef || nbAlias != nbRef )
            {
               printf( "EFI in-place Fail [period %d start %d, %s]: "
                       "range (%d,%d) vs (%d,%d)\n",
                       period, startIdx, tag, (int)begAlias, (int)nbAlias,
                       (int)begRef, (int)nbRef );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            for( i = 0; i < nbRef; i++ )
            {
               if( memcmp( &work[i], &outRef[i], sizeof(double) ) != 0 )
               {
                  printf( "EFI in-place Fail [period %d start %d, %s] at "
                          "out[%d]: got %.17g expected %.17g\n",
                          period, startIdx, tag, i, work[i], outRef[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* ==========================================================================
 * QSTICK -- Chande & Kroll's average candle body, SMA( close - open ).
 *
 * Its fused loop is sma.c with inReal[x] replaced by (inClose[x] - inOpen[x])
 * and nothing else changed, which is what makes leg (1) a memcmp rather than a
 * tolerance: TA_SUB performs the same single subtraction, and TA_SMA then runs
 * the identical accumulate / capture / subtract-trailing / divide sequence.
 * ========================================================================== */

/* Periods spanning the branches that exist: 1 (lookback 0, the seeding while
 * loop is skipped), 2 (the shortest window that seeds), the usual defaults, and
 * 100 -- well past any bar count the corpus warms up with. */
static const int qstickGrid[] = { 1, 2, 3, 5, 8, 10, 14, 30, 100 };
#define NB_QSTICK_GRID (sizeof(qstickGrid)/sizeof(qstickGrid[0]))

/* startIdx values for the same differential. A non-zero startIdx moves the
 * trailing index off the array head, which is where an off-by-one in the
 * seeding loop hides -- at startIdx 0 the seed window and the output window
 * begin at the same place and a swap of the two is invisible. */
static const int qstickStartGrid[] = { 0, 1, 40, 200 };
#define NB_QSTICK_START (sizeof(qstickStartGrid)/sizeof(qstickStartGrid[0]))

/* External oracle, arm 1 of 2: Achelis, Technical Analysis from A to Z, page
 * 280, transcribed in Tulip Indicators 0.9.2 tests/atoz.txt:187 ("qstick 4").
 *
 * Pinned EXACTLY, not at a tolerance. Every price on that page is a sixteenth,
 * so every body, every window sum and every quotient (the divisor 4 is a power
 * of two) is dyadic and representable -- the whole vector is computed without a
 * single rounding. The book prints four decimals; the values below are the
 * exact binary results and round to precisely the digits it prints:
 * .7969 .4688 -.2031 .0156 .1875. */
static const double qstickBookOpen[]  =
   { 62.5625, 64.625, 63.5625, 63.9375, 64.5, 65.1875, 60.5625, 62.25 };
static const double qstickBookClose[] =
   { 64.5625, 64.125, 64.3125, 64.875, 65.1875, 62, 62.1875, 63.875 };
static const double qstickBookExp[]   =
   { 0.796875, 0.46875, -0.203125, 0.015625, 0.1875 };
#define NB_QSTICK_BOOK (sizeof(qstickBookExp)/sizeof(qstickBookExp[0]))

/* External oracle, arm 2 of 2: Tulip Indicators 0.9.2 tests/untest.txt:327
 * ("qstick 5"), 15 bars in, 11 values out. Ordinary two-decimal prices, so
 * unlike the book vector this one does round; the values below were recomputed
 * at full precision from those inputs and agree with all three decimals Tulip
 * prints (0.304 0.304 0.358 0.352 0.404 0.324 0.676 0.868 0.752 0.636 0.552).
 *
 * Worth having in addition to the book vector: Tulip's own loop finishes with
 * sum * (1.0/period) where this one divides, so an implementation that switched
 * to the reciprocal multiply would still pass here but would stop being
 * bit-exact in leg (1). The two legs constrain different things. */
static const double qstickTulipOpen[]  =
   { 81.85, 81.20, 81.55, 82.91, 83.10, 83.41, 82.71, 82.70,
     84.20, 84.25, 84.03, 85.45, 86.18, 88.00, 87.60 };
static const double qstickTulipClose[] =
   { 81.59, 81.06, 82.87, 83.00, 83.61, 83.15, 82.84, 83.99,
     84.55, 84.36, 85.53, 86.54, 86.89, 87.77, 87.29 };
static const double qstickTulipExp[]   =
   { 0.3040000000000049,  0.3040000000000049,  0.3580000000000069,
     0.35200000000000387, 0.404000000000002,   0.3240000000000009,
     0.675999999999999,   0.8679999999999979,  0.7519999999999982,
     0.6359999999999986,  0.552000000000001 };
#define NB_QSTICK_TULIP (sizeof(qstickTulipExp)/sizeof(qstickTulipExp[0]))

/* Away from zero the relative band is what matters; QSTICK crosses zero by
 * design, so an absolute floor is needed for the values that sit near it. */
#define QSTICK_ORACLE_TOL 1e-12
#define QSTICK_ORACLE_ABS 1e-14

/* (1) DIFFERENTIAL: QSTICK == SMA( SUB(close, open) ), bit-for-bit. */
static ErrorNumber test_qstick_differential( const TA_History *history )
{
   unsigned int g, s;
   int i, nbBars;
   TA_RetCode rcQ, rcS, rcM;
   TA_Integer begQ, nbQ, begS, nbS, begM, nbM;
   static TA_Real body[OUT_CAP];
   static TA_Real outQstick[OUT_CAP];
   static TA_Real outSma[OUT_CAP];

   nbBars = (int)history->nbBars;

   /* The body series, from the shipped TA_SUB rather than an inline loop: the
    * reference must contain no new numerical logic of its own. */
   rcS = TA_SUB( 0, nbBars - 1, history->close, history->open,
                 &begS, &nbS, body );
   if( rcS != TA_SUCCESS || begS != 0 || nbS != nbBars )
   {
      printf( "QSTICK differential Fail: TA_SUB rc=%d beg=%d nb=%d (expected 0/%d)\n",
              (int)rcS, (int)begS, (int)nbS, nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( s = 0; s < NB_QSTICK_START; s++ )
   {
      int startIdx = qstickStartGrid[s];

      for( g = 0; g < NB_QSTICK_GRID; g++ )
      {
         int period = qstickGrid[g];

         rcQ = TA_QSTICK( startIdx, nbBars - 1, history->open, history->close,
                          period, &begQ, &nbQ, outQstick );

         /* The reference: one call to the shipped TA_SMA over the body series.
          * Note it is fed the SAME startIdx, so the lookback clamp is compared
          * too, not just the values. */
         rcM = TA_SMA( startIdx, nbBars - 1, body, period, &begM, &nbM, outSma );

         if( rcQ != rcM )
         {
            printf( "QSTICK differential Fail [start %d period %d]: "
                    "retCode QSTICK=%d SMA=%d\n",
                    startIdx, period, (int)rcQ, (int)rcM );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( rcQ != TA_SUCCESS )
            continue;

         if( begQ != begM || nbQ != nbM )
         {
            printf( "QSTICK differential Fail [start %d period %d]: "
                    "range QSTICK(%d,%d) SMA(%d,%d)\n",
                    startIdx, period, (int)begQ, (int)nbQ, (int)begM, (int)nbM );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbQ; i++ )
         {
            if( memcmp( &outQstick[i], &outSma[i], sizeof(double) ) != 0 )
            {
               printf( "QSTICK differential Fail [start %d period %d] at out[%d]: "
                       "fused %.17g != compose %.17g (must be BIT-exact)\n",
                       startIdx, period, i, outQstick[i], outSma[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) EXTERNAL ORACLE. Leg (1) proves the fused loop matches the composition;
 * it cannot prove the composition is Qstick, since both sides would share a
 * wrong formula. These two published vectors do -- and between them they also
 * pin the argument order, because open and close enter asymmetrically: swap
 * them and every value negates. */
static ErrorNumber qstick_check_vector( const char *tag,
                                        const double *open, const double *close,
                                        int nbIn, int period,
                                        const double *expected, int nbExpected,
                                        int exact )
{
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i;
   static TA_Real out[OUT_CAP];

   rc = TA_QSTICK( 0, nbIn - 1, open, close, period, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "QSTICK %s Fail: retCode %d\n", tag, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != period - 1 || nb != nbExpected )
   {
      printf( "QSTICK %s Fail: got beg=%d nb=%d expected %d/%d\n",
              tag, (int)beg, (int)nb, period - 1, nbExpected );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbExpected; i++ )
   {
      if( exact )
      {
         /* Dyadic inputs, dyadic divisor: anything but equality is a defect. */
         if( memcmp( &out[i], &expected[i], sizeof(double) ) != 0 )
         {
            printf( "QSTICK %s Fail at out[%d]: got %.17g expected %.17g "
                    "(this vector is exact in binary)\n",
                    tag, i, out[i], expected[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      else
      {
         double err; const char *mode;
         if( !checkOracleValue( out[i], expected[i],
                                QSTICK_ORACLE_TOL, QSTICK_ORACLE_ABS,
                                &err, &mode ) )
         {
            printf( "QSTICK %s Fail at out[%d]: got %.17g expected %.17g "
                    "(%s=%.3e > rel %.3e / abs %.3e)\n",
                    tag, i, out[i], expected[i], mode, err,
                    QSTICK_ORACLE_TOL, QSTICK_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_qstick_oracle( void )
{
   ErrorNumber retValue;

   retValue = qstick_check_vector( "book vector (Achelis p.280)",
                                   qstickBookOpen, qstickBookClose,
                                   (int)(sizeof(qstickBookOpen)/sizeof(double)),
                                   4, qstickBookExp, (int)NB_QSTICK_BOOK, 1 );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = qstick_check_vector( "tulip vector (untest.txt qstick 5)",
                                   qstickTulipOpen, qstickTulipClose,
                                   (int)(sizeof(qstickTulipOpen)/sizeof(double)),
                                   5, qstickTulipExp, (int)NB_QSTICK_TULIP, 0 );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/* Exact error term of ( a - b ), via Knuth's TwoSum on ( a, -b ). The result is
 * non-zero if and only if the subtraction rounded. Exact for any finite
 * operands -- no ordering precondition, unlike Fast2Sum.
 *
 * Load-bearing: the library builds with -ffp-contract=off. An FMA contraction
 * across these expressions would fold the error term away and make this report
 * zero unconditionally, i.e. silently vacuous. */
static double qstick_sub_error( double a, double b )
{
   double nb = -b;
   double s  = a  + nb;
   double ap = s  - nb;
   double bp = s  - ap;
   return (a - ap) + (nb - bp);
}

/* (3) PERIOD 1: the no-averaging case. Lookback drops to 0, the seeding while
 * loop is skipped entirely, and the output must be the raw body, bit-exact.
 *
 * Note what this corpus can and cannot exercise. By Sterbenz's lemma a - b is
 * EXACT whenever b/2 <= a <= 2b, and every OHLC bar satisfies that by a wide
 * margin, so close - open never rounds here -- measured over all 10000 bars of
 * ta_gDataOpen/Close, zero of them round. The guard below therefore asserts the
 * two properties this series really does carry: bodies that need the full
 * double (so the comparison is not merely re-checking short decimal literals),
 * and the Sterbenz-exactness itself, so that swapping in a corpus which
 * violates it is a loud failure rather than a silent change of meaning.
 *
 * The rounding case is not skipped -- it is covered by leg (6), which has to
 * build its own series to reach it. */
static ErrorNumber test_qstick_period_one( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i, nbBars, nbWideBody = 0, nbRounded = 0;
   static TA_Real out[OUT_CAP];

   nbBars = (int)history->nbBars;

   rc = TA_QSTICK( 0, nbBars - 1, history->open, history->close,
                   1, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 0 || nb != nbBars )
   {
      printf( "QSTICK period-1 Fail: rc=%d beg=%d nb=%d (expected 0/%d)\n",
              (int)rc, (int)beg, (int)nb, nbBars );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbBars; i++ )
   {
      double want = history->close[i] - history->open[i];
      if( memcmp( &out[i], &want, sizeof(double) ) != 0 )
      {
         printf( "QSTICK period-1 Fail at out[%d]: got %.17g expected %.17g\n",
                 i, out[i], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      /* Bodies that a float could not hold: the comparison above is then
       * testing the full double, not a short decimal literal. */
      if( want != 0.0 && (double)(float)want != want )
         nbWideBody++;
      /* Sterbenz: this must stay zero for any real OHLC series. */
      if( qstick_sub_error( history->close[i], history->open[i] ) != 0.0 )
         nbRounded++;
   }

   if( nbWideBody == 0 )
   {
      printf( "QSTICK period-1 Fail: corpus no longer has bodies needing more "
              "than float precision, the check above proves little\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( nbRounded != 0 )
   {
      printf( "QSTICK period-1 Fail: %d of %d bodies round, so this corpus is "
              "no longer Sterbenz-exact -- leg (6) owns the rounding case and "
              "this leg's premise needs revisiting\n", nbRounded, nbBars );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) FLAT INPUT: every bar closes where it opened. Issue #112 asks that a
 * successful call never emit NaN or Inf; here that is structural, since the
 * only division is by a positive integer parameter. The window sum is exactly
 * 0.0 and stays exactly 0.0 -- not merely small -- because each body is an
 * exact zero and the running total never accumulates anything else. */
static ErrorNumber test_qstick_flat( void )
{
#define QSTICK_FLAT_N 60
   static TA_Real open[QSTICK_FLAT_N], close[QSTICK_FLAT_N], out[OUT_CAP];
   TA_RetCode rc;
   TA_Integer beg, nb;
   int i;
   const int period = 10;

   for( i = 0; i < QSTICK_FLAT_N; i++ )
   {
      open[i]  = 100.0 + (double)i * 0.37;   /* prices move; bodies do not */
      close[i] = open[i];
   }

   rc = TA_QSTICK( 0, QSTICK_FLAT_N - 1, open, close, period, &beg, &nb, out );
   if( rc != TA_SUCCESS )
   {
      printf( "QSTICK flat Fail: retCode %d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( beg != period - 1 || nb != QSTICK_FLAT_N - period + 1 )
   {
      printf( "QSTICK flat Fail: got beg=%d nb=%d expected %d/%d\n",
              (int)beg, (int)nb, period - 1, QSTICK_FLAT_N - period + 1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* Bitwise against +0.0, not `!= 0.0`, which accepts -0.0 too. The sign is
    * reachable in principle -- x - x is +0.0 under round-to-nearest and a sum
    * of +0.0 terms stays +0.0, so -0.0 here would mean the accumulation or the
    * final divide had started producing one. That is exactly the class #147
    * records as invisible to a `== 0` comparison. */
   {
      const double posZero = 0.0;

      for( i = 0; i < nb; i++ )
      {
         if( memcmp( &out[i], &posZero, sizeof(double) ) != 0 )
         {
            printf( "QSTICK flat Fail at out[%d]: got %.17g (%s), expected "
                    "exactly +0.0\n", i, out[i],
                    out[i] == 0.0 ? "negative zero" : "non-zero" );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
#undef QSTICK_FLAT_N
}

/* (5) IN-PLACE ALIASING. The C API allows the output buffer to be one of the
 * inputs. QSTICK reads both trailing terms before it stores, so this holds for
 * either -- and the two are separate risks, because they are separate reads in
 * the same statement and a future rewrite could reorder one past the store.
 *
 * Run at period 1 as well: there the write index equals the trailing read index
 * on every iteration, which is the tightest the aliasing gets. */
static const int qstickInPlaceGrid[] = { 1, 10, 30 };
#define NB_QSTICK_INPLACE (sizeof(qstickInPlaceGrid)/sizeof(qstickInPlaceGrid[0]))

static ErrorNumber test_qstick_inplace( const TA_History *history )
{
   unsigned int g;
   int i, nbBars;
   TA_RetCode rc;
   TA_Integer begRef, nbRef, begAlias, nbAlias;
   static TA_Real outRef[OUT_CAP];
   static TA_Real work[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < NB_QSTICK_INPLACE; g++ )
   {
      int period = qstickInPlaceGrid[g];
      int which;

      rc = TA_QSTICK( 0, nbBars - 1, history->open, history->close,
                      period, &begRef, &nbRef, outRef );
      if( rc != TA_SUCCESS )
      {
         printf( "QSTICK in-place Fail [period %d]: reference retCode %d\n",
                 period, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( which = 0; which < 2; which++ )
      {
         const char *tag = which == 0 ? "outReal==inOpen" : "outReal==inClose";

         /* Copy the aliased input into the scratch buffer, then hand that same
          * buffer in as both that input and the output. */
         for( i = 0; i < nbBars; i++ )
            work[i] = which == 0 ? history->open[i] : history->close[i];

         if( which == 0 )
            rc = TA_QSTICK( 0, nbBars - 1, work, history->close,
                            period, &begAlias, &nbAlias, work );
         else
            rc = TA_QSTICK( 0, nbBars - 1, history->open, work,
                            period, &begAlias, &nbAlias, work );

         if( rc != TA_SUCCESS )
         {
            printf( "QSTICK in-place Fail [period %d, %s]: retCode %d\n",
                    period, tag, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( begAlias != begRef || nbAlias != nbRef )
         {
            printf( "QSTICK in-place Fail [period %d, %s]: range (%d,%d) vs (%d,%d)\n",
                    period, tag, (int)begAlias, (int)nbAlias,
                    (int)begRef, (int)nbRef );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbRef; i++ )
         {
            if( memcmp( &work[i], &outRef[i], sizeof(double) ) != 0 )
            {
               printf( "QSTICK in-place Fail [period %d, %s] at out[%d]: "
                       "got %.17g expected %.17g\n",
                       period, tag, i, work[i], outRef[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) A SERIES WHOSE BODIES ACTUALLY ROUND, PLUS EXACT ZEROS AND THE
 * CANCELLATION CASE.
 *
 * Leg (3) cannot reach the rounding case at all: Sterbenz's lemma makes
 * close - open exact whenever the two are within a factor of two, which every
 * real OHLC bar is, so not one of the 10000 corpus bars rounds. Three regimes
 * are built here instead, interleaved so that a single window can span more
 * than one:
 *
 *   A. ROUNDING   -- opens near 1e-9 against closes near 1e8. The exact
 *                    difference needs far more than 53 bits, so the subtraction
 *                    rounds and the body carries an error term.
 *   B. EXACT ZERO -- close == open, body +0.0. Enough consecutive bars that at
 *                    period 10 at least one whole window is nothing but zeros.
 *   C. STERBENZ   -- ordinary prices, body exact, matching leg (3)'s regime.
 *   D. CANCELLING -- equal and opposite bodies, so a window sums to exactly
 *                    zero out of non-zero terms. Distinct from B: there the
 *                    accumulator never leaves zero, here it returns to it.
 *
 * Every regime is counted and every count is asserted non-zero, so editing the
 * series cannot quietly empty one out. Which windows are all-zero is derived
 * from the data rather than hardcoded, so the +0.0 assertion follows an edit. */
static ErrorNumber test_qstick_rounding_corpus( void )
{
#define QSTICK_RC_N 64
   static TA_Real open[QSTICK_RC_N], close[QSTICK_RC_N];
   static TA_Real body[QSTICK_RC_N];
   static TA_Real out[OUT_CAP], outSma[OUT_CAP];
   static const int periods[] = { 1, 2, 3, 5, 10 };
   const double posZero = 0.0;
   TA_RetCode rc, rcS, rcM;
   TA_Integer beg, nb, begS, nbS, begM, nbM;
   int i, g;
   int nbRounded = 0, nbZero = 0, nbSterbenz = 0, nbZeroWindowResidue = 0;

   for( i = 0; i < QSTICK_RC_N; i++ )
   {
      if( i < 40 )                       /* A: rounding */
      {
         open[i]  = 1e-9 * (double)(i + 1);
         close[i] = 1e8 + (double)i * 0.5;
      }
      else if( i < 52 )                  /* B: exact zero bodies */
      {
         open[i]  = 100.0 + (double)i * 0.37;
         close[i] = open[i];
      }
      else if( i < 58 )                  /* C: Sterbenz-exact bodies */
      {
         open[i]  = 50.0 + (double)i * 0.125;
         close[i] = open[i] + 0.25;
      }
      else                               /* D: equal and opposite */
      {
         open[i]  = 200.0;
         close[i] = ( i & 1 ) ? 200.5 : 199.5;
      }
   }

   for( i = 0; i < QSTICK_RC_N; i++ )
   {
      body[i] = close[i] - open[i];
      if( qstick_sub_error( close[i], open[i] ) != 0.0 )
         nbRounded++;
      else if( body[i] == 0.0 )
         nbZero++;
      else
         nbSterbenz++;
   }

   if( nbRounded == 0 || nbZero == 0 || nbSterbenz == 0 )
   {
      printf( "QSTICK rounding-corpus Fail: regimes are %d rounding / %d zero / "
              "%d exact, each must be non-zero or the leg tests less than it "
              "claims\n", nbRounded, nbZero, nbSterbenz );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( g = 0; g < (int)( sizeof(periods)/sizeof(periods[0]) ); g++ )
   {
      int period = periods[g];

      rc = TA_QSTICK( 0, QSTICK_RC_N - 1, open, close, period, &beg, &nb, out );
      if( rc != TA_SUCCESS || beg != period - 1 ||
          nb != QSTICK_RC_N - period + 1 )
      {
         printf( "QSTICK rounding-corpus Fail [period %d]: rc=%d beg=%d nb=%d "
                 "(expected 0/%d/%d)\n", period, (int)rc, (int)beg, (int)nb,
                 period - 1, QSTICK_RC_N - period + 1 );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      /* Same differential as leg (1), on inputs that round. */
      rcS = TA_SUB( 0, QSTICK_RC_N - 1, close, open, &begS, &nbS, body );
      rcM = TA_SMA( 0, QSTICK_RC_N - 1, body, period, &begM, &nbM, outSma );
      if( rcS != TA_SUCCESS || rcM != TA_SUCCESS || begM != beg || nbM != nb )
      {
         printf( "QSTICK rounding-corpus Fail [period %d]: reference rc=%d/%d "
                 "range (%d,%d) vs (%d,%d)\n", period, (int)rcS, (int)rcM,
                 (int)begM, (int)nbM, (int)beg, (int)nb );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nb; i++ )
      {
         int j, allZero = 1;

         if( memcmp( &out[i], &outSma[i], sizeof(double) ) != 0 )
         {
            printf( "QSTICK rounding-corpus Fail [period %d] at out[%d]: "
                    "fused %.17g != compose %.17g (must be BIT-exact even when "
                    "the body rounds)\n", period, i, out[i], outSma[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( !( out[i] == out[i] ) ||
             out[i] > 1e300 || out[i] < -1e300 )
         {
            printf( "QSTICK rounding-corpus Fail [period %d] at out[%d]: "
                    "non-finite %.17g\n", period, i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         /* An all-zero window does NOT have to give zero here, and that is the
          * point of measuring it. After bodies near 1e8 the running total does
          * not return exactly to zero once the window fills with zeros: at
          * period 2, out[40] covers two zero-body bars and reads 4.47e-08, one
          * ulp-scale at 2e8. TA_SMA over the same body series gives the
          * identical bits, so this is the running-sum accumulator's residue,
          * shared by both, not a QSTICK defect -- the differential above is
          * what pins it. Leg (4) owns the exactly-+0.0 contract, where it does
          * hold because the accumulator never sees anything but zeros.
          *
          * What must never appear is a NEGATIVE zero: x - x is +0.0 under
          * round-to-nearest and a sum of +0.0 terms stays +0.0, so a -0.0 would
          * mean the accumulation or the divide started producing one. That is
          * the class #147 records as invisible to a `== 0` comparison. */
         for( j = i; j < i + period; j++ )
         {
            if( close[j] != open[j] ) { allZero = 0; break; }
         }
         if( allZero && out[i] != 0.0 )
            nbZeroWindowResidue++;
         if( out[i] == 0.0 &&
             memcmp( &out[i], &posZero, sizeof(double) ) != 0 )
         {
            printf( "QSTICK rounding-corpus Fail [period %d] at out[%d]: "
                    "negative zero in the output\n", period, i );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      /* Period 1 leaves the raw body, rounding included. */
      if( period == 1 )
      {
         for( i = 0; i < nb; i++ )
         {
            double want = close[i] - open[i];
            if( memcmp( &out[i], &want, sizeof(double) ) != 0 )
            {
               printf( "QSTICK rounding-corpus Fail [period 1] at out[%d]: "
                       "got %.17g expected %.17g\n", i, out[i], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   /* The residue case above is the one this series exists to reach; if an edit
    * ever stops producing it, the comment describes behaviour nothing tests. */
   if( nbZeroWindowResidue == 0 )
   {
      printf( "QSTICK rounding-corpus Fail: no all-zero window carries "
              "accumulator residue, so the documented running-sum behaviour is "
              "no longer exercised\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
#undef QSTICK_RC_N
}

/* ======================================================================
 * AO - Bill Williams' Awesome Oscillator (#227)
 *
 *    median_t = (high_t + low_t) / 2
 *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t     (5 / 34)
 *
 * Two legs, per this file's doctrine.
 *
 * (1) DIFFERENTIAL, bit-exact. The reference is TA_MEDPRICE, two TA_SMA calls
 *     and a TA_SUB - four shipped primitives, no new numerical logic. Both
 *     SMA calls take the SAME startIdx as the AO call (clamped to AO's own
 *     lookback), which is what makes the comparison bit-exact rather than
 *     merely close: TA_SMA seeds its running sum at startIdx-(period-1), so a
 *     reference that called it with startIdx 0 would accumulate the fast leg
 *     over a completely different history and disagree in the last ulp. That
 *     is measurable, not hypothetical - at 2/100 it disagrees on all 153
 *     values while the anchored form matches all 153.
 *
 * (2) EXTERNAL ORACLE, two independent arms. Tulip Indicators 0.9.2 (ti_ao,
 *     pinned SHA be18abb13e075ba866898dcc7cb52399603302a6) and
 *     pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1), both driven live
 *     over this exact 252-bar corpus through the private ../ta-lib-oracles
 *     servers. The two arms are NOT interchangeable: ti_ao hardcodes 5/34 and
 *     refuses anything else, so it can only corroborate the default pair;
 *     pandas honours both periods and is the only arm that reaches the
 *     parametric space. Neither is circular - Tulip is independent C, and
 *     pandas-ta's ao() calls its own sma() with talib defaulted off, so the
 *     arm is pandas' rolling mean and not TA-Lib validating TA-Lib. Agreement
 *     is ulp-scale but NOT bitwise, which is why the values below are the
 *     ORACLES' and are compared at tolerance; freezing our own output here
 *     would prove nothing about the formula.
 *
 * There is no "expected value" arm from a book: Williams publishes charts, not
 * tables, and Tulip's own tests/untest.txt carries an `ao` entry with an EMPTY
 * expected vector (its 15-bar smoke corpus is shorter than the 33-bar
 * lookback). Running the oracle is the only way to get numbers, and that is
 * what was done.
 * ====================================================================== */

/* Period pairs. Covers the default; the shortest legal pair; equal pairs at
 * both ends (where AO is identically zero and every trailing subtraction must
 * cancel to the bit); the off-default pairs the pandas arm also pins; both
 * orderings of a small AND a large inverted pair (no swap is performed - see
 * ao.c); adjacent periods, where the two windows differ by one bar; and a slow
 * period longer than the corpus so the "nothing to evaluate" path is walked.
 *
 * The spread of the pair is what this grid is really sweeping. The reference
 * anchors both TA_SMA calls at AO's own clamped startIdx, so each leg's running
 * sum has been through exactly the same number of add/subtract roll steps as
 * AO's - which is what makes the comparison bit-exact for EVERY pair rather
 * than for the lucky ones. Measured on this corpus, an unanchored reference
 * (TA_SMA called at startIdx 0) agrees for 5/34 and 3/10 and disagrees on all
 * 153 values at 2/100 and 100/2; 6/100 agrees where 5/100 does not. That
 * pattern is coincidence, not structure - no period's rolling sum is exact on
 * this series - which is exactly why the anchoring is not optional. */
static const struct { int fast; int slow; } aoGrid[] =
{
   {   5,  34 },
   {   2,   2 },
   {   2,   3 },
   {   3,  10 },
   {   2, 100 },
   { 100,   2 },
   {  10,   5 },
   {  34,   5 },
   {   5, 100 },
   {  99, 100 },
   { 100, 100 },
   {   2, 300 },
};
#define NB_AO_GRID (sizeof(aoGrid)/sizeof(aoGrid[0]))

/* startIdx values for the same differential. A non-zero startIdx moves both
 * trailing indices off the array head; at startIdx 0 the two seed windows and
 * the output window share an origin and an off-by-one between them is
 * invisible. 251 leaves exactly one output. */
static const int aoStartGrid[] = { 0, 1, 34, 40, 200, 251 };
#define NB_AO_START (sizeof(aoStartGrid)/sizeof(aoStartGrid[0]))

/* AO external-oracle golden values.
 *
 * Every number below was read off a live oracle run over
 * TA_SREF_high/low_daily_ref_0_PRIV, transported as hex-of-IEEE-bits (#115) so
 * the capture is lossless, and emitted mechanically - none was transcribed by
 * hand. Rows are spread across each output range and every one has |value| >= 2
 * so the relative term governs; the bar nearest a crossing is deliberately
 * excluded (bar 44, AO = -0.119, where the disagreement is pure cancellation).
 *
 * Measured full-corpus agreement between the shipped TA_AO and each arm:
 *
 *   tulip  5/34  : max abs 1.563e-13, max rel 6.699e-13, 8.884e-14 where |v|>=1
 *   pandas 5/34  : max abs 2.132e-13, max rel 6.925e-13, 1.244e-13 where |v|>=1
 *   pandas 3/10  : max abs 1.563e-13, max rel 5.168e-12, 1.312e-13 where |v|>=1
 *   pandas 2/100 : max abs 1.847e-13, max rel 2.324e-12, 9.127e-14 where |v|>=1
 *
 * The whole-corpus maxima land at crossings - tulip's 6.699e-13 is bar 167
 * where AO = -0.1697 - which is exactly the spread ta_test_priv.h cites when
 * explaining why a relative-only oracle check is wrong for an oscillator. */
typedef enum { AO_ORC_TULIP = 0, AO_ORC_PANDAS = 1 } AoOracle;
static const char * const aoOracleName[] = { "tulip 0.9.2", "pandas-ta-classic 0.6.52" };

static const struct { int fast; int slow; int beg; int nb; } aoOracleShape[] =
{
   {   5,  34,  33, 219 },
   {   3,  10,   9, 243 },
   {   2, 100,  99, 153 },
};
#define NB_AO_ORACLE_SHAPE (sizeof(aoOracleShape)/sizeof(aoOracleShape[0]))

static const struct { AoOracle src; int fast; int slow; int idx; double value; } aoOracle[] =
{
   { AO_ORC_TULIP,    5,  34,   0,  -3.3146323529411887 },   /* bar  33 */
   { AO_ORC_TULIP,    5,  34,  54,  12.41173529411762   },   /* bar  87 */
   { AO_ORC_TULIP,    5,  34, 110,  -2.107441176470573  },   /* bar 143 */
   { AO_ORC_TULIP,    5,  34, 164, -13.103117647058895  },   /* bar 197 */
   { AO_ORC_TULIP,    5,  34, 218,   2.5937941176469224 },   /* bar 251 */

   { AO_ORC_PANDAS,   5,  34,   0,  -3.3146323529411745 },   /* bar  33 */
   { AO_ORC_PANDAS,   5,  34,  54,  12.411735294117648  },   /* bar  87 */
   { AO_ORC_PANDAS,   5,  34, 110,  -2.107441176470587  },   /* bar 143 */
   { AO_ORC_PANDAS,   5,  34, 164, -13.103117647058795  },   /* bar 197 */
   { AO_ORC_PANDAS,   5,  34, 218,   2.5937941176470787 },   /* bar 251 */

   { AO_ORC_PANDAS,   3,  10,   3,   2.553750000000008  },   /* bar  12 */
   { AO_ORC_PANDAS,   3,  10,  62,  -2.2715833333333393 },   /* bar  71 */
   { AO_ORC_PANDAS,   3,  10, 121,   4.792833333333334  },   /* bar 130 */
   { AO_ORC_PANDAS,   3,  10, 181,  -2.791499999999999  },   /* bar 190 */
   { AO_ORC_PANDAS,   3,  10, 234,  -2.4710000000000036 },   /* bar 243 */

   { AO_ORC_PANDAS,   2, 100,   0,  19.006375000000006  },   /* bar  99 */
   { AO_ORC_PANDAS,   2, 100,  38,  21.3403             },   /* bar 137 */
   { AO_ORC_PANDAS,   2, 100,  76,  11.81337499999998   },   /* bar 175 */
   { AO_ORC_PANDAS,   2, 100, 114, -29.12644999999999   },   /* bar 213 */
   { AO_ORC_PANDAS,   2, 100, 152,  -3.8640999999999934 },   /* bar 251 */
};
#define NB_AO_ORACLE (sizeof(aoOracle)/sizeof(aoOracle[0]))

/* Relative band, from the measured agreement where |value| >= 1: the worst of
 * the four arms is 1.312e-13, and the worst PINNED row is 7.670e-14. */
#define AO_ORACLE_TOL 1e-12
/* Near-zero absolute floor. AO crosses zero by construction, so a relative-only
 * check is unbounded there. Sized ~10x the worst measured absolute agreement
 * (2.132e-13). It does not govern any row above - every pinned |value| >= 2, so
 * the relative term is at least 2e-12 - but it keeps a later addition near a
 * crossing from turning into a spurious failure. See checkOracleValue(). */
#define AO_ORACLE_ABS 2e-12

/* pandas-ta-classic's ao() SWAPS an inverted pair before computing, so asking
 * it for (10,5) returns its AO_5_10 column. TA-Lib does not swap: subtraction
 * is exactly antisymmetric in IEEE-754, so our AO(10,5) is the exact negation
 * of our AO(5,10) and therefore of pandas' answer, to the same ulp-scale
 * agreement as everywhere else. Pinning it is what stops the "no swap" ruling
 * from being quietly reverted into a swap - a swap would make these five rows
 * come back with the wrong sign, a 2x relative error that no tolerance hides.
 *
 * Captured live: pandas-ta-classic 0.6.52, column AO_5_10, outBegIdx 9, 243
 * values. Measured over the full corpus, max |ours + theirs| = 1.279e-13. */
static const struct { int idx; double value; } aoOracleSwapped[] =
{
   {  15, -2.4810000000000088 },   /* bar  24 */
   {  63, -2.007499999999993  },   /* bar  72 */
   { 121,  3.4735000000000014 },   /* bar 130 */
   { 181, -2.031499999999994  },   /* bar 190 */
   { 235, -2.140500000000003  },   /* bar 244 */
};
#define NB_AO_ORACLE_SWAPPED (sizeof(aoOracleSwapped)/sizeof(aoOracleSwapped[0]))

/* (1) DIFFERENTIAL: AO == SUB( SMA(MEDPRICE,fast), SMA(MEDPRICE,slow) ),
 * bit-for-bit, over the period grid crossed with the startIdx grid. */
static ErrorNumber test_ao_differential( const TA_History *history )
{
   unsigned int g, s;
   int i, nbBars, nbChecked;
   TA_RetCode rcAO, rcMed, rcFast, rcSlow, rcSub;
   TA_Integer begAO, nbAO, begMed, nbMed, begFast, nbFast, begSlow, nbSlow, begSub, nbSub;
   static TA_Real med[OUT_CAP];
   static TA_Real smaFast[OUT_CAP];
   static TA_Real smaSlow[OUT_CAP];
   static TA_Real outAO[OUT_CAP];
   static TA_Real outRef[OUT_CAP];

   nbBars = (int)history->nbBars;
   nbChecked = 0;

   /* The median series, from the shipped TA_MEDPRICE rather than an inline
    * loop: the reference must contain no new numerical logic of its own. */
   rcMed = TA_MEDPRICE( 0, nbBars - 1, history->high, history->low,
                        &begMed, &nbMed, med );
   if( rcMed != TA_SUCCESS || begMed != 0 || nbMed != nbBars )
   {
      printf( "AO differential Fail: TA_MEDPRICE rc=%d beg=%d nb=%d (expected 0/%d)\n",
              (int)rcMed, (int)begMed, (int)nbMed, nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( s = 0; s < NB_AO_START; s++ )
   {
      int startIdx = aoStartGrid[s];

      for( g = 0; g < NB_AO_GRID; g++ )
      {
         int fast = aoGrid[g].fast;
         int slow = aoGrid[g].slow;
         int lookback, refStart;

         rcAO = TA_AO( startIdx, nbBars - 1, history->high, history->low,
                       fast, slow, &begAO, &nbAO, outAO );
         if( rcAO != TA_SUCCESS )
         {
            printf( "AO differential Fail [start %d fast %d slow %d]: retCode %d\n",
                    startIdx, fast, slow, (int)rcAO );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         /* The lookback contract, checked against the closed form rather than
          * against the reference (which shares TA_AO_Lookback with it). */
         lookback = TA_AO_Lookback( fast, slow );
         if( lookback != (fast > slow ? fast : slow) - 1 )
         {
            printf( "AO differential Fail [fast %d slow %d]: lookback %d, expected %d\n",
                    fast, slow, lookback, (fast > slow ? fast : slow) - 1 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         refStart = startIdx < lookback ? lookback : startIdx;

         if( refStart > nbBars - 1 )
         {
            /* Not enough data for a single output. */
            if( nbAO != 0 )
            {
               printf( "AO differential Fail [start %d fast %d slow %d]: "
                       "expected no output, got nb=%d\n",
                       startIdx, fast, slow, (int)nbAO );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            continue;
         }

         if( begAO != refStart || nbAO != nbBars - refStart )
         {
            printf( "AO differential Fail [start %d fast %d slow %d]: "
                    "range (%d,%d), expected (%d,%d)\n",
                    startIdx, fast, slow, (int)begAO, (int)nbAO,
                    refStart, nbBars - refStart );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* The reference. Both SMA calls take AO's OWN clamped startIdx, which
          * is what anchors their running sums identically to AO's - see the
          * banner above. */
         rcFast = TA_SMA( refStart, nbBars - 1, med, fast, &begFast, &nbFast, smaFast );
         rcSlow = TA_SMA( refStart, nbBars - 1, med, slow, &begSlow, &nbSlow, smaSlow );
         if( rcFast != TA_SUCCESS || rcSlow != TA_SUCCESS )
         {
            printf( "AO differential Fail [start %d fast %d slow %d]: "
                    "reference TA_SMA rc %d / %d\n",
                    startIdx, fast, slow, (int)rcFast, (int)rcSlow );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( begFast != refStart || begSlow != refStart ||
             nbFast != nbAO || nbSlow != nbAO )
         {
            printf( "AO differential Fail [start %d fast %d slow %d]: "
                    "reference range fast(%d,%d) slow(%d,%d) vs AO(%d,%d)\n",
                    startIdx, fast, slow, (int)begFast, (int)nbFast,
                    (int)begSlow, (int)nbSlow, (int)begAO, (int)nbAO );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         rcSub = TA_SUB( 0, nbAO - 1, smaFast, smaSlow, &begSub, &nbSub, outRef );
         if( rcSub != TA_SUCCESS || nbSub != nbAO )
         {
            printf( "AO differential Fail [start %d fast %d slow %d]: "
                    "TA_SUB rc=%d nb=%d\n",
                    startIdx, fast, slow, (int)rcSub, (int)nbSub );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         for( i = 0; i < nbAO; i++ )
         {
            if( memcmp( &outAO[i], &outRef[i], sizeof(double) ) != 0 )
            {
               printf( "AO differential Fail [start %d fast %d slow %d] at out[%d]: "
                       "fused %.17g != compose %.17g (must be BIT-exact)\n",
                       startIdx, fast, slow, i, outAO[i], outRef[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         /* An equal pair must be identically zero, sign included: the two
          * running sums walk the same bars in the same order, so every
          * trailing subtraction has to cancel exactly. This is the one grid
          * row whose expected value is known without any reference at all. */
         if( fast == slow )
         {
            for( i = 0; i < nbAO; i++ )
            {
               if( outAO[i] != 0.0 )
               {
                  printf( "AO differential Fail [start %d fast=slow=%d] at out[%d]: "
                          "%.17g, expected exactly 0\n",
                          startIdx, fast, i, outAO[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }

         nbChecked++;

         /* Cross-language: AO must also be bit-identical on every language
          * server (no transcendental, so all four stay zero-tolerance). */
         if( server_verify_active() )
         {
            ErrorNumber e = server_verify( "AO", startIdx, nbBars - 1, history->nbBars,
                                           rcAO, begAO, nbAO,
                                           (const TA_Real*[]){ history->high, history->low, NULL },
                                           (double[]){ (double)fast, (double)slow }, 2,
                                           (const TA_Real*[]){ outAO, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   /* Non-vacuity: the grid must actually have compared something. A period
    * pair that silently stopped producing output would otherwise leave this
    * leg green on zero comparisons. */
   if( nbChecked < (int)(NB_AO_GRID * NB_AO_START) / 2 )
   {
      printf( "AO differential Fail: only %d of %d grid cells produced output\n",
              nbChecked, (int)(NB_AO_GRID * NB_AO_START) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (2) EXTERNAL-ORACLE: TA_AO against Tulip Indicators and pandas-ta-classic. */
static ErrorNumber test_ao_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int k, c;
   int nbCompared = 0;

   for( c = 0; c < NB_AO_ORACLE_SHAPE; c++ )
   {
      int fast = aoOracleShape[c].fast;
      int slow = aoOracleShape[c].slow;

      retCode = TA_AO( 0, (int)history->nbBars - 1, history->high, history->low,
                       fast, slow, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "AO oracle Fail [fast %d slow %d]: retCode = %d\n",
                 fast, slow, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != aoOracleShape[c].beg || nbElement != aoOracleShape[c].nb )
      {
         printf( "AO oracle Fail [fast %d slow %d]: shape got (%d,%d) expected (%d,%d)\n",
                 fast, slow, (int)begIdx, (int)nbElement,
                 aoOracleShape[c].beg, aoOracleShape[c].nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < NB_AO_ORACLE; k++ )
      {
         int idx;
         double want, got, err;
         const char *mode;

         if( aoOracle[k].fast != fast || aoOracle[k].slow != slow )
            continue;

         idx  = aoOracle[k].idx;
         want = aoOracle[k].value;
         got  = out[idx];

         if( !checkOracleValue( got, want, AO_ORACLE_TOL, AO_ORACLE_ABS, &err, &mode ) )
         {
            printf( "AO oracle Fail [%s fast %d slow %d] at out[%d]: "
                    "got %.17g expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                    aoOracleName[aoOracle[k].src], fast, slow, idx, got, want,
                    mode, err, AO_ORACLE_TOL, AO_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         nbCompared++;
      }
   }

   /* Every pinned row must have been reached: a period typo in either table
    * would otherwise silently drop its rows and leave the leg passing on
    * fewer. */
   if( nbCompared != (int)NB_AO_ORACLE )
   {
      printf( "AO oracle Fail: compared %d value(s) but the table carries %d\n",
              nbCompared, (int)NB_AO_ORACLE );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (3) INVERTED PAIR: TA_AO does NOT swap fast and slow. */
static ErrorNumber test_ao_inverted_pair( const TA_History *history )
{
   TA_RetCode rcInv, rcFwd;
   TA_Integer begInv, nbInv, begFwd, nbFwd;
   static TA_Real outInv[OUT_CAP];
   static TA_Real outFwd[OUT_CAP];
   int nbBars = (int)history->nbBars;
   unsigned int k;
   int i;

   rcInv = TA_AO( 0, nbBars - 1, history->high, history->low,
                  10, 5, &begInv, &nbInv, outInv );
   rcFwd = TA_AO( 0, nbBars - 1, history->high, history->low,
                  5, 10, &begFwd, &nbFwd, outFwd );
   if( rcInv != TA_SUCCESS || rcFwd != TA_SUCCESS )
   {
      printf( "AO inverted Fail: retCode %d / %d\n", (int)rcInv, (int)rcFwd );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begInv != begFwd || nbInv != nbFwd || begInv != 9 )
   {
      printf( "AO inverted Fail: ranges (%d,%d) and (%d,%d), expected both (9,%d)\n",
              (int)begInv, (int)nbInv, (int)begFwd, (int)nbFwd, nbBars - 9 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   /* Exact antisymmetry. IEEE-754 subtraction gives fl(a-b) == -fl(b-a) with
    * no rounding slack, so this is an equality, not a tolerance. It is also
    * the cheapest possible detector of a swap having been reintroduced: a
    * swapped implementation would make both calls return the same array. */
   for( i = 0; i < nbInv; i++ )
   {
      if( outInv[i] != -outFwd[i] )
      {
         printf( "AO inverted Fail at out[%d]: AO(10,5)=%.17g but -AO(5,10)=%.17g\n",
                 i, outInv[i], -outFwd[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* And the sign is pinned against an outside opinion, not just against
    * ourselves: pandas-ta-classic swaps, so its AO_5_10 answers our AO(10,5)
    * negated. Without this, "no swap" would be self-certified. */
   for( k = 0; k < NB_AO_ORACLE_SWAPPED; k++ )
   {
      int idx     = aoOracleSwapped[k].idx;
      double want = -aoOracleSwapped[k].value;
      double got  = outInv[idx];
      double err;
      const char *mode;

      if( !checkOracleValue( got, want, AO_ORACLE_TOL, AO_ORACLE_ABS, &err, &mode ) )
      {
         printf( "AO inverted oracle Fail at out[%d]: got %.17g expected %.17g "
                 "(%s=%.3e > rel %.3e / abs %.3e)\n",
                 idx, got, want, mode, err, AO_ORACLE_TOL, AO_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) IN-PLACE ALIASING: outReal written over inHigh, then over inLow.
 *
 * This needs its own leg rather than an argument. When startIdx is clamped to
 * the lookback, the longer window's trailing index equals outIdx exactly, so
 * the bar the loop is about to subtract is the slot it is about to write. The
 * implementation subtracts both trailing terms before storing; hoisting the
 * store above them would corrupt the running sum without changing any value on
 * a non-aliased call, so nothing else in the suite would notice. Swept over
 * startIdx because the risky slot moves with it. */
static ErrorNumber test_ao_inplace( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begRef, nbRef, begAlias, nbAlias;
   static TA_Real outRef[OUT_CAP];
   static TA_Real work[OUT_CAP];
   int nbBars = (int)history->nbBars;
   unsigned int g, s;
   int i, which;

   for( s = 0; s < NB_AO_START; s++ )
   {
      int startIdx = aoStartGrid[s];

      for( g = 0; g < NB_AO_GRID; g++ )
      {
         int fast = aoGrid[g].fast;
         int slow = aoGrid[g].slow;

         rc = TA_AO( startIdx, nbBars - 1, history->high, history->low,
                     fast, slow, &begRef, &nbRef, outRef );
         if( rc != TA_SUCCESS )
         {
            printf( "AO inplace Fail [start %d fast %d slow %d]: reference retCode %d\n",
                    startIdx, fast, slow, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( nbRef == 0 )
            continue;

         for( which = 0; which < 2; which++ )
         {
            const TA_Real *h = history->high;
            const TA_Real *l = history->low;
            const char *tag = which == 0 ? "outReal==inHigh" : "outReal==inLow";

            /* Copy the aliased input into the output buffer and point that
             * parameter at it, so out and that input are the same storage. */
            for( i = 0; i < nbBars; i++ )
               work[i] = which == 0 ? history->high[i] : history->low[i];
            if( which == 0 )
               h = work;
            else
               l = work;

            rc = TA_AO( startIdx, nbBars - 1, h, l, fast, slow,
                        &begAlias, &nbAlias, work );
            if( rc != TA_SUCCESS || begAlias != begRef || nbAlias != nbRef )
            {
               printf( "AO inplace Fail [%s start %d fast %d slow %d]: "
                       "rc=%d range (%d,%d) vs (%d,%d)\n",
                       tag, startIdx, fast, slow, (int)rc,
                       (int)begAlias, (int)nbAlias, (int)begRef, (int)nbRef );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }

            for( i = 0; i < nbRef; i++ )
            {
               if( memcmp( &work[i], &outRef[i], sizeof(double) ) != 0 )
               {
                  printf( "AO inplace Fail [%s start %d fast %d slow %d] at out[%d]: "
                          "aliased %.17g != separate %.17g\n",
                          tag, startIdx, fast, slow, i, work[i], outRef[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) PARAMETER FLOOR. AO's periods are declared [2, 100000], matching MACD's
 * fast/slow. Period 1 is therefore NOT an accepted value on either axis, even
 * though SMA(x,1) is well defined and pandas-ta-classic does accept it - so
 * the answer to "does AO(1,100) match the composite?" is that AO(1,100) is a
 * guarded reject, not a computed answer. Pinned here because the range is a
 * published API contract, and because it is the one part of the accepted-range
 * question the differential grid cannot express. */
static ErrorNumber test_ao_param_reject( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real out[OUT_CAP];
   static const struct { int fast; int slow; } badPair[] =
   {
      {   1, 100 },   /* fast below the floor            */
      { 100,   1 },   /* slow below the floor            */
      {   1,   1 },   /* both                            */
      {   0,  34 },
      {   5,   0 },
      {  -1,  34 },
      {   5,  -1 },
      { 100001, 34 }, /* above the ceiling               */
      {   5, 100001 },
   };
   unsigned int k;
   int nbBars = (int)history->nbBars;

   for( k = 0; k < sizeof(badPair)/sizeof(badPair[0]); k++ )
   {
      rc = TA_AO( 0, nbBars - 1, history->high, history->low,
                  badPair[k].fast, badPair[k].slow, &beg, &nb, out );
      if( rc != TA_BAD_PARAM )
      {
         printf( "AO param reject Fail [fast %d slow %d]: retCode %d, expected TA_BAD_PARAM(%d)\n",
                 badPair[k].fast, badPair[k].slow, (int)rc, (int)TA_BAD_PARAM );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   /* And the boundary itself is accepted, so the floor is a floor and not an
    * off-by-one that rejects the smallest legal pair. */
   rc = TA_AO( 0, nbBars - 1, history->high, history->low, 2, 2, &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 1 || nb != nbBars - 1 )
   {
      printf( "AO param reject Fail: the legal floor (2,2) was refused or mis-shaped "
              "(rc=%d beg=%d nb=%d)\n", (int)rc, (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   return TA_TEST_PASS;
}

/* (6) THE ANCHORING DISCIPLINE ITSELF, and the non-vacuity of the grid that
 * enforces it.
 *
 * Leg (1) is bit-exact only because both TA_SMA calls take AO's OWN clamped
 * startIdx. TA_SMA seeds its running sum at startIdx-(period-1) and then rolls
 * add-new / snapshot / subtract-trailing once per bar, so how many roll steps
 * a sum has been through at a given output bar depends on where the call
 * started. A reference calling TA_SMA at startIdx 0 gives the SHORTER leg
 * max(fast,slow)-min(fast,slow) extra roll steps, and each step rounds.
 *
 * The danger this leg exists for: an unanchored reference still passes at
 * plenty of period pairs. Measured on this corpus it is bit-exact at 5/34,
 * 3/10, 34/5 and 6/100, and wrong on ALL 153 values at 2/100, 100/2, 5/100 and
 * 99/100 - 6/100 agrees where 5/100 does not, one roll step apart. So the
 * agreement is arithmetic coincidence, not structure (no period's rolling sum
 * is exact on this series), and a differential written the natural unanchored
 * way and tested only at the default pair would be GREEN while proving nothing.
 *
 * So this leg asserts two things leg (1) cannot:
 *   (a) the anchored reference is bit-exact -- the contract; and
 *   (b) at least one pair in aoGrid actually SEPARATES anchored from
 *       unanchored. Without (b), leg (1) could be silently reduced to lucky
 *       pairs and no gate in the tree would notice.
 */
static ErrorNumber test_ao_reference_anchoring( const TA_History *history )
{
   unsigned int g;
   int i, nbBars, nbSeparating = 0, nbPairs = 0;
   TA_RetCode rc;
   TA_Integer begAO, nbAO, begM, nbM, begF, nbF, begS, nbS;
   static TA_Real med[OUT_CAP];
   static TA_Real smaF[OUT_CAP];
   static TA_Real smaS[OUT_CAP];
   static TA_Real outAO[OUT_CAP];

   nbBars = (int)history->nbBars;

   rc = TA_MEDPRICE( 0, nbBars - 1, history->high, history->low, &begM, &nbM, med );
   if( rc != TA_SUCCESS || begM != 0 || nbM != nbBars )
   {
      printf( "AO anchoring Fail: TA_MEDPRICE rc=%d beg=%d nb=%d\n",
              (int)rc, (int)begM, (int)nbM );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( g = 0; g < NB_AO_GRID; g++ )
   {
      int fast = aoGrid[g].fast;
      int slow = aoGrid[g].slow;
      int anchoredBad = 0, unanchoredBad = 0;

      rc = TA_AO( 0, nbBars - 1, history->high, history->low,
                  fast, slow, &begAO, &nbAO, outAO );
      if( rc != TA_SUCCESS )
      {
         printf( "AO anchoring Fail [fast %d slow %d]: retCode %d\n",
                 fast, slow, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( nbAO == 0 )
         continue;               /* lookback exceeds the corpus; nothing to compare */
      nbPairs++;

      /* (a) ANCHORED: both legs at AO's clamped startIdx. Must be bit-exact. */
      if( TA_SMA( begAO, nbBars - 1, med, fast, &begF, &nbF, smaF ) != TA_SUCCESS ||
          TA_SMA( begAO, nbBars - 1, med, slow, &begS, &nbS, smaS ) != TA_SUCCESS ||
          begF != begAO || begS != begAO || nbF != nbAO || nbS != nbAO )
      {
         printf( "AO anchoring Fail [fast %d slow %d]: anchored reference shape\n",
                 fast, slow );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbAO; i++ )
      {
         double want = smaF[i] - smaS[i];
         if( memcmp( &outAO[i], &want, sizeof(double) ) != 0 )
            anchoredBad++;
      }
      if( anchoredBad != 0 )
      {
         printf( "AO anchoring Fail [fast %d slow %d]: the ANCHORED reference "
                 "differs on %d of %d value(s) - leg (1)'s premise is broken\n",
                 fast, slow, anchoredBad, (int)nbAO );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* (b) UNANCHORED: both legs from startIdx 0, aligned by absolute bar.
       * Not a contract - just counted, to prove the grid can tell them apart. */
      if( TA_SMA( 0, nbBars - 1, med, fast, &begF, &nbF, smaF ) != TA_SUCCESS ||
          TA_SMA( 0, nbBars - 1, med, slow, &begS, &nbS, smaS ) != TA_SUCCESS )
      {
         printf( "AO anchoring Fail [fast %d slow %d]: unanchored reference rc\n",
                 fast, slow );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( i = 0; i < nbAO; i++ )
      {
         int bar = begAO + i;
         double want = smaF[bar - begF] - smaS[bar - begS];
         if( memcmp( &outAO[i], &want, sizeof(double) ) != 0 )
            unanchoredBad++;
      }
      if( unanchoredBad != 0 )
         nbSeparating++;
   }

   if( nbPairs < 2 )
   {
      printf( "AO anchoring Fail: only %d grid pair(s) produced output\n", nbPairs );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* THE NON-VACUITY ASSERTION. If this fires, aoGrid has been reduced to pairs
    * where anchored and unanchored happen to agree, and leg (1) is no longer
    * evidence that the reference is anchored correctly. Restore a wide-spread
    * pair (2/100, 100/2, 5/100 and 99/100 all separate on this corpus). */
   if( nbSeparating == 0 )
   {
      printf( "AO anchoring Fail: no pair in aoGrid separates the anchored "
              "reference from the unanchored one (%d pair(s) compared), so the "
              "differential leg would pass with a mis-anchored reference\n",
              nbPairs );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* ======================================================================
 * AC - Bill Williams' Accelerator/Decelerator Oscillator (#228)
 *
 *    median_t = (high_t + low_t) / 2
 *    AO_t     = SMA(median, fast)_t - SMA(median, slow)_t     (5 / 34)
 *    AC_t     = AO_t - SMA(AO, signal)_t                      (5)
 *
 * Two legs, per this file's doctrine.
 *
 * (1) DIFFERENTIAL, bit-exact. The reference is TA_AO, TA_SMA and TA_SUB -
 *     three shipped primitives, no new numerical logic. TA_AO is called at
 *     the bar AC's own oscillator series starts on, which is AC's clamped
 *     startIdx less the signal window; TA_SMA then runs over that oscillator
 *     array from its head. Both anchorings are load-bearing, for the reason
 *     spelled out on leg (6): every running sum in the fused loop has been
 *     through exactly the same number of add/subtract roll steps as the
 *     reference's, and a reference anchored anywhere else disagrees in the
 *     last ulp. Measured on this corpus, an unanchored reference disagrees on
 *     35 of the 84 (grid x startIdx) cells.
 *
 * (2) EXTERNAL ORACLE, one arm. trading-signals 8.3.0 (npm, MIT), class
 *     `AC` in momentum/AC/AC.js, driven live over this exact 252-bar corpus
 *     and captured as hex-of-IEEE-bits so nothing is lost in transport. It
 *     parameterises all three windows exactly as this does, and is NOT
 *     circular: its moving average re-sums the stored window on every bar
 *     (util/math/getAverage.js: `values.reduce((sum, x) => sum + x, 0) /
 *     values.length`) where TA-Lib rolls a running total, so 202 of the 215
 *     values at the default triple differ in the last bits. Agreement is
 *     ulp-scale but NOT bitwise, which is why the values below are the
 *     ORACLE'S and are compared at tolerance; freezing our own output here
 *     would prove nothing about the formula.
 *
 * There is no second arm, and this is the one function in this file where
 * that is a fact about the world rather than a choice: neither of the two
 * oracle servers implements AC. Tulip Indicators' indicator directory has
 * ao.c and no ac.c; pandas-ta-classic 0.6.52 has momentum/ao.py and no
 * ac.py anywhere in its package tree. Both were checked, not assumed.
 *
 * The trading-signals capture was itself checked before being trusted, since
 * a silently-wrong oracle is worse than none: every one of its 1024 captured
 * values was compared against the same formula evaluated in EXACT rational
 * arithmetic (Python Fraction over the corpus doubles, no rounding anywhere).
 * Worst deviation 1.3e-13 absolute over the five parameter triples - i.e. it
 * is a correctly-rounded evaluation of this formula, not merely a plausible
 * one.
 * ====================================================================== */

/* Parameter triples. Covers the default; the legal floor on every axis;
 * equal price windows at both ends (where AO is identically zero and so is
 * AC); the off-default triples the oracle also pins; both orderings of an
 * inverted price pair (no swap is performed - see ac.c and ao.c); the
 * shortest legal signal window, where AC degenerates to half the bar-to-bar
 * change in AO; a signal window longer than either price window; and a
 * signal period longer than the corpus so the "nothing to evaluate" path is
 * walked. */
static const struct { int fast; int slow; int sig; } acGrid[] =
{
   {   5,  34,   5 },
   {   2,   2,   2 },
   {   2,   3,   3 },
   {   3,  10,   4 },
   {   2, 100,   7 },
   { 100,   2,   5 },
   {  10,   5,   5 },
   {   5, 100,   2 },
   {  99, 100,  30 },
   { 100, 100,   2 },
   {  10,  20,  30 },
   {   5,  34, 300 },
};
#define NB_AC_GRID (sizeof(acGrid)/sizeof(acGrid[0]))

/* startIdx values for the same differential. Unlike AO's, this grid MUST
 * carry values above the lookback: at startIdx 0 the reference's TA_AO call
 * clamps to exactly the same bar an unanchored one would, so no cell there
 * can tell a correctly anchored reference from a careless one. See leg (6). */
static const int acStartGrid[] = { 0, 1, 40, 60, 120, 200, 251 };
#define NB_AC_START (sizeof(acStartGrid)/sizeof(acStartGrid[0]))

/* AC external-oracle golden values.
 *
 * Every number below was read off a live trading-signals 8.3.0 run over
 * TA_SREF_high/low_daily_ref_0_PRIV (node 22.21.1), transported as
 * hex-of-IEEE-bits (#115) so the capture is lossless, and emitted
 * mechanically - none was transcribed by hand. Five rows per triple, one per
 * fifth of the output range, each the largest |value| in its fifth so the
 * relative term governs rather than the near-zero floor.
 *
 * Measured full-corpus agreement between the shipped TA_AC and the arm:
 *
 *   5/34/5   : max abs 5.951e-14, max rel 2.521e-12, 4.820e-14 where |v|>=1
 *   3/10/4   : max abs 5.684e-14, max rel 1.814e-12, 3.087e-14 where |v|>=1
 *   2/100/7  : max abs 5.151e-14, max rel 5.288e-12, 3.289e-14 where |v|>=1
 *   5/34/2   : max abs 3.553e-14, max rel 2.422e-12, 2.735e-14 where |v|>=1
 *   10/20/30 : max abs 1.315e-13, max rel 3.641e-12, 9.381e-14 where |v|>=1
 *
 * The whole-corpus relative maxima all land at zero crossings, which AC has
 * by construction - hence the mixed predicate below rather than a bare
 * relative one. */
static const struct { int fast; int slow; int sig; int beg; int nb; } acOracleShape[] =
{
   {   5,  34,   5,  37, 215 },
   {   3,  10,   4,  12, 240 },
   {   2, 100,   7, 105, 147 },
   {   5,  34,   2,  34, 218 },
   {  10,  20,  30,  48, 204 },
};
#define NB_AC_ORACLE_SHAPE (sizeof(acOracleShape)/sizeof(acOracleShape[0]))

static const struct { int fast; int slow; int sig; int idx; double value; } acOracle[] =
{
   {   5,  34,  5,  42,   7.0632470588235465 },   /* bar  79 */
   {   5,  34,  5,  43,   4.987732352941177  },   /* bar  80 */
   {   5,  34,  5, 103,  -5.1706235294117819 },   /* bar 140 */
   {   5,  34,  5, 160,  -3.3305117647058857 },   /* bar 197 */
   {   5,  34,  5, 189,   5.2308470588235396 },   /* bar 226 */

   {   3,  10,  4,  17,   2.6822499999999998 },   /* bar  29 */
   {   3,  10,  4,  65,   7.3000416666666652 },   /* bar  77 */
   {   3,  10,  4, 126,  -3.672791666666658  },   /* bar 138 */
   {   3,  10,  4, 189,   2.8039583333333233 },   /* bar 201 */
   {   3,  10,  4, 227,  -5.0104166666666643 },   /* bar 239 */

   {   2, 100,  7,  12,   4.2657071428571491 },   /* bar 117 */
   {   2, 100,  7,  34,  -8.1432607142857236 },   /* bar 139 */
   {   2, 100,  7,  74,  -5.0883428571428428 },   /* bar 179 */
   {   2, 100,  7,  98, -11.304242857142874  },   /* bar 203 */
   {   2, 100,  7, 120,   9.6306071428571371 },   /* bar 225 */

   {   5,  34,  2,  42,   1.1246544117647019 },   /* bar  76 */
   {   5,  34,  2,  44,   2.0532720588235378 },   /* bar  78 */
   {   5,  34,  2, 105,  -1.3852647058823493 },   /* bar 139 */
   {   5,  34,  2, 162,  -1.0833235294117713 },   /* bar 196 */
   {   5,  34,  2, 191,   1.5842352941176472 },   /* bar 225 */

   {  10,  20, 30,  36,   6.3676749999999869 },   /* bar  84 */
   {  10,  20, 30,  58,  -5.0599208333333152 },   /* bar 106 */
   {  10,  20, 30,  98,  -7.3506833333333326 },   /* bar 146 */
   {  10,  20, 30, 142,  -5.3971000000000338 },   /* bar 190 */
   {  10,  20, 30, 184,   7.7485083333333389 },   /* bar 232 */
};
#define NB_AC_ORACLE (sizeof(acOracle)/sizeof(acOracle[0]))

/* Relative band, from the measured agreement where |value| >= 1: the worst of
 * the five triples is 9.381e-14, and the worst PINNED row is 1.264e-14. */
#define AC_ORACLE_TOL 1e-12
/* Near-zero absolute floor. AC crosses zero by construction, so a
 * relative-only check is unbounded there. Sized ~15x the worst measured
 * absolute agreement (1.315e-13). Every pinned |value| >= 1, so the relative
 * term governs every row above; this keeps a later addition near a crossing
 * from turning into a spurious failure. See checkOracleValue(). */
#define AC_ORACLE_ABS 2e-12

/* Build the bit-exact reference for one AC call: TA_AO over the oscillator's
 * own range, TA_SMA over that array, then TA_SUB of the two aligned halves.
 *
 * aoStartIdx selects the anchoring, which is the whole point of leg (6): pass
 * acBegIdx-(sig-1) for the contract, anything else to see it break.
 *
 * Returns 0 on success, or a non-zero ErrorNumber. On success *nbRef holds
 * the element count, which the caller must find equal to AC's own. */
static ErrorNumber ac_build_reference( const TA_History *history,
                                       int fast, int slow, int sig,
                                       int aoStartIdx, int endIdx,
                                       double *aoBuf, double *smaBuf,
                                       double *outRef, int *nbRef )
{
   TA_RetCode rc;
   TA_Integer begAO, nbAO, begSma, nbSma, begSub, nbSub;

   rc = TA_AO( aoStartIdx, endIdx, history->high, history->low,
               fast, slow, &begAO, &nbAO, aoBuf );
   if( rc != TA_SUCCESS || nbAO < sig )
   {
      printf( "AC reference Fail [fast %d slow %d sig %d]: TA_AO rc=%d nb=%d\n",
              fast, slow, sig, (int)rc, (int)nbAO );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   rc = TA_SMA( 0, nbAO - 1, aoBuf, sig, &begSma, &nbSma, smaBuf );
   if( rc != TA_SUCCESS || begSma != sig - 1 )
   {
      printf( "AC reference Fail [fast %d slow %d sig %d]: TA_SMA rc=%d beg=%d\n",
              fast, slow, sig, (int)rc, (int)begSma );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   rc = TA_SUB( 0, nbSma - 1, &aoBuf[sig-1], smaBuf, &begSub, &nbSub, outRef );
   if( rc != TA_SUCCESS || nbSub != nbSma )
   {
      printf( "AC reference Fail [fast %d slow %d sig %d]: TA_SUB rc=%d nb=%d\n",
              fast, slow, sig, (int)rc, (int)nbSub );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   *nbRef = (int)nbSub;
   return TA_TEST_PASS;
}

/* (1) DIFFERENTIAL: AC == SUB( AO, SMA(AO,signal) ), bit-for-bit, over the
 * parameter grid crossed with the startIdx grid. */
static ErrorNumber test_ac_differential( const TA_History *history )
{
   unsigned int g, s;
   int i, nbBars, nbChecked = 0;
   ErrorNumber e;
   TA_RetCode rcAC;
   TA_Integer begAC, nbAC;
   static TA_Real aoBuf[OUT_CAP];
   static TA_Real smaBuf[OUT_CAP];
   static TA_Real outAC[OUT_CAP];
   static TA_Real outRef[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( s = 0; s < NB_AC_START; s++ )
   {
      int startIdx = acStartGrid[s];

      for( g = 0; g < NB_AC_GRID; g++ )
      {
         int fast = acGrid[g].fast;
         int slow = acGrid[g].slow;
         int sig  = acGrid[g].sig;
         int lookback, refStart, nbRef;

         rcAC = TA_AC( startIdx, nbBars - 1, history->high, history->low,
                       fast, slow, sig, &begAC, &nbAC, outAC );
         if( rcAC != TA_SUCCESS )
         {
            printf( "AC differential Fail [start %d fast %d slow %d sig %d]: retCode %d\n",
                    startIdx, fast, slow, sig, (int)rcAC );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }

         /* The lookback contract, checked against the closed form rather than
          * against the reference (which shares TA_AC_Lookback with it). */
         lookback = TA_AC_Lookback( fast, slow, sig );
         if( lookback != (fast > slow ? fast : slow) - 1 + sig - 1 )
         {
            printf( "AC differential Fail [fast %d slow %d sig %d]: lookback %d, expected %d\n",
                    fast, slow, sig, lookback,
                    (fast > slow ? fast : slow) - 1 + sig - 1 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         refStart = startIdx < lookback ? lookback : startIdx;

         if( refStart > nbBars - 1 )
         {
            /* Not enough data for a single output. */
            if( nbAC != 0 )
            {
               printf( "AC differential Fail [start %d fast %d slow %d sig %d]: "
                       "expected no output, got nb=%d\n",
                       startIdx, fast, slow, sig, (int)nbAC );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            continue;
         }

         if( begAC != refStart || nbAC != nbBars - refStart )
         {
            printf( "AC differential Fail [start %d fast %d slow %d sig %d]: "
                    "range (%d,%d), expected (%d,%d)\n",
                    startIdx, fast, slow, sig, (int)begAC, (int)nbAC,
                    refStart, nbBars - refStart );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         /* The reference, anchored on AC's OWN clamped startIdx - see the
          * banner above and leg (6). */
         e = ac_build_reference( history, fast, slow, sig,
                                 refStart - (sig-1), nbBars - 1,
                                 aoBuf, smaBuf, outRef, &nbRef );
         if( e != TA_TEST_PASS )
            return e;
         if( nbRef != nbAC )
         {
            printf( "AC differential Fail [start %d fast %d slow %d sig %d]: "
                    "reference nb %d vs AC nb %d\n",
                    startIdx, fast, slow, sig, nbRef, (int)nbAC );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbAC; i++ )
         {
            if( memcmp( &outAC[i], &outRef[i], sizeof(double) ) != 0 )
            {
               printf( "AC differential Fail [start %d fast %d slow %d sig %d] at out[%d]: "
                       "fused %.17g != compose %.17g (must be BIT-exact)\n",
                       startIdx, fast, slow, sig, i, outAC[i], outRef[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }

         /* Equal price windows make AO identically zero, so the signal
          * average of it is zero and so is AC - sign included. Every
          * subtraction in all three running sums has to cancel exactly. This
          * is the one grid row whose expected value is known without any
          * reference at all. */
         if( fast == slow )
         {
            for( i = 0; i < nbAC; i++ )
            {
               if( outAC[i] != 0.0 )
               {
                  printf( "AC differential Fail [start %d fast=slow=%d sig %d] at out[%d]: "
                          "%.17g, expected exactly 0\n",
                          startIdx, fast, sig, i, outAC[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }

         nbChecked++;

         /* Cross-language: AC must also be bit-identical on every language
          * server (no transcendental, so all four stay zero-tolerance). */
         if( server_verify_active() )
         {
            e = server_verify( "AC", startIdx, nbBars - 1, history->nbBars,
                               rcAC, begAC, nbAC,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               (double[]){ (double)fast, (double)slow, (double)sig }, 3,
                               (const TA_Real*[]){ outAC, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
         }
      }
   }

   /* Non-vacuity: the grid must actually have compared something. A triple
    * that silently stopped producing output would otherwise leave this leg
    * green on zero comparisons. */
   if( nbChecked < (int)(NB_AC_GRID * NB_AC_START) / 2 )
   {
      printf( "AC differential Fail: only %d of %d grid cells produced output\n",
              nbChecked, (int)(NB_AC_GRID * NB_AC_START) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (2) EXTERNAL-ORACLE: TA_AC against trading-signals 8.3.0. */
static ErrorNumber test_ac_oracle( const TA_History *history )
{
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   static TA_Real out[OUT_CAP];
   unsigned int k, c;
   int nbCompared = 0;

   for( c = 0; c < NB_AC_ORACLE_SHAPE; c++ )
   {
      int fast = acOracleShape[c].fast;
      int slow = acOracleShape[c].slow;
      int sig  = acOracleShape[c].sig;

      retCode = TA_AC( 0, (int)history->nbBars - 1, history->high, history->low,
                       fast, slow, sig, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "AC oracle Fail [fast %d slow %d sig %d]: retCode = %d\n",
                 fast, slow, sig, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( begIdx != acOracleShape[c].beg || nbElement != acOracleShape[c].nb )
      {
         printf( "AC oracle Fail [fast %d slow %d sig %d]: shape got (%d,%d) expected (%d,%d)\n",
                 fast, slow, sig, (int)begIdx, (int)nbElement,
                 acOracleShape[c].beg, acOracleShape[c].nb );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }

      for( k = 0; k < NB_AC_ORACLE; k++ )
      {
         int idx;
         double want, got, err;
         const char *mode;

         if( acOracle[k].fast != fast || acOracle[k].slow != slow ||
             acOracle[k].sig != sig )
            continue;

         idx  = acOracle[k].idx;
         want = acOracle[k].value;
         got  = out[idx];

         if( !checkOracleValue( got, want, AC_ORACLE_TOL, AC_ORACLE_ABS, &err, &mode ) )
         {
            printf( "AC oracle Fail [trading-signals 8.3.0 fast %d slow %d sig %d] at out[%d]: "
                    "got %.17g expected %.17g (%s=%.3e > rel %.3e / abs %.3e)\n",
                    fast, slow, sig, idx, got, want,
                    mode, err, AC_ORACLE_TOL, AC_ORACLE_ABS );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         nbCompared++;
      }
   }

   /* Every pinned row must have been reached: a parameter typo in either
    * table would otherwise silently drop its rows and leave the leg passing
    * on fewer. */
   if( nbCompared != (int)NB_AC_ORACLE )
   {
      printf( "AC oracle Fail: compared %d value(s) but the table carries %d\n",
              nbCompared, (int)NB_AC_ORACLE );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (3) INVERTED PAIR: TA_AC does NOT swap fast and slow, inheriting AO's
 * ruling.
 *
 * Negating a series negates every partial sum, every average and every
 * difference exactly in IEEE-754 - no rounding slack anywhere - so AC(10,5,s)
 * must be the exact negation of AC(5,10,s), not merely close. That makes this
 * the cheapest possible detector of a swap having been introduced: a swapped
 * implementation returns the SAME array for both calls.
 *
 * The sign itself is pinned against an outside opinion one level down, by
 * test_ao_inverted_pair against pandas-ta-classic; AC is a strictly
 * increasing function of AO, so it cannot silently disagree about which way
 * up the oscillator is while AO agrees. */
static ErrorNumber test_ac_inverted_pair( const TA_History *history )
{
   TA_RetCode rcInv, rcFwd;
   TA_Integer begInv, nbInv, begFwd, nbFwd;
   static TA_Real outInv[OUT_CAP];
   static TA_Real outFwd[OUT_CAP];
   int nbBars = (int)history->nbBars;
   int i, nbNonZero = 0;

   rcInv = TA_AC( 0, nbBars - 1, history->high, history->low,
                  10, 5, 5, &begInv, &nbInv, outInv );
   rcFwd = TA_AC( 0, nbBars - 1, history->high, history->low,
                  5, 10, 5, &begFwd, &nbFwd, outFwd );
   if( rcInv != TA_SUCCESS || rcFwd != TA_SUCCESS )
   {
      printf( "AC inverted Fail: retCode %d / %d\n", (int)rcInv, (int)rcFwd );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begInv != begFwd || nbInv != nbFwd || begInv != 13 )
   {
      printf( "AC inverted Fail: ranges (%d,%d) and (%d,%d), expected both (13,%d)\n",
              (int)begInv, (int)nbInv, (int)begFwd, (int)nbFwd, nbBars - 13 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbInv; i++ )
   {
      if( outInv[i] != -outFwd[i] )
      {
         printf( "AC inverted Fail at out[%d]: AC(10,5,5)=%.17g but -AC(5,10,5)=%.17g\n",
                 i, outInv[i], -outFwd[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( outInv[i] != 0.0 )
         nbNonZero++;
   }

   /* An all-zero pair would satisfy the equality above while proving nothing
    * about a swap, since 0 == -0 numerically. */
   if( nbNonZero < nbInv / 2 )
   {
      printf( "AC inverted Fail: only %d of %d values are non-zero, so the "
              "antisymmetry check is vacuous\n", nbNonZero, (int)nbInv );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (4) IN-PLACE ALIASING: outReal written over inHigh, then over inLow.
 *
 * Weaker than AO's leg of the same name, and the difference is worth writing
 * down rather than copying the stronger claim across. For AO, a clamped
 * startIdx puts the longer window's trailing index exactly on outIdx, so
 * hoisting the store above the trailing reads is a live defect that only an
 * aliased call can see. AC has slack: the signal window leaves both trailing
 * indices at least optInSignalPeriod-1 bars ahead of outIdx, and the declared
 * floor of that period is 2, so the collision is unreachable. Measured, not
 * assumed - the same hoist applied to ac.c passes every leg here and the
 * generic #130 gate too.
 *
 * So this leg is not sabotage-backed the way AO's is; it asserts the
 * published calling convention holds, and stands as the regression guard if
 * that slack is ever removed (admitting a signal period of 1 would remove
 * it). Swept over startIdx because the trailing indices move with it. */
static ErrorNumber test_ac_inplace( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begRef, nbRef, begAlias, nbAlias;
   static TA_Real outRef[OUT_CAP];
   static TA_Real work[OUT_CAP];
   int nbBars = (int)history->nbBars;
   unsigned int g, s;
   int i, which;

   for( s = 0; s < NB_AC_START; s++ )
   {
      int startIdx = acStartGrid[s];

      for( g = 0; g < NB_AC_GRID; g++ )
      {
         int fast = acGrid[g].fast;
         int slow = acGrid[g].slow;
         int sig  = acGrid[g].sig;

         rc = TA_AC( startIdx, nbBars - 1, history->high, history->low,
                     fast, slow, sig, &begRef, &nbRef, outRef );
         if( rc != TA_SUCCESS )
         {
            printf( "AC inplace Fail [start %d fast %d slow %d sig %d]: reference retCode %d\n",
                    startIdx, fast, slow, sig, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( nbRef == 0 )
            continue;

         for( which = 0; which < 2; which++ )
         {
            const TA_Real *h = history->high;
            const TA_Real *l = history->low;
            const char *tag = which == 0 ? "outReal==inHigh" : "outReal==inLow";

            /* Copy the aliased input into the output buffer and point that
             * parameter at it, so out and that input are the same storage. */
            for( i = 0; i < nbBars; i++ )
               work[i] = which == 0 ? history->high[i] : history->low[i];
            if( which == 0 )
               h = work;
            else
               l = work;

            rc = TA_AC( startIdx, nbBars - 1, h, l, fast, slow, sig,
                        &begAlias, &nbAlias, work );
            if( rc != TA_SUCCESS || begAlias != begRef || nbAlias != nbRef )
            {
               printf( "AC inplace Fail [%s start %d fast %d slow %d sig %d]: "
                       "rc=%d range (%d,%d) vs (%d,%d)\n",
                       tag, startIdx, fast, slow, sig, (int)rc,
                       (int)begAlias, (int)nbAlias, (int)begRef, (int)nbRef );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }

            for( i = 0; i < nbRef; i++ )
            {
               if( memcmp( &work[i], &outRef[i], sizeof(double) ) != 0 )
               {
                  printf( "AC inplace Fail [%s start %d fast %d slow %d sig %d] at out[%d]: "
                          "aliased %.17g != separate %.17g\n",
                          tag, startIdx, fast, slow, sig, i, work[i], outRef[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) PARAMETER FLOOR. All three of AC's periods are declared [2, 100000],
 * matching AO's pair and MACD's fast/slow. Period 1 is therefore NOT an
 * accepted value on any axis, even though SMA(x,1) is well defined - so the
 * answer to "does AC(1,100,5) match the composite?" is that AC(1,100,5) is a
 * guarded reject, not a computed answer. Pinned here because the range is a
 * published API contract, and because it is the one part of the
 * accepted-range question the differential grid cannot express. */
static ErrorNumber test_ac_param_reject( const TA_History *history )
{
   TA_RetCode rc;
   TA_Integer beg, nb;
   static TA_Real out[OUT_CAP];
   static const struct { int fast; int slow; int sig; } badTriple[] =
   {
      {   1, 100,   5 },   /* fast below the floor   */
      { 100,   1,   5 },   /* slow below the floor   */
      {   5,  34,   1 },   /* signal below the floor */
      {   1,   1,   1 },   /* all three              */
      {   0,  34,   5 },
      {   5,   0,   5 },
      {   5,  34,   0 },
      {  -1,  34,   5 },
      {   5,  -1,   5 },
      {   5,  34,  -1 },
      { 100001, 34,  5 },  /* above the ceiling      */
      {   5, 100001, 5 },
      {   5,  34, 100001 },
   };
   unsigned int k;
   int nbBars = (int)history->nbBars;

   for( k = 0; k < sizeof(badTriple)/sizeof(badTriple[0]); k++ )
   {
      rc = TA_AC( 0, nbBars - 1, history->high, history->low,
                  badTriple[k].fast, badTriple[k].slow, badTriple[k].sig,
                  &beg, &nb, out );
      if( rc != TA_BAD_PARAM )
      {
         printf( "AC param reject Fail [fast %d slow %d sig %d]: retCode %d, "
                 "expected TA_BAD_PARAM(%d)\n",
                 badTriple[k].fast, badTriple[k].slow, badTriple[k].sig,
                 (int)rc, (int)TA_BAD_PARAM );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   /* And the boundary itself is accepted, so the floor is a floor and not an
    * off-by-one that rejects the smallest legal triple. */
   rc = TA_AC( 0, nbBars - 1, history->high, history->low, 2, 2, 2,
               &beg, &nb, out );
   if( rc != TA_SUCCESS || beg != 2 || nb != nbBars - 2 )
   {
      printf( "AC param reject Fail: the legal floor (2,2,2) was refused or mis-shaped "
              "(rc=%d beg=%d nb=%d)\n", (int)rc, (int)beg, (int)nb );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   return TA_TEST_PASS;
}

/* (6) THE ANCHORING DISCIPLINE ITSELF, and the non-vacuity of the grid that
 * enforces it.
 *
 * Leg (1) is bit-exact only because the reference's TA_AO call starts at AC's
 * own clamped startIdx less the signal window. TA_SMA - which TA_AO is two of,
 * and which the reference then runs a third time over the oscillator array -
 * seeds its running sum at startIdx-(period-1) and rolls add-new / snapshot /
 * subtract-trailing once per bar, so how many roll steps a sum has been
 * through at a given output bar depends on where the call started, and each
 * step rounds.
 *
 * The danger this leg exists for is sharper here than it was for AO. At
 * startIdx 0 the reference's TA_AO clamps to exactly the bar an unanchored
 * TA_AO(0, ...) clamps to, so the two references are the SAME CALL and agree
 * on every value, for every triple, by construction. A differential written
 * the natural unanchored way and tested only from bar 0 would therefore be
 * GREEN over the entire parameter grid while proving nothing at all - which
 * is why acStartGrid carries values above the lookback.
 *
 * So this leg asserts two things leg (1) cannot:
 *   (a) the anchored reference is bit-exact -- the contract; and
 *   (b) at least one cell in acGrid x acStartGrid actually SEPARATES anchored
 *       from unanchored. Without (b), leg (1) could be silently reduced to
 *       cells where the two agree and no gate in the tree would notice.
 */
static ErrorNumber test_ac_reference_anchoring( const TA_History *history )
{
   unsigned int g, s;
   int i, nbBars, nbCells = 0, nbSeparating = 0;
   TA_RetCode rc;
   TA_Integer begAC, nbAC, begAO, nbAO, begSma, nbSma;
   ErrorNumber e;
   static TA_Real aoBuf[OUT_CAP];
   static TA_Real smaBuf[OUT_CAP];
   static TA_Real outAC[OUT_CAP];
   static TA_Real outRef[OUT_CAP];

   nbBars = (int)history->nbBars;

   for( s = 0; s < NB_AC_START; s++ )
   {
      for( g = 0; g < NB_AC_GRID; g++ )
      {
         int fast = acGrid[g].fast;
         int slow = acGrid[g].slow;
         int sig  = acGrid[g].sig;
         int startIdx = acStartGrid[s];
         int nbRef, anchoredBad = 0, unanchoredBad = 0;

         rc = TA_AC( startIdx, nbBars - 1, history->high, history->low,
                     fast, slow, sig, &begAC, &nbAC, outAC );
         if( rc != TA_SUCCESS )
         {
            printf( "AC anchoring Fail [start %d fast %d slow %d sig %d]: retCode %d\n",
                    startIdx, fast, slow, sig, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( nbAC == 0 )
            continue;            /* lookback exceeds the corpus; nothing to compare */
         nbCells++;

         /* (a) ANCHORED. Must be bit-exact. */
         e = ac_build_reference( history, fast, slow, sig,
                                 begAC - (sig-1), nbBars - 1,
                                 aoBuf, smaBuf, outRef, &nbRef );
         if( e != TA_TEST_PASS )
            return e;
         if( nbRef != nbAC )
         {
            printf( "AC anchoring Fail [start %d fast %d slow %d sig %d]: "
                    "anchored reference nb %d vs AC nb %d\n",
                    startIdx, fast, slow, sig, nbRef, (int)nbAC );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < nbAC; i++ )
         {
            if( memcmp( &outAC[i], &outRef[i], sizeof(double) ) != 0 )
               anchoredBad++;
         }
         if( anchoredBad != 0 )
         {
            printf( "AC anchoring Fail [start %d fast %d slow %d sig %d]: the ANCHORED "
                    "reference differs on %d of %d value(s) - leg (1)'s premise is broken\n",
                    startIdx, fast, slow, sig, anchoredBad, (int)nbAC );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         /* (b) UNANCHORED: the whole chain from bar 0, aligned by absolute
          * bar. Not a contract - just counted, to prove the grid can tell the
          * two apart. */
         if( TA_AO( 0, nbBars - 1, history->high, history->low,
                    fast, slow, &begAO, &nbAO, aoBuf ) != TA_SUCCESS ||
             TA_SMA( 0, nbAO - 1, aoBuf, sig, &begSma, &nbSma, smaBuf ) != TA_SUCCESS )
         {
            printf( "AC anchoring Fail [start %d fast %d slow %d sig %d]: unanchored "
                    "reference rc\n", startIdx, fast, slow, sig );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         for( i = 0; i < nbAC; i++ )
         {
            int jAO = (begAC + i) - begAO;      /* index into the AO array  */
            int jSma = jAO - begSma;            /* index into the SMA array */
            double want;
            if( jSma < 0 || jAO >= nbAO || jSma >= nbSma )
               continue;
            want = aoBuf[jAO] - smaBuf[jSma];
            if( memcmp( &outAC[i], &want, sizeof(double) ) != 0 )
               unanchoredBad++;
         }
         if( unanchoredBad != 0 )
            nbSeparating++;
      }
   }

   if( nbCells < (int)(NB_AC_GRID * NB_AC_START) / 2 )
   {
      printf( "AC anchoring Fail: only %d of %d cells produced output\n",
              nbCells, (int)(NB_AC_GRID * NB_AC_START) );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* THE NON-VACUITY ASSERTION. If this fires, acStartGrid has been reduced to
    * values at or below the lookback (where the two references are the same
    * call) or acGrid to triples where they happen to agree, and leg (1) is no
    * longer evidence that the reference is anchored correctly. Restore a
    * startIdx well above the lookback - 60, 120 and 200 all separate on this
    * corpus, 35 cells in total. */
   if( nbSeparating == 0 )
   {
      printf( "AC anchoring Fail: no cell separates the anchored reference from "
              "the unanchored one (%d cell(s) compared), so the differential leg "
              "would pass with a mis-anchored reference\n", nbCells );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}
