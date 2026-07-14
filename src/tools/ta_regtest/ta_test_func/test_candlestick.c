/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  082304 MF   First version.
 *  041305 MF   Add latest list of function.
 */

/* Description:
 *     Test functions for candlestick.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
#define MAX_OPTIN_PARAM    5
#define MAX_TESTED_OUTPUT  3

TA_RetCode TA_SetCandleSettings( TA_CandleSettingType settingType,
                                 TA_RangeType rangeType,
                                 int avgPeriod,
                                 double factor );

typedef struct
{
   TA_RangeType bodyLong_type;
   int          bodyLong_avg;
   double       bodyLong_factor;
   TA_RangeType bodyVeryLong_type;
   int          bodyVeryLong_avg;
   double       bodyVeryLong_factor;
   TA_RangeType bodyShort_type;
   int          bodyShort_avg;
   double       bodyShort_factor;
   TA_RangeType bodyDoji_type;
   int          bodyDoji_avg;
   double       bodyDoji_factor;
   TA_RangeType shadowLong_type;
   int          shadowLong_avg;
   double       shadowLong_factor;
   TA_RangeType shadowVeryLong_type;
   int          shadowVeryLong_avg;
   double       shadowVeryLong_factor;
   TA_RangeType shadowShort_type;
   int          shadowShort_avg;
   double       shadowShort_factor;
   TA_RangeType shadowVeryShort_type;
   int          shadowVeryShort_avg;
   double       shadowVeryShort_factor;
   TA_RangeType near_type;
   int          near_avg;
   double       near_factor;
   TA_RangeType far_type;
   int          far_avg;
   double       far_factor;
} TA_CDLGlobals;

typedef struct
{
   int index;
   int value;
} TA_ExpectedOutput;


typedef struct
{
   /* Indicate which function will be called */
   const char *name;

   /* Indicate if ranging test should be done.
    * (These tests are very time consuming).
    */
   int doRangeTestFlag;

   /* Range for the function call.
    * When both value are -1 a series of automated range
    * tests are performed.
    */
   TA_Integer startIdx;
   TA_Integer endIdx;

   /* Up to 5 parameters depending of functions.
    * Will be converted to integer when input is integer.
    */
   TA_Real params[MAX_OPTIN_PARAM];

   /* The expected return code. */
   TA_RetCode expectedRetCode;

   /* When return code is TA_SUCCESS, the following output's
    * element are verified.
    */
   TA_ExpectedOutput output[MAX_TESTED_OUTPUT];
} TA_Test;


typedef struct
{
   /* Allows to pass key information as an
    * opaque parameter for doRangeTest.
    */
   const TA_Test *test;
   const TA_Real *open;
   const TA_Real *high;
   const TA_Real *low;
   const TA_Real *close;

   TA_ParamHolder *paramHolder;
} TA_RangeTestParam;

/**** Local functions declarations.    ****/
static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test );

static ErrorNumber callCandlestick( TA_ParamHolder **paramHolderPtr,
                                    const char   *name,
                                    int           startIdx,
                                    int           endIdx,
                                    const double *inOpen,
                                    const double *inHigh,
                                    const double *inLow,
                                    const double *inClose,
                                    const double  optInArray[],
                                    int          *outBegIdx,
                                    int          *outNbElement,
                                    int           outInteger[],
                                    int          *lookback,
                                    TA_RetCode   *retCode );
/**** Local variables definitions.     ****/

/* Some set of globals */

