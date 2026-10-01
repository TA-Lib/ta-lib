/* v0.8.1, the first release with explicit fma().
 *
 * Compared from period 1 and over every MAType. The rows cover the value
 * changes made since, each sized at 3x its measured maximum; the waiver covers
 * the flat-start 0/0 (#480). */

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
};

/* The ceiling sits halfway between the largest share measured on one function
 * and 1. */
enum { W_RSI };
static const TaRefWaiver WAIVERS[] = {
   [W_RSI] = { "rsi_flat_480",
               "RSI with no change from the first bar read to the first output, where 0.8.1 answers 0 for 0/0, and STOCHRSI once a move follows (#480)", 0.56 },
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
