/* v0.8.1, the first release with explicit fma().
 *
 * Compared from period 1 and over every MAType. The rows cover the value
 * changes made since, each sized at 3x its measured maximum; the waivers cover
 * the flat-start 0/0 (#480), TRIX at a period of 1 (#505) and the index named
 * on a tie (#503). */

#include <stddef.h>
#include <string.h>

#include "ta_ref.h"

static const TaRefTol TOL[] = {
   /* #411 the hoisted 1/period in the Wilder step: absolute for the
    * oscillators, output-relative for a DM (a running sum of price moves). */
   { "CMO",      TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 8.44e-14 */
   { "PLUS_DI",  TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 4.26e-14 */
   { "MINUS_DI", TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 3.55e-14 */
   { "DX",       TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 5.68e-14 */
   { "ADX",      TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 8.53e-14 */
   { "ADXR",     TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 7.11e-14 */
   { "PLUS_DM",  TA_REF_TOL_REL_OUT, 3e-15, 0.0 }, /* measured 9.53e-16 */
   { "MINUS_DM", TA_REF_TOL_REL_OUT, 3e-15, 0.0 }, /* measured 7.98e-16 */
   /* #434 the sums of squares rebuilt against their peak. Output-relative for
    * the variance family; absolute for CORREL, a coefficient in [-1,1], and for
    * RVI, a bounded oscillator. */
   { "VAR",      TA_REF_TOL_REL_OUT, 2e-9,  0.0 }, /* measured 4.73e-10 */
   { "STDDEV",   TA_REF_TOL_REL_OUT, 8e-10, 0.0 }, /* measured 2.37e-10 */
   { "BBANDS",   TA_REF_TOL_REL_OUT, 3e-10, 0.0 }, /* measured 8.96e-11 */
   { "CORREL",   TA_REF_TOL_ABS,     2e-9,  0.0 }, /* measured 3.89e-10 */
   { "RVI",      TA_REF_TOL_ABS,     3e-11, 0.0 }, /* measured 7.5e-12 */
   /* #505 the EMA step as one FMA, k*x + beta*prev. Floored at the input scale
    * for an output in price units, where a zero crossing is all cancellation;
    * floored at 1 for the percentages; absolute for the bounded oscillators.
    * EFI is price x volume, a unit no mode floors at: its row is sized by its
    * zero crossings against the price floor. STOCH, STOCHF, KDJ and STOCHRSI
    * move only through an EMA-family MAType. */
   { "EMA",      TA_REF_TOL_REL_OUT_INFLOOR, 1e-15, 0.0 }, /* measured 3.23e-16 */
   { "DEMA",     TA_REF_TOL_REL_OUT_INFLOOR, 2e-15, 0.0 }, /* measured 5.19e-16 */
   { "TEMA",     TA_REF_TOL_REL_OUT_INFLOOR, 4e-15, 0.0 }, /* measured 1.13e-15 */
   { "ZLEMA",    TA_REF_TOL_REL_OUT_INFLOOR, 2e-15, 0.0 }, /* measured 5.83e-16 */
   { "MA",       TA_REF_TOL_REL_OUT_INFLOOR, 3e-15, 0.0 }, /* measured 9.7e-16 */
   { "MAVP",     TA_REF_TOL_REL_OUT_INFLOOR, 4e-15, 0.0 }, /* measured 1.02e-15 */
   { "KC",       TA_REF_TOL_REL_OUT_INFLOOR, 2e-15, 0.0 }, /* measured 4.37e-16 */
   { "APO",      TA_REF_TOL_REL_OUT_INFLOOR, 6e-15, 0.0 }, /* measured 1.83e-15 */
   { "MACD",     TA_REF_TOL_REL_OUT_INFLOOR, 2e-15, 0.0 }, /* measured 3.93e-16 */
   { "MACDEXT",  TA_REF_TOL_REL_OUT_INFLOOR, 4e-15, 0.0 }, /* measured 1.13e-15 */
   { "MACDFIX",  TA_REF_TOL_REL_OUT_INFLOOR, 2e-15, 0.0 }, /* measured 3.89e-16 */
   { "ERI",      TA_REF_TOL_REL_OUT_INFLOOR, 8e-16, 0.0 }, /* measured 2.6e-16 */
   { "TRIX",     TA_REF_TOL_REL_OUT_FLOOR1,  3e-13, 0.0 }, /* measured 9.17e-14 */
   { "CVI",      TA_REF_TOL_REL_OUT_FLOOR1,  2e-13, 0.0 }, /* measured 5.61e-14 */
   { "PPO",      TA_REF_TOL_REL_OUT_FLOOR1,  2e-12, 0.0 }, /* measured 4.48e-13 */
   { "PVO",      TA_REF_TOL_REL_OUT_FLOOR1,  5e-13, 0.0 }, /* measured 1.61e-13 */
   { "EFI",      TA_REF_TOL_REL_OUT_INFLOOR, 3e-12, 0.0 }, /* measured 9.62e-13 */
   { "TSI",      TA_REF_TOL_ABS,             2e-13, 0.0 }, /* measured 4.97e-14 */
   { "SMI",      TA_REF_TOL_ABS,             2e-13, 0.0 }, /* measured 5.68e-14 */
   { "STOCH",    TA_REF_TOL_ABS,             6e-13, 0.0 }, /* measured 1.71e-13 */
   { "STOCHF",   TA_REF_TOL_ABS,             3e-13, 0.0 }, /* measured 7.11e-14 */
   { "KDJ",      TA_REF_TOL_ABS,             6e-13, 0.0 }, /* measured 1.99e-13 */
   { "STOCHRSI", TA_REF_TOL_ABS,             3e-13, 0.0 }, /* measured 9.95e-14 */
   { "MASSI",    TA_REF_TOL_ABS,             3e-13, 0.0 }, /* measured 7.11e-14 */
   /* #507 ADOSC takes the EMA coefficients of #505. Volume x a ratio: it does
    * not scale with price, so no input floor applies, and the row is sized by
    * its zero crossings. */
   { "ADOSC",    TA_REF_TOL_REL_OUT,         2e-9,  0.0 }, /* measured 5.77e-10 */
};

/* Each ceiling sits halfway between the largest share measured on one function
 * and 1. */
enum { W_RSI, W_TRIX1, W_INDEX_TIE };
static const TaRefWaiver WAIVERS[] = {
   [W_RSI] = { "rsi_flat_480",
               "RSI with no change from the first bar read to the first output, where 0.8.1 answers 0 for 0/0, and STOCHRSI once a move follows (#480)", 0.56 },
   [W_TRIX1] = { "trix_period_1",
                 "TRIX at a period of 1 over a bar where (x-prev)+prev is not x: 0.8.1 takes three such steps and loses the input, the one-FMA step copies it (#505)", 0.51 },
   [W_INDEX_TIE] = { "index_tie_503",
               "MAXINDEX/MININDEX/MINMAXINDEX over a window that holds its extreme on several bars, where 0.8.1 names the oldest or the newest of them by where the call started (#503)", 0.71 },
};

/* RSI's gain and loss sums are both zero while nothing has moved since the
 * first bar read, so the bars from that one to the first output tell whether
 * the case starts on a 0/0. */
static int flat( const double *x, int from, int to )
{
   int t;

   for( t = from + 1; t <= to; t++ )
      if( x[t] != x[t-1] ) return 0;
   return 1;
}

/* STOCHRSI ranks the RSI inside its own recent range: on input flat
 * throughout, both ranges are empty and both answer 0, so it is waived only
 * when a move follows the flat start. Its lookback also spans the stochastic,
 * so the RSI's seed is sized by optInTimePeriod. The predicate is wider than
 * the difference: many waived cases match, among them every flat start
 * followed by moves in one direction only. */
static int waive( const TaRefCase *c )
{
   int isRsi = strcmp( c->func, "RSI" ) == 0;
   int first, from, seed;

   if( strcmp( c->func, "TRIX" ) == 0 )
   {
      if( (int)ta_ref_opt( c, "optInTimePeriod", 0 ) != 1 || c->lookback < 0 ) return -1;
      first = (c->startIdx > c->lookback) ? c->startIdx : c->lookback;
      if( first > c->endIdx || c->endIdx >= c->n ) return -1;
      for( from = first - c->lookback + 1; from <= c->endIdx; from++ )
         if( (c->close[from] - c->close[from-1]) + c->close[from-1] != c->close[from] )
            return W_TRIX1;
      return -1;
   }
   if( strcmp( c->func, "MAXINDEX" ) == 0 || strcmp( c->func, "MININDEX" ) == 0 ||
       strcmp( c->func, "MINMAXINDEX" ) == 0 )
      return ta_ref_index_tie( c ) ? W_INDEX_TIE : -1;
   if( !isRsi && strcmp( c->func, "STOCHRSI" ) != 0 ) return -1;
   if( c->lookback < 1 ) return -1;
   first = (c->startIdx > c->lookback) ? c->startIdx : c->lookback;
   if( first > c->endIdx || c->endIdx >= c->n ) return -1;
   from = first - c->lookback;

   if( isRsi )
      return flat( c->close, from, first ) ? W_RSI : -1;

   seed = (int)ta_ref_opt( c, "optInTimePeriod", 0 );
   if( seed < 1 || seed > c->lookback ) return -1;
   return flat( c->close, from, from + seed ) && !flat( c->close, from, c->endIdx ) ? W_RSI : -1;
}

const TaRefMember ta_ref_member = {
   .commit      = "5e766ddf650bbd4f5b49ac3c03489559ee094f28",
   .nbFunctions = 201,
   .tol         = TOL,
   .nbTol       = (int)(sizeof(TOL) / sizeof(TOL[0])),
   .waivers     = WAIVERS,
   .nbWaivers   = (int)(sizeof(WAIVERS) / sizeof(WAIVERS[0])),
   .waive       = waive,
};
