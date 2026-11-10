/* v0.8.1, the first release with explicit fma().
 *
 * Compared from period 1 and over every MAType. The rows cover the value
 * changes made since, each sized at 3x its measured maximum; the waivers cover
 * the flat-start 0/0 (#480), TRIX at a period of 1 (#505), the index named
 * on a tie (#503), inputs near the domain bound for six of the rows, and the
 * VORTEX/ULTOSC windows whose running sums are now rebuilt (#588). */

#include <math.h>
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
   /* #560 CCI's window average as a running sum. Floored at 1: the largest move
    * is a period-2 window of two near-equal prices, all cancellation. */
   { "CCI",      TA_REF_TOL_REL_OUT_FLOOR1,  3e-10, 0.0 }, /* measured 7.34e-11 */
};

/* Each ceiling sits halfway between the largest share measured on one function
 * and 1, except the #588 two: their predicate replays the exact sums, so they
 * sit at about 3x the largest share measured (0.31%). */
enum { W_RSI, W_TRIX1, W_INDEX_TIE, W_DOMAIN_EDGE, W_REBUILT_EDGE, W_REBUILT };
static const TaRefWaiver WAIVERS[] = {
   [W_RSI] = { "rsi_flat_480",
               "RSI with no change from the first bar read to the first output, where 0.8.1 answers 0 for 0/0, and STOCHRSI once a move follows (#480)", 0.56 },
   [W_TRIX1] = { "trix_period_1",
                 "TRIX at a period of 1 over a bar where (x-prev)+prev is not x: 0.8.1 takes three such steps and loses the input, the one-FMA step copies it (#505)", 0.51 },
   [W_INDEX_TIE] = { "index_tie_503",
               "MAXINDEX/MININDEX/MINMAXINDEX over a window that holds its extreme on several bars, where 0.8.1 names the oldest or the newest of them by where the call started (#503)", 0.71 },
   [W_DOMAIN_EDGE] = { "domain_edge",
               "VAR/STDDEV/CORREL/TRIX/PPO/EFI reading an input near the domain bound, where 0.8.1's rounding sits further from the current values than the rows sized on ordinary data", 0.55 },
   [W_REBUILT_EDGE] = { "rebuilt_edge_588",
               "VORTEX/ULTOSC where a running sum first comes out negative as a bar near the domain bound retires: the current library rebuilds it from the window, 0.8.1 keeps the residue (#588)", 0.01 },
   [W_REBUILT] = { "rebuilt_588",
               "VORTEX/ULTOSC where a running sum first comes out negative away from the domain bound, a few units in the last place below an exact zero: the current library rebuilds it from the window, 0.8.1 keeps the residue (#588)", 0.01 },
};

/* Keeps those rows at their ordinary-data size: a case whose inputs pass
 * DOMAIN_EDGE in the span it reads is not compared at all. */
#define DOMAIN_EDGE 1e30
static int domain_edge( const TaRefCase *c )
{
   static const char *const FUNCS[] = { "VAR", "STDDEV", "CORREL", "TRIX", "PPO", "EFI", NULL };
   int i, first, j, readsVolume;

   for( i = 0; FUNCS[i] && strcmp( c->func, FUNCS[i] ) != 0; i++ ) ;
   if( !FUNCS[i] || c->lookback < 0 ) return 0;
   first = (c->startIdx > c->lookback) ? c->startIdx : c->lookback;
   if( first > c->endIdx || c->endIdx >= c->n ) return 0;
   readsVolume = strcmp( c->func, "CORREL" ) == 0 || strcmp( c->func, "EFI" ) == 0;
   for( j = first - c->lookback; j <= c->endIdx; j++ )
      if( fabs( c->close[j] ) > DOMAIN_EDGE || (readsVolume && fabs( c->volume[j] ) > DOMAIN_EDGE) )
         return 1;
   return 0;
}

/* Replays the running sums the current library keeps over the case's call,
 * in its order of operations, and reports the first window where one of them
 * comes out negative: there it rebuilds the sums from the window and 0.8.1
 * does not, and the two agree on every window before it. The retiring bar and
 * the one it reads decide which waiver the window takes. */
static int bar_at_edge( const TaRefCase *c, int j )
{
   return fabs( c->high[j] ) > DOMAIN_EDGE || fabs( c->low[j] ) > DOMAIN_EDGE
       || fabs( c->close[j] ) > DOMAIN_EDGE;
}

static int rebuilt_window( const TaRefCase *c, int span )
{
   int j;
   for( j = span; j >= 0 && j >= span - 1; j-- )
      if( bar_at_edge( c, j ) ) return W_REBUILT_EDGE;
   return W_REBUILT;
}

