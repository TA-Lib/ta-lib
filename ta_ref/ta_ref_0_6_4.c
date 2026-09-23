/* v0.6.4, the last release of the hand-written C library.
 *
 * Proves the current library bit-identical to it, function by function, apart
 * from the carve-outs below. Each one is a fix made since, so comparing there
 * would diff the fix itself; the fixed behaviour is pinned by its own tests.
 *
 * - Periods are floored at 2: 0.6.4 rejects period 1 or has period-1
 *   out-of-bounds bugs.
 * - STOCHRSI is excluded (#107): 0.6.4 divided a sub-epsilon flat-RSI residue
 *   into full-scale [0,100] noise. STOCH/STOCHF on raw OHLC do not diverge and
 *   stay compared.
 * - 0.6.4 predates explicit fma() (PR #96), so every function without a named
 *   row may differ by the 1e-9 relative FMA contract ("*" below).
 *
 * The waiver predicates are two-pass on purpose: deciding whether to trust the
 * oracle must not re-run either implementation's algorithm. */

#include <float.h>
#include <math.h>
#include <string.h>

#include "ta_ref.h"

#define TA_MATYPE_KAMA 6
#define MAX_BARS 4096

static const char *const EXCLUDED[] = { "STOCHRSI", NULL };

static const TaRefTol TOL[] = {
   { "CCI",                 TA_REF_TOL_ABS,     1e-9,  0.0 }, /* #7   near-zero identical-price fix */
   /* #118 cancellation-free variance, output-relative because VAR's output is a
    * squared quantity. Only well-conditioned windows reach here (variance_condition). */
   { "VAR",                 TA_REF_TOL_REL_OUT, 1e-9,  0.0 },
   { "STDDEV",              TA_REF_TOL_REL_OUT, 1e-9,  0.0 },
   { "BBANDS",              TA_REF_TOL_REL_OUT, 1e-9,  0.0 },
   /* #242 the same for the two-series forms. CORREL is a coefficient in [-1,1],
    * so absolute: relative error is ill-posed as r nears 0. BETA is a ratio of
    * return scales, so output-relative. */
   { "CORREL",              TA_REF_TOL_ABS,     6e-11, 0.0 }, /* measured 1.69e-11 */
   { "BETA",                TA_REF_TOL_REL_OUT, 1e-9,  0.0 }, /* measured 3.12e-10 */
   { "LINEARREG",           TA_REF_TOL_REL_IN,  1e-9,  0.0 }, /* #103 O(1) sliding-sum recurrence */
   { "LINEARREG_SLOPE",     TA_REF_TOL_REL_IN,  1e-9,  0.0 },
   { "LINEARREG_INTERCEPT", TA_REF_TOL_REL_IN,  1e-9,  0.0 },
   { "LINEARREG_ANGLE",     TA_REF_TOL_REL_IN,  1e-9,  0.5 }, /* bounded degrees */
   { "TSF",                 TA_REF_TOL_REL_IN,  1e-9,  0.0 },
   /* #255 WMA re-anchors its weighted totals every 8*period bars. STOCH/STOCHF
    * are here only because their smoothing dispatches to TA_MAType_WMA. */
   { "WMA",                 TA_REF_TOL_REL_IN,  1e-9,  0.0 },
   { "STOCH",               TA_REF_TOL_REL_IN,  1e-9,  0.0 },
   { "STOCHF",              TA_REF_TOL_REL_IN,  1e-9,  0.0 },
   /* #338 the two-coefficient Wilder step. Named so the 1e-9 "*" row, six orders
    * looser, cannot swallow a real ATR regression; NATR divides by a close. */
   { "ATR",                 TA_REF_TOL_REL_IN,  3e-15, 0.0 }, /* measured 7.78e-16 */
   { "NATR",                TA_REF_TOL_REL_OUT, 4e-15, 0.0 }, /* measured 1.32e-15 */
   /* #395 %R is a bounded dimensionless oscillator: its floor is a ULP of 100
    * whatever the input magnitude. */
   { "WILLR",               TA_REF_TOL_ABS,     5e-14, 0.0 }, /* measured 1.42e-14 */
   /* #410/#411 the hoisted 1/period in the Wilder step. Absolute for the
    * oscillators (measured identical at price scale 1e-7, 1 and 1e9);
    * output-relative for a DM, a running sum of price moves. Measured through
    * default+51. */
   { "RSI",                 TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 5.68e-14 */
   { "CMO",                 TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 8.44e-14 */
   { "PLUS_DM",             TA_REF_TOL_REL_OUT, 5e-15, 0.0 }, /* measured 1.40e-15 */
   { "MINUS_DM",            TA_REF_TOL_REL_OUT, 2e-14, 0.0 }, /* measured 3.53e-15 */
   { "PLUS_DI",             TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 4.26e-14 */
   { "MINUS_DI",            TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 5.68e-14 */
   { "DX",                  TA_REF_TOL_ABS,     2e-13, 0.0 }, /* measured 5.68e-14 */
   { "ADX",                 TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 8.53e-14 */
   { "ADXR",                TA_REF_TOL_ABS,     3e-13, 0.0 }, /* measured 7.11e-14 */
   /* #112 an all-flat window made 0.6.4 compute 100*(0/0); the guard answers 50. */
   { "IMI",                 TA_REF_TOL_NAN_TO,  50.0,  0.0 },
   /* The FMA contract, floored at the input scale where the output differences
    * two large near-equal quantities (EMA cascades near a crossing, near-zero
    * phasor components): there the drift is a ULP of the price-scale operands,
    * unbounded relative to the output. Bounded oscillators stay output-relative
    * so an extreme-scale regression cannot hide behind the input scale. */
   { "DEMA",                TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "TEMA",                TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "TRIX",                TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "MACD",                TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "MACDFIX",             TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "MACDEXT",             TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "APO",                 TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "PPO",                 TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "HT_PHASOR",           TA_REF_TOL_REL_OUT_INFLOOR, 1e-9, 0.0 },
   { "*",                   TA_REF_TOL_REL_OUT,         1e-9, 0.0 },
};

/* Each ceiling sits halfway between the largest share measured on one function
 * and 1. */
enum { W_TRIX_NATR, W_VARIANCE, W_CORREL_BETA, W_MFI, W_KAMA, W_ULTOSC, W_MAVP };
static const TaRefWaiver WAIVERS[] = {
   [W_TRIX_NATR]   = { "trix_natr_98",
                       "TRIX/NATR with startIdx past the lookback, and NATR over a zero close (#98)", 0.77 },
   [W_VARIANCE]    = { "variance_118",
                       "VAR/STDDEV/BBANDS windows ill-conditioned for 0.6.4's E[x^2]-mean^2 (#118)", 0.73 },
   [W_CORREL_BETA] = { "correl_beta_242",
                       "CORREL/BETA windows ill-conditioned for 0.6.4's one-pass sums (#242)", 0.80 },
   [W_MFI]         = { "mfi_244",
                       "MFI windows where 0.6.4 reports a non-index: its 1.0 guard, or an empty or one-sided window (#244)", 0.78 },
   [W_KAMA]        = { "kama_253",
                       "KAMA ratio decided by accumulator residue (#253, #390), and STOCH/STOCHF smoothing with KAMA", 0.61 },
   [W_ULTOSC]      = { "ultosc_253",
                       "ULTOSC with an emptied window, where 0.6.4 divides residue (#253)", 0.56 },
   [W_MAVP]        = { "mavp_inverted_94",
                       "MAVP with its minimum period above its maximum, where 0.6.4 read uninitialized results (#94)", 0.53 },
};

/* 0.6.4 computes variance as E[x^2] - mean^2, losing about log10(kappa)
 * digits. Its accumulators run over the whole case, so a window of tiny values
 * that absorbed ~1e9 values earlier carries their rounding: the measure is the
 * largest magnitude absorbed, squared, against the smallest window variance.
 * The threshold is an order tighter than DBL_EPSILON * kappa predicts, because
 * the sliding sums also round once per step (a 240-bar case measured 5x the
 * naive estimate). A flat window can go negative through the sqrt. */
#define VAR_MAX_KAPPA 1.0e5
static double variance_condition( const double *x, int n, int period, int s, int e )
{
   double maxAbs = 0.0, minVar = HUGE_VAL;
   int t, first, j;

   if( period < 2 ) return 0.0;
   first = (s > period - 1) ? s : period - 1;
   if( first > e || first >= n ) return 0.0;

   for( j = first - period + 1; j <= e && j < n; j++ )
   {
      double m = fabs( x[j] );
      if( m > maxAbs ) maxAbs = m;
   }
   for( t = first; t <= e && t < n; t++ )
   {
      double sum = 0.0, mean, var = 0.0;
      for( j = t - period + 1; j <= t; j++ ) sum += x[j];
      mean = sum / (double)period;
      for( j = t - period + 1; j <= t; j++ ) { double dv = x[j] - mean; var += dv * dv; }
      var /= (double)period;
      if( !(var > 0.0) ) return HUGE_VAL;
      if( var < minVar ) minVar = var;
   }
   if( !(minVar > 0.0) || !(maxAbs > 0.0) ) return HUGE_VAL;
   return (maxAbs * maxAbs) / minVar;
}

/* The same measure for CORREL/BETA's S2 - (S*S)/n. HUGE_VAL also marks the
 * windows where 0.6.4 has no answer at all (a flat window, or one its absolute
 * epsilon guard zeroes): it returned exactly 0 or a correlation outside [-1,1]. */
#define XY_MAX_KAPPA 1.0e5
static double series_condition( const double *v, int n, int period,
                                int first, int e, double *outMinSS )
{
   double maxAbs = 0.0, minVar = HUGE_VAL, minSS = HUGE_VAL;
   int t, j;

   *outMinSS = 0.0;
   if( period < 2 ) return 0.0;
   if( first > e || first >= n ) return 0.0;

   for( j = first - period + 1; j <= e && j < n; j++ )
   {
      double m = fabs( v[j] );
      if( m > maxAbs ) maxAbs = m;
   }
   for( t = first; t <= e && t < n; t++ )
   {
      double sum = 0.0, mean, ss = 0.0;
      for( j = t - period + 1; j <= t; j++ ) sum += v[j];
      mean = sum / (double)period;
      for( j = t - period + 1; j <= t; j++ ) { double d = v[j] - mean; ss += d * d; }
      if( !(ss > 0.0) ) return HUGE_VAL;
      if( ss < minSS ) minSS = ss;
      if( ss / (double)period < minVar ) minVar = ss / (double)period;
   }
   if( !(minVar > 0.0) || !(maxAbs > 0.0) ) return HUGE_VAL;
   *outMinSS = minSS;
   return (maxAbs * maxAbs) / minVar;
}

static double correl_condition( const double *x, const double *y,
                                int n, int period, int s, int e )
{
   double kx, ky, ssx = 0.0, ssy = 0.0;
   int first = (s > period - 1) ? s : period - 1;

   kx = series_condition( x, n, period, first, e, &ssx );
   if( kx == HUGE_VAL ) return HUGE_VAL;
   ky = series_condition( y, n, period, first, e, &ssy );
   if( ky == HUGE_VAL ) return HUGE_VAL;
   if( kx == 0.0 && ky == 0.0 ) return 0.0;
   if( ssx * ssy < 1e-14 ) return HUGE_VAL;   /* 0.6.4: !TA_IS_ZERO_OR_NEG(ssX*ssY) */
   return (kx > ky) ? kx : ky;
}

/* BETA regresses the RETURNS, with its own zero-price guard and an absolute
 * guard on n*S_xx - S_x*S_x (= period * ssx). Its numerator also cancels on
 * its own axis where the two return series decouple: the slope is then pure
 * residue whose sign the two versions disagree on. 1/|r| is that measure, so
 * at the shared threshold this skips |r| < 1e-5. */
static double beta_condition( const double *p0, const double *p1,
                              int n, int period, int s, int e )
{
   static double rx[MAX_BARS], ry[MAX_BARS];
   double kx, ky, ssx = 0.0, ssy = 0.0;
   int first, j;

   if( n > MAX_BARS || n < 2 ) return HUGE_VAL;
   rx[0] = ry[0] = 0.0;
   for( j = 1; j < n; j++ )
   {
      rx[j] = ( p0[j-1] > 1e-14 || p0[j-1] < -1e-14 ) ? ( p0[j] - p0[j-1] ) / p0[j-1] : 0.0;
      ry[j] = ( p1[j-1] > 1e-14 || p1[j-1] < -1e-14 ) ? ( p1[j] - p1[j-1] ) / p1[j-1] : 0.0;
   }
   first = (s > period) ? s : period;
   if( first > e || first >= n ) return 0.0;

   kx = series_condition( rx, n, period, first, e, &ssx );
   if( kx == HUGE_VAL ) return HUGE_VAL;
   ky = series_condition( ry, n, period, first, e, &ssy );
   if( ky == HUGE_VAL ) return HUGE_VAL;
   if( kx == 0.0 && ky == 0.0 ) return 0.0;
   if( (double)period * ssx < 1e-14 ) return HUGE_VAL;
   if( ky > kx ) kx = ky;

   for( j = first; j <= e && j < n; j++ )
   {
      double mx = 0.0, my = 0.0, sxx = 0.0, syy = 0.0, sxy = 0.0, kn;
      int t;
      for( t = j - period + 1; t <= j; t++ ) { mx += rx[t]; my += ry[t]; }
      mx /= (double)period; my /= (double)period;
      for( t = j - period + 1; t <= j; t++ )
      {
         double dx = rx[t] - mx, dy = ry[t] - my;
         sxx += dx * dx; syy += dy * dy; sxy += dx * dy;
      }
      if( sxx <= 0.0 || syy <= 0.0 ) return HUGE_VAL;
      if( !(fabs( sxy ) > 0.0) ) return HUGE_VAL;
      kn = sqrt( sxx * syy ) / fabs( sxy );
      if( kn > kx ) kx = kn;
   }
   return kx;
}

/* MFI: categorical, not graded. 0.6.4 zeroed the index whenever a window's
 * money flow summed under a literal 1.0 (a price times a volume), and divided
 * residue by itself on an empty or one-sided window. Everything else is
 * compared; on this corpus, bit-exact. */
static int mfi_blind( const double *h, const double *l, const double *c,
                      const double *v, int n, int period, int s, int e )
{
   int t, j, first;

   if( period < 1 ) return 0;
   first = (s > period) ? s : period;
   if( first > e || first >= n ) return 0;

   for( t = first; t <= e && t < n; t++ )
   {
      double pos = 0.0, neg = 0.0, total;
      for( j = t - period + 1; j <= t; j++ )
      {
         double tp  = (h[j]   + l[j]   + c[j])   / 3.0;
         double tpp = (h[j-1] + l[j-1] + c[j-1]) / 3.0;
         if     ( tp > tpp ) pos += tp * v[j];
         else if( tp < tpp ) neg += tp * v[j];
      }
      total = pos + neg;
      if( !(total > 0.0) )               return 1;   /* empty window   */
      if( total < 1.0 )                  return 1;   /* 0.6.4's guard  */
      if( !(pos > 0.0) || !(neg > 0.0) ) return 1;   /* one-sided      */
   }
   return 0;
}

/* KAMA's efficiency ratio divides by a sliding sum of |1-bar changes| kept by
 * add-then-subtract. On a flat window 0.6.4 decides the 0/0 with an absolute
 * band on the residue (#253); where absorption puts the residue at the scale of
 * the window's own sum, 0.6.4's ratio leaves [0,1] (#390). The scan starts at
 * the first full window, not at startIdx, because prevKAMA carries a divergence
 * forward. */
static int kama_blind( const double *x, int n, int period, int e )
{
   int t, j;
   double everSeen = 0.0;

   if( period < 2 ) return 0;
   if( e >= n ) e = n - 1;

   for( t = 1; t < period && t < n; t++ )
   {
      double d = fabs( x[t] - x[t-1] );
      if( d > everSeen ) everSeen = d;
   }
   for( t = period; t <= e; t++ )
   {
      double sum = 0.0, d;
      int flat = 1;
      d = fabs( x[t] - x[t-1] );
      if( d > everSeen ) everSeen = d;
      for( j = t - period + 1; j <= t; j++ )
      {
         double c = x[j] - x[j-1];
         if( c != 0.0 ) flat = 0;
         sum += fabs( c );
      }
      if( flat ) return 1;
      if( sum < 1e-14 ) return 1;
      if( everSeen * DBL_EPSILON >= sum ) return 1;
   }
   return 0;
}

/* STOCH/STOCHF smoothing with KAMA runs it over the Fast-K series, which is not
 * an input and cannot be examined without re-running the library. MACDEXT can
 * smooth with KAMA too and stays compared: it does not diverge on this corpus. */
static int smooths_with_kama( const TaRefCase *c )
{
   int i;
   for( i = 0; i < c->nbOpt; i++ )
      if( strstr( c->optName[i], "MAType" ) && (int)c->optValue[i] == TA_MATYPE_KAMA )
         return 1;
   return 0;
}

/* ULTOSC's three moving totals are sliding sums of true ranges: an emptied
 * window leaves residue of arbitrary sign, and 0.6.4 divides one residue by
 * another (-92.9 on the ZEROSUM shape, for a 0..100 oscillator). */
static int ultosc_blind( const double *h, const double *l, const double *c,
                         int n, int p1, int p2, int p3, int e )
{
   int per[3], k, t, j, longest;

   per[0] = p1; per[1] = p2; per[2] = p3;
   longest = p1 > p2 ? p1 : p2;
   if( p3 > longest ) longest = p3;
   if( longest < 1 || longest >= n ) return 0;
   if( e >= n ) e = n - 1;

   for( t = longest; t <= e; t++ )
      for( k = 0; k < 3; k++ )
      {
         double total = 0.0;
         if( per[k] < 1 ) continue;
         for( j = t - per[k] + 1; j <= t; j++ )
         {
            double prevClose = c[j-1];
            double trueLow   = ( l[j] < prevClose ) ? l[j] : prevClose;
            double trueHigh  = ( h[j] > prevClose ) ? h[j] : prevClose;
            total += trueHigh - trueLow;
         }
         if( total < 1e-14 ) return 1;
      }
   return 0;
}

/* Real inputs are mapped close first, then volume. */
static int waive( const TaRefCase *c )
{
   const char *f = c->func;
   int period = (int)ta_ref_opt( c, "optInTimePeriod", 0 );
   int s = c->startIdx, e = c->endIdx, n = c->n;

   if( strcmp( f, "TRIX" ) == 0 || strcmp( f, "NATR" ) == 0 )
   {
      int z;
      if( c->lookback < 0 ) return -1;
      if( s > c->lookback ) return W_TRIX_NATR;
      if( strcmp( f, "NATR" ) == 0 )
         for( z = (s > c->lookback ? s : c->lookback); z <= e; z++ )
            if( c->close[z] < 0.00000001 && c->close[z] > -0.00000001 )
               return W_TRIX_NATR;
      return -1;
   }
   if( strcmp( f, "VAR" ) == 0 || strcmp( f, "STDDEV" ) == 0 || strcmp( f, "BBANDS" ) == 0 )
      return variance_condition( c->close, n, period, s, e ) > VAR_MAX_KAPPA ? W_VARIANCE : -1;
   if( strcmp( f, "CORREL" ) == 0 )
      return correl_condition( c->close, c->volume, n, period, s, e ) > XY_MAX_KAPPA ? W_CORREL_BETA : -1;
   if( strcmp( f, "BETA" ) == 0 )
      return beta_condition( c->close, c->volume, n, period, s, e ) > XY_MAX_KAPPA ? W_CORREL_BETA : -1;
   if( strcmp( f, "MFI" ) == 0 )
      return mfi_blind( c->high, c->low, c->close, c->volume, n, period, s, e ) ? W_MFI : -1;
   if( strcmp( f, "KAMA" ) == 0 )
      return kama_blind( c->close, n, period, e ) ? W_KAMA : -1;
   if( strcmp( f, "STOCH" ) == 0 || strcmp( f, "STOCHF" ) == 0 )
      return smooths_with_kama( c ) ? W_KAMA : -1;
   if( strcmp( f, "MAVP" ) == 0 )
      return ta_ref_opt( c, "optInMinPeriod", 0 ) > ta_ref_opt( c, "optInMaxPeriod", 0 ) ? W_MAVP : -1;
   if( strcmp( f, "ULTOSC" ) == 0 )
      return ultosc_blind( c->high, c->low, c->close, n,
                           (int)ta_ref_opt( c, "optInTimePeriod1", 0 ),
                           (int)ta_ref_opt( c, "optInTimePeriod2", 0 ),
                           (int)ta_ref_opt( c, "optInTimePeriod3", 0 ), e ) ? W_ULTOSC : -1;
   return -1;
}

const TaRefMember ta_ref_member = {
   .commit      = "43f9d5042ecc4bd367941846494ad907bf20ea50",
   .nbFunctions = 161,
   .intFloor    = 2,
   .excluded    = EXCLUDED,
   .tol         = TOL,
   .nbTol       = (int)(sizeof(TOL) / sizeof(TOL[0])),
   .waivers     = WAIVERS,
   .nbWaivers   = (int)(sizeof(WAIVERS) / sizeof(WAIVERS[0])),
   .waive       = waive,
};
