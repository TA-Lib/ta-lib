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
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "../server_verify.h"

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
   /* Equal completes the set. The struct sat here for two decades with ten of
    * the eleven settings and no reader at all (#216 gap 2); a matrix missing
    * Equal could never vary what CDLTASUKIGAP / CDLSTICKSANDWICH read. */
   TA_RangeType equal_type;
   int          equal_avg;
   double       equal_factor;
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

/* ------------------------------------------------------------------------ *
 * The candle-settings matrix (#216 gap 2/3, on the #215 transport).
 *
 * Every candlestick in this file has only ever run at the DEFAULT settings, in
 * every language. `TA_CDLGlobals` above was declared for this and then never
 * read. That left two holes at once: the eleven thresholds had no C-side
 * coverage away from their defaults, and — because no server could be told to
 * change them until #215 — no cross-language comparison could either.
 *
 * The matrix below is the C side; server_verify() carries the same settings to
 * every language server (sync_candle_settings), so one sweep closes both.
 *
 * Row 0 IS the defaults, and is the baseline every other row is diffed against
 * rather than a redundant re-run: a row that moves no output is a row that
 * tested nothing, and the run fails if none of them move.
 */
static const TA_CDLGlobals cdlGlobalsMatrix[] =
{
   /* 0: the documented defaults, verbatim from TA_RestoreCandleDefaultSettings. */
   { TA_RangeType_RealBody, 10, 1.0,    /* BodyLong        */
     TA_RangeType_RealBody, 10, 3.0,    /* BodyVeryLong    */
     TA_RangeType_RealBody, 10, 1.0,    /* BodyShort       */
     TA_RangeType_HighLow,  10, 0.1,    /* BodyDoji        */
     TA_RangeType_RealBody,  0, 1.0,    /* ShadowLong      */
     TA_RangeType_RealBody,  0, 2.0,    /* ShadowVeryLong  */
     TA_RangeType_Shadows,  10, 1.0,    /* ShadowShort     */
     TA_RangeType_HighLow,  10, 0.1,    /* ShadowVeryShort */
     TA_RangeType_HighLow,   5, 0.2,    /* Near            */
     TA_RangeType_HighLow,   5, 0.6,    /* Far             */
     TA_RangeType_HighLow,   5, 0.05 }, /* Equal           */

   /* 1: every setting measured against Shadows. Reaches the arm that only
    * ShadowShort reaches by default -- the arm #217 found spelled two ways. */
   { TA_RangeType_Shadows, 10, 1.0,
     TA_RangeType_Shadows, 10, 3.0,
     TA_RangeType_Shadows, 10, 1.0,
     TA_RangeType_Shadows, 10, 0.1,
     TA_RangeType_Shadows,  0, 1.0,
     TA_RangeType_Shadows,  0, 2.0,
     TA_RangeType_Shadows, 10, 1.0,
     TA_RangeType_Shadows, 10, 0.1,
     TA_RangeType_Shadows,  5, 0.2,
     TA_RangeType_Shadows,  5, 0.6,
     TA_RangeType_Shadows,  5, 0.05 },

   /* 2: every averaging window collapsed to 0 -- the "instant candle" branch,
    * where ta_candleaverage reads the CURRENT bar instead of the running sum.
    * Also drives every lookback that is an avgPeriod down to its floor. */
   { TA_RangeType_RealBody, 0, 1.0,
     TA_RangeType_RealBody, 0, 3.0,
     TA_RangeType_RealBody, 0, 1.0,
     TA_RangeType_HighLow,  0, 0.1,
     TA_RangeType_RealBody, 0, 1.0,
     TA_RangeType_RealBody, 0, 2.0,
     TA_RangeType_Shadows,  0, 1.0,
     TA_RangeType_HighLow,  0, 0.1,
     TA_RangeType_HighLow,  0, 0.2,
     TA_RangeType_HighLow,  0, 0.6,
     TA_RangeType_HighLow,  0, 0.05 },

   /* 3: long windows and permissive factors -- patterns that almost never fire
    * at the defaults start firing, which is the direction all-zero output
    * cannot detect. */
   { TA_RangeType_HighLow, 25, 0.25,
     TA_RangeType_HighLow, 25, 0.5,
     TA_RangeType_HighLow, 25, 2.0,
     TA_RangeType_RealBody,25, 0.9,
     TA_RangeType_HighLow, 20, 0.2,
     TA_RangeType_HighLow, 20, 0.3,
     TA_RangeType_RealBody,25, 0.2,
     TA_RangeType_RealBody,25, 0.9,
     TA_RangeType_RealBody,12, 0.9,
     TA_RangeType_RealBody,12, 0.05,
     TA_RangeType_RealBody,12, 0.9 },

   /* 4: strict factors -- the opposite direction, so a pattern that stops
    * firing is visible too, and a negative factor because that is legal and
    * ta_global.c used to claim it "never matches" when in fact it makes a
    * `range > factor*avg` test unconditionally TRUE.
    *
    * The negative sits on Far, and the earlier claim here that Far's comparison
    * is "the one place that shows up" was wrong: Far is used as
    * `rb(b2) > rb(b1) - Far`, where a negative makes the test HARDER, not
    * unconditionally true. The per-setting sweep is what settled it -- Far moves
    * 2 pairs, which is both of the two functions that read it, and none of them
    * through this row's sign. Kept for the strict-factor direction it does
    * provide; the unconditional-true case is exercised via BodyLong/BodyShort. */
   { TA_RangeType_RealBody, 5, 4.0,
     TA_RangeType_RealBody, 5, 9.0,
     TA_RangeType_RealBody, 5, 0.05,
     TA_RangeType_HighLow,  5, 0.01,
     TA_RangeType_RealBody, 3, 4.0,
     TA_RangeType_RealBody, 3, 8.0,
     TA_RangeType_Shadows,  5, 0.05,
     TA_RangeType_HighLow,  5, 0.01,
     TA_RangeType_HighLow,  3, 0.02,
     TA_RangeType_HighLow,  3, -1.0,
     TA_RangeType_HighLow,  3, 0.005 },
};

#define NB_CDL_GLOBALS (sizeof(cdlGlobalsMatrix)/sizeof(TA_CDLGlobals))

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
/* PB_N sizes the scenario tape. It is deliberately far larger than any builder
 * needs: a builder is primer + pattern + flat filler per scenario, and a
 * pattern with ~20 conditions needs a detect, a flip and a control for each.
 * 512 fitted the six builders that existed when this was written and nothing
 * else; the ceiling is not a budget to spend, it is headroom. The four migrated
 * builders use 305/558/719/765 bars, and the largest plausible future one
 * (CDLHIKKAKEMOD, ~27 conditions over 4 pattern bars) computes to ~1330 -- so
 * 4096 is about 3x the worst case rather than the 1.5x that 2048 would be.
 *
 * The bound is CHECKED now. pb_bar4 and pb_expect used to write past the end of
 * these static arrays with no guard at all -- silent memory corruption, not an
 * assert -- which was survivable only because six builders happened to fit. */
#define PB_N      4096
#define PB_MAXEXP  256
#define PB_MAXWAIVE 64
#define PB_MAXCOND  64
#define PB_MAXCLASS  8
static double pbO[PB_N], pbH[PB_N], pbL[PB_N], pbC[PB_N];
static int    pbCur;
static int    pbOverflow;            /* sticky: a builder ran past a buffer */
static int    pbEi[PB_MAXEXP], pbEv[PB_MAXEXP], pbNe;
static const char *pbEl[PB_MAXEXP];

/* What each recorded expectation IS, which is what turns this from a golden
 * table into an MC/DC gate. See pb_check_mcdc for the contract each kind
 * carries. PB_LEGACY is the un-migrated pb_expect used by the Hikkake
 * predicate tests, which are not MC/DC and assert their own output classes. */
#define PB_LEGACY  0
#define PB_DETECT  1
#define PB_FLIP    2
#define PB_CONTROL 3
static int pbEkind[PB_MAXEXP];
static int pbEcond[PB_MAXEXP];       /* condition id for FLIP/CONTROL, else -1 */

/* Conditions the pattern is DECLARED to have, and the ones deliberately not
 * given a flip case. Every declared condition must be either flipped or
 * waived -- that equality is what stops a condition from being silently
 * skipped, which no counter of "cases written" can detect. */
static int         pbNbCond;
static int         pbWcond[PB_MAXWAIVE];
static const char *pbWhy[PB_MAXWAIVE];
static int         pbNw;

/* Distinct non-zero values the pattern's firing arm can emit: 1 for a pattern
 * that hard-codes 100 or -100, 2 for one whose arm is ta_candlecolor(...)*100.
 * Declared by the builder, defaulted to 1, and pinned against the pattern
 * source by scripts/check_mcdc_conditions.py -- so a bi-signed pattern that
 * never declares it fails the static check rather than quietly covering half
 * its decision. See the coverage check in pb_check_mcdc for why. */
static int         pbNbSigns;

static int pb_bar4( double o, double h, double l, double c )
{
   if( pbCur >= PB_N )
   {
      /* Do not write. Return a valid index so the caller's arithmetic stays in
       * bounds; pbOverflow is what fails the run. */
      pbOverflow = 1;
      return PB_N - 1;
   }
   pbO[pbCur]=o; pbH[pbCur]=h; pbL[pbCur]=l; pbC[pbCur]=c; return pbCur++;
}
/* body-as-a-point bar: O=C=mid, caller controls the high/low geometry */
static int pb_barm( double hi, double lo ) { double m=(hi+lo)/2.0; return pb_bar4(m,hi,lo,m); }
/* bar with an exact close v (O=C=v, valid candle) — for tight confirmation margins */
static int pb_close( double v ) { return pb_bar4(v, v+1.0, v-1.0, v); }
/* flat filler: constant high/low so it forms no inside-bar pattern and the
 * confirmation countdown expires between scenarios */
static void pb_flat( int k ) { while(k-->0) pb_bar4(100.0,101.0,99.0,100.0); }
/* Record one expectation. `kind` and `cond` are what pb_check_mcdc enforces;
 * pb_expect keeps the un-kinded legacy shape for the Hikkake predicate tests. */
static void pb_record( int i, int v, const char *s, int kind, int cond )
{
   if( pbNe >= PB_MAXEXP ) { pbOverflow = 1; return; }
   pbEi[pbNe]=i; pbEv[pbNe]=v; pbEl[pbNe]=s; pbEkind[pbNe]=kind; pbEcond[pbNe]=cond;
   pbNe++;
}
static void pb_expect( int i, int v, const char *s ) { pb_record(i, v, s, PB_LEGACY, -1); }

/* --- The MC/DC scenario vocabulary -------------------------------------- *
 *
 * pb_conditions(n)  declare how many atomic conditions the pattern's detection
 *                   decision has. Every one must end up either flipped or
 *                   waived; pb_check_mcdc fails if the totals disagree.
 * pb_signs(n)       declare how many distinct non-zero values the pattern can
 *                   emit (1, or 2 for a ta_candlecolor(...)*100 arm). Defaults
 *                   to 1. Every declared class must be exercised by a detect or
 *                   a control -- see the coverage check in pb_check_mcdc.
 * pb_detect(i,v,s)  bars satisfying EVERY condition: the pattern must fire.
 * pb_flip(i,c,s)    the detect bars with condition `c` broken: must NOT fire.
 * pb_control(i,v,c,s)
 *                   the FLIP bars with only the target quantity moved back
 *                   across its threshold: must fire again.
 * pb_waive(c,why)   condition `c` cannot be broken alone by any valid OHLC
 *                   input. A claim, not an excuse -- refutable by anyone who
 *                   can produce a passing flip+control pair for it.
 *
 * The control is the load-bearing half and the reason this is a gate rather
 * than a freeze. A flip case asserts 0, but 0 is also what a distant miss
 * returns, what a DIFFERENT accidentally-broken condition returns, and what
 * the pattern returns nearly everywhere on nearly any data. Pairing it with a
 * control that fires proves the zero is attributable to the one quantity the
 * case claims to be testing. Without it a wrong case is green today, green
 * tomorrow, and still green after a rewrite breaks the very predicate it was
 * written to guard.
 */
static void pb_conditions( int n ) { pbNbCond = n; }
static void pb_signs( int n ) { pbNbSigns = n; }
static void pb_detect( int i, int v, const char *s ) { pb_record(i, v, s, PB_DETECT, -1); }
static void pb_flip( int i, int cond, const char *s ) { pb_record(i, 0, s, PB_FLIP, cond); }
/* Ids are checked at pb_check_mcdc against pbNbCond, which a builder may declare
 * after its first pb_flip; validating here would depend on call order. */
static void pb_control( int i, int v, int cond, const char *s )
{
   pb_record(i, v, s, PB_CONTROL, cond);
}
static void pb_waive( int cond, const char *why )
{
   int k;
   if( pbNw >= PB_MAXWAIVE ) { pbOverflow = 1; return; }
   /* A duplicate waiver inflates the total while covering one condition, and a
    * waiver with no reason is not a claim anybody can refute. */
   for( k = 0; k < pbNw; k++ )
      if( pbWcond[k] == cond ) { pbOverflow = 1; return; }
   if( !why ) { pbOverflow = 1; return; }
   pbWcond[pbNw] = cond; pbWhy[pbNw] = why; pbNw++;
}

static void pb_reset( void )
{
   pbCur=0; pbNe=0; pbNw=0; pbNbCond=0; pbNbSigns=1; pbOverflow=0;
}

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
   TA_RetCode rc;
   /* Same guard pb_check_mcdc carries. It was added to only one of the two
    * consumers, so an overflowing Hikkake tape lost expectations and still
    * returned PASS. */
   if( pbOverflow )
   {
      printf("  %s predicate test: builder overflowed a harness buffer "
             "(bars=%d/%d expectations=%d/%d)\n", name, pbCur, PB_N, pbNe, PB_MAXEXP);
      return TA_TSTCDL_PREDICATE_VACUOUS;
   }
   rc = fn(0, pbCur-1, pbO, pbH, pbL, pbC, &begIdx, &nb, out);
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

   /* Cross-language, same as pb_check_mcdc. These tapes are the ONLY bars in
    * the tree that reach the +/-200 confirmation arm, and until now they never
    * crossed a pipe -- the only CDLHIKKAKE* requests any server ever saw were
    * the settings matrix's, which never produce a 200. */
   if( server_verify_active() )
   {
      const TA_Real *inputs[5];
      const TA_Integer *outInteger[2];
      ErrorNumber svErr;
      int cmpBefore = server_verify_comparisons();
      inputs[0] = pbO; inputs[1] = pbH; inputs[2] = pbL; inputs[3] = pbC; inputs[4] = NULL;
      outInteger[0] = out; outInteger[1] = NULL;
      svErr = server_verify( name, 0, pbCur-1, pbCur, rc, begIdx, nb,
                             inputs, NULL, 0, NULL, outInteger );
      if( svErr != TA_TEST_PASS )
      {
         printf("  %s: cross-language mismatch on the predicate bars\n", name);
         return svErr;
      }
      if( server_verify_comparisons() == cmpBefore )
      {
         printf("  %s: arm B compared nothing despite live servers\n", name);
         return TA_TSTCDL_PREDICATE_VACUOUS;
      }
   }
   return TA_TEST_PASS;
}