/* List of test to perform. */
static TA_Test tableTest[] =
{
   { "CDL2CROWS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDL3BLACKCROWS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDL3INSIDE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDL3LINESTRIKE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDL3OUTSIDE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDL3STARSINSOUTH",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDL3WHITESOLDIERS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLABANDONEDBABY",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLADVANCEBLOCK",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLBELTHOLD",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLBREAKAWAY",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLCLOSINGMARUBOZU",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLCONCEALBABYSWALL",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLCOUNTERATTACK",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLDARKCLOUDCOVER",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLDOJI",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLDOJISTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLDRAGONFLYDOJI",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLENGULFING",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLEVENINGDOJISTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLEVENINGSTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLGAPSIDESIDEWHITE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLGRAVESTONEDOJI",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHAMMER",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHANGINGMAN",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHARAMI",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHARAMICROSS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHIKKAKE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHIKKAKEMOD",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHIGHWAVE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLHOMINGPIGEON",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLIDENTICAL3CROWS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLINNECK",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLINVERTEDHAMMER",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLKICKING",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLKICKINGBYLENGTH",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLLADDERBOTTOM",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLLONGLEGGEDDOJI",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLLONGLINE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLMARUBOZU",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLMATCHINGLOW",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLMATHOLD",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLMORNINGDOJISTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLMORNINGSTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLONNECK",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLPIERCING",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLRICKSHAWMAN",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLRISEFALL3METHODS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLSEPARATINGLINES",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLSHOOTINGSTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLSHORTLINE",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLSPINNINGTOP",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLSTALLEDPATTERN",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLSTICKSANDWICH",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLTAKURI",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLTASUKIGAP",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLTHRUSTING",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLTRISTAR",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLUNIQUE3RIVER",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLUPSIDEGAP2CROWS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }},
   { "CDLXSIDEGAP3METHODS",1, 0, 0, {0.0,0.0}, TA_SUCCESS, { {0,0}, {1,1} }}
};

#define NB_TEST (sizeof(tableTest)/sizeof(TA_Test))

/**** Global functions definitions.   ****/
/* ------------------------------------------------------------------------ *
 * Predicate-coverage (MC/DC) tests for the Hikkake candlesticks.
 *
 * A candlestick test can pass VACUOUSLY when the data never triggers the
 * pattern: the output is all-zero, and all-zero == all-zero regardless of the
 * implementation. That hides bugs in the pattern logic — exactly the risk when
 * a pattern is rewritten (e.g. for the streaming API). These deterministic
 * scenarios drive the ACTUAL TA function through detection (bull AND bear),
 * confirmation (in-window, out-of-window, predicate-false) and one variant per
 * structural predicate flipped false, asserting the exact integer output each
 * time, so every decision boundary is exercised in both directions. A run also
 * fails if any of the four output classes (+/-100, +/-200) is absent (vacuous).
 * The complementary differential coverage — current batch vs frozen v0.6.4, and
 * stream vs batch — runs on the same pattern geometry via fuzz_data.h's
 * FUZZ_CANDLE shape (fuzz-064 and stream_verify).
 * ------------------------------------------------------------------------ */
#define PB_N 512
static double pbO[PB_N], pbH[PB_N], pbL[PB_N], pbC[PB_N];
static int    pbCur;
static int    pbEi[80], pbEv[80], pbNe;
static const char *pbEl[80];

static int pb_bar4( double o, double h, double l, double c )
{
   pbO[pbCur]=o; pbH[pbCur]=h; pbL[pbCur]=l; pbC[pbCur]=c; return pbCur++;
}
/* body-as-a-point bar: O=C=mid, caller controls the high/low geometry */
static int pb_barm( double hi, double lo ) { double m=(hi+lo)/2.0; return pb_bar4(m,hi,lo,m); }
/* bar with an exact close v (O=C=v, valid candle) — for tight confirmation margins */
static int pb_close( double v ) { return pb_bar4(v, v+1.0, v-1.0, v); }
/* flat filler: constant high/low so it forms no inside-bar pattern and the
 * confirmation countdown expires between scenarios */
static void pb_flat( int k ) { while(k-->0) pb_bar4(100.0,101.0,99.0,100.0); }
static void pb_expect( int i, int v, const char *s ) { pbEi[pbNe]=i; pbEv[pbNe]=v; pbEl[pbNe]=s; pbNe++; }
static void pb_reset( void ) { pbCur=0; pbNe=0; }

/* HIKKAKE detection window (3 bars). dir +1 bull/-1 bear; each of p1/p2/p34/p56
 * flips one detection predicate false. Returns the detection (3rd) bar index. */
static int pb_hk_win( int dir, int p1, int p2, int p34, int p56 )
{
   double h2,l2,h3,l3;
   pb_barm(120.0, 80.0);                              /* 1st (widest) */
   h2 = p1 ? 121.0 : 115.0;                           /* P1: H[i-1] < H[i-2] */
   l2 = p2 ? 79.0  : 85.0;                            /* P2: L[i-1] > L[i-2] */
   pb_barm(h2, l2);                                   /* 2nd (inside 1st) */
   if( dir>0 ) { h3 = p34 ? h2+2.0 : h2-3.0; l3 = l2-5.0; }  /* bull breakout */
   else        { h3 = p56 ? h2-2.0 : h2+3.0; l3 = l2+5.0; }  /* bear breakout */
   if( h3 < l3 ) { double t=h3; h3=l3+1.0; l3=t; }
   return pb_barm(h3, l3);                            /* 3rd (detection) */
}

/* HIKKAKEMOD detection window (4 bars). dir +1/-1; brk 0=intact, 1=break the
 * 2nd-inside-1st nest, 3=break the 3rd-inside-2nd nest, 5=break the breakout.
 * The 2nd candle closes on its low (bull) / high (bear) => "close near" holds
 * for any candle setting. Returns the detection (4th) bar index. */
static int pb_mod_win( int dir, int brk )
{
   double h2,l2,c2,h3,l3,h4,l4;
   pb_bar4(100.0,130.0,70.0,100.0);                          /* 1st */
   h2 = (brk==1)? 131.0 : 120.0;  l2 = (brk==1)? 69.0 : 80.0;
   c2 = dir>0 ? l2 : h2;                                     /* close near low/high */
   pb_bar4(100.0,h2,l2,c2);                                  /* 2nd (inside 1st) */
   h3 = (brk==3)? h2+2.0 : 115.0;  l3 = 85.0;
   pb_bar4(100.0,h3,l3,100.0);                               /* 3rd (inside 2nd) */
   if( dir>0 ) { h4=(brk==5)? h3+2.0 : 112.0; l4=75.0; }     /* bull breakout */
   else        { h4=(brk==5)? h3-2.0 : 118.0; l4=95.0; }     /* bear breakout */
   return pb_barm(h4,l4);                                    /* 4th (detection) */
}

typedef TA_RetCode (*PbCdlFn)(int,int,const double*,const double*,const double*,const double*,int*,int*,int*);

static ErrorNumber pb_check( const char *name, PbCdlFn fn )
{
   int out[PB_N], begIdx=0, nb=0, k, fails=0;
   int s1=0, sn1=0, s2=0, sn2=0;
   TA_RetCode rc = fn(0, pbCur-1, pbO, pbH, pbL, pbC, &begIdx, &nb, out);
   if( rc != TA_SUCCESS ) { printf("  %s predicate test: retCode %d\n", name, rc); return TA_TSTCDL_PREDICATE_MISMATCH; }
   for( k=0; k<nb; k++ ) { int v=out[k]; if(v==100)s1=1; else if(v==-100)sn1=1; else if(v==200)s2=1; else if(v==-200)sn2=1; }
   for( k=0; k<pbNe; k++ )
   {
      int oi = pbEi[k]-begIdx;
      int got = (oi>=0 && oi<nb) ? out[oi] : -99999;
      if( got != pbEv[k] )
      {
         printf("  %s PREDICATE FAIL bar=%d expected=%d got=%d  (%s)\n", name, pbEi[k], pbEv[k], got, pbEl[k]);
         fails++;
      }
   }
   if( fails ) return TA_TSTCDL_PREDICATE_MISMATCH;
   if( !(s1 && sn1 && s2 && sn2) )
   {
      printf("  %s PREDICATE VACUOUS: missing an output class (+100=%d -100=%d +200=%d -200=%d)\n", name, s1,sn1,s2,sn2);
      return TA_TSTCDL_PREDICATE_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* ---- MC/DC gate helpers for the marquee multi-candle patterns (issue #109) ---
 * Same idea as the Hikkake gate above, for single-sign patterns: a detection
 * scenario that fires the exact value, then one near-miss per structural
 * predicate (that predicate flipped false, the rest held) asserting 0 — so every
 * decision boundary is exercised in both directions. Each scenario self-primes
 * and is separated by flat filler so the candle-setting averages reset between
 * them. Every scenario was validated against the shipped library. */

/* Valid-candle bar: clamps high>=max(o,c), low<=min(o,c). Returns its index. */
static int pb_bar( double o, double h, double l, double c )
{
   double hi=h, lo=l;
   if(hi<o)hi=o; if(hi<c)hi=c;
   if(lo>o)lo=o; if(lo>c)lo=c;
   return pb_bar4(o,hi,lo,c);
}
/* k alternating small-body bars (real body ~bd, half-range ~hr) around base:
 * seeds the BodyLong/Short/Doji/shadow averages small, matching fuzz_cdl_primer. */
static void pb_primer( int k, double base, double bd, double hr )
{
   int i;
   for(i=0;i<k;i++){ double o=(i&1)?base:base+bd, c=(i&1)?base+bd:base;
      pb_bar(o, base+bd+hr, base-hr, c); }
}
/* Single-sign check: every pb_expect must match, AND at least one expected
 * NON-zero must actually fire (else the gate is vacuous). */
static ErrorNumber pb_check_mcdc( const char *name, PbCdlFn fn )
{
   int out[PB_N], begIdx=0, nb=0, k, fails=0, sawNonzero=0;
   TA_RetCode rc = fn(0, pbCur-1, pbO, pbH, pbL, pbC, &begIdx, &nb, out);
   if( rc != TA_SUCCESS ) { printf("  %s MC/DC: retCode %d\n", name, rc); return TA_TSTCDL_PREDICATE_MISMATCH; }
   for( k=0; k<pbNe; k++ )
   {
      int oi = pbEi[k]-begIdx;
      int got = (oi>=0 && oi<nb) ? out[oi] : -99999;
      if( got != pbEv[k] )
      {
         printf("  %s MC/DC FAIL bar=%d expected=%d got=%d  (%s)\n", name, pbEi[k], pbEv[k], got, pbEl[k]);
         fails++;
      }
      if( pbEv[k] != 0 && got == pbEv[k] ) sawNonzero = 1;
   }
   if( fails ) return TA_TSTCDL_PREDICATE_MISMATCH;
   if( !sawNonzero )
   {
      printf("  %s MC/DC VACUOUS: no expected non-zero output fired\n", name);
      return TA_TSTCDL_PREDICATE_VACUOUS;
   }
   return TA_TEST_PASS;
}

static ErrorNumber test_hikkake_predicate_coverage( void )
{
   ErrorNumber e;
   int d, c;

   /* ---------- CDLHIKKAKE ---------- */
   pb_reset();
   pb_flat(6);                                                   /* warm-up >= lookback(5) */
   d = pb_hk_win(+1,0,0,0,0); pb_expect(d,100,"bull detect");
   c = pb_close(117.0);       pb_expect(c,200,"bull confirm @117 (pins savedHigh vs i-2=120)"); pb_flat(6);
   d = pb_hk_win(-1,0,0,0,0); pb_expect(d,-100,"bear detect");
   c = pb_close(82.0);        pb_expect(c,-200,"bear confirm @82 (pins savedLow vs i-2=80)");   pb_flat(6);
   d = pb_hk_win(-1,0,0,0,0); pb_expect(d,-100,"bear detect");
   c = pb_close(87.0);        pb_expect(c,0,"bear no-confirm @87 (pins savedLow vs i=90)");     pb_flat(6);
   d = pb_hk_win(+1,0,0,0,0); pb_expect(d,100,"bull detect (i+3 confirm)");
   pb_barm(112.0,108.0); pb_barm(113.0,107.0);
   c = pb_barm(130.0,118.0);  pb_expect(c,200,"confirm at i+3 (edge in-window)"); pb_flat(6);
   d = pb_hk_win(+1,0,0,0,0); pb_expect(d,100,"bull detect (i+4 late)");
   pb_flat(3);
   c = pb_barm(130.0,118.0);  pb_expect(c,0,"confirm at i+4 (out of window -> 0)"); pb_flat(6);
   d = pb_hk_win(+1,1,0,0,0);  pb_expect(d,0,"break P1 (2nd lower high)");   pb_flat(6);
   d = pb_hk_win(+1,0,1,0,0);  pb_expect(d,0,"break P2 (2nd higher low)");   pb_flat(6);
   d = pb_hk_win(+1,0,0,1,0);  pb_expect(d,0,"break bull breakout");         pb_flat(6);
   d = pb_hk_win(-1,0,0,0,1);  pb_expect(d,0,"break bear breakout");         pb_flat(6);
   d = pb_hk_win(+1,0,0,0,0); pb_expect(d,100,"bull detect");
   c = pb_barm(114.0,112.0);  pb_expect(c,0,"confirm predicate false -> 0"); pb_flat(6);
   e = pb_check("CDLHIKKAKE", TA_CDLHIKKAKE);
   if( e != TA_TEST_PASS ) return e;

   /* ---------- CDLHIKKAKEMOD ----------
    * The confirmation reads the cached patternHigh/patternLow = inHigh/inLow[i-1]
    * (the 3rd candle: H=115 / L=85). Mis-cache candidates are inHigh/inLow[i]
    * (4th: H=112 / L=95) and [i-2] (2nd: H=120 / L=80). Tight-margin confirm
    * closes discriminate them, and a D+3-edge confirm pins the countdown SEED. */
   pb_reset();
   pb_flat(20);                                                  /* warm-up >= lookback + Near ring */
   d = pb_mod_win(+1,0); pb_expect(d,100,"mod bull detect");
   c = pb_close(117.0);  pb_expect(c,200,"mod bull confirm @117 (pins patternHigh vs i-2=120)");   pb_flat(8);
   d = pb_mod_win(+1,0); pb_expect(d,100,"mod bull detect");
   c = pb_close(114.0);  pb_expect(c,0,"mod no-confirm @114 (pins patternHigh vs i=112)");         pb_flat(8);
   d = pb_mod_win(-1,0); pb_expect(d,-100,"mod bear detect");
   c = pb_close(82.0);   pb_expect(c,-200,"mod bear confirm @82 (pins patternLow vs i-2=80)");     pb_flat(8);
   d = pb_mod_win(-1,0); pb_expect(d,-100,"mod bear detect");
   c = pb_close(90.0);   pb_expect(c,0,"mod no-confirm @90 (pins patternLow vs i=95)");            pb_flat(8);
   d = pb_mod_win(+1,0); pb_expect(d,100,"mod bull detect (i+3 edge)");
   pb_flat(2);
   c = pb_close(125.0);  pb_expect(c,200,"mod confirm at i+3 edge (pins patternCount=4)");         pb_flat(8);
   d = pb_mod_win(+1,1); pb_expect(d,0,"mod break 2nd-inside-1st");         pb_flat(8);
   d = pb_mod_win(+1,3); pb_expect(d,0,"mod break 3rd-inside-2nd");         pb_flat(8);
   d = pb_mod_win(+1,5); pb_expect(d,0,"mod break breakout");               pb_flat(8);
   d = pb_mod_win(+1,0); pb_expect(d,100,"mod bull detect (i+4 late)");
   pb_flat(3);
   c = pb_close(125.0);  pb_expect(c,0,"mod confirm at i+4 (out of window -> 0)"); pb_flat(8);
   e = pb_check("CDLHIKKAKEMOD", TA_CDLHIKKAKEMOD);
   if( e != TA_TEST_PASS ) return e;

   return TA_TEST_PASS;
}

/* CDL2CROWS MC/DC: detection (-100) + one flip per structural predicate. */
static void build_2crows( void )
{
  pb_flat(6);
  /* DETECTION: 1st long white, 2nd black gap-up, 3rd black opening in 2nd rb & closing in 1st rb */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);                 /* 1st: long white  body[100,105] rb=5>avg2 */
  pb_bar(110,111,106,107);                /* 2nd: black gap-up body[107,110] min107>105 */
  int d=pb_bar(109,110,102,103);          /* 3rd: black open109 in(107,110), close103 in(100,105) */
  pb_expect(d,-100,"detect");
  pb_flat(8);
  /* FLIP 1: break 1st-white==1  (make 1st BLACK)  [also breaks close3<close1 - coupled] */
  pb_primer(12,100,2,1);
  pb_bar(102,103,98,99);                  /* 1st BLACK body[99,102] rb=3>2 */
  pb_bar(110,111,106,107);
  int f1=pb_bar(109,110,102,103);
  pb_expect(f1,0,"break 1st-white");
  pb_flat(8);
  /* FLIP 2: break 1st-long  (rb==avg, not > ) */
  pb_primer(12,100,2,1);
  pb_bar(102,106,99,104);                 /* 1st white SHORT body[102,104] rb=2, 2>2 false */
  pb_bar(110,111,106,107);
  int f2=pb_bar(109,110,102,103);
  pb_expect(f2,0,"break 1st-long");
  pb_flat(8);
  /* FLIP 3: break 2nd-black==-1  (make 2nd WHITE)  [also breaks open3>close2 - coupled] */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,112,106,111);                /* 2nd WHITE body[110,111] */
  int f3=pb_bar(109,110,102,103);
  pb_expect(f3,0,"break 2nd-black");
  pb_flat(8);
  /* FLIP 4: break gap-up  (2nd body overlaps 1st: min104 <= max105) */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,103,104);                /* 2nd black body[104,110] min104 !> 105 */
  int f4=pb_bar(109,110,102,103);
  pb_expect(f4,0,"break gapup");
  pb_flat(8);
  /* FLIP 5: break 3rd-black==-1  (make 3rd WHITE)  [also breaks close3<close1 - coupled] */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f5=pb_bar(109,111,108,110);         /* 3rd WHITE body[109,110] */
  pb_expect(f5,0,"break 3rd-black");
  pb_flat(8);
  /* FLIP 6: break open3<open2  (open3=111 >= open2=110) */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f6=pb_bar(111,112,102,103);
  pb_expect(f6,0,"break open3<open2");
  pb_flat(8);
  /* FLIP 7: break open3>close2  (open3=106 <= close2=107) */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f7=pb_bar(106,107,102,103);
  pb_expect(f7,0,"break open3>close2");
  pb_flat(8);
  /* FLIP 8: break close3>open1  (close3=99 <= open1=100) */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f8=pb_bar(109,110,98,99);
  pb_expect(f8,0,"break close3>open1");
  pb_flat(8);
  /* FLIP 9: break close3<close1  (close3=106 >= close1=105) */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f9=pb_bar(109,110,104,106);
  pb_expect(f9,0,"break close3<close1");
  pb_flat(8);
}

/* CDL3BLACKCROWS MC/DC: detection (-100) + one flip per structural predicate. */
static void build_3blackcrows( void )
{
  pb_flat(6);
  /* ===== DETECTION =====
     A white, B/C/D three declining black crows, each opens within prior body,
     each near-zero lower shadow, A.high > B.close. */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);        /* A  white */
  pb_bar(103,103.5,101,101);      /* B  1st black */
  pb_bar(102,102.5,99,99);        /* C  2nd black */
  int d=pb_bar(101,101.5,97,97);  /* D  3rd black */
  pb_expect(d,-100,"detect");
  pb_flat(8);

  /* FLIP 1 : break p1  colorA==white -> make A black (only p1) */
  pb_primer(12,100,2,1);
  pb_bar(104,105,100,100);        /* A  black */
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f1=pb_bar(101,101.5,97,97);
  pb_expect(f1,0,"break p1 colorA-white");
  pb_flat(8);

  /* FLIP 2 : break p2  colorB==black -> B doji/white (p6 co-flips: coupled) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,103,103);      /* B  doji -> white */
  pb_bar(102,102.5,99,99);
  int f2=pb_bar(101,101.5,97,97);
  pb_expect(f2,0,"break p2 colorB-black");
  pb_flat(8);

  /* FLIP 3 : break p3  colorC==black -> C doji/white (p8,p10 co-flip: coupled) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,102,102);      /* C  doji -> white */
  int f3=pb_bar(101,101.5,97,97);
  pb_expect(f3,0,"break p3 colorC-black");
  pb_flat(8);

  /* FLIP 4 : break p4  colorD==black -> D doji/white (p11 co-flips: coupled) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f4=pb_bar(101,101.5,101,101); /* D  doji -> white */
  pb_expect(f4,0,"break p4 colorD-black");
  pb_flat(8);

  /* FLIP 5 : break p5  openC<openB -> openC above openB (only p5) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(103.5,104,99,99);        /* C  opens above B.open=103 */
  int f5=pb_bar(101,101.5,97,97);
  pb_expect(f5,0,"break p5 openC<openB");
  pb_flat(8);

  /* FLIP 6 : break p6  openC>closeB -> openC below closeB (only p6) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(100,100.5,99,99);        /* C  opens below B.close=101 */
  int f6=pb_bar(99.5,100,97,97);  /* D  re-fit inside C body (99,100) */
  pb_expect(f6,0,"break p6 openC>closeB");
  pb_flat(8);

  /* FLIP 7 : break p7  openD<openC -> openD above openC (only p7) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f7=pb_bar(102.5,103,97,97); /* D  opens above C.open=102 */
  pb_expect(f7,0,"break p7 openD<openC");
  pb_flat(8);

  /* FLIP 8 : break p8  openD>closeC -> openD below closeC (only p8) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f8=pb_bar(98,98.5,97,97);   /* D  opens below C.close=99 */
  pb_expect(f8,0,"break p8 openD>closeC");
  pb_flat(8);

  /* FLIP 9 : break p9  highA>closeB -> A.high below B.close (only p9) */
  pb_primer(12,100,2,1);
  pb_bar(100,100.9,100,100.8);    /* A  white, high 100.9 < B.close 101 */
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f9=pb_bar(101,101.5,97,97);
  pb_expect(f9,0,"break p9 highA>closeB");
  pb_flat(8);

  /* FLIP 10: break p10 closeB>closeC -> B.close below C.close (only p10) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,98.5,98.5);    /* B  close 98.5 <= C.close 99 */
  pb_bar(102,102.5,99,99);
  int f10=pb_bar(101,101.5,97,97);
  pb_expect(f10,0,"break p10 closeB>closeC");
  pb_flat(8);

  /* FLIP 11: break p11 closeC>closeD -> D.close above C.close (only p11) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f11=pb_bar(101,101.5,99.5,99.5); /* D  close 99.5 >= C.close 99 */
  pb_expect(f11,0,"break p11 closeC>closeD");
  pb_flat(8);

  /* FLIP 12: break p12 B lower shadow short -> long lower shadow (only p12) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,100,101);      /* B  lowershadow = 101-100 = 1 > ~0.4 */
  pb_bar(102,102.5,99,99);
  int f12=pb_bar(101,101.5,97,97);
  pb_expect(f12,0,"break p12 shadowB-short");
  pb_flat(8);

  /* FLIP 13: break p13 C lower shadow short -> long lower shadow (only p13) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,97,99);        /* C  lowershadow = 99-97 = 2 > ~0.4 */
  int f13=pb_bar(101,101.5,97,97);
  pb_expect(f13,0,"break p13 shadowC-short");
  pb_flat(8);

  /* FLIP 14: break p14 D lower shadow short -> long lower shadow (only p14) */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f14=pb_bar(101,101.5,95,97); /* D  lowershadow = 97-95 = 2 > ~0.4 */
  pb_expect(f14,0,"break p14 shadowD-short");
  pb_flat(8);
}

