/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100126 KL,CC  First version (#486).
 */

/* Description:
 *
 *   Test the five Swiss Army Knife rows: TA_SWAK_GAUSS, TA_SWAK_BUTTER,
 *   TA_SWAK_HP, TA_SWAK_2PHP and TA_SWAK_BP.
 *
 *   ONE file, not five. The five are one second-order IIR
 *
 *       y[i] = c0*(b0*x[i] + b1*x[i-1] + b2*x[i-2]) + a1*y[i-1] + a2*y[i-2]
 *
 *   under five coefficient rows, and every gate below is the SAME analytic
 *   property evaluated per row. Split five ways, the pole arithmetic each gate
 *   derives its bound from would be copied five times, and a bound corrected in
 *   one copy would silently stay wrong in the other four.
 *
 *   NO ORACLE, AND NO FIXED THRESHOLD. There is no independent implementation
 *   of these rows this suite can link against, so the gates assert properties
 *   the coefficient row itself fixes -- what the filter does to a constant, to
 *   a Nyquist alternation, and (band-pass) to a sinusoid at its own centre
 *   period. Every transient bound is the ANALYTIC ENVELOPE of that row's own
 *   poles, never a constant:
 *
 *       GAUSS / BUTTER / 2PHP   double real pole om = 1-a2p    k * om^k
 *       HP                      single real pole 1-a1p         p^k
 *       BP                      complex pair, |pole| = sqrt(abp)   r^k
 *
 *   Three earlier drafts used fixed thresholds and each went red on a CORRECT
 *   implementation, because the seed's transient decays at a rate the threshold
 *   knew nothing about. The seed is the steady state of a constant input, so on
 *   a constant there is no transient at all and leg 1 is exact; on any other
 *   probe there is one, and its size is the envelope.
 *
 *   Legs:
 *     1. DC: a constant input. GAUSS and BUTTER return the constant; HP, 2PHP
 *        and BP return zero, and the gate pins the SIGN of that zero. What it
 *        catches is a kernel rewritten around an outer negation: every
 *        cancellation these rows perform is x - x, which is +0.0 under
 *        round-to-nearest whichever way the terms are ordered, so only an
 *        explicit negation of the whole expression can deliver -0.0. Writing
 *        HP's step as -((c0*(x1-x0)) + a1*(-y1)) -- bit-identical everywhere
 *        else -- is the mutation that reddens it. Reordering WITHIN the
 *        numerator does not and is a known negative.
 *     2. NYQUIST: an alternating +/-1. HP and 2PHP pass it at +/-1, BUTTER and
 *        BP annihilate it, and GAUSS -- having no zero anywhere -- must NOT.
 *        That last one is the control arm: without it, a row that returned 0
 *        everywhere would satisfy every other Nyquist case in this leg.
 *     3. CENTRE: BP returns cos(2*pi*i/P) unchanged, at unity gain and zero
 *        phase. This is the only leg that pins a1, which the DC and Nyquist
 *        legs are both blind to (the numerator vanishes there whatever a1 is).
 *     4. LOOKBACK is the unstable period and nothing else, under several
 *        settings of it, per row.
 *     5. ALIASING: outReal == inReal.
 *     6. The generic start/end range sweep, for four of the five rows.
 *
 *   NOT COVERED, and deliberately left so rather than papered over: BP has no
 *   range-sweep leg. Its convergence envelope classification is the open
 *   question of #486 -- the same probe fails it deterministically under
 *   --codegen while the other four pass at every seed -- and a leg written
 *   before that is ruled would be encoding a guess. See the issue.
 *
 *   SERVER_VERIFY: the constant and Nyquist shapes are routed. The --codegen
 *   sweep sends the 252-bar corpus with one parameter moved at a time and
 *   reaches neither shape, and both are exactly where a coefficient
 *   transcription error in another language shows up as a whole number rather
 *   than a last-bit difference.
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
#define SWAK_PI 3.14159265358979323846

#define SWAK_NB_FUNC 5
#define SWAK_GAUSS   0
#define SWAK_BUTTER  1
#define SWAK_HP      2
#define SWAK_2PHP    3
#define SWAK_BP      4

/* Long enough for the slowest row to reach its steady state: leg 1 asks for
 * 30 cutoff periods and the largest period swept is 10000. */
#define SWAK_N_MAX   60000
#define SWAK_N_NYQ   20000
#define SWAK_N_ROUTE  2000

#define NB_OF(a) ((int)(sizeof(a)/sizeof((a)[0])))

/* Pinned coverage counts. Every input below is synthesized and every count is a
 * loop trip decided by a period and a length -- integer arithmetic only -- so
 * these are platform-free in the sense the suite requires. */
#define SWAK_DC_CMP       1878000
#define SWAK_NYQ_CMP       260005
#define SWAK_CENTRE_CMP    180000
#define SWAK_LOOKBACK_CMP      132
#define SWAK_INPLACE_CMP   114700
#define SWAK_ROUTE_CMP         10

static double swakIn[SWAK_N_MAX];
static double swakOut[SWAK_N_MAX];
static double swakAlias[SWAK_N_MAX];

static int g_swakDcCmp;
static int g_swakNyqCmp;
static int g_swakCentreCmp;
static int g_swakLookbackCmp;
static int g_swakInplaceCmp;
static int g_swakRouteCmp;

static const char * const swakName[SWAK_NB_FUNC] =
   { "SWAK_GAUSS", "SWAK_BUTTER", "SWAK_HP", "SWAK_2PHP", "SWAK_BP" };

static const TA_FuncUnstId swakUnstId[SWAK_NB_FUNC] =
{
   TA_FUNC_UNST_SWAK_GAUSS, TA_FUNC_UNST_SWAK_BUTTER, TA_FUNC_UNST_SWAK_HP,
   TA_FUNC_UNST_SWAK_2PHP,  TA_FUNC_UNST_SWAK_BP
};

/* 1 when the row's numerator weighs 1 at DC (gain 1), 0 when it cancels there. */
static const int swakDcUnity[SWAK_NB_FUNC] = { 1, 1, 0, 0, 0 };

/* What the row does to an alternating +/-1:
 *   -1  it has no zero at all and must NOT annihilate it (the control arm)
 *    0  the numerator vanishes there
 *    1  the numerator weighs unity there, so the alternation passes through */
static const int swakNyquist[SWAK_NB_FUNC] = { -1, 0, 1, 1, 0 };

/* Periods per row, spanning each one's declared range. HP starts at 5 (at 4 its
 * alpha rounds to exactly 0 and it degenerates into an integrator) and BP's cap
 * is 2000. */
static const int swakPerGauss[] = { 2, 5, 20, 1000, 10000 };
static const int swakPerHp[]    = { 5, 20, 1000, 100000 };
static const int swakPerBp[]    = { 5, 20, 200, 2000 };

/* Three magnitudes: well below 1, around the series scale, and well above. */
static const double swakLevel[] = { 0.00777, 100.0, 30000.7 };
/* The declared range of BP's delta, and its default. */
static const double swakDelta[] = { 0.05, 0.1, 0.5 };

static const unsigned int swakUnstValues[] = { 0, 1, 3, 50 };

static int          swakNbPeriods( int f );
static int          swakPeriod( int f, int i );
static int          swakNbDeltas( int f );
static void         swakPoles( int f, int P, double delta, double *r, int *mult );
static double       swakEnvelope( int f, int P, double delta, int k );
static TA_RetCode   swakCall( int f, int startIdx, int endIdx, const double *in,
                              int P, double delta, int *outBegIdx, int *outNbElement,
                              double *out );
static int          swakLookback( int f, int P, double delta );
static ErrorNumber  swakRoute( int f, const char *tag, int n, int P, double delta,
                               TA_RetCode rc, int begIdx, int nbElement );
static ErrorNumber  test_swak_dc( void );
static ErrorNumber  test_swak_nyquist( void );
static ErrorNumber  test_swak_centre( void );
static ErrorNumber  test_swak_lookback( void );
static ErrorNumber  test_swak_inplace( void );
static ErrorNumber  test_swak_range( const TA_Real *in );

/**** Global functions definitions. ****/
ErrorNumber test_func_swak( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   g_swakDcCmp = g_swakNyqCmp = g_swakCentreCmp = 0;
   g_swakLookbackCmp = g_swakInplaceCmp = g_swakRouteCmp = 0;

   err = test_swak_dc();
   if( err == TA_TEST_PASS )
      err = test_swak_nyquist();
   if( err == TA_TEST_PASS )
      err = test_swak_centre();
   if( err == TA_TEST_PASS )
      err = test_swak_lookback();
   if( err == TA_TEST_PASS )
      err = test_swak_inplace();

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS )
      err = test_swak_range( history->close );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS
       && ( g_swakDcCmp != SWAK_DC_CMP || g_swakNyqCmp != SWAK_NYQ_CMP
            || g_swakCentreCmp != SWAK_CENTRE_CMP
            || g_swakLookbackCmp != SWAK_LOOKBACK_CMP
            || g_swakInplaceCmp != SWAK_INPLACE_CMP
            || ( server_verify_active() && g_swakRouteCmp != SWAK_ROUTE_CMP ) ) )
   {
      printf( "SWAK Fail: coverage counters (dc %d, nyquist %d, centre %d, lookback %d, "
              "inplace %d, routed %d) are not what this file was written with "
              "(%d, %d, %d, %d, %d, %d)\n",
              g_swakDcCmp, g_swakNyqCmp, g_swakCentreCmp, g_swakLookbackCmp,
              g_swakInplaceCmp, g_swakRouteCmp,
              SWAK_DC_CMP, SWAK_NYQ_CMP, SWAK_CENTRE_CMP, SWAK_LOOKBACK_CMP,
              SWAK_INPLACE_CMP, SWAK_ROUTE_CMP );
      return TA_SWAK_VACUOUS;
   }

   return err;
}