/* ---- MC/DC gate helpers for the marquee multi-candle patterns (issue #109) ---
 * Same idea as the Hikkake gate above: a detection scenario that fires the exact
 * value, then one near-miss per structural predicate (that predicate flipped
 * false, the rest held) asserting 0 — so every decision boundary is exercised in
 * both directions. Each scenario self-primes and is separated by flat filler so
 * the candle-setting averages reset between them. Every scenario was validated
 * against the shipped library.
 *
 * BI-SIGNED patterns are in scope and need pb_signs(2). This read "for
 * single-sign patterns" while every builder happened to be one, and the
 * assumption went straight into the checker: nothing required a firing scenario
 * per output class, so the first two-sign patterns to arrive (#220's BELTHOLD
 * and CLOSINGMARUBOZU) were covered on their white arm alone and the gate was
 * satisfied. See the output-class check in pb_check_mcdc.
 *
 * That gap and #221's are one shape, and naming it is worth more than either
 * instance: A SCENARIO MUST BE NON-DEGENERATE ALONG WHATEVER THE CONDITION
 * COMPARES. White-only bars leave a colour disjunction unreached; open == close
 * leaves a min/max pair unreached, because min(open,close) and max(open,close)
 * are then the SAME NUMBER and reading the wrong one changes nothing --
 * CDLRICKSHAWMAN's c3 carries a real body for exactly that reason. Neither gap
 * moves the totals: the condition is declared, flipped and controlled precisely
 * as the checker asks, so nothing here can report it. The only fallback is the
 * v0.6.4 freeze, and it reaches the 35 of 61 patterns that fire on the 252-bar
 * series -- for the other 26 there is none. */

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
/* --- The independent condition model ------------------------------------- *
 *
 * This is what turns the paired control from a convention into a checked
 * property. Each builder supplies a function that evaluates the pattern's
 * atomic conditions DIRECTLY from the bars, transcribed by hand from
 * ta_codegen/input/<name>/<name>.c. pb_check_mcdc then asserts, at every
 * scenario:
 *
 *   DETECT  / CONTROL   every condition true
 *   FLIP                exactly one condition false, and it is the declared one
 *
 * The duplication is the point. Matching a flip to a control by condition id
 * proves nothing -- rewiring every control to the detect bar passed byte for
 * byte before this existed. Re-deriving the conditions here is a second,
 * independent statement of the pattern, so a case that breaks the WRONG
 * condition, a copy-pasted block, or a control unrelated to its flip all fail.
 * And if this model and the library ever disagree, that disagreement is itself
 * the signal -- which is the only mechanism in this file that could catch a
 * predicate that has been wrong since 2005.
 *
 * The leaf helpers below are re-implemented rather than shared with
 * ta_utility.h on purpose, so the averaging windows are computed independently
 * too. They are the naive form (sum the trailing window each time) where the
 * library slides an incremental total -- a deliberate second implementation.
 */
static double pb_range( int rangeType, int i )
{
   double o=pbO[i], h=pbH[i], l=pbL[i], c=pbC[i];
   switch( rangeType )
   {
   case TA_RangeType_RealBody: return c > o ? c-o : o-c;
   case TA_RangeType_HighLow:  return h-l;
   case TA_RangeType_Shadows:  return (h - (c>=o?c:o)) + ((c>=o?o:c) - l);
   default:                    return 0.0;
   }
}

static double pb_avg( TA_CandleSettingType st, int i )
{
   const TA_CandleSetting *cs = &TA_Globals->candleSettings[st];
   double avg, divisor;
   if( cs->avgPeriod == 0 )
      avg = pb_range(cs->rangeType, i);
   else
   {
      double sum = 0.0;
      int k;
      for( k = i - cs->avgPeriod; k < i; k++ )
         sum += pb_range(cs->rangeType, k);
      avg = sum / cs->avgPeriod;
   }
   divisor = (cs->rangeType == TA_RangeType_Shadows) ? 2.0 : 1.0;
   return cs->factor * avg / divisor;
}

static double pb_body( int i )  { return pb_range(TA_RangeType_RealBody, i); }
static double pb_upsh( int i )  { return pbH[i] - (pbC[i] >= pbO[i] ? pbC[i] : pbO[i]); }
static double pb_losh( int i )  { return (pbC[i] >= pbO[i] ? pbO[i] : pbC[i]) - pbL[i]; }
static int    pb_white( int i ) { return pbC[i] >= pbO[i]; }
static double pb_bodylo( int i ){ return pbC[i] >= pbO[i] ? pbO[i] : pbC[i]; }
static double pb_bodyhi( int i ){ return pbC[i] >= pbO[i] ? pbC[i] : pbO[i]; }

typedef void (*PbCondFn)( int i, int *c );

/* Totals across every MC/DC function, printed once so the gate's own coverage
 * is visible rather than asserted in the dark. A waiver count that drifts is
 * the thing most likely to hollow this gate out silently. */
static int pbTotDetect, pbTotFlip, pbTotControl, pbTotWaive, pbTotCond;
static int pbTotMigrated;

/* Print what the gate actually covered. A gate that does not report its own
 * coverage cannot be seen to be shrinking: a waiver count that drifts upward,
 * or a migration that stalls, both look exactly like a green run. */
static void pb_report_totals( void )
{
   printf("  MC/DC: %d function(s), %d condition(s) declared, %d detect(s), "
          "%d flipped, %d control(s), %d waived\n",
          pbTotMigrated, pbTotCond, pbTotDetect, pbTotFlip, pbTotControl,
          pbTotWaive);
}