/* Predicate-coverage (MC/DC) gate for the marquee multi-candle patterns. Runs
 * the actual TA function over detection + per-predicate near-miss scenarios and
 * asserts the exact integer output at each (the rarer multi-candle patterns that
 * random fuzz data never triggers). Complements the FUZZ_CANDLE differential/
 * stream coverage; extended one pattern at a time (issue #109). */
static ErrorNumber test_marquee_predicate_coverage( void )
{
   ErrorNumber e;
   pb_reset(); build_2crows();      e = pb_check_mcdc("CDL2CROWS",      TA_CDL2CROWS);      if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3blackcrows(); e = pb_check_mcdc("CDL3BLACKCROWS", TA_CDL3BLACKCROWS); if( e != TA_TEST_PASS ) return e;
   return TA_TEST_PASS;
}

ErrorNumber test_candlestick( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* Predicate-coverage (MC/DC) gate: prove the pattern logic is exercised
    * (non-vacuously) and each decision boundary is correct, before the
    * data-driven table tests below. */
   retValue = test_hikkake_predicate_coverage();
   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed: Hikkake predicate-coverage test (retValue=%d)\n", retValue );
      return retValue;
   }

   /* MC/DC gate for the marquee multi-candle patterns (issue #109). */
   retValue = test_marquee_predicate_coverage();
   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed: marquee predicate-coverage test (retValue=%d)\n", retValue );
      return retValue;
   }

   /* Initialize all the unstable period with a large number that would
    * break the logic if a candlestick unexpectably use a function affected
    * by an unstable period.
    */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 20000 );

   /* Perform sequentialy all the tests. */
   for( i=0; i < NB_TEST; i++ )
   {
      retValue = do_test( history, &tableTest[i] );
      if( retValue != 0 )
      {
         printf( "Failed Test #%d for %s (retValue=%d)\n", i, tableTest[i].name, retValue );
         return retValue;
      }
   }

   /* Re-initialize all the unstable period to zero. */
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   /* All tests succeed. */
   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* Abstract call for all candlestick functions.
 *
 * Call the function by 'name'.
 *
 * Optional inputs are pass as an array of double.
 * Elements will be converted to integer as needed.
 *
 * All outputs are returned in the remaining parameters.
 *
 * 'lookback' is the return value of the corresponding Lookback function.
 * taFuncRetCode is the return code from the call of the TA function.
 *
 */