/**** Local functions definitions. ****/
static int swakNbPeriods( int f )
{
   if( f == SWAK_HP ) return NB_OF(swakPerHp);
   if( f == SWAK_BP ) return NB_OF(swakPerBp);
   return NB_OF(swakPerGauss);
}

static int swakPeriod( int f, int i )
{
   if( f == SWAK_HP ) return swakPerHp[i];
   if( f == SWAK_BP ) return swakPerBp[i];
   return swakPerGauss[i];
}

static int swakNbDeltas( int f )
{
   return ( f == SWAK_BP ) ? NB_OF(swakDelta) : 1;
}

/* The magnitude of the row's pole, and its multiplicity. This is the whole
 * basis of every bound in this file: a second-order section's transient is
 * sum(c_j * k^(m_j-1) * pole_j^k), so after k bars nothing survives above
 * k^(m-1) * |pole|^k times the seed error. */
static void swakPoles( int f, int P, double delta, double *r, int *mult )
{
   double w = (2.0 * SWAK_PI) / (double)P;

   if( f == SWAK_HP )
   {
      double cw = cos(w);
      *r = fabs( 1.0 - (cw + sin(w) - 1.0) / cw );
      *mult = 1;
      return;
   }

   if( f == SWAK_BP )
   {
      double t = (4.0 * SWAK_PI * delta) / (double)P;
      /* The complex pair satisfies z^2 - a1*z - a2 = 0 with a2 = -abp, so the
       * product of the roots is abp and each has magnitude sqrt(abp). */
      *r = sqrt( (1.0 - sin(t)) / cos(t) );
      *mult = 1;
      return;
   }

   {
      double b2p = 2.415 * (1.0 - cos(w));
      *r = fabs( 1.0 - (-b2p + sqrt( b2p*b2p + 2.0*b2p )) );
      *mult = 2;
   }
}