static ErrorNumber pb_check_mcdc( const char *name, PbCdlFn fn, PbCondFn conds )
{
   int out[PB_N], begIdx=0, nb=0, k, j, fails=0;
   int nDetect=0, nFlip=0, nControl=0;
   TA_RetCode rc;

   /* A builder that ran off the end of the tape wrote nothing, so every
    * expectation past that point reads a bar that was never laid down. Caught
    * before the values are compared -- otherwise it surfaces as a puzzling
    * value mismatch instead of what it is. */
   if( pbOverflow )
   {
      printf("  %s MC/DC: builder overflowed a harness buffer "
             "(bars=%d/%d expectations=%d/%d waivers=%d/%d)\n",
             name, pbCur, PB_N, pbNe, PB_MAXEXP, pbNw, PB_MAXWAIVE);
      return TA_TSTCDL_PREDICATE_VACUOUS;
   }

   rc = fn(0, pbCur-1, pbO, pbH, pbL, pbC, &begIdx, &nb, out);
   if( rc != TA_SUCCESS ) { printf("  %s MC/DC: retCode %d\n", name, rc); return TA_TSTCDL_PREDICATE_MISMATCH; }

   for( k=0; k<pbNe; k++ )
   {
      int oi = pbEi[k]-begIdx;
      int got = (oi>=0 && oi<nb) ? out[oi] : -99999;
      if( got != pbEv[k] )
      {
         printf("  %s MC/DC FAIL bar=%d expected=%d got=%d  (%s)\n", name, pbEi[k], pbEv[k], got, pbEl[k]);
         /* Print the bars the failing expectation was written against. Deriving
          * these by re-reading the builder is the slowest part of writing one,
          * and a wrong number is usually visible the moment the four values are
          * side by side. */
         { int z, z0 = pbEi[k]-3; if( z0 < 0 ) z0 = 0;
           printf("     bars:");
           for(z=z0; z<=pbEi[k]; z++)
              printf(" [%d o=%g h=%g l=%g c=%g]", z, pbO[z],pbH[z],pbL[z],pbC[z]);
           printf("\n"); }
         fails++;
      }
      switch( pbEkind[k] )
      {
      case PB_DETECT:  nDetect++;  break;
      case PB_FLIP:    nFlip++;    break;
      case PB_CONTROL: nControl++; break;
      default:                     break;
      }
      /* A detect or a control that expects 0 asserts nothing: 0 is what the
       * pattern returns nearly everywhere. Only a firing expectation carries
       * information. */
      if( (pbEkind[k] == PB_DETECT || pbEkind[k] == PB_CONTROL) && pbEv[k] == 0 )
      {
         printf("  %s MC/DC VACUOUS: %s expects 0 (%s)\n", name,
                pbEkind[k] == PB_DETECT ? "detect" : "control", pbEl[k]);
         fails++;
      }
      if( pbEkind[k] == PB_FLIP && pbEv[k] != 0 )
      {
         printf("  %s MC/DC: flip expects %d, must be 0 (%s)\n", name, pbEv[k], pbEl[k]);
         fails++;
      }
   }

   /* THE ATTRIBUTION CHECK. Evaluate the pattern's conditions independently at
    * every scenario and require exactly what MC/DC means: nothing false at a
    * detect or a control, and precisely the declared condition false at a flip.
    * This is what the id-matching loop below cannot do. */
   if( conds && pbNbCond > 0 )
   {
      if( pbNbCond > PB_MAXCOND )
      {
         printf("  %s MC/DC: %d conditions exceeds PB_MAXCOND\n", name, pbNbCond);
         return TA_TSTCDL_PREDICATE_MISMATCH;
      }
      for( k=0; k<pbNe; k++ )
      {
         int cv[PB_MAXCOND];
         int nFalse = 0, firstFalse = -1, m;
         if( pbEkind[k] == PB_LEGACY ) continue;
         for( m=0; m<pbNbCond; m++ ) cv[m] = -1;
         conds(pbEi[k], cv);
         for( m=0; m<pbNbCond; m++ )
         {
            if( cv[m] == -1 )
            {
               printf("  %s MC/DC: condition model left c%d unset (%s)\n", name, m, pbEl[k]);
               fails++;
            }
            else if( !cv[m] ) { nFalse++; if( firstFalse < 0 ) firstFalse = m; }
         }
         if( pbEkind[k] == PB_DETECT || pbEkind[k] == PB_CONTROL )
         {
            if( nFalse != 0 )
            {
               printf("  %s MC/DC: %s has %d condition(s) false, first c%d (%s)\n",
                      name, pbEkind[k]==PB_DETECT ? "detect" : "control",
                      nFalse, firstFalse, pbEl[k]);
               fails++;
            }
         }
         else if( pbEkind[k] == PB_FLIP )
         {
            if( nFalse != 1 )
            {
               printf("  %s MC/DC: flip for c%d breaks %d condition(s)%s (%s)\n",
                      name, pbEcond[k], nFalse,
                      nFalse > 1 ? " -- not a single-condition case" : " -- it breaks nothing",
                      pbEl[k]);
               fails++;
            }
            else if( firstFalse != pbEcond[k] )
            {
               printf("  %s MC/DC: flip filed under c%d actually breaks c%d (%s)\n",
                      name, pbEcond[k], firstFalse, pbEl[k]);
               fails++;
            }
         }
      }
   }
   else if( pbNbCond > 0 )
   {
      printf("  %s MC/DC: no condition model supplied -- attribution unchecked\n", name);
      fails++;
   }

   /* Every flip needs its control.
    *
    * KNOW WHAT THIS DOES AND DOES NOT CHECK. It checks only that a control
    * EXISTS carrying the same condition id. Nothing here compares the control's
    * bars to the flip's, so the contract stated above -- "the flip bars with
    * only the target quantity moved back" -- is upheld by the builder's author
    * and not by this loop. Rewiring every control to the detect bar passes,
    * byte for byte. Binding the two structurally (or, better, evaluating the
    * pattern's conditions directly at each scenario) is the next thing this
    * harness needs; until then the control catches an honest mistake in the
    * bars, not a dishonest or copy-pasted case. */
   for( k=0; k<pbNe; k++ )
   {
      int found = 0;
      if( pbEkind[k] != PB_FLIP ) continue;
      for( j=0; j<pbNe; j++ )
         if( pbEkind[j] == PB_CONTROL && pbEcond[j] == pbEcond[k] ) { found = 1; break; }
      if( !found )
      {
         printf("  %s MC/DC: flip for condition %d has no paired control (%s)\n",
                name, pbEcond[k], pbEl[k]);
         fails++;
      }
   }

   if( nDetect == 0 && nFlip > 0 )
   {
      printf("  %s MC/DC VACUOUS: flips but no detect -- nothing proves the "
             "pattern can fire on these bars at all\n", name);
      fails++;
   }

   /* OUTPUT-CLASS COVERAGE. A pattern whose firing arm is
    * ta_candlecolor(...)*100 emits BOTH +100 and -100, and every scenario
    * written for it can still sit on a single colour without anything noticing:
    * the flips assert 0 whatever the colour, and the controls assert whichever
    * sign the author happened to build. What is then left untested is the other
    * arm of the decision and the sign of the output itself.
    *
    * That is not hypothetical. CDLBELTHOLD and CDLCLOSINGMARUBOZU arrived
    * white-only, and BOTH of these survived the whole MC/DC tier green:
    * replacing ta_candlecolor(...)*100 with a bare 100, and deleting the black
    * arm of the colour disjunction outright. Only the v0.6.4 freeze caught
    * them -- and it reaches just the 35 of 61 patterns that fire on the 252-bar
    * series, so for a bi-signed pattern among the other 26 there would have
    * been no gate at all.
    *
    * Note the granularity this compensates for: `condition` here means a
    * TOP-LEVEL CONJUNCT, which is what check-mcdc counts, so a
    * (white && x) || (black && y) disjunction is one condition and its interior
    * is invisible to the completeness check by construction. Requiring every
    * output class to fire is what reaches inside it. */
   if( pbNbCond > 0 )
   {
      int cls[PB_MAXCLASS], nCls = 0, tooMany = 0;
      for( k=0; k<pbNe; k++ )
      {
         int seen = 0;
         if( pbEkind[k] != PB_DETECT && pbEkind[k] != PB_CONTROL ) continue;
         if( pbEv[k] == 0 ) continue;
         for( j=0; j<nCls; j++ ) if( cls[j] == pbEv[k] ) { seen = 1; break; }
         if( seen ) continue;
         if( nCls >= PB_MAXCLASS ) { tooMany = 1; break; }
         cls[nCls++] = pbEv[k];
      }
      if( tooMany )
      {
         printf("  %s MC/DC: more than %d distinct output classes\n", name, PB_MAXCLASS);
         fails++;
      }
      else if( nCls != pbNbSigns )
      {
         printf("  %s MC/DC: %d of the pattern's %d output class(es) exercised"
                " -- a firing scenario is missing for one of its arms\n",
                name, nCls, pbNbSigns);
         fails++;
      }
   }

   /* Completeness. Every DECLARED condition is either flipped or waived, and
    * the totals must agree exactly. A counter of "cases written" cannot see a
    * condition that was quietly skipped; this can. */
   if( pbNbCond > 0 )
   {
      int covered = 0;
      for( k=0; k<pbNe; k++ )
      {
         if( pbEkind[k] != PB_FLIP && pbEkind[k] != PB_CONTROL ) continue;
         if( pbEcond[k] < 0 || pbEcond[k] >= pbNbCond )
         {
            printf("  %s MC/DC: condition id %d outside 0..%d (%s)\n",
                   name, pbEcond[k], pbNbCond-1, pbEl[k]);
            fails++;
         }
      }
      for( k=0; k<pbNw; k++ )
      {
         if( pbWcond[k] < 0 || pbWcond[k] >= pbNbCond )
         {
            printf("  %s MC/DC: waived condition %d outside 0..%d\n",
                   name, pbWcond[k], pbNbCond-1);
            fails++;
         }
         /* Printed, not just stored: a waiver is a claim, and a claim nobody
          * can see is a claim nobody can refute. */
         printf("  %s MC/DC waiver c%d: %s\n", name, pbWcond[k], pbWhy[k]);
      }
      for( k=0; k<pbNbCond; k++ )
      {
         int hit = 0;
         for( j=0; j<pbNe; j++ )
            if( pbEkind[j] == PB_FLIP && pbEcond[j] == k ) { hit = 1; break; }
         if( !hit )
            for( j=0; j<pbNw; j++ )
               if( pbWcond[j] == k ) { hit = 1; break; }
         if( hit ) covered++;
         else printf("  %s MC/DC: condition %d is neither flipped nor waived\n", name, k);
      }
      if( covered != pbNbCond )
      {
         printf("  %s MC/DC: %d of %d declared condition(s) covered\n",
                name, covered, pbNbCond);
         fails++;
      }
      pbTotCond += pbNbCond;
   }
   else if( nFlip > 0 )
   {
      printf("  %s MC/DC: flips recorded but pb_conditions() never declared a "
             "total -- completeness cannot be checked\n", name);
      fails++;
   }

   if( fails ) return TA_TSTCDL_PREDICATE_MISMATCH;

   pbTotDetect += nDetect; pbTotFlip += nFlip; pbTotControl += nControl;
   pbTotWaive  += pbNw;
   pbTotMigrated++;

   /* The pre-migration guard, kept for the builders still on the legacy shape:
    * at least one non-zero expectation must have fired. Strictly weaker than
    * everything above -- one non-zero anywhere satisfied it, so on a function
    * with 18 flips, 18 wrong flips still passed. */
   if( nDetect == 0 )
   {
      int sawNonzero = 0;
      for( k=0; k<pbNe; k++ )
      {
         int oi = pbEi[k]-begIdx;
         int got = (oi>=0 && oi<nb) ? out[oi] : -99999;
         if( pbEv[k] != 0 && got == pbEv[k] ) sawNonzero = 1;
      }
      if( !sawNonzero )
      {
         printf("  %s MC/DC VACUOUS: no expected non-zero output fired\n", name);
         return TA_TSTCDL_PREDICATE_VACUOUS;
      }
   }

   /* Arm B: the same bars, compared against every language server. These are
    * the best candlestick inputs in the tree -- hand-placed on the decision
    * boundaries, where the 252-bar series and the fuzz shapes essentially
    * never land -- and until now they were never sent anywhere. Degrades to a
    * no-op when no server is running, so a bare ./ta_regtest is unaffected. */
   if( server_verify_active() )
   {
      const TA_Real *inputs[5];
      const TA_Integer *outInteger[2];
      ErrorNumber svErr;
      inputs[0] = pbO; inputs[1] = pbH; inputs[2] = pbL; inputs[3] = pbC; inputs[4] = NULL;
      outInteger[0] = out; outInteger[1] = NULL;
      int cmpBefore = server_verify_comparisons();
      svErr = server_verify( name, 0, pbCur-1, pbCur, rc, begIdx, nb,
                             inputs, NULL, 0, NULL, outInteger );
      if( svErr != TA_TEST_PASS )
      {
         printf("  %s MC/DC: cross-language mismatch on the predicate bars\n", name);
         return svErr;
      }
      /* And prove it actually compared. A server that answers every request
       * with an error used to leave this leg green and silent. */
      if( server_verify_comparisons() == cmpBefore )
      {
         printf("  %s MC/DC: arm B compared nothing despite live servers\n", name);
         return TA_TSTCDL_PREDICATE_VACUOUS;
      }
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
/* Condition models, transcribed from ta_codegen/input/<name>/<name>.c in source
 * order. cN here is cN there and cN in the builder below. */

static void cond_2crows( int i, int *c )
{
   c[0] = pb_white(i-2);
   c[1] = pb_body(i-2) > pb_avg(TA_BodyLong, i-2);
   c[2] = !pb_white(i-1);
   c[3] = pb_bodylo(i-1) > pb_bodyhi(i-2);
   c[4] = !pb_white(i);
   c[5] = pbO[i] < pbO[i-1];
   c[6] = pbO[i] > pbC[i-1];
   c[7] = pbC[i] > pbO[i-2];
   c[8] = pbC[i] < pbC[i-2];
}

static void cond_3blackcrows( int i, int *c )
{
   c[0]  = pb_white(i-3);
   c[1]  = !pb_white(i-2);
   c[2]  = !pb_white(i-1);
   c[3]  = !pb_white(i);
   c[4]  = pbO[i-1] < pbO[i-2];
   c[5]  = pbO[i-1] > pbC[i-2];
   c[6]  = pbO[i]   < pbO[i-1];
   c[7]  = pbO[i]   > pbC[i-1];
   c[8]  = pbH[i-3] > pbC[i-2];
   c[9]  = pbC[i-2] > pbC[i-1];
   c[10] = pbC[i-1] > pbC[i];
   c[11] = pb_losh(i-2) < pb_avg(TA_ShadowVeryShort, i-2);
   c[12] = pb_losh(i-1) < pb_avg(TA_ShadowVeryShort, i-1);
   c[13] = pb_losh(i)   < pb_avg(TA_ShadowVeryShort, i);
}

static void cond_3whitesoldiers( int i, int *c )
{
   c[0]  = pb_white(i-2);
   c[1]  = pb_upsh(i-2) < pb_avg(TA_ShadowVeryShort, i-2);
   c[2]  = pb_white(i-1);
   c[3]  = pb_upsh(i-1) < pb_avg(TA_ShadowVeryShort, i-1);
   c[4]  = pb_white(i);
   c[5]  = pb_upsh(i)   < pb_avg(TA_ShadowVeryShort, i);
   c[6]  = pbC[i]   > pbC[i-1];
   c[7]  = pbC[i-1] > pbC[i-2];
   c[8]  = pbO[i-1] > pbO[i-2];
   c[9]  = pbO[i-1] <= pbC[i-2] + pb_avg(TA_Near, i-2);
   c[10] = pbO[i]   > pbO[i-1];
   c[11] = pbO[i]   <= pbC[i-1] + pb_avg(TA_Near, i-1);
   c[12] = pb_body(i-1) > pb_body(i-2) - pb_avg(TA_Far, i-2);
   c[13] = pb_body(i)   > pb_body(i-1) - pb_avg(TA_Far, i-1);
   c[14] = pb_body(i)   > pb_avg(TA_BodyShort, i);
}

static void cond_3starsinsouth( int i, int *c )
{
   c[0]  = !pb_white(i-2);
   c[1]  = !pb_white(i-1);
   c[2]  = !pb_white(i);
   c[3]  = pb_body(i-2) > pb_avg(TA_BodyLong, i-2);
   c[4]  = pb_losh(i-2) > pb_avg(TA_ShadowLong, i-2);
   c[5]  = pb_body(i-1) < pb_body(i-2);
   c[6]  = pbO[i-1] > pbC[i-2];
   c[7]  = pbO[i-1] <= pbH[i-2];
   c[8]  = pbL[i-1] < pbC[i-2];
   c[9]  = pbL[i-1] >= pbL[i-2];
   c[10] = pb_losh(i-1) > pb_avg(TA_ShadowVeryShort, i-1);
   c[11] = pb_body(i)   < pb_avg(TA_BodyShort, i);
   c[12] = pb_losh(i)   < pb_avg(TA_ShadowVeryShort, i);
   c[13] = pb_upsh(i)   < pb_avg(TA_ShadowVeryShort, i);
   c[14] = pbL[i] > pbL[i-1];
   c[15] = pbH[i] < pbH[i-1];
}

/* CDL2CROWS -- the reference MC/DC builder. Every other one follows this shape.
 *
 * The nine atomic conditions of the detection decision, in source order
 * (cdl2crows.c), indexed as pb_conditions() declares them:
 *
 *   c0  color(i-2) == 1                     1st white
 *   c1  realbody(i-2) > avg(BodyLong)       1st long
 *   c2  color(i-1) == -1                    2nd black
 *   c3  realbodygapup(i-1, i-2)             2nd body clears the 1st
 *   c4  color(i) == -1                      3rd black
 *   c5  open[i]  < open[i-1]
 *   c6  open[i]  > close[i-1]
 *   c7  close[i] > open[i-2]
 *   c8  close[i] < close[i-2]
 *
 * Three of the nine cannot be falsified alone by ANY valid OHLC input -- they
 * are entailed by their siblings -- so they are waived with the derivation
 * rather than given a flip case that would really be testing two conditions at
 * once. Each waiver is refutable: produce a flip whose paired control fires and
 * it is wrong.
 *
 * Detect bars, referenced by every derivation below:
 *   bar1 (100,106, 99,105)  white, rb 5 > avg 2
 *   bar2 (110,111,106,107)  black, min(110,107)=107 > max(100,105)=105
 *   bar3 (109,110,102,103)  black, 109<110, 109>107, 103>100, 103<105
 */
static void build_2crows( void )
{
  pb_conditions(9);

  pb_waive(0, "c7 & c8 give open1 < close3 < close1, so the 1st is white by construction");
  pb_waive(2, "c5 & c6 give close2 < open3 < open2, so the 2nd is black by construction");
  pb_waive(4, "c3 & c6 give open3 > close2 > close1, and c8 gives close3 < close1, "
              "so open3 > close3 and the 3rd is black by construction");

  pb_flat(6);
  /* DETECT: all nine hold. */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int d=pb_bar(109,110,102,103);
  pb_detect(d,-100,"detect");
  pb_flat(8);

  /* c1: 1st long. Flip shrinks the 1st real body to exactly the average
   * (rb 2, avg 2 -- the test is strict >). Control widens it to 3. */
  pb_primer(12,100,2,1);
  pb_bar(102,106,99,104);
  pb_bar(110,111,106,107);
  int f1=pb_bar(109,110,102,103);
  pb_flip(f1,1,"break c1 1st-long: rb==avg");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(102,106,99,105);
  pb_bar(110,111,106,107);
  int k1=pb_bar(109,110,102,103);
  pb_control(k1,-100,1,"restore c1: rb 3 > avg 2");
  pb_flat(8);

  /* c3: gap up. Flip drops the 2nd body's floor to 104, below the 1st body's
   * top of 105. Control raises that same close to 106. */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,103,105);
  int f3=pb_bar(109,110,102,103);
  pb_flip(f3,3,"break c3 gap-up: min(O2,C2)=105 !> 105");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,103,106);
  int k3=pb_bar(109,110,102,103);
  pb_control(k3,-100,3,"restore c3: min(O2,C2)=106 > 105");
  pb_flat(8);

  /* c5: open3 < open2. Flip opens the 3rd at 111, above the 2nd's 110.
   * Control moves that one open back to 109. */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f5=pb_bar(110,111,102,103);
  pb_flip(f5,5,"break c5: open3 110 >= open2 110");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int k5=pb_bar(109,112,102,103);
  pb_control(k5,-100,5,"restore c5: open3 109 < open2 110");
  pb_flat(8);

  /* c6: open3 > close2. Flip opens the 3rd at 106, at or below the 2nd's close
   * of 107. Control moves it to 108, back inside the 2nd's body. */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f6=pb_bar(107,108,102,103);
  pb_flip(f6,6,"break c6: open3 107 <= close2 107");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int k6=pb_bar(108,109,102,103);
  pb_control(k6,-100,6,"restore c6: open3 108 > close2 107");
  pb_flat(8);

  /* c7: close3 > open1. Flip closes the 3rd at 99, below the 1st's open of 100.
   * Control lifts that close to 101, back inside the 1st body. */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f7=pb_bar(109,110,98,100);
  pb_flip(f7,7,"break c7: close3 100 <= open1 100");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int k7=pb_bar(109,110,98,101);
  pb_control(k7,-100,7,"restore c7: close3 101 > open1 100");
  pb_flat(8);

  /* c8: close3 < close1. Flip closes the 3rd at 106, at or above the 1st's
   * close of 105. Control lowers that close to 104. */
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int f8=pb_bar(109,110,103,105);
  pb_flip(f8,8,"break c8: close3 105 >= close1 105");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,106,99,105);
  pb_bar(110,111,106,107);
  int k8=pb_bar(109,110,103,104);
  pb_control(k8,-100,8,"restore c8: close3 104 < close1 105");
  pb_flat(8);
}

/* CDL3BLACKCROWS -- second reference builder. Adds the condition class 2CROWS
 * has none of: a SHADOW AVERAGE (ShadowVeryShort, HighLow/10/0.1).
 *
 * Fourteen atomic conditions (cdl3blackcrows.c), bars A=i-3, B=i-2, C=i-1, D=i:
 *
 *   c0  color(A) == 1                 c7  open(D)  > close(C)
 *   c1  color(B) == -1                c8  high(A)  > close(B)
 *   c2  color(C) == -1                c9  close(B) > close(C)
 *   c3  color(D) == -1                c10 close(C) > close(D)
 *   c4  open(C) < open(B)             c11 lowershadow(B) < avg(ShadowVeryShort)
 *   c5  open(C) > close(B)            c12 lowershadow(C) < avg(ShadowVeryShort)
 *   c6  open(D) < open(C)             c13 lowershadow(D) < avg(ShadowVeryShort)
 *
 * The same structural fact as 2CROWS decides the waivers, and it is worth
 * stating as a rule because it recurs across the family: WHERE A PATTERN
 * ORDERS A BAR'S OPEN AND CLOSE AGAINST OTHER PRICES, ITS COLOUR TEST IS
 * ENTAILED AND CANNOT BE FLIPPED ALONE. ta_candlecolor returns only +1/-1, so
 * "not black" is "white", and the ordering conditions already force which.
 * c0 is the exception here: A's open and close are ordered against nothing, so
 * its colour IS independently flippable.
 *
 * The shadow threshold is ~0.39-0.41 (primer range 4, factor 0.1, and the
 * pattern bars themselves enter the trailing window). Flips overshoot it to 1.0
 * and controls sit at 0.2, so neither rides the boundary.
 */