static ErrorNumber callCandlestick( TA_ParamHolder **paramHolderPtr,
                                    const char   *name,
                                    int           startIdx,
                                    int           endIdx,
                                    const double *inOpen,
                                    const double *inHigh,
                                    const double *inLow,
                                    const double *inClose,
                                    const double  optInArray[],
                                    int          *outBegIdx,
                                    int          *outNbElement,
                                    int           outInteger[],
                                    int          *lookback,
                                    TA_RetCode   *taFuncRetCode )
{

   /* Use the abstract interface to call the function by name. */
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outputInfo;

   TA_RetCode retCode;

   (void)optInArray;

   /* Speed optimization if paramHolder is already initialized. */
   paramHolder = *paramHolderPtr;
   if( !paramHolder )
   {
      retCode = TA_GetFuncHandle( name, &handle );
      if( retCode != TA_SUCCESS )
      {
         printf( "Can't get the function handle [%d]\n", retCode );
         return TA_TSTCDL_GETFUNCHANDLE_FAIL;
      }

      retCode = TA_ParamHolderAlloc( handle, &paramHolder );
      if( retCode != TA_SUCCESS )
      {
         printf( "Can't allocate the param holder [%d]\n", retCode );
         return TA_TSTCDL_PARAMHOLDERALLOC_FAIL;
      }

      *paramHolderPtr = paramHolder;
      TA_GetFuncInfo( handle, &funcInfo );

      /* Verify that the input are only OHLC. */
      if( funcInfo->nbInput != 1 )
      {
         printf( "Candlestick are expected to use only OHLC as input.\n" );
         return TA_TSTCDL_NBINPUT_WRONG;
      }

      TA_GetInputParameterInfo( handle, 0, &inputInfo );

      if( inputInfo->type != TA_Input_Price )
      {
         printf( "Candlestick are expected to use only OHLC as input.\n" );
         return TA_TSTCDL_INPUT_TYPE_WRONG;
      }

      if( inputInfo->flags != (TA_IN_PRICE_OPEN |
                               TA_IN_PRICE_HIGH |
                               TA_IN_PRICE_LOW  |
                               TA_IN_PRICE_CLOSE) )
      {
         printf( "Candlestick are expected to use only OHLC as input.\n" );
         return TA_TSTCDL_INPUT_FLAG_WRONG;
      }

      /* Set the optional inputs. */

      /* Verify that there is only one output. */
      if( funcInfo->nbOutput != 1 )
      {
         printf( "Candlestick are expected to have only one output array.\n" );
         return TA_TSTCDL_NBOUTPUT_WRONG;
      }

      TA_GetOutputParameterInfo( handle, 0, &outputInfo );
      if( outputInfo->type != TA_Output_Integer )
      {
         printf( "Candlestick are expected to have only one output array of type integer.\n" );
         return TA_TSTCDL_OUTPUT_TYPE_WRONG;
      }

      /* !!!!!!!!!!!!! TO BE DONE !!!!!!!!!!!!!!!!!!
       * For now all candlestick functions will be called with default optional parameter.
       */
   }

   /* Set the input buffers. */
   TA_SetInputParamPricePtr( paramHolder, 0,
                             inOpen, inHigh, inLow, inClose, NULL, NULL );

   TA_SetOutputParamIntegerPtr(paramHolder,0,outInteger);


   /* Do the function call. */
   *taFuncRetCode = TA_CallFunc(paramHolder,startIdx,endIdx,outBegIdx,outNbElement);

   if( *taFuncRetCode != TA_SUCCESS )
   {
      printf( "TA_CallFunc() failed [%d]\n", *taFuncRetCode );
      TA_ParamHolderFree( paramHolder );
      return TA_TSTCDL_CALLFUNC_FAIL;
   }

   /* Do the lookback function call. */
   retCode = TA_GetLookback( paramHolder, lookback );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_GetLookback() failed [%d]\n", retCode );
      TA_ParamHolderFree( paramHolder );
      return TA_TSTCDL_GETLOOKBACK_FAIL;
   }

   return TA_TEST_PASS;
}

