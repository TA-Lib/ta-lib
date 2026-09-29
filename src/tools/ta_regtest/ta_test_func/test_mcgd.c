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
 *  092926 MF,CC  First version (issue #471).
 */

/* Description:
 *
 *   Test TA_MCGD (McGinley Dynamic).
 *
 *   Legs:
 *     1. Frozen goldens on the issue's 1200-bar series, and McGinley's own
 *        worked example (N = 7, p. 17), which pins exponent 4 and k = 1.
 *     2. Oracle rows from ta-lib-oracles: pandas-ta-classic (same seed, from
 *        the first output), trading-signals (k = 0.6 at interval I is
 *        TA_MCGD(0.6*I)), LEAN (SMA seed, tail only); and LEAN's published
 *        talipp table on SPY (SMA seed, tail only).
 *     3. Seed and anchor, bit-exact: a literal re-derivation seeded with
 *        x[startIdx - lookback], over periods, unstable periods, start and
 *        end indexes.
 *     4. Lookback and the unstable-period id.
 *     5. Degenerate and extreme input: each non-finite path of the step, a
 *        positive series that takes the line to or below 0, a ratio LEAN
 *        refuses.
 *     6. The startIdx/endIdx range sweep.
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
#define MCGD_NB        1200
#define MCGD_GOLD_REL  1e-14
#define MCGD_BOOK_REL  1e-15

#define MCGD_SWEEP_CMP 170963
#define MCGD_DEGEN_CMP 209

/* A bar at 0.44-0.45 of the line cancels 1 + (r-1)/(N r^4) down to a few
 * percent, which magnifies rounding in the line and in the synthesized input
 * about a hundredfold. */
#define MCGD_CANCEL_REL 1e-13

typedef struct { int period; int bar; double want; } McgdGolden;

/* 60-digit re-derivation over the exact binary input, rounded to 17 digits.
 * pandas-ta-classic 0.6.52 agrees to at most 4.2e-16 on every row (#471). */
static const McgdGolden mcgdGolden[] =
{
   { 14,   13, 104.56984870222737 },
   { 14,   14, 104.92303278442968 },
   { 14,   15, 105.27535079028175 },
   { 14,   50, 103.12719679511233 },
   { 14,  100, 112.89970701014078 },
   { 14,  500, 175.75372293610594 },
   { 14,  999, 245.58749629044835 },
   { 14, 1199, 279.20106447901852 },
   { 10,    9, 104.21143170788586 },
   { 10, 1199, 281.27841736001127 },
   { 50,   49, 101.43750998151752 },
   { 50, 1199, 270.84466110358636 },
};
#define NB_MCGD_GOLDEN ((int)(sizeof(mcgdGolden)/sizeof(mcgdGolden[0])))

/* Bar 121 of the series' first 300 bars replaced by frac * MD[120]
 * (MD[120] = 113.04991518555316): 0.45 leaves the line stuck near 4.74, 0.44
 * takes it below 0. 60-digit re-derivation, as mcgdGolden. */
typedef struct { double bar121; int bar; double want; } McgdCancel;
static const McgdCancel mcgdCancel[] =
{
   { 50.87246183349892,  121,  4.743335871329709  },
   { 50.87246183349892,  122,  4.743362488068026  },
   { 50.87246183349892,  200,  4.744785554635167  },
   { 50.87246183349892,  299,  4.7461761811089485 },
   { 49.741962681643386, 121, -7.597917938841933  },
   { 49.741962681643386, 122, -7.597721995553392  },
   { 49.741962681643386, 200, -7.587409814820774  },
   { 49.741962681643386, 299, -7.577483337433553  },
};
#define NB_MCGD_CANCEL ((int)(sizeof(mcgdCancel)/sizeof(mcgdCancel[0])))

/* ta-lib-oracles capture_471_mcgd.py (arm commit b4746c1), on mcgdSynth's
 * series. pandas-ta-classic 0.6.52 mcgd(close, length=N) seeds with the first
 * bar as TA_MCGD does and publishes from bar 0: within 2.2e-16 of TA_MCGD on
 * every bar from N-1 at every N captured (2 to 200). The rows are the first
 * eleven outputs, where a seed or anchor error is largest, and a few late ones. */
static const McgdGolden mcgdPandas[] =
{
   {   2,    1, 101.09804237988908 },
   {   2,    2, 102.64359744468672 },
   {   2,    3, 104.32026896710614 },
   {   2,    4, 105.91441506826841 },
   {   2,    5, 107.26916627570974 },
   {   2,    6, 108.28618773105936 },
   {   2,    7, 108.939595063916 },
   {   2,    8, 109.27976624162912 },
   {   2,    9, 109.41455168266661 },
   {   2,   10, 109.47226655242036 },
   {   2,   11, 109.56298964604771 },
   {   2,  100, 123.30753216318989 },
   {   2, 1199, 288.5230143627825 },
   {  10,    9, 104.21143170788585 },
   {  10,   10, 104.64729957363146 },
   {  10,   11, 105.06266610224834 },
   {  10,   12, 105.46991224788349 },
   {  10,   13, 105.87803983140832 },
   {  10,   14, 106.2886431232341 },
   {  10,   15, 106.6938005783885 },
   {  10,   16, 107.07560436066761 },
   {  10,   17, 107.40652787268598 },
   {  10,   18, 107.65021812770914 },
   {  10,   19, 107.76324804625196 },
   {  10,  100, 115.12769926143069 },
   {  10, 1199, 281.27841736001125 },
   {  14,   13, 104.56984870222738 },
   {  14,   14, 104.92303278442968 },
   {  14,   15, 105.27535079028175 },
   {  14,   16, 105.61524617556036 },
   {  14,   17, 105.9248501392836 },
   {  14,   18, 106.18018765174074 },
   {  14,   19, 106.35174279097379 },
   {  14,   20, 106.40632260335487 },
   {  14,   21, 106.31154571578588 },
   {  14,   22, 106.04376494572621 },
   {  14,   23, 105.59820245539228 },
   {  14,   50, 103.12719679511231 },
   {  14,  500, 175.753722936106 },
   {  14, 1199, 279.2010644790185 },
   {  50,   49, 101.43750998151751 },
   {  50,   50, 101.605290312734 },
   {  50,   51, 101.77303638073397 },
   {  50,   52, 101.94010686693426 },
   {  50,   53, 102.10667888421276 },
   {  50,   54, 102.27354826670165 },
   {  50,   55, 102.44176146563454 },
   {  50,   56, 102.61217039194517 },
   {  50,   57, 102.78506364500417 },
   {  50,   58, 102.95997579178129 },
   {  50,   59, 103.13565804845221 },
   {  50,  300, 132.97266536798656 },
   {  50, 1199, 270.8446611035865 },
   { 200,  199, 106.5894277362585 },
   { 200,  200, 106.638502963186 },
   { 200,  201, 106.68691817461165 },
   { 200,  202, 106.73496738619835 },
   { 200,  203, 106.78283084902775 },
   { 200,  204, 106.83053673524613 },
   { 200,  205, 106.87797006379543 },
   { 200,  206, 106.92491177765544 },
   { 200,  207, 106.9710961137502 },
   { 200,  208, 107.01628589789097 },
   { 200,  209, 107.06036768512644 },
   { 200,  600, 130.80799864560979 },
   { 200, 1199, 174.07110180722322 },
};
#define NB_MCGD_PANDAS ((int)(sizeof(mcgdPandas)/sizeof(mcgdPandas[0])))

/* The same pandas N = 14 run at bars 13, 16, ..., 43: TA_MCGD's first output
 * at unstable period 0, 3, ..., 30. */
static const double mcgdPandasUnst[] =
{
   104.56984870222738, 105.61524617556036, 106.35174279097379, 106.04376494572621,
   104.2891317744965,  102.15700745494874, 100.74247643742153, 99.85583798542304,
   99.09778830310381,  98.78396133220734,  99.42731243943032
};

/* trading-signals 8.3.0 McGinleyDynamic(10), whose hard-coded 0.6 makes it
 * TA_MCGD(6); first-bar seed, first value at bar 9. Captured as mcgdPandas. */
static const McgdGolden mcgdTradingSignals[] =
{
   { 6,    9, 106.15794731151539 },
   { 6,   10, 106.65393857724152 },
   { 6,   11, 107.10155708302268 },
   { 6,   12, 107.52856146326705 },
   { 6,   13, 107.95426102492418 },
   { 6,   14, 108.38207863509561 },
   { 6,   15, 108.79648279520656 },
   { 6,   16, 109.16335594746394 },
   { 6,   17, 109.43229071040822 },
   { 6,   18, 109.5404121718008 },
   { 6,   19, 109.41905098781562 },
   { 6,   50, 107.17052931612197 },
   { 6,  500, 181.6389202730609 },
   { 6, 1199, 284.66731065863354 },
};
#define NB_MCGD_TS ((int)(sizeof(mcgdTradingSignals)/sizeof(mcgdTradingSignals[0])))

/* QuantConnect LEAN McGinleyDynamic(14) through the lean_serve TA_MCGD arm
 * (LEAN 5b0c9975d, System.Decimal), captured as mcgdPandas. LEAN seeds with
 * the SMA, so it is 2.9e-2 apart at bar 13; every bar from 454 on is within
 * 1e-14. Only that tail is an oracle. */
static const McgdGolden mcgdLean[] =
{
   { 14,  454, 168.22473127503469 },
   { 14,  600, 188.07652156011272 },
   { 14,  999, 245.58749629044834 },
   { 14, 1199,  279.2010644790185 },
};
#define NB_MCGD_LEAN ((int)(sizeof(mcgdLean)/sizeof(mcgdLean[0])))

/* LEAN Tests/TestData/spy_with_McGinleyDynamic.csv at 5b0c9975d318: 500 SPY
 * daily closes, 2013-02-22 to 2015-02-17, and a McGinleyDynamic14 column that
 * is talipp output (LEAN's generate_reference_data_from_talipp.py). talipp
 * seeds with the SMA: 1.4e-3 apart at bar 13, within 1e-14 on every bar from
 * 387 on. Transcribed unchanged. */
static const double mcgdSpyClose[] =
{
   151.89, 149.00, 150.02, 151.91, 151.61, 152.11, 152.92, 154.29, 154.50, 154.78,
   155.44, 156.03, 155.68, 155.90, 156.73, 155.83, 154.97, 154.61, 155.69, 154.36,
   155.60, 154.95, 156.19, 156.19, 156.67, 156.05, 156.82, 155.23, 155.86, 155.16,
   156.21, 156.75, 158.67, 159.19, 158.80, 155.12, 157.41, 155.11, 154.14, 155.48,
   156.17, 157.78, 157.88, 158.52, 158.24, 159.30, 159.68, 158.28, 159.75, 161.37,
   161.78, 162.60, 163.34, 162.88, 163.41, 163.54, 165.23, 166.12, 165.34, 166.94,
   166.93, 167.17, 165.93, 165.45, 165.31, 166.30, 165.22, 165.83, 163.45, 164.35,
   163.56, 161.27, 162.73, 164.80, 164.80, 163.10, 161.75, 164.21, 163.18, 164.44,
   165.74, 163.45, 159.40, 159.07, 157.06, 158.57, 160.14, 161.08, 160.42, 161.36,
   161.21, 161.28, 163.02, 163.95, 165.13, 165.19, 167.44, 167.51, 168.15, 167.52,
   167.95, 168.87, 169.17, 169.50, 169.14, 168.52, 168.93, 169.11, 168.59, 168.59,
   168.71, 170.66, 170.95, 170.70, 169.73, 169.18, 169.80, 169.31, 169.11, 169.61,
   168.74, 166.38, 165.83, 164.77, 165.58, 164.56, 166.06, 166.62, 166.00, 163.33,
   163.91, 164.17, 163.65, 164.39, 165.75, 165.96, 166.04, 167.63, 168.87, 169.40,
   168.95, 169.33, 170.31, 171.07, 173.05, 172.76, 170.72, 169.93, 169.53, 169.04,
   169.69, 168.91, 168.01, 169.34, 169.18, 167.62, 168.89, 167.43, 165.48, 165.60,
   169.17, 170.26, 170.94, 169.70, 172.07, 173.22, 174.39, 174.40, 175.41, 174.57,
   175.15, 175.95, 176.23, 177.17, 176.29, 175.79, 176.21, 176.83, 176.27, 177.17,
   174.93, 177.29, 177.32, 176.96, 178.38, 179.27, 180.05, 179.42, 179.03, 178.47,
   179.91, 180.81, 180.63, 180.68, 181.12, 181.00, 180.53, 179.75, 179.73, 178.94,
   180.94, 181.40, 180.75, 178.72, 178.13, 178.11, 179.22, 178.65, 181.70, 181.49,
   181.56, 182.53, 182.93, 183.85, 183.85, 183.82, 184.69, 182.92, 182.88, 182.36,
   183.48, 183.52, 183.64, 184.14, 181.68, 183.67, 184.66, 184.42, 183.63, 184.18,
   184.30, 182.79, 178.89, 178.01, 179.07, 177.35, 179.23, 178.18, 174.17, 175.38,
   175.17, 177.48, 179.68, 180.01, 181.98, 182.07, 183.01, 184.02, 184.24, 183.02,
   184.10, 183.89, 184.91, 184.84, 184.85, 185.82, 186.29, 184.98, 187.58, 187.75,
   188.18, 188.26, 188.16, 187.23, 187.28, 185.18, 184.66, 186.33, 187.66, 186.66,
   187.75, 186.20, 185.43, 186.31, 184.97, 184.58, 185.49, 187.01, 188.25, 188.88,
   188.63, 186.40, 184.34, 185.10, 187.09, 183.15, 181.51, 182.94, 184.20, 186.12,
   186.39, 187.04, 187.89, 187.45, 187.83, 186.29, 186.88, 187.75, 188.31, 188.32,
   188.06, 188.42, 186.78, 187.88, 187.68, 187.96, 189.79, 189.96, 189.06, 187.40,
   188.05, 188.74, 187.55, 189.13, 189.59, 190.35, 191.52, 191.38, 192.37, 192.68,
   192.90, 192.80, 193.19, 194.45, 195.38, 195.58, 195.60, 194.92, 193.54, 194.13,
   194.29, 194.83, 196.26, 196.48, 195.94, 195.88, 194.70, 195.58, 195.44, 195.82,
   195.72, 197.03, 197.23, 198.20, 197.51, 196.24, 197.12, 196.34, 196.61, 197.60,
   197.23, 197.96, 195.71, 197.71, 197.34, 198.20, 198.64, 198.65, 197.72, 197.80,
   196.95, 196.98, 193.09, 192.50, 193.89, 192.01, 192.07, 191.03, 193.24, 193.79,
   193.53, 194.84, 195.76, 195.72, 197.36, 198.39, 198.92, 199.50, 199.19, 200.20,
   200.33, 200.25, 200.14, 200.71, 200.61, 200.50, 200.21, 201.11, 200.59, 199.32,
   200.07, 200.30, 199.13, 198.98, 200.48, 200.75, 201.82, 200.70, 199.15, 198.01,
   199.56, 196.34, 197.90, 197.54, 197.02, 194.35, 194.38, 196.52, 196.29, 193.26,
   196.64, 192.74, 190.54, 187.41, 187.70, 186.43, 186.27, 188.47, 190.30, 194.07,
   192.69, 194.93, 196.43, 196.16, 198.41, 198.11, 199.38, 201.66, 201.77, 201.07,
   202.34, 203.15, 203.34, 203.98, 204.18, 203.96, 204.19, 204.24, 204.37, 205.55,
   205.22, 205.58, 206.68, 207.26, 207.11, 207.64, 207.20, 205.76, 207.09, 207.89,
   207.66, 208.00, 206.61, 206.47, 203.16, 204.19, 200.89, 199.51, 197.91, 201.79,
   206.78, 206.52, 207.47, 207.75, 207.77, 208.44, 208.72, 207.60, 205.54, 205.43,
   201.72, 199.82, 202.31, 205.90, 204.25, 202.65, 202.08, 200.86, 199.02, 201.63,
   202.05, 203.08, 206.10, 204.97, 205.45, 202.74, 200.14, 201.99, 199.45, 201.92,
   204.84, 204.06, 206.12, 205.55, 204.63, 206.81, 206.93, 208.92, 209.78, 210.11,
};
#define NB_MCGD_SPY ((int)(sizeof(mcgdSpyClose)/sizeof(mcgdSpyClose[0])))

static const McgdGolden mcgdSpyTalipp[] =
{
   { 14, 387, 198.055375134293   },
   { 14, 450, 203.46274301678216 },
   { 14, 499, 204.97446646298158 },
};
#define NB_MCGD_SPY_ROWS ((int)(sizeof(mcgdSpyTalipp)/sizeof(mcgdSpyTalipp[0])))

static const int mcgdSweepPeriods[] = { 2, 3, 7, 14, 50, 200 };
static const int mcgdSweepUnst[]    = { 0, 1, 9, 40 };
static const int mcgdSweepStarts[]  = { 0, 1, 13, 14, 37, 250, 777, 1199 };
#define NB_MCGD_SWEEP_PERIODS ((int)(sizeof(mcgdSweepPeriods)/sizeof(int)))
#define NB_MCGD_SWEEP_UNST    ((int)(sizeof(mcgdSweepUnst)/sizeof(int)))
#define NB_MCGD_SWEEP_STARTS  ((int)(sizeof(mcgdSweepStarts)/sizeof(int)))

static int g_mcgdSweepCmp;
static int g_mcgdDegenCmp;

/**** Local functions declarations. ****/
static void mcgdSynth( double *c );
static int  mcgdBrute( const double *x, int startIdx, int endIdx, int period,
                       int unst, int seedShift, double *out, int *outBeg );
static ErrorNumber mcgdVerify( const char *tag, const double *x, int nb, int period,
                               TA_RetCode retCode, int begIdx, int nbElement,
                               const double *out );
static ErrorNumber mcgdCall( const char *tag, const double *x, int nb, int period,
                             double *out, int *begIdx, int *nbElement );
static ErrorNumber test_mcgd_goldens( void );
static ErrorNumber test_mcgd_book( void );
static ErrorNumber test_mcgd_sweep( void );
static ErrorNumber test_mcgd_lookback( void );
static ErrorNumber test_mcgd_degenerate( void );
static ErrorNumber test_mcgd_range( const TA_Real *in );

/**** Global functions definitions. ****/
ErrorNumber test_func_mcgd( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   g_mcgdSweepCmp = g_mcgdDegenCmp = 0;

   err = test_mcgd_goldens();
   if( err == TA_TEST_PASS )
      err = test_mcgd_book();
   if( err == TA_TEST_PASS )
      err = test_mcgd_sweep();
   if( err == TA_TEST_PASS )
      err = test_mcgd_lookback();
   if( err == TA_TEST_PASS )
      err = test_mcgd_degenerate();

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS )
      err = test_mcgd_range( history->close );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* Literal counts: every input is synthesized, so each leg is deterministic. */
   if( err == TA_TEST_PASS
       && ( g_mcgdSweepCmp != MCGD_SWEEP_CMP || g_mcgdDegenCmp != MCGD_DEGEN_CMP ) )
   {
      printf( "MCGD Fail: coverage counters (sweep %d, degenerate %d) "
              "are not what this file was written with (%d, %d)\n",
              g_mcgdSweepCmp, g_mcgdDegenCmp, MCGD_SWEEP_CMP, MCGD_DEGEN_CMP );
      return TA_MCGD_VACUOUS;
   }

   return err;
}