static double swakEnvelope( int f, int P, double delta, int k )
{
   double r;
   int m;

   swakPoles( f, P, delta, &r, &m );
   return ( m == 2 ? (double)k : 1.0 ) * pow( r, (double)k );
}

static TA_RetCode swakCall( int f, int startIdx, int endIdx, const double *in,
                            int P, double delta, int *outBegIdx, int *outNbElement,
                            double *out )
{
   switch( f )
   {
   case SWAK_GAUSS:
      return TA_SWAK_GAUSS( startIdx, endIdx, in, P, outBegIdx, outNbElement, out );
   case SWAK_BUTTER:
      return TA_SWAK_BUTTER( startIdx, endIdx, in, P, outBegIdx, outNbElement, out );
   case SWAK_HP:
      return TA_SWAK_HP( startIdx, endIdx, in, P, outBegIdx, outNbElement, out );
   case SWAK_2PHP:
      return TA_SWAK_2PHP( startIdx, endIdx, in, P, outBegIdx, outNbElement, out );
   default:
      return TA_SWAK_BP( startIdx, endIdx, in, P, delta, outBegIdx, outNbElement, out );
   }
}

static int swakLookback( int f, int P, double delta )
{
   switch( f )
   {
   case SWAK_GAUSS:  return TA_SWAK_GAUSS_Lookback( P );
   case SWAK_BUTTER: return TA_SWAK_BUTTER_Lookback( P );
   case SWAK_HP:     return TA_SWAK_HP_Lookback( P );
   case SWAK_2PHP:   return TA_SWAK_2PHP_Lookback( P );
   default:          return TA_SWAK_BP_Lookback( P, delta );
   }
}