static double true_range( const TaRefCase *c, int i )
{
   double tr = c->high[i] - c->low[i];
   double d = fabs( c->close[i-1] - c->high[i] );
   if( d > tr ) tr = d;
   d = fabs( c->close[i-1] - c->low[i] );
   if( d > tr ) tr = d;
   return tr;
}

static int vortex_rebuilt( const TaRefCase *c )
{
   int n = (int)ta_ref_opt( c, "optInTimePeriod", 0 );
   int s, t, i, nullRun = 0, live;
   double sTR = 0.0, sP = 0.0, sM = 0.0, tr;

   if( n < 1 || c->endIdx >= c->n ) return -1;
   s = c->startIdx > n ? c->startIdx : n;
   if( s > c->endIdx ) return -1;
   for( i = s - n + 1; i < s; i++ )
   {
      tr = true_range( c, i );
      sTR += tr;
      sP  += fabs( c->high[i] - c->low[i-1] );
      sM  += fabs( c->low[i] - c->high[i-1] );
      live = tr <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (live - 1);
   }
   for( t = s; t <= c->endIdx; t++ )
   {
      tr = true_range( c, t );
      sTR += tr;
      sP  += fabs( c->high[t] - c->low[t-1] );
      sM  += fabs( c->low[t] - c->high[t-1] );
      live = tr <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (live - 1);
      if( nullRun >= n ) { nullRun = n; sTR = 0.0; }
      if( sTR < 0.0 || sP < 0.0 || sM < 0.0 ) return rebuilt_window( c, t - n );
      i = t - n + 1;
      sTR -= true_range( c, i );
      sP  -= fabs( c->high[i] - c->low[i-1] );
      sM  -= fabs( c->low[i] - c->high[i-1] );
   }
   return -1;
}

static void ultosc_terms( const TaRefCase *c, int i, double *a, double *b )
{
   double trueLow = c->low[i] < c->close[i-1] ? c->low[i] : c->close[i-1];
   *a = c->close[i] - trueLow;
   *b = true_range( c, i );
}

static int ultosc_rebuilt( const TaRefCase *c )
{
   int p[3], k, m, s, t, i, nullRun = 0, live;
   double A[3] = { 0.0, 0.0, 0.0 }, B[3] = { 0.0, 0.0, 0.0 }, a, b;

   p[0] = (int)ta_ref_opt( c, "optInTimePeriod1", 0 );
   p[1] = (int)ta_ref_opt( c, "optInTimePeriod2", 0 );
   p[2] = (int)ta_ref_opt( c, "optInTimePeriod3", 0 );
   for( k = 0; k < 2; k++ )
      for( m = 0; m < 2 - k; m++ )
         if( p[m] > p[m+1] ) { i = p[m]; p[m] = p[m+1]; p[m+1] = i; }
   if( p[0] < 1 || c->endIdx >= c->n ) return -1;
   s = c->startIdx > p[2] ? c->startIdx : p[2];
   if( s > c->endIdx ) return -1;
   for( i = s - p[2] + 1; i < s; i++ )
   {
      ultosc_terms( c, i, &a, &b );
      live = b + fabs( a ) <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (live - 1);
      for( k = 0; k < 3; k++ )
         if( i >= s - p[k] + 1 ) { A[k] += a; B[k] += b; }
   }
   for( t = s; t <= c->endIdx; t++ )
   {
      ultosc_terms( c, t, &a, &b );
      for( k = 0; k < 3; k++ ) A[k] += a;
      for( k = 0; k < 3; k++ ) B[k] += b;
      live = b + fabs( a ) <= 0.0 ? 0 : 1;
      nullRun = (nullRun + 1) & (live - 1);
      for( k = 0; k < 3 && nullRun >= p[k]; k++ ) { A[k] = 0.0; B[k] = 0.0; }
      if( nullRun > p[2] ) nullRun = p[2];
      for( k = 0; k < 3; k++ )
         if( A[k] < 0.0 || B[k] < 0.0 ) return rebuilt_window( c, t - p[k] );
      for( k = 0; k < 3; k++ )
      {
         ultosc_terms( c, t - p[k] + 1, &a, &b );
         A[k] -= a;
         B[k] -= b;
      }
   }
   return -1;
}

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

   if( domain_edge( c ) ) return W_DOMAIN_EDGE;
   if( strcmp( c->func, "VORTEX" ) == 0 ) return vortex_rebuilt( c );
   if( strcmp( c->func, "ULTOSC" ) == 0 ) return ultosc_rebuilt( c );
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
