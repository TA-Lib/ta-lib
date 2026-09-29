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
 *  092926 MF,CC  First version (#465).
 */

/* Description:
 *
 *   Test TA_EMV (Arms Ease of Movement).
 *
 *   The value leg lives elsewhere: test_composite1.c rebuilds EMV from
 *   MEDPRICE, MOM, DIV, SUB and SMA through the public API and compares with
 *   memcmp, over a grid of periods, start indices and divisors. What is here
 *   is everything that rebuild cannot see.
 *
 *   Legs:
 *     0. The published tables: Achelis p.132 (through Tulip's atoz.txt) and
 *        LEAN's spy_emv.txt. The only legs anchored outside this library.
 *     1. The lookback tier. The divisor scales the output and must not move
 *        the first valid bar, so the lookback is the period for every divisor,
 *        and the reported start follows the caller's startIdx once it is past
 *        the warm-up. Routed to the language servers.
 *     2. Guarded bars -- zero volume or zero range -- which the composite leg
 *        cannot reach, because its corpus is real history where no bar has
 *        either. The framework's all-zero sweep does reach the guard, but only
 *        with EVERY bar degenerate, so it cannot see a mixed series.
 *     2b. A non-zero volume whose scaled box ratio underflows to zero.
 *     3. Aliasing of the output over each of the three inputs.
 *     4. The generic start/end range sweep.
 *     5. The single-precision entry point, bitwise, over a period grid that
 *        includes the heap CIRCBUF path.
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

static const int    emvPeriod[] = { 1, 2, 14, 50, 51, 251 };
static const double emvDiv[]    = { 1.0, 10000.0, 1.0e8 };

#define NB_EMV_PERIOD (int)(sizeof(emvPeriod)/sizeof(emvPeriod[0]))
#define NB_EMV_DIV    (int)(sizeof(emvDiv)/sizeof(emvDiv[0]))

typedef struct
{
   int           period;
   double        divisor;
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *volume;
} EmvRangeParam;

static ErrorNumber test_emv_golden    ( void );
static ErrorNumber test_emv_lookback  ( const TA_History *history );
static ErrorNumber test_emv_degenerate( void );
static ErrorNumber test_emv_underflow ( void );
static ErrorNumber test_emv_aliasing  ( const TA_History *history );
static ErrorNumber test_emv_range     ( const TA_History *history );
static ErrorNumber test_emv_single    ( const TA_History *history );

/**** Global functions definitions.   ****/
ErrorNumber test_func_emv( TA_History *history )
{
   ErrorNumber retValue;

   /* EMV smooths with a finite window, so it has no unstable period. A
    * leftover global setting from an earlier group must not reach it. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   retValue = test_emv_golden();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_emv_lookback( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_emv_degenerate();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_emv_underflow();
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_emv_aliasing( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_emv_range( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_emv_single( history );
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* (0) The published tables.
 *
 * Every other leg here is comparative -- the composite rebuild, the language
 * servers, the single-precision tier -- and none of them can see an error that
 * the formula itself carries, because they all evaluate the same formula. This
 * leg is the only one anchored outside the library.
 *
 * Two independent tables, both at period 1 and D = 10,000.
 *
 * ACHELIS. Steven B. Achelis, Technical Analysis from A to Z, 2nd ed., p. 132:
 * eighteen bars and the seventeen EMV values they produce. Transcribed from
 * Tulip Indicators 0.9.2 `tests/atoz.txt` (`emv`, `#page 132`), which cites
 * the printed page. Four decimals, so the band is a half-unit of the last one:
 * 5e-5. Measured worst here is 4.7e-5 -- the band is the table's precision,
 * not slack.
 *
 * LEAN. QuantConnect/Lean `Tests/TestData/spy_emv.txt`: thirty SPY bars and
 * twenty-nine values at nine decimals, band 5e-10, measured worst 4.7e-10.
 * The file states no provenance, and LEAN's own assertion on it is absolute
 * 1.0 against values of at most 3.8e-4, so LEAN does not itself test what is
 * tested here.
 *
 * Neither band is vacuous, and the two fail differently:
 *   - Achelis's ONLINE entry states the range "in eighths with the denominator
 *     dropped". That reading misses his own table by 6.1e-1, four orders past
 *     the band. Points, not eighths.
 *   - On the LEAN table, D = 1e8 (the StockCharts and TradingView-help
 *     constant) misses by 3.8, and measuring the change from the previous
 *     CLOSE rather than the previous midpoint misses by 1.8e-4.
 */
#define EMV_ATOZ_N   18
#define EMV_ATOZ_TOL 5e-5
#define EMV_SPY_N    30
#define EMV_SPY_TOL  5e-10

static const TA_Real atozHigh[EMV_ATOZ_N] = {
   23.75,   23.75,   23.75,   25.0625, 26.25,   26.875,  27.0,    26.875,
   28.0,    28.1875, 27.6875, 27.1875, 26.25,   26.5,    26.0,    25.875,
   25.375,  25.5 };

static const TA_Real atozLow[EMV_ATOZ_N] = {
   23.0,    23.1875, 23.25,   23.5,    25.0,    25.8125, 25.875,  25.75,
   26.3125, 27.375,  27.125,  26.0,    25.1875, 25.375,  24.875,  24.3125,
   24.25,   24.75 };

static const TA_Real atozVolume[EMV_ATOZ_N] = {
   125733.0, 83819.0,  111390.0, 211366.0, 240664.0, 219933.0, 155943.0,
   138913.0, 226220.0, 164528.0, 132053.0, 109900.0, 138313.0, 143421.0,
   106053.0, 141425.0, 96921.0,  93208.0 };

static const TA_Real atozEmv[EMV_ATOZ_N-1] = {
   0.0063,  0.0014,  0.0578,  0.0698,  0.0347,  0.0068, -0.0101,  0.0629,
   0.0309, -0.0160, -0.0878, -0.0672,  0.0172, -0.0530, -0.0380, -0.0326,
   0.0251 };

static const TA_Real spyHigh[EMV_SPY_N] = {
   63.74, 64.51, 64.57, 64.31, 63.43, 62.85,
   62.7, 63.18, 62.47, 64.16, 64.38, 64.89,
   65.25, 64.69, 64.26, 64.51, 63.46, 62.69,
   63.52, 63.52, 63.74, 64.58, 65.31, 65.1,
   63.68, 63.66, 62.95, 63.73, 64.99, 65.31 };

static const TA_Real spyLow[EMV_SPY_N] = {
   62.63, 63.85, 63.81, 62.62, 62.73, 61.95,
   62.06, 62.69, 61.54, 63.21, 63.87, 64.29,
   64.48, 63.65, 63.68, 63.12, 62.5, 61.86,
   62.56, 62.95, 62.63, 63.39, 64.72, 64.21,
   62.51, 62.55, 62.15, 62.97, 63.64, 64.6 };

static const TA_Real spyVolume[EMV_SPY_N] = {
   32178836.0, 36461672.0, 51372680.0, 42476356.0, 29504176.0,
   33098600.0, 30577960.0, 35693928.0, 49768136.0, 44759968.0,
   33425504.0, 15895085.0, 37015388.0, 40672116.0, 35627200.0,
   47337336.0, 43373576.0, 57651752.0, 32357184.0, 27620876.0,
   42467704.0, 44460240.0, 52992592.0, 40561552.0, 48636228.0,
   57230032.0, 46260856.0, 41926492.0, 42620976.0, 37809176.0 };

static const TA_Real spyEmv[EMV_SPY_N-1] = {
   0.000180107, 0.000001479, -0.000288455, -0.000091343,
   -0.000184902, -0.000004186, 0.000076189, -0.000173786,
   0.000356569, 0.000067134, 0.000175526, 0.000057206,
   -0.000177714, -0.000032559, -0.000045514, -0.000184813,
   -0.000101497, 0.000226967, 0.000040241, -0.000013069,
   0.000214124, 0.000114676, -0.000078991, -0.000375276,
   0.000001940, -0.000095977, 0.000145016, 0.000305659,
   0.000120182 };

static ErrorNumber emv_check_table( const char *name,
                                    const TA_Real *high, const TA_Real *low,
                                    const TA_Real *volume, const TA_Real *want,
                                    int nbBars, double tol, TA_Real *out )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   rc = TA_EMV( 0, nbBars-1, high, low, volume, 1, 10000.0,
                &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS )
   {
      printf( "EMV golden Fail [%s]: retCode = %d\n", name, (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != 1 || nbElement != nbBars-1 )
   {
      printf( "EMV golden Fail [%s]: shape (%d,%d), expected (1,%d)\n",
              name, (int)begIdx, (int)nbElement, nbBars-1 );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   for( i = 0; i < nbBars-1; i++ )
   {
      double err = out[i] - want[i];

      if( err < 0.0 )
         err = -err;
      /* NaN fails every comparison, so test the bound the way round that
       * rejects it rather than the way that waves it through. */
      if( !( err <= tol ) )
      {
         printf( "EMV golden Fail [%s] at bar %d: got %.17g, the table prints "
                 "%.9g (abs %.3e > %.1e)\n",
                 name, i+1, out[i], want[i], err, tol );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_emv_golden( void )
{
   static TA_Real out[EMV_SPY_N > EMV_ATOZ_N ? EMV_SPY_N : EMV_ATOZ_N];
   ErrorNumber e;

   e = emv_check_table( "Achelis p.132", atozHigh, atozLow, atozVolume,
                        atozEmv, EMV_ATOZ_N, EMV_ATOZ_TOL, out );
   if( e != TA_TEST_PASS )
      return e;

   return emv_check_table( "LEAN spy_emv.txt", spyHigh, spyLow, spyVolume,
                           spyEmv, EMV_SPY_N, EMV_SPY_TOL, out );
}

/* (1) The lookback tier.
 *
 * One bar is consumed forming the first midpoint change, then the window's own
 * warm-up on top: 1 + (period-1). The divisor only scales, so the same period
 * must give the same lookback under every divisor -- a divisor that reached
 * the lookback would silently move the first valid bar with the units.
 *
 * The shape is checked from three starts per period: before the warm-up, where
 * the function must clamp to it; exactly at it; and past it, where the caller's
 * startIdx wins.
 */
static ErrorNumber test_emv_lookback( const TA_History *history )
{
   static TA_Real out[OUT_CAP];
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement;
   int p, d, k, nb;

   nb = (int)history->nbBars;

   for( p = 0; p < NB_EMV_PERIOD; p++ )
   {
      int period = emvPeriod[p];
      int lb0 = TA_EMV_Lookback( period, emvDiv[0] );

      if( lb0 != period )
      {
         printf( "EMV lookback Fail (period %d): lookback %d, expected %d\n",
                 period, lb0, period );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( d = 0; d < NB_EMV_DIV; d++ )
      {
         if( TA_EMV_Lookback( period, emvDiv[d] ) != lb0 )
         {
            printf( "EMV lookback Fail (period %d): divisor %g moved the "
                    "lookback to %d from %d\n", period, emvDiv[d],
                    TA_EMV_Lookback( period, emvDiv[d] ), lb0 );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }

         if( period >= nb )
            continue;

         for( k = 0; k < 3; k++ )
         {
            int startIdx = ( k == 0 ) ? 0 : ( k == 1 ) ? period : period + 5;
            int wantBeg  = ( startIdx < period ) ? period : startIdx;

            if( wantBeg > nb - 1 )
               continue;

            retCode = TA_EMV( startIdx, nb - 1,
                              history->high, history->low, history->volume,
                              period, emvDiv[d], &begIdx, &nbElement, out );
            if( retCode != TA_SUCCESS )
            {
               printf( "EMV lookback Fail (period %d div %g start %d): "
                       "retCode = %d\n", period, emvDiv[d], startIdx,
                       (int)retCode );
               return TA_TESTUTIL_TFRR_BAD_RETCODE;
            }
            if( begIdx != wantBeg || nbElement != nb - wantBeg )
            {
               printf( "EMV lookback Fail (period %d div %g start %d): "
                       "shape (%d,%d), expected (%d,%d)\n",
                       period, emvDiv[d], startIdx, (int)begIdx,
                       (int)nbElement, wantBeg, nb - wantBeg );
               return TA_TESTUTIL_TFRR_BAD_BEGIDX;
            }

            /* Cross-language: EMV must be bit-identical on every server. */
            if( server_verify_active() )
            {
               double optIn[2];
               ErrorNumber e;

               optIn[0] = (double)period;
               optIn[1] = emvDiv[d];
               e = server_verify( "EMV", startIdx, nb - 1, history->nbBars,
                                  retCode, begIdx, nbElement,
                                  (const TA_Real*[]){ history->high,
                                                      history->low,
                                                      history->volume, NULL },
                                  optIn, 2,
                                  (const TA_Real*[]){ out, NULL }, NULL );
               if( e != TA_TEST_PASS )
                  return e;
            }
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) Guarded bars.
 *
 * Three properties, all of them decisions this implementation had to make:
 *
 *   1. A guarded bar contributes exactly zero, not the midpoint change it
 *      would have had. Bar 7 below is one, and the change there is -6.
 *   2. The midpoint advances THROUGH a guarded bar. The bar after one is
 *      measured from the guarded bar's own midpoint, never from the last bar
 *      that produced a value.
 *   3. Zeros summed from a standing start average to +0.0.
 *
 * At period 1 the output is the raw kernel bit for bit, so 1 and 2 are checked
 * with memcmp against arithmetic written out here in the same operand order.
 *
 * What this does NOT check, because nothing downstream can: the SIGN of the
 * zero a guarded bar writes. Replacing the literal with a product against the
 * midpoint change gives -0.0 on every falling guarded bar, and this leg stays
 * green -- the sum starts at +0.0 and 0.0 + (-0.0) is +0.0, so the sign is
 * gone before the first division. The zero's sign is unobservable here rather
 * than verified.
 */
#define EMV_DEGEN_N   40
#define EMV_DEGEN_CMP 80

static const double emvDegenDiv[] = { 1.0, 10000.0 };

static int g_emvDegenCmp;

static ErrorNumber test_emv_degenerate( void )
{
   static TA_Real high[EMV_DEGEN_N], low[EMV_DEGEN_N], volume[EMV_DEGEN_N];
   static TA_Real out[EMV_DEGEN_N];
   const double zero = 0.0;
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, d, t;

   for( i = 0; i < EMV_DEGEN_N; i++ )
   {
      /* Every value is a small integer, so the midpoint and the range are
       * exact and the only rounding left is the division under test. The
       * modulus makes the midpoint fall as well as rise. */
      double base = 100.0 + (double)(i % 7);
      high[i]   = base + 2.0;
      low[i]    = base - 2.0;
      volume[i] = 1000.0 + (double)(i % 5);
   }

   /* Bar 7 is where the midpoint falls by 6 -- a guarded bar there is the one
    * that exposes a midpoint change leaking through the guard. */
   volume[7]  = 0.0;
   volume[10] = 0.0;
   volume[11] = 0.0;
   volume[20] = 0.0;
   /* Zero range, not zero volume: the other half of the guard. */
   high[15] = low[15];
   /* A run long enough for one whole period-14 window to be degenerate. */
   for( i = 24; i <= 37; i++ )
      volume[i] = 0.0;

   g_emvDegenCmp = 0;

   for( d = 0; d < (int)(sizeof(emvDegenDiv)/sizeof(emvDegenDiv[0])); d++ )
   {
      double D = emvDegenDiv[d];

      rc = TA_EMV( 0, EMV_DEGEN_N-1, high, low, volume, 1, D,
                   &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS || begIdx != 1 || nbElement != EMV_DEGEN_N-1 )
      {
         printf( "EMV degenerate Fail [D %g]: period 1 rc=%d beg=%d nb=%d\n",
                 D, (int)rc, (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( t = 1; t < EMV_DEGEN_N; t++ )
      {
         double mid     = (high[t] + low[t]) / 2.0;
         double prevMid = (high[t-1] + low[t-1]) / 2.0;
         double range   = high[t] - low[t];
         double want;

         if( volume[t] != 0.0 && range != 0.0 )
            want = (mid - prevMid) / ((volume[t] / D) / range);
         else
            want = 0.0;

         g_emvDegenCmp++;
         if( memcmp( &out[t-1], &want, sizeof(double) ) != 0 )
         {
            printf( "EMV degenerate Fail [D %g] bar %d: got %.17g want %.17g "
                    "(must be BIT-exact)\n", D, t, out[t-1], want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      /* Bars 24..37 are all guarded, and asking for bar 37 alone starts the
       * running sum at bar 24, so nothing but zeros is ever added to it.
       *
       * The start matters. Asking for the same bar from index 0 does NOT give
       * an exact zero: the sum has carried every earlier bar through an add
       * and a matching subtract, and those do not cancel to the last bit. That
       * residue is sma.c's, not this function's -- the composite leg in
       * test_composite1.c pins EMV to TA_SMA bit for bit, so a window that
       * re-zeroed itself here would be the thing out of step. What is being
       * checked is narrower and true: zeros summed from a standing start
       * average to +0.0.
       */
      rc = TA_EMV( 37, EMV_DEGEN_N-1, high, low, volume, 14, D,
                   &begIdx, &nbElement, out );
      if( rc != TA_SUCCESS || begIdx != 37 )
      {
         printf( "EMV degenerate Fail [D %g]: period 14 rc=%d beg=%d\n",
                 D, (int)rc, (int)begIdx );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      g_emvDegenCmp++;
      if( memcmp( &out[0], &zero, sizeof(double) ) != 0 )
      {
         printf( "EMV degenerate Fail [D %g]: a wholly degenerate window gave "
                 "%.17g, not +0.0\n", D, out[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   /* Literal: a leg that compared nothing prints nothing either. */
   if( g_emvDegenCmp != EMV_DEGEN_CMP )
   {
      printf( "EMV degenerate Fail: compared %d times, not the %d this file "
              "was written with\n", g_emvDegenCmp, EMV_DEGEN_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/* (2b) The box the division actually uses, not just its operands.
 *
 * `optInVolumeDivisor` runs to TA_REAL_MAX, so the scaling can reach zero from
 * a volume that is not zero. At inVolume 5e-324 -- the smallest positive
 * double -- and a divisor of 2, both inside their declared ranges,
 * inVolume/optInVolumeDivisor rounds to 0.0 and the box ratio with it. A guard
 * that tests only the operands passes this bar through and the function
 * returns an infinity; measured here before the guard was moved onto the box.
 *
 * The generator's own divisor_guard_suite is what named this: it reports a
 * divisor "SCALED from a value the guard does test, which scaling can underflow
 * to 0.0 independently". Its verdict is what this leg pins.
 */
static ErrorNumber test_emv_underflow( void )
{
   static TA_Real high[4], low[4], volume[4];
   static TA_Real out[4];
   const double zero = 0.0;
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   for( i = 0; i < 4; i++ )
   {
      high[i]   = 10.0 + 2.0 * (double)i;
      low[i]    = 9.0  + 2.0 * (double)i;
      volume[i] = 5e-324;          /* DBL_TRUE_MIN, and not zero */
   }

   rc = TA_EMV( 0, 3, high, low, volume, 1, 2.0, &begIdx, &nbElement, out );
   if( rc != TA_SUCCESS || begIdx != 1 || nbElement != 3 )
   {
      printf( "EMV underflow Fail: rc=%d shape (%d,%d), expected (1,3)\n",
              (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < 3; i++ )
   {
      if( memcmp( &out[i], &zero, sizeof(double) ) != 0 )
      {
         printf( "EMV underflow Fail at bar %d: got %.17g, expected +0.0 -- "
                 "the volume is non-zero but the box it scales to is not\n",
                 i+1, out[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (3) The output aliased over each input in turn.
 *
 * Every read a bar needs happens before its store, and the window carries the
 * raw values rather than re-reading the inputs at the trailing index, so the
 * caller may hand back any of the three arrays as the output.
 */
static ErrorNumber test_emv_aliasing( const TA_History *history )
{
   static TA_Real ref[OUT_CAP];
   static TA_Real work[OUT_CAP];
   const TA_Real *src[3];
   const char *name[3] = { "inHigh", "inLow", "inVolume" };
   TA_RetCode retCode;
   TA_Integer begIdx, nbElement, begIdx2, nbElement2;
   int which, i, nb;

   nb = (int)history->nbBars;

   retCode = TA_EMV( 0, nb - 1, history->high, history->low, history->volume,
                     14, 10000.0, &begIdx, &nbElement, ref );
   if( retCode != TA_SUCCESS )
   {
      printf( "EMV aliasing Fail: baseline retCode = %d\n", (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   src[0] = history->high;
   src[1] = history->low;
   src[2] = history->volume;

   for( which = 0; which < 3; which++ )
   {
      const TA_Real *in[3];

      for( i = 0; i < nb; i++ )
         work[i] = src[which][i];

      for( i = 0; i < 3; i++ )
         in[i] = ( i == which ) ? work : src[i];

      retCode = TA_EMV( 0, nb - 1, in[0], in[1], in[2],
                        14, 10000.0, &begIdx2, &nbElement2, work );
      if( retCode != TA_SUCCESS || begIdx2 != begIdx || nbElement2 != nbElement )
      {
         printf( "EMV aliasing Fail (%s): rc=%d shape (%d,%d) vs (%d,%d)\n",
                 name[which], (int)retCode, (int)begIdx2, (int)nbElement2,
                 (int)begIdx, (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( memcmp( ref, work, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
      {
         printf( "EMV aliasing Fail (%s): output differs from the "
                 "non-aliased call\n", name[which] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}

/* (4) The generic start/end range sweep. */
static TA_RetCode emvRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                        TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                        TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                        TA_Integer *lookback, void *opaqueData,
                                        unsigned int outputNb, unsigned int *isOutputInteger )
{
   EmvRangeParam *p = (EmvRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = TA_EMV_Lookback( p->period, p->divisor );
   return TA_EMV( startIdx, endIdx, p->high, p->low, p->volume,
                  p->period, p->divisor, outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_emv_range( const TA_History *history )
{
   EmvRangeParam param;

   param.period  = 14;
   param.divisor = 10000.0;
   param.high    = history->high;
   param.low     = history->low;
   param.volume  = history->volume;

   return doRangeTestEx( emvRangeTestFunction,
                         TA_STABLE_EPSILON, TA_TEST_UNST_NONE,
                         (void *)&param, 1, 0 );
}

/* (5) The single-precision entry point.
 *
 * TA_S_EMV takes float inputs but computes in double throughout, so on the
 * SAME widened values it must be bit-identical to TA_EMV. The period grid
 * spans the stack CIRCBUF and the heap one (>50), because a defect confined to
 * the heap path is invisible at a single small period.
 */
static ErrorNumber test_emv_single( const TA_History *history )
{
   static TA_Real outD[OUT_CAP], outS[OUT_CAP];
   static TA_Real dHigh[OUT_CAP], dLow[OUT_CAP], dVolume[OUT_CAP];
   static float   fHigh[OUT_CAP], fLow[OUT_CAP], fVolume[OUT_CAP];
   TA_RetCode rcD, rcS;
   TA_Integer begD, nbD, begS, nbS;
   int i, nb, p;

   nb = (int)history->nbBars;
   if( nb > OUT_CAP )
      nb = OUT_CAP;

   for( i = 0; i < nb; i++ )
   {
      fHigh[i]   = (float)history->high[i];
      fLow[i]    = (float)history->low[i];
      fVolume[i] = (float)history->volume[i];
      dHigh[i]   = (TA_Real)fHigh[i];
      dLow[i]    = (TA_Real)fLow[i];
      dVolume[i] = (TA_Real)fVolume[i];
   }

   for( p = 0; p < NB_EMV_PERIOD; p++ )
   {
      int period = emvPeriod[p];

      if( period >= nb )
         continue;

      rcD = TA_EMV  ( 0, nb - 1, dHigh, dLow, dVolume, period, 10000.0,
                      &begD, &nbD, outD );
      rcS = TA_S_EMV( 0, nb - 1, fHigh, fLow, fVolume, period, 10000.0,
                      &begS, &nbS, outS );

      if( rcD != rcS || begD != begS || nbD != nbS )
      {
         printf( "EMV single Fail (period %d): rc %d/%d shape (%d,%d)/(%d,%d)\n",
                 period, (int)rcD, (int)rcS, (int)begD, (int)nbD,
                 (int)begS, (int)nbS );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }
      if( rcD != TA_SUCCESS )
         continue;
      if( memcmp( outD, outS, (size_t)nbD * sizeof(TA_Real) ) != 0 )
      {
         printf( "EMV single Fail (period %d): TA_S_EMV differs from TA_EMV "
                 "on the same widened values\n", period );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   return TA_TEST_PASS;
}
