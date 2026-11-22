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
 *  112126 MF,CC  First version (issue #610).
 */

/* Description:
 *
 *   Test TA_MAENV (Moving Average Envelope) on the 252-bar reference close.
 *
 *   Legs:
 *     A. EXTERNAL ORACLE: eight SMA/EMA runs against Pine's Envelope.
 *     B. EXTERNAL ORACLE: unequal percentages, each band against the run
 *        carrying its own percentage. Plus cross-language.
 *     C. FORM AND DISPATCH, bitwise, no oracle: every MA type, the middle band
 *        against TA_MA and each band as one multiplication by its factor; a
 *        negated series inverts the band order; the moving average's unstable
 *        period carries through. Plus cross-language.
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
#define OUT_CAP 1024   /* > history->nbBars */

/* Produced by TradingView's built-in Envelope script (Pine v6:
 * basis = exponential ? ta.ema(src, len) : ta.sma(src, len), k = percent/100.0,
 * upper = basis * (1 + k), lower = basis * (1 - k)) run on PyneCore 6.8.14
 * (pynesys-pynecore, a Pine runtime), Python 3.11.18, over the 252 ta_regtest
 * close bars, by pine_serve/capture_610_maenv.py of the private ta-lib-oracles
 * repo at commit 51d0076. Rows are its goldens.txt unchanged: every 32nd bar
 * from the first output, and the last bar. They show what Pine specifies, not
 * what TradingView computes.
 *
 * A tolerance and not bits: Pine's ta.sma rounds its rolling sum differently
 * from TA-Lib's running total, and its ta.ema seed rides on it. Measured over
 * every bar of all eight runs and all three outputs, the worst relative
 * disagreement is 1.7e-15 (sma_20_10, upper, bar 215).
 */
#define MAENV_ORACLE_TOL 1e-12

typedef struct { int bar; double basis; double upper; double lower; } MaenvRow;

typedef struct { const char *tag; TA_MAType maType; int period; double percent;
                 const MaenvRow *row; int nbRow; } MaenvBlock;

#define NB_ROWS(a) ((int)(sizeof(a)/sizeof(a[0])))

static const MaenvRow sma_20_10[] =
{
   { 19, 92.890999999999991, 102.1801, 83.601900000000001 },
   { 51, 87.951000000000022, 96.746100000000027, 79.155900000000017 },
   { 83, 95.463000000000051, 105.00930000000007, 85.916700000000048 },
   { 115, 115.95775000000006, 127.55352500000008, 104.36197500000006 },
   { 147, 129.31250000000011, 142.24375000000015, 116.38125000000011 },
   { 179, 128.05550000000014, 140.86105000000018, 115.24995000000013 },
   { 211, 102.08000000000013, 112.28800000000015, 91.872000000000114 },
   { 243, 108.94250000000011, 119.83675000000012, 98.048250000000095 },
   { 251, 110.57000000000012, 121.62700000000014, 99.513000000000105 },
};
static const MaenvRow sma_20_2_5[] =
{
   { 19, 92.890999999999991, 95.213274999999982, 90.568724999999986 },
   { 51, 87.951000000000022, 90.14977500000002, 85.752225000000024 },
   { 83, 95.463000000000051, 97.849575000000044, 93.076425000000043 },
   { 115, 115.95775000000006, 118.85669375000005, 113.05880625000006 },
   { 147, 129.31250000000011, 132.54531250000011, 126.07968750000011 },
   { 179, 128.05550000000014, 131.25688750000012, 124.85411250000013 },
   { 211, 102.08000000000013, 104.63200000000012, 99.528000000000119 },
   { 243, 108.94250000000011, 111.66606250000011, 106.21893750000011 },
   { 251, 110.57000000000012, 113.33425000000011, 107.80575000000012 },
};
static const MaenvRow sma_9_10[] =
{
   { 8, 93.309444444444438, 102.64038888888889, 83.978499999999997 },
   { 40, 85.917777777777772, 94.509555555555551, 77.325999999999993 },
   { 72, 89.86166666666665, 98.847833333333327, 80.875499999999988 },
   { 104, 114.05277777777775, 125.45805555555553, 102.64749999999998 },
   { 136, 135.52888888888887, 149.08177777777777, 121.97599999999998 },
   { 168, 124.06888888888891, 136.47577777777781, 111.66200000000002 },
   { 200, 109.79666666666668, 120.77633333333335, 98.817000000000021 },
   { 232, 105.69333333333334, 116.26266666666669, 95.124000000000009 },
   { 251, 109.01333333333335, 119.91466666666669, 98.112000000000023 },
};
static const MaenvRow sma_9_2_5[] =
{
   { 8, 93.309444444444438, 95.642180555555541, 90.97670833333332 },
   { 40, 85.917777777777772, 88.065722222222206, 83.769833333333324 },
   { 72, 89.86166666666665, 92.108208333333309, 87.615124999999978 },
   { 104, 114.05277777777775, 116.90409722222218, 111.20145833333331 },
   { 136, 135.52888888888887, 138.91711111111107, 132.14066666666665 },
   { 168, 124.06888888888891, 127.17061111111111, 120.96716666666669 },
   { 200, 109.79666666666668, 112.54158333333334, 107.05175000000001 },
   { 232, 105.69333333333334, 108.33566666666667, 103.051 },
   { 251, 109.01333333333335, 111.73866666666667, 106.28800000000001 },
};
static const MaenvRow ema_20_10[] =
{
   { 19, 92.890999999999991, 102.1801, 83.601900000000001 },
   { 51, 88.637448853697308, 97.501193739067048, 79.773703968327581 },
   { 83, 97.438512771190148, 107.18236404830917, 87.694661494071141 },
   { 115, 116.04295486558618, 127.64725035214481, 104.43865937902757 },
   { 147, 125.90676961775969, 138.49744657953568, 113.31609265598372 },
   { 179, 128.76758877953702, 141.64434765749073, 115.89082990158332 },
   { 211, 102.00788254328894, 112.20867079761784, 91.807094288960045 },
   { 243, 108.04653910284888, 118.85119301313378, 97.241885192563998 },
   { 251, 108.52646097903104, 119.37910707693416, 97.673814881127939 },
};
static const MaenvRow ema_20_2_5[] =
{
   { 19, 92.890999999999991, 95.213274999999982, 90.568724999999986 },
   { 51, 88.637448853697308, 90.853385075039739, 86.421512632354876 },
   { 83, 97.438512771190148, 99.874475590469899, 95.002549951910396 },
   { 115, 116.04295486558618, 118.94402873722582, 113.14188099394653 },
   { 147, 125.90676961775969, 129.05443885820367, 122.7591003773157 },
   { 179, 128.76758877953702, 131.98677849902543, 125.5483990600486 },
   { 211, 102.00788254328894, 104.55807960687116, 99.457685479706711 },
   { 243, 108.04653910284888, 110.7477025804201, 105.34537562527765 },
   { 251, 108.52646097903104, 111.23962250350681, 105.81329945455526 },
};
static const MaenvRow ema_9_10[] =
{
   { 8, 93.309444444444438, 102.64038888888889, 83.978499999999997 },
   { 40, 85.201600073419087, 93.721760080761001, 76.681440066077187 },
   { 72, 88.144881697016643, 96.959369866718319, 79.330393527314982 },
   { 104, 114.12538503864718, 125.5379235425119, 102.71284653478247 },
   { 136, 133.60268287737429, 146.96295116511175, 120.24241458963687 },
   { 168, 124.60730156508602, 137.06803172159462, 112.14657140857742 },
   { 200, 110.07910455431298, 121.08701500974428, 99.071194098881676 },
   { 232, 104.99219413194888, 115.49141354514377, 94.492974718753999 },
   { 251, 108.94720710690683, 119.84192781759752, 98.052486396216153 },
};
static const MaenvRow ema_9_2_5[] =
{
   { 8, 93.309444444444438, 95.642180555555541, 90.97670833333332 },
   { 40, 85.201600073419087, 87.331640075254555, 83.071560071583605 },
   { 72, 88.144881697016643, 90.348503739442052, 85.941259654591221 },
   { 104, 114.12538503864718, 116.97851966461334, 111.272250412681 },
   { 136, 133.60268287737429, 136.94274994930865, 130.26261580543994 },
   { 168, 124.60730156508602, 127.72248410421315, 121.49211902595887 },
   { 200, 110.07910455431298, 112.83108216817079, 107.32712694045514 },
   { 232, 104.99219413194888, 107.61699898524759, 102.36738927865015 },
   { 251, 108.94720710690683, 111.67088728457949, 106.22352692923415 },
};

static const MaenvBlock maenvBlock[] =
{
   { "sma_20_10", TA_MAType_SMA, 20, 10.0, sma_20_10, NB_ROWS(sma_20_10) },
   { "sma_20_2.5", TA_MAType_SMA, 20, 2.5, sma_20_2_5, NB_ROWS(sma_20_2_5) },
   { "sma_9_10", TA_MAType_SMA, 9, 10.0, sma_9_10, NB_ROWS(sma_9_10) },
   { "sma_9_2.5", TA_MAType_SMA, 9, 2.5, sma_9_2_5, NB_ROWS(sma_9_2_5) },
   { "ema_20_10", TA_MAType_EMA, 20, 10.0, ema_20_10, NB_ROWS(ema_20_10) },
   { "ema_20_2.5", TA_MAType_EMA, 20, 2.5, ema_20_2_5, NB_ROWS(ema_20_2_5) },
   { "ema_9_10", TA_MAType_EMA, 9, 10.0, ema_9_10, NB_ROWS(ema_9_10) },
   { "ema_9_2.5", TA_MAType_EMA, 9, 2.5, ema_9_2_5, NB_ROWS(ema_9_2_5) },
};
#define NB_MAENV_BLOCK ((int)(sizeof(maenvBlock)/sizeof(maenvBlock[0])))

/**** Local functions declarations. ****/
static ErrorNumber test_maenv_oracle( const TA_History *history, int *nbCompared );
static ErrorNumber test_maenv_unequal( const TA_History *history, int *nbCompared );
static ErrorNumber test_maenv_form( const TA_History *history );

/**** Global functions definitions. ****/
ErrorNumber test_func_maenv( TA_History *history )
{
   ErrorNumber e;
   int nbCompared = 0, nbRows = 0, b;

   for( b = 0; b < NB_MAENV_BLOCK; b++ )
      nbRows += maenvBlock[b].nbRow;

   /* The oracle rows are the moving averages at their own lookback. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   e = test_maenv_oracle( history, &nbCompared );
   if( e != TA_TEST_PASS ) return e;
   if( nbCompared != 3 * nbRows )
   {
      printf( "MAENV oracle Fail: compared %d values, expected %d\n", nbCompared, 3 * nbRows );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   e = test_maenv_unequal( history, &nbCompared );
   if( e != TA_TEST_PASS ) return e;

   return test_maenv_form( history );
}

/**** Local functions definitions. ****/

static const MaenvBlock *maenv_find( TA_MAType maType, int period, double percent )
{
   int b;
   for( b = 0; b < NB_MAENV_BLOCK; b++ )
      if( maenvBlock[b].maType == maType && maenvBlock[b].period == period &&
          maenvBlock[b].percent == percent )
         return &maenvBlock[b];
   return NULL;
}

static int maenv_close( double got, double want )
{
   return fabs( got - want ) <= MAENV_ORACLE_TOL * fabs( want );
}

static ErrorNumber maenv_check( const TA_History *history, const char *label,
                                TA_MAType maType, int period,
                                double pctUp, double pctDn,
                                const MaenvBlock *mid, const MaenvBlock *up,
                                const MaenvBlock *dn, int route, int *nbCompared )
{
   static TA_Real outUp[OUT_CAP], outMid[OUT_CAP], outLo[OUT_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   int k;

   if( !mid || !up || !dn || mid->nbRow != up->nbRow || mid->nbRow != dn->nbRow )
   {
      printf( "MAENV Fail [%s]: golden block missing or mismatched\n", label );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   retCode = TA_MAENV( 0, nbBars - 1, history->close, period, pctUp, pctDn, maType,
                       &begIdx, &nbElement, outUp, outMid, outLo );
   if( retCode != TA_SUCCESS )
   {
      printf( "MAENV Fail [%s]: retCode = %d\n", label, (int)retCode );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   if( begIdx != period - 1 || nbElement != nbBars - ( period - 1 ) )
   {
      printf( "MAENV Fail [%s]: shape got (%d,%d) expected (%d,%d)\n",
              label, begIdx, nbElement, period - 1, nbBars - ( period - 1 ) );
      return begIdx != period - 1 ? TA_TESTUTIL_TFRR_BAD_BEGIDX
                                  : TA_TESTUTIL_TFRR_BAD_OUTNBELEMENT;
   }

   for( k = 0; k < mid->nbRow; k++ )
   {
      int bar = mid->row[k].bar;
      int idx = bar - begIdx;
      double want[3], got[3];
      int band;

      if( up->row[k].bar != bar || dn->row[k].bar != bar || idx < 0 || idx >= nbElement )
      {
         printf( "MAENV Fail [%s]: bar %d unusable\n", label, bar );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      want[0] = up->row[k].upper;  got[0] = outUp[idx];
      want[1] = mid->row[k].basis; got[1] = outMid[idx];
      want[2] = dn->row[k].lower;  got[2] = outLo[idx];
      for( band = 0; band < 3; band++ )
      {
         ( *nbCompared )++;
         if( !maenv_close( got[band], want[band] ) )
         {
            printf( "MAENV oracle Fail [Pine %s] band %d at bar %d: got %.17g expected %.17g "
                    "(rel %.3e > %.0e)\n", label, band, bar, got[band], want[band],
                    fabs( got[band] - want[band] ) / fabs( want[band] ), MAENV_ORACLE_TOL );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
   }

   if( route && server_verify_active() )
   {
      double optIn[4];
      optIn[0] = (double)period;
      optIn[1] = pctUp;
      optIn[2] = pctDn;
      optIn[3] = (double)maType;
      return server_verify( "MAENV", 0, nbBars - 1, nbBars, retCode, begIdx, nbElement,
                            (const TA_Real*[]){ history->close, NULL }, optIn, 4,
                            (const TA_Real*[]){ outUp, outMid, outLo, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

/* A: each run against its own block. */
static ErrorNumber test_maenv_oracle( const TA_History *history, int *nbCompared )
{
   int b;
   for( b = 0; b < NB_MAENV_BLOCK; b++ )
   {
      const MaenvBlock *g = &maenvBlock[b];
      ErrorNumber e = maenv_check( history, g->tag, g->maType, g->period,
                                   g->percent, g->percent, g, g, g, 0, nbCompared );
      if( e != TA_TEST_PASS ) return e;
   }
   return TA_TEST_PASS;
}

/* B: each band follows its own percentage, not the other's. */
static ErrorNumber test_maenv_unequal( const TA_History *history, int *nbCompared )
{
   static const struct { TA_MAType maType; int period; double up; double dn; int route; } cell[] =
   {
      { TA_MAType_SMA, 20, 10.0, 2.5, 0 },
      { TA_MAType_SMA, 20, 2.5, 10.0, 0 },
      { TA_MAType_EMA,  9, 2.5, 10.0, 1 },
   };
   unsigned int c;

   for( c = 0; c < sizeof(cell)/sizeof(cell[0]); c++ )
   {
      const MaenvBlock *up = maenv_find( cell[c].maType, cell[c].period, cell[c].up );
      char label[64];
      ErrorNumber e;

      snprintf( label, sizeof(label), "%s_%d up %g dn %g",
                cell[c].maType == TA_MAType_SMA ? "sma" : "ema",
                cell[c].period, cell[c].up, cell[c].dn );
      e = maenv_check( history, label, cell[c].maType, cell[c].period, cell[c].up, cell[c].dn,
                       up, up, maenv_find( cell[c].maType, cell[c].period, cell[c].dn ),
                       cell[c].route, nbCompared );
      if( e != TA_TEST_PASS ) return e;
   }
   return TA_TEST_PASS;
}

static ErrorNumber maenv_form_cell( const TA_Real *in, int nbBars, TA_MAType maType,
                                    int period, double pctUp, double pctDn, int route )
{
   static TA_Real outUp[OUT_CAP], outMid[OUT_CAP], outLo[OUT_CAP], ma[OUT_CAP];
   TA_Integer begIdx, nbElement, maBeg, maNb;
   TA_RetCode retCode, maRetCode;
   double upFactor = 1.0 + pctUp / 100.0;
   double dnFactor = 1.0 - pctDn / 100.0;
   int lookback = TA_MA_Lookback( period, maType );
   int i;

   if( TA_MAENV_Lookback( period, pctUp, pctDn, maType ) != lookback || lookback < 0 )
   {
      printf( "MAENV form Fail [type %d, %d]: lookback %d, TA_MA_Lookback %d\n", (int)maType,
              period, TA_MAENV_Lookback( period, pctUp, pctDn, maType ), lookback );
      return TA_TESTUTIL_TFRR_BAD_BEGIDX;
   }

   retCode = TA_MAENV( 0, nbBars - 1, in, period, pctUp, pctDn, maType,
                       &begIdx, &nbElement, outUp, outMid, outLo );
   maRetCode = TA_MA( 0, nbBars - 1, in, period, maType, &maBeg, &maNb, ma );
   if( retCode != TA_SUCCESS || maRetCode != TA_SUCCESS || begIdx != lookback ||
       maBeg != begIdx || maNb != nbElement || nbElement != nbBars - lookback )
   {
      printf( "MAENV form Fail [type %d, %d]: rc=%d/%d begIdx=%d/%d nb=%d/%d lookback %d\n",
              (int)maType, period, (int)retCode, (int)maRetCode, begIdx, maBeg,
              nbElement, maNb, lookback );
      if( retCode != TA_SUCCESS || maRetCode != TA_SUCCESS )
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      return ( begIdx != lookback || maBeg != begIdx ) ? TA_TESTUTIL_TFRR_BAD_BEGIDX
                                                       : TA_TESTUTIL_TFRR_BAD_OUTNBELEMENT;
   }

   if( memcmp( outMid, ma, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
   {
      printf( "MAENV form Fail [type %d, %d]: middle band differs from TA_MA\n",
              (int)maType, period );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   if( maType == TA_MAType_SMA && period == 1 &&
       memcmp( outMid, in, (size_t)nbElement * sizeof(TA_Real) ) != 0 )
   {
      printf( "MAENV form Fail: SMA period 1 middle band is not the input\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   for( i = 0; i < nbElement; i++ )
   {
      double up = outMid[i] * upFactor;
      double lo = outMid[i] * dnFactor;

      if( memcmp( &outUp[i], &up, sizeof(double) ) != 0 ||
          memcmp( &outLo[i], &lo, sizeof(double) ) != 0 )
      {
         printf( "MAENV form Fail [type %d, %d, up %g, dn %g] bar %d: (%.17g,%.17g) != "
                 "middle * factor (%.17g,%.17g)\n", (int)maType, period, pctUp, pctDn,
                 begIdx + i, outUp[i], outLo[i], up, lo );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   if( route && server_verify_active() )
   {
      double optIn[4];
      optIn[0] = (double)period;
      optIn[1] = pctUp;
      optIn[2] = pctDn;
      optIn[3] = (double)maType;
      return server_verify( "MAENV", 0, nbBars - 1, nbBars, retCode, begIdx, nbElement,
                            (const TA_Real*[]){ in, NULL }, optIn, 4,
                            (const TA_Real*[]){ outUp, outMid, outLo, NULL }, NULL );
   }
   return TA_TEST_PASS;
}

static ErrorNumber test_maenv_form( const TA_History *history )
{
   static const struct { int period; double up; double dn; } cell[] =
   {
      { 20, 10.0, 10.0 },
      {  9,  2.5, 10.0 },
      {  5, -3.0,  0.0 },
      {  1, 10.0,  2.5 },
   };
   static TA_Real neg[OUT_CAP], outUp[OUT_CAP], outMid[OUT_CAP], outLo[OUT_CAP];
   TA_Integer begIdx, nbElement;
   TA_RetCode retCode;
   int nbBars = (int)history->nbBars;
   static const struct { TA_MAType maType; TA_FuncUnstId id; unsigned int unst; } warm[] =
   {
      { TA_MAType_EMA,  TA_FUNC_UNST_EMA,  5 },
      { TA_MAType_KAMA, TA_FUNC_UNST_KAMA, 7 },
   };
   int t, i;
   unsigned int c;
   ErrorNumber e;

   for( t = TA_MATYPE_MIN; t <= TA_MATYPE_MAX; t++ )
   {
      for( c = 0; c < sizeof(cell)/sizeof(cell[0]); c++ )
      {
         e = maenv_form_cell( history->close, nbBars, (TA_MAType)t, cell[c].period,
                              cell[c].up, cell[c].dn, 0 );
         if( e != TA_TEST_PASS ) return e;
      }
   }

   /* The lookback has to move, or the cell repeats one above. */
   for( c = 0; c < sizeof(warm)/sizeof(warm[0]); c++ )
   {
      double optIn[4];
      int cold = TA_MAENV_Lookback( 9, 2.5, 10.0, warm[c].maType );
      int moved;

      optIn[0] = 9.0;
      optIn[1] = 2.5;
      optIn[2] = 10.0;
      optIn[3] = (double)warm[c].maType;
      TA_SetUnstablePeriod( warm[c].id, warm[c].unst );
      moved = TA_MAENV_Lookback( 9, 2.5, 10.0, warm[c].maType ) - cold;
      e = maenv_form_cell( history->close, nbBars, warm[c].maType, 9, 2.5, 10.0, 1 );
      if( e == TA_TEST_PASS && server_verify_active() )
         e = server_verify_lookback_value( "MAENV", optIn, 4 );
      TA_SetUnstablePeriod( warm[c].id, 0 );
      if( e != TA_TEST_PASS ) return e;
      if( moved != (int)warm[c].unst )
      {
         printf( "MAENV form Fail [type %d]: unstable period %u moved the lookback by %d\n",
                 (int)warm[c].maType, warm[c].unst, moved );
         return TA_TESTUTIL_TFRR_BAD_BEGIDX;
      }
   }

   /* On a negative series the upper band is the lower line: the bands are
    * named by their percentage, not their order. */
   for( i = 0; i < nbBars; i++ )
      neg[i] = -history->close[i];

   retCode = TA_MAENV( 0, nbBars - 1, neg, 20, 10.0, 10.0, TA_MAType_SMA,
                       &begIdx, &nbElement, outUp, outMid, outLo );
   if( retCode != TA_SUCCESS || nbElement != nbBars - 19 )
   {
      printf( "MAENV negated Fail: rc=%d nb=%d\n", (int)retCode, nbElement );
      return retCode != TA_SUCCESS ? TA_TESTUTIL_TFRR_BAD_RETCODE
                                   : TA_TESTUTIL_TFRR_BAD_OUTNBELEMENT;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( !( outUp[i] < outLo[i] ) )
      {
         printf( "MAENV negated Fail at bar %d: upper %.17g not below lower %.17g\n",
                 begIdx + i, outUp[i], outLo[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
   }

   e = maenv_form_cell( neg, nbBars, TA_MAType_SMA, 20, 10.0, 10.0, 1 );
   if( e != TA_TEST_PASS ) return e;
   return maenv_form_cell( neg, nbBars, TA_MAType_EMA, 9, -2.5, 4.0, 1 );
}
