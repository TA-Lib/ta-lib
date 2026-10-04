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
 *  100326 MF,CC  Frozen goldens, range edges, LEAN and eSignal bars (#486).
 */

/* Description:
 *
 *   Test the five Swiss Army Knife rows: TA_SWAK_GAUSS, TA_SWAK_BUTTER,
 *   TA_SWAK_HP, TA_SWAK_2PHP and TA_SWAK_BP.
 *
 *   The five are one second-order IIR
 *
 *       y[i] = c0*(b0*x[i] + b1*x[i-1] + b2*x[i-2]) + a1*y[i-1] + a2*y[i-2]
 *
 *   under five coefficient rows. Legs 1 to 3 assert what a row does to a
 *   constant, to a Nyquist alternation and (band-pass) to a sinusoid at its
 *   centre period. The transient bounds of legs 2 and 3 are the envelope of
 *   that row's own poles, because the seed's transient decays at the row's
 *   rate and a fixed threshold is wrong at one end of the period range or the
 *   other:
 *
 *       GAUSS / BUTTER / 2PHP   double real pole om = 1-a2p    k * om^k
 *       HP                      single real pole 1-a1p         p^k
 *       BP                      complex pair, |pole| = sqrt(abp)   r^k
 *
 *   Legs 7 and 9 hold frozen values at fixed tolerances.
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
 *        phase. Of the analytic legs only this one pins a1, which the DC and
 *        Nyquist legs are both blind to (the numerator vanishes there whatever
 *        a1 is).
 *     4. LOOKBACK is the unstable period and nothing else, under several
 *        settings of it, per row.
 *     5. ALIASING: outReal == inReal.
 *     6. The generic start/end range sweep, all five rows.
 *     7. GOLDEN: frozen values from a 60-digit evaluation of the same rows and
 *        seed. The early bars are the only ones that see the seed, and the
 *        BP rows at its period cap are the only gate that tells its abp from
 *        the paper's gamma - sqrt(gamma^2 - 1).
 *     8. EDGES: the first period and delta outside each declared range are
 *        refused.
 *     9. ORACLE: frozen bars from two other implementations, each taken where
 *        its different start has decayed below the tolerance. QuantConnect
 *        LEAN's SwissArmyKnife for GAUSS and BUTTER, and for HP and 2PHP times
 *        the ratio of the two c0 (LEAN's is (1+a)/2 and (1+a)^2/4). The 2006
 *        eSignal port, Swak.efs, for HP, 2PHP and BP: the one independent
 *        implementation of the band-pass row.
 *
 *   SERVER_VERIFY: the constant and Nyquist shapes are routed. The --codegen
 *   sweep sends the 252-bar corpus with one parameter moved at a time and
 *   reaches neither shape, and both are exactly where a coefficient
 *   transcription error in another language shows up as a whole number rather
 *   than a last-bit difference. The golden series is routed too, so the other
 *   languages are held to the frozen bars' input as well.
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

#define SWAK_N_MAX   60000
#define SWAK_N_NYQ   20000
#define SWAK_N_ROUTE  2000

#define NB_OF(a) ((int)(sizeof(a)/sizeof((a)[0])))

/* Pinned coverage counts. Every input below is synthesized and every count is a
 * loop trip decided by a period, a delta and a length in basic arithmetic. */
#define SWAK_DC_CMP       1878000
#define SWAK_NYQ_CMP       210004
#define SWAK_CENTRE_CMP    180000
#define SWAK_LOOKBACK_CMP      132
#define SWAK_INPLACE_CMP   114700
#define SWAK_ROUTE_CMP         15
#define SWAK_GOLDEN_CMP         49
#define SWAK_EDGE_CMP           16
#define SWAK_ORACLE_CMP         28

static double swakIn[SWAK_N_MAX];
static double swakOut[SWAK_N_MAX];
static double swakAlias[SWAK_N_MAX];

static int g_swakDcCmp;
static int g_swakNyqCmp;
static int g_swakCentreCmp;
static int g_swakLookbackCmp;
static int g_swakInplaceCmp;
static int g_swakRouteCmp;
static int g_swakGoldenCmp;
static int g_swakEdgeCmp;
static int g_swakOracleCmp;

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
static ErrorNumber  test_swak_golden( void );
static ErrorNumber  test_swak_edges( void );
static ErrorNumber  test_swak_oracle( void );

/**** Global functions definitions. ****/
ErrorNumber test_func_swak( TA_History *history )
{
   ErrorNumber err;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   g_swakDcCmp = g_swakNyqCmp = g_swakCentreCmp = 0;
   g_swakLookbackCmp = g_swakInplaceCmp = g_swakRouteCmp = 0;
   g_swakGoldenCmp = g_swakEdgeCmp = g_swakOracleCmp = 0;

   err = test_swak_dc();
   if( err == TA_TEST_PASS )
      err = test_swak_nyquist();
   if( err == TA_TEST_PASS )
      err = test_swak_centre();
   if( err == TA_TEST_PASS )
      err = test_swak_lookback();
   if( err == TA_TEST_PASS )
      err = test_swak_inplace();
   if( err == TA_TEST_PASS )
      err = test_swak_golden();
   if( err == TA_TEST_PASS )
      err = test_swak_edges();
   if( err == TA_TEST_PASS )
      err = test_swak_oracle();

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS )
      err = test_swak_range( history->close );

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   if( err == TA_TEST_PASS
       && ( g_swakDcCmp != SWAK_DC_CMP || g_swakNyqCmp != SWAK_NYQ_CMP
            || g_swakCentreCmp != SWAK_CENTRE_CMP
            || g_swakLookbackCmp != SWAK_LOOKBACK_CMP
            || g_swakInplaceCmp != SWAK_INPLACE_CMP
            || g_swakGoldenCmp != SWAK_GOLDEN_CMP
            || g_swakEdgeCmp != SWAK_EDGE_CMP
            || g_swakOracleCmp != SWAK_ORACLE_CMP
            || ( server_verify_active() && g_swakRouteCmp != SWAK_ROUTE_CMP ) ) )
   {
      printf( "SWAK Fail: coverage counters (dc %d, nyquist %d, centre %d, lookback %d, "
              "inplace %d, routed %d, golden %d, edges %d, oracle %d) are not what this "
              "file was written with (%d, %d, %d, %d, %d, %d, %d, %d, %d)\n",
              g_swakDcCmp, g_swakNyqCmp, g_swakCentreCmp, g_swakLookbackCmp,
              g_swakInplaceCmp, g_swakRouteCmp, g_swakGoldenCmp, g_swakEdgeCmp, g_swakOracleCmp,
              SWAK_DC_CMP, SWAK_NYQ_CMP, SWAK_CENTRE_CMP, SWAK_LOOKBACK_CMP,
              SWAK_INPLACE_CMP, SWAK_ROUTE_CMP, SWAK_GOLDEN_CMP, SWAK_EDGE_CMP,
              SWAK_ORACLE_CMP );
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

      /* Route the first period of EVERY row, so before the exits below: the
       * routed floor is only asserted when a pipe is up, so a row that routes
       * nothing is invisible to a run without servers. */
      if( pi == 0 && di == 0 )
      {
         /* Its own shape locals: writing the measured call's begIdx/nbElement
          * here would silently shrink the count the leg asserts below. */
         TA_RetCode rrc;
         int rb, rn;

         for( i = 0; i < SWAK_N_ROUTE; i++ )
            swakIn[i] = (i & 1) ? -1.0 : 1.0;
         rrc = swakCall( f, 0, SWAK_N_ROUTE-1, swakIn, P, delta, &rb, &rn, swakOut );
         if( rrc != TA_SUCCESS )
         {
            printf( "%s Nyquist route Fail [P=%d]: rc=%d\n", swakName[f], P, (int)rrc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         e = swakRoute( f, "nyquist", SWAK_N_ROUTE, P, delta, rrc, rb, rn );
         if( e != TA_TEST_PASS )
            return e;

         for( i = 0; i < SWAK_N_ROUTE; i++ )
            swakIn[i] = swakLevel[1];
         rrc = swakCall( f, 0, SWAK_N_ROUTE-1, swakIn, P, delta, &rb, &rn, swakOut );
         if( rrc != TA_SUCCESS )
         {
            printf( "%s DC route Fail [P=%d]: rc=%d\n", swakName[f], P, (int)rrc );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         e = swakRoute( f, "dc", SWAK_N_ROUTE, P, delta, rrc, rb, rn );
         if( e != TA_TEST_PASS )
            return e;

         /* Restore the measured series: the route calls overwrote it. */
         for( i = 0; i < n; i++ )
            swakIn[i] = (i & 1) ? -1.0 : 1.0;
      }

      /* A cell whose transient outlasts the k0 settled bars cannot fail, so it
       * is not compared and not counted; that holds for the control arm too,
       * whose residue there is all transient. Kept when two periods fit in
       * k0, over delta for BP, whose band narrows with it. Decided in basic
       * arithmetic so that the pinned count is the same on every host; the
       * envelope then proves each kept cell can fail. */
      if( 2.0*(double)P > (double)k0 * ( (f == SWAK_BP) ? delta : 1.0 ) )
         continue;
      env = swakEnvelope( f, P, delta, k0 );
      if( 3.0*env > 1e-3 )
      {
         printf( "%s Nyquist Fail [P=%d delta=%g]: kept a cell still in its transient\n",
                 swakName[f], P, delta );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
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

      bound = 3.0*env + 1e-12;
      if( worst > bound )
      {
         printf( "%s Nyquist Fail [P=%d delta=%g]: worst %.3e > 3*envelope %.3e\n",
                 swakName[f], P, delta, worst, env );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakNyqCmp += nbElement - k0;

   }

   return TA_TEST_PASS;
}

/* (3) BP at its own centre period returns the input untouched. */
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
       * measured). 1e-10 clears both with margin; a1 = beta*(1-abp) misses by
       * order 1. */
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

/* (6) The generic range sweep, all five rows under TA_STABLE_CONVERGING. */
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
      param.f = f;
      err = doRangeTestEx( swakRangeTestFunction, TA_STABLE_CONVERGING,
                           swakUnstId[f], (void *)&param, 1, 0 );
   }

   return err;
}

/* (7) Frozen values. The series is 100 + 10*sin(i/7) + 0.15*i + 2*sin(i/2.3);
 * every expected value is the same row and seed evaluated in 60-digit decimal
 * over the doubles of that series. A libm whose sin is an ULP off moves an
 * input near 290 by 6e-14, and no row's gain exceeds 1, so the 1e-12 absolute
 * and 1e-13 relative tolerances keep at least one order over it. */
#define SWAK_GOLDEN_N       1200
#define SWAK_GOLDEN_ROWS       9
#define SWAK_COND_N        20000
#define SWAK_COND_ROWS         4

static const int swakGoldenBar[SWAK_GOLDEN_ROWS] = { 0, 1, 2, 10, 19, 100, 500, 1000, 1199 };

/* P = 20, delta = 0.1. */
static const double swakGolden[SWAK_NB_FUNC][SWAK_GOLDEN_ROWS] =
{
   /* GAUSS  */ { 1.00000000000000000e+02, 1.00352885890122877e+02, 1.01114688108096715e+02,
                  1.08665021094407507e+02, 1.10303329168047640e+02, 1.22199881282870834e+02,
                  1.83801722817338145e+02, 2.41768353519822853e+02, 2.86858377267428125e+02 },
   /* BUTTER */ { 1.00000000000000000e+02, 1.00088221472530719e+02, 1.00455114972085610e+02,
                  1.08221391743158790e+02, 1.10490383391746562e+02, 1.21686709025691016e+02,
                  1.84239500074226981e+02, 2.41930430535751697e+02, 2.86109472174069936e+02 },
   /* HP     */ { 0.00000000000000000e+00, 2.08578750310559435e+00, 3.44083779157216885e+00,
                  8.99393518790790769e-01, -1.41232732329778310e+00, 1.78589982044724604e+00,
                  -2.35441575501536171e+00, 2.06178853125851512e-02, 2.57183745360370208e+00 },
   /* 2PHP   */ { 0.00000000000000000e+00, 1.58098936244228661e+00, 1.83201534137916155e+00,
                  -7.44889218080521887e-01, -1.13593692970976368e+00, 8.48822166360145292e-02,
                  -1.55074996769183682e+00, 8.97460181170890103e-01, 1.95163082787267517e-01 },
   /* BP     */ { 0.00000000000000000e+00, 7.36168740559686036e-02, 2.77334960845761869e-01,
                  9.25884431552123965e-01, -8.04144579561751893e-01, -5.31168357062935304e-01,
                  -2.25745496512508775e-01, -1.52703253764034375e-01, -4.23559364649652048e-01 }
};

/* SWAK_BP at its period cap and narrowest band: P = 2000, delta = 0.05. */
static const int    swakCondBar[SWAK_COND_ROWS] = { 1000, 5000, 10000, 19999 };
static const double swakCond[SWAK_COND_ROWS] =
   { 8.83578336954305676e+00, 6.94599135761044018e+00,
     3.80275530291380548e+00, 4.57574609786069786e+00 };

static void swakGoldenSeries( int n )
{
   int i;
   for( i = 0; i < n; i++ )
      swakIn[i] = 100.0 + 10.0*sin( (double)i/7.0 ) + 0.15*(double)i + 2.0*sin( (double)i/2.3 );
}

static ErrorNumber test_swak_golden( void )
{
   int f, k, begIdx, nbElement;
   TA_RetCode rc;
   ErrorNumber e;

   swakGoldenSeries( SWAK_GOLDEN_N );
   for( f = 0; f < SWAK_NB_FUNC; f++ )
   {
      rc = swakCall( f, 0, SWAK_GOLDEN_N-1, swakIn, 20, 0.1, &begIdx, &nbElement, swakOut );
      if( rc != TA_SUCCESS || begIdx != 0 || nbElement != SWAK_GOLDEN_N )
      {
         printf( "%s golden Fail: rc=%d (%d,%d)\n", swakName[f], (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( k = 0; k < SWAK_GOLDEN_ROWS; k++ )
      {
         double want = swakGolden[f][k];
         double got  = swakOut[swakGoldenBar[k]];
         /* The price-level rows are held relative, the zero-centred ones
          * absolute: their output crosses zero, where a relative bound means
          * nothing. */
         double tol  = swakDcUnity[f] ? 1e-13 * fabs( want ) : 1e-12;
         if( !( fabs( got - want ) <= tol ) )
         {
            printf( "%s golden Fail at bar %d: %.17g, expected %.17g\n",
                    swakName[f], swakGoldenBar[k], got, want );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_swakGoldenCmp++;
      }
      e = swakRoute( f, "golden", SWAK_GOLDEN_N, 20, 0.1, rc, begIdx, nbElement );
      if( e != TA_TEST_PASS )
         return e;
   }

   swakGoldenSeries( SWAK_COND_N );
   rc = TA_SWAK_BP( 0, SWAK_COND_N-1, swakIn, 2000, 0.05, &begIdx, &nbElement, swakOut );
   if( rc != TA_SUCCESS || begIdx != 0 || nbElement != SWAK_COND_N )
   {
      printf( "SWAK_BP conditioning Fail: rc=%d (%d,%d)\n", (int)rc, begIdx, nbElement );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( k = 0; k < SWAK_COND_ROWS; k++ )
   {
      if( !( fabs( swakOut[swakCondBar[k]] - swakCond[k] ) <= 1e-9 ) )
      {
         printf( "SWAK_BP conditioning Fail at bar %d: %.17g, expected %.17g\n",
                 swakCondBar[k], swakOut[swakCondBar[k]], swakCond[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakGoldenCmp++;
   }

   return TA_TEST_PASS;
}

/* (8) One step outside each declared range. HP's 4 is the period at which its
 * alpha rounds to 0 and the row turns into an integrator. */
static ErrorNumber test_swak_edges( void )
{
   static const struct { int f; int P; double delta; } bad[] =
   {
      { SWAK_GAUSS,  1, 0.1 }, { SWAK_GAUSS,  10001, 0.1 },
      { SWAK_BUTTER, 1, 0.1 }, { SWAK_BUTTER, 10001, 0.1 },
      { SWAK_2PHP,   1, 0.1 }, { SWAK_2PHP,   10001, 0.1 },
      { SWAK_HP,     4, 0.1 }, { SWAK_HP,    100001, 0.1 },
      { SWAK_BP,     4, 0.1 }, { SWAK_BP,      2001, 0.1 },
      { SWAK_BP,    20, 0.049 }, { SWAK_BP,    20, 0.501 }
   };
   static const struct { int f; int P; double delta; } good[] =
   {
      { SWAK_HP, 5, 0.1 }, { SWAK_BP, 5, 0.5 }, { SWAK_BP, 2000, 0.05 }, { SWAK_GAUSS, 2, 0.1 }
   };
   int k, i, begIdx, nbElement;
   TA_RetCode rc;

   swakGoldenSeries( 400 );

   for( k = 0; k < NB_OF(bad); k++ )
   {
      rc = swakCall( bad[k].f, 0, 399, swakIn, bad[k].P, bad[k].delta,
                     &begIdx, &nbElement, swakOut );
      if( rc != TA_BAD_PARAM )
      {
         printf( "%s edge Fail [P=%d delta=%g]: rc=%d, expected TA_BAD_PARAM\n",
                 swakName[bad[k].f], bad[k].P, bad[k].delta, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakEdgeCmp++;
   }

   for( k = 0; k < NB_OF(good); k++ )
   {
      rc = swakCall( good[k].f, 0, 399, swakIn, good[k].P, good[k].delta,
                     &begIdx, &nbElement, swakOut );
      if( rc != TA_SUCCESS || nbElement != 400 )
      {
         printf( "%s edge Fail [P=%d delta=%g]: rc=%d nb=%d\n",
                 swakName[good[k].f], good[k].P, good[k].delta, (int)rc, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = 0; i < nbElement; i++ )
      {
         /* Every row here is stable, so nothing it returns on this series can
          * leave the series' own scale. */
         if( !( fabs( swakOut[i] ) < 1000.0 ) )
         {
            printf( "%s edge Fail [P=%d delta=%g] at bar %d: %.17g\n",
                    swakName[good[k].f], good[k].P, good[k].delta, i, swakOut[i] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_swakEdgeCmp++;
   }

   return TA_TEST_PASS;
}

/* (9) Bars frozen from other implementations, over 3000 bars of the golden
 * series. Bars 1500 and 2999 only: each oracle starts differently (LEAN's
 * output slots at zero, eSignal's input slots at zero and its band-pass at
 * zero for four bars), and these are past where any of that is above the
 * tolerance. LEAN returns a decimal, so its rows carry 15 digits. */
#define SWAK_ORACLE_N    3000
#define SWAK_ARM_LEAN       0
#define SWAK_ARM_LEAN_K     1
#define SWAK_ARM_ESIGNAL    2

static ErrorNumber test_swak_oracle( void )
{
   static const char * const armName[] = { "LEAN", "LEAN x K", "eSignal" };
   static const struct { int arm; int f; int P; double delta; int bar; double want; } row[] =
   {
      { SWAK_ARM_LEAN,   SWAK_GAUSS,   5, 0.1, 1500, 3.28562394307659986e+02 },
      { SWAK_ARM_LEAN,   SWAK_GAUSS,   5, 0.1, 2999, 5.58738384412170035e+02 },
      { SWAK_ARM_LEAN,   SWAK_GAUSS,  40, 0.1, 1500, 3.21864541823054992e+02 },
      { SWAK_ARM_LEAN,   SWAK_GAUSS,  40, 0.1, 2999, 5.51066129489068999e+02 },
      { SWAK_ARM_LEAN,   SWAK_BUTTER,  5, 0.1, 1500, 3.27340272925321017e+02 },
      { SWAK_ARM_LEAN,   SWAK_BUTTER,  5, 0.1, 2999, 5.58567166099751944e+02 },
      { SWAK_ARM_LEAN,   SWAK_BUTTER, 40, 0.1, 1500, 3.20929525882366022e+02 },
      { SWAK_ARM_LEAN,   SWAK_BUTTER, 40, 0.1, 2999, 5.49803543418992035e+02 },
      { SWAK_ARM_LEAN_K, SWAK_HP,      5, 0.1, 1500, 9.32993668517707153e-01 },
      { SWAK_ARM_LEAN_K, SWAK_HP,      5, 0.1, 2999, -5.29813452315126398e-03 },
      { SWAK_ARM_LEAN_K, SWAK_HP,     40, 0.1, 1500, 6.14218338347589210e+00 },
      { SWAK_ARM_LEAN_K, SWAK_HP,     40, 0.1, 2999, 6.16192965396030079e+00 },
      { SWAK_ARM_LEAN_K, SWAK_2PHP,    5, 0.1, 1500, 1.34669354356382387e-01 },
      { SWAK_ARM_LEAN_K, SWAK_2PHP,    5, 0.1, 2999, -1.80836061609586624e-01 },
      { SWAK_ARM_LEAN_K, SWAK_2PHP,   40, 0.1, 1500, 6.15801805003232450e-01 },
      { SWAK_ARM_LEAN_K, SWAK_2PHP,   40, 0.1, 2999, -1.67615004713303128e+00 },
      { SWAK_ARM_ESIGNAL, SWAK_HP,     10, 0.1, 1500, 1.83979255443937362e+00 },
      { SWAK_ARM_ESIGNAL, SWAK_HP,     10, 0.1, 2999, 4.57706339989101429e-01 },
      { SWAK_ARM_ESIGNAL, SWAK_2PHP,   10, 0.1, 1500, 2.42726180103395694e-01 },
      { SWAK_ARM_ESIGNAL, SWAK_2PHP,   10, 0.1, 2999, -4.92711094032510732e-01 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,     20, 0.1, 1500, 7.60910708736650454e-01 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,     20, 0.1, 2999, 1.17927731098047217e+00 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,     20, 0.3, 1500, 2.25254371809556897e+00 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,     20, 0.3, 2999, 3.33190903522994741e+00 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,     10, 0.1, 1500, 4.70839018433414913e-01 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,     10, 0.1, 2999, -2.55166994385333246e-01 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,      5, 0.5, 1500, 1.30839332619827142e+00 },
      { SWAK_ARM_ESIGNAL, SWAK_BP,      5, 0.5, 2999, 7.77053521872017872e-02 }
   };
   int k, begIdx, nbElement;
   TA_RetCode rc;

   swakGoldenSeries( SWAK_ORACLE_N );

   for( k = 0; k < NB_OF(row); k++ )
   {
      double got, tol;

      rc = swakCall( row[k].f, 0, SWAK_ORACLE_N-1, swakIn, row[k].P, row[k].delta,
                     &begIdx, &nbElement, swakOut );
      if( rc != TA_SUCCESS || begIdx != 0 || nbElement != SWAK_ORACLE_N )
      {
         printf( "%s oracle Fail [P=%d]: rc=%d (%d,%d)\n", swakName[row[k].f], row[k].P,
                 (int)rc, begIdx, nbElement );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      got = swakOut[row[k].bar];
      tol = swakDcUnity[row[k].f] ? 1e-13 * fabs( row[k].want ) : 1e-12;
      if( !( fabs( got - row[k].want ) <= tol ) )
      {
         printf( "%s oracle Fail [%s P=%d delta=%g] at bar %d: %.17g, expected %.17g\n",
                 swakName[row[k].f], armName[row[k].arm], row[k].P, row[k].delta,
                 row[k].bar, got, row[k].want );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_swakOracleCmp++;
   }

   return TA_TEST_PASS;
}
