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
 *  092726 MF,CC  First version (#451).
 */

/* Description:
 *
 *   Test TA_SI (Wilder Swing Index) and TA_ASI (its running total).
 *
 *   Legs:
 *     1. Wilder's book: the two worked examples and the rows of his Daily
 *        Work Sheet (New Concepts in Technical Trading Systems, 1978,
 *        pp.91-93 and 105-106, limit 3.00). He rounds every SI to a whole
 *        number and accumulates the rounded values, so the sheet holds SI
 *        within 1.0 and ASI within 5.0; days 23 and 28 contradict their own
 *        columns and are skipped for SI.
 *     2. trading-signals 8.3.0, bit for bit, at chosen bars of the 252-bar
 *        reference series. The only leg that sees the operation order.
 *     3. ASI against CUMSUM over SI, bit for bit, for startIdx > 0 too: at 0
 *        SI's clamp makes TA_SI(0, e) and TA_SI(1, e) one call.
 *     4. Degenerate bars, sign of zero included.
 *     5. The limit move: range, default, pure scale.
 *     6. Lookback and output range.
 *     7. ASI earns path_dependent; SI does not claim it.
 *     8. outReal aliasing each input, ASI's anchor bar at startIdx 0 included.
 *     9. Stream against batch, a stream opened mid-series included.
 *
 *   SERVER_VERIFY: the sheet, the book examples, the degenerate vectors and
 *   the 252-bar series at limits 3 and 8.
 */

/**** Headers ****/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

#define SHEET_NB 47
#define MAX_NB   256

/* (O, H, L, C, sheet SI, sheet ASI); day 1 carries no SI. */
static const double sheet[SHEET_NB][6] = {
   {40.50,42.00,40.00,41.50,   0,  0}, {42.00,44.00,42.00,43.00,  37, 37},
   {42.80,43.50,41.50,42.00, -13, 24}, {41.70,43.00,41.70,42.90,  15, 39},
   {43.00,44.00,42.30,43.50,  11, 50}, {44.50,46.00,44.00,45.80,  54,104},
   {44.80,45.00,43.00,43.50, -45, 59}, {43.00,44.80,43.00,44.50,  15, 74},
   {44.70,45.70,44.50,45.00,  13, 87}, {45.00,46.00,44.90,46.00,  22,109},
   {45.80,47.50,45.50,47.20,  24,133}, {47.00,47.50,45.80,46.00, -16,117},
   {46.20,46.20,45.00,45.50, -13,104}, {45.80,47.70,45.50,47.50,  41,145},
   {48.50,50.00,48.40,49.80,  57,202}, {50.00,52.80,50.00,52.80,  73,275},
   {55.80,55.80,55.80,55.80,  84,359}, {58.80,58.80,58.80,58.80, 100,459},
   {61.80,61.80,59.00,59.50,  -8,451}, {60.00,60.00,56.50,57.00, -56,395},
   {57.50,58.00,55.00,55.00, -36,359}, {54.00,57.00,54.00,56.50,  20,379},
   {57.00,57.50,54.70,54.80, -20,359}, {54.50,55.50,54.00,55.00,  -1,358},
   {54.50,55.00,53.00,54.00, -18,340}, {54.00,55.00,54.00,54.50,   9,349},
   {55.00,55.00,53.00,53.20, -25,324}, {53.80,56.80,53.80,56.00,  65,379},
   {56.50,59.00,56.00,59.00,  68,447}, {62.00,62.00,59.00,59.20,  -8,439},
   {59.50,61.50,59.00,60.00,   4,443}, {59.50,59.80,58.50,59.00, -19,424},
   {58.50,59.00,57.00,57.20, -41,383}, {57.00,58.50,56.50,57.50,   2,385},
   {57.00,57.50,55.00,55.00, -53,332}, {54.00,54.00,52.00,52.00, -75,257},
   {50.00,52.50,50.00,52.00,   6,263}, {52.00,55.00,51.00,54.50,  47,310},
   {54.00,54.50,52.00,52.50, -28,282}, {53.00,54.20,53.00,53.20,   7,289},
   {53.50,53.80,52.00,52.50, -13,276}, {52.00,54.00,51.80,53.80,  20,296},
   {54.00,54.00,51.00,51.50, -42,254}, {51.00,52.50,50.00,52.50,   9,263},
   {52.00,52.00,49.00,51.00, -27,236}, {51.00,53.00,49.80,52.80,  24,260},
   {52.40,54.20,52.00,54.00,  22,282}
};

/* Exact p.90 values of chosen sheet days, one per R case and the (1)/(2)
 * tie of day 18, from an exact rational evaluation of the double inputs. */
static const struct { int day; double si; } sheetExact[] = {
   {  2,  37.5 },
   {  3, -12.777777777777763 },
   {  7, -44.954128440366894 },
   { 18, 100.0 },
   { 20, -56.13496932515337 },
   { 36, -75.0 },
   { 44,   9.0 },
   { 47,  21.572327044025254 }
};

typedef struct
{
   double o[MAX_NB], h[MAX_NB], l[MAX_NB], c[MAX_NB];
   int nb;
} Bars;

static int sameBits( double a, double b )
{
   return memcmp( &a, &b, sizeof(double) ) == 0;
}

static int relClose( double got, double want, double tol )
{
   double scale = fabs(want) > 1.0 ? fabs(want) : 1.0;
   return fabs( got - want ) <= tol * scale;
}

static void loadSheet( Bars *b )
{
   int i;
   b->nb = SHEET_NB;
   for( i = 0; i < SHEET_NB; i++ )
   {
      b->o[i] = sheet[i][0]; b->h[i] = sheet[i][1];
      b->l[i] = sheet[i][2]; b->c[i] = sheet[i][3];
   }
}

static void loadHistory( Bars *b, const TA_History *history )
{
   int i;
   b->nb = (int)history->nbBars;
   for( i = 0; i < b->nb; i++ )
   {
      b->o[i] = history->open[i];  b->h[i] = history->high[i];
      b->l[i] = history->low[i];   b->c[i] = history->close[i];
   }
}

/* Replays one batch call on every language server; the count check is the
 * only witness that something was compared. */
static ErrorNumber routeCall( const char *tag, const char *func, const Bars *b,
                              int startIdx, int endIdx, double limit,
                              TA_RetCode rc, int beg, int nbOut, const double *out )
{
   int before;
   ErrorNumber e;
   double opt[1];

   if( !server_verify_active() )
      return TA_TEST_PASS;
   opt[0] = limit;
   before = server_verify_value_comparisons();
   e = server_verify( func, startIdx, endIdx, b->nb, rc, beg, nbOut,
                      (const TA_Real*[]){ b->o, b->h, b->l, b->c, NULL },
                      opt, 1,
                      (const TA_Real*[]){ out, NULL }, NULL );
   if( e != TA_TEST_PASS )
      return e;
   if( server_verify_value_comparisons() == before )
   {
      printf( "%s %s: server_verify compared no server despite live pipes\n", func, tag );
      return TA_SV_ROUTED_VACUOUS;
   }
   return TA_TEST_PASS;
}

static TA_RetCode callSI( const Bars *b, int s, int e, double limit,
                          int *beg, int *nbOut, double *out )
{
   return TA_SI( s, e, b->o, b->h, b->l, b->c, limit, beg, nbOut, out );
}

static TA_RetCode callASI( const Bars *b, int s, int e, double limit,
                           int *beg, int *nbOut, double *out )
{
   return TA_ASI( s, e, b->o, b->h, b->l, b->c, limit, beg, nbOut, out );
}

/**** Leg 1: Wilder's book. ****/
static ErrorNumber legBook( void )
{
   /* p.91-92 up day and p.92-93 down day, as two-bar vectors. */
   static const struct { const char *tag; double bars[8]; double si; } ex[] = {
      { "p.91 up day",   { 50.50,52.00,50.00,51.50, 51.80,53.00,51.30,52.80 }, 0x1.a48348348346ep+4 },
      { "p.92 down day", { 53.50,54.00,52.00,52.50, 52.00,52.00,51.00,51.00 }, -37.5 }
   };
   Bars b;
   double out[MAX_NB];
   int beg, nbOut, i, k, day;
   TA_RetCode rc;
   ErrorNumber err;

   for( k = 0; k < 2; k++ )
   {
      b.nb = 2;
      for( i = 0; i < 2; i++ )
      {
         b.o[i] = ex[k].bars[4*i];   b.h[i] = ex[k].bars[4*i+1];
         b.l[i] = ex[k].bars[4*i+2]; b.c[i] = ex[k].bars[4*i+3];
      }
      rc = callSI( &b, 0, 1, 3.0, &beg, &nbOut, out );
      if( rc != TA_SUCCESS || beg != 1 || nbOut != 1 || !relClose( out[0], ex[k].si, 1e-13 ) )
      {
         printf( "SI %s: rc=%d beg=%d nb=%d got %.17g want %.17g\n",
                 ex[k].tag, rc, beg, nbOut, out[0], ex[k].si );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      err = routeCall( ex[k].tag, "SI", &b, 0, 1, 3.0, rc, beg, nbOut, out );
      if( err != TA_TEST_PASS ) return err;
   }

   loadSheet( &b );
   rc = callSI( &b, 0, SHEET_NB-1, 3.0, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || beg != 1 || nbOut != SHEET_NB-1 )
   {
      printf( "SI sheet: rc=%d beg=%d nb=%d\n", rc, beg, nbOut );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( k = 0; k < (int)(sizeof(sheetExact)/sizeof(sheetExact[0])); k++ )
   {
      day = sheetExact[k].day;
      if( !relClose( out[day-2], sheetExact[k].si, 1e-13 ) )
      {
         printf( "SI sheet day %d: got %.17g want %.17g\n", day, out[day-2], sheetExact[k].si );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   for( day = 2; day <= SHEET_NB; day++ )
   {
      if( day == 23 || day == 28 )
         continue;
      if( !(fabs( out[day-2] - sheet[day-1][4] ) < 1.0) )
      {
         printf( "SI sheet day %d: got %.6f, Wilder printed %.0f\n", day, out[day-2], sheet[day-1][4] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   err = routeCall( "sheet", "SI", &b, 0, SHEET_NB-1, 3.0, rc, beg, nbOut, out );
   if( err != TA_TEST_PASS ) return err;

   rc = callASI( &b, 0, SHEET_NB-1, 3.0, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || beg != 0 || nbOut != SHEET_NB || !sameBits( out[0], 0.0 ) )
   {
      printf( "ASI sheet: rc=%d beg=%d nb=%d first %.17g\n", rc, beg, nbOut, out[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   for( day = 2; day <= SHEET_NB; day++ )
   {
      if( !(fabs( out[day-1] - sheet[day-1][5] ) < 5.0) )
      {
         printf( "ASI sheet day %d: got %.6f, Wilder printed %.0f\n", day, out[day-1], sheet[day-1][5] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return routeCall( "sheet", "ASI", &b, 0, SHEET_NB-1, 3.0, rc, beg, nbOut, out );
}

/**** Leg 2: trading-signals 8.3.0, bit for bit. ****/
/* trading-signals 8.3.0 SwingIndex and AccumulativeSwingIndex on the 252-bar
 * reference series, captured by ta-lib-oracles capture_451_si.py. Beyond bars
 * 1, 2, 50 and 251, each SI bar is one where an alternative operation order,
 * ((50N)/R)*K/T, 50*N*(K/T)/R or (50NK)/(RT), changes the bits; ASI is checked
 * on the same bars. A power-of-two limit makes the last two orders agree, so
 * limit 3 is the one that separates them.
 */
struct Spot { int bar; double v; };

static const struct Spot t1_limit3[] = {
   {   1, 0x1.1e22f1e8c74c6p+6     }, /* 71.534125935711103 */
   {   2, -0x1.688b89b23569ap-7    }, /* -0.011002962336003553; kills ((50N)/R)*K/T */
   {  15, 0x1.20c9bd06f87acp+5     }, /* 36.09850507207679; kills ((50N)/R)*K/T */
   {  21, -0x1.0acc4dc04f679p+4    }, /* -16.674878836833589; kills ((50N)/R)*K/T */
   {  22, -0x1.d3c4e734193a4p+5    }, /* -58.471144110698589; kills (50NK)/(RT) */
   {  39, -0x1.676c242d9db00p+3    }, /* -11.231950844854055; kills (50NK)/(RT) */
   {  50, -0x1.1c5d1745d1741p+5    }, /* -35.545454545454511; kills (50NK)/(RT) */
   {  56, 0x1.be0f83e0f83efp+3     }, /* 13.939393939393964; kills 50*N*(K/T)/R */
   {  70, -0x1.7f8cc46b603e1p+4    }, /* -23.971867007672589; kills 50*N*(K/T)/R */
   { 251, -0x1.43a8e2d335d29p+4    }, /* -20.228731942214981; kills 50*N*(K/T)/R */
};

static const struct Spot t1_limit8[] = {
   {   1, 0x1.ad346add2af29p+4     }, /* 26.825297225891664 */
   {   2, -0x1.0e68a745a80f3p-8    }, /* -0.0041261108760013319 */
   {   4, -0x1.f24306002f439p+2    }, /* -7.7853407861521697; kills 50*N*(K/T)/R, (50NK)/(RT) */
   {   5, 0x1.b33f9cb3f9c9bp+0     }, /* 1.7001893939393884; kills ((50N)/R)*K/T */
   {   9, 0x1.c1db1e5f75277p+3     }, /* 14.057997881355943; kills ((50N)/R)*K/T */
   {  14, 0x1.049bb8eb97c45p+1     }, /* 2.0360022688251171; kills 50*N*(K/T)/R, (50NK)/(RT) */
   {  19, -0x1.213092c320855p+3    }, /* -9.0371793566775214; kills 50*N*(K/T)/R, (50NK)/(RT) */
   {  21, -0x1.903274a0771b4p+2    }, /* -6.2530795638125944; kills 50*N*(K/T)/R, (50NK)/(RT) */
   {  50, -0x1.aa8ba2e8ba2e2p+3    }, /* -13.329545454545443; kills 50*N*(K/T)/R, (50NK)/(RT) */
   { 251, -0x1.e57d543cd0bbdp+2    }, /* -7.5857744783306176 */
};

static const struct Spot t2_limit3[] = {
   {   1, 0x1.1e22f1e8c74c6p+6     }, /* 71.534125935711103 */
   {   2, 0x1.1e17ad8c79babp+6     }, /* 71.523122973375095 */
   {  15, -0x1.ddb7ceb8d202ep+5    }, /* -59.71474975958732 */
   {  21, -0x1.4d7cc323c9ef5p+7    }, /* -166.74367629852318 */
   {  22, -0x1.c26dfcf0d03dep+7    }, /* -225.21482040922177 */
   {  39, -0x1.c156ca038c4d9p+7    }, /* -224.6695099934357 */
   {  50, -0x1.1de4210fcf87ep+7    }, /* -142.94556474121413 */
   {  56, -0x1.cd81b521f9aaap+7    }, /* -230.7533350579518 */
   {  70, -0x1.2621632fb565cp+7    }, /* -147.06520985688519 */
   { 251, 0x1.d8a384684d996p+3     }, /* 14.769960597722086 */
};

static const struct Spot t2_limit8[] = {
   {   1, 0x1.ad346add2af29p+4     }, /* 26.825297225891664 */
   {   2, 0x1.ad238452b6981p+4     }, /* 26.821171115015662 */
   {   4, 0x1.74fd5a8f3a872p+4     }, /* 23.311853942381681 */
   {   5, 0x1.9031545a7a23cp+4     }, /* 25.012043336321071 */
   {   9, 0x1.40e1d81187bd2p+2     }, /* 5.0137844248956345 */
   {  14, -0x1.1f709467ebef2p+5    }, /* -35.929970561874043 */
   {  19, -0x1.61a4ceafd0f65p+5    }, /* -44.205472348751563 */
   {  21, -0x1.f43b24b5aee72p+5    }, /* -62.528878611946212 */
   {  50, -0x1.acd63197b74bep+5    }, /* -53.604586777955305 */
   { 251, 0x1.627aa34e3a19bp+2     }, /* 5.5387352241454222 */
};

static ErrorNumber checkSpots( const char *tag, const double *out, int outBeg,
                               const struct Spot *spots, int nbSpots )
{
   int k;
   for( k = 0; k < nbSpots; k++ )
   {
      if( !sameBits( out[spots[k].bar - outBeg], spots[k].v ) )
      {
         printf( "%s bar %d: got %a (%.17g), trading-signals %a\n", tag, spots[k].bar,
                 out[spots[k].bar - outBeg], out[spots[k].bar - outBeg], spots[k].v );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

#define NB_SPOTS(a) ((int)(sizeof(a)/sizeof((a)[0])))

static ErrorNumber legOracle( const Bars *b )
{
   double out[MAX_NB];
   int beg, nbOut;
   ErrorNumber err;

   if( b->nb != 252 )
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   if( callSI( b, 0, 251, 3.0, &beg, &nbOut, out ) != TA_SUCCESS || beg != 1 ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( (err = checkSpots( "SI limit 3", out, beg, t1_limit3, NB_SPOTS(t1_limit3) )) != TA_TEST_PASS ) return err;
   if( callSI( b, 0, 251, 8.0, &beg, &nbOut, out ) != TA_SUCCESS || beg != 1 ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( (err = checkSpots( "SI limit 8", out, beg, t1_limit8, NB_SPOTS(t1_limit8) )) != TA_TEST_PASS ) return err;
   if( callASI( b, 0, 251, 3.0, &beg, &nbOut, out ) != TA_SUCCESS || beg != 0 ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   if( (err = checkSpots( "ASI limit 3", out, beg, t2_limit3, NB_SPOTS(t2_limit3) )) != TA_TEST_PASS ) return err;
   if( callASI( b, 0, 251, 8.0, &beg, &nbOut, out ) != TA_SUCCESS || beg != 0 ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   return checkSpots( "ASI limit 8", out, beg, t2_limit8, NB_SPOTS(t2_limit8) );
}

/**** Leg 3: ASI == [+0.0] ++ CUMSUM(SI(s+1, e)). ****/
static ErrorNumber legComposition( const char *tag, const Bars *b, double limit )
{
   double asi[MAX_NB], si[MAX_NB], cum[MAX_NB];
   int starts[6], s, k, e, beg, nbOut, beg2, nb2, beg3, nb3;
   TA_RetCode rc;

   e = b->nb - 1;
   starts[0] = 0; starts[1] = 1; starts[2] = 2; starts[3] = 37 < e ? 37 : e/2;
   starts[4] = e - 1; starts[5] = e;
   for( k = 0; k < 6; k++ )
   {
      s = starts[k];
      rc = callASI( b, s, e, limit, &beg, &nbOut, asi );
      if( rc != TA_SUCCESS || beg != s || nbOut != e - s + 1 || !sameBits( asi[0], 0.0 ) )
      {
         printf( "ASI %s s=%d: rc=%d beg=%d nb=%d anchor %.17g\n", tag, s, rc, beg, nbOut, asi[0] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( s == e )
         continue;
      rc = callSI( b, s+1, e, limit, &beg2, &nb2, si );
      if( rc != TA_SUCCESS || beg2 != s+1 || nb2 != e - s )
      {
         printf( "SI %s s=%d: rc=%d beg=%d nb=%d\n", tag, s+1, rc, beg2, nb2 );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
      rc = TA_CUMSUM( 0, nb2-1, si, &beg3, &nb3, cum );
      if( rc != TA_SUCCESS || nb3 != nb2 || memcmp( asi+1, cum, sizeof(double)*(size_t)nb3 ) != 0 )
      {
         printf( "ASI %s s=%d limit %g: not CUMSUM of SI(s+1, e) bit for bit\n", tag, s, limit );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/**** Leg 4: degenerate bars. ****/
static ErrorNumber legDegenerate( void )
{
   static const struct { const char *tag; double bars[8]; int negZero; } v[] = {
      { "0/0",                   { 10,10,10,10, 10,10,10,10 }, 0 },
      { "x/0",                   { 10,10,10,10, 11,10,10,12 }, 0 },
      { "K == 0 after down bar", { 11,11, 9,10, 10,10,10,10 }, 1 },
      { "K == 0 after up bar",   {  9,11, 9,10, 10,10,10,10 }, 0 }
   };
   Bars b;
   double out[MAX_NB], u, want;
   int beg, nbOut, i, k;
   TA_RetCode rc;
   ErrorNumber err;

   for( k = 0; k < 4; k++ )
   {
      b.nb = 2;
      for( i = 0; i < 2; i++ )
      {
         b.o[i] = v[k].bars[4*i];   b.h[i] = v[k].bars[4*i+1];
         b.l[i] = v[k].bars[4*i+2]; b.c[i] = v[k].bars[4*i+3];
      }
      rc = callSI( &b, 0, 1, 3.0, &beg, &nbOut, out );
      if( rc != TA_SUCCESS || nbOut != 1 || out[0] != 0.0 || (signbit( out[0] ) != 0) != v[k].negZero )
      {
         printf( "SI %s: got %g, want %s0.0\n", v[k].tag, out[0], v[k].negZero ? "-" : "+" );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      err = routeCall( v[k].tag, "SI", &b, 0, 1, 3.0, rc, beg, nbOut, out );
      if( err != TA_TEST_PASS ) return err;

      rc = callASI( &b, 0, 1, 3.0, &beg, &nbOut, out );
      if( rc != TA_SUCCESS || nbOut != 2 || !sameBits( out[0], 0.0 ) || !sameBits( out[1], 0.0 ) )
      {
         printf( "ASI %s: got %g %g, want +0.0 +0.0\n", v[k].tag, out[0], out[1] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      err = routeCall( v[k].tag, "ASI", &b, 0, 1, 3.0, rc, beg, nbOut, out );
      if( err != TA_TEST_PASS ) return err;
   }

   /* One ulp of range: the value is tiny, not zero, so no epsilon band may
    * stand in for the exact R == 0 test. */
   b.nb = 2;
   b.o[0] = b.h[0] = b.l[0] = b.c[0] = 10.0;
   u = nextafter( 10.0, 11.0 );
   b.o[1] = b.l[1] = 10.0;
   b.h[1] = b.c[1] = u;
   want = 75.0 * ((u - 10.0) / 3.0);
   rc = callSI( &b, 0, 1, 3.0, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || nbOut != 1 || want == 0.0 || !sameBits( out[0], want ) )
   {
      printf( "SI near-flat: got %.17g want %.17g\n", out[0], want );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   err = routeCall( "near-flat", "SI", &b, 0, 1, 3.0, rc, beg, nbOut, out );
   if( err != TA_TEST_PASS ) return err;

   /* A flat run inside a moving series: +0.0 on each flat bar and ASI holds. */
   {
      static const double fr[6][4] = {
         { 10.0, 11.0,  9.0, 10.5 }, { 10.5, 10.5, 10.5, 10.5 }, { 10.5, 10.5, 10.5, 10.5 },
         { 10.5, 10.5, 10.5, 10.5 }, { 10.4, 12.0, 10.0, 11.7 }, { 11.7, 11.7, 11.7, 11.7 }
      };
      static const int flat[6] = { 0, 1, 1, 1, 0, 1 };
      double asi[MAX_NB];
      b.nb = 6;
      for( i = 0; i < 6; i++ )
      {
         b.o[i] = fr[i][0]; b.h[i] = fr[i][1]; b.l[i] = fr[i][2]; b.c[i] = fr[i][3];
      }
      rc = callSI( &b, 0, 5, 3.0, &beg, &nbOut, out );
      if( rc != TA_SUCCESS || nbOut != 5 ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
      rc = callASI( &b, 0, 5, 3.0, &beg, &nbOut, asi );
      if( rc != TA_SUCCESS || nbOut != 6 ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
      for( i = 1; i < 6; i++ )
      {
         if( flat[i] && (!sameBits( out[i-1], 0.0 ) || !sameBits( asi[i], asi[i-1] )) )
         {
            printf( "SI/ASI flat run bar %d: SI %g, ASI %g after %g\n", i, out[i-1], asi[i], asi[i-1] );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         if( !flat[i] && out[i-1] == 0.0 )
         {
            printf( "SI flat run bar %d: a moving bar read 0\n", i );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      err = routeCall( "flat run", "SI", &b, 0, 5, 3.0, TA_SUCCESS, 1, 5, out );
      if( err != TA_TEST_PASS ) return err;
      err = routeCall( "flat run", "ASI", &b, 0, 5, 3.0, TA_SUCCESS, 0, 6, asi );
      if( err != TA_TEST_PASS ) return err;
   }
   return TA_TEST_PASS;
}

/**** Leg 5: the limit move. ****/
static ErrorNumber legLimit( const Bars *b )
{
   const double bad[4] = { 0.0, -3.0, 0.0000000099, NAN };
   double out[MAX_NB], ref[MAX_NB], half[MAX_NB], last;
   int beg, nbOut, k, i, e;
   TA_RetCode rc;
   TA_SI_Stream  *ss;
   TA_ASI_Stream *as;

   e = b->nb - 1;
   for( k = 0; k < 4; k++ )
   {
      if( callSI( b, 0, e, bad[k], &beg, &nbOut, out ) != TA_BAD_PARAM ||
          callASI( b, 0, e, bad[k], &beg, &nbOut, out ) != TA_BAD_PARAM ||
          TA_SI_Lookback( bad[k] ) != -1 || TA_ASI_Lookback( bad[k] ) != -1 )
      {
         printf( "SI/ASI limit %g: not rejected by batch and lookback\n", bad[k] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
      ss = NULL; as = NULL;
      if( TA_SI_Open( &ss, b->o, b->h, b->l, b->c, b->nb, bad[k], &last ) != TA_BAD_PARAM ||
          TA_ASI_Open( &as, b->o, b->h, b->l, b->c, b->nb, bad[k], &last ) != TA_BAD_PARAM )
      {
         printf( "SI/ASI limit %g: not rejected by the stream open\n", bad[k] );
         if( ss ) TA_SI_Close( ss );
         if( as ) TA_ASI_Close( as );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }
   }

   callSI( b, 0, e, 3.0, &beg, &nbOut, ref );
   rc = callSI( b, 0, e, TA_REAL_DEFAULT, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || memcmp( out, ref, sizeof(double)*(size_t)nbOut ) != 0 )
   {
      printf( "SI: the default limit move is not 3.0\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   callASI( b, 0, e, 3.0, &beg, &nbOut, ref );
   rc = callASI( b, 0, e, TA_REAL_DEFAULT, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || memcmp( out, ref, sizeof(double)*(size_t)nbOut ) != 0 )
   {
      printf( "ASI: the default limit move is not 3.0\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   /* The limit is a pure scale and halving is exact. */
   callSI( b, 0, e, 3.0, &beg, &nbOut, ref );
   callSI( b, 0, e, 6.0, &beg, &nbOut, half );
   for( i = 0; i < nbOut; i++ )
   {
      if( !sameBits( half[i], 0.5*ref[i] ) )
      {
         printf( "SI limit 6 bar %d: %.17g is not half of %.17g\n", i+1, half[i], ref[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   for( k = 0; k < 2; k++ )
   {
      double lim = k == 0 ? 0.00000001 : TA_REAL_MAX;
      rc = callSI( b, 0, e, lim, &beg, &nbOut, out );
      for( i = 0; rc == TA_SUCCESS && i < nbOut; i++ )
         if( !isfinite( out[i] ) ) rc = TA_INTERNAL_ERROR(0);
      if( rc != TA_SUCCESS )
      {
         printf( "SI limit %g: not finite everywhere\n", lim );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   return TA_TEST_PASS;
}

/**** Leg 6: lookback and output range. ****/
static ErrorNumber legRange( const Bars *b )
{
   double full[MAX_NB], out[MAX_NB];
   const int starts[3] = { 1, 2, 37 };
   int beg, nbOut, begF, nbF, k, e;
   TA_RetCode rc;

   e = b->nb - 1;
   if( TA_SI_Lookback( 3.0 ) != 1 || TA_ASI_Lookback( 3.0 ) != 0 ||
       TA_SI_Lookback( TA_REAL_DEFAULT ) != 1 || TA_ASI_Lookback( TA_REAL_DEFAULT ) != 0 )
   {
      printf( "SI/ASI: lookback is not 1/0\n" );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   rc = callSI( b, 0, 0, 3.0, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || beg != 0 || nbOut != 0 )
   {
      printf( "SI(0, 0): rc=%d beg=%d nb=%d, want no output\n", rc, beg, nbOut );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }
   rc = callSI( b, 0, e, 3.0, &begF, &nbF, full );
   if( rc != TA_SUCCESS || begF != 1 || nbF != e )
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   for( k = 0; k < 3; k++ )
   {
      rc = callSI( b, starts[k], e, 3.0, &beg, &nbOut, out );
      if( rc != TA_SUCCESS || beg != starts[k] || nbOut != e - starts[k] + 1 ||
          memcmp( out, full + (starts[k] - 1), sizeof(double)*(size_t)nbOut ) != 0 )
      {
         printf( "SI(%d, e) differs from SI(0, e) on the shared bars\n", starts[k] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }
   rc = callASI( b, 17, 17, 3.0, &beg, &nbOut, out );
   if( rc != TA_SUCCESS || beg != 17 || nbOut != 1 || !sameBits( out[0], 0.0 ) )
   {
      printf( "ASI(17, 17): rc=%d beg=%d nb=%d, want one +0.0\n", rc, beg, nbOut );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/**** Leg 7: path_dependent. ****/
static ErrorNumber legPathDependent( const Bars *b )
{
   const TA_FuncHandle *handle;
   const TA_FuncInfo *info;
   double full[MAX_NB], sub[MAX_NB], cut[MAX_NB];
   int beg, nbOut, begF, nbF, begC, nbC, s, e, i, differs;

   if( TA_GetFuncHandle( "ASI", &handle ) != TA_SUCCESS || TA_GetFuncInfo( handle, &info ) != TA_SUCCESS ||
       !(info->flags & TA_FUNC_FLG_PATH_DEP) )
   {
      printf( "ASI: TA_FUNC_FLG_PATH_DEP is not published\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }
   if( TA_GetFuncHandle( "SI", &handle ) != TA_SUCCESS || TA_GetFuncInfo( handle, &info ) != TA_SUCCESS ||
       (info->flags & TA_FUNC_FLG_PATH_DEP) )
   {
      printf( "SI: TA_FUNC_FLG_PATH_DEP is published but SI reads one prior bar only\n" );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   e = b->nb - 1;
   s = 37;
   callASI( b, 0, e, 3.0, &begF, &nbF, full );
   callASI( b, s, e, 3.0, &beg, &nbOut, sub );
   differs = 0;
   for( i = 0; i < nbOut; i++ )
      if( !sameBits( sub[i], full[s+i] ) ) differs = 1;
   if( !differs )
   {
      printf( "ASI(%d, e) equals ASI(0, e): path_dependent is not earned\n", s );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   TA_ASI( 0, e-s, b->o+s, b->h+s, b->l+s, b->c+s, 3.0, &begC, &nbC, cut );
   if( nbC != nbOut || memcmp( sub, cut, sizeof(double)*(size_t)nbOut ) != 0 )
   {
      printf( "ASI(%d, e, x) is not ASI(0, e-%d, x+%d): stale state, not a re-anchor\n", s, s, s );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   return TA_TEST_PASS;
}

/**** Leg 8: outReal aliasing each input. ****/
static ErrorNumber legAliasing( const Bars *b )
{
   const int starts[3] = { 0, 1, 37 };
   double ref[MAX_NB], buf[4][MAX_NB];
   int f, k, x, s, e, beg, nbOut, begR, nbR;
   TA_RetCode rc;

   e = b->nb - 1;
   for( f = 0; f < 2; f++ )
   {
      for( k = 0; k < 3; k++ )
      {
         s = starts[k];
         rc = f == 0 ? callSI( b, s, e, 3.0, &begR, &nbR, ref ) : callASI( b, s, e, 3.0, &begR, &nbR, ref );
         if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
         for( x = 0; x < 4; x++ )
         {
            memcpy( buf[0], b->o, sizeof(double)*(size_t)b->nb );
            memcpy( buf[1], b->h, sizeof(double)*(size_t)b->nb );
            memcpy( buf[2], b->l, sizeof(double)*(size_t)b->nb );
            memcpy( buf[3], b->c, sizeof(double)*(size_t)b->nb );
            rc = f == 0
               ? TA_SI( s, e, buf[0], buf[1], buf[2], buf[3], 3.0, &beg, &nbOut, buf[x] )
               : TA_ASI( s, e, buf[0], buf[1], buf[2], buf[3], 3.0, &beg, &nbOut, buf[x] );
            if( rc != TA_SUCCESS || beg != begR || nbOut != nbR ||
                memcmp( buf[x], ref, sizeof(double)*(size_t)nbR ) != 0 )
            {
               printf( "%s s=%d: outReal aliasing input %d changes the result\n", f == 0 ? "SI" : "ASI", s, x );
               return TA_TESTUTIL_TFRR_BAD_CALCULATION;
            }
         }
      }
   }
   return TA_TEST_PASS;
}

/**** Leg 9: stream against batch. ****/
static ErrorNumber legStream( const Bars *b, double limit )
{
   const int starts[3] = { 0, 1, 37 };
   double batch[MAX_NB], got, peeked;
   int k, s, h, e, i, beg, nbOut;
   TA_RetCode rc;

   e = b->nb - 1;
   rc = callSI( b, 0, e, limit, &beg, &nbOut, batch );
   if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
   {
      TA_SI_Stream *st = NULL;
      h = 5;
      rc = TA_SI_Open( &st, b->o, b->h, b->l, b->c, h, limit, &got );
      if( rc != TA_SUCCESS || !sameBits( got, batch[h-2] ) )
      {
         printf( "SI stream open: rc=%d got %.17g want %.17g\n", rc, got, batch[h-2] );
         if( st ) TA_SI_Close( st );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = h; i <= e; i++ )
      {
         TA_SI_Peek( st, b->o[i], b->h[i], b->l[i], b->c[i], &peeked );
         rc = TA_SI_Update( st, b->o[i], b->h[i], b->l[i], b->c[i], &got );
         if( rc != TA_SUCCESS || !sameBits( got, batch[i-1] ) || !sameBits( peeked, got ) )
         {
            printf( "SI stream bar %d: got %.17g peek %.17g batch %.17g\n", i, got, peeked, batch[i-1] );
            TA_SI_Close( st );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      TA_SI_Close( st );
   }

   /* An ASI stream opened on bars s.. anchors at s. */
   for( k = 0; k < 3; k++ )
   {
      TA_ASI_Stream *st = NULL;
      s = starts[k];
      rc = callASI( b, s, e, limit, &beg, &nbOut, batch );
      if( rc != TA_SUCCESS ) return TA_TESTUTIL_TFRR_BAD_RETCODE;
      h = 1;
      rc = TA_ASI_Open( &st, b->o+s, b->h+s, b->l+s, b->c+s, h, limit, &got );
      if( rc != TA_SUCCESS || !sameBits( got, 0.0 ) )
      {
         printf( "ASI stream opened at %d: rc=%d first %.17g\n", s, rc, got );
         if( st ) TA_ASI_Close( st );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      for( i = s+h; i <= e; i++ )
      {
         TA_ASI_Peek( st, b->o[i], b->h[i], b->l[i], b->c[i], &peeked );
         rc = TA_ASI_Update( st, b->o[i], b->h[i], b->l[i], b->c[i], &got );
         if( rc != TA_SUCCESS || !sameBits( got, batch[i-s] ) || !sameBits( peeked, got ) )
         {
            printf( "ASI stream from %d, bar %d: got %.17g peek %.17g batch %.17g\n",
                    s, i, got, peeked, batch[i-s] );
            TA_ASI_Close( st );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      TA_ASI_Close( st );
   }
   return TA_TEST_PASS;
}

/**** Global functions definitions. ****/
ErrorNumber test_func_si( TA_History *history )
{
   Bars sheetBars, ref;
   double out[MAX_NB];
   int beg, nbOut, k;
   ErrorNumber err;
   TA_RetCode rc;
   const double limits[2] = { 3.0, 8.0 };

   if( history->nbBars > MAX_NB )
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   loadSheet( &sheetBars );
   loadHistory( &ref, history );

   err = legBook();
   if( err != TA_TEST_PASS ) return err;

   err = legOracle( &ref );
   if( err != TA_TEST_PASS ) return err;

   for( k = 0; k < 2; k++ )
   {
      rc = callSI( &ref, 0, ref.nb-1, limits[k], &beg, &nbOut, out );
      err = routeCall( "252-bar", "SI", &ref, 0, ref.nb-1, limits[k], rc, beg, nbOut, out );
      if( err != TA_TEST_PASS ) return err;
      rc = callASI( &ref, 0, ref.nb-1, limits[k], &beg, &nbOut, out );
      err = routeCall( "252-bar", "ASI", &ref, 0, ref.nb-1, limits[k], rc, beg, nbOut, out );
      if( err != TA_TEST_PASS ) return err;
   }

   if( (err = legComposition( "sheet", &sheetBars, 3.0 )) != TA_TEST_PASS ) return err;
   if( (err = legComposition( "252-bar", &ref, 3.0 )) != TA_TEST_PASS ) return err;
   if( (err = legComposition( "252-bar", &ref, 0.37 )) != TA_TEST_PASS ) return err;
   if( (err = legDegenerate()) != TA_TEST_PASS ) return err;
   if( (err = legLimit( &ref )) != TA_TEST_PASS ) return err;
   if( (err = legLimit( &sheetBars )) != TA_TEST_PASS ) return err;
   if( (err = legRange( &ref )) != TA_TEST_PASS ) return err;
   if( (err = legPathDependent( &ref )) != TA_TEST_PASS ) return err;
   if( (err = legAliasing( &ref )) != TA_TEST_PASS ) return err;
   if( (err = legStream( &ref, 3.0 )) != TA_TEST_PASS ) return err;
   if( (err = legStream( &sheetBars, 8.0 )) != TA_TEST_PASS ) return err;

   return TA_TEST_PASS;
}