/* rangeTestFunction is a different way to call any of
 * the TA function.
 *
 * This is called by doRangeTest found in test_util.c
 *
 * The doRangeTest verifies behavior that should be common
 * for ALL TA functions. It detects bugs like:
 *   - outBegIdx, outNbElement and lookback inconsistency.
 *   - off-by-one writes to output.
 *   - output inconsistency for different start/end index.
 *   - ... many other limit cases...
 *
 * In the case of candlestick, the output is integer and
 * should be put in outputBufferInt, and outputBuffer is
 * ignored.
 */
static TA_RetCode rangeTestFunction( TA_Integer   startIdx,
                                     TA_Integer   endIdx,
                                     TA_Real     *outputBuffer,
                                     TA_Integer  *outputBufferInt,
                                     TA_Integer  *outBegIdx,
                                     TA_Integer  *outNbElement,
                                     TA_Integer  *lookback,
                                     void        *opaqueData,
                                     unsigned int outputNb,
                                     unsigned int *isOutputInteger )
{
   TA_RangeTestParam *testParam1;
   const TA_Test *testParam2;
   ErrorNumber errNb;

   TA_RetCode retCode;

   (void)outputBuffer;
   (void)outputNb;

   testParam1 = (TA_RangeTestParam *)opaqueData;
   testParam2 = (const TA_Test *)testParam1->test;

   *isOutputInteger = 1; /* Must be != 0 */

   retCode = TA_INTERNAL_ERROR(166);

   /* Call the TA function by name */
   errNb = callCandlestick( &testParam1->paramHolder,
                            testParam2->name,
                            startIdx, endIdx,
                            testParam1->open,
                            testParam1->high,
                            testParam1->low,
                            testParam1->close,
                            testParam2->params,
                            outBegIdx,
                            outNbElement,
                            outputBufferInt,
                            lookback,
                            &retCode );

   if( errNb != TA_TEST_PASS )
      retCode = TA_INTERNAL_ERROR(168);

   return retCode;
}

