/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 */

/* Description:
 *
 *   Regression test category for COMPOSITE functions: indicators defined as an
 *   exact arithmetic composition of functions TA-Lib already ships. Each such
 *   function is verified two independent ways:
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

/**** Local functions declarations. ****/
static ErrorNumber test_pvo_differential( const TA_History *history );
static ErrorNumber test_pvo_oracle( const TA_History *history );
static ErrorNumber test_pvo_default_is_ema( const TA_History *history );

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
      double ad   = fabs( got - want );
      double rel  = ad / fabs( want );   /* every golden |value| >> 1, no zero guard needed */

      if( rel > PVO_ORACLE_TOL )
      {
         printf( "PVO oracle Fail at out[%d]: got %.17g expected %.17g (rel=%.3e > %.3e)\n",
                 idx, got, want, rel, PVO_ORACLE_TOL );
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