static void build_3blackcrows( void )
{
  pb_conditions(14);

  pb_waive(1, "c4 & c5 give close(B) < open(C) < open(B), so B is black by construction");
  pb_waive(2, "c6 & c7 give close(C) < open(D) < open(C), so C is black by construction");
  pb_waive(3, "c7 & c10 give open(D) > close(C) > close(D), so D is black by construction");

  pb_flat(6);
  /* DETECT */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int d=pb_bar(101,101.5,97,97);
  pb_detect(d,-100,"detect");
  pb_flat(8);

  /* c0: A is white. A's open/close are ordered against nothing, so unlike
   * B/C/D this colour test is genuinely independent. */
  pb_primer(12,100,2,1);
  pb_bar(104,105,100,100);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f0=pb_bar(101,101.5,97,97);
  pb_flip(f0,0,"break c0: A black");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,103);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k0=pb_bar(101,101.5,97,97);
  pb_control(k0,-100,0,"restore c0: A white");
  pb_flat(8);

  /* c4: open(C) < open(B). */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(103,103.5,99,99);
  int f4=pb_bar(101,101.5,97,97);
  pb_flip(f4,4,"break c4: open(C) 103 >= open(B) 103");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k4=pb_bar(101,101.5,97,97);
  pb_control(k4,-100,4,"restore c4: open(C) 102 < open(B) 103");
  pb_flat(8);

  /* c5: open(C) > close(B). D's open moves with it only to keep c6 true --
   * the condition under test is still the only one broken. */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(101,101.5,99,99);
  int f5=pb_bar(100,100.5,97,97);
  pb_flip(f5,5,"break c5: open(C) 101 <= close(B) 101");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k5=pb_bar(100,100.5,97,97);
  pb_control(k5,-100,5,"restore c5: open(C) 102 > close(B) 101");
  pb_flat(8);

  /* c6: open(D) < open(C). */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f6=pb_bar(102,102.5,97,97);
  pb_flip(f6,6,"break c6: open(D) 102 >= open(C) 102");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k6=pb_bar(101,101.5,97,97);
  pb_control(k6,-100,6,"restore c6: open(D) 101 < open(C) 102");
  pb_flat(8);

  /* c7: open(D) > close(C). */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f7=pb_bar(99,99.5,97,97);
  pb_flip(f7,7,"break c7: open(D) 99 <= close(C) 99");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k7=pb_bar(100,100.5,97,97);
  pb_control(k7,-100,7,"restore c7: open(D) 100 > close(C) 99");
  pb_flat(8);

  /* c8: high(A) > close(B). Only A's high moves. */
  pb_primer(12,100,2,1);
  pb_bar(100,101,100,100.5);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f8=pb_bar(101,101.5,97,97);
  pb_flip(f8,8,"break c8: high(A) 101 <= close(B) 101");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,102,100,100.5);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k8=pb_bar(101,101.5,97,97);
  pb_control(k8,-100,8,"restore c8: high(A) 102 > close(B) 101");
  pb_flat(8);

  /* c9: close(B) > close(C). D's open moves only to keep c7 true. */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,101,101);
  int f9=pb_bar(101.5,102,97,97);
  pb_flip(f9,9,"break c9: close(B) 101 <= close(C) 101");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k9=pb_bar(101.5,102,97,97);
  pb_control(k9,-100,9,"restore c9: close(B) 101 > close(C) 99");
  pb_flat(8);

  /* c10: close(C) > close(D). */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f10=pb_bar(101,101.5,99,99);
  pb_flip(f10,10,"break c10: close(D) 99 >= close(C) 99");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k10=pb_bar(101,101.5,97,97);
  pb_control(k10,-100,10,"restore c10: close(D) 97 < close(C) 99");
  pb_flat(8);

  /* c11: B's lower shadow under the ShadowVeryShort average (~0.41). */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,100,101);
  pb_bar(102,102.5,99,99);
  int f11=pb_bar(101,101.5,97,97);
  pb_flip(f11,11,"break c11: lowershadow(B) 1.0 >= avg");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,100.8,101);
  pb_bar(102,102.5,99,99);
  int k11=pb_bar(101,101.5,97,97);
  pb_control(k11,-100,11,"restore c11: lowershadow(B) 0.2 < avg");
  pb_flat(8);

  /* c12: C's lower shadow. */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,98,99);
  int f12=pb_bar(101,101.5,97,97);
  pb_flip(f12,12,"break c12: lowershadow(C) 1.0 >= avg");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,98.8,99);
  int k12=pb_bar(101,101.5,97,97);
  pb_control(k12,-100,12,"restore c12: lowershadow(C) 0.2 < avg");
  pb_flat(8);

  /* c13: D's lower shadow. */
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int f13=pb_bar(101,101.5,96,97);
  pb_flip(f13,13,"break c13: lowershadow(D) 1.0 >= avg");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(100,105,100,104);
  pb_bar(103,103.5,101,101);
  pb_bar(102,102.5,99,99);
  int k13=pb_bar(101,101.5,96.8,97);
  pb_control(k13,-100,13,"restore c13: lowershadow(D) 0.2 < avg");
  pb_flat(8);
}

/* CDL3WHITESOLDIERS -- third reference builder. Adds the TOLERANCE-BASED
 * threshold class: Near (HighLow/5/0.2) and Far (HighLow/5/0.6), where the
 * comparison is against "a neighbouring price PLUS an average" rather than
 * against a price directly.
 *
 * Fifteen conditions (cdl3whitesoldiers.c), bars b1=i-2, b2=i-1, b3=i:
 *
 *   c0  color(b1) == 1                c7  close(b2) > close(b1)
 *   c1  uppershadow(b1) < avg(SVS)    c8  open(b2)  > open(b1)
 *   c2  color(b2) == 1                c9  open(b2)  <= close(b1) + avg(Near)
 *   c3  uppershadow(b2) < avg(SVS)    c10 open(b3)  > open(b2)
 *   c4  color(b3) == 1                c11 open(b3)  <= close(b2) + avg(Near)
 *   c5  uppershadow(b3) < avg(SVS)    c12 rb(b2) > rb(b1) - avg(Far)
 *   c6  close(b3) > close(b2)         c13 rb(b3) > rb(b2) - avg(Far)
 *                                     c14 rb(b3) > avg(BodyShort)
 *
 * NO WAIVERS -- and that is the point of keeping this one as a reference. The
 * colour-test rule that forces three waivers in 2CROWS and 3BLACKCROWS does NOT
 * apply here, because the ordering conditions are separated by a tolerance:
 * c8 and c9 give open(b1) < open(b2) <= close(b1) + Near, which permits
 * open(b1) > close(b1) whenever the real body is smaller than Near. So all
 * three colour tests are independently flippable, with a black bar whose body
 * fits inside the tolerance. A rule inferred from the first two patterns alone
 * would have waived them wrongly.
 *
 * Primer range is 18 (pb_primer(12,100,2,8)), giving SVS ~1.6-1.8, Near ~3.1-3.6
 * and Far ~9.4-10.8; the trailing windows also swallow the pattern bars, so each
 * scenario below keeps the bars it is not testing at a range that leaves those
 * thresholds where the detect case put them.
 */
static void build_3whitesoldiers( void )
{
  pb_conditions(15);

  pb_flat(6);
  /* DETECT */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int d=pb_bar(110,118.2,109.8,118);
  pb_detect(d,100,"detect");
  pb_flat(8);

  /* c0: b1 white. Flip uses a black b1 whose 2-wide body fits inside Near, so
   * c8/c9 still hold -- the tolerance is exactly what makes this flippable. */
  pb_primer(12,100,2,8);
  pb_bar(103,103.2,100.8,101);
  pb_bar(104,112.2,103.8,112);
  int f0=pb_bar(110,118.2,109.8,118);
  pb_flip(f0,0,"break c0: b1 black");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(101,103.2,100.8,103);
  pb_bar(104,112.2,103.8,112);
  int k0=pb_bar(110,118.2,109.8,118);
  pb_control(k0,100,0,"restore c0: b1 white, same range");
  pb_flat(8);

  /* c1: b1 upper shadow. */
  pb_primer(12,100,2,8);
  pb_bar(100,110,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int f1=pb_bar(110,118.2,109.8,118);
  pb_flip(f1,1,"break c1: uppershadow(b1) 4.0 >= avg");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,107.5,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int k1=pb_bar(110,118.2,109.8,118);
  pb_control(k1,100,1,"restore c1: uppershadow(b1) 1.5 < avg");
  pb_flat(8);

  /* c2: b2 white. Body kept inside Near, and b3's open follows it so c10/c11
   * stay true -- only the colour of b2 is under test. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(109,109.2,107.8,108);
  int f2=pb_bar(110,118.2,109.8,118);
  pb_flip(f2,2,"break c2: b2 black");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(108,109.2,107.8,109);
  int k2=pb_bar(110,118.2,109.8,118);
  pb_control(k2,100,2,"restore c2: b2 white, same range");
  pb_flat(8);

  /* c3: b2 upper shadow. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,118,103.8,112);
  int f3=pb_bar(110,118.2,109.8,118);
  pb_flip(f3,3,"break c3: uppershadow(b2) 6.0 >= avg");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,113.2,103.8,112);
  int k3=pb_bar(110,118.2,109.8,118);
  pb_control(k3,100,3,"restore c3: uppershadow(b2) 1.2 < avg");
  pb_flat(8);

  /* c4: b3 white -- again only reachable through the Near tolerance on c11. */
  pb_primer(12,100,2,8);
  pb_bar(100,102.2,94,102);
  pb_bar(103,105.2,102.8,105);
  int f4=pb_bar(107.9,108.1,105.2,105.4);
  pb_flip(f4,4,"break c4: b3 black");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,102.2,94,102);
  pb_bar(103,105.2,102.8,105);
  int k4=pb_bar(105.4,108.1,105.2,107.9);
  pb_control(k4,100,4,"restore c4: b3 white, same range");
  pb_flat(8);

  /* c5: b3 upper shadow. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int f5=pb_bar(110,125,109.8,118);
  pb_flip(f5,5,"break c5: uppershadow(b3) 7.0 >= avg");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int k5=pb_bar(110,119.2,109.8,118);
  pb_control(k5,100,5,"restore c5: uppershadow(b3) 1.2 < avg");
  pb_flat(8);

  /* c6: close(b3) > close(b2). The high moves with the close only to keep c5
   * true; the quantity under test is the close. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int f6=pb_bar(105,112.2,104.8,112);
  pb_flip(f6,6,"break c6: close(b3) 112 <= close(b2) 112");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int k6=pb_bar(105,113.2,104.8,113);
  pb_control(k6,100,6,"restore c6: close(b3) 113 > close(b2) 112");
  pb_flat(8);

  /* c7: close(b2) > close(b1). b3 opens lower so c11 survives the smaller
   * close(b2); only c7 is broken. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,106.2,103.8,106);
  int f7=pb_bar(108,118.2,107.8,118);
  pb_flip(f7,7,"break c7: close(b2) 106 <= close(b1) 106");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,108.2,103.8,108);
  int k7=pb_bar(108,118.2,107.8,118);
  pb_control(k7,100,7,"restore c7: close(b2) 108 > close(b1) 106");
  pb_flat(8);

  /* c8: open(b2) > open(b1). */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(100,112.2,99.8,112);
  int f8=pb_bar(110,118.2,109.8,118);
  pb_flip(f8,8,"break c8: open(b2) 100 <= open(b1) 100");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(101,112.2,100.8,112);
  int k8=pb_bar(110,118.2,109.8,118);
  pb_control(k8,100,8,"restore c8: open(b2) 101 > open(b1) 100");
  pb_flat(8);

  /* c9: open(b2) <= close(b1) + Near. b3's open follows b2's so c10 holds. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(110,118.2,109.8,118);
  int f9=pb_bar(112,120.2,111.8,120);
  pb_flip(f9,9,"break c9: open(b2) 110 > close(b1) 106 + Near");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(109,118.2,108.8,118);
  int k9=pb_bar(112,120.2,111.8,120);
  pb_control(k9,100,9,"restore c9: open(b2) 109 <= close(b1) 106 + Near");
  pb_flat(8);

  /* c10: open(b3) > open(b2). */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int f10=pb_bar(104,118.2,103.8,118);
  pb_flip(f10,10,"break c10: open(b3) 104 <= open(b2) 104");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int k10=pb_bar(105,118.2,104.8,118);
  pb_control(k10,100,10,"restore c10: open(b3) 105 > open(b2) 104");
  pb_flat(8);

  /* c11: open(b3) <= close(b2) + Near. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int f11=pb_bar(116,124.2,115.8,124);
  pb_flip(f11,11,"break c11: open(b3) 116 > close(b2) 112 + Near");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int k11=pb_bar(115,123.2,114.8,123);
  pb_control(k11,100,11,"restore c11: open(b3) 115 <= close(b2) 112 + Near");
  pb_flat(8);

  /* c12: rb(b2) > rb(b1) - Far. Needs a 22-wide b1 before the shortfall is
   * even reachable, since Far is ~10.8 -- which is why this condition is
   * nearly always true on ordinary data and why it needs a case at all. */
  pb_primer(12,100,2,8);
  pb_bar(100,122.2,99.8,122);
  pb_bar(124,126.2,123.8,126);
  int f12=pb_bar(128,134.2,127.8,134);
  pb_flip(f12,12,"break c12: rb(b2) 2 <= rb(b1) 22 - Far");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,122.2,99.8,122);
  pb_bar(114,126.2,113.8,126);
  int k12=pb_bar(128,134.2,127.8,134);
  pb_control(k12,100,12,"restore c12: rb(b2) 12 > rb(b1) 22 - Far");
  pb_flat(8);

  /* c13: rb(b3) > rb(b2) - Far. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,122.2,103.8,122);
  int f13=pb_bar(124,130.2,123.8,130);
  pb_flip(f13,13,"break c13: rb(b3) 6 <= rb(b2) 18 - Far");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,122.2,103.8,122);
  int k13=pb_bar(118,130.2,117.8,130);
  pb_control(k13,100,13,"restore c13: rb(b3) 12 > rb(b2) 18 - Far");
  pb_flat(8);

  /* c14: rb(b3) > avg(BodyShort). The condition that is easiest to miss when
   * enumerating this pattern by eye -- it sits alone after the two Far tests
   * and reads like a repeat of them. Every threshold puzzle in this builder
   * traced back to omitting it. */
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int f14=pb_bar(111,113.2,110.8,113);
  pb_flip(f14,14,"break c14: rb(b3) 2 <= avg(BodyShort) 3");
  pb_flat(8);
  pb_primer(12,100,2,8);
  pb_bar(100,106.2,99.8,106);
  pb_bar(104,112.2,103.8,112);
  int k14=pb_bar(109,113.2,108.8,113);
  pb_control(k14,100,14,"restore c14: rb(b3) 4 > avg(BodyShort) 3");
  pb_flat(8);
}