static ErrorNumber do_test( const TA_History *history,
                            const TA_Test *test )
{
   TA_RangeTestParam testParam;
   ErrorNumber errNb;
   TA_RetCode retCode;

   (void)test;

   /* Set to NAN all the elements of the gBuffers.  */
   clearAllBuffers();

   /* Build the input. */
   setInputBuffer( 0, history->open,  history->nbBars );
   setInputBuffer( 1, history->high,  history->nbBars );
   setInputBuffer( 2, history->low,   history->nbBars );
   setInputBuffer( 3, history->close, history->nbBars );


#if 0
   /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
   /* Test for specific value not yet implemented */

   /* Make a simple first call. */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        gBuffer[1].in,
                        gBuffer[2].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].out0 );
      break;

   case TA_WILLR_TEST:
      retCode = TA_WILLR( test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInTimePeriod,
                          &outBegIdx,
                          &outNbElement,
                          gBuffer[0].out0 );
      break;

   default:
      retCode = TA_INTERNAL_ERROR(133);
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].out0, 0 );

   outBegIdx = outNbElement = 0;

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        gBuffer[1].in,
                        gBuffer[2].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[0].in );
      break;
   case TA_WILLR_TEST:
      retCode = TA_WILLR( test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInTimePeriod,
                          &outBegIdx,
                          &outNbElement,
                          gBuffer[0].in );
      break;
   default:
      retCode = TA_INTERNAL_ERROR(134);
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call to TA_MA should have the same output
    * as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[0].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[0].in, 0 );
   setInputBuffer( 0, history->high,  history->nbBars );

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        gBuffer[1].in,
                        gBuffer[2].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[1].in );
      break;
   case TA_WILLR_TEST:
      retCode = TA_WILLR( test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInTimePeriod,
                          &outBegIdx,
                          &outNbElement,
                          gBuffer[1].in );
      break;
   default:
      retCode = TA_INTERNAL_ERROR(135);
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[2].in, history->close,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call should have the same output as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[1].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[1].in, 0 );
   setInputBuffer( 1, history->low,   history->nbBars );

   /* Make another call where the input and the output are the
    * same buffer.
    */
   switch( test->theFunction )
   {
   case TA_CCI_TEST:
      retCode = TA_CCI( test->startIdx,
                        test->endIdx,
                        gBuffer[0].in,
                        gBuffer[1].in,
                        gBuffer[2].in,
                        test->optInTimePeriod,
                        &outBegIdx,
                        &outNbElement,
                        gBuffer[2].in );
      break;
   case TA_WILLR_TEST:
      retCode = TA_WILLR( test->startIdx,
                          test->endIdx,
                          gBuffer[0].in,
                          gBuffer[1].in,
                          gBuffer[2].in,
                          test->optInTimePeriod,
                          &outBegIdx,
                          &outNbElement,
                          gBuffer[2].in );
      break;
   default:
      retCode = TA_INTERNAL_ERROR(136);
   }

   /* Check that the input were preserved. */
   errNb = checkDataSame( gBuffer[0].in, history->high,history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;
   errNb = checkDataSame( gBuffer[1].in, history->low, history->nbBars );
   if( errNb != TA_TEST_PASS )
      return errNb;

   /* The previous call to TA_MA should have the same output
    * as this call.
    *
    * checkSameContent verify that all value different than NAN in
    * the first parameter is identical in the second parameter.
    */
   errNb = checkSameContent( gBuffer[0].out0, gBuffer[2].in );
   if( errNb != TA_TEST_PASS )
      return errNb;

   CHECK_EXPECTED_VALUE( gBuffer[2].in, 0 );
   setInputBuffer( 2, history->close, history->nbBars );
#endif

   /* Do a systematic test of most of the
    * possible startIdx/endIdx range.
    */
   testParam.test  = test;
   testParam.open  = history->open;
   testParam.high  = history->high;
   testParam.low   = history->low;
   testParam.close  = history->close;
   testParam.paramHolder = NULL;

   if( test->doRangeTestFlag )
   {

      errNb = doRangeTest( rangeTestFunction,
                           TA_FUNC_UNST_NONE,
                           (void *)&testParam, 1, 0 );

      if( testParam.paramHolder )
      {
         retCode = TA_ParamHolderFree( testParam.paramHolder );
         if( retCode != TA_SUCCESS )
         {
            printf( "TA_ParamHolderFree failed [%d]\n", retCode );
            return TA_TSTCDL_PARAMHOLDERFREE_FAIL;
         }
      }

      if( errNb != TA_TEST_PASS )
         return errNb;
   }

   return TA_TEST_PASS;
}