/**** Local functions definitions. ****/
static void mcgdSynth( double *c )
{
   int i;

   for( i = 0; i < MCGD_NB; i++ )
      c[i] = 100.0 + 10.0*sin(i/7.0) + 0.15*i + 2.0*sin(i/2.3);
}

/* The formula as the issue writes it, one bar at a time. seedShift moves the
 * seed bar, so the sweep can show that it sees a seed one bar off. */
static int mcgdBrute( const double *x, int startIdx, int endIdx, int period,
                      int unst, int seedShift, double *out, int *outBeg )
{
   int lookback = period - 1 + unst;
   int seedBar, i, n = 0;
   double md, r, next;

   if( startIdx < lookback )
      startIdx = lookback;
   *outBeg = startIdx;
   if( startIdx > endIdx )
      return 0;

   seedBar = startIdx - lookback + seedShift;
   md = x[seedBar];
   for( i = seedBar + 1; i <= endIdx; i++ )
   {
      r = x[i] / md;
      next = md + (x[i] - md) / ((double)period * ((r*r)*(r*r)));
      if( isfinite( next ) )
         md = next;
      if( i >= startIdx )
         out[n++] = md;
   }
   return n;
}

static ErrorNumber mcgdVerify( const char *tag, const double *x, int nb, int period,
                               TA_RetCode retCode, int begIdx, int nbElement,
                               const double *out )
{
   double optIn[1];
   ErrorNumber e;
   int cmpBefore;

   if( !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)period;
   cmpBefore = server_verify_comparisons();
   e = server_verify( "MCGD", 0, nb-1, nb, retCode, begIdx, nbElement,
                      (const TA_Real*[]){ x, NULL }, optIn, 1,
                      (const TA_Real*[]){ out, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "MCGD %s [N=%d]: compared no server despite live pipes\n", tag, period );
      return TA_SV_ROUTED_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* Whole-series call at unstable period 0, checked for its range and routed to
 * the language servers. */
static ErrorNumber mcgdCall( const char *tag, const double *x, int nb, int period,
                             double *out, int *begIdx, int *nbElement )
{
   TA_RetCode retCode;

   retCode = TA_MCGD( 0, nb-1, x, period, begIdx, nbElement, out );
   if( retCode != TA_SUCCESS || *begIdx != period-1 || *nbElement != nb-period+1 )
   {
      printf( "MCGD %s Fail [N=%d]: rc=%d (%d,%d) expected (%d,%d)\n", tag, period,
              (int)retCode, *begIdx, *nbElement, period-1, nb-period+1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   return mcgdVerify( tag, x, nb, period, retCode, *begIdx, *nbElement, out );
}

static ErrorNumber mcgdCheckRows( const char *tag, const double *c, int nb,
                                  const McgdGolden *rows, int nbRows )
{
   static double out[MCGD_NB];
   TA_Integer begIdx = 0, nbElement = 0;
   ErrorNumber e;
   int k, period = -1;
   double err;
   const char *mode;

   for( k = 0; k < nbRows; k++ )
   {
      int idx;

      if( rows[k].period != period )
      {
         period = rows[k].period;
         e = mcgdCall( tag, c, nb, period, out, &begIdx, &nbElement );
         if( e != TA_TEST_PASS )
            return e;
      }

      idx = rows[k].bar - begIdx;
      if( idx < 0 || idx >= nbElement )
      {
         printf( "MCGD %s Fail [N=%d]: bar %d outside the output\n",
                 tag, period, rows[k].bar );
         return TA_MCGD_VACUOUS;
      }
      if( !checkOracleValue( out[idx], rows[k].want, MCGD_GOLD_REL, 0.0,
                             &err, &mode ) )
      {
         printf( "MCGD %s Fail [N=%d] at bar %d: got %.17g expected %.17g "
                 "(%s %.3e)\n", tag, period, rows[k].bar, out[idx],
                 rows[k].want, mode, err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (1) and (2) */
static ErrorNumber test_mcgd_goldens( void )
{
   static double c[MCGD_NB];
   ErrorNumber e;

   mcgdSynth( c );

   e = mcgdCheckRows( "golden", c, MCGD_NB, mcgdGolden, NB_MCGD_GOLDEN );
   if( e == TA_TEST_PASS )
      e = mcgdCheckRows( "pandas-ta-classic", c, MCGD_NB, mcgdPandas, NB_MCGD_PANDAS );
   if( e == TA_TEST_PASS )
      e = mcgdCheckRows( "trading-signals", c, MCGD_NB, mcgdTradingSignals, NB_MCGD_TS );
   if( e == TA_TEST_PASS )
      e = mcgdCheckRows( "LEAN", c, MCGD_NB, mcgdLean, NB_MCGD_LEAN );
   if( e == TA_TEST_PASS )
      e = mcgdCheckRows( "talipp SPY", mcgdSpyClose, NB_MCGD_SPY,
                         mcgdSpyTalipp, NB_MCGD_SPY_ROWS );
   return e;
}

/* (1) McGinley, MTA Journal 1997, p. 17: N = 7, a Dynamic of 10, a close of 14
 * gives a step of "0.15" (2500/16807 at exponent 4, 12500/117649 at 5), so
 * 170570/16807. The close of 6 gives 3170/567, 1 ulp below TA_MCGD's double;
 * the printed -6.66 matches no integer exponent. */
static ErrorNumber test_mcgd_book( void )
{
   static const struct { double close; double want; } book[] =
   {
      { 14.0, 170570.0/16807.0 },
      {  6.0,   3170.0/567.0   },
   };
   double x[8], out[8], err;
   const char *mode;
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   int k, i;

   for( k = 0; k < 2; k++ )
   {
      for( i = 0; i < 7; i++ )
         x[i] = 10.0;
      x[7] = book[k].close;

      e = mcgdCall( "book", x, 8, 7, out, &begIdx, &nbElement );
      if( e != TA_TEST_PASS )
         return e;
      if( out[0] != 10.0
          || !checkOracleValue( out[1], book[k].want, MCGD_BOOK_REL, 0.0, &err, &mode ) )
      {
         printf( "MCGD book Fail [close %g]: got %.17g, %.17g expected 10, %.17g\n",
                 book[k].close, out[0], out[1], book[k].want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/* (3) Every (period, unstable period, startIdx) against mcgdBrute, bit for
 * bit, to the end of the series and to a short endIdx. */
static ErrorNumber test_mcgd_sweep( void )
{
   static double c[MCGD_NB], out[MCGD_NB], want[MCGD_NB];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int p, u, s, t, i, n, wantBeg, seenShift = 0;

   mcgdSynth( c );

   for( p = 0; p < NB_MCGD_SWEEP_PERIODS; p++ )
   for( u = 0; u < NB_MCGD_SWEEP_UNST; u++ )
   {
      int period = mcgdSweepPeriods[p];
      int unst   = mcgdSweepUnst[u];

      TA_SetUnstablePeriod( TA_FUNC_UNST_MCGD, (unsigned int)unst );

      for( s = 0; s < NB_MCGD_SWEEP_STARTS; s++ )
      for( t = 0; t < 2; t++ )
      {
         int startIdx = mcgdSweepStarts[s];
         int endIdx   = t == 0 ? MCGD_NB-1 : startIdx + 13;

         if( endIdx >= MCGD_NB )
            endIdx = MCGD_NB-1;

         n = mcgdBrute( c, startIdx, endIdx, period, unst, 0, want, &wantBeg );
         retCode = TA_MCGD( startIdx, endIdx, c, period, &begIdx, &nbElement, out );
         if( retCode != TA_SUCCESS || nbElement != n || (n > 0 && begIdx != wantBeg) )
         {
            printf( "MCGD sweep Fail [N=%d unst=%d %d..%d]: rc=%d (%d,%d) expected (%d,%d)\n",
                    period, unst, startIdx, endIdx, (int)retCode, begIdx, nbElement,
                    wantBeg, n );
            return TA_TESTUTIL_TFRR_BAD_BEGIDX;
         }
         for( i = 0; i < n; i++ )
         {
            if( memcmp( &out[i], &want[i], sizeof(double) ) != 0 )
            {
               printf( "MCGD sweep Fail [N=%d unst=%d %d..%d] at bar %d: got %.17g "
                       "expected %.17g\n", period, unst, startIdx, endIdx,
                       begIdx+i, out[i], want[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_mcgdSweepCmp++;
         }

         /* A seed one bar late must be visible at the first output. */
         if( period == 14 && unst == 0 && startIdx == 13 && t == 0 )
         {
            double shifted[MCGD_NB];
            int sb;
            mcgdBrute( c, startIdx, endIdx, period, unst, 1, shifted, &sb );
            if( shifted[0] == out[0] )
            {
               printf( "MCGD sweep Fail: a seed one bar late is invisible\n" );
               return TA_MCGD_VACUOUS;
            }
            seenShift = 1;
         }
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_MCGD, 0 );

   if( !seenShift )
   {
      printf( "MCGD sweep Fail: the seed-shift check never ran\n" );
      return TA_MCGD_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* (4) The unstable period moves the lookback and the first output bar, and
 * only MCGD's own id does. */
static ErrorNumber test_mcgd_lookback( void )
{
   static const unsigned int others[] = { TA_FUNC_UNST_RMA, TA_FUNC_UNST_EMA };
   static double c[MCGD_NB], out[MCGD_NB];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int k, j;

   mcgdSynth( c );

   for( k = 0; k <= 30; k += 3 )
   {
      double err;
      const char *mode;

      TA_SetUnstablePeriod( TA_FUNC_UNST_MCGD, (unsigned int)k );
      retCode = TA_MCGD( 0, MCGD_NB-1, c, 14, &begIdx, &nbElement, out );
      if( TA_MCGD_Lookback( 14 ) != 13 + k || TA_MCGD_Lookback( 2 ) != 1 + k
          || retCode != TA_SUCCESS || begIdx != 13 + k )
      {
         printf( "MCGD lookback Fail [unst=%d]: lookback %d, outBegIdx %d, expected %d\n",
                 k, TA_MCGD_Lookback( 14 ), begIdx, 13 + k );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      /* The unstable steps run on the bars pandas-ta-classic runs them on. */
      if( !checkOracleValue( out[0], mcgdPandasUnst[k/3], MCGD_GOLD_REL, 0.0,
                             &err, &mode ) )
      {
         printf( "MCGD lookback Fail [unst=%d]: first output %.17g, pandas-ta-classic "
                 "%.17g at bar %d\n", k, out[0], mcgdPandasUnst[k/3], 13 + k );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_MCGD, 0 );

   for( j = 0; j < 2; j++ )
   {
      TA_SetUnstablePeriod( (TA_FuncUnstId)others[j], 17 );
      if( TA_MCGD_Lookback( 14 ) != 13 )
      {
         printf( "MCGD lookback Fail: unstable id %u moves MCGD's lookback\n", others[j] );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      TA_SetUnstablePeriod( (TA_FuncUnstId)others[j], 0 );
   }

   if( TA_MCGD_Lookback( 1 ) != -1 || TA_MCGD_Lookback( 100001 ) != -1 )
   {
      printf( "MCGD lookback Fail: an out-of-range period is not rejected\n" );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   return TA_TEST_PASS;
}

static int mcgdAllFinite( const double *out, int n )
{
   int i;
   for( i = 0; i < n; i++ )
      if( !isfinite( out[i] ) )
         return 0;
   return 1;
}

/* (5) */
static ErrorNumber test_mcgd_degenerate( void )
{
   static double c[MCGD_NB], x[MCGD_NB], out[MCGD_NB];
   TA_Integer begIdx, nbElement;
   ErrorNumber e;
   double err;
   const char *mode;
   int k, i;

   mcgdSynth( c );

   /* A zero price after a non-zero line: r = 0, the step is -inf, held. */
   memcpy( x, c, sizeof(x) );
   x[150] = 0.0;
   e = mcgdCall( "zero price", x, 300, 14, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   if( !mcgdAllFinite( out, nbElement ) || out[150-13] != out[149-13]
       || out[151-13] == out[150-13] )
   {
      printf( "MCGD zero price Fail: %.17g %.17g %.17g\n",
              out[149-13], out[150-13], out[151-13] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_mcgdDegenCmp++;

   /* The same zero price before the first output, in the warm-up loop. */
   memcpy( x, c, sizeof(x) );
   x[5] = 0.0;
   e = mcgdCall( "zero price in seed", x, 300, 14, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   {
      static double want[MCGD_NB];
      int wantBeg, n = mcgdBrute( x, 0, 299, 14, 0, 0, want, &wantBeg );
      if( n != nbElement || !mcgdAllFinite( out, nbElement )
          || memcmp( out, want, (size_t)n * sizeof(double) ) != 0 )
      {
         printf( "MCGD zero price in seed Fail\n" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   g_mcgdDegenCmp++;

   /* A zero first bar is a zero seed: r = inf, every step is 0. Then a zero
    * second bar too: r = 0/0, held. */
   for( k = 1; k <= 2; k++ )
   {
      memcpy( x, c, sizeof(x) );
      for( i = 0; i < k; i++ )
         x[i] = 0.0;
      e = mcgdCall( k == 1 ? "zero seed" : "zero seed and price", x, 100, 14,
                    out, &begIdx, &nbElement );
      if( e != TA_TEST_PASS )
         return e;
      for( i = 0; i < nbElement; i++ )
      {
         if( out[i] != 0.0 )
         {
            printf( "MCGD zero seed Fail [%d zero bars] at bar %d: %.17g\n",
                    k, begIdx+i, out[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_mcgdDegenCmp++;
      }
   }

   /* A flat 100 and one bar at 50, N = 8: 1 + (0.5-1)/(8*0.5^4) is exactly 0.
    * The zero price at bar 10 then meets the zero line: r = 0/0, held. */
   for( i = 0; i < 29; i++ )
      x[i] = i == 8 ? 50.0 : (i == 10 ? 0.0 : 100.0);
   e = mcgdCall( "flat to zero", x, 29, 8, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   for( i = 0; i < nbElement; i++ )
   {
      if( out[i] != (begIdx+i < 8 ? 100.0 : 0.0) )
      {
         printf( "MCGD flat to zero Fail at bar %d: %.17g\n", begIdx+i, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_mcgdDegenCmp++;
   }

   /* Past LEAN's range: at r = 8.8e6, N*r^4 overflows System.Decimal and LEAN
    * refuses the call. The 60-digit value is 1 + 1.05e-22. */
   for( i = 0; i < 14; i++ )
      x[i] = 1.0;
   x[14] = 8.8e6;
   e = mcgdCall( "large ratio", x, 15, 14, out, &begIdx, &nbElement );
   if( e != TA_TEST_PASS )
      return e;
   if( out[1] != 1.0 )
   {
      printf( "MCGD large ratio Fail: %.17g expected 1\n", out[1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_mcgdDegenCmp++;

   /* A positive series that takes the line to or below 0. */
   for( k = 0; k < NB_MCGD_CANCEL; k++ )
   {
      if( k == 0 || mcgdCancel[k].bar121 != mcgdCancel[k-1].bar121 )
      {
         memcpy( x, c, sizeof(x) );
         x[121] = mcgdCancel[k].bar121;
         e = mcgdCall( "to zero", x, 300, 14, out, &begIdx, &nbElement );
         if( e != TA_TEST_PASS )
            return e;
      }
      if( !checkOracleValue( out[mcgdCancel[k].bar - begIdx], mcgdCancel[k].want,
                             MCGD_CANCEL_REL, 0.0, &err, &mode ) )
      {
         printf( "MCGD to zero Fail [bar121 %.17g] at bar %d: got %.17g expected "
                 "%.17g (%s %.3e)\n", mcgdCancel[k].bar121, mcgdCancel[k].bar,
                 out[mcgdCancel[k].bar - begIdx], mcgdCancel[k].want, mode, err );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_mcgdDegenCmp++;
   }

   /* Values beyond any meaning, which must still be finite: bar 121 at 0.30 of
    * the line, and a series crossing zero. */
   for( k = 0; k < 2; k++ )
   {
      memcpy( x, c, sizeof(x) );
      if( k == 0 )
         x[121] = 33.91497455566595;
      else
      {
         x[40] = -1.0;
         for( i = 41; i < 300; i++ )
            x[i] = 5.0;
      }
      e = mcgdCall( k == 0 ? "blow-up" : "sign flip", x, 300, 14, out,
                    &begIdx, &nbElement );
      if( e != TA_TEST_PASS )
         return e;
      if( !mcgdAllFinite( out, nbElement ) )
      {
         printf( "MCGD %s Fail: a non-finite output\n", k == 0 ? "blow-up" : "sign flip" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_mcgdDegenCmp++;
   }

   return TA_TEST_PASS;
}

/* (6) An IIR recursion: TA_STABLE_CONVERGING with its own id. */
typedef struct { int period; const TA_Real *in; } McgdRangeParam;

static TA_RetCode mcgdRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   McgdRangeParam *p = (McgdRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_MCGD_Lookback( p->period );
   return TA_MCGD( startIdx, endIdx, p->in, p->period,
                   outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_mcgd_range( const TA_Real *in )
{
   McgdRangeParam param;

   param.period = 14;
   param.in     = in;

   return doRangeTestEx( mcgdRangeTestFunction,
                         TA_STABLE_CONVERGING, TA_FUNC_UNST_MCGD,
                         (void *)&param, 1, 0 );
}