/* CDL3STARSINSOUTH -- fourth reference builder. Adds the last threshold class:
 * a SELF-REFERENTIAL average. ShadowLong defaults to {RealBody, 0, 1.0}, and an
 * avgPeriod of 0 makes ta_candleaverage return the CURRENT bar's own range
 * rather than a trailing mean -- so c4 reads "this candle's lower shadow
 * exceeds this candle's own body". The threshold moves with the bar under test,
 * which is why the eight functions reading ShadowLong/ShadowVeryLong cannot be
 * flipped by the usual "move the value, leave the threshold" edit.
 *
 * Sixteen conditions (cdl3starsinsouth.c), bars C1=i-2, C2=i-1, C3=i:
 *
 *   c0  color(C1) == -1               c8  low(C2)  < close(C1)
 *   c1  color(C2) == -1               c9  low(C2)  >= low(C1)
 *   c2  color(C3) == -1               c10 lowershadow(C2) > avg(SVS)
 *   c3  rb(C1) > avg(BodyLong)        c11 rb(C3) < avg(BodyShort)
 *   c4  lowershadow(C1) > avg(ShadowLong)   [== rb(C1), self-referential]
 *   c5  rb(C2) < rb(C1)               c12 lowershadow(C3) < avg(SVS)
 *   c6  open(C2) > close(C1)          c13 uppershadow(C3) < avg(SVS)
 *   c7  open(C2) <= high(C1)          c14 low(C3)  > low(C2)
 *                                     c15 high(C3) < high(C2)
 *
 * No waivers: every colour test here is ordered against prices loosely enough
 * (or not at all) to be flipped on its own, unlike 2CROWS and 3BLACKCROWS.
 *
 * Several flips move a SECOND bar as well -- breaking c4 raises C1's low, which
 * would drag c9 and c14 down with it unless C2 and C3 follow. That is allowed
 * and is the normal case: the contract is that exactly one CONDITION ends up
 * false, not that exactly one number changes.
 */
static void build_3starsinsouth( void )
{
  pb_conditions(16);

  pb_flat(6);
  /* DETECT */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int d=pb_bar(106,106.3,104.7,105);
  pb_detect(d,100,"detect");
  pb_flat(8);

  /* c0: C1 black. C2 opens above C1's white close so c6 survives. */
  pb_primer(12,100,2,1);
  pb_bar(110,121,95,120);
  pb_bar(120.5,120.6,100,115);
  int f0=pb_bar(106,106.3,104.7,105);
  pb_flip(f0,0,"break c0: C1 white");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(120.5,120.6,100,115);
  int k0=pb_bar(106,106.3,104.7,105);
  pb_control(k0,100,0,"restore c0: C1 black");
  pb_flat(8);

  /* c1: C2 black. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,124,100,122);
  int f1=pb_bar(106,106.3,104.7,105);
  pb_flip(f1,1,"break c1: C2 white");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,124,100,114);
  int k1=pb_bar(106,106.3,104.7,105);
  pb_control(k1,100,1,"restore c1: C2 black, same range");
  pb_flat(8);

  /* c2: C3 black. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int f2=pb_bar(105,106.3,104.7,106);
  pb_flip(f2,2,"break c2: C3 white");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int k2=pb_bar(106,106.3,104.7,105);
  pb_control(k2,100,2,"restore c2: C3 black");
  pb_flat(8);

  /* c3: C1's body over BodyLong. C2 shrinks so c5 survives the shorter C1. */
  pb_primer(12,100,2,1);
  pb_bar(112,121,95,110);
  pb_bar(118,118,100,117);
  int f3=pb_bar(106,106.3,104.7,105);
  pb_flip(f3,3,"break c3: rb(C1) 2 <= avg(BodyLong) 2");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(113,121,95,110);
  pb_bar(118,118,100,117);
  int k3=pb_bar(106,106.3,104.7,105);
  pb_control(k3,100,3,"restore c3: rb(C1) 3 > avg(BodyLong) 2");
  pb_flat(8);

  /* c4: THE SELF-REFERENTIAL ONE. Raising C1's low shrinks its lower shadow
   * below its own body; C2 and C3 follow so c9 and c14 stay true. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,105,110);
  pb_bar(118,118,106,112);
  int f4=pb_bar(108,108.3,106.7,107);
  pb_flip(f4,4,"break c4: lowershadow(C1) 5 <= rb(C1) 10");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,106,112);
  int k4=pb_bar(108,108.3,106.7,107);
  pb_control(k4,100,4,"restore c4: lowershadow(C1) 15 > rb(C1) 10");
  pb_flat(8);

  /* c5: rb(C2) < rb(C1). */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,106);
  int f5=pb_bar(106,106.3,104.7,105);
  pb_flip(f5,5,"break c5: rb(C2) 12 >= rb(C1) 10");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,110);
  int k5=pb_bar(106,106.3,104.7,105);
  pb_control(k5,100,5,"restore c5: rb(C2) 8 < rb(C1) 10");
  pb_flat(8);

  /* c6: open(C2) > close(C1). */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(110,118,100,105);
  int f6=pb_bar(106,106.3,104.7,105);
  pb_flip(f6,6,"break c6: open(C2) 110 <= close(C1) 110");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(112,118,100,107);
  int k6=pb_bar(106,106.3,104.7,105);
  pb_control(k6,100,6,"restore c6: open(C2) 112 > close(C1) 110");
  pb_flat(8);

  /* c7: open(C2) <= high(C1). */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(122,124,100,116);
  int f7=pb_bar(106,106.3,104.7,105);
  pb_flip(f7,7,"break c7: open(C2) 122 > high(C1) 121");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(120,124,100,114);
  int k7=pb_bar(106,106.3,104.7,105);
  pb_control(k7,100,7,"restore c7: open(C2) 120 <= high(C1) 121");
  pb_flat(8);

  /* c8: low(C2) < close(C1). C3 rises so c14 survives the higher C2 low. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,110,112);
  int f8=pb_bar(112,112.3,110.7,111);
  pb_flip(f8,8,"break c8: low(C2) 110 >= close(C1) 110");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,108,112);
  int k8=pb_bar(112,112.3,110.7,111);
  pb_control(k8,100,8,"restore c8: low(C2) 108 < close(C1) 110");
  pb_flat(8);

  /* c9: low(C2) >= low(C1). */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,94,112);
  int f9=pb_bar(106,106.3,104.7,105);
  pb_flip(f9,9,"break c9: low(C2) 94 < low(C1) 95");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,96,112);
  int k9=pb_bar(106,106.3,104.7,105);
  pb_control(k9,100,9,"restore c9: low(C2) 96 >= low(C1) 95");
  pb_flat(8);

  /* c10: C2's lower shadow over the SVS average. C2's close drops near its low
   * (a low alone cannot shrink the shadow without breaking c8), and C3 rises. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,109.4,109.5);
  int f10=pb_bar(111,111.3,109.7,110);
  pb_flip(f10,10,"break c10: lowershadow(C2) 0.1 <= avg(SVS)");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,108,109.5);
  int k10=pb_bar(111,111.3,109.7,110);
  pb_control(k10,100,10,"restore c10: lowershadow(C2) 1.5 > avg(SVS)");
  pb_flat(8);

  /* c11: rb(C3) under BodyShort. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int f11=pb_bar(110,110.3,104.7,105);
  pb_flip(f11,11,"break c11: rb(C3) 5 >= avg(BodyShort) 3.2");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int k11=pb_bar(107,107.3,104.7,105);
  pb_control(k11,100,11,"restore c11: rb(C3) 2 < avg(BodyShort) 3.2");
  pb_flat(8);

  /* c12: C3's lower shadow under the SVS average. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int f12=pb_bar(106,106.3,102,105);
  pb_flip(f12,12,"break c12: lowershadow(C3) 3.0 >= avg(SVS)");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int k12=pb_bar(106,106.3,104.7,105);
  pb_control(k12,100,12,"restore c12: lowershadow(C3) 0.3 < avg(SVS)");
  pb_flat(8);

  /* c13: C3's upper shadow under the SVS average. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int f13=pb_bar(106,109,104.7,105);
  pb_flip(f13,13,"break c13: uppershadow(C3) 3.0 >= avg(SVS)");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int k13=pb_bar(106,106.3,104.7,105);
  pb_control(k13,100,13,"restore c13: uppershadow(C3) 0.3 < avg(SVS)");
  pb_flat(8);

  /* c14: low(C3) > low(C2). C3 drops as a whole so its shadows stay short. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int f14=pb_bar(101,101.3,99.7,100);
  pb_flip(f14,14,"break c14: low(C3) 99.7 <= low(C2) 100");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int k14=pb_bar(101.5,101.8,100.7,101);
  pb_control(k14,100,14,"restore c14: low(C3) 100.7 > low(C2) 100");
  pb_flat(8);

  /* c15: high(C3) < high(C2). C3 rises as a whole so its shadows stay short. */
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int f15=pb_bar(117.7,118,116.4,117);
  pb_flip(f15,15,"break c15: high(C3) 118 >= high(C2) 118");
  pb_flat(8);
  pb_primer(12,100,2,1);
  pb_bar(120,121,95,110);
  pb_bar(118,118,100,112);
  int k15=pb_bar(117.2,117.5,116.4,116.9);
  pb_control(k15,100,15,"restore c15: high(C3) 117.5 < high(C2) 118");
  pb_flat(8);
}

/* ---- Mechanical tier, unit 1: the six smallest single-bar patterns -------- *
 *
 * Every one of these decides on the CURRENT bar alone, so the primer is the
 * whole of the averaging context and the scenario is a single bar appended to
 * it. That makes the thresholds exact, which is the point:
 *
 *   pb_primer(12,100,2,4) lays down bars with RealBody 2 and HighLow 10, so
 *
 *     avg(BodyLong)         = 1.0 * (20/10) / 1  = 2.0   exactly
 *     avg(BodyDoji)         = 0.1 * (100/10) / 1 = 1.0   exactly
 *     avg(ShadowVeryShort)  = 0.1 * (100/10) / 1 = 1.0   exactly
 *     avg(ShadowLong)       = the CURRENT bar's real body (avgPeriod 0)
 *
 *   0.1*10.0 is exactly 1.0 in IEEE-754 and the library reaches it by the same
 *   multiplication, so a flip can sit ON the comparison boundary instead of a
 *   safe distance from it. Every flip below does: where the library tests `>`
 *   or `<` the flip is placed at equality, and where it tests `<=` the paired
 *   control is placed at equality. A relaxed comparison therefore fails a case
 *   rather than passing one, which is what the whole-unit-off flips in #219's
 *   point 4 could not do.
 *
 * A single-bar pattern has no bar-to-bar coupling, so nothing here needs a
 * waiver: all 13 conditions across the six are independently falsifiable and
 * all 13 get a flip.
 */

/* CDLDOJI -- one condition: the body is no larger than the doji threshold.
 *
 *   c0  realbody(i) <= avg(BodyDoji)
 *
 * The control sits exactly at equality, which is what pins the comparison as
 * inclusive: were it `<`, the control would stop firing.
 */
static void cond_doji( int i, int *c )
{
   c[0] = pb_body(i) <= pb_avg(TA_BodyDoji, i);
}

static void build_doji( void )
{
  pb_conditions(1);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,105,95,100);            /* body 0 */
  pb_detect(d,100,"detect: body 0 <= 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,105,95,102);           /* body 2 */
  pb_flip(f0,0,"break c0: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,105,95,101);           /* body 1 == avg */
  pb_control(k0,100,0,"restore c0: body 1 == 1, inclusive");
  pb_flat(8);
}

/* CDLBELTHOLD -- a long body whose OPENING side has no shadow to speak of.
 *
 *   c0  realbody(i) > avg(BodyLong)
 *   c1  ( white(i) && lowershadow(i) < avg(ShadowVeryShort) )
 *       || ( black(i) && uppershadow(i) < avg(ShadowVeryShort) )
 *
 * c1 is one condition, not two: the colour selects which shadow is read, so
 * the disjunction is the atomic thing the decision tests. A white scenario
 * falsifies the black arm by construction, so raising the lower shadow to the
 * threshold falsifies c1 as a whole.
 */
static void cond_belthold( int i, int *c )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_body(i) > pb_avg(TA_BodyLong, i);
   c[1] = (  pb_white(i) && pb_losh(i) < vs )
       || ( !pb_white(i) && pb_upsh(i) < vs );
}

