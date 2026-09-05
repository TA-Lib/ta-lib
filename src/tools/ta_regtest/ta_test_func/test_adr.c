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
 *  090426 MF,CC  First version (issue #367).
 */

/* Description:
 *
 *   Test TA_ADR (Average Day Range) = SMA( high - low, n ).
 *
 *   Legs:
 *     1. DIFFERENTIAL against TA_SUB followed by TA_SMA, BIT-EXACT. The fused
 *        body is sma.c with inReal[x] replaced by (inHigh[x] - inLow[x]), so
 *        the reference performs the identical operations in the identical
 *        order and anything but memcmp equality is a defect. TA_SMA is fed the
 *        SAME startIdx as TA_ADR, so the lookback clamp is compared too and
 *        both running sums have taken the same number of roll steps.
 *        Near-tautological on its own -- both sides assume ADR == SMA(H-L) --
 *        which is what leg 2 exists to falsify.
 *     2. EXTERNAL ORACLE, kand 0.2.2 (Rust), two arms. Leg 1 cannot tell a
 *        wrong formula from a right one; these can. Arm A replays kand's own
 *        committed unit-test vector, arm B its output over the SREF corpus at
 *        seven periods. Both are cross-checked on every language server.
 *     3. EDGES the generic gates do not reach: period 1 (lookback 0, seeding
 *        loop skipped, output must be the raw range bit-exactly), exactly one
 *        output, fewer bars than the lookback, an all-flat corpus that must
 *        give exactly +0.0, and high below low, which is not an error.
 *     4. IN-PLACE ALIASING on both components, bitwise.
 *     5. The startIdx/endIdx range sweep, EPSILON class -- a finite window
 *        carried in a running accumulator, so a different anchor may move the
 *        last ulp but nothing more.
 *
 *   Not covered here, deliberately: --fuzz-064 has nothing frozen to compare
 *   against for a post-cutover function, and --codegen value-compares nothing
 *   for the same reason. --xlang-hash and the server_verify calls below are the
 *   cross-language value gate.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define ADR_CAP 300   /* > MAX_NB_TEST_ELEMENT and > nbBars */

/* Oracle band, measured. Arm A agrees with TA_ADR BIT-FOR-BIT on all 98 values;
 * arm B's worst relative gap is 5.8e-16 over all 1577 values of the seven-period
 * sweep (144 of them differ in the last ulp). Three decimal orders of headroom,
 * and it is headroom for one structural difference only: kand rolls the window
 * as (sum + new) - old where TA-Lib captures the sum before subtracting the
 * trailing term, which rounds the same window differently. It is nowhere near
 * enough to hide a formula error -- SMA(high)-SMA(low), the competing published
 * spelling, and a window off by one bar both miss by whole percent. The
 * absolute floor never governs these arms, whose smallest value is 1.33; it is
 * here so that adding a golden from a near-flat series later cannot silently
 * turn the check into a spurious failure. */
#define ADR_ORACLE_REL 1e-12
#define ADR_ORACLE_ABS 1e-14

/* Periods spanning every branch that exists: 1 (lookback 0, the seeding loop is
 * skipped entirely), 2 (the shortest window that seeds), the shipped default,
 * and 100 -- far past what the corpus warms up with. */
static const int adrGrid[] = { 1, 2, 3, 5, 14, 20, 50, 100 };
#define NB_ADR_GRID ((int)(sizeof(adrGrid)/sizeof(adrGrid[0])))

/* A non-zero startIdx moves the trailing index off the array head, which is
 * where an off-by-one in the seeding loop hides: at startIdx 0 the seed window
 * and the output window begin at the same place and a swap of the two is
 * invisible. */
static const int adrStartGrid[] = { 0, 1, 40, 200 };
#define NB_ADR_START ((int)(sizeof(adrStartGrid)/sizeof(adrStartGrid[0])))

/* ==========================================================================
 * ORACLE ARM A -- kand's own committed unit-test vector.
 *
 * Input series and expected output both from kand 0.2.2 (crates.io,
 * `kand::ohlcv::adr`, Apache-2.0 OR MIT), EXECUTED on 2026-09-04 with rustc
 * 1.97.0 over the 100 high/low bars its own `adr.rs` unit test ships. The run
 * reproduced all 98 of that test's committed expected doubles BIT-FOR-BIT
 * (0 mismatches), so these values are simultaneously kand's published table
 * and this machine's execution of it.
 *
 * kand is an independent Rust implementation that reaches the same series by a
 * different route: it materialises the range vector and hands it to its own
 * SMA. Nothing here shares code, an author or a language with TA-Lib.
 *
 * kand pads its first period-1 outputs with NaN and returns InsufficientData
 * where TA-Lib returns TA_SUCCESS with outNBElement 0; only the values are
 * transcribed, and leg 3 owns those two edges on TA-Lib's own terms.
 * ========================================================================== */
static const double adrKandHigh[100] =
{
   35266.0, 35247.5, 35235.7, 35190.8, 35182.0, 35258.0,
   35262.9, 35281.5, 35256.0, 35210.0, 35185.4, 35230.0,
   35241.0, 35218.1, 35212.6, 35128.9, 35047.7, 35019.5,
   35078.8, 35085.0, 35034.1, 34984.4, 35010.8, 35047.1,
   35091.4, 35150.4, 35123.9, 35110.0, 35092.1, 35179.2,
   35244.9, 35150.2, 35136.0, 35133.6, 35188.0, 35215.3,
   35221.9, 35219.2, 35234.0, 35216.7, 35197.9, 35178.4,
   35183.4, 35129.7, 35149.1, 35129.3, 35125.5, 35114.5,
   35120.1, 35129.4, 35105.4, 35054.1, 35034.6, 35032.9,
   35070.8, 35086.0, 35086.9, 35048.9, 34988.6, 35004.3,
   34985.0, 35004.2, 35010.0, 35041.8, 35024.7, 34982.0,
   35018.0, 34978.2, 34959.5, 34965.0, 34985.3, 35002.4,
   35018.0, 34989.0, 34943.0, 34900.0, 34932.1, 34930.0,
   34920.3, 34929.9, 34940.0, 35019.7, 35009.1, 34980.2,
   34977.3, 34976.1, 34969.4, 35000.0, 35010.0, 35015.9,
   35062.9, 35084.8, 35085.1, 35077.9, 35118.0, 35104.0,
   35086.2, 35041.7, 35009.2, 34994.2
};

static const double adrKandLow[100] =
{
   35216.1, 35206.5, 35180.0, 35130.7, 35153.6, 35174.7,
   35202.6, 35203.5, 35175.0, 35166.0, 35170.9, 35154.1,
   35186.0, 35143.9, 35080.1, 35021.1, 34950.1, 34966.0,
   35012.3, 35022.2, 34931.6, 34911.0, 34952.5, 34977.9,
   35039.0, 35073.0, 35055.0, 35084.0, 35060.0, 35073.1,
   35090.0, 35072.0, 35078.0, 35088.0, 35124.8, 35169.4,
   35138.0, 35141.0, 35182.0, 35151.1, 35158.4, 35140.0,
   35087.0, 35085.8, 35114.7, 35086.0, 35090.6, 35074.1,
   35078.4, 35100.0, 35030.2, 34986.3, 34988.1, 34973.1,
   35012.3, 35048.3, 35038.9, 34937.3, 34937.0, 34958.7,
   34925.0, 34910.0, 34981.6, 34980.2, 34982.0, 34940.9,
   34970.0, 34924.7, 34922.1, 34914.0, 34955.8, 34975.0,
   34975.0, 34926.0, 34865.1, 34821.0, 34830.4, 34883.5,
   34888.5, 34904.6, 34880.6, 34934.0, 34978.5, 34965.9,
   34936.4, 34942.5, 34945.0, 34969.3, 34983.8, 35003.9,
   35001.1, 35032.1, 35027.3, 35062.3, 35067.8, 35070.7,
   35030.2, 34981.0, 34970.5, 34974.5
};

static const double adrKandExp[98] =
{
   48.866666666666184, 52.26666666666764, 48.06666666666812, 57.26666666667006,
   57.33333333333576, 73.8666666666686, 73.10000000000097, 67.66666666666667,
   46.5, 44.80000000000049, 48.46666666666715, 68.36666666666618,
   87.23333333333237, 104.83333333333333, 112.63333333333382, 86.30000000000048,
   72.53333333333285, 60.9333333333343, 77.26666666666763, 79.56666666666813,
   78.06666666666813, 66.96666666666715, 59.96666666666715, 66.33333333333333,
   66.23333333333478, 57.4333333333343, 42.333333333333336, 54.73333333333236,
   97.69999999999952, 113.0666666666657, 97.03333333333285, 60.599999999998545,
   55.599999999998545, 51.5666666666657, 64.33333333333333, 69.33333333333333,
   71.36666666666618, 65.26666666666522, 52.366666666666184, 47.833333333333336,
   58.10000000000097, 59.5666666666657, 58.23333333333236, 40.53333333333285,
   37.53333333333527, 39.53333333333527, 39.0, 37.166666666666664,
   48.76666666666764, 57.46666666666715, 63.166666666666664, 58.03333333333285,
   54.9333333333343, 52.0, 48.0666666666657, 65.76666666666522,
   70.39999999999903, 69.60000000000097, 52.400000000001455, 66.60000000000097,
   60.866666666666184, 61.400000000001455, 44.23333333333479, 48.46666666666715,
   43.93333333333188, 47.53333333333285, 46.30000000000049, 47.30000000000049,
   39.30000000000049, 35.96666666666715, 33.30000000000049, 44.46666666666715,
   61.30000000000049, 73.30000000000048, 86.19999999999952, 75.73333333333237,
   60.0, 34.53333333333527, 38.83333333333576, 56.80000000000049,
   58.5666666666657, 43.533333333330425, 28.599999999998545, 29.599999999998545,
   32.96666666666715, 29.566666666665697, 27.099999999998545, 22.966666666664725,
   33.333333333333336, 42.166666666669094, 57.4333333333343, 42.03333333333285,
   41.19999999999709, 33.03333333333285, 46.5, 50.0,
   51.79999999999806, 39.69999999999709
};

#define NB_ADR_KAND_BARS   ((int)(sizeof(adrKandHigh)/sizeof(double)))
#define NB_ADR_KAND_EXP    ((int)(sizeof(adrKandExp)/sizeof(double)))
#define ADR_KAND_PERIOD    3

/* ==========================================================================
 * ORACLE ARM B -- kand over the shipped 252-bar reference corpus.
 *
 * Same binary, same session: kand 0.2.2 driven over TA_SREF_high_daily_ref_0_PRIV
 * and TA_SREF_low_daily_ref_0_PRIV at each period below, printed at shortest
 * round-trip precision (every literal was verified to round-trip to the exact
 * bits kand produced).
 *
 * Arm A pins one period on one series; this arm is what keeps the window
 * alignment and the lookback honest across the whole parameter grid. Each
 * period contributes its first two output bars (where a seeding off-by-one
 * shows), its extreme-valued bars (where a relative tolerance is least
 * forgiving), and its last.
 *
 * `bar` is the ABSOLUTE bar index; the output index is bar - begIdx.
 * ========================================================================== */
typedef struct { int period; int bar; double want; } AdrGolden;

static const AdrGolden adrSrefOracle[] =
{
   {   2,   1,   3.0174999999999983 },
   {   2,   2,   2.8299999999999983 },
   {   2,  50,   1.3275000000000006 },
   {   2,  63,   2.4849999999999994 },
   {   2, 126,   2.9099999999999966 },
   {   2, 189,    4.405000000000001 },
   {   2, 236,    7.685000000000002 },
   {   2, 251,   2.4099999999999966 },
   {   3,   2,    2.719999999999999 },
   {   3,   3,   2.7833333333333314 },
   {   3,  51,   1.6866666666666674 },
   {   3,  64,   3.0849999999999986 },
   {   3, 127,    2.896666666666666 },
   {   3, 189,   3.8133333333333326 },
   {   3, 238,    6.813333333333333 },
   {   3, 251,   2.1899999999999977 },
   {   5,   4,   2.8069999999999995 },
   {   5,   5,   2.5509999999999993 },
   {   5,  66,   2.8319999999999994 },
   {   5, 128,   3.0740000000000007 },
   {   5, 190,   3.6879999999999997 },
   {   5, 238,                5.938 },
   {   5, 250,   1.9019999999999981 },
   {   5, 251,   2.0279999999999974 },
   {  14,  13,   2.9442857142857144 },
   {  14,  14,    3.091785714285714 },
   {  14,  51,   2.1942857142857144 },
   {  14,  72,   2.6853571428571428 },
   {  14, 132,    3.299285714285714 },
   {  14, 191,    4.313571428571428 },
   {  14, 192,    4.059285714285713 },
   {  14, 251,   2.7699999999999982 },
   {  20,  19,   2.9844999999999997 },
   {  20,  20,              3.00175 },
   {  20,  61,   2.2392499999999997 },
   {  20,  77,               3.0295 },
   {  20, 135,   3.3724999999999996 },
   {  20, 193,    3.978999999999999 },
   {  20, 196,    4.282499999999999 },
   {  20, 251,    3.560499999999999 },
   {  50,  49,   2.8051999999999997 },
   {  50,  50,               2.7752 },
   {  50,  68,               2.5521 },
   {  50,  99,   3.0928999999999998 },
   {  50, 150,   3.5143999999999993 },
   {  50, 196,   3.7674000000000003 },
   {  50, 201,   3.7246000000000006 },
   {  50, 251,    3.186199999999999 },
   { 100,  99,   2.9490499999999997 },
   { 100, 100,              2.96655 },
   { 100, 137,   3.1290999999999993 },
   { 100, 175,   3.4645500000000005 },
   { 100, 202,    3.642700000000001 },
   { 100, 213,    3.521500000000001 },
   { 100, 251,    3.455400000000001 },
};
#define NB_ADR_SREF ((int)(sizeof(adrSrefOracle)/sizeof(AdrGolden)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_adrDiffCmp;
static int g_adrKandCmp;
static int g_adrSrefCmp;
static int g_adrEdgeCmp;
static int g_adrAliasCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_adr_differential( const TA_History *history );
static ErrorNumber test_adr_kand_vector( void );
static ErrorNumber test_adr_sref_oracle( const TA_History *history );
static ErrorNumber test_adr_edges( const TA_History *history );
static ErrorNumber test_adr_inplace( const TA_History *history );
static ErrorNumber test_adr_range( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_adr( TA_History *history )
{
   ErrorNumber retValue;

   /* ADR has no unstable period; a leftover global setting must not reach it,
    * and the range sweep below asserts the same thing from the other side. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_adrDiffCmp = g_adrKandCmp = g_adrSrefCmp = 0;
   g_adrEdgeCmp = g_adrAliasCmp = 0;

   retValue = test_adr_differential( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_adr_kand_vector();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_adr_sref_oracle( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_adr_edges( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_adr_inplace( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_adr_range( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every leg
    * above is deterministic. */
   if( history->nbBars == 252
       && ( g_adrDiffCmp != 5701 || g_adrKandCmp != NB_ADR_KAND_EXP
            || g_adrSrefCmp != NB_ADR_SREF || g_adrEdgeCmp != 2095
            || g_adrAliasCmp != 3658 ) )
   {
      printf( "ADR Fail: coverage counters (diff %d, kand %d, sref %d, edges %d, "
              "alias %d) are not what this file was written with "
              "(5701, %d, %d, 2095, 3658)\n",
              g_adrDiffCmp, g_adrKandCmp, g_adrSrefCmp, g_adrEdgeCmp,
              g_adrAliasCmp, NB_ADR_KAND_EXP, NB_ADR_SREF );
      return TA_ADR_VACUOUS;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (1) DIFFERENTIAL: ADR == SMA( SUB(high, low) ), bit-for-bit.
 *
 * TA_SUB(inReal0, inReal1) computes inReal0 - inReal1, so the arguments are
 * (high, low) in that order -- reversing them yields the negated series and
 * still "compares", against a reference that is no longer ADR.
 */
static ErrorNumber test_adr_differential( const TA_History *history )
{
   int g, s, i, nbBars, nbCells = 0;
   TA_RetCode rcA, rcS, rcM;
   TA_Integer begA, nbA, begS, nbS, begM, nbM;
   static TA_Real range[ADR_CAP];
   static TA_Real outAdr[ADR_CAP];
   static TA_Real outSma[ADR_CAP];

   nbBars = (int)history->nbBars;

   /* The range series, from the shipped TA_SUB rather than an inline loop: the
    * reference must contain no new numerical logic of its own. */
   rcS = TA_SUB( 0, nbBars - 1, history->high, history->low, &begS, &nbS, range );
   if( rcS != TA_SUCCESS || begS != 0 || nbS != nbBars )
   {
      printf( "ADR differential Fail: TA_SUB rc=%d beg=%d nb=%d (expected 0/%d)\n",
              (int)rcS, (int)begS, (int)nbS, nbBars );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( s = 0; s < NB_ADR_START; s++ )
   {
      int startIdx = adrStartGrid[s];

      for( g = 0; g < NB_ADR_GRID; g++ )
      {
         int period = adrGrid[g];

         rcA = TA_ADR( startIdx, nbBars - 1, history->high, history->low,
                       period, &begA, &nbA, outAdr );

         /* The reference, fed the SAME startIdx so the lookback clamp is
          * compared too, not just the values. ADR's lookback IS SMA's, so both
          * anchors reach the same window by the same add/subtract order. */
         rcM = TA_SMA( startIdx, nbBars - 1, range, period, &begM, &nbM, outSma );

         if( rcA != rcM )
         {
            printf( "ADR differential Fail [start %d period %d]: retCode ADR=%d SMA=%d\n",
                    startIdx, period, (int)rcA, (int)rcM );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         if( rcA != TA_SUCCESS )
            continue;

         if( begA != begM || nbA != nbM )
         {
            printf( "ADR differential Fail [start %d period %d]: range ADR(%d,%d) SMA(%d,%d)\n",
                    startIdx, period, (int)begA, (int)nbA, (int)begM, (int)nbM );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( nbA > 0 )
            nbCells++;

         for( i = 0; i < nbA; i++ )
         {
            g_adrDiffCmp++;
            if( memcmp( &outAdr[i], &outSma[i], sizeof(double) ) != 0 )
            {
               printf( "ADR differential Fail [start %d period %d] at out[%d]: "
                       "fused %.17g != compose %.17g (must be BIT-exact)\n",
                       startIdx, period, i, outAdr[i], outSma[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   if( nbCells != NB_ADR_GRID * NB_ADR_START )
   {
      printf( "ADR differential Fail: only %d of %d grid cells produced output\n",
              nbCells, NB_ADR_GRID * NB_ADR_START );
      return TA_ADR_VACUOUS;
   }

   return TA_TEST_PASS;
}

/* (2a) EXTERNAL ORACLE: kand's own 100-bar vector at period 3. */
static ErrorNumber test_adr_kand_vector( void )
{
   static TA_Real out[ADR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i;

   retCode = TA_ADR( 0, NB_ADR_KAND_BARS - 1, adrKandHigh, adrKandLow,
                     ADR_KAND_PERIOD, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != ADR_KAND_PERIOD - 1
       || nbElement != NB_ADR_KAND_EXP )
   {
      printf( "ADR kand-vector Fail: rc=%d (%d,%d) expected (%d,%d)\n",
              (int)retCode, (int)begIdx, (int)nbElement,
              ADR_KAND_PERIOD - 1, NB_ADR_KAND_EXP );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   if( server_verify_active() )
   {
      double optIn[1];
      ErrorNumber e;
      int cmpBefore = server_verify_comparisons();

      optIn[0] = (double)ADR_KAND_PERIOD;
      e = server_verify( "ADR", 0, NB_ADR_KAND_BARS - 1, NB_ADR_KAND_BARS,
                         retCode, begIdx, nbElement,
                         (const TA_Real*[]){ adrKandHigh, adrKandLow, NULL },
                         optIn, 1,
                         (const TA_Real*[]){ out, NULL }, NULL );
      if( e != TA_TEST_PASS )
         return e;
      /* "No failure reported" and "nothing was compared" are the same
       * observation without this. */
      if( server_verify_comparisons() == cmpBefore )
      {
         printf( "ADR kand-vector: compared no server despite live pipes\n" );
         return TA_ADR_VACUOUS;
      }
   }

   for( i = 0; i < nbElement; i++ )
   {
      double err;
      const char *mode;

      g_adrKandCmp++;
      if( !checkOracleValue( out[i], adrKandExp[i],
                             ADR_ORACLE_REL, ADR_ORACLE_ABS, &err, &mode ) )
      {
         printf( "ADR kand-vector Fail at out[%d]: got %.17g expected %.17g "
                 "(%s err %.3g, tol rel %g abs %g)\n",
                 i, out[i], adrKandExp[i], mode, err,
                 ADR_ORACLE_REL, ADR_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (2b) EXTERNAL ORACLE: kand over the shipped reference corpus, seven periods. */
static ErrorNumber test_adr_sref_oracle( const TA_History *history )
{
   static TA_Real out[ADR_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int nbBars = (int)history->nbBars;
   int k, lastPeriod = -1;

   if( nbBars != 252 )
   {
      printf( "ADR sref-oracle skip: goldens were captured on the 252-bar "
              "corpus, got %d\n", nbBars );
      return TA_TEST_PASS;
   }

   begIdx = 0; nbElement = 0;

   for( k = 0; k < NB_ADR_SREF; k++ )
   {
      double got, err;
      const char *mode;

      if( adrSrefOracle[k].period != lastPeriod )
      {
         lastPeriod = adrSrefOracle[k].period;
         retCode = TA_ADR( 0, nbBars - 1, history->high, history->low,
                           lastPeriod, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod - 1
             || nbElement != nbBars - lastPeriod + 1 )
         {
            printf( "ADR sref-oracle Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    lastPeriod, (int)retCode, (int)begIdx, (int)nbElement,
                    lastPeriod - 1, nbBars - lastPeriod + 1 );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         if( server_verify_active() )
         {
            double optIn[1];
            ErrorNumber e;
            int cmpBefore = server_verify_comparisons();

            optIn[0] = (double)lastPeriod;
            e = server_verify( "ADR", 0, nbBars - 1, nbBars,
                               retCode, begIdx, nbElement,
                               (const TA_Real*[]){ history->high, history->low, NULL },
                               optIn, 1,
                               (const TA_Real*[]){ out, NULL }, NULL );
            if( e != TA_TEST_PASS )
               return e;
            if( server_verify_comparisons() == cmpBefore )
            {
               printf( "ADR sref-oracle [N=%d]: compared no server despite live "
                       "pipes\n", lastPeriod );
               return TA_ADR_VACUOUS;
            }
         }
      }

      /* Each row's bar is hand-transcribed from the capture; the index below is
       * unchecked arithmetic on it, so a bar above the anchor would be a silent
       * out-of-bounds read rather than a mismatch. */
      if( adrSrefOracle[k].bar < begIdx
          || adrSrefOracle[k].bar - begIdx >= nbElement )
      {
         printf( "ADR sref-oracle Fail [N=%d]: golden bar %d is outside the "
                 "output [%d..%d]\n", adrSrefOracle[k].period,
                 adrSrefOracle[k].bar, (int)begIdx,
                 (int)(begIdx + nbElement - 1) );
         return TA_ADR_VACUOUS;
      }

      got = out[adrSrefOracle[k].bar - begIdx];
      g_adrSrefCmp++;
      if( !checkOracleValue( got, adrSrefOracle[k].want,
                             ADR_ORACLE_REL, ADR_ORACLE_ABS, &err, &mode ) )
      {
         printf( "ADR sref-oracle Fail [N=%d] at bar %d: got %.17g expected "
                 "%.17g (%s err %.3g, tol rel %g abs %g)\n",
                 adrSrefOracle[k].period, adrSrefOracle[k].bar, got,
                 adrSrefOracle[k].want, mode, err,
                 ADR_ORACLE_REL, ADR_ORACLE_ABS );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The edges no generic gate reaches.
 *
 * The flat and inverted corpora are built from exact binary fractions, so both
 * assertions there are equalities rather than tolerances.
 */
static ErrorNumber test_adr_edges( const TA_History *history )
{
#define ADR_EDGE_N 64
   static TA_Real high[ADR_EDGE_N], low[ADR_EDGE_N], out[ADR_CAP];
   const double posZero = 0.0;
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int i, period, nbBars = (int)history->nbBars;
   int nbWide = 0;

   /* Period 1: lookback 0, the seeding loop is skipped, and the output must be
    * the raw range, bit for bit. */
   retCode = TA_ADR( 0, nbBars - 1, history->high, history->low,
                     1, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != nbBars )
   {
      printf( "ADR period-1 Fail: rc=%d (%d,%d) expected (0,%d)\n",
              (int)retCode, (int)begIdx, (int)nbElement, nbBars );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   for( i = 0; i < nbBars; i++ )
   {
      double want = history->high[i] - history->low[i];

      g_adrEdgeCmp++;
      if( memcmp( &out[i], &want, sizeof(double) ) != 0 )
      {
         printf( "ADR period-1 Fail at out[%d]: got %.17g expected %.17g\n",
                 i, out[i], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      /* Ranges a float could not hold: the comparison above is then testing the
       * full double, not a short decimal literal. */
      if( want != 0.0 && (double)(float)want != want )
         nbWide++;
   }
   if( nbWide == 0 )
   {
      printf( "ADR period-1 Fail: corpus no longer has ranges needing more than "
              "float precision, the check above proves little\n" );
      return TA_ADR_VACUOUS;
   }

   /* Exactly one output: startIdx == endIdx == lookback. */
   for( period = 1; period <= 30; period++ )
   {
      retCode = TA_ADR( period - 1, period - 1, history->high, history->low,
                        period, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period - 1 || nbElement != 1 )
      {
         printf( "ADR single-output Fail [N=%d]: rc=%d (%d,%d) expected (%d,1)\n",
                 period, (int)retCode, (int)begIdx, (int)nbElement, period - 1 );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      g_adrEdgeCmp++;
   }

   /* Fewer bars than the lookback: TA_SUCCESS with nothing produced, not an
    * error, and outBegIdx pinned to 0. */
   retCode = TA_ADR( 0, 9, history->high, history->low,
                     40, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 0 || nbElement != 0 )
   {
      printf( "ADR short-input Fail: rc=%d (%d,%d) expected (0,0) with success\n",
              (int)retCode, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   g_adrEdgeCmp++;

   /* Every bar flat: the range is an exact zero and the running total never
    * accumulates anything else, so the answer is exactly +0.0 -- not merely
    * small, and never the negative zero #147 records as invisible to `== 0`. */
   for( i = 0; i < ADR_EDGE_N; i++ )
   {
      high[i] = 100.0 + (double)i * 0.25;
      low[i]  = high[i];
   }
   for( period = 1; period <= 30; period++ )
   {
      retCode = TA_ADR( 0, ADR_EDGE_N - 1, high, low, period,
                        &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period - 1
          || nbElement != ADR_EDGE_N - period + 1 )
      {
         printf( "ADR flat Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_adrEdgeCmp++;
         if( memcmp( &out[i], &posZero, sizeof(double) ) != 0 )
         {
            printf( "ADR flat Fail [N=%d] at out[%d]: got %.17g (%s), expected "
                    "exactly +0.0\n", period, i, out[i],
                    out[i] == 0.0 ? "negative zero" : "non-zero" );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   /* high below low is garbage input, not an error: no price-sanity validation
    * exists or should be added. The bars below are dyadic and the divisor is a
    * power of two at the periods swept, so the expected mean is exact. */
   for( i = 0; i < ADR_EDGE_N; i++ )
   {
      high[i] = 100.0;
      low[i]  = 100.5;          /* every range is exactly -0.5 */
   }
   for( period = 1; period <= 32; period *= 2 )
   {
      const double want = -0.5;

      retCode = TA_ADR( 0, ADR_EDGE_N - 1, high, low, period,
                        &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != period - 1
          || nbElement != ADR_EDGE_N - period + 1 )
      {
         printf( "ADR inverted Fail [N=%d]: rc=%d (%d,%d)\n",
                 period, (int)retCode, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_adrEdgeCmp++;
         if( memcmp( &out[i], &want, sizeof(double) ) != 0 )
         {
            printf( "ADR inverted Fail [N=%d] at out[%d]: got %.17g, expected "
                    "exactly %.17g -- high below low must average to a negative "
                    "range, not be rejected or clamped\n",
                    period, i, out[i], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
#undef ADR_EDGE_N
}

/* (4) IN-PLACE ALIASING, bitwise, on each component in turn.
 *
 * The trailing term is read before the store, so both are legal -- and they are
 * separate risks, because they are separate reads in the same statement and a
 * rewrite could reorder one past the store. Period 1 is included: there the
 * write index equals the trailing read index on every iteration, which is the
 * tightest the aliasing gets.
 */
static ErrorNumber test_adr_inplace( const TA_History *history )
{
   int g, i, which, nbBars;
   TA_RetCode rc;
   TA_Integer begRef, nbRef, begAlias, nbAlias;
   static TA_Real outRef[ADR_CAP];
   static TA_Real work[ADR_CAP];

   nbBars = (int)history->nbBars;

   for( g = 0; g < NB_ADR_GRID; g++ )
   {
      int period = adrGrid[g];

      rc = TA_ADR( 0, nbBars - 1, history->high, history->low,
                   period, &begRef, &nbRef, outRef );
      if( rc != TA_SUCCESS )
      {
         printf( "ADR in-place Fail [period %d]: reference retCode %d\n",
                 period, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( which = 0; which < 2; which++ )
      {
         const char *tag = which == 0 ? "outReal==inHigh" : "outReal==inLow";

         for( i = 0; i < nbBars; i++ )
            work[i] = which == 0 ? history->high[i] : history->low[i];

         if( which == 0 )
            rc = TA_ADR( 0, nbBars - 1, work, history->low,
                         period, &begAlias, &nbAlias, work );
         else
            rc = TA_ADR( 0, nbBars - 1, history->high, work,
                         period, &begAlias, &nbAlias, work );

         if( rc != TA_SUCCESS || begAlias != begRef || nbAlias != nbRef )
         {
            printf( "ADR in-place Fail [period %d, %s]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                    period, tag, (int)rc, (int)begAlias, (int)nbAlias,
                    (int)begRef, (int)nbRef );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }

         for( i = 0; i < nbRef; i++ )
         {
            g_adrAliasCmp++;
            if( memcmp( &work[i], &outRef[i], sizeof(double) ) != 0 )
            {
               printf( "ADR in-place Fail [period %d, %s] at out[%d]: separate "
                       "%.17g, in-place %.17g -- a store landed under a read the "
                       "next bar still needed\n",
                       period, tag, i, outRef[i], work[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (5) The startIdx/endIdx range sweep. TA_STABLE_EPSILON: a finite window
 * carried in a running accumulator, so a different anchor reaches the same
 * window through a different sequence of adds and subtracts and may move the
 * last ulp. No unstable-period id, matching the abstract metadata that
 * doRangeTestEx cross-checks the class against. */
typedef struct { int period; const TA_Real *high; const TA_Real *low; } AdrRangeParam;

static TA_RetCode adrRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   AdrRangeParam *p = (AdrRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_ADR_Lookback( p->period );
   return TA_ADR( startIdx, endIdx, p->high, p->low, p->period,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_adr_range( const TA_History *history )
{
   AdrRangeParam param;

   param.period = 14;
   param.high   = history->high;
   param.low    = history->low;

   return doRangeTestEx( adrRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}