/* Replay one call on every live language server. The caller has already made
 * the C call over [0, n-1] with the arrays below. */
static ErrorNumber swakRoute( int f, const char *tag, int n, int P, double delta,
                              TA_RetCode rc, int begIdx, int nbElement )
{
   double optIn[2];
   ErrorNumber e;
   int cmpBefore;
   int nbOpt = ( f == SWAK_BP ) ? 2 : 1;

   if( !server_verify_active() )
      return TA_TEST_PASS;

   optIn[0] = (double)P;
   optIn[1] = delta;

   cmpBefore = server_verify_comparisons();
   e = server_verify( swakName[f], 0, n-1, n, rc, begIdx, nbElement,
                      (const TA_Real*[]){ swakIn, NULL }, optIn, nbOpt,
                      (const TA_Real*[]){ swakOut, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_comparisons() == cmpBefore )
   {
      printf( "%s %s [P=%d]: compared no server despite live pipes\n",
              swakName[f], tag, P );
      return TA_SV_ROUTED_VACUOUS;
   }
   g_swakRouteCmp++;
   return TA_TEST_PASS;
}

/* (1) A constant input. The seed IS the steady state of a constant, so there is
 * no transient here and the only error is the conditioning of the coefficient
 * arithmetic itself. */
static ErrorNumber test_swak_dc( void )
{
   int f, pi, di, li, i, n, begIdx, nbElement;
   TA_RetCode rc;

   for( f = 0; f < SWAK_NB_FUNC; f++ )
   for( pi = 0; pi < swakNbPeriods(f); pi++ )
   for( di = 0; di < swakNbDeltas(f); di++ )
   for( li = 0; li < NB_OF(swakLevel); li++ )
   {
      int P = swakPeriod( f, pi );
      double L = swakLevel[li];
      double delta = swakDelta[di];

      n = 4000;
      if( n < 30*P ) n = 30*P;
      if( n > SWAK_N_MAX ) n = SWAK_N_MAX;

      for( i = 0; i < n; i++ )
         swakIn[i] = L;

      rc = swakCall( f, 0, n-1, swakIn, P, delta, &begIdx, &nbElement, swakOut );
      if( rc != TA_SUCCESS || nbElement != n )
      {
         printf( "%s DC Fail [P=%d L=%g]: rc=%d nb=%d expected %d\n",
                 swakName[f], P, L, (int)rc, nbElement, n );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      if( swakDcUnity[f] )
      {
         /* The row's own conditioning and nothing else. The coefficients are
          * formed from cos(2*pi/P), which loses relative precision as 1/P^2 as
          * the cutoff lengthens -- so the bound carries that factor rather than
          * being a flat number that would be slack at P=20 and wrong at P=10000.
          */
         double bound = 2e-15 * ( (P > 20) ? ((double)P/20.0)*((double)P/20.0) : 1.0 );

         for( i = 0; i < nbElement; i++ )
         {
            double r = fabs( swakOut[i] - L ) / L;
            if( r > bound )
            {
               printf( "%s DC Fail [P=%d L=%g] at bar %d: %.17g, relative %.3e > %.3e\n",
                       swakName[f], P, L, begIdx+i, swakOut[i], r, bound );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_swakDcCmp++;
         }
      }
      else
      {
         for( i = 0; i < nbElement; i++ )
         {
            /* Exactly zero, and POSITIVE zero. A -0.0 here compares equal to
             * 0.0, so the value check alone cannot see it; see the header for
             * what can produce one and what cannot. */
            if( swakOut[i] != 0.0 || signbit( swakOut[i] ) )
            {
               printf( "%s DC Fail [P=%d L=%g] at bar %d: %.17g, expected +0\n",
                       swakName[f], P, L, begIdx+i, swakOut[i] );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_swakDcCmp++;
         }
      }
   }

   return TA_TEST_PASS;
}

/* (2) An alternating +/-1: the Nyquist frequency. */
static ErrorNumber test_swak_nyquist( void )
{
   int f, pi, di, i, n, k0, begIdx, nbElement;
   TA_RetCode rc;
   ErrorNumber e;

   n = SWAK_N_NYQ;

   for( f = 0; f < SWAK_NB_FUNC; f++ )
   for( pi = 0; pi < swakNbPeriods(f); pi++ )
   for( di = 0; di < swakNbDeltas(f); di++ )
   {
      int P = swakPeriod( f, pi );
      double delta = swakDelta[di];
      double worst = 0.0, env, bound;

      for( i = 0; i < n; i++ )
         swakIn[i] = (i & 1) ? -1.0 : 1.0;

      rc = swakCall( f, 0, n-1, swakIn, P, delta, &begIdx, &nbElement, swakOut );
      if( rc != TA_SUCCESS || nbElement != n )
      {
         printf( "%s Nyquist Fail [P=%d]: rc=%d nb=%d expected %d\n",
                 swakName[f], P, (int)rc, nbElement, n );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      /* Measure over the back half only: the front half is the transient this
       * leg is not about. */
      k0 = nbElement / 2;
      for( i = k0; i < nbElement; i++ )
      {
         double want = ( swakNyquist[f] == 1 ) ? ( (i & 1) ? -1.0 : 1.0 ) : 0.0;
         double d = fabs( swakOut[i] - want );
         if( d > worst ) worst = d;
      }

      if( swakNyquist[f] == -1 )
      {
         /* The control arm. GAUSS's numerator is (1,0,0): it has no zero
          * anywhere, so whatever it does to this input it may not be to erase
          * it. A row returning 0 everywhere passes every OTHER case in this
          * leg; this is the one it cannot. */
         if( worst <= 1e-12 )
         {
            printf( "%s Nyquist Fail [P=%d]: annihilated an input it has no zero for\n",
                    swakName[f], P );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_swakNyqCmp++;
         continue;
      }

      env = swakEnvelope( f, P, delta, k0 );
      bound = 3.0*env + 1e-12;
      if( worst > bound )
      {
         printf( "%s Nyquist Fail [P=%d delta=%g]: worst %.3e > 3*envelope %.3e\n",
                 swakName[f], P, delta, worst, env );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakNyqCmp += nbElement - k0;

      /* Route the first period of each row: a shape the generic sweep never
       * sends, at a length the transport can carry. */
      if( pi == 0 && di == 0 )
      {
         for( i = 0; i < SWAK_N_ROUTE; i++ )
            swakIn[i] = (i & 1) ? -1.0 : 1.0;
         rc = swakCall( f, 0, SWAK_N_ROUTE-1, swakIn, P, delta,
                        &begIdx, &nbElement, swakOut );
         if( rc != TA_SUCCESS )
         {
            printf( "%s Nyquist route Fail [P=%d]: rc=%d\n", swakName[f], P, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         e = swakRoute( f, "nyquist", SWAK_N_ROUTE, P, delta, rc, begIdx, nbElement );
         if( e != TA_TEST_PASS )
            return e;

         for( i = 0; i < SWAK_N_ROUTE; i++ )
            swakIn[i] = swakLevel[1];
         rc = swakCall( f, 0, SWAK_N_ROUTE-1, swakIn, P, delta,
                        &begIdx, &nbElement, swakOut );
         if( rc != TA_SUCCESS )
         {
            printf( "%s DC route Fail [P=%d]: rc=%d\n", swakName[f], P, (int)rc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         e = swakRoute( f, "dc", SWAK_N_ROUTE, P, delta, rc, begIdx, nbElement );
         if( e != TA_TEST_PASS )
            return e;
      }
   }

   return TA_TEST_PASS;
}

/* (3) BP at its own centre period returns the input untouched. The DC and
 * Nyquist legs cannot see a1 at all -- the numerator is zero at both ends
 * whatever the feedback is -- so this is the leg that pins it. */
static ErrorNumber test_swak_centre( void )
{
   int pi, di, i, n, k0, begIdx, nbElement;
   TA_RetCode rc;

   n = SWAK_N_MAX;

   for( pi = 0; pi < NB_OF(swakPerBp); pi++ )
   for( di = 0; di < NB_OF(swakDelta); di++ )
   {
      int P = swakPerBp[pi];
      double delta = swakDelta[di];
      double worst = 0.0, env, bound;

      for( i = 0; i < n; i++ )
         swakIn[i] = cos( 2.0 * SWAK_PI * (double)i / (double)P );

      rc = TA_SWAK_BP( 0, n-1, swakIn, P, delta, &begIdx, &nbElement, swakOut );
      if( rc != TA_SUCCESS || nbElement != n )
      {
         printf( "SWAK_BP centre Fail [P=%d delta=%g]: rc=%d nb=%d\n",
                 P, delta, (int)rc, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      k0 = nbElement * 3 / 4;
      for( i = k0; i < nbElement; i++ )
      {
         double d = fabs( swakOut[i] - swakIn[i] );
         if( d > worst ) worst = d;
      }

      env = swakEnvelope( SWAK_BP, P, delta, k0 );
      /* The additive floor is the PROBE's own precision, measured, not a
       * tolerance on the filter. Two sources, both independent of what is under
       * test: cos(2*pi*i/P) at i = 60000 carries an argument up to 75398 (P=5),
       * whose libm reduction loses |arg|*eps ~ 8.3e-12; and 60000 recursive
       * steps accumulate rounding, which dominates at large P where the
       * argument is small (P=2000: 2.1e-14 predicted from reduction, 3.8e-12
       * measured). 1e-10 clears both with margin and still leaves the gate able
       * to see a wrong a1 -- the band-pass form this row was first written with
       * misses by order 1. */
      bound = 3.0*env + 1e-10;
      if( worst > bound )
      {
         printf( "SWAK_BP centre Fail [P=%d delta=%g]: worst %.3e > %.3e "
                 "(3*envelope %.3e + probe floor 1e-10)\n",
                 P, delta, worst, bound, env );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakCentreCmp += nbElement - k0;
   }

   return TA_TEST_PASS;
}

/* (4) The lookback is the unstable period and nothing else. None of these rows
 * reads a bar before its first: every slot is seeded from it. */
static ErrorNumber test_swak_lookback( void )
{
   static const int starts[] = { 0, 60 };
   int f, pi, ui, is, n, begIdx, nbElement;
   TA_RetCode rc;

   n = 400;
   for( f = 0; f < SWAK_NB_FUNC; f++ )
   for( ui = 0; ui < NB_OF(swakUnstValues); ui++ )
   {
      unsigned int u = swakUnstValues[ui];

      TA_SetUnstablePeriod( swakUnstId[f], u );

      for( pi = 0; pi < swakNbPeriods(f); pi++ )
      {
         int P = swakPeriod( f, pi );
         int lb = swakLookback( f, P, swakDelta[0] );
         int i;

         if( lb != (int)u )
         {
            printf( "%s lookback Fail [P=%d unst %u]: %d\n", swakName[f], P, u, lb );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_swakLookbackCmp++;

         if( pi != 0 )
            continue;

         for( i = 0; i < n; i++ )
            swakIn[i] = 100.0 + (double)(i % 17) - (double)(i % 5);

         for( is = 0; is < NB_OF(starts); is++ )
         {
            int want = ( starts[is] < lb ) ? lb : starts[is];

            rc = swakCall( f, starts[is], n-1, swakIn, P, swakDelta[0],
                           &begIdx, &nbElement, swakOut );
            if( rc != TA_SUCCESS || begIdx != want || nbElement != n - want )
            {
               printf( "%s lookback Fail [P=%d unst %u start %d]: rc=%d (%d,%d) "
                       "expected (%d,%d)\n", swakName[f], P, u, starts[is], (int)rc,
                       begIdx, nbElement, want, n - want );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
            g_swakLookbackCmp++;
         }
      }

      TA_SetUnstablePeriod( swakUnstId[f], 0 );
   }

   return TA_TEST_PASS;
}

/* (5) outReal may alias inReal: every bar is read into a local before its slot
 * is written, and the earlier bars the recurrence needs are carried in locals
 * rather than re-read. */
static ErrorNumber test_swak_inplace( void )
{
   static const int starts[] = { 0, 300 };
   int f, pi, di, is, i, n, b1, n1, b2, n2;
   TA_RetCode rc1, rc2;

   n = 2000;
   for( i = 0; i < n; i++ )
      swakIn[i] = 100.0 + (double)(i % 23) * 0.5 - (double)(i % 7);

   for( f = 0; f < SWAK_NB_FUNC; f++ )
   for( pi = 0; pi < swakNbPeriods(f); pi++ )
   for( di = 0; di < swakNbDeltas(f); di++ )
   for( is = 0; is < NB_OF(starts); is++ )
   {
      int P = swakPeriod( f, pi );
      double delta = swakDelta[di];

      memcpy( swakAlias, swakIn, (size_t)n * sizeof(double) );

      rc1 = swakCall( f, starts[is], n-1, swakIn, P, delta, &b1, &n1, swakOut );
      rc2 = swakCall( f, starts[is], n-1, swakAlias, P, delta, &b2, &n2, swakAlias );

      if( rc1 != TA_SUCCESS || rc2 != TA_SUCCESS || b1 != b2 || n1 != n2 || n1 <= 0
          || memcmp( swakOut, swakAlias, (size_t)n1 * sizeof(double) ) != 0 )
      {
         printf( "%s in-place Fail [P=%d delta=%g start %d]\n",
                 swakName[f], P, delta, starts[is] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakInplaceCmp += n1;
   }

   return TA_TEST_PASS;
}

/* (6) The generic range sweep, four rows of five.
 *
 * BP is absent on purpose, not by omission: #486 is open on how its range
 * dependence should be classified, and the --codegen sweep fails it
 * deterministically under TA_STABLE_CONVERGING while the other four pass at
 * every seed. A leg written now would be a guess at the answer. */
typedef struct { const TA_Real *in; int f; } SwakRangeParam;

static TA_RetCode swakRangeTestFunction( TA_Integer startIdx, TA_Integer endIdx,
                                         TA_Real *outputBuffer, TA_Integer *outputBufferInt,
                                         TA_Integer *outBegIdx, TA_Integer *outNbElement,
                                         TA_Integer *lookback, void *opaqueData,
                                         unsigned int outputNb, unsigned int *isOutputInteger )
{
   SwakRangeParam *p = (SwakRangeParam *)opaqueData;

   (void)outputNb;
   (void)outputBufferInt;
   *isOutputInteger = 0;

   *lookback = swakLookback( p->f, 20, 0.1 );
   return swakCall( p->f, startIdx, endIdx, p->in, 20, 0.1,
                    outBegIdx, outNbElement, outputBuffer );
}

static ErrorNumber test_swak_range( const TA_Real *in )
{
   SwakRangeParam param;
   ErrorNumber err = TA_TEST_PASS;
   int f;

   param.in = in;
   for( f = 0; f < SWAK_NB_FUNC && err == TA_TEST_PASS; f++ )
   {
      if( f == SWAK_BP )
         continue;
      param.f = f;
      err = doRangeTestEx( swakRangeTestFunction, TA_STABLE_CONVERGING,
                           swakUnstId[f], (void *)&param, 1, 0 );
   }

   return err;
}