static void build_belthold( void )
{
  pb_conditions(2);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,108,99.5,105);          /* white, body 5, lower shadow 0.5 */
  pb_detect(d,100,"detect: body 5 > 2, lower shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,108,99.5,102);         /* body 2 == avg, strict > fails */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,108,99.5,103);         /* body 3 */
  pb_control(k0,100,0,"restore c0: body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,108,99,105);           /* lower shadow 1 == avg */
  pb_flip(f1,1,"break c1: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,108,99.5,105);
  pb_control(k1,100,1,"restore c1: lower shadow 0.5 < 1");
  pb_flat(8);

  /* The BLACK arm. Everything above is white, which leaves the other half of
   * c1's disjunction and the -100 output class untouched -- the gap pb_signs()
   * exists to close. Black reads the UPPER shadow here, the mirror of the white
   * arm above and the opposite of CLOSINGMARUBOZU's black arm. */
  pb_primer(12,100,2,4);
  int db=pb_bar(105,105.5,97,100);         /* black, body 5, upper shadow 0.5 */
  pb_detect(db,-100,"detect black: body 5 > 2, upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1b=pb_bar(105,106,97,100);          /* upper shadow 1 == avg */
  pb_flip(f1b,1,"break c1 black: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1b=pb_bar(105,105.5,97,100);
  pb_control(k1b,-100,1,"restore c1 black: upper shadow 0.5 < 1");
  pb_flat(8);
}

/* CDLCLOSINGMARUBOZU -- the mirror of BELTHOLD: a long body whose CLOSING
 * side has no shadow to speak of.
 *
 *   c0  realbody(i) > avg(BodyLong)
 *   c1  ( white(i) && uppershadow(i) < avg(ShadowVeryShort) )
 *       || ( black(i) && lowershadow(i) < avg(ShadowVeryShort) )
 *
 * The two patterns differ only in which shadow each colour reads, so a
 * copy-paste between them would leave the model agreeing with the wrong
 * library function -- the flips below are on the upper shadow for white,
 * where BELTHOLD's are on the lower.
 */
static void cond_closingmarubozu( int i, int *c )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_body(i) > pb_avg(TA_BodyLong, i);
   c[1] = (  pb_white(i) && pb_upsh(i) < vs )
       || ( !pb_white(i) && pb_losh(i) < vs );
}

static void build_closingmarubozu( void )
{
  pb_conditions(2);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,105.5,99,105);          /* white, body 5, upper shadow 0.5 */
  pb_detect(d,100,"detect: body 5 > 2, upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,102.5,99,102);         /* body 2 == avg */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,103.5,99,103);         /* body 3 */
  pb_control(k0,100,0,"restore c0: body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,106,99,105);           /* upper shadow 1 == avg */
  pb_flip(f1,1,"break c1: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,105.5,99,105);
  pb_control(k1,100,1,"restore c1: upper shadow 0.5 < 1");
  pb_flat(8);

  /* The BLACK arm, which here reads the LOWER shadow -- exactly the exchange
   * that separates this pattern from BELTHOLD. A model copy-pasted between the
   * two stays self-consistent on the white arm alone, so this is the pair that
   * makes the copy fail. */
  pb_primer(12,100,2,4);
  int db=pb_bar(105,108,99.5,100);         /* black, body 5, lower shadow 0.5 */
  pb_detect(db,-100,"detect black: body 5 > 2, lower shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1b=pb_bar(105,108,99,100);          /* lower shadow 1 == avg */
  pb_flip(f1b,1,"break c1 black: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1b=pb_bar(105,108,99.5,100);
  pb_control(k1b,-100,1,"restore c1 black: lower shadow 0.5 < 1");
  pb_flat(8);
}

/* CDLLONGLEGGEDDOJI -- a doji with at least one shadow longer than its body.
 *
 *   c0  realbody(i) <= avg(BodyDoji)
 *   c1  lowershadow(i) > avg(ShadowLong) || uppershadow(i) > avg(ShadowLong)
 *
 * ShadowLong has avgPeriod 0, so avg(ShadowLong) is the CURRENT bar's real
 * body rather than a windowed mean. That is the one place in this unit where
 * the threshold moves with the bar under test, and it is why c1's flip cannot
 * be built by shrinking a shadow while holding the body: at body 0 the
 * threshold is 0 and only a shadowless bar falsifies it. The flip is therefore
 * the degenerate O=H=L=C bar, which is a valid candle and leaves c0 true.
 */
static void cond_longleggeddoji( int i, int *c )
{
   double sl = pb_avg(TA_ShadowLong, i);
   c[0] = pb_body(i) <= pb_avg(TA_BodyDoji, i);
   c[1] = pb_losh(i) > sl || pb_upsh(i) > sl;
}

static void build_longleggeddoji( void )
{
  pb_conditions(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,104,96,100);            /* body 0, shadows 4 and 4 */
  pb_detect(d,100,"detect: body 0 <= 1, lower shadow 4 > 0");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,104,96,102);           /* body 2 > 1; shadows 4 and 2 still clear it */
  pb_flip(f0,0,"break c0: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,104,96,101);           /* body 1 == avg */
  pb_control(k0,100,0,"restore c0: body 1 == 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,100,100,100);          /* no shadows at all: 0 > 0 is false both sides */
  pb_flip(f1,1,"break c1: both shadows 0, threshold 0, strict >");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,104,96,100);
  pb_control(k1,100,1,"restore c1: lower shadow 4 > 0");
  pb_flat(8);
}

/* CDLDRAGONFLYDOJI -- a doji sitting at the top of its range.
 *
 *   c0  realbody(i)    <= avg(BodyDoji)
 *   c1  uppershadow(i) <  avg(ShadowVeryShort)
 *   c2  lowershadow(i) >  avg(ShadowVeryShort)
 *
 * c1 and c2 read the SAME threshold in opposite directions, so a model that
 * swapped them would still be self-consistent -- the flips are what separate
 * them: c1's raises the upper shadow to exactly 1 and c2's shrinks the lower
 * shadow to exactly 1, and each leaves the other satisfied.
 */
static void cond_dragonflydoji( int i, int *c )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_body(i) <= pb_avg(TA_BodyDoji, i);
   c[1] = pb_upsh(i) < vs;
   c[2] = pb_losh(i) > vs;
}

static void build_dragonflydoji( void )
{
  pb_conditions(3);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,100.5,95,100);          /* body 0, upper 0.5, lower 5 */
  pb_detect(d,100,"detect: body 0 <= 1, upper 0.5 < 1, lower 5 > 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,102.5,95,102);         /* body 2; upper 0.5; lower 5 */
  pb_flip(f0,0,"break c0: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,101.5,95,101);         /* body 1 == avg */
  pb_control(k0,100,0,"restore c0: body 1 == 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,101,95,100);           /* upper shadow 1 == avg */
  pb_flip(f1,1,"break c1: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,100.5,95,100);
  pb_control(k1,100,1,"restore c1: upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,100.5,99,100);         /* lower shadow 1 == avg */
  pb_flip(f2,2,"break c2: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,100.5,95,100);
  pb_control(k2,100,2,"restore c2: lower shadow 5 > 1");
  pb_flat(8);
}

/* CDLGRAVESTONEDOJI -- DRAGONFLY upside down: the doji sits at the bottom.
 *
 *   c0  realbody(i)    <= avg(BodyDoji)
 *   c1  lowershadow(i) <  avg(ShadowVeryShort)
 *   c2  uppershadow(i) >  avg(ShadowVeryShort)
 *
 * Identical in shape to DRAGONFLY with the two shadows exchanged, which is
 * exactly the mistake this pair is placed to catch: reusing DRAGONFLY's model
 * here would agree with itself and disagree with the library.
 */
static void cond_gravestonedoji( int i, int *c )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_body(i) <= pb_avg(TA_BodyDoji, i);
   c[1] = pb_losh(i) < vs;
   c[2] = pb_upsh(i) > vs;
}

static void build_gravestonedoji( void )
{
  pb_conditions(3);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,105,99.5,100);          /* body 0, lower 0.5, upper 5 */
  pb_detect(d,100,"detect: body 0 <= 1, lower 0.5 < 1, upper 5 > 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,105,99.5,102);         /* body 2; lower 0.5; upper 3 */
  pb_flip(f0,0,"break c0: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,105,99.5,101);         /* body 1 == avg */
  pb_control(k0,100,0,"restore c0: body 1 == 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,105,99,100);           /* lower shadow 1 == avg */
  pb_flip(f1,1,"break c1: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,105,99.5,100);
  pb_control(k1,100,1,"restore c1: lower shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,101,99.5,100);         /* upper shadow 1 == avg */
  pb_flip(f2,2,"break c2: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,105,99.5,100);
  pb_control(k2,100,2,"restore c2: upper shadow 5 > 1");
  pb_flat(8);
}

/* ---- Mechanical tier, unit 2: the remaining seven single-bar patterns ----- *
 *
 * Same primer as unit 1, pb_primer(12,100,2,4) -- RealBody 2, HighLow 10 -- and
 * the same discipline: every flip on the comparison boundary. Three more
 * thresholds come into play here, and all three are exact under that primer
 * (checked by running the arithmetic, not by assuming it):
 *
 *   avg(BodyShort)       = 1.0 * (20/10) / 1   = 2.0
 *   avg(ShadowShort)     = 1.0 * (80/10) / 2   = 4.0   <- Shadows-typed, so the
 *                                                         divisor is 2, not 1
 *   avg(Near)            = 0.2 * (50/5)  / 1   = 2.0   <- avgPeriod 5, not 10
 *   avg(ShadowVeryLong)  = 2.0 * body(i)       (avgPeriod 0)
 *   avg(ShadowLong)      = 1.0 * body(i)       (avgPeriod 0)
 *
 * The two avgPeriod-0 settings are the reason several flips below cannot be
 * built by moving a shadow while holding the body: the threshold moves with
 * the bar. Where that bites, the comment says so.
 *
 * Five of the seven are BI-SIGNED and carry pb_signs(2) with a firing scenario
 * on each colour. Unit 1 shipped two bi-signed patterns with white-only
 * scenarios, which left the -100 class and half of a disjunction unexercised;
 * that gap is what pb_signs() now refuses to let happen silently.
 *
 * No waivers: single-bar patterns have no cross-bar entailment, so all 22
 * conditions are independently falsifiable.
 */

/* CDLHIGHWAVE -- a small body with two very long shadows.
 *
 *   c0  realbody(i)    <  avg(BodyShort)
 *   c1  uppershadow(i) >  avg(ShadowVeryLong)      = 2 * realbody(i)
 *   c2  lowershadow(i) >  avg(ShadowVeryLong)
 *
 * c1 and c2 share a threshold that is a multiple of the CURRENT body, so the
 * c0 flip has to grow the shadows as it grows the body: at body 2 the shadow
 * threshold is 4, and a bar that merely kept the detect's shadows would break
 * three conditions at once instead of one.
 */
static void cond_highwave( int i, int *c )
{
   double vl = pb_avg(TA_ShadowVeryLong, i);
   c[0] = pb_body(i) < pb_avg(TA_BodyShort, i);
   c[1] = pb_upsh(i) > vl;
   c[2] = pb_losh(i) > vl;
}

static void build_highwave( void )
{
  pb_conditions(3);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,104,97,101);            /* white, body 1, shadows 3 and 3, threshold 2 */
  pb_detect(d,100,"detect: body 1 < 2, both shadows 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,107,94,102);           /* body 2 == avg; shadows 5 and 6 clear 4 */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,104,97,101);
  pb_control(k0,100,0,"restore c0: body 1 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,103,97,101);           /* upper shadow 2 == 2*body */
  pb_flip(f1,1,"break c1: upper shadow 2 == threshold 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,104,97,101);
  pb_control(k1,100,1,"restore c1: upper shadow 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,104,98,101);           /* lower shadow 2 == 2*body */
  pb_flip(f2,2,"break c2: lower shadow 2 == threshold 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,104,97,101);
  pb_control(k2,100,2,"restore c2: lower shadow 3 > 2");
  pb_flat(8);

  /* The BLACK arm: same geometry with open and close exchanged. */
  pb_primer(12,100,2,4);
  int db=pb_bar(101,104,97,100);
  pb_detect(db,-100,"detect black: body 1 < 2, both shadows 3 > 2");
  pb_flat(8);
}

/* CDLLONGLINE -- a long body with short shadows.
 *
 *   c0  realbody(i)    >  avg(BodyLong)
 *   c1  uppershadow(i) <  avg(ShadowShort)
 *   c2  lowershadow(i) <  avg(ShadowShort)
 *
 * ShadowShort is the one Shadows-typed setting in this unit, so its threshold
 * carries the halving divisor: 8/2, not 8. A model that forgot the divisor
 * would put the threshold at 8 and agree with the library on every scenario
 * whose shadows are under 4 -- which is why both flips sit exactly at 4.
 */
static void cond_longline( int i, int *c )
{
   double ss = pb_avg(TA_ShadowShort, i);
   c[0] = pb_body(i) > pb_avg(TA_BodyLong, i);
   c[1] = pb_upsh(i) < ss;
   c[2] = pb_losh(i) < ss;
}

static void build_longline( void )
{
  pb_conditions(3);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,108,98,105);            /* white, body 5, upper 3, lower 2 */
  pb_detect(d,100,"detect: body 5 > 2, shadows 3 and 2 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,105,98,102);           /* body 2 == avg */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,106,98,103);           /* body 3 */
  pb_control(k0,100,0,"restore c0: body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,109,98,105);           /* upper shadow 4 == avg */
  pb_flip(f1,1,"break c1: upper shadow 4 == avg 4, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,108,98,105);
  pb_control(k1,100,1,"restore c1: upper shadow 3 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,108,96,105);           /* lower shadow 4 == avg */
  pb_flip(f2,2,"break c2: lower shadow 4 == avg 4, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,108,98,105);
  pb_control(k2,100,2,"restore c2: lower shadow 2 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int db=pb_bar(105,108,98,100);
  pb_detect(db,-100,"detect black: body 5 > 2, shadows 3 and 2 < 4");
  pb_flat(8);
}

/* CDLMARUBOZU -- a long body with almost no shadow at either end.
 *
 *   c0  realbody(i)    >  avg(BodyLong)
 *   c1  uppershadow(i) <  avg(ShadowVeryShort)
 *   c2  lowershadow(i) <  avg(ShadowVeryShort)
 *
 * Same shape as LONGLINE with a much tighter shadow threshold (1 rather than
 * 4), and that difference is the whole distinction between the two patterns.
 * The flips are placed at each pattern's own threshold, so a model that
 * borrowed the other's setting fails here rather than agreeing by luck.
 */
static void cond_marubozu( int i, int *c )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_body(i) > pb_avg(TA_BodyLong, i);
   c[1] = pb_upsh(i) < vs;
   c[2] = pb_losh(i) < vs;
}

static void build_marubozu( void )
{
  pb_conditions(3);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,105.5,99.5,105);        /* white, body 5, both shadows 0.5 */
  pb_detect(d,100,"detect: body 5 > 2, both shadows 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,102.5,99.5,102);       /* body 2 == avg */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,103.5,99.5,103);       /* body 3 */
  pb_control(k0,100,0,"restore c0: body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,106,99.5,105);         /* upper shadow 1 == avg */
  pb_flip(f1,1,"break c1: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,105.5,99.5,105);
  pb_control(k1,100,1,"restore c1: upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,105.5,99,105);         /* lower shadow 1 == avg */
  pb_flip(f2,2,"break c2: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,105.5,99.5,105);
  pb_control(k2,100,2,"restore c2: lower shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int db=pb_bar(105,105.5,99.5,100);
  pb_detect(db,-100,"detect black: body 5 > 2, both shadows 0.5 < 1");
  pb_flat(8);
}

/* CDLSHORTLINE -- a short body with short shadows.
 *
 *   c0  realbody(i)    <  avg(BodyShort)
 *   c1  uppershadow(i) <  avg(ShadowShort)
 *   c2  lowershadow(i) <  avg(ShadowShort)
 *
 * The exact complement of LONGLINE on c0 and identical on c1/c2, so the c0
 * flip is the one that separates them: at body 2 SHORTLINE stops firing and
 * LONGLINE starts.
 */
static void cond_shortline( int i, int *c )
{
   double ss = pb_avg(TA_ShadowShort, i);
   c[0] = pb_body(i) < pb_avg(TA_BodyShort, i);
   c[1] = pb_upsh(i) < ss;
   c[2] = pb_losh(i) < ss;
}

static void build_shortline( void )
{
  pb_conditions(3);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,103,98,101);            /* white, body 1, shadows 2 and 2 */
  pb_detect(d,100,"detect: body 1 < 2, both shadows 2 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,104,98,102);           /* body 2 == avg */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,103,98,101);
  pb_control(k0,100,0,"restore c0: body 1 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,105,98,101);           /* upper shadow 4 == avg */
  pb_flip(f1,1,"break c1: upper shadow 4 == avg 4, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,103,98,101);
  pb_control(k1,100,1,"restore c1: upper shadow 2 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,103,96,101);           /* lower shadow 4 == avg */
  pb_flip(f2,2,"break c2: lower shadow 4 == avg 4, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,103,98,101);
  pb_control(k2,100,2,"restore c2: lower shadow 2 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int db=pb_bar(101,103,98,100);
  pb_detect(db,-100,"detect black: body 1 < 2, both shadows 2 < 4");
  pb_flat(8);
}

/* CDLSPINNINGTOP -- a small body with shadows longer than itself.
 *
 *   c0  uppershadow(i) >  realbody(i)
 *   c1  lowershadow(i) >  realbody(i)
 *   c2  realbody(i)    <  avg(BodyShort)
 *
 * The only pattern in this unit whose first two thresholds are the body
 * ITSELF rather than a setting -- no candle setting is consulted for c0 or c1.
 * That makes the c2 flip the awkward one: growing the body to the BodyShort
 * threshold also raises c0's and c1's thresholds, so the shadows have to grow
 * with it or three conditions break at once.
 */
static void cond_spinningtop( int i, int *c )
{
   double b = pb_body(i);
   c[0] = pb_upsh(i) > b;
   c[1] = pb_losh(i) > b;
   c[2] = b < pb_avg(TA_BodyShort, i);
}

static void build_spinningtop( void )
{
  pb_conditions(3);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,103,98,101);            /* white, body 1, shadows 2 and 2 */
  pb_detect(d,100,"detect: shadows 2 and 2 > body 1, body 1 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,102,98,101);           /* upper shadow 1 == body */
  pb_flip(f0,0,"break c0: upper shadow 1 == body 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,103,98,101);
  pb_control(k0,100,0,"restore c0: upper shadow 2 > body 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,103,99,101);           /* lower shadow 1 == body */
  pb_flip(f1,1,"break c1: lower shadow 1 == body 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,103,98,101);
  pb_control(k1,100,1,"restore c1: lower shadow 2 > body 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,105,97,102);           /* body 2 == avg; shadows 3 and 3 still clear it */
  pb_flip(f2,2,"break c2: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,103,98,101);
  pb_control(k2,100,2,"restore c2: body 1 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int db=pb_bar(101,103,98,100);
  pb_detect(db,-100,"detect black: shadows 2 and 2 > body 1, body 1 < 2");
  pb_flat(8);
}

/* CDLTAKURI -- a dragonfly doji whose lower shadow is very long.
 *
 *   c0  realbody(i)    <= avg(BodyDoji)
 *   c1  uppershadow(i) <  avg(ShadowVeryShort)
 *   c2  lowershadow(i) >  avg(ShadowVeryLong)      = 2 * realbody(i)
 *
 * Single-class: the firing arm is a literal 100, so pb_signs stays at its
 * default of 1. c2's threshold is twice the CURRENT body, which at a body of 0
 * is 0 -- so the only bar that falsifies c2 alone is one with no lower shadow
 * at all, and the c0 flip has to keep a lower shadow above the RAISED
 * threshold rather than the detect's.
 */
static void cond_takuri( int i, int *c )
{
   c[0] = pb_body(i) <= pb_avg(TA_BodyDoji, i);
   c[1] = pb_upsh(i) <  pb_avg(TA_ShadowVeryShort, i);
   c[2] = pb_losh(i) >  pb_avg(TA_ShadowVeryLong, i);
}

static void build_takuri( void )
{
  pb_conditions(3);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,100.5,95,100);          /* body 0, upper 0.5, lower 5, threshold 0 */
  pb_detect(d,100,"detect: body 0 <= 1, upper 0.5 < 1, lower 5 > 0");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,102.5,95,102);         /* body 2 > 1; lower 5 still clears 2*2=4 */
  pb_flip(f0,0,"break c0: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,101.5,95,101);         /* body 1 == avg, inclusive; lower 5 > 2 */
  pb_control(k0,100,0,"restore c0: body 1 == 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,101,95,100);           /* upper shadow 1 == avg */
  pb_flip(f1,1,"break c1: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,100.5,95,100);
  pb_control(k1,100,1,"restore c1: upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,100.5,100,100);        /* no lower shadow: 0 > 0 is false */
  pb_flip(f2,2,"break c2: lower shadow 0 == threshold 0, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,100.5,95,100);
  pb_control(k2,100,2,"restore c2: lower shadow 5 > 0");
  pb_flat(8);
}

/* CDLRICKSHAWMAN -- a long-legged doji whose body sits near the midpoint.
 *
 *   c0  realbody(i)    <= avg(BodyDoji)
 *   c1  lowershadow(i) >  avg(ShadowLong)          = realbody(i)
 *   c2  uppershadow(i) >  avg(ShadowLong)
 *   c3  bodylo(i) <= low + hlrange/2 + avg(Near)
 *       && bodyhi(i) >= low + hlrange/2 - avg(Near)
 *
 * c3 is a single top-level conjunct holding a two-sided band, the same shape
 * as BELTHOLD's disjunction in unit 1: pb_conditions() counts what the
 * decision tests, and this is one test of "near the midpoint". Its flip pushes
 * the body clear of the band's upper edge -- and because the band is measured
 * from the midpoint, moving the body also moves the shadows, so the flip has
 * to keep both of them above their own threshold.
 *
 * Near is the only setting in either unit with avgPeriod 5 rather than 10.
 * Under this primer that changes nothing (every primer bar has HighLow 10, so
 * a 5-bar mean and a 10-bar mean agree), which is worth stating: the exactness
 * of 2.0 here does not depend on the window length.
 */
static void cond_rickshawman( int i, int *c )
{
   double sl   = pb_avg(TA_ShadowLong, i);
   double near = pb_avg(TA_Near, i);
   double mid  = pbL[i] + (pbH[i] - pbL[i]) / 2;
   c[0] = pb_body(i) <= pb_avg(TA_BodyDoji, i);
   c[1] = pb_losh(i) > sl;
   c[2] = pb_upsh(i) > sl;
   c[3] = pb_bodylo(i) <= mid + near && pb_bodyhi(i) >= mid - near;
}

static void build_rickshawman( void )
{
  pb_conditions(4);

  pb_flat(6);
  pb_primer(12,100,2,4);
  int d=pb_bar(100,104,96,100);            /* body 0 at the midpoint, shadows 4 and 4 */
  pb_detect(d,100,"detect: body 0 <= 1, both shadows 4 > 0, body on the midpoint");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f0=pb_bar(100,106,94,102);           /* body 2 > 1; shadows 4 and 6 clear 2; band +/-2 around 100 */
  pb_flip(f0,0,"break c0: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k0=pb_bar(100,105,95,101);           /* body 1 == avg, inclusive */
  pb_control(k0,100,0,"restore c0: body 1 == 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f1=pb_bar(100,104,100,100);          /* lower shadow 0 == threshold 0 */
  pb_flip(f1,1,"break c1: lower shadow 0 == threshold 0, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k1=pb_bar(100,104,96,100);
  pb_control(k1,100,1,"restore c1: lower shadow 4 > 0");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f2=pb_bar(100,100,96,100);           /* upper shadow 0 == threshold 0 */
  pb_flip(f2,2,"break c2: upper shadow 0 == threshold 0, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k2=pb_bar(100,104,96,100);
  pb_control(k2,100,2,"restore c2: upper shadow 4 > 0");
  pb_flat(8);

  /* c3: the body must sit inside a band of +/-Near around the bar's midpoint.
   * With high 110 and low 96 the midpoint is 103 and Near is 2, so the band is
   * [101,105].
   *
   * Both bars here carry a real body of 1 rather than the doji used above, and
   * that is load-bearing: c3 reads min(open,close) against the band's top and
   * max(open,close) against its bottom, so a scenario with open == close makes
   * those two terms the SAME NUMBER and any confusion between them invisible.
   * A mutation swapping the min for a max sailed through an earlier version of
   * these cases for exactly that reason -- it was the v0.6.4 freeze that caught
   * it, and the freeze reaches only 35 of the 61 patterns.
   *
   * The control puts min(open,close) exactly ON 105, which pins the comparison
   * as inclusive, while max sits at 106 above it -- so the two terms disagree
   * and reading the wrong one stops the control firing.
   */
  pb_primer(12,100,2,4);
  int f3=pb_bar(106,110,96,107);           /* min 106 is above the band top 105 */
  pb_flip(f3,3,"break c3: min(open,close) 106 > band top 105");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k3=pb_bar(105,110,96,106);           /* min 105 == band top, max 106 above it */
  pb_control(k3,100,3,"restore c3: min 105 == band top, inclusive; max 106 differs");
  pb_flat(8);
}

static ErrorNumber test_marquee_predicate_coverage( void )
{
   ErrorNumber e;
   pb_reset(); build_2crows();      e = pb_check_mcdc("CDL2CROWS",      TA_CDL2CROWS, cond_2crows);      if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3blackcrows(); e = pb_check_mcdc("CDL3BLACKCROWS", TA_CDL3BLACKCROWS, cond_3blackcrows); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3whitesoldiers(); e = pb_check_mcdc("CDL3WHITESOLDIERS", TA_CDL3WHITESOLDIERS, cond_3whitesoldiers); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3starsinsouth(); e = pb_check_mcdc("CDL3STARSINSOUTH", TA_CDL3STARSINSOUTH, cond_3starsinsouth); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_doji();             e = pb_check_mcdc("CDLDOJI",             TA_CDLDOJI,             cond_doji);             if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_belthold();         e = pb_check_mcdc("CDLBELTHOLD",         TA_CDLBELTHOLD,         cond_belthold);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_closingmarubozu();  e = pb_check_mcdc("CDLCLOSINGMARUBOZU",  TA_CDLCLOSINGMARUBOZU,  cond_closingmarubozu);  if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_longleggeddoji();   e = pb_check_mcdc("CDLLONGLEGGEDDOJI",   TA_CDLLONGLEGGEDDOJI,   cond_longleggeddoji);   if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_dragonflydoji();    e = pb_check_mcdc("CDLDRAGONFLYDOJI",    TA_CDLDRAGONFLYDOJI,    cond_dragonflydoji);    if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_gravestonedoji();   e = pb_check_mcdc("CDLGRAVESTONEDOJI",   TA_CDLGRAVESTONEDOJI,   cond_gravestonedoji);   if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_highwave();          e = pb_check_mcdc("CDLHIGHWAVE",          TA_CDLHIGHWAVE,          cond_highwave);          if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_longline();          e = pb_check_mcdc("CDLLONGLINE",          TA_CDLLONGLINE,          cond_longline);          if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_marubozu();          e = pb_check_mcdc("CDLMARUBOZU",          TA_CDLMARUBOZU,          cond_marubozu);          if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_shortline();         e = pb_check_mcdc("CDLSHORTLINE",         TA_CDLSHORTLINE,         cond_shortline);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_spinningtop();       e = pb_check_mcdc("CDLSPINNINGTOP",       TA_CDLSPINNINGTOP,       cond_spinningtop);       if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_takuri();            e = pb_check_mcdc("CDLTAKURI",            TA_CDLTAKURI,            cond_takuri);            if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_rickshawman();       e = pb_check_mcdc("CDLRICKSHAWMAN",       TA_CDLRICKSHAWMAN,       cond_rickshawman);       if( e != TA_TEST_PASS ) return e;
   pb_report_totals();
   return TA_TEST_PASS;
}

/* Push one matrix row into the library. Fails loudly rather than skipping: a
 * rejected setting would leave the sweep comparing a row it never applied. */
static ErrorNumber apply_cdl_globals( const TA_CDLGlobals *g )
{
   static const struct { size_t typeOff, avgOff, factorOff; } field[] = {
      { offsetof(TA_CDLGlobals, bodyLong_type),        offsetof(TA_CDLGlobals, bodyLong_avg),        offsetof(TA_CDLGlobals, bodyLong_factor) },
      { offsetof(TA_CDLGlobals, bodyVeryLong_type),    offsetof(TA_CDLGlobals, bodyVeryLong_avg),    offsetof(TA_CDLGlobals, bodyVeryLong_factor) },
      { offsetof(TA_CDLGlobals, bodyShort_type),       offsetof(TA_CDLGlobals, bodyShort_avg),       offsetof(TA_CDLGlobals, bodyShort_factor) },
      { offsetof(TA_CDLGlobals, bodyDoji_type),        offsetof(TA_CDLGlobals, bodyDoji_avg),        offsetof(TA_CDLGlobals, bodyDoji_factor) },
      { offsetof(TA_CDLGlobals, shadowLong_type),      offsetof(TA_CDLGlobals, shadowLong_avg),      offsetof(TA_CDLGlobals, shadowLong_factor) },
      { offsetof(TA_CDLGlobals, shadowVeryLong_type),  offsetof(TA_CDLGlobals, shadowVeryLong_avg),  offsetof(TA_CDLGlobals, shadowVeryLong_factor) },
      { offsetof(TA_CDLGlobals, shadowShort_type),     offsetof(TA_CDLGlobals, shadowShort_avg),     offsetof(TA_CDLGlobals, shadowShort_factor) },
      { offsetof(TA_CDLGlobals, shadowVeryShort_type), offsetof(TA_CDLGlobals, shadowVeryShort_avg), offsetof(TA_CDLGlobals, shadowVeryShort_factor) },
      { offsetof(TA_CDLGlobals, near_type),            offsetof(TA_CDLGlobals, near_avg),            offsetof(TA_CDLGlobals, near_factor) },
      { offsetof(TA_CDLGlobals, far_type),             offsetof(TA_CDLGlobals, far_avg),             offsetof(TA_CDLGlobals, far_factor) },
      { offsetof(TA_CDLGlobals, equal_type),           offsetof(TA_CDLGlobals, equal_avg),           offsetof(TA_CDLGlobals, equal_factor) },
   };
   const char *base = (const char *)g;
   TA_RetCode retCode;
   int i;

   /* One row per setting, in TA_CandleSettingType order, or the loop below
    * would set the wrong slots. Constant-folded away when it holds. */
   if( sizeof(field)/sizeof(field[0]) != (size_t)TA_AllCandleSettings )
      return TA_CDLSET_SETTING_REJECTED;

   for( i = 0; i < (int)TA_AllCandleSettings; i++ )
   {
      TA_RangeType rt;
      int          avg;
      double       factor;
      memcpy(&rt,     base + field[i].typeOff,   sizeof(rt));
      memcpy(&avg,    base + field[i].avgOff,    sizeof(avg));
      memcpy(&factor, base + field[i].factorOff, sizeof(factor));

      retCode = TA_SetCandleSettings( (TA_CandleSettingType)i, rt, avg, factor );
      if( retCode != TA_SUCCESS )
      {
         printf( "TA_SetCandleSettings(%d,%d,%d,%f) rejected [%d]\n",
                 i, (int)rt, avg, factor, retCode );
         return TA_CDLSET_SETTING_REJECTED;
      }
   }
   return TA_TEST_PASS;
}

/* Did this row change what C computes, versus the defaults row?
 *
 * Aligned on the BAR index, not on the buffer offset. A row that changes an
 * avgPeriod changes the lookback, so the two runs start at different bars and
 * produce different counts -- comparing element 0 against element 0 would diff
 * two unrelated bars, and comparing over the current row's count would read
 * past what the defaults row actually wrote. (Doing exactly that made this
 * counter fluctuate run to run: row 2 zeroes every avgPeriod, which SHRINKS the
 * lookback, so the current row is the longer one and the overrun landed in
 * uninitialized malloc memory.)
 *
 * A differing begin index or count is itself a move -- the lookback moved.
 */
/* Did a VALUE change on a bar both runs produced? Distinct from cdl_output_moved,
 * which also counts a lookback shift. Both are real changes, but only this one
 * says the pattern decided differently about a bar it actually evaluated -- and
 * a headline "196 outputs moved" built mostly from lookback shifts on all-zero
 * output would overstate what the matrix proves. */
static int cdl_value_moved( const int *ref, int refBeg, int refNb,
                            const int *cur, int curBeg, int curNb, int *sawNonZero )
{
   int firstBar = (refBeg > curBeg) ? refBeg : curBeg;
   int lastBar  = ((refBeg + refNb) < (curBeg + curNb) ? (refBeg + refNb)
                                                       : (curBeg + curNb)) - 1;
   int bar, diff = 0;
   for( bar = firstBar; bar <= lastBar; bar++ )
   {
      int a = ref[bar - refBeg], b = cur[bar - curBeg];
      if( a || b ) *sawNonZero = 1;
      if( a != b ) diff = 1;
   }
   return diff;
}

static int cdl_output_moved( const int *ref, int refBeg, int refNb,
                             const int *cur, int curBeg, int curNb )
{
   int firstBar, lastBar, bar;

   if( refBeg != curBeg || refNb != curNb )
      return 1;

   firstBar = (refBeg > curBeg) ? refBeg : curBeg;
   lastBar  = ((refBeg + refNb) < (curBeg + curNb) ? (refBeg + refNb)
                                                   : (curBeg + curNb)) - 1;
   for( bar = firstBar; bar <= lastBar; bar++ )
   {
      if( ref[bar - refBeg] != cur[bar - curBeg] )
         return 1;
   }
   return 0;
}

/* Per-setting coverage.
 *
 * A row that varies eleven settings at once proves nothing about any single
 * one: the matrix could exercise BodyLong thoroughly and never touch Far, and
 * the headline count would look identical. For each setting, this puts one
 * setting back to its default while the rest of the row stays custom, and
 * counts the (row, function) pairs whose output changes. A zero means the
 * matrix says nothing about that setting.
 *
 * TA_BodyVeryLong is exempt and the exemption is the finding: it is part of the
 * public TA_SetCandleSettings API and NO shipped indicator reads it
 * (`grep -lw TA_BodyVeryLong src/ta_func/ta_CDL*.c` is empty). No matrix can
 * make it matter, so requiring coverage of it would be requiring a lie.
 */
static ErrorNumber cdl_setting_coverage( const TA_History *history, int perSetting[] )
{
   int nbBars = (int)history->nbBars;
   int *outA = (int *)malloc((size_t)nbBars * sizeof(int));
   int *outB = (int *)malloc((size_t)nbBars * sizeof(int));
   ErrorNumber errNb = TA_TEST_PASS;
   int st, r;
   unsigned int f;

   if( !outA || !outB ) { free(outA); free(outB); return TA_CDLSET_CALL_FAILED; }

   for( st = 0; st < (int)TA_AllCandleSettings; st++ )
   {
      perSetting[st] = 0;
      for( r = 1; r < (int)NB_CDL_GLOBALS && errNb == TA_TEST_PASS; r++ )
      {
         for( f = 0; f < NB_TEST; f++ )
         {
            int begA=0, nbA=0, begB=0, nbB=0, lb=0;
            TA_RetCode rcA=TA_SUCCESS, rcB=TA_SUCCESS;
            TA_ParamHolder *ph;

            errNb = apply_cdl_globals( &cdlGlobalsMatrix[r] );
            if( errNb != TA_TEST_PASS ) break;
            ph = NULL;
            errNb = callCandlestick( &ph, tableTest[f].name, 0, nbBars-1,
                                     history->open, history->high,
                                     history->low, history->close,
                                     tableTest[f].params, &begA, &nbA,
                                     outA, &lb, &rcA );
            if( ph ) TA_ParamHolderFree( ph );
            if( errNb != TA_TEST_PASS ) break;

            /* Same row, this one setting back at its default. */
            errNb = apply_cdl_globals( &cdlGlobalsMatrix[r] );
            if( errNb != TA_TEST_PASS ) break;
            if( TA_RestoreCandleDefaultSettings( (TA_CandleSettingType)st ) != TA_SUCCESS )
            { errNb = TA_CDLSET_RESTORE_FAILED; break; }
            ph = NULL;
            errNb = callCandlestick( &ph, tableTest[f].name, 0, nbBars-1,
                                     history->open, history->high,
                                     history->low, history->close,
                                     tableTest[f].params, &begB, &nbB,
                                     outB, &lb, &rcB );
            if( ph ) TA_ParamHolderFree( ph );
            if( errNb != TA_TEST_PASS ) break;

            if( cdl_output_moved(outA, begA, nbA, outB, begB, nbB) )
               perSetting[st]++;
         }
      }
   }

   free(outA); free(outB);
   return errNb;
}

/* Run every candlestick under every matrix row, in C and (when servers are up)
 * in every language, and prove the sweep is not vacuous.
 *
 * Three assertions, in ascending order of what they can catch:
 *   - the C output MOVED for some (row, function) pair vs the defaults row.
 *     A matrix whose rows all compute the defaults tests nothing;
 *   - server_verify() agreed, per call, bit-for-bit. That is the cross-language
 *     half, and it is what the #215 transport bought;
 *   - restoring the defaults reproduces row 0's output exactly, so the sweep
 *     cannot leave the library in a state later groups inherit.
 */
static ErrorNumber test_candle_settings_matrix( const TA_History *history )
{
   /* Two output buffers: the defaults row is kept for the whole sweep so every
    * later row can be diffed against it. */
   int *outDefault = NULL;
   int *outCur     = NULL;
   int *defBeg     = NULL;   /* row 0's outBegIdx / outNBElement, per function: */
   int *defNb      = NULL;   /* every later row is aligned against these.       */
   TA_ParamHolder *paramHolder;
   ErrorNumber errNb = TA_TEST_PASS;
   int nbBars = (int)history->nbBars;
   unsigned int r, f;
   int moved = 0, valueMoved = 0, nonZeroPairs = 0, calls = 0, restoredMismatch = 0;
   int syncsBefore = server_verify_candle_syncs();

   outDefault = (int *)malloc((size_t)nbBars * NB_TEST * sizeof(int));
   outCur     = (int *)malloc((size_t)nbBars * sizeof(int));
   defBeg     = (int *)malloc(NB_TEST * sizeof(int));
   defNb      = (int *)malloc(NB_TEST * sizeof(int));
   if( !outDefault || !outCur || !defBeg || !defNb )
   {
      free(outDefault); free(outCur); free(defBeg); free(defNb);
      return TA_CDLSET_CALL_FAILED;
   }

   for( r = 0; r < NB_CDL_GLOBALS && errNb == TA_TEST_PASS; r++ )
   {
      errNb = apply_cdl_globals( &cdlGlobalsMatrix[r] );
      if( errNb != TA_TEST_PASS )
         break;

      for( f = 0; f < NB_TEST; f++ )
      {
         int outBegIdx = 0, outNbElement = 0, lookback = 0;
         TA_RetCode taFuncRetCode = TA_SUCCESS;
         const TA_Real *inputs[5];
         const TA_Integer *outInteger[2];
         int *dst = (r == 0) ? (outDefault + (size_t)f * nbBars) : outCur;

         paramHolder = NULL;
         errNb = callCandlestick( &paramHolder,
                                  tableTest[f].name,
                                  0, nbBars - 1,
                                  history->open, history->high,
                                  history->low,  history->close,
                                  tableTest[f].params,
                                  &outBegIdx, &outNbElement,
                                  dst, &lookback, &taFuncRetCode );
         if( paramHolder )
            TA_ParamHolderFree( paramHolder );
         if( errNb != TA_TEST_PASS )
            break;
         calls++;

         if( r == 0 )
         {
            defBeg[f] = outBegIdx;
            defNb[f]  = outNbElement;
         }
         else
         {
            int sawNonZero = 0;
            if( cdl_output_moved( outDefault + (size_t)f * nbBars,
                                  defBeg[f], defNb[f],
                                  dst, outBegIdx, outNbElement ) )
               moved++;
            if( cdl_value_moved( outDefault + (size_t)f * nbBars,
                                 defBeg[f], defNb[f],
                                 dst, outBegIdx, outNbElement, &sawNonZero ) )
               valueMoved++;
            if( sawNonZero )
               nonZeroPairs++;
         }

         /* Cross-language: server_verify carries this row's settings to every
          * server through sync_candle_settings before comparing (#215). */
         inputs[0] = history->open;
         inputs[1] = history->high;
         inputs[2] = history->low;
         inputs[3] = history->close;
         inputs[4] = NULL;
         outInteger[0] = dst;
         outInteger[1] = NULL;
         errNb = server_verify( tableTest[f].name,
                                0, nbBars - 1, nbBars,
                                taFuncRetCode, outBegIdx, outNbElement,
                                inputs, NULL, 0, NULL, outInteger );
         if( errNb != TA_TEST_PASS )
         {
            printf( "Failed: candle settings matrix row %u, %s (cross-language)\n",
                    r, tableTest[f].name );
            errNb = TA_CDLSET_XLANG_MISMATCH;
            break;
         }
      }
   }

   /* Always restore, even on failure: later groups inherit these globals. */
   if( TA_RestoreCandleDefaultSettings( TA_AllCandleSettings ) != TA_SUCCESS )
   {
      free(outDefault); free(outCur); free(defBeg); free(defNb);
      return TA_CDLSET_RESTORE_FAILED;
   }
   if( errNb != TA_TEST_PASS )
   {
      free(outDefault); free(outCur); free(defBeg); free(defNb);
      return errNb;
   }

   /* The restore must reproduce row 0 exactly -- otherwise the sweep leaks a
    * changed threshold into every group that runs after it. */
   for( f = 0; f < NB_TEST; f++ )
   {
      int outBegIdx = 0, outNbElement = 0, lookback = 0;
      TA_RetCode taFuncRetCode = TA_SUCCESS;
      paramHolder = NULL;
      errNb = callCandlestick( &paramHolder, tableTest[f].name,
                               0, nbBars - 1,
                               history->open, history->high,
                               history->low,  history->close,
                               tableTest[f].params,
                               &outBegIdx, &outNbElement,
                               outCur, &lookback, &taFuncRetCode );
      if( paramHolder )
         TA_ParamHolderFree( paramHolder );
      if( errNb != TA_TEST_PASS )
      {
         free(outDefault); free(outCur); free(defBeg); free(defNb);
         return errNb;
      }
      /* Same bar-aligned comparison the move counter uses -- here it must find
       * NO difference, index and count included. */
      if( cdl_output_moved( outDefault + (size_t)f * nbBars, defBeg[f], defNb[f],
                            outCur, outBegIdx, outNbElement ) )
      {
         printf( "Failed: %s differs after restoring the candle defaults\n",
                 tableTest[f].name );
         restoredMismatch++;
      }

      /* Verified cross-language too, which is what carries the restore OUT to
       * the servers: sync_candle_settings pushes the delta back to the defaults
       * before comparing. Without this the servers would still be holding the
       * last row's settings when this group ends, and "restore works" would be
       * a C-only claim. */
      {
         const TA_Real *inputs[5];
         const TA_Integer *outInteger[2];
         inputs[0] = history->open;
         inputs[1] = history->high;
         inputs[2] = history->low;
         inputs[3] = history->close;
         inputs[4] = NULL;
         outInteger[0] = outCur;
         outInteger[1] = NULL;
         errNb = server_verify( tableTest[f].name,
                                0, nbBars - 1, nbBars,
                                taFuncRetCode, outBegIdx, outNbElement,
                                inputs, NULL, 0, NULL, outInteger );
         if( errNb != TA_TEST_PASS )
         {
            printf( "Failed: %s after restoring the candle defaults "
                    "(cross-language)\n", tableTest[f].name );
            free(outDefault); free(outCur); free(defBeg); free(defNb);
            return TA_CDLSET_XLANG_MISMATCH;
         }
      }
   }

   free(outDefault);
   free(outCur);
   free(defBeg);
   free(defNb);

   if( restoredMismatch )
      return TA_CDLSET_NOT_RESTORED;

   printf( "  Candle settings matrix: %u row(s) x %u function(s), %d call(s), "
           "%d moved (%d by value, %d on a non-zero output), %d setting(s) "
           "pushed to the language servers\n",
           (unsigned int)NB_CDL_GLOBALS, (unsigned int)NB_TEST, calls, moved,
           valueMoved, nonZeroPairs, server_verify_candle_syncs() - syncsBefore );

   /* Per-setting coverage. Without this the totals above can be carried
    * entirely by two or three popular settings while the rest are inert. */
   {
      static const char *SETTING_NAME[] = {
         "BodyLong","BodyVeryLong","BodyShort","BodyDoji","ShadowLong",
         "ShadowVeryLong","ShadowShort","ShadowVeryShort","Near","Far","Equal" };
      int perSetting[TA_AllCandleSettings];
      int st, inert = 0;
      errNb = cdl_setting_coverage( history, perSetting );
      if( errNb != TA_TEST_PASS )
         return errNb;
      printf( "  Candle settings matrix, per setting:" );
      for( st = 0; st < (int)TA_AllCandleSettings; st++ )
         printf( " %s=%d", SETTING_NAME[st], perSetting[st] );
      printf( "\n" );
      for( st = 0; st < (int)TA_AllCandleSettings; st++ )
      {
         /* BodyVeryLong is exempt BY MEASUREMENT, not by convenience: it is in
          * the public setter's domain and no shipped candlestick reads it. */
         if( st == (int)TA_BodyVeryLong ) continue;
         if( perSetting[st] == 0 )
         {
            printf( "Failed: the matrix never exercises %s -- no row changes any "
                    "output through it\n", SETTING_NAME[st] );
            inert++;
         }
      }
      if( inert )
         return TA_CDLSET_VACUOUS_NO_MOVE;
   }

   /* Non-vacuity. A row that never changes an output, or settings that never
    * reach the servers, would let this whole sweep pass while comparing every
    * language at the defaults -- the exact hole #216 was filed for. */
   if( moved == 0 || valueMoved == 0 || nonZeroPairs == 0 )
   {
      printf( "Failed: the candle settings matrix is vacuous "
              "(%d moved, %d by value, %d on a non-zero output)\n",
              moved, valueMoved, nonZeroPairs );
      return TA_CDLSET_VACUOUS_NO_MOVE;
   }
   if( server_verify_active() && server_verify_candle_syncs() == syncsBefore )
   {
      printf( "Failed: no candle setting was pushed to any language server\n" );
      return TA_CDLSET_VACUOUS_NO_SYNC;
   }

   return TA_TEST_PASS;
}

ErrorNumber test_candlestick( TA_History *history )
{
   unsigned int i;
   ErrorNumber retValue;

   /* DO_TEST resets compatibility between groups but not candle settings, and
    * earlier groups change them. Establish the state this group needs rather
    * than inherit it -- ta_test_legacy.c does the same. Without this the MC/DC
    * gates fail with messages blaming pattern logic for a threshold someone
    * else moved. */
   TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );

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

   /* Everything above runs at the DEFAULT candle settings, in C only. The
    * matrix is the other half: non-default thresholds, and the same thresholds
    * carried to every language server (#215/#216). */
   retValue = test_candle_settings_matrix( history );
   if( retValue != TA_TEST_PASS )
   {
      printf( "Failed: candle settings matrix (retValue=%d)\n", retValue );
      return retValue;
   }

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
      /* Clear the out-param before freeing: it was published to the caller at
       * allocation time and every call site frees it too. Unreachable today --
       * no legal candle setting makes TA_CallFunc fail -- but a double free is
       * a poor thing to leave armed behind a metadata change. */
      *paramHolderPtr = NULL;
      TA_ParamHolderFree( paramHolder );
      return TA_TSTCDL_CALLFUNC_FAIL;
   }

   /* Do the lookback function call. */
   retCode = TA_GetLookback( paramHolder, lookback );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_GetLookback() failed [%d]\n", retCode );
      *paramHolderPtr = NULL;
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
                           TA_TEST_UNST_NONE,
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
