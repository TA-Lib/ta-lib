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
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091626 KL     First version (proposal-drafts issue #74).
 *  092126 MF,CC  External-oracle goldens and legs 9 to 14 (issue #430).
 *
 */

/* Description:
 *     Test TA_CTI, Ehlers' Correlation Trend Indicator.
 *
 *     The body correlates against y as BARS AGO, which runs backward in time,
 *     and negates the coefficient once. Dropping that negation reverses every
 *     value while keeping every magnitude, so legs 5 to 8 and 12 to 14 cannot
 *     see it; legs 1 to 4 and 9 to 11 can.
 *
 *     Every leg but 10 checks the body against itself, against TA_CORREL
 *     (which shares its #242 shift-and-reseed), or against exact arithmetic.
 *     Leg 10 is the one that checks the formula against code TA-Lib did not
 *     write.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define CTI_CAP 1100

/* Sends a leg's own vectors to the language servers, bit for bit (#427). The
 * synthetic corpora here reach no other sweep. The floor makes "compared
 * nothing" a failure: server_verify() answers PASS without comparing when it
 * skips a case, and every call site here is a success case. */
#define CTI_SERVER_VERIFY(sIdx, eIdx, nbBars, rc, beg, nb, inArr, period, outArr) \
   do {                                                                          \
      if( server_verify_active() )                                               \
      {                                                                          \
         int svCmp_ = server_verify_value_comparisons();                         \
         ErrorNumber svErr_ = server_verify(                                     \
            "CTI", (sIdx), (eIdx), (nbBars), (rc), (beg), (nb),                  \
            (const TA_Real*[]){ (inArr), NULL },                                 \
            (double[]){ (double)(period) }, 1,                                   \
            (const TA_Real*[]){ (outArr), NULL }, NULL );                        \
         if( svErr_ != TA_TEST_PASS )                                            \
            return svErr_;                                                       \
         if( server_verify_value_comparisons() == svCmp_ )                       \
         {                                                                       \
            printf( "CTI [period %d]: server_verify compared no server "         \
                    "despite live pipes\n", (int)(period) );                     \
            return TA_CTI_VACUOUS;                                               \
         }                                                                       \
      }                                                                          \
   } while(0)

/* Level 100, spread 1e-5, 40 bars. The goldens are the exact Pearson r against
 * an ascending ramp over the exact doubles, rounded to nearest. */
static const double ctiCancelCorpus[] = {
   100.00000052728242, 99.99999972063162, 100.00000331676542,
   100.00000094869883, 99.99999872896572, 100.00000199852343,
   100.00000237232099, 99.99999993158009, 100.00000488002743,
   100.00000308037387, 100.00000100535377, 100.00000413847395,
   99.99999792227185, 100.00000472610638, 100.00000376223514,
   100.00000484775724, 99.99999540147753, 100.00000179882429,
   100.00000041738383, 100.00000164245195, 99.9999962106636,
   100.00000378821413, 100.0000040362304, 99.99999972475116,
   99.99999609294525, 100.00000323786804, 100.00000188249547,
   99.99999598157812, 99.99999975536775, 99.99999944905079,
   100.00000269534941, 100.00000142433277, 99.99999929568433,
   100.0000042203537, 100.00000167494639, 100.00000480511711,
   99.99999542360842, 99.99999618084148, 99.99999731524505,
   100.00000447450944,
};
#define NB_CTI_CANCEL ((int)(sizeof(ctiCancelCorpus)/sizeof(double)))

/* period 20: 21 values */
static const double ctiCancelGolden[] = {
   0.04188697767321594, -0.1609529790917248, -0.13972795912991218,
   -0.004570546281627058, -0.08066290853068546, -0.31306516612322316,
   -0.24064790998780647, -0.20091833872852413, -0.370654659299611,
   -0.31425513456899035, -0.294917672600824, -0.23526001807554123,
   -0.12828623582924842, -0.25744914491221194, -0.0422099257555875,
   0.07752942283779674, 0.3300407782193946, 0.009707565037271146,
   -0.08032882030998995, -0.16043887061613263, -0.0051788511639493506,
};
#define NB_CTI_CANCEL_GOLDEN ((int)(sizeof(ctiCancelGolden)/sizeof(double)))

/* Leg 10. Absolute, because CTI crosses zero, where relative error is
 * unbounded. Against the exact column this body's worst over EVERY bar at
 * these periods is 2.6e-13. pandas-ta-classic and Pine evaluate raw-level sums
 * and sit further from exact themselves: worst over every bar 1.1e-11 and
 * 3.5e-11, both at period 5. */
#define CTI_EXACT_ABS  1e-12
#define CTI_ORACLE_ABS 1e-10

typedef struct { int period; int bar; double exact; double pandas; double pine; } CtiGolden;

/* Captured on the 252-bar TA_SREF close series at %.17g, by
 * ta-lib-oracles/capture_430_cti.py. `bar` is the ABSOLUTE bar index.
 *
 *   exact  : Pearson r against y = 0..n-1 in rational arithmetic over the
 *            exact doubles, rounded to the nearest double.
 *   pandas : pandas-ta-classic 0.6.52 (pandas 3.0.3, numpy 2.5.1),
 *            `ta.cti(close, length=n)`, with talib blocked from import.
 *   pine   : Pine `ta.correlation(close, bar_index, n)` on PyneCore 6.8.14,
 *            a from-scratch Pine runtime, not TradingView itself.
 *
 * Per period: the first three output bars, bars 100, 200 and 251, both
 * extremes, the bar nearest zero, the bars where this body, pandas and Pine
 * each sit furthest from exact, and at period 5 the periodic reseed bar and
 * the one after it. Bar 85 at period 5 is an exact zero. ta4j 0.22.6's
 * PearsonCorrelationIndicator and a transcription of the author's listing
 * were captured too and stay within CTI_ORACLE_ABS of every row. */
static const CtiGolden ctiGoldens[] =
{
   /* period 5: outBegIdx 4, outNBElement 248 */
   {  5,   4,      0.53240105681714722,
                    0.53240105681681482,       0.5324010568168871 },
   {  5,   5,     -0.30899021310025276,
                   -0.30899021309867442,     -0.30899021309939745 },
   {  5,   6,      -0.6655557020866546,
                     -0.665555702086015,     -0.66555570208555381 },
   {  5,  85,                        0,
                -3.6071751969220608e-14,                        0 },
   {  5,  98,     -0.98555044636971123,
                   -0.98555044636961608,     -0.98555044636957201 },
   {  5, 100,      0.40423272187263959,
                    0.40423272187263076,      0.40423272187200021 },
   {  5, 121,     -0.64295966373218105,
                   -0.64295966372076685,      -0.6429596637206958 },
   {  5, 130,      0.97851434339424148,
                    0.97851434339426091,      0.97851434339500676 },
   {  5, 163,     0.045773725458540616,
                   0.045773725458499684,     0.045773725454560578 },
   {  5, 164,    0.0090850711596036221,
                   0.009085071159577256,    0.0090850711559908679 },
   {  5, 200,       0.6243571009080825,
                    0.62435710090729857,      0.62435710090412755 },
   {  5, 201,     -0.31269549579641098,
                   -0.31269549579912581,     -0.31269549583133766 },
   {  5, 251,     -0.95421787944883718,
                   -0.95421787945280245,     -0.95421787946689895 },
   /* period 12: outBegIdx 11, outNBElement 241 */
   { 12,  11,      0.20812400793454888,
                     0.2081240079346183,      0.20812400793458491 },
   { 12,  12,      0.30474329407065576,
                    0.30474329407076844,       0.3047432940706305 },
   { 12,  13,     0.050243199619530686,
                   0.050243199619519542,     0.050243199619522713 },
   { 12, 100,     -0.41972988731793676,
                   -0.41972988731793415,     -0.41972988731749056 },
   { 12, 130,      0.96955411762295252,
                    0.96955411762300914,      0.96955411762258203 },
   { 12, 140,     -0.93251611628366515,
                   -0.93251611628367659,      -0.9325161162842851 },
   { 12, 165,     -0.48247905510942668,
                   -0.48247905510981987,     -0.48247905511282102 },
   { 12, 179,    0.0018916966547785328,
                  0.0018916966547656367,    0.0018916966531894953 },
   { 12, 200,     -0.92329671578222217,
                   -0.92329671578224815,      -0.9232967157829981 },
   { 12, 250,        0.165531938827742,
                    0.16553193882802686,      0.16553193881790312 },
   { 12, 251,     0.015475155243466916,
                   0.015475155243425548,     0.015475155230277952 },
   /* period 20: outBegIdx 19, outNBElement 233 */
   { 20,  19,     -0.41225578731748108,
                   -0.41225578731753487,     -0.41225578731744505 },
   { 20,  20,     -0.56789030745275226,
                   -0.56789030745272429,     -0.56789030745266966 },
   { 20,  21,     -0.61422678847574841,
                   -0.61422678847580536,     -0.61422678847585466 },
   { 20, 100,      0.67645802391684862,
                    0.67645802391691168,      0.67645802391677856 },
   { 20, 129,      0.95700553387310017,
                    0.95700553387315923,       0.9570055338728497 },
   { 20, 155,     -0.46877239804365289,
                   -0.46877239804360787,     -0.46877239804602261 },
   { 20, 183,    0.0077833962592747611,
                  0.0077833962592765366,    0.0077833962583975435 },
   { 20, 191,     -0.94343712377397237,
                   -0.94343712377399414,     -0.94343712377525257 },
   { 20, 200,     -0.93901027837285078,
                   -0.93901027837284434,     -0.93901027837358642 },
   { 20, 251,     -0.67160132180809173,
                   -0.67160132180782184,     -0.67160132181002052 },
   /* period 50: outBegIdx 49, outNBElement 203 */
   { 50,  49,     -0.53121268160634982,
                   -0.53121268160635648,      -0.5312126816063576 },
   { 50,  50,     -0.51588571782214465,
                   -0.51588571782212367,     -0.51588571782213888 },
   { 50,  51,     -0.47990263994231502,
                   -0.47990263994223115,     -0.47990263994229909 },
   { 50,  71,       0.5321970691987199,
                    0.53219706919863496,       0.5321970691986615 },
   { 50, 100,      0.91377409291034772,
                     0.9137740929103465,       0.9137740929103636 },
   { 50, 101,      0.91960208284385347,
                    0.91960208284384892,      0.91960208284386447 },
   { 50, 168,     -0.49911041442536674,
                   -0.49911041442534787,     -0.49911041442559284 },
   { 50, 171,     -0.50926246281386334,
                     -0.509262462813864,     -0.50926246281425769 },
   { 50, 180,    0.0022286453075005845,
                  0.0022286453075005949,    0.0022286453072470455 },
   { 50, 200,     -0.56887629805559004,
                   -0.56887629805555129,      -0.5688762980558455 },
   { 50, 219,     -0.96192102645773114,
                    -0.9619210264577317,      -0.9619210264578566 },
   { 50, 251,      0.84120615542054422,
                     0.8412061554205581,      0.84120615542058153 },
};
#define NB_CTI_GOLDEN ((int)(sizeof(ctiGoldens)/sizeof(CtiGolden)))

/* Coverage counters. Every leg is silent on success, so a count that reached
 * zero is the only remaining way one could run while comparing nothing. */
static int g_ctiDiffCmp;
static int g_ctiSignCmp;
static int g_ctiEndpointCmp;
static int g_ctiCancelCmp;
static int g_ctiReseedCmp;
static int g_ctiGoldenCmp;
static int g_ctiPairCmp;
static int g_ctiBoundCmp;
static int g_ctiFlatCmp;
static int g_ctiAliasCmp;
static int g_ctiHistCmp;
static int g_ctiHistFlatCmp;
static int g_ctiLongCmp;
static int g_ctiUlpCmp;

/**** Local functions declarations. ****/
static ErrorNumber test_cti_sign( void );
static ErrorNumber test_cti_differential( const TA_History *history );
static ErrorNumber test_cti_endpoints( void );
static ErrorNumber test_cti_cancellation( void );
static ErrorNumber test_cti_reseed( const TA_History *history );
static ErrorNumber test_cti_oracle( const TA_History *history );
static ErrorNumber test_cti_two_bar( const TA_History *history );
static ErrorNumber test_cti_bounds( const TA_History *history );
static ErrorNumber test_cti_flat( const TA_History *history );
static ErrorNumber test_cti_aliasing( const TA_History *history );
static ErrorNumber test_cti_range( const TA_History *history );
static ErrorNumber test_cti_history( void );
static ErrorNumber test_cti_long( void );
static ErrorNumber test_cti_ulp( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_cti( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   g_ctiDiffCmp = g_ctiSignCmp = g_ctiEndpointCmp = g_ctiCancelCmp = 0;
   g_ctiReseedCmp = g_ctiGoldenCmp = g_ctiPairCmp = 0;
   g_ctiBoundCmp = g_ctiFlatCmp = g_ctiAliasCmp = 0;
   g_ctiHistCmp = g_ctiHistFlatCmp = g_ctiLongCmp = g_ctiUlpCmp = 0;

   err = test_cti_sign();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_differential( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_endpoints();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_cancellation();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_reseed( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_oracle( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_two_bar( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_bounds( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_flat( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_aliasing( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_range( history );
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_history();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_long();
   if( err != TA_TEST_PASS )
      return err;

   err = test_cti_ulp();
   if( err != TA_TEST_PASS )
      return err;

   /* LITERAL counts rather than floors: on the shipped 252-bar corpus every
    * leg above is deterministic. */
   if( history->nbBars == 252 )
   {
      /* One source for each count: a second copy in the message can disagree
       * with the one the gate tests, and only a mutation would show it.  */
      static const struct { const char *leg; int want; const int *got; } cov[] = {
         { "sign",             3330, &g_ctiSignCmp     },
         { "differential",     2007, &g_ctiDiffCmp     },
         { "endpoints",        6678, &g_ctiEndpointCmp },
         { "cancellation",       21, &g_ctiCancelCmp   },
         { "reseed",             21, &g_ctiReseedCmp   },
         { "oracle",             46, &g_ctiGoldenCmp   },
         { "two-bar",           251, &g_ctiPairCmp     },
         { "bounds",           2007, &g_ctiBoundCmp    },
         { "flat",            47790, &g_ctiFlatCmp     },
         { "alias",            2007, &g_ctiAliasCmp    },
         { "history",          1356, &g_ctiHistCmp     },
         { "history flat",     2628, &g_ctiHistFlatCmp },
         { "long walk",      299928, &g_ctiLongCmp     },
         { "one ulp",          4001, &g_ctiUlpCmp      },
      };
      unsigned int c;

      for( c = 0; c < sizeof(cov)/sizeof(cov[0]); c++ )
      {
         if( *cov[c].got != cov[c].want )
         {
            printf( "CTI Fail: the %s leg compared %d times, not the %d this "
                    "file was written with\n",
                    cov[c].leg, *cov[c].got, cov[c].want );
            return TA_CTI_VACUOUS;
         }
      }
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions. ****/

/* (2) THE SIGN. A rising series must give a positive coefficient. The series
 * is concave, so every window of three or more bars lies strictly inside
 * (0, 1): the leg asserts the sign of an interior value, which leg 3's exact
 * endpoints cannot stand in for. */
static ErrorNumber test_cti_sign( void )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int n, i;

   for( i = 0; i < 400; i++ )
      in[i] = 10.0 * sqrt( 1.0 + (double)i );

   for( n = 3; n <= 59; n += 7 )
   {
      retCode = TA_CTI( 0, 399, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI sign Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      CTI_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiSignCmp++;
         if( !(out[i] > 0.0 && out[i] < 1.0) )
         {
            printf( "CTI sign Fail [N=%d] bar %d: %.17g on a strictly rising "
                    "concave series, expected inside (0, 1). The ramp is "
                    "bars-ago and runs backward, so the coefficient must be "
                    "negated once\n", n, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (1) Differential against shipped TA_CORREL over an ascending ramp, which is
 * what this function is by definition. Worst absolute difference on this
 * corpus is 8.9e-12, at n = 2, so 1e-9 leaves two orders and still fails
 * anything structural: removing the negation moves it to 2.0.
 */
static ErrorNumber test_cti_differential( const TA_History *history )
{
   static TA_Real ramp[CTI_CAP], cti[CTI_CAP], cor[CTI_CAP];
   TA_Integer begC, nbC, begR, nbR;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   for( i = 0; i < nbBars; i++ )
      ramp[i] = (double)i;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, nbBars-1, history->close, n, &begC, &nbC, cti );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI differential Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      retCode = TA_CORREL( 0, nbBars-1, history->close, ramp, n,
                           &begR, &nbR, cor );
      if( retCode != TA_SUCCESS || begR != begC || nbR != nbC )
      {
         printf( "CTI differential Fail [N=%d]: CORREL rc=%d (%d,%d) vs "
                 "(%d,%d)\n", n, (int)retCode, begR, nbR, begC, nbC );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      for( i = 0; i < nbC; i++ )
      {
         g_ctiDiffCmp++;
         if( !(fabs( cti[i] - cor[i] ) <= 1e-9) )
         {
            printf( "CTI differential Fail [N=%d] bar %d: %.17g against "
                    "TA_CORREL's %.17g, absolute %.3e over 1e-9\n",
                    n, begC+i, cti[i], cor[i], fabs(cti[i]-cor[i]) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (3) Exact endpoints. A strictly linear rise is +1.0 exactly and a strictly
 * linear fall -1.0 exactly: the 0.5-step ramp keeps every sum exact. That
 * makes this leg independent of the clamp; leg 5 is the clamp's gate.
 */
static ErrorNumber test_cti_endpoints( void )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int n, i, dir;
   double want;

   for( dir = 0; dir < 2; dir++ )
   {
      want = dir == 0 ? 1.0 : -1.0;
      for( i = 0; i < 400; i++ )
         in[i] = dir == 0 ? 10.0 + 0.5*(double)i : 10.0 - 0.5*(double)i;

      for( n = 2; n <= 60; n += 7 )
      {
         retCode = TA_CTI( 0, 399, in, n, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "CTI endpoint Fail [N=%d dir=%d]: rc=%d\n",
                    n, dir, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         CTI_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
         for( i = 0; i < nbElement; i++ )
         {
            g_ctiEndpointCmp++;
            if( out[i] != want )
            {
               printf( "CTI endpoint Fail [N=%d] bar %d: %.17g, expected "
                       "exactly %.1f on a perfectly linear series\n",
                       n, begIdx+i, out[i], want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (4) Cancellation (#242). On this corpus a literal transcription of the
 * author's listing, raw sums over the price levels, errs by 2.05e-01 against
 * the exact reference; this body errs by 6.1e-17. The 1e-9 gate sits seven
 * orders above one and eight below the other.
 */
static ErrorNumber test_cti_cancellation( void )
{
   static TA_Real out[64];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int i;
   double err;

   retCode = TA_CTI( 0, NB_CTI_CANCEL-1, ctiCancelCorpus, 20,
                     &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 19 || nbElement != NB_CTI_CANCEL_GOLDEN )
   {
      printf( "CTI cancellation Fail: rc=%d (%d,%d), expected (19,%d)\n",
              (int)retCode, begIdx, nbElement, NB_CTI_CANCEL_GOLDEN );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   CTI_SERVER_VERIFY( 0, NB_CTI_CANCEL-1, NB_CTI_CANCEL, retCode, begIdx,
                      nbElement, ctiCancelCorpus, 20, out );

   for( i = 0; i < nbElement; i++ )
   {
      g_ctiCancelCmp++;
      err = fabs( out[i] - ctiCancelGolden[i] );
      if( !(err <= 1e-9) )
      {
         printf( "CTI cancellation Fail at bar %d: %.17g against %.17g, "
                 "absolute %.3e over 1e-9\n",
                 begIdx+i, out[i], ctiCancelGolden[i], err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (9) The reseed. Leg 4's corpus behind twenty real closes: the shift is now
 * taken far from the data, and only a rebuild recovers leg 4's goldens on the
 * windows lying wholly inside the corpus. Leg 4 alone never rebuilds.
 */
static ErrorNumber test_cti_reseed( const TA_History *history )
{
   static TA_Real in[20 + NB_CTI_CANCEL], out[20 + NB_CTI_CANCEL];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbIn = 20 + NB_CTI_CANCEL;
   int i, bar;
   double err;

   if( history->nbBars < 20 )
      return TA_TEST_PASS;

   for( i = 0; i < 20; i++ )
      in[i] = history->close[i];
   for( i = 0; i < NB_CTI_CANCEL; i++ )
      in[20+i] = ctiCancelCorpus[i];

   retCode = TA_CTI( 0, nbIn-1, in, 20, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 19 || nbElement != nbIn-19 )
   {
      printf( "CTI reseed Fail: rc=%d (%d,%d), expected (19,%d)\n",
              (int)retCode, begIdx, nbElement, nbIn-19 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   CTI_SERVER_VERIFY( 0, nbIn-1, nbIn, retCode, begIdx, nbElement, in, 20, out );

   for( i = 0; i < NB_CTI_CANCEL_GOLDEN; i++ )
   {
      bar = 20 + 19 + i;
      g_ctiReseedCmp++;
      err = fabs( out[bar-begIdx] - ctiCancelGolden[i] );
      if( !(err <= 1e-9) )
      {
         printf( "CTI reseed Fail at bar %d: %.17g against %.17g, absolute "
                 "%.3e over 1e-9\n", bar, out[bar-begIdx], ctiCancelGolden[i],
                 err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (10) The frozen external-oracle goldens, plus the cross-language replay. */
static ErrorNumber test_cti_oracle( const TA_History *history )
{
   static TA_Real out[CTI_CAP];
   TA_Integer begIdx = 0, nbElement = 0;
   TA_RetCode retCode;
   int k, lastPeriod = -1;
   const char *which;
   double got, want;

   if( history->nbBars != 252 )
      return TA_TEST_PASS;

   for( k = 0; k < NB_CTI_GOLDEN; k++ )
   {
      const CtiGolden *g = &ctiGoldens[k];

      if( g->period != lastPeriod )
      {
         lastPeriod = g->period;
         retCode = TA_CTI( 0, 251, history->close, lastPeriod,
                           &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || begIdx != lastPeriod-1
             || nbElement != 252-begIdx )
         {
            printf( "CTI oracle Fail [N=%d]: rc=%d (%d,%d)\n", lastPeriod,
                    (int)retCode, begIdx, nbElement );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         CTI_SERVER_VERIFY( 0, 251, 252, retCode, begIdx, nbElement,
                            history->close, lastPeriod, out );
      }

      /* A golden's bar is hand-transcribed and indexes `out` unchecked. */
      if( g->bar < begIdx || g->bar - begIdx >= nbElement )
      {
         printf( "CTI oracle Fail [N=%d]: golden bar %d is outside the output "
                 "[%d..%d]\n", g->period, g->bar, begIdx,
                 begIdx + nbElement - 1 );
         return TA_CTI_VACUOUS;
      }

      got = out[g->bar - begIdx];
      g_ctiGoldenCmp++;
      which = NULL;
      if( !(fabs( got - g->exact ) <= CTI_EXACT_ABS) )
      {
         which = "exact";
         want  = g->exact;
      }
      else if( !(fabs( got - g->pandas ) <= CTI_ORACLE_ABS) )
      {
         which = "pandas-ta-classic";
         want  = g->pandas;
      }
      else if( !(fabs( got - g->pine ) <= CTI_ORACLE_ABS) )
      {
         which = "Pine";
         want  = g->pine;
      }
      if( which )
      {
         printf( "CTI oracle Fail [N=%d] at bar %d: got %.17g, %s expects "
                 "%.17g (absolute %.3e)\n", g->period, g->bar, got, which,
                 want, fabs( got - want ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (11) Period 2. Two points always lie on a line, so every window is exactly
 * +1 or -1 by the sign of the change, and 0.0 where the two closes are equal
 * (bar 101 of the 252-bar corpus). Worst measured here is 3.7e-11.
 */
static ErrorNumber test_cti_two_bar( const TA_History *history )
{
   static TA_Real out[CTI_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int i, bar;
   double want;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   retCode = TA_CTI( 0, nbBars-1, history->close, 2, &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != 1 )
   {
      printf( "CTI two-bar Fail: rc=%d beg %d\n", (int)retCode, begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nbElement; i++ )
   {
      bar = begIdx + i;
      g_ctiPairCmp++;
      if( history->close[bar] == history->close[bar-1] )
      {
         if( out[i] != 0.0 )
         {
            printf( "CTI two-bar Fail at bar %d: %.17g on two equal closes, "
                    "expected exactly 0.0\n", bar, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         continue;
      }
      want = history->close[bar] > history->close[bar-1] ? 1.0 : -1.0;
      if( !(fabs( out[i] - want ) <= 1e-9) )
      {
         printf( "CTI two-bar Fail at bar %d: %.17g, expected %.1f within "
                 "1e-9\n", bar, out[i], want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (5) |CTI| <= 1 exactly, over the caller's corpus: the clamp's gate. Blind
 * on its own to a sign reversal. */
static ErrorNumber test_cti_bounds( const TA_History *history )
{
   static TA_Real out[CTI_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, nbBars-1, history->close, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI bounds Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      CTI_SERVER_VERIFY( 0, nbBars-1, nbBars, retCode, begIdx, nbElement,
                         history->close, n, out );
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiBoundCmp++;
         if( !(fabs( out[i] ) <= 1.0) )
         {
            printf( "CTI bounds Fail [N=%d] bar %d: %.17g is outside [-1,1]\n",
                    n, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (6) A window with no spread returns exactly 0.0, TA_CORREL's convention.
 * The flat stretch follows real closes, so the running sums enter it carrying
 * residue from the values that left; which residue survives depends on the
 * period, so every period runs. Holding the previous output, as the author's
 * listing does, fails here.
 */
static ErrorNumber test_cti_flat( const TA_History *history )
{
   static TA_Real in[400], out[400];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int t, n, i, bar;
   double levels[3];

   if( history->nbBars < 252 )
      return TA_TEST_PASS;

   levels[0] = history->close[26];
   levels[1] = history->close[251];
   levels[2] = 0.1;

   for( t = 0; t < 3; t++ )
   {
      for( i = 0; i < 100; i++ )
         in[i] = history->close[i];
      for( i = 100; i < 400; i++ )
         in[i] = levels[t];

      for( n = 2; n <= 60; n++ )
      {
         retCode = TA_CTI( 0, 399, in, n, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "CTI flat Fail [N=%d]: rc=%d\n", n, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         CTI_SERVER_VERIFY( 0, 399, 400, retCode, begIdx, nbElement, in, n, out );
         for( i = 0; i < nbElement; i++ )
         {
            bar = begIdx + i;
            if( bar - (n-1) < 100 )
               continue;
            g_ctiFlatCmp++;
            if( out[i] != 0.0 )
            {
               printf( "CTI flat Fail [N=%d level %.17g] bar %d: %.17g, "
                       "expected exactly 0.0\n", n, levels[t], bar, out[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (7) In-place aliasing. Each bar's output lands on the cell that the same
 * bar's slide reads as its trailing value, so the read has to come first.
 * Nothing is sent to the servers: leg 5 already routes the non-aliased call.
 */
static ErrorNumber test_cti_aliasing( const TA_History *history )
{
   static TA_Real clean[CTI_CAP], alias[CTI_CAP];
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int n, i;

   if( nbBars > CTI_CAP )
      nbBars = CTI_CAP;

   for( n = 2; n <= 60; n += 7 )
   {
      retCode = TA_CTI( 0, nbBars-1, history->close, n,
                        &begIdx, &nbElement, clean );
      if( retCode != TA_SUCCESS )
      {
         printf( "CTI alias Fail [N=%d]: rc=%d\n", n, (int)retCode );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbBars; i++ )
         alias[i] = history->close[i];
      retCode = TA_CTI( 0, nbBars-1, alias, n, &begIdx2, &nbElement2, alias );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "CTI alias Fail [N=%d]: rc=%d shape (%d,%d) vs (%d,%d)\n",
                 n, (int)retCode, begIdx2, nbElement2, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         g_ctiAliasCmp++;
         if( memcmp( &clean[i], &alias[i], sizeof(double) ) != 0 )
         {
            printf( "CTI alias Fail [N=%d] bar %d: separate %.17g, in-place "
                    "%.17g\n", n, begIdx+i, clean[i], alias[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (8) The startIdx/endIdx range sweep. TA_STABLE_EPSILON: the first shift is
 * taken from the call's own startIdx, so two calls covering the same bar from
 * different starts do not agree bit for bit (measured up to 4e-13 here). No
 * unstable period: the window is finite.
 */
typedef struct { int period; const TA_Real *in; } CtiRangeParam;

static TA_RetCode ctiRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   CtiRangeParam *p = (CtiRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_CTI_Lookback( p->period );
   return TA_CTI( startIdx, endIdx, p->in, p->period,
                  outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_cti_range( const TA_History *history )
{
   CtiRangeParam param;

   param.period = 20;
   param.in     = history->close;

   return doRangeTestEx( ctiRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (12) History. After a flat lead-in, the series jumps away and decays
 * geometrically back onto its first value, which is the shift until a rebuild,
 * so once the spread has gone the running sums hold nothing but the rounding
 * left by the values that departed. The lead-in keeps the largest sum of
 * squares out of the first window, where it would be counted anyway. Every
 * window must agree with the same window computed with no history
 * (startIdx == endIdx), and a fully flat one must be exactly 0.0. A rebuild
 * trigger that compares against the current sum of squares, rather than the
 * largest one held since the last rebuild, misses this and errs by up to 0.9.
 */
static ErrorNumber test_cti_history( void )
{
   static const double levels[] = { 100.37, 0.1, 1.0, 3.3 };
   static const double ratios[] = { 0.1, 0.05, 0.2 };
   static const int periods[]   = { 3, 5, 8, 12, 20 };
   static TA_Real in[75], out[75];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one;
   int lv, rt, pr, n, i, j, bar, flat;
   double dev;

   for( lv = 0; lv < 4; lv++ )
   for( rt = 0; rt < 3; rt++ )
   {
      for( j = 0; j < 20; j++ )
         in[j] = levels[lv];
      dev = 0.5*levels[lv];
      for( j = 1; j <= 15; j++ )
      {
         dev *= ratios[rt];
         in[19+j] = (j*7) % 3 ? levels[lv] + dev : levels[lv] - dev;
      }
      for( j = 35; j < 75; j++ )
         in[j] = levels[lv];

      for( pr = 0; pr < 5; pr++ )
      {
         n = periods[pr];
         retCode = TA_CTI( 0, 74, in, n, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS )
         {
            printf( "CTI history Fail [N=%d]: rc=%d\n", n, (int)retCode );
            return TA_TESTUTIL_TFRR_BAD_RETCODE;
         }
         CTI_SERVER_VERIFY( 0, 74, 75, retCode, begIdx, nbElement, in, n, out );

         for( i = 0; i < nbElement; i++ )
         {
            bar  = begIdx + i;
            flat = 1;
            for( j = bar-n+1; j <= bar; j++ )
               if( in[j] != in[bar] )
                  flat = 0;

            if( flat )
            {
               g_ctiHistFlatCmp++;
               if( out[i] != 0.0 )
               {
                  printf( "CTI history Fail [N=%d level %.17g ratio %g] bar %d: "
                          "%.17g on a flat window, expected exactly 0.0\n",
                          n, levels[lv], ratios[rt], bar, out[i] );
                  return TA_TESTUTIL_TFRR_BAD_CALCULATION;
               }
               continue;
            }

            retCode = TA_CTI( bar, bar, in, n, &begOne, &nbOne, &one );
            if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
            {
               printf( "CTI history Fail [N=%d]: one-bar call at %d rc=%d "
                       "(%d,%d)\n", n, bar, (int)retCode, begOne, nbOne );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }
            g_ctiHistCmp++;
            if( !(fabs( out[i] - one ) <= 1e-9) )
            {
               printf( "CTI history Fail [N=%d level %.17g ratio %g] bar %d: "
                       "%.17g against %.17g with no history, absolute %.3e "
                       "over 1e-9\n", n, levels[lv], ratios[rt], bar, out[i],
                       one, fabs( out[i] - one ) );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (13) The periodic rebuild. A long walk that never comes near its shift fires
 * no other rebuild, and the rounding the running sums carry grows with every
 * bar they slide over. Every window must agree with the same window computed
 * with no history: measured worst 1.5e-11 here, and 4e-10 to 1.2e-8 across
 * these periods once the rebuild every 32 windows is removed. Not sent to the
 * servers, whose request buffer is far smaller than this vector; leg 10's
 * period-5 call already takes them through a periodic rebuild.
 */
#define CTI_LONG_BARS 100000
static ErrorNumber test_cti_long( void )
{
   static const int periods[] = { 5, 20, 50 };
   static TA_Real in[CTI_LONG_BARS], out[CTI_LONG_BARS];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one, v;
   unsigned int seed;
   int pr, n, i, bar;

   seed = 12345u;
   v    = 100.0;
   for( i = 0; i < CTI_LONG_BARS; i++ )
   {
      seed = (seed*1103515245u + 12345u) & 0x7fffffffu;
      v += (double)((int)((seed >> 8) % 2001u) - 1000) * 1e-5;
      in[i] = v;
   }

   for( pr = 0; pr < 3; pr++ )
   {
      n = periods[pr];
      retCode = TA_CTI( 0, CTI_LONG_BARS-1, in, n, &begIdx, &nbElement, out );
      if( retCode != TA_SUCCESS || begIdx != n-1
          || nbElement != CTI_LONG_BARS-begIdx )
      {
         printf( "CTI long walk Fail [N=%d]: rc=%d (%d,%d)\n", n, (int)retCode,
                 begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      for( bar = begIdx; bar < CTI_LONG_BARS; bar++ )
      {
         retCode = TA_CTI( bar, bar, in, n, &begOne, &nbOne, &one );
         if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
         {
            printf( "CTI long walk Fail [N=%d]: one-bar call at %d rc=%d "
                    "(%d,%d)\n", n, bar, (int)retCode, begOne, nbOne );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         g_ctiLongCmp++;
         if( !(fabs( out[bar-begIdx] - one ) <= 1e-10) )
         {
            printf( "CTI long walk Fail [N=%d] bar %d: %.17g against %.17g with "
                    "no history, absolute %.3e over 1e-10\n", n, bar,
                    out[bar-begIdx], one, fabs( out[bar-begIdx] - one ) );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (14) One ulp in a flat stretch. A window flat to within the rounding of its
 * own mean is re-anchored on one of its own values; anchored on the mean, the
 * rebuild would fire again on every bar and the windows holding the odd bar
 * would come out up to 1.6e-10 from the same window computed with no history,
 * where this body is within 2e-15.
 */
#define CTI_ULP_WALK   500
#define CTI_ULP_PERIOD 2000
#define CTI_ULP_BARS   (CTI_ULP_WALK + 3*CTI_ULP_PERIOD)
static ErrorNumber test_cti_ulp( void )
{
   static TA_Real in[CTI_ULP_BARS], out[CTI_ULP_BARS];
   TA_Integer begIdx, nbElement, begOne, nbOne;
   TA_RetCode retCode;
   TA_Real one;
   unsigned int seed;
   int cents, i, bar;

   seed  = 777u;
   cents = 10037;
   for( i = 0; i < CTI_ULP_WALK; i++ )
   {
      seed = (seed*1103515245u + 12345u) & 0x7fffffffu;
      cents += (int)((seed >> 8) % 5u) - 2;
      in[i] = (double)cents / 100.0;
   }
   for( i = CTI_ULP_WALK; i < CTI_ULP_BARS; i++ )
      in[i] = 100.37;
   in[CTI_ULP_WALK + CTI_ULP_PERIOD + 5] = nextafter( 100.37, 200.0 );

   retCode = TA_CTI( 0, CTI_ULP_BARS-1, in, CTI_ULP_PERIOD,
                     &begIdx, &nbElement, out );
   if( retCode != TA_SUCCESS || begIdx != CTI_ULP_PERIOD-1
       || nbElement != CTI_ULP_BARS-begIdx )
   {
      printf( "CTI one-ulp Fail: rc=%d (%d,%d)\n", (int)retCode, begIdx,
              nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   CTI_SERVER_VERIFY( 0, CTI_ULP_BARS-1, CTI_ULP_BARS, retCode, begIdx,
                      nbElement, in, CTI_ULP_PERIOD, out );

   for( bar = CTI_ULP_WALK + CTI_ULP_PERIOD - 1; bar < CTI_ULP_BARS; bar++ )
   {
      retCode = TA_CTI( bar, bar, in, CTI_ULP_PERIOD, &begOne, &nbOne, &one );
      if( retCode != TA_SUCCESS || begOne != bar || nbOne != 1 )
      {
         printf( "CTI one-ulp Fail: one-bar call at %d rc=%d (%d,%d)\n",
                 bar, (int)retCode, begOne, nbOne );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      g_ctiUlpCmp++;
      if( !(fabs( out[bar-begIdx] - one ) <= 1e-13) )
      {
         printf( "CTI one-ulp Fail at bar %d: %.17g against %.17g with no "
                 "history, absolute %.3e over 1e-13\n", bar, out[bar-begIdx],
                 one, fabs( out[bar-begIdx] - one ) );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
