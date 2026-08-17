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
#define PB_SOLE    4
static int pbEkind[PB_MAXEXP];
static int pbEcond[PB_MAXEXP];       /* condition id for FLIP/CONTROL/SOLE, else -1 */
static int pbEdisj[PB_MAXEXP];       /* disjunct id for SOLE, else -1 */
static int pbEarm[PB_MAXEXP];        /* arm id for an attributed FLIP, else -1 */
static int pbEconj[PB_MAXEXP];       /* conjunct id within that arm, else -1 */

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

/* ALTERNATIVES INSIDE ONE CONDITION. pb_conditions() counts TOP-LEVEL
 * conjuncts, so `A && (B || C || D)` is two conditions and B, C, D are
 * invisible to it. A flip cannot reach them either: falsifying the condition
 * means falsifying every alternative at once, and a control only needs one of
 * them back. So an entire alternative can be deleted from a pattern with the
 * tier green -- which is not hypothetical. CDLLONGLEGGEDDOJI shipped in unit 1
 * with `rb <= BodyDoji && (lsh > ShadowLong || ush > ShadowLong)`, and BOTH
 * arms turned out to be individually deletable: its scenarios made both shadows
 * long, so neither arm was ever the one carrying the decision.
 *
 * pb_signs cannot cover this. That axis exists for a disjunction whose arms are
 * OUTPUT CLASSES (`white && x` vs `black && y`, emitting +100 and -100), and it
 * works by requiring each class to be fired. These alternatives all emit the
 * same value; there is no output difference to key on.
 *
 * The axis that does reach them is SOLE-TRUE: a firing case in which exactly
 * one alternative holds. That is what shows the alternative to be independently
 * sufficient, and it is the disjunction's form of the control. */
#define PB_MAXALT 8
static int         pbNbDisj[PB_MAXCOND];   /* alternatives per condition, 0 = plain */
/* Conjuncts INSIDE each alternative. pb_signs and the sole-true axis both reach
 * arm SELECTION only -- which alternative carries the decision -- and say
 * nothing about what an alternative is made of. An arm of `colour && shadow`
 * has its colour half covered by the class axis and its shadow half covered by
 * nothing, and an arm of eight conjuncts has seven such halves. Declaring the
 * sizes is what lets the completeness check ask for a case per conjunct. */
static int         pbArmN[PB_MAXCOND][PB_MAXALT];
static int         pbNbArm[PB_MAXCOND];
static int         pbAwCond[PB_MAXWAIVE], pbAwArm[PB_MAXWAIVE], pbAwJ[PB_MAXWAIVE];
static const char *pbAwWhy[PB_MAXWAIVE];
static int         pbNaw;
static int         pbDwCond[PB_MAXWAIVE];
static int         pbDwK[PB_MAXWAIVE];
static const char *pbDwWhy[PB_MAXWAIVE];
static int         pbNdw;

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
   pbEdisj[pbNe]=-1; pbEarm[pbNe]=-1; pbEconj[pbNe]=-1;
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

/* pb_disjuncts(c,n)   condition `c` is an n-way disjunction. Each alternative
 *                     needs a sole-true case or a disjunct waiver; the count is
 *                     pinned against the pattern source by
 *                     scripts/check_mcdc_conditions.py, so omitting the
 *                     declaration fails the static check rather than quietly
 *                     leaving the alternatives uncovered.
 * pb_sole(i,v,c,k,s)  bars where EVERY condition holds and, within condition
 *                     `c`, alternative `k` is the ONLY one true. The pattern
 *                     must fire. Deleting alternative k then turns this case
 *                     off, which is exactly what nothing could see before.
 * pb_waive_disjunct(c,k,why)
 *                     alternative k cannot be isolated -- typically because a
 *                     sibling is implied by it. Same status as pb_waive: a
 *                     refutable claim, printed at run time.
 */
static void pb_disjuncts( int cond, int n )
{
   if( cond < 0 || cond >= PB_MAXCOND || n < 2 || n > PB_MAXALT )
      { pbOverflow = 1; return; }
   pbNbDisj[cond] = n;
}
/* pb_arm(c,k,n)        alternative k of condition c is a conjunction of n terms.
 * pb_flip_in(i,c,k,j,s) the decision does not fire, every other condition holds,
 *                      and within alternative k exactly term j is false -- so the
 *                      zero is attributable to that term and not merely to the
 *                      alternative. This is the flip an interior term can have;
 *                      a plain pb_flip of the condition falsifies every
 *                      alternative at once and names none of them.
 * pb_waive_arm(c,k,j,why)
 *                      term j cannot be broken alone. Two shapes recur: the
 *                      arm's own selector (a colour test, already covered on the
 *                      output-class axis) and a term entailed by its siblings. */
static void pb_arm( int cond, int arm, int n )
{
   if( cond < 0 || cond >= PB_MAXCOND || arm < 0 || arm >= PB_MAXALT || n < 1 )
      { pbOverflow = 1; return; }
   pbArmN[cond][arm] = n;
   if( arm + 1 > pbNbArm[cond] ) pbNbArm[cond] = arm + 1;
}
static void pb_flip_in( int i, int cond, int arm, int j, const char *s )
{
   pb_record(i, 0, s, PB_FLIP, cond);
   if( pbNe > 0 ) { pbEarm[pbNe-1] = arm; pbEconj[pbNe-1] = j; }
}
static void pb_waive_arm( int cond, int arm, int j, const char *why )
{
   int m;
   if( pbNaw >= PB_MAXWAIVE || !why ) { pbOverflow = 1; return; }
   for( m = 0; m < pbNaw; m++ )
      if( pbAwCond[m]==cond && pbAwArm[m]==arm && pbAwJ[m]==j ) { pbOverflow=1; return; }
   pbAwCond[pbNaw]=cond; pbAwArm[pbNaw]=arm; pbAwJ[pbNaw]=j; pbAwWhy[pbNaw]=why; pbNaw++;
}
static void pb_sole( int i, int v, int cond, int k, const char *s )
{
   pb_record(i, v, s, PB_SOLE, cond);
   if( pbNe > 0 ) pbEdisj[pbNe-1] = k;
}
static void pb_waive_disjunct( int cond, int k, const char *why )
{
   int m;
   if( pbNdw >= PB_MAXWAIVE || !why ) { pbOverflow = 1; return; }
   for( m = 0; m < pbNdw; m++ )
      if( pbDwCond[m] == cond && pbDwK[m] == k ) { pbOverflow = 1; return; }
   pbDwCond[pbNdw] = cond; pbDwK[pbNdw] = k; pbDwWhy[pbNdw] = why; pbNdw++;
}

/* The disjunct model, registered by the builder rather than passed to
 * pb_check_mcdc: the 34 builders that have no disjunction should not have to
 * mention one, and a new argument on the entry point would make every one of
 * them say "none". Cleared by pb_reset, set inside build_<x>(), read by
 * pb_check_mcdc_finish. */
/* The model reports each alternative's TERMS, not merely whether the
 * alternative holds: an arm's truth is the AND of its terms, so the finer model
 * yields the coarser one and there is only ever one function to write. */
typedef void (*PbArmFn)( int i, int cond, int arm, int *a );
static PbArmFn pbArmModel;
static void pb_arm_model( PbArmFn f ) { pbArmModel = f; }

/* Truth of alternative `arm`, or -1 if the model left a term unset. */
static int pb_arm_true( int i, int cond, int arm )
{
   int a[PB_MAXCOND], q, n = pbArmN[cond][arm], v = 1;
   if( n <= 0 || n > PB_MAXCOND ) return -1;
   for( q = 0; q < n; q++ ) a[q] = -1;
   pbArmModel(i, cond, arm, a);
   for( q = 0; q < n; q++ )
   {
      if( a[q] == -1 ) return -1;
      if( !a[q] ) v = 0;
   }
   return v;
}

static void pb_reset( void )
{
   int k;
   pbCur=0; pbNe=0; pbNw=0; pbNbCond=0; pbNbSigns=1; pbOverflow=0;
   pbNdw=0; pbNaw=0; pbArmModel=0;
   for( k=0; k<PB_MAXCOND; k++ )
   { int q; pbNbDisj[k] = 0; pbNbArm[k] = 0;
     for( q=0; q<PB_MAXALT; q++ ) pbArmN[k][q] = 0; }
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
/* Same shape, for the seven candlesticks that take optInPenetration
 * (CDLABANDONEDBABY, CDLDARKCLOUDCOVER, CDLEVENINGDOJISTAR, CDLEVENINGSTAR,
 * CDLMATHOLD, CDLMORNINGDOJISTAR, CDLMORNINGSTAR). PbCdlFn has no slot for
 * the extra real, so none of the seven can be passed to pb_check_mcdc at
 * all -- see pb_check_mcdc_p below. */
typedef TA_RetCode (*PbCdlFnP)(int,int,const double*,const double*,const double*,const double*,double,int*,int*,int*);

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
static double pb_abs( double v ) { return v < 0.0 ? -v : v; }
static double pb_bodyhi( int i ){ return pbC[i] >= pbO[i] ? pbC[i] : pbO[i]; }

typedef void (*PbCondFn)( int i, int *c );

/* Totals across every MC/DC function, printed once so the gate's own coverage
 * is visible rather than asserted in the dark. A waiver count that drifts is
 * the thing most likely to hollow this gate out silently. */
static int pbTotDetect, pbTotFlip, pbTotControl, pbTotWaive, pbTotCond;
static int pbTotSole, pbTotDisj, pbTotDisjCond, pbTotDisjWaive;
static int pbTotArmTerm, pbTotArmFlip, pbTotArmWaive;
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
   printf("  MC/DC disjuncts: %d alternative(s) inside %d condition(s), "
          "%d sole-true case(s), %d waived\n",
          pbTotDisj, pbTotDisjCond, pbTotSole, pbTotDisjWaive);
   printf("  MC/DC arm terms: %d term(s) inside those alternatives, "
          "%d attributed flip(s), %d waived\n",
          pbTotArmTerm, pbTotArmFlip, pbTotArmWaive);
}

/* The body shared by pb_check_mcdc and pb_check_mcdc_p -- everything past
 * "call the pattern function", which is the one thing that differs between a
 * parameterless candlestick and one of the seven that take optInPenetration.
 * `out` is read-only here: both callers own their own PB_N buffer. */
static ErrorNumber pb_check_mcdc_finish( const char *name, TA_RetCode rc,
                                          const int out[], int begIdx, int nb,
                                          PbCondFn conds )
{
   int k, j, fails=0;
   int nDetect=0, nFlip=0, nControl=0, nSole=0;

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
      case PB_SOLE:    nSole++;    break;
      default:                     break;
      }
      /* A detect, control or sole-true case that expects 0 asserts nothing: 0 is
       * what the pattern returns nearly everywhere. Only a firing expectation
       * carries information. */
      if( (pbEkind[k] == PB_DETECT || pbEkind[k] == PB_CONTROL ||
           pbEkind[k] == PB_SOLE) && pbEv[k] == 0 )
      {
         printf("  %s MC/DC VACUOUS: %s expects 0 (%s)\n", name,
                pbEkind[k] == PB_DETECT ? "detect" :
                pbEkind[k] == PB_SOLE   ? "sole-true" : "control", pbEl[k]);
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
         /* ANCHOR THE DECOMPOSITION. Everything else about a disjunct model
          * is self-consistent by construction: it is asked which alternatives
          * hold and its answer is checked only against the label on the case.
          * A model that reports exactly one alternative true at each sole-true
          * case therefore satisfies every check without describing the
          * condition at all -- `d[0] = high < 102, d[1] = high >= 102` passes
          * both of CDLLONGLEGGEDDOJI's sole cases and decomposes nothing.
          * Requiring the alternatives to OR back to the condition, at every
          * scenario rather than only the sole-true ones, is what makes them a
          * decomposition OF that condition: the flips are where OR is false,
          * and a fabricated model has to get those right too. */
         if( pbArmModel )
         {
            int c2;
            for( c2 = 0; c2 < pbNbCond && c2 < PB_MAXCOND; c2++ )
            {
               int any = 0, q, n2 = pbNbArm[c2];
               if( n2 <= 0 ) continue;
               for( q = 0; q < n2; q++ ) if( pb_arm_true(pbEi[k], c2, q) == 1 ) any = 1;
               if( cv[c2] != -1 && any != (cv[c2] ? 1 : 0) )
               {
                  printf("  %s MC/DC: c%d's alternatives OR to %d but the "
                         "condition model says %d -- the alternatives are not a "
                         "decomposition of that condition (%s)\n",
                         name, c2, any, cv[c2] ? 1 : 0, pbEl[k]);
                  fails++;
               }
            }
         }
         if( pbEkind[k] == PB_DETECT || pbEkind[k] == PB_CONTROL ||
             pbEkind[k] == PB_SOLE )
         {
            if( nFalse != 0 )
            {
               printf("  %s MC/DC: %s has %d condition(s) false, first c%d (%s)\n",
                      name, pbEkind[k]==PB_DETECT ? "detect" :
                            pbEkind[k]==PB_SOLE   ? "sole-true" : "control",
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

   /* DISJUNCT COVERAGE. Everything above this point treats a condition as
    * atomic, so an alternative inside one is reachable by nothing: the flip
    * falsifies the whole disjunction at once and the control only restores some
    * arm of it. This is the axis that reaches them -- a firing case in which
    * exactly one alternative holds, which is what makes deleting that
    * alternative turn the case off. */
   {
      int c;
      for( c = 0; c < pbNbCond && c < PB_MAXCOND; c++ )
      {
         if( pbNbDisj[c] <= 0 ) continue;
         if( !pbArmModel )
         {
            printf("  %s MC/DC: c%d declares %d alternative(s) but no arm "
                   "model was registered -- they are unchecked\n",
                   name, c, pbNbArm[c]);
            fails++;
            continue;
         }
         for( j = 0; j < pbNbDisj[c]; j++ )
         {
            int found = 0, w;
            for( k = 0; k < pbNe; k++ )
               if( pbEkind[k] == PB_SOLE && pbEcond[k] == c && pbEdisj[k] == j )
                  { found = 1; break; }
            for( w = 0; w < pbNdw && !found; w++ )
               if( pbDwCond[w] == c && pbDwK[w] == j )
               {
                  found = 1;
                  printf("  %s MC/DC disjunct waiver c%d alt%d: %s\n",
                         name, c, j, pbDwWhy[w]);
               }
            if( !found )
            {
               printf("  %s MC/DC: c%d alternative %d has no sole-true case and "
                      "no waiver -- deleting it from the pattern would change "
                      "nothing here\n", name, c, j);
               fails++;
            }
         }
      }
   }

   /* INTERIOR TERM COVERAGE. Everything above reaches arm SELECTION -- which
    * alternative carries the decision -- and nothing inside one. A plain flip of
    * the condition falsifies every alternative at once and names none of their
    * terms, so an arm of eight conjuncts is satisfied by breaking any one of
    * them and the other seven are asked for by nothing. That is not theoretical:
    * CDLBELTHOLD's arms hold a colour test and a shadow test, and its shadow
    * halves are covered only because unit 1 chose to write a second flip per
    * arm; the gate would have printed the same "ok" without them. */
   {
      int c, a, j, w;
      for( c = 0; c < pbNbCond && c < PB_MAXCOND; c++ )
      {
         if( pbNbArm[c] <= 0 ) continue;
         for( a = 0; a < pbNbArm[c] && a < PB_MAXALT; a++ )
         {
            if( pbArmN[c][a] <= 0 )
            {
               printf("  %s MC/DC: c%d alt%d declares no term count -- its "
                      "interior is unchecked\n", name, c, a);
               fails++;
               continue;
            }
            /* An arm of ONE term needs no attribution: the term and the arm
             * are the same proposition, so the sole-true case (that arm alone
             * true) and the condition's own flip (every arm false) already show
             * the term determining the condition in both directions. Only from
             * two terms up does "the arm is false" stop naming which one. */
            if( pbArmN[c][a] == 1 ) continue;
            for( j = 0; j < pbArmN[c][a]; j++ )
            {
               int found = 0;
               for( k = 0; k < pbNe; k++ )
                  if( pbEkind[k] == PB_FLIP && pbEcond[k] == c &&
                      pbEarm[k] == a && pbEconj[k] == j ) { found = 1; break; }
               for( w = 0; w < pbNaw && !found; w++ )
                  if( pbAwCond[w]==c && pbAwArm[w]==a && pbAwJ[w]==j )
                  {
                     found = 1;
                     printf("  %s MC/DC arm waiver c%d alt%d term%d: %s\n",
                            name, c, a, j, pbAwWhy[w]);
                  }
               if( !found )
               {
                  printf("  %s MC/DC: c%d alt%d term %d has no attributed flip "
                         "and no waiver -- breaking it alone is asked for by "
                         "nothing\n", name, c, a, j);
                  fails++;
               }
            }
         }
      }
   }

   /* An attributed flip must be attributable: exactly the named term false
    * inside the named arm, every other term of that arm true, and every other
    * alternative false -- otherwise the zero it asserts belongs to something
    * else and the case proves nothing about the term on its label. */
   if( pbArmModel )
   {
      for( k = 0; k < pbNe; k++ )
      {
         int a[PB_MAXCOND], q, n, nFalse = 0, firstFalse = -1, other;
         if( pbEkind[k] != PB_FLIP || pbEarm[k] < 0 ) continue;
         n = pbArmN[pbEcond[k]][pbEarm[k]];
         if( n <= 0 || n > PB_MAXCOND ) continue;
         for( q = 0; q < n; q++ ) a[q] = -1;
         pbArmModel(pbEi[k], pbEcond[k], pbEarm[k], a);
         for( q = 0; q < n; q++ )
         {
            if( a[q] == -1 )
            {
               printf("  %s MC/DC: arm model left c%d alt%d term%d unset (%s)\n",
                      name, pbEcond[k], pbEarm[k], q, pbEl[k]);
               fails++;
            }
            else if( !a[q] ) { nFalse++; if( firstFalse < 0 ) firstFalse = q; }
         }
         if( nFalse != 1 || firstFalse != pbEconj[k] )
         {
            printf("  %s MC/DC: flip filed as c%d alt%d term%d has %d term(s) "
                   "false%s (%s)\n", name, pbEcond[k], pbEarm[k], pbEconj[k],
                   nFalse, nFalse == 1 ? " -- and it is a different term" : "",
                   pbEl[k]);
            fails++;
         }
         for( other = 0; other < pbNbArm[pbEcond[k]] && other < PB_MAXALT; other++ )
         {
            if( other == pbEarm[k] ) continue;
            if( pb_arm_true(pbEi[k], pbEcond[k], other) == 1 )
            {
               printf("  %s MC/DC: flip filed as c%d alt%d term%d leaves alt%d "
                      "true, so the condition still holds (%s)\n",
                      name, pbEcond[k], pbEarm[k], pbEconj[k], other, pbEl[k]);
               fails++;
            }
         }
      }
   }

   /* Each sole-true case must actually be sole-true. Without this the kind is
    * just a second control: a case tagged alt0 that happens to satisfy alt0 AND
    * alt2 proves neither of them independently sufficient. */
   if( pbArmModel )
   {
      for( k = 0; k < pbNe; k++ )
      {
         int n, m, nTrue = 0, firstTrue = -1;
         if( pbEkind[k] != PB_SOLE ) continue;
         n = (pbEcond[k] >= 0 && pbEcond[k] < PB_MAXCOND) ? pbNbDisj[pbEcond[k]] : 0;
         if( n <= 0 )
         {
            printf("  %s MC/DC: sole-true case names c%d, which declares no "
                   "alternatives (%s)\n", name, pbEcond[k], pbEl[k]);
            fails++;
            continue;
         }
         for( m = 0; m < n; m++ )
         {
            int t = pb_arm_true(pbEi[k], pbEcond[k], m);
            if( t == -1 )
            {
               printf("  %s MC/DC: arm model left a term of c%d alt%d unset (%s)\n",
                      name, pbEcond[k], m, pbEl[k]);
               fails++;
            }
            else if( t ) { nTrue++; if( firstTrue < 0 ) firstTrue = m; }
         }
         if( nTrue != 1 )
         {
            printf("  %s MC/DC: case filed as c%d alt%d has %d alternative(s) "
                   "true%s (%s)\n", name, pbEcond[k], pbEdisj[k], nTrue,
                   nTrue > 1 ? " -- not a sole-true case" : " -- the condition cannot hold",
                   pbEl[k]);
            fails++;
         }
         else if( firstTrue != pbEdisj[k] )
         {
            printf("  %s MC/DC: case filed as c%d alt%d is actually alt%d (%s)\n",
                   name, pbEcond[k], pbEdisj[k], firstTrue, pbEl[k]);
            fails++;
         }
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
   pbTotSole   += nSole;
   pbTotDisjWaive += pbNdw;
   pbTotArmWaive  += pbNaw;
   for( k=0; k<pbNe; k++ ) if( pbEkind[k]==PB_FLIP && pbEarm[k]>=0 ) pbTotArmFlip++;
   { int c2,a2; for( c2=0; c2<pbNbCond && c2<PB_MAXCOND; c2++ )
        for( a2=0; a2<pbNbArm[c2] && a2<PB_MAXALT; a2++ ) pbTotArmTerm += pbArmN[c2][a2]; }
   { int c; for( c=0; c<pbNbCond && c<PB_MAXCOND; c++ )
        if( pbNbDisj[c] > 0 ) { pbTotDisj += pbNbDisj[c]; pbTotDisjCond++; } }
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

static ErrorNumber pb_check_mcdc( const char *name, PbCdlFn fn, PbCondFn conds )
{
   int out[PB_N], begIdx=0, nb=0;
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
   return pb_check_mcdc_finish( name, rc, out, begIdx, nb, conds );
}

/* Same contract, for the seven optInPenetration candlesticks (see PbCdlFnP).
 * `penetration` is the CALLER's job, not a shared constant here: the seven do
 * NOT share one default -- CDLABANDONEDBABY, CDLEVENINGDOJISTAR,
 * CDLEVENINGSTAR, CDLMORNINGDOJISTAR and CDLMORNINGSTAR default to 0.3
 * (TA_DEF_UI_Penetration_30), CDLDARKCLOUDCOVER and CDLMATHOLD to 0.5
 * (TA_DEF_UI_Penetration_50) -- see each ta_codegen/input/<name>/<name>.yaml.
 * A single hardcoded 0.5 would silently test five of the seven off their
 * documented default. Pass the pattern's own default so a boundary flip
 * works the same way as on every parameterless pattern; probing the
 * parameter itself is out of scope here. */
static ErrorNumber pb_check_mcdc_p( const char *name, PbCdlFnP fn,
                                     double penetration, PbCondFn conds )
{
   int out[PB_N], begIdx=0, nb=0;
   TA_RetCode rc;

   if( pbOverflow )
   {
      printf("  %s MC/DC: builder overflowed a harness buffer "
             "(bars=%d/%d expectations=%d/%d waivers=%d/%d)\n",
             name, pbCur, PB_N, pbNe, PB_MAXEXP, pbNw, PB_MAXWAIVE);
      return TA_TSTCDL_PREDICATE_VACUOUS;
   }

   rc = fn(0, pbCur-1, pbO, pbH, pbL, pbC, penetration, &begIdx, &nb, out);
   return pb_check_mcdc_finish( name, rc, out, begIdx, nb, conds );
}

/* The CONFIRMATION half of the two Hikkakes: the +/-200 emitted up to three
 * bars after a detection, off a countdown and a cached high/low. It is a state
 * machine rather than a decision over one bar's window, so the MC/DC model --
 * conditions evaluated at a single index -- cannot express it, and these
 * scenarios are its gate: the countdown edges, both confirmation directions and
 * the mis-cache candidates. The DETECTION half is an ordinary decision and is
 * covered by build_hikkake / build_hikkakemod instead. */
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
   /* ON the confirmation boundary. The two cases above straddle it by 2 and 5
    * points, which says nothing about whether the compare is strict: relaxing
    * both `>` and `<` to inclusive passed this entire suite. A confirmation
    * needs a close STRICTLY past the 2nd candle's edge, so a close sitting
    * exactly on it must not confirm. */
   d = pb_hk_win(+1,0,0,0,0); pb_expect(d,100,"bull detect");
   c = pb_close(115.0);       pb_expect(c,0,"bull close == savedHigh 115 -> no confirm, the test is strict"); pb_flat(6);
   d = pb_hk_win(-1,0,0,0,0); pb_expect(d,-100,"bear detect");
   c = pb_close(85.0);        pb_expect(c,0,"bear close == savedLow 85 -> no confirm, the test is strict");   pb_flat(6);
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
   /* ON the confirmation boundary -- see the note on the CDLHIKKAKE pair. */
   d = pb_mod_win(+1,0); pb_expect(d,100,"mod bull detect");
   c = pb_close(115.0);  pb_expect(c,0,"mod close == patternHigh 115 -> no confirm, the test is strict"); pb_flat(8);
   d = pb_mod_win(-1,0); pb_expect(d,-100,"mod bear detect");
   c = pb_close(85.0);   pb_expect(c,0,"mod close == patternLow 85 -> no confirm, the test is strict");   pb_flat(8);
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
/* Each arm is a colour selector AND a shadow test. pb_signs reaches the
 * selectors; until these were declared, nothing reached the shadows. */
static void arm_belthold( int i, int cond, int arm, int *a )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   if( cond != 1 ) return;
   if( arm == 0 ) { a[0] =  pb_white(i); a[1] = pb_losh(i) < vs; }
   else           { a[0] = !pb_white(i); a[1] = pb_upsh(i) < vs; }
}

static void build_belthold( void )
{
  pb_conditions(2);
  pb_signs(2);
  pb_arm(1,0,2); pb_arm(1,1,2);
  pb_arm_model(arm_belthold);
  pb_waive_arm(1,0,0,"the arm's own colour selector -- it is what chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(1,1,0,"the arm's own colour selector -- it is what chooses the arm, and the class it chooses is fired by pb_signs(2)");

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
  pb_flip_in(f1,1,0,1,"break c1 alt0 term1: lower shadow 1 == avg 1, the test is strict");
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
  pb_flip_in(f1b,1,1,1,"break c1 alt1 term1: upper shadow 1 == avg 1, the test is strict");
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
static void arm_closingmarubozu( int i, int cond, int arm, int *a )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   if( cond != 1 ) return;
   if( arm == 0 ) { a[0] =  pb_white(i); a[1] = pb_upsh(i) < vs; }
   else           { a[0] = !pb_white(i); a[1] = pb_losh(i) < vs; }
}

static void build_closingmarubozu( void )
{
  pb_conditions(2);
  pb_signs(2);
  pb_arm(1,0,2); pb_arm(1,1,2);
  pb_arm_model(arm_closingmarubozu);
  pb_waive_arm(1,0,0,"the arm's own colour selector -- it is what chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(1,1,0,"the arm's own colour selector -- it is what chooses the arm, and the class it chooses is fired by pb_signs(2)");

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
  pb_flip_in(f1,1,0,1,"break c1 alt0 term1: upper shadow 1 == avg 1, the test is strict");
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
  pb_flip_in(f1b,1,1,1,"break c1 alt1 term1: lower shadow 1 == avg 1, the test is strict");
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
/* c1 is a two-way disjunction, and every scenario below the detect used to make
 * BOTH shadows long -- so neither arm was ever the one carrying the decision and
 * either could be deleted from the pattern with the whole suite green. The two
 * sole-true cases are what close that. */
static void arm_longleggeddoji( int i, int cond, int arm, int *a )
{
   double sl = pb_avg(TA_ShadowLong, i);
   if( cond == 1 ) a[0] = arm == 0 ? pb_losh(i) > sl : pb_upsh(i) > sl;
}

static void build_longleggeddoji( void )
{
  pb_conditions(2);
  pb_disjuncts(1,2);
  pb_arm(1,0,1); pb_arm(1,1,1);
  pb_arm_model(arm_longleggeddoji);

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

  pb_primer(12,100,2,4);
  int s0=pb_bar(100,100,96,100);           /* upper shadow 0: only the lower arm holds */
  pb_sole(s0,100,1,0,"c1 alt0 alone: lower shadow 4 > 0, upper shadow 0 is not");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int s1=pb_bar(100,104,100,100);          /* lower shadow 0: only the upper arm holds */
  pb_sole(s1,100,1,1,"c1 alt1 alone: upper shadow 4 > 0, lower shadow 0 is not");
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
 * No waivers: single-bar patterns have no cross-bar entailment, so all 23
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
 *   c3  bodylo(i) <= low + hlrange/2 + avg(Near)      the band's UPPER edge
 *   c4  bodyhi(i) >= low + hlrange/2 - avg(Near)      the band's LOWER edge
 *
 * The source writes c3 and c4 as one parenthesised group -- "body near the
 * midpoint" -- and that grouping is NOT the BELTHOLD shape it resembles.
 * BELTHOLD's parens hold a DISJUNCTION, where a disjunct cannot be falsified
 * while its sibling holds; these hold a plain conjunction, so `A && (B && C)`
 * is `A && B && C` and the parens mean nothing. Counting them as one condition
 * cost a real boundary: with only the upper edge pinned, relaxing the lower one
 * from >= to > left the entire tier green -- 89 conditions, 85 flips, every one
 * passing -- because no flip and no control ever sat on it. check-mcdc now
 * flattens a pure-conjunction group, so this is five conditions and a builder
 * cannot under-declare the shape again.
 *
 * Both edges are INCLUSIVE, and an inclusive boundary cannot be pinned by a
 * flip: there is no minimal value above a `<=` threshold, so the flip is always
 * a whole unit clear and says nothing about the comparison. The control does it
 * instead, sitting exactly on the equality -- k3 puts bodylo ON the band's top,
 * k4 puts bodyhi ON its bottom. Both keep bodylo != bodyhi so that reading one
 * for the other is visible; a doji makes them the same number and hides it.
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
   c[3] = pb_bodylo(i) <= mid + near;
   c[4] = pb_bodyhi(i) >= mid - near;
}

static void build_rickshawman( void )
{
  pb_conditions(5);

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

  /* c3 and c4: the body must sit inside a band of +/-Near around the bar's
   * midpoint. Every bar below has high 110 and low 96, so the midpoint is 103
   * and Near is 2: the band is [101,105]. c3 tests its TOP against
   * min(open,close), c4 its BOTTOM against max(open,close).
   *
   * All four bars carry a real body of 1 rather than the doji used above, and
   * that is load-bearing: with open == close the two terms are the SAME NUMBER
   * and any confusion between them is invisible. A mutation swapping the min
   * for a max sailed through an earlier version of these cases for exactly that
   * reason -- it was the v0.6.4 freeze that caught it, and the freeze reaches
   * only 35 of the 61 patterns.
   *
   * Both edges are inclusive, so the CONTROL is what pins each: k3 puts
   * min(open,close) exactly ON 105 with max at 106 above it, k4 puts
   * max(open,close) exactly ON 101 with min at 100 below it. In both the two
   * terms disagree, so reading the wrong one stops the control firing.
   */
  pb_primer(12,100,2,4);
  int f3=pb_bar(106,110,96,107);           /* min 106 is above the band top 105 */
  pb_flip(f3,3,"break c3: min(open,close) 106 > band top 105");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k3=pb_bar(105,110,96,106);           /* min 105 == band top, max 106 above it */
  pb_control(k3,100,3,"restore c3: min 105 == band top, inclusive; max 106 differs");
  pb_flat(8);

  pb_primer(12,100,2,4);
  int f4=pb_bar(99,110,96,100);            /* max 100 is below the band bottom 101 */
  pb_flip(f4,4,"break c4: max(open,close) 100 < band bottom 101");
  pb_flat(8);
  pb_primer(12,100,2,4);
  int k4=pb_bar(100,110,96,101);           /* max 101 == band bottom, min 100 below it */
  pb_control(k4,100,4,"restore c4: max 101 == band bottom, inclusive; min 100 differs");
  pb_flat(8);
}

/* ---- Moderate tier, unit 3: the first six two-bar patterns --------------- *
 *
 * These are the first builders whose decision reads a bar OTHER than the one
 * under test, and that changes how the scenarios have to be built:
 *
 *   The prior bar sits inside the current bar's averaging window. A 10-bar
 *   window ending at i covers bars i-10..i-1, so bar i-1 is one of the ten
 *   that set avg(BodyShort), avg(BodyDoji) and the rest AT i. Give it any
 *   geometry that differs from the primer's and every threshold at i moves off
 *   its exact value.
 *
 * Every prior bar below therefore keeps the primer's RealBody 2 and HighLow
 * 10, even where the pattern wants it to look different -- DOJISTAR needs a
 * body of 5 on the prior bar and gets it as (100,106,96,105), which is a body
 * of 5 inside a HighLow of exactly 10. The thresholds at i stay 2.0 and 1.0
 * and the flips stay on their boundaries.
 *
 * One threshold is new here and is exact for the same reason as the others:
 *
 *   avg(Equal) = 0.05 * (50/5) / 1 = 0.5     avgPeriod 5, HighLow-typed
 *
 * HAMMER and HANGINGMAN read avg(Near) at i-1 rather than at i, so their band
 * is set by the five bars before the PRIOR bar. Under this primer that is
 * still 2.0, but the model computes it at i-1 as the library does rather than
 * assuming the two agree.
 *
 * The pairs are built together on purpose, as in units 1 and 2: HAMMER and
 * HANGINGMAN differ only in whether the body sits near the prior bar's low or
 * its high, INVERTEDHAMMER and SHOOTINGSTAR only in the direction of the gap.
 * A model copy-pasted between either pair stays self-consistent and disagrees
 * with the library.
 */

/* CDLHAMMER -- a small body with a long lower shadow, near the prior low.
 *
 *   c0  realbody(i)    <  avg(BodyShort, i)
 *   c1  lowershadow(i) >  avg(ShadowLong, i)        = realbody(i)
 *   c2  uppershadow(i) <  avg(ShadowVeryShort, i)
 *   c3  min(open,close)(i) <= low(i-1) + avg(Near, i-1)
 *
 * c1's threshold is the current body, so the c0 flip -- which raises the body
 * to the BodyShort boundary -- has to lengthen the lower shadow with it.
 */
static void cond_hammer( int i, int *c )
{
   c[0] = pb_body(i) <  pb_avg(TA_BodyShort, i);
   c[1] = pb_losh(i) >  pb_avg(TA_ShadowLong, i);
   c[2] = pb_upsh(i) <  pb_avg(TA_ShadowVeryShort, i);
   c[3] = pb_bodylo(i) <= pbL[i-1] + pb_avg(TA_Near, i-1);
}

static void build_hammer( void )
{
  pb_conditions(4);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);                  /* prior: primer geometry, low 96 -> band top 98 */
  int d=pb_bar(98,98.5,94,97.5);           /* body 0.5, lower 3.5, upper 0.5, min 97.5 */
  pb_detect(d,100,"detect: body 0.5 < 2, lower 3.5 > 0.5, upper 0.5 < 1, min 97.5 <= 98");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f0=pb_bar(98,98.5,93,96);            /* body 2 == avg; lower 3 still clears it */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k0=pb_bar(98,98.5,94,97.5);
  pb_control(k0,100,0,"restore c0: body 0.5 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f1=pb_bar(98,98.5,97,97.5);          /* lower shadow 0.5 == body */
  pb_flip(f1,1,"break c1: lower shadow 0.5 == body 0.5, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k1=pb_bar(98,98.5,94,97.5);
  pb_control(k1,100,1,"restore c1: lower shadow 3.5 > 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f2=pb_bar(98,99,94,97.5);            /* upper shadow 1 == avg */
  pb_flip(f2,2,"break c2: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k2=pb_bar(98,98.5,94,97.5);
  pb_control(k2,100,2,"restore c2: upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f3=pb_bar(98.5,99,94,98.6);          /* min 98.5 is above the band top 98 */
  pb_flip(f3,3,"break c3: min 98.5 > band top 98");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k3=pb_bar(98,98.5,94,98.2);          /* min exactly 98 */
  pb_control(k3,100,3,"restore c3: min 98 == band top, inclusive");
  pb_flat(8);
}

/* CDLHANGINGMAN -- HAMMER's mirror: the same candle near the prior HIGH.
 *
 *   c0..c2  identical to HAMMER
 *   c3      min(open,close)(i) >= high(i-1) - avg(Near, i-1)
 *
 * Only c3 differs, and it differs in both the bar-i-1 field it reads and the
 * direction of the comparison. Its flip and control are what separate this
 * builder from HAMMER's.
 */
static void cond_hangingman( int i, int *c )
{
   c[0] = pb_body(i) <  pb_avg(TA_BodyShort, i);
   c[1] = pb_losh(i) >  pb_avg(TA_ShadowLong, i);
   c[2] = pb_upsh(i) <  pb_avg(TA_ShadowVeryShort, i);
   c[3] = pb_bodylo(i) >= pbH[i-1] - pb_avg(TA_Near, i-1);
}

static void build_hangingman( void )
{
  pb_conditions(4);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);                  /* prior: high 106 -> band bottom 104 */
  int d=pb_bar(105,105.5,101,104.5);       /* body 0.5, lower 3.5, upper 0.5, min 104.5 */
  pb_detect(d,-100,"detect: body 0.5 < 2, lower 3.5 > 0.5, upper 0.5 < 1, min 104.5 >= 104");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f0=pb_bar(106,106.5,101,104);        /* body 2 == avg; min 104 still on the band */
  pb_flip(f0,0,"break c0: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k0=pb_bar(105,105.5,101,104.5);
  pb_control(k0,-100,0,"restore c0: body 0.5 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f1=pb_bar(105,105.5,104,104.5);      /* lower shadow 0.5 == body */
  pb_flip(f1,1,"break c1: lower shadow 0.5 == body 0.5, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k1=pb_bar(105,105.5,101,104.5);
  pb_control(k1,-100,1,"restore c1: lower shadow 3.5 > 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f2=pb_bar(105,106,101,104.5);        /* upper shadow 1 == avg */
  pb_flip(f2,2,"break c2: upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k2=pb_bar(105,105.5,101,104.5);
  pb_control(k2,-100,2,"restore c2: upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f3=pb_bar(104,104.5,100,103.5);      /* min 103.5 is below the band bottom 104 */
  pb_flip(f3,3,"break c3: min 103.5 < band bottom 104");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k3=pb_bar(104.5,105,101,104);        /* min exactly 104 */
  pb_control(k3,-100,3,"restore c3: min 104 == band bottom, inclusive");
  pb_flat(8);
}

/* CDLINVERTEDHAMMER -- a small body with a long UPPER shadow, gapping down.
 *
 *   c0  realbodygapdown(i, i-1)   max(open,close)(i) < min(open,close)(i-1)
 *   c1  realbody(i)    <  avg(BodyShort, i)
 *   c2  uppershadow(i) >  avg(ShadowLong, i)        = realbody(i)
 *   c3  lowershadow(i) <  avg(ShadowVeryShort, i)
 *
 * The gap is the first condition in this unit that is a relation BETWEEN two
 * bars rather than a bar against a threshold, so its flip moves the current
 * body up to touch the prior body's floor rather than changing any average.
 */
static void cond_invertedhammer( int i, int *c )
{
   c[0] = pb_bodyhi(i) < pb_bodylo(i-1);
   c[1] = pb_body(i) <  pb_avg(TA_BodyShort, i);
   c[2] = pb_upsh(i) >  pb_avg(TA_ShadowLong, i);
   c[3] = pb_losh(i) <  pb_avg(TA_ShadowVeryShort, i);
}

static void build_invertedhammer( void )
{
  pb_conditions(4);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);                  /* prior: body floor 100 */
  int d=pb_bar(98,102,97.5,98.5);          /* body 0.5, upper 3.5, lower 0.5, top 98.5 */
  pb_detect(d,100,"detect: gap down 98.5 < 100, body 0.5 < 2, upper 3.5 > 0.5, lower 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f0=pb_bar(99.5,103,99,100);          /* body top 100 == prior floor */
  pb_flip(f0,0,"break c0: body top 100 == prior floor 100, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k0=pb_bar(98,102,97.5,98.5);
  pb_control(k0,100,0,"restore c0: body top 98.5 < 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f1=pb_bar(97,103,96.5,99);           /* body 2 == avg; upper 4 still clears it */
  pb_flip(f1,1,"break c1: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k1=pb_bar(98,102,97.5,98.5);
  pb_control(k1,100,1,"restore c1: body 0.5 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f2=pb_bar(98,99,97.5,98.5);          /* upper shadow 0.5 == body */
  pb_flip(f2,2,"break c2: upper shadow 0.5 == body 0.5, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k2=pb_bar(98,102,97.5,98.5);
  pb_control(k2,100,2,"restore c2: upper shadow 3.5 > 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f3=pb_bar(98,102,97,98.5);           /* lower shadow 1 == avg */
  pb_flip(f3,3,"break c3: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k3=pb_bar(98,102,97.5,98.5);
  pb_control(k3,100,3,"restore c3: lower shadow 0.5 < 1");
  pb_flat(8);
}

/* CDLSHOOTINGSTAR -- INVERTEDHAMMER's mirror: the same candle gapping UP.
 *
 *   c0  realbodygapup(i, i-1)     min(open,close)(i) > max(open,close)(i-1)
 *   c1..c3  identical to INVERTEDHAMMER
 *
 * Only c0 differs, and only in direction. Its flip sits on the prior body's
 * ceiling where INVERTEDHAMMER's sits on the prior body's floor.
 */
static void cond_shootingstar( int i, int *c )
{
   c[0] = pb_bodylo(i) > pb_bodyhi(i-1);
   c[1] = pb_body(i) <  pb_avg(TA_BodyShort, i);
   c[2] = pb_upsh(i) >  pb_avg(TA_ShadowLong, i);
   c[3] = pb_losh(i) <  pb_avg(TA_ShadowVeryShort, i);
}

static void build_shootingstar( void )
{
  pb_conditions(4);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);                  /* prior: body ceiling 102 */
  int d=pb_bar(103,107,102.5,103.5);       /* body 0.5, upper 3.5, lower 0.5, floor 103 */
  pb_detect(d,-100,"detect: gap up 103 > 102, body 0.5 < 2, upper 3.5 > 0.5, lower 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f0=pb_bar(102,106,101.5,102.5);      /* body floor 102 == prior ceiling */
  pb_flip(f0,0,"break c0: body floor 102 == prior ceiling 102, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k0=pb_bar(103,107,102.5,103.5);
  pb_control(k0,-100,0,"restore c0: body floor 103 > 102");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f1=pb_bar(103,108,102.5,105);        /* body 2 == avg; upper 3 still clears it */
  pb_flip(f1,1,"break c1: body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k1=pb_bar(103,107,102.5,103.5);
  pb_control(k1,-100,1,"restore c1: body 0.5 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f2=pb_bar(103,104,102.5,103.5);      /* upper shadow 0.5 == body */
  pb_flip(f2,2,"break c2: upper shadow 0.5 == body 0.5, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k2=pb_bar(103,107,102.5,103.5);
  pb_control(k2,-100,2,"restore c2: upper shadow 3.5 > 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int f3=pb_bar(103,107,102,103.5);        /* lower shadow 1 == avg */
  pb_flip(f3,3,"break c3: lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);
  int k3=pb_bar(103,107,102.5,103.5);
  pb_control(k3,-100,3,"restore c3: lower shadow 0.5 < 1");
  pb_flat(8);
}

/* CDLMATCHINGLOW -- two black candles closing at the same level.
 *
 *   c0  color(i-1) == -1
 *   c1  color(i)   == -1
 *   c2  close(i) <= close(i-1) + avg(Equal, i-1)
 *   c3  close(i) >= close(i-1) - avg(Equal, i-1)
 *
 * c2 and c3 are the two edges of one band, and the counter now reads them as
 * two conditions rather than one -- which is what makes each edge flippable on
 * its own. Both are inclusive, so each is pinned by a CONTROL sitting exactly
 * on it: an inclusive boundary has no minimal violating value, so a flip a
 * whole step away says nothing about whether the comparison is `<=` or `<`.
 */
static void cond_matchinglow( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-1);
   c[0] = !pb_white(i-1);
   c[1] = !pb_white(i);
   c[2] = pbC[i] <= pbC[i-1] + eq;
   c[3] = pbC[i] >= pbC[i-1] - eq;
}

static void build_matchinglow( void )
{
  pb_conditions(4);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior: black, close 100, band [99.5,100.5] */
  int d=pb_bar(102,106,96,100);            /* black, close 100 */
  pb_detect(d,100,"detect: both black, close 100 inside [99.5,100.5]");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);                  /* prior WHITE, close 102 -> band [101.5,102.5] */
  int f0=pb_bar(104,106,96,102);           /* black, close 102 keeps c2 and c3 true */
  pb_flip(f0,0,"break c0: the prior candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int k0=pb_bar(102,106,96,100);
  pb_control(k0,100,0,"restore c0: the prior candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int f1=pb_bar(98,106,96,100);            /* WHITE, close still 100 */
  pb_flip(f1,1,"break c1: this candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int k1=pb_bar(102,106,96,100);
  pb_control(k1,100,1,"restore c1: this candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int f2=pb_bar(103,106,96,100.6);         /* close 100.6 above the band top 100.5 */
  pb_flip(f2,2,"break c2: close 100.6 > band top 100.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int k2=pb_bar(103,106,96,100.5);         /* close exactly on the band top */
  pb_control(k2,100,2,"restore c2: close 100.5 == band top, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int f3=pb_bar(103,106,96,99.4);          /* close 99.4 below the band bottom 99.5 */
  pb_flip(f3,3,"break c3: close 99.4 < band bottom 99.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);
  int k3=pb_bar(103,106,96,99.5);          /* close exactly on the band bottom */
  pb_control(k3,100,3,"restore c3: close 99.5 == band bottom, inclusive");
  pb_flat(8);
}

/* CDLDOJISTAR -- a long body followed by a doji that gaps away from it.
 *
 *   c0  realbody(i-1) >  avg(BodyLong, i-1)
 *   c1  realbody(i)   <= avg(BodyDoji, i)
 *   c2  ( white(i-1) && realbodygapup(i, i-1) )
 *       || ( black(i-1) && realbodygapdown(i, i-1) )
 *
 * Bi-signed, and the sign comes from the PRIOR bar rather than this one:
 * -color(i-1)*100, so a white first candle gives -100. c2 is a disjunction
 * selected by that same colour, which makes the two output classes and the two
 * disjuncts the same axis -- exactly the shape pb_signs() exists for. The
 * black-first scenario at the end is what reaches the second disjunct.
 *
 * The prior bar carries a body of 5 inside a HighLow of exactly 10, so it
 * leaves the averages at i untouched while still clearing BodyLong.
 */
static void cond_dojistar( int i, int *c )
{
   c[0] = pb_body(i-1) >  pb_avg(TA_BodyLong, i-1);
   c[1] = pb_body(i)   <= pb_avg(TA_BodyDoji, i);
   c[2] = (  pb_white(i-1) && pb_bodylo(i) > pb_bodyhi(i-1) )
       || ( !pb_white(i-1) && pb_bodyhi(i) < pb_bodylo(i-1) );
}
static void arm_dojistar( int i, int cond, int arm, int *a )
{
   if( cond != 2 ) return;
   if( arm == 0 ) { a[0] =  pb_white(i-1); a[1] = pb_bodylo(i) > pb_bodyhi(i-1); }
   else           { a[0] = !pb_white(i-1); a[1] = pb_bodyhi(i) < pb_bodylo(i-1); }
}

static void build_dojistar( void )
{
  pb_conditions(3);
  pb_signs(2);
  pb_arm(2,0,2); pb_arm(2,1,2);
  pb_arm_model(arm_dojistar);
  pb_waive_arm(2,0,0,"the arm's own colour selector -- it is what chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(2,1,0,"the arm's own colour selector -- it is what chooses the arm, and the class it chooses is fired by pb_signs(2)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,105);                  /* prior: WHITE, body 5, HighLow still 10 */
  int d=pb_bar(106,106.5,105.5,106);       /* doji gapping up over the ceiling 105 */
  pb_detect(d,-100,"detect white-first: prior body 5 > 2, doji body 0 <= 1, gap up 106 > 105");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,102);                  /* prior body 2 == avg */
  int f0=pb_bar(103,103.5,102.5,103);
  pb_flip(f0,0,"break c0: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,105);
  int k0=pb_bar(106,106.5,105.5,106);
  pb_control(k0,-100,0,"restore c0: prior body 5 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,105);
  int f1=pb_bar(106,108.5,105.5,108);      /* body 2 > 1 */
  pb_flip(f1,1,"break c1: body 2 > 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,105);
  int k1=pb_bar(106,107.5,105.5,107);      /* body 1 == avg */
  pb_control(k1,-100,1,"restore c1: body 1 == 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,105);
  int f2=pb_bar(105,105.5,104.5,105);      /* floor 105 == prior ceiling: no gap */
  pb_flip_in(f2,2,0,1,"break c2 alt0 term1: body floor 105 == prior ceiling 105, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,105);
  int k2=pb_bar(106,106.5,105.5,106);
  pb_control(k2,-100,2,"restore c2: gap up 106 > 105");
  pb_flat(8);

  /* BLACK-FIRST: the other output class AND the other disjunct of c2. The
   * prior candle is black, so the sign flips to +100 and the gap must be
   * downward. Reaching this arm is what pb_signs(2) requires. */
  pb_primer(12,100,2,4);
  pb_bar(105,106,96,100);                  /* prior: BLACK, body 5, HighLow still 10 */
  int db=pb_bar(99,99.5,98.5,99);          /* doji gapping down under the floor 100 */
  pb_detect(db,100,"detect black-first: prior body 5 > 2, doji body 0 <= 1, gap down 99 < 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(105,106,96,100);
  int f2b=pb_bar(100,100.5,99.5,100);      /* doji body ceiling 100 == prior floor 100 */
  pb_flip_in(f2b,2,1,1,"break c2 alt1 term1: body ceiling 100 == prior floor 100, the gap test is strict");
  pb_flat(8);
}

/* ---- Moderate tier, unit 4: the penetration family + counterattack -------- *
 *
 * Six flat, parameterless patterns, and the first unit where CONDITIONS ARE
 * ENTAILED BY THEIR SIBLINGS. Every previous unit had a flip for every
 * condition; these constrain the same prices from several directions at once,
 * so five of the thirty-six cannot be falsified alone and carry a derivation
 * instead. Each waiver is refutable in the usual way: produce a flip whose
 * paired control fires and it is wrong -- and a sixth was, so read the ones
 * that remain as claims rather than as settled facts. All five reason purely
 * from inequalities among prices, which hold for any valid OHLC; the refuted
 * one reasoned about a settings average, which the builder controls.
 *
 * INNECK, ONNECK, THRUSTING and PIERCING share their first conditions
 * character for character -- prior black, prior long, this white, this opening
 * below the prior low -- and diverge only in where the closing price has to
 * land. A model copy-pasted between them agrees on two thirds of its
 * conditions, which is precisely the case where agreement proves nothing, so
 * the four are built together and each penetration test is flipped at its OWN
 * reference point:
 *
 *   INNECK     close(i) between close(i-1) and close(i-1) + Equal
 *   ONNECK     close(i) within +/-Equal of the prior LOW
 *   THRUSTING  close(i) above close(i-1) + Equal, up to the prior body's midpoint
 *   PIERCING   close(i) above that same midpoint, below the prior OPEN
 *
 * The prior bar is (112,113,96,100) throughout: black, open 112, close 100,
 * low 96, real body 12. The body is the load-bearing part. It sits inside the
 * 10-bar window ending at i, so it sets the averages THERE, and 12 keeps them
 * exact where unit 3's body of 5 would not:
 *
 *   prior body  5  ->  avg at i = (9*2 +  5)/10 = 2.2999999999999998
 *   prior body 12  ->  avg at i = (9*2 + 12)/10 = 3.0   exact
 *                      midpoint = 100 + 12*0.5  = 106   exact
 *
 * Only RealBody-typed settings and an Equal read at i-1 are consulted by these
 * six, and avg(Equal, i-1) excludes bar i-1, so the prior bar's HighLow is free
 * to be whatever the body needs. avg(Equal, i-1) is 0.5 off the primer alone,
 * and avg(BodyLong, i-1) is 2.
 */

/* CDLINNECK -- the white candle closes just barely into the prior black body.
 *
 *   c0  color(i-1) == -1
 *   c1  realbody(i-1) > avg(BodyLong, i-1)
 *   c2  color(i) == 1
 *   c3  open(i) < low(i-1)
 *   c4  close(i) <= close(i-1) + avg(Equal, i-1)
 *   c5  close(i) >= close(i-1)
 *
 * c4 and c5 are both inclusive, so each is pinned by a control sitting exactly
 * on it rather than by its flip.
 */
static void cond_inneck( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-1);
   c[0] = !pb_white(i-1);
   c[1] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[2] = pb_white(i);
   c[3] = pbO[i] < pbL[i-1];
   c[4] = pbC[i] <= pbC[i-1] + eq;
   c[5] = pbC[i] >= pbC[i-1];
}

static void build_inneck( void )
{
  pb_conditions(6);

  pb_waive(2, "c3 puts open(i) below low(i-1), and low(i-1) <= close(i-1) on any bar; "
              "c5 puts close(i) at or above close(i-1). So close(i) >= close(i-1) >= "
              "low(i-1) > open(i) and the candle is white by construction");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* prior: black, body 12, low 96, close 100 */
  int d=pb_bar(95,101,94,100.2);
  pb_detect(d,-100,"detect: prior black long, white opening at 95 < 96, close 100.2 in [100,100.5]");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* prior WHITE, close 112 -> band [112,112.5] */
  int f0=pb_bar(95,113,94,112.2);
  pb_flip(f0,0,"break c0: the prior candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k0=pb_bar(95,101,94,100.2);
  pb_control(k0,-100,0,"restore c0: the prior candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior body 2 == avg */
  int f1=pb_bar(95,101,94,100.2);
  pb_flip(f1,1,"break c1: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k1=pb_bar(95,101,94,100.2);
  pb_control(k1,-100,1,"restore c1: prior body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f3=pb_bar(96,101,94,100.2);          /* open 96 == the prior low */
  pb_flip(f3,3,"break c3: open 96 == prior low 96, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k3=pb_bar(95,101,94,100.2);
  pb_control(k3,-100,3,"restore c3: open 95 < prior low 96");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f4=pb_bar(95,101,94,100.6);          /* close above the band top 100.5 */
  pb_flip(f4,4,"break c4: close 100.6 > band top 100.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k4=pb_bar(95,101,94,100.5);          /* close exactly on it */
  pb_control(k4,-100,4,"restore c4: close 100.5 == band top, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f5=pb_bar(95,101,94,99.9);           /* close below the prior close */
  pb_flip(f5,5,"break c5: close 99.9 < prior close 100");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k5=pb_bar(95,101,94,100);            /* close exactly on it */
  pb_control(k5,-100,5,"restore c5: close 100 == prior close, inclusive");
  pb_flat(8);
}

/* CDLONNECK -- the white candle closes back at the prior LOW, not its close.
 *
 *   c0..c3  identical to INNECK
 *   c4  close(i) <= low(i-1) + avg(Equal, i-1)
 *   c5  close(i) >= low(i-1) - avg(Equal, i-1)
 *
 * Changing the reference point from the prior close to the prior low is the
 * whole difference, and it also breaks INNECK's entailment: the band now sits
 * AT the prior low rather than above the prior close, so a black second candle
 * is reachable and c2 gets a real flip here where INNECK needs a waiver.
 */
static void cond_onneck( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-1);
   c[0] = !pb_white(i-1);
   c[1] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[2] = pb_white(i);
   c[3] = pbO[i] < pbL[i-1];
   c[4] = pbC[i] <= pbL[i-1] + eq;
   c[5] = pbC[i] >= pbL[i-1] - eq;
}

static void build_onneck( void )
{
  pb_conditions(6);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* prior: black, body 12, low 96 -> band [95.5,96.5] */
  int d=pb_bar(95,97,94,96);
  pb_detect(d,-100,"detect: white opening at 95 < 96, close 96 in [95.5,96.5]");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* prior WHITE, body 12; low still 96, band unchanged */
  int f0=pb_bar(95,97,94,96);
  pb_flip(f0,0,"break c0: the prior candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k0=pb_bar(95,97,94,96);
  pb_control(k0,-100,0,"restore c0: the prior candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior body 2 == avg */
  int f1=pb_bar(95,97,94,96);
  pb_flip(f1,1,"break c1: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k1=pb_bar(95,97,94,96);
  pb_control(k1,-100,1,"restore c1: prior body 12 > 2");
  pb_flat(8);

  /* c2 is flippable here, unlike INNECK: the band reaches BELOW the opening
   * price, so a black candle can still satisfy c3, c4 and c5. */
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f2=pb_bar(95.9,97,94,95.5);          /* black: closes under its own open */
  pb_flip(f2,2,"break c2: this candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k2=pb_bar(95,97,94,96);
  pb_control(k2,-100,2,"restore c2: this candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f3=pb_bar(96,97,94,96.2);            /* open 96 == the prior low */
  pb_flip(f3,3,"break c3: open 96 == prior low 96, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k3=pb_bar(95,97,94,96);
  pb_control(k3,-100,3,"restore c3: open 95 < prior low 96");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f4=pb_bar(95,97,94,96.6);            /* close above the band top 96.5 */
  pb_flip(f4,4,"break c4: close 96.6 > band top 96.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k4=pb_bar(95,97,94,96.5);
  pb_control(k4,-100,4,"restore c4: close 96.5 == band top, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f5=pb_bar(95,97,94,95.4);            /* close below the band bottom 95.5 */
  pb_flip(f5,5,"break c5: close 95.4 < band bottom 95.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k5=pb_bar(95,97,94,95.5);
  pb_control(k5,-100,5,"restore c5: close 95.5 == band bottom, inclusive");
  pb_flat(8);
}

/* CDLTHRUSTING -- the white candle pushes INTO the prior body but stops short
 * of its midpoint.
 *
 *   c0..c3  identical to INNECK
 *   c4  close(i) >  close(i-1) + avg(Equal, i-1)
 *   c5  close(i) <= close(i-1) + realbody(i-1) * 0.5
 *
 * c4 is INNECK's c4 turned around: where INNECK caps the close just above the
 * prior close, THRUSTING requires it to be past that point. So the two
 * patterns are mutually exclusive on the same bars, and a model that borrowed
 * INNECK's direction would fire on nothing this builder detects.
 */
static void cond_thrusting( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-1);
   c[0] = !pb_white(i-1);
   c[1] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[2] = pb_white(i);
   c[3] = pbO[i] < pbL[i-1];
   c[4] = pbC[i] >  pbC[i-1] + eq;
   c[5] = pbC[i] <= pbC[i-1] + pb_body(i-1) * 0.5;
}

static void build_thrusting( void )
{
  pb_conditions(6);

  pb_waive(2, "c3 puts open(i) below low(i-1) <= close(i-1), and c4 puts close(i) "
              "above close(i-1). So close(i) > close(i-1) >= low(i-1) > open(i) and "
              "the candle is white by construction");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* prior: close 100, body 12 -> midpoint 106 */
  int d=pb_bar(95,103,94,101);
  pb_detect(d,-100,"detect: white opening at 95 < 96, close 101 in (100.5, 102.5]");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* prior WHITE: close 112, body 12 -> midpoint 118 */
  int f0=pb_bar(95,119,94,113);            /* close 113 in (112.5, 118] */
  pb_flip(f0,0,"break c0: the prior candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k0=pb_bar(95,103,94,101);
  pb_control(k0,-100,0,"restore c0: the prior candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior body 2 == avg -> midpoint 101 */
  int f1=pb_bar(95,102,94,100.8);
  pb_flip(f1,1,"break c1: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k1=pb_bar(95,103,94,101);
  pb_control(k1,-100,1,"restore c1: prior body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f3=pb_bar(96,103,94,101);            /* open 96 == the prior low */
  pb_flip(f3,3,"break c3: open 96 == prior low 96, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k3=pb_bar(95,103,94,101);
  pb_control(k3,-100,3,"restore c3: open 95 < prior low 96");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f4=pb_bar(95,103,94,100.5);          /* close exactly at close(i-1)+Equal */
  pb_flip(f4,4,"break c4: close 100.5 == 100 + Equal, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k4=pb_bar(95,103,94,101);
  pb_control(k4,-100,4,"restore c4: close 101 > 100.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f5=pb_bar(95,107,94,106.1);          /* close above the midpoint 106 */
  pb_flip(f5,5,"break c5: close 106.1 > midpoint 106");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k5=pb_bar(95,107,94,106);            /* close exactly on the midpoint */
  pb_control(k5,-100,5,"restore c5: close 106 == midpoint, inclusive");
  pb_flat(8);
}

/* CDLPIERCING -- the white candle closes ABOVE the prior body's midpoint.
 *
 *   c0  color(i-1) == -1
 *   c1  realbody(i-1) > avg(BodyLong, i-1)
 *   c2  color(i) == 1
 *   c3  realbody(i)   > avg(BodyLong, i)
 *   c4  open(i)  < low(i-1)
 *   c5  close(i) < open(i-1)
 *   c6  close(i) > close(i-1) + realbody(i-1) * 0.5
 *
 * Two of the seven are entailed, for two different reasons, so each carries its
 * own derivation rather than one shared note. THRUSTING's c5 and this c6 are
 * the same midpoint approached from opposite sides, so the two patterns
 * partition the prior body between them.
 *
 * c3 is NOT entailed, though it reads as if it were: c4 and c6 do bound
 * realbody(i) from below, but by a quantity built from the PRIOR bar, while the
 * average c3 tests against is set by the ten bars before i -- which the builder
 * chooses. Lift the primer and the average clears the bound. See the flip.
 */
static void cond_piercing( int i, int *c )
{
   c[0] = !pb_white(i-1);
   c[1] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[2] = pb_white(i);
   c[3] = pb_body(i)   > pb_avg(TA_BodyLong, i);
   c[4] = pbO[i]  < pbL[i-1];
   c[5] = pbC[i]  < pbO[i-1];
   c[6] = pbC[i]  > pbC[i-1] + pb_body(i-1) * 0.5;
}

static void build_piercing( void )
{
  pb_conditions(7);

  pb_waive(0, "were the prior candle white, open(i-1) < close(i-1), so c6 requires "
              "close(i) above close(i-1) + half a body -- itself above open(i-1) -- "
              "while c5 requires close(i) below open(i-1). No value satisfies both, "
              "so c5 & c6 entail a black prior candle");
  pb_waive(2, "c4 puts open(i) below low(i-1) <= close(i-1), and c6 puts close(i) "
              "above close(i-1). So close(i) > open(i) and the candle is white by "
              "construction");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* prior: black, open 112, close 100, midpoint 106 */
  int d=pb_bar(95,113,94,108);
  pb_detect(d,100,"detect: white opening at 95 < 96, close 108 above midpoint 106, below open 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior body 2 == avg -> midpoint 101 */
  int f1=pb_bar(95,102,94,101.5);          /* close 101.5 > 101 and < open 102 */
  pb_flip(f1,1,"break c1: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k1=pb_bar(95,113,94,108);
  pb_control(k1,100,1,"restore c1: prior body 12 > 2");
  pb_flat(8);

  /* c3 is the only condition in this unit whose threshold the BUILDER sets, and
   * that is what makes it flippable. c4 and c6 do bound realbody(i) from below
   * -- open(i) < low(i-1) and close(i) > close(i-1) + realbody(i-1)/2 give
   * realbody(i) > realbody(i-1)/2 + (close(i-1) - low(i-1)) -- but every term
   * there comes from the PRIOR bar, while avg(BodyLong, i) is the mean body
   * over the ten bars ending at i-1. Raising the primer raises the average and
   * leaves the floor where it was.
   *
   * Primer 10 with a prior body of 20 and low == close puts that floor at
   * 20/2 + 0 = 10 and the average at (9*10 + 20)/10 = 11 exact, so realbody(i)
   * in (10, 11] breaks c3 and nothing else. c4 then needs open(i) < 100 and c6
   * needs close(i) > 110: open 99.5 / close 110.5 clears both by a hair while
   * realbody(i) lands exactly ON the average, which is where a strict test has
   * to be pinned -- the same shape as c1's flip above. */
  pb_primer(12,100,10,4);
  pb_bar(120,120,100,100);                 /* prior: black, body 20, low == close */
  int f3=pb_bar(99.5,110.5,99.5,110.5);    /* body 11 == avg 11 */
  pb_flip(f3,3,"break c3: body 11 == avg 11, the test is strict");
  pb_flat(8);
  pb_primer(12,100,10,4);
  pb_bar(120,120,100,100);
  int k3=pb_bar(99.5,111.5,99.5,111.5);    /* body 12 > avg 11 */
  pb_control(k3,100,3,"restore c3: body 12 > avg 11");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f4=pb_bar(96,113,94,108);            /* open 96 == the prior low */
  pb_flip(f4,4,"break c4: open 96 == prior low 96, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k4=pb_bar(95,113,94,108);
  pb_control(k4,100,4,"restore c4: open 95 < prior low 96");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f5=pb_bar(95,113,94,112);            /* close 112 == the prior open */
  pb_flip(f5,5,"break c5: close 112 == prior open 112, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k5=pb_bar(95,113,94,108);
  pb_control(k5,100,5,"restore c5: close 108 < prior open 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f6=pb_bar(95,113,94,106);            /* close exactly on the midpoint */
  pb_flip(f6,6,"break c6: close 106 == midpoint 106, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k6=pb_bar(95,113,94,108);
  pb_control(k6,100,6,"restore c6: close 108 > midpoint 106");
  pb_flat(8);
}

/* CDLHOMINGPIGEON -- a small black candle entirely inside a large black one.
 *
 *   c0  color(i-1) == -1
 *   c1  color(i)   == -1
 *   c2  realbody(i-1) > avg(BodyLong, i-1)
 *   c3  realbody(i)  <= avg(BodyShort, i)
 *   c4  open(i)  < open(i-1)
 *   c5  close(i) > close(i-1)
 *
 * c4 and c5 together are the containment, expressed on the open and the close
 * because both candles are black. That is also what entails c0: with a white
 * prior candle the two would force this candle white too, contradicting c1.
 */
static void cond_homingpigeon( int i, int *c )
{
   c[0] = !pb_white(i-1);
   c[1] = !pb_white(i);
   c[2] = pb_body(i-1) >  pb_avg(TA_BodyLong, i-1);
   c[3] = pb_body(i)   <= pb_avg(TA_BodyShort, i);
   c[4] = pbO[i]  < pbO[i-1];
   c[5] = pbC[i]  > pbC[i-1];
}

static void build_homingpigeon( void )
{
  pb_conditions(6);

  pb_waive(0, "were the prior candle white, open(i-1) < close(i-1); c4 puts open(i) "
              "below open(i-1) and c5 puts close(i) above close(i-1), so close(i) > "
              "close(i-1) > open(i-1) > open(i) makes this candle white -- which c1 "
              "forbids. So c1, c4 and c5 entail a black prior candle");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* prior: black, open 112, close 100 */
  int d=pb_bar(110,111,99,108);            /* black, body 2, inside on both ends */
  pb_detect(d,100,"detect: both black, body 2 <= 3, open 110 < 112, close 108 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f1=pb_bar(106,111,99,108);           /* WHITE */
  pb_flip(f1,1,"break c1: this candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k1=pb_bar(110,111,99,108);
  pb_control(k1,100,1,"restore c1: this candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior body 2 == avg */
  int f2=pb_bar(101.5,102,99,100.5);
  pb_flip(f2,2,"break c2: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k2=pb_bar(110,111,99,108);
  pb_control(k2,100,2,"restore c2: prior body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f3=pb_bar(110,111,99,106);           /* body 4 > 3 */
  pb_flip(f3,3,"break c3: body 4 > avg 3");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k3=pb_bar(110,111,99,107);           /* body 3 == avg */
  pb_control(k3,100,3,"restore c3: body 3 == avg 3, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f4=pb_bar(112,113,99,110);           /* open 112 == the prior open */
  pb_flip(f4,4,"break c4: open 112 == prior open 112, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k4=pb_bar(110,111,99,108);
  pb_control(k4,100,4,"restore c4: open 110 < prior open 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f5=pb_bar(103,104,99,100);           /* close 100 == the prior close */
  pb_flip(f5,5,"break c5: close 100 == prior close 100, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k5=pb_bar(110,111,99,108);
  pb_control(k5,100,5,"restore c5: close 108 > prior close 100");
  pb_flat(8);
}

/* CDLCOUNTERATTACK -- two long opposite candles closing at the same level.
 *
 *   c0  color(i-1) == -color(i)
 *   c1  realbody(i-1) > avg(BodyLong, i-1)
 *   c2  realbody(i)   > avg(BodyLong, i)
 *   c3  close(i) <= close(i-1) + avg(Equal, i-1)
 *   c4  close(i) >= close(i-1) - avg(Equal, i-1)
 *
 * Bi-signed, and the sign comes from THIS candle: color(i)*100. c0 compares
 * the two colours rather than fixing either, so both classes are reachable
 * with the same five conditions -- unlike DOJISTAR, where the disjunct and the
 * class were selected by the same colour. Flipping c0 means making the two
 * candles the SAME colour, which is a different scenario from either detect.
 */
static void cond_counterattack( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-1);
   c[0] = pb_white(i-1) != pb_white(i);
   c[1] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[2] = pb_body(i)   > pb_avg(TA_BodyLong, i);
   c[3] = pbC[i] <= pbC[i-1] + eq;
   c[4] = pbC[i] >= pbC[i-1] - eq;
}

static void build_counterattack( void )
{
  pb_conditions(5);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* prior: BLACK, body 12, close 100 -> band [99.5,100.5] */
  int d=pb_bar(95,101,94,100);             /* white, body 5, close 100 */
  pb_detect(d,100,"detect black-then-white: opposite, bodies 12 and 5 > 3, closes equal");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f0=pb_bar(104,105,95,100);           /* BLACK: same colour as the prior */
  pb_flip(f0,0,"break c0: both candles are black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k0=pb_bar(95,101,94,100);
  pb_control(k0,100,0,"restore c0: opposite colours");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* prior body 2 == avg */
  int f1=pb_bar(95,101,94,100);
  pb_flip(f1,1,"break c1: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k1=pb_bar(95,101,94,100);
  pb_control(k1,100,1,"restore c1: prior body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f2=pb_bar(97,101,96,100);            /* body 3 == avg */
  pb_flip(f2,2,"break c2: body 3 == avg 3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k2=pb_bar(95,101,94,100);
  pb_control(k2,100,2,"restore c2: body 5 > 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f3=pb_bar(95,101,94,100.6);          /* close above the band top 100.5 */
  pb_flip(f3,3,"break c3: close 100.6 > band top 100.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k3=pb_bar(95,101,94,100.5);
  pb_control(k3,100,3,"restore c3: close 100.5 == band top, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int f4=pb_bar(95,101,94,99.4);           /* close below the band bottom 99.5 */
  pb_flip(f4,4,"break c4: close 99.4 < band bottom 99.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  int k4=pb_bar(95,101,94,99.5);
  pb_control(k4,100,4,"restore c4: close 99.5 == band bottom, inclusive");
  pb_flat(8);

  /* The other output class: a white candle first, so THIS one is black and the
   * sign flips. Same five conditions, mirrored. */
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* prior: WHITE, body 12, close 112 */
  int db=pb_bar(117,118,111,112);          /* black, body 5, close 112 */
  pb_detect(db,-100,"detect white-then-black: opposite, bodies 12 and 5 > 3, closes equal");
  pb_flat(8);
}

/* ---- Moderate tier, unit 5: the penetration patterns --------------------- *
 *
 * The five non-hard candlesticks that take optInPenetration, and the first
 * builders to go through pb_check_mcdc_p. Two things are new here.
 *
 * THE PARAMETER IS PASSED PER PATTERN, NOT SHARED. CDLDARKCLOUDCOVER defaults
 * to 0.5 and the four stars to 0.3; each builder passes its own, so a boundary
 * flip means the same thing here as on a parameterless pattern.
 *
 * A PENETRATION BOUNDARY IS WRITTEN AS THE ARITHMETIC, NOT AS A DECIMAL.
 * close(i-2) +/- realbody(i-2) * penetration is not a round number for a
 * penetration of 0.3 -- 100 + 12*0.3 is 103.59999999999999432 -- and while
 * that happens to be the same double as the literal 103.6, relying on that is
 * relying on a coincidence. The flips below write `100.0 + 12.0*0.3`, which is
 * the expression the library evaluates, so the two agree by construction.
 *
 * THREE-BAR PATTERNS PUT TWO BARS INSIDE THE WINDOW AT i. Unit 4's lesson
 * doubled: with primer body P, first bar A and second bar B,
 *
 *   avg(BodyLong,  i-2) = P                  window is all primer
 *   avg(BodyShort, i-1) = (9P + A)/10        the first bar is in it
 *   avg(BodyShort, i)   = (8P + A + B)/10    both are
 *
 * P=2 with A=12, B=2 gives 2.0, 3.0, 3.0 -- all exact -- and that is what the
 * two STAR builders use, except in the c5 control, which raises B to 3 so that
 * it sits ON the inclusive boundary (3 <= 3) instead of inside it. c5 is the
 * one conjunct here spelled <= rather than <, and a control strictly inside it
 * leaves the two indistinguishable: 2 <= 3 and 2 < 3 are both true, so nothing
 * would fail were the library to lose the equality. The DOJISTAR pair gets the
 * placement for free -- its doji body of 1 is exactly its threshold.
 *
 * The DOJISTAR builders cannot use the STAR geometry: their middle bar must
 * be a doji, so B is 0, and their c5 reads BodyDoji, which is HighLow-typed
 * and therefore constrains the FIRST bar's high-low range to 10 -- capping its
 * body at 10, so A=12 is unavailable. A=4, B=0 gives 2.0, 1.0, 2.0 instead,
 * also exact. The two geometries are why the four are not one copy-pasted
 * builder.
 */

/* CDLDARKCLOUDCOVER -- a black candle opening above a long white one and
 * closing well into it.
 *
 *   c0  color(i-1) == 1
 *   c1  realbody(i-1) > avg(BodyLong, i-1)
 *   c2  color(i) == -1
 *   c3  open(i)  > high(i-1)
 *   c4  close(i) > open(i-1)
 *   c5  close(i) < close(i-1) - realbody(i-1) * penetration
 *
 * Two conditions are entailed, and both derivations use only price ordering --
 * no settings average, which is the distinction that made unit 4's PIERCING c3
 * waiver wrong.
 */
static void cond_darkcloudcover( int i, int *c )
{
   c[0] = pb_white(i-1);
   c[1] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[2] = !pb_white(i);
   c[3] = pbO[i] > pbH[i-1];
   c[4] = pbC[i] > pbO[i-1];
   c[5] = pbC[i] < pbC[i-1] - pb_body(i-1) * 0.5;
}

static void build_darkcloudcover( void )
{
  pb_conditions(6);

  pb_waive(0, "were the prior candle black, open(i-1) > close(i-1); c4 puts close(i) "
              "above open(i-1) and c5 puts it below close(i-1) minus half a body, "
              "itself below close(i-1). No value is both above open(i-1) and below "
              "close(i-1) when the first exceeds the second, so c4 & c5 entail a "
              "white prior candle");
  pb_waive(2, "c3 puts open(i) above high(i-1), which is at or above close(i-1); c5 "
              "puts close(i) below close(i-1). So close(i) < open(i) and the candle "
              "is black by construction");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* prior: white, body 12, open 100, close 112, high 113 */
  int d=pb_bar(114,115,103,104);           /* black, opens above 113, closes at 104 in (100,106) */
  pb_detect(d,-100,"detect: opens 114 > high 113, closes 104 above open 100 and below 112 - 6");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(105,113,96,107);                  /* prior body 2 == avg -> band (105,106) */
  int f1=pb_bar(114,115,104,105.5);
  pb_flip(f1,1,"break c1: prior body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int k1=pb_bar(114,115,103,104);
  pb_control(k1,-100,1,"restore c1: prior body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int f3=pb_bar(113,115,103,104);          /* opens exactly at the prior high */
  pb_flip(f3,3,"break c3: open 113 == prior high 113, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int k3=pb_bar(114,115,103,104);
  pb_control(k3,-100,3,"restore c3: open 114 > prior high 113");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int f4=pb_bar(114,115,99,100);           /* closes exactly at the prior open */
  pb_flip(f4,4,"break c4: close 100 == prior open 100, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int k4=pb_bar(114,115,103,104);
  pb_control(k4,-100,4,"restore c4: close 104 > prior open 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int f5=pb_bar(114,115,105,112.0-12.0*0.5);   /* closes exactly on the penetration line */
  pb_flip(f5,5,"break c5: close 106 == 112 - 12*0.5, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  int k5=pb_bar(114,115,103,104);
  pb_control(k5,-100,5,"restore c5: close 104 < 106");
  pb_flat(8);
}

/* CDLMORNINGSTAR -- a long black candle, a small body gapping down from it,
 * then a white candle closing well back into the first.
 *
 *   c0  color(i-2) == -1
 *   c1  color(i)   == 1
 *   c2  realbodygapdown(i-1, i-2)
 *   c3  close(i) >  close(i-2) + realbody(i-2) * penetration
 *   c4  realbody(i-2) >  avg(BodyLong,  i-2)
 *   c5  realbody(i-1) <= avg(BodyShort, i-1)
 *   c6  realbody(i)   >  avg(BodyShort, i)
 *
 * Three separate averaging windows, at three different bars. c4's is entirely
 * primer, c5's contains the first bar, c6's contains both -- which is why the
 * c4 flip has to be checked against what it does to the other two: shrinking
 * the first bar's body to the BodyLong boundary also lowers c5's and c6's
 * thresholds, and the scenario has to stay inside them.
 */
static void cond_morningstar( int i, int *c )
{
   c[0] = !pb_white(i-2);
   c[1] = pb_white(i);
   c[2] = pb_bodyhi(i-1) < pb_bodylo(i-2);
   c[3] = pbC[i] >  pbC[i-2] + pb_body(i-2) * 0.3;
   c[4] = pb_body(i-2) >  pb_avg(TA_BodyLong,  i-2);
   c[5] = pb_body(i-1) <= pb_avg(TA_BodyShort, i-1);
   c[6] = pb_body(i)   >  pb_avg(TA_BodyShort, i);
}

static void build_morningstar( void )
{
  pb_conditions(7);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* 1st: black, body 12, close 100 */
  pb_bar(97,98,94,95);                     /* 2nd: body 2, gaps down under 100 */
  int d=pb_bar(99,105,98,104);             /* 3rd: white, body 5, closes 104 */
  pb_detect(d,100,"detect: close 104 > 100 + 12*0.3, bodies 12 > 2, 2 <= 3, 5 > 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* 1st WHITE, close 112 -> line 115.6 */
  pb_bar(97,98,94,95);
  int f0=pb_bar(111,117,110,116);
  pb_flip(f0,0,"break c0: the first candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int k0=pb_bar(99,105,98,104);
  pb_control(k0,100,0,"restore c0: the first candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int f1=pb_bar(108,109,102,104);          /* BLACK third candle, body 4 */
  pb_flip(f1,1,"break c1: the third candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int k1=pb_bar(99,105,98,104);
  pb_control(k1,100,1,"restore c1: the third candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(100,101,98,98);                   /* 2nd body top 100 == the 1st's floor */
  int f2=pb_bar(99,105,98,104);
  pb_flip(f2,2,"break c2: body top 100 == the first candle's floor 100, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int k2=pb_bar(99,105,98,104);
  pb_control(k2,100,2,"restore c2: body top 97 < 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int f3=pb_bar(99,105,98,100.0+12.0*0.3); /* closes exactly on the penetration line */
  pb_flip(f3,3,"break c3: close == 100 + 12*0.3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int k3=pb_bar(99,105,98,104);
  pb_control(k3,100,3,"restore c3: close 104 is above the line");
  pb_flat(8);

  /* c4's flip drops the first body to the BodyLong boundary, which also drops
   * c5's threshold to 2.0 and c6's to 2.0 -- both still satisfied by the same
   * second and third bars, and the penetration line falls to 100 + 2*0.3. */
  pb_primer(12,100,2,4);
  pb_bar(102,113,96,100);                  /* 1st body 2 == avg */
  pb_bar(97,98,94,95);
  int f4=pb_bar(99,105,98,104);
  pb_flip(f4,4,"break c4: first body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int k4=pb_bar(99,105,98,104);
  pb_control(k4,100,4,"restore c4: first body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,92,93);                     /* 2nd body 4 > 3 */
  int f5=pb_bar(99,105,98,104);
  pb_flip(f5,5,"break c5: second body 4 > avg 3");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,93,94);                     /* body 3 == avg 3, the inclusive side */
  int k5=pb_bar(99,105,98,104);
  pb_control(k5,100,5,"restore c5: second body 3 == avg 3, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int f6=pb_bar(101,105,100,104);          /* 3rd body 3 == avg */
  pb_flip(f6,6,"break c6: third body 3 == avg 3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);
  pb_bar(97,98,94,95);
  int k6=pb_bar(99,105,98,104);
  pb_control(k6,100,6,"restore c6: third body 5 > 3");
  pb_flat(8);
}

/* CDLEVENINGSTAR -- MORNINGSTAR upside down.
 *
 *   c0  color(i-2) == 1
 *   c1  color(i)   == -1
 *   c2  realbodygapup(i-1, i-2)
 *   c3  close(i) <  close(i-2) - realbody(i-2) * penetration
 *   c4..c6  identical to MORNINGSTAR
 *
 * Only the first four differ from MORNINGSTAR, and all four differ only in
 * direction: the colours swap, the gap inverts, and the penetration line is
 * subtracted rather than added. That is the whole pattern, which is exactly
 * why a model copy-pasted from MORNINGSTAR would agree on c4, c5 and c6 and
 * disagree with the library on everything that matters.
 */
static void cond_eveningstar( int i, int *c )
{
   c[0] = pb_white(i-2);
   c[1] = !pb_white(i);
   c[2] = pb_bodylo(i-1) > pb_bodyhi(i-2);
   c[3] = pbC[i] <  pbC[i-2] - pb_body(i-2) * 0.3;
   c[4] = pb_body(i-2) >  pb_avg(TA_BodyLong,  i-2);
   c[5] = pb_body(i-1) <= pb_avg(TA_BodyShort, i-1);
   c[6] = pb_body(i)   >  pb_avg(TA_BodyShort, i);
}

static void build_eveningstar( void )
{
  pb_conditions(7);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);                  /* 1st: white, body 12, close 112 */
  pb_bar(115,118,114,117);                 /* 2nd: body 2, gaps up over 112 */
  int d=pb_bar(113,114,107,108);           /* 3rd: black, body 5, closes 108 */
  pb_detect(d,-100,"detect: close 108 < 112 - 12*0.3, bodies 12 > 2, 2 <= 3, 5 > 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,96,100);                  /* 1st BLACK, close 100 -> line 96.4 */
  pb_bar(115,118,114,117);
  int f0=pb_bar(101,102,95,96);
  pb_flip(f0,0,"break c0: the first candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int k0=pb_bar(113,114,107,108);
  pb_control(k0,-100,0,"restore c0: the first candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int f1=pb_bar(104,114,103,108);          /* WHITE third candle, body 4 */
  pb_flip(f1,1,"break c1: the third candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int k1=pb_bar(113,114,107,108);
  pb_control(k1,-100,1,"restore c1: the third candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(112,118,111,114);                 /* 2nd body floor 112 == the 1st's ceiling */
  int f2=pb_bar(113,114,107,108);
  pb_flip(f2,2,"break c2: body floor 112 == the first candle's ceiling 112, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int k2=pb_bar(113,114,107,108);
  pb_control(k2,-100,2,"restore c2: body floor 115 > 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int f3=pb_bar(113,114,107,112.0-12.0*0.3);  /* closes exactly on the penetration line */
  pb_flip(f3,3,"break c3: close == 112 - 12*0.3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int k3=pb_bar(113,114,107,108);
  pb_control(k3,-100,3,"restore c3: close 108 is below the line");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,102);                  /* 1st body 2 == avg -> line 101.4 */
  pb_bar(115,118,114,117);
  int f4=pb_bar(113,114,100,101);
  pb_flip(f4,4,"break c4: first body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int k4=pb_bar(113,114,107,108);
  pb_control(k4,-100,4,"restore c4: first body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,120,114,119);                 /* 2nd body 4 > 3 */
  int f5=pb_bar(113,114,107,108);
  pb_flip(f5,5,"break c5: second body 4 > avg 3");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,119,114,118);                 /* body 3 == avg 3, the inclusive side */
  int k5=pb_bar(113,114,107,108);
  pb_control(k5,-100,5,"restore c5: second body 3 == avg 3, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int f6=pb_bar(111,114,107,108);          /* 3rd body 3 == avg */
  pb_flip(f6,6,"break c6: third body 3 == avg 3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,113,96,112);
  pb_bar(115,118,114,117);
  int k6=pb_bar(113,114,107,108);
  pb_control(k6,-100,6,"restore c6: third body 5 > 3");
  pb_flat(8);
}

/* CDLMORNINGDOJISTAR -- MORNINGSTAR with the middle candle required to be a
 * doji rather than merely short.
 *
 *   c0..c4, c6  identical to MORNINGSTAR
 *   c5  realbody(i-1) <= avg(BodyDoji, i-1)
 *
 * One condition differs, and it changes the whole geometry. BodyDoji is
 * HighLow-typed, so its window measures the RANGE of the ten bars ending at
 * i-2 -- which pins the first bar's high-low to 10 for the threshold to stay
 * exact, capping its body at 10 and putting MORNINGSTAR's A=12 out of reach.
 * A=4, B=0 gives 2.0 / 1.0 / 2.0 instead. Sharing MORNINGSTAR's bars here
 * would leave c5's flip nowhere near its boundary.
 */
static void cond_morningdojistar( int i, int *c )
{
   c[0] = !pb_white(i-2);
   c[1] = pb_white(i);
   c[2] = pb_bodyhi(i-1) < pb_bodylo(i-2);
   c[3] = pbC[i] >  pbC[i-2] + pb_body(i-2) * 0.3;
   c[4] = pb_body(i-2) >  pb_avg(TA_BodyLong,  i-2);
   c[5] = pb_body(i-1) <= pb_avg(TA_BodyDoji,  i-1);
   c[6] = pb_body(i)   >  pb_avg(TA_BodyShort, i);
}

static void build_morningdojistar( void )
{
  pb_conditions(7);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);                  /* 1st: black, body 4, HighLow 10, close 100 */
  pb_bar(98,99,97,98);                     /* 2nd: doji, body 0, gaps down under 100 */
  int d=pb_bar(100,104,99,103);            /* 3rd: white, body 3, closes 103 */
  pb_detect(d,100,"detect: close 103 > 100 + 4*0.3, bodies 4 > 2, 0 <= 1, 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,106,94,100);                   /* 1st WHITE, body 4, close 100 */
  pb_bar(94,95,93,94);
  int f0=pb_bar(100,104,99,103);
  pb_flip(f0,0,"break c0: the first candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int k0=pb_bar(100,104,99,103);
  pb_control(k0,100,0,"restore c0: the first candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int f1=pb_bar(106,107,102,103);          /* BLACK third candle, body 3 */
  pb_flip(f1,1,"break c1: the third candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int k1=pb_bar(100,104,99,103);
  pb_control(k1,100,1,"restore c1: the third candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(100,101,99,100);                  /* 2nd body top 100 == the 1st's floor */
  int f2=pb_bar(100,104,99,103);
  pb_flip(f2,2,"break c2: body top 100 == the first candle's floor 100, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int k2=pb_bar(100,104,99,103);
  pb_control(k2,100,2,"restore c2: body top 98 < 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int f3=pb_bar(99,104,98,100.0+4.0*0.3);  /* closes exactly on the line, body 2.2 keeps c6 true */
  pb_flip(f3,3,"break c3: close == 100 + 4*0.3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int k3=pb_bar(100,104,99,103);
  pb_control(k3,100,3,"restore c3: close 103 is above the line");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,100);                  /* 1st body 2 == avg */
  pb_bar(98,99,97,98);
  int f4=pb_bar(100,104,99,103);
  pb_flip(f4,4,"break c4: first body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int k4=pb_bar(100,104,99,103);
  pb_control(k4,100,4,"restore c4: first body 4 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,96,96);                     /* 2nd body 2 > 1 */
  int f5=pb_bar(100,104,99,103);
  pb_flip(f5,5,"break c5: second body 2 > doji threshold 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,96,97);                     /* body 1 == threshold */
  int k5=pb_bar(100,104,99,103);
  pb_control(k5,100,5,"restore c5: second body 1 == threshold 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int f6=pb_bar(101,104,100,103);          /* 3rd body 2 == avg */
  pb_flip(f6,6,"break c6: third body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(98,99,97,98);
  int k6=pb_bar(100,104,99,103);
  pb_control(k6,100,6,"restore c6: third body 3 > 2");
  pb_flat(8);
}

/* CDLEVENINGDOJISTAR -- EVENINGSTAR with a doji middle candle.
 *
 *   c0..c4, c6  identical to EVENINGSTAR
 *   c5  realbody(i-1) <= avg(BodyDoji, i-1)
 *
 * The fourth corner of the square: MORNING/EVENING differ by direction,
 * STAR/DOJISTAR by which setting the middle body is measured against. Each of
 * the four shares six of its seven conditions with two of the others, so the
 * only cases that separate them are the ones placed on the condition that
 * differs -- c0..c3 between the directions, c5 between the two settings.
 */
static void cond_eveningdojistar( int i, int *c )
{
   c[0] = pb_white(i-2);
   c[1] = !pb_white(i);
   c[2] = pb_bodylo(i-1) > pb_bodyhi(i-2);
   c[3] = pbC[i] <  pbC[i-2] - pb_body(i-2) * 0.3;
   c[4] = pb_body(i-2) >  pb_avg(TA_BodyLong,  i-2);
   c[5] = pb_body(i-1) <= pb_avg(TA_BodyDoji,  i-1);
   c[6] = pb_body(i)   >  pb_avg(TA_BodyShort, i);
}

static void build_eveningdojistar( void )
{
  pb_conditions(7);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);                   /* 1st: white, body 4, HighLow 10, close 100 */
  pb_bar(102,103,101,102);                 /* 2nd: doji, body 0, gaps up over 100 */
  int d=pb_bar(100,101,96,97);             /* 3rd: black, body 3, closes 97 */
  pb_detect(d,-100,"detect: close 97 < 100 - 4*0.3, bodies 4 > 2, 0 <= 1, 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);                  /* 1st BLACK, body 4, close 100 */
  pb_bar(106,107,105,106);
  int f0=pb_bar(100,101,96,97);
  pb_flip(f0,0,"break c0: the first candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int k0=pb_bar(100,101,96,97);
  pb_control(k0,-100,0,"restore c0: the first candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int f1=pb_bar(94,101,93,97);             /* WHITE third candle, body 3 */
  pb_flip(f1,1,"break c1: the third candle is white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int k1=pb_bar(100,101,96,97);
  pb_control(k1,-100,1,"restore c1: the third candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(100,101,99,100);                  /* 2nd body floor 100 == the 1st's ceiling */
  int f2=pb_bar(100,101,96,97);
  pb_flip(f2,2,"break c2: body floor 100 == the first candle's ceiling 100, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int k2=pb_bar(100,101,96,97);
  pb_control(k2,-100,2,"restore c2: body floor 102 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int f3=pb_bar(101,102,96,100.0-4.0*0.3); /* closes exactly on the line, body 2.2 keeps c6 true */
  pb_flip(f3,3,"break c3: close == 100 - 4*0.3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int k3=pb_bar(100,101,96,97);
  pb_control(k3,-100,3,"restore c3: close 97 is below the line");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(98,106,96,100);                   /* 1st body 2 == avg */
  pb_bar(102,103,101,102);
  int f4=pb_bar(100,101,96,97);
  pb_flip(f4,4,"break c4: first body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int k4=pb_bar(100,101,96,97);
  pb_control(k4,-100,4,"restore c4: first body 4 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,105,101,104);                 /* 2nd body 2 > 1 */
  int f5=pb_bar(100,101,96,97);
  pb_flip(f5,5,"break c5: second body 2 > doji threshold 1");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,104,101,103);                 /* body 1 == threshold */
  int k5=pb_bar(100,101,96,97);
  pb_control(k5,-100,5,"restore c5: second body 1 == threshold 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int f6=pb_bar(99,101,96,97);             /* 3rd body 2 == avg */
  pb_flip(f6,6,"break c6: third body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(96,106,96,100);
  pb_bar(102,103,101,102);
  int k6=pb_bar(100,101,96,97);
  pb_control(k6,-100,6,"restore c6: third body 3 > 2");
  pb_flat(8);
}

/* ---- Hard tier: CDLADVANCEBLOCK ------------------------------------------ *
 *
 * Three rising white candles whose advance is "blocked" -- the bodies shrink or
 * the upper shadows lengthen. Twelve top-level conjuncts, five settings, and a
 * structure no builder before this one has had to deal with: c11 is a FOUR-WAY
 * DISJUNCTION of alternative blocking shapes, nine atomic comparisons inside it.
 *
 *   c0   color(i-2) == 1
 *   c1   color(i-1) == 1
 *   c2   color(i)   == 1
 *   c3   close(i)   >  close(i-1)
 *   c4   close(i-1) >  close(i-2)
 *   c5   open(i-1)  >  open(i-2)
 *   c6   open(i-1)  <= close(i-2) + avg(Near, i-2)
 *   c7   open(i)    >  open(i-1)
 *   c8   open(i)    <= close(i-1) + avg(Near, i-1)
 *   c9   realbody(i-2) > avg(BodyLong, i-2)
 *   c10  uppershadow(i-2) < avg(ShadowShort, i-2)
 *   c11  D1 || D2 || D3 || D4, where
 *          D1 = rb(i-1) < rb(i-2) - avg(Far,i-2) && rb(i) < rb(i-1) + avg(Near,i-1)
 *          D2 = rb(i)   < rb(i-1) - avg(Far,i-1)
 *          D3 = rb(i) < rb(i-1) && rb(i-1) < rb(i-2) &&
 *               ( ush(i) > avg(ShadowShort,i) || ush(i-1) > avg(ShadowShort,i-1) )
 *          D4 = rb(i) < rb(i-1) && ush(i) > avg(ShadowLong,i)
 *
 * THE GEOMETRY IS SOLVED, NOT CHOSEN. Five settings with three window lengths
 * (BodyLong/ShadowShort over 10, Near/Far over 5, ShadowLong self-referential)
 * and three range types (RealBody, HighLow, Shadows -- the last carrying a
 * divisor of 2) all read windows that the scenario bars fall into. Keeping a
 * threshold exactly representable is a constraint on the bars, not a matter of
 * picking round numbers:
 *
 *   HighLow(i-2)  = 10   keeps Near(i-1) = 2 and Far(i-1) = 6 exact
 *   Shadows(i-2)  = 3    keeps ShadowShort(i-1) = 3.75 exact
 *   Shadows(i-1)  = 3    keeps ShadowShort(i)   = 3.5  exact
 *
 * and BodyLong/ShadowShort/Near/Far at i-2 stay on all-primer windows, so they
 * are the primer's own 2, 4, 2 and 6. There is no assignment that leaves every
 * window at its primer value: c9 needs realbody(i-2) ABOVE the primer body,
 * which moves HighLow(i-2) into every HighLow-typed window downstream of it.
 *
 * THE PRIMER DECIDES WHETHER c0 IS A WAIVER. A black first candle needs
 * open(i-2) > close(i-2) + avg(BodyLong) from c9, while c5 and c6 bound
 * open(i-1) into (open(i-2), close(i-2) + avg(Near)] -- satisfiable only when
 * Near(i-2) > BodyLong(i-2), i.e. only when hr > 2*bd. The file's usual
 * pb_primer(12,100,2,4) sits EXACTLY on that boundary (both are 2), so c0 is
 * entailed there and a builder copying the convention would waive it and be
 * right by accident. It is flipped below on hr=5, where Near is 2.4 against a
 * BodyLong of 2 and a black first candle exists.
 *
 * c11 IS COUPLED TO EVERYTHING. Every disjunct compares real bodies, and c3..c8
 * are opens and closes -- which ARE the bodies. Moving a close to its boundary
 * moves a body, and three of the twelve flips (c4, c6, c7) drop c11 with them
 * unless bar i is re-cut to restore a disjunct. c7 is the sharp case: with
 * open(i) == open(i-1) and close(i) > close(i-1), realbody(i) > realbody(i-1)
 * is FORCED, which kills D2, D3 and D4 outright. Only D1 survives, and D1 needs
 * realbody(i-2) > realbody(i-1) + avg(Far,i-2) -- a geometry the other eleven
 * flips do not use. That flip therefore carries its own primer-to-bar layout.
 */
static void cond_advanceblock( int i, int *c )
{
   c[0]  = pb_white(i-2);
   c[1]  = pb_white(i-1);
   c[2]  = pb_white(i);
   c[3]  = pbC[i]   > pbC[i-1];
   c[4]  = pbC[i-1] > pbC[i-2];
   c[5]  = pbO[i-1] > pbO[i-2];
   c[6]  = pbO[i-1] <= pbC[i-2] + pb_avg(TA_Near, i-2);
   c[7]  = pbO[i]   > pbO[i-1];
   c[8]  = pbO[i]   <= pbC[i-1] + pb_avg(TA_Near, i-1);
   c[9]  = pb_body(i-2) > pb_avg(TA_BodyLong, i-2);
   c[10] = pb_upsh(i-2) < pb_avg(TA_ShadowShort, i-2);
   c[11] = ( pb_body(i-1) < pb_body(i-2) - pb_avg(TA_Far,  i-2) &&
             pb_body(i)   < pb_body(i-1) + pb_avg(TA_Near, i-1) )
        || ( pb_body(i)   < pb_body(i-1) - pb_avg(TA_Far,  i-1) )
        || ( pb_body(i)   < pb_body(i-1) &&
             pb_body(i-1) < pb_body(i-2) &&
             ( pb_upsh(i)   > pb_avg(TA_ShadowShort, i) ||
               pb_upsh(i-1) > pb_avg(TA_ShadowShort, i-1) ) )
        || ( pb_body(i)   < pb_body(i-1) &&
             pb_upsh(i)   > pb_avg(TA_ShadowLong, i) );
}

/* c11's four alternatives. A flip of c11 falsifies all four at once and a
 * control only restores whichever one the bars happen to satisfy, so nothing
 * above this makes any single alternative necessary: with D3 and D4 both live
 * in the detect, D2 could be deleted from the pattern outright and every case
 * here still passed. */
static void arm_advanceblock( int i, int cond, int arm, int *a )
{
   if( cond != 11 ) return;
   switch( arm ) {
   case 0:
      a[0] = pb_body(i-1) < pb_body(i-2) - pb_avg(TA_Far,  i-2);
      a[1] = pb_body(i)   < pb_body(i-1) + pb_avg(TA_Near, i-1);
      break;
   case 1:
      a[0] = pb_body(i)   < pb_body(i-1) - pb_avg(TA_Far,  i-1);
      break;
   case 2:
      a[0] = pb_body(i)   < pb_body(i-1);
      a[1] = pb_body(i-1) < pb_body(i-2);
      a[2] = ( pb_upsh(i)   > pb_avg(TA_ShadowShort, i) ||
               pb_upsh(i-1) > pb_avg(TA_ShadowShort, i-1) );
      break;
   default:
      a[0] = pb_body(i)   < pb_body(i-1);
      a[1] = pb_upsh(i)   > pb_avg(TA_ShadowLong, i);
      break;
   }
}

static void build_advanceblock( void )
{
  pb_conditions(12);
  pb_disjuncts(11,4);
  pb_arm(11,0,2); pb_arm(11,1,1); pb_arm(11,2,3); pb_arm(11,3,2);
  pb_arm_model(arm_advanceblock);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s1=pb_bar(108,117,106,112);
  pb_detect(s1,-100,"detect: three rising whites, bodies 7/6/4, ush(i) 5 > ShadowLong 4 -- D3 and D4 both hold");
  pb_flat(8);

  pb_primer(12,100,2,5);
  pb_bar(102.25,103.25,99,100);
  pb_bar(102.375,107,102,106);
  int s2=pb_bar(104,111,103,107);
  pb_flip(s2,0,"break c0: the first candle is black (hr=5 puts Near 2.4 above BodyLong 2)");
  pb_flat(8);

  pb_primer(12,100,2,5);
  pb_bar(100,103.25,99,102.25);
  pb_bar(102.375,107,102,106);
  int s3=pb_bar(104,111,103,107);
  pb_control(s3,-100,0,"restore c0: the first candle is white, same hr=5 layout");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(109,110,107.5,108);
  int s4=pb_bar(109.5,112,109,110);
  pb_flip(s4,1,"break c1: the second candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s5=pb_bar(108,117,106,112);
  pb_control(s5,-100,1,"restore c1: the second candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s6=pb_bar(112,117,110,111);
  pb_flip(s6,2,"break c2: the third candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s7=pb_bar(108,117,106,112);
  pb_control(s7,-100,2,"restore c2: the third candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s8=pb_bar(108,117,106,110);
  pb_flip(s8,3,"break c3: close 110 == the second close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s9=pb_bar(108,117,106,112);
  pb_control(s9,-100,3,"restore c3: close 112 > 110");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,107);
  int s10=pb_bar(108,117,106,110);
  pb_flip(s10,4,"break c4: close 107 == the first close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s11=pb_bar(108,117,106,112);
  pb_control(s11,-100,4,"restore c4: close 110 > 107");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(100,111,98,110);
  int s12=pb_bar(108,117,106,112);
  pb_flip(s12,5,"break c5: open 100 == the first open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s13=pb_bar(108,117,106,112);
  pb_control(s13,-100,5,"restore c5: open 104 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(109.5,111,102,110);
  int s14=pb_bar(110,112,109,110.25);
  pb_flip(s14,6,"break c6: open 109.5 above close(i-2) 107 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(109,111,102,110);
  int s15=pb_bar(110,112,109,110.25);
  pb_control(s15,-100,6,"restore c6: open 109 == 107 + Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);
  pb_bar(108,110,107,109);
  int s16=pb_bar(108,111,107,110);
  pb_flip(s16,7,"break c7: open 108 == the second open, the test is strict (D1 is the only disjunct that survives it)");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);
  pb_bar(108,110,107,109);
  int s17=pb_bar(109,111,107,110);
  pb_control(s17,-100,7,"restore c7: open 109 > 108, same D1 layout");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s18=pb_bar(112.5,117,106,113);
  pb_flip(s18,8,"break c8: open 112.5 above close(i-1) 110 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s19=pb_bar(112,117,106,113);
  pb_control(s19,-100,8,"restore c8: open 112 == 110 + Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,103,98,102);
  pb_bar(104,111,102,110);
  int s20=pb_bar(108,117,106,112);
  pb_flip(s20,9,"break c9: first body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s21=pb_bar(108,117,106,112);
  pb_control(s21,-100,9,"restore c9: first body 7 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,98,107);
  pb_bar(104,111,102,110);
  int s22=pb_bar(108,117,106,112);
  pb_flip(s22,10,"break c10: first upper shadow 4 == avg 4, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s23=pb_bar(108,117,106,112);
  pb_control(s23,-100,10,"restore c10: first upper shadow 1 < 4");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s24=pb_bar(108,119,106,118);
  pb_flip(s24,11,"break c11: body(i) 10 exceeds body(i-1) 6, so D2/D3/D4 fail, and body(i-1) 6 is not below body(i-2) 7 - Far 6, so D1 fails too");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int s25=pb_bar(108,117,106,112);
  pb_control(s25,-100,11,"restore c11: D3 and D4 hold");
  pb_flat(8);

  /* One firing case per alternative, each with the other three false. */
  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);                  /* body 8 */
  pb_bar(108,110,107,109);                 /* body 1 < 8 - Far 6 */
  int a0=pb_bar(109,111,107,110);
  pb_sole(a0,-100,11,0,"c11 alt D1 alone: 2nd far smaller than 1st, 3rd not longer than 2nd");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,114,103,113);                 /* body 8 */
  int a1=pb_bar(114,115,113,115);          /* body 1 < 8 - Far 6 */
  pb_sole(a1,-100,11,1,"c11 alt D2 alone: 3rd far smaller than 2nd");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);                  /* body 7 */
  pb_bar(104,114,102,110);                 /* body 6, upper shadow 4 > 3.75 */
  int a2=pb_bar(106,111,105,111);          /* body 5, upper shadow 0 -- D4 cannot hold */
  pb_sole(a2,-100,11,2,"c11 alt D3 alone: bodies 7 > 6 > 5 with the 2nd carrying the long upper shadow");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,114,103,113);                 /* body 8 */
  int a3=pb_bar(114,121,113,117);          /* body 3, ush 4 > ShadowLong 3; 3 is not < 8-6 */
  pb_sole(a3,-100,11,3,"c11 alt D4 alone: 3rd smaller than 2nd with a long upper shadow");
  pb_flat(8);

  /* c11's alternatives are conjunctions, and a flip of c11 falsifies all four
   * at once without naming a term inside any of them. These do: each leaves
   * exactly one term of one alternative false, every other term of that
   * alternative true, and the other three alternatives false -- so the zero is
   * attributable to the term on the label. D2 is a single term and needs none:
   * there, the term and the alternative are the same proposition. */
  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);
  pb_bar(108,110,107,110);
  int t00=pb_bar(109,111,107,111);
  pb_flip_in(t00,11,0,0,"break c11 alt D1 term0: 2nd body 2 == 1st body 8 - Far 6, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);
  pb_bar(108,110,107,109);
  int t01=pb_bar(109,116,107,115);
  pb_flip_in(t01,11,0,1,"break c11 alt D1 term1: 3rd body 6 exceeds 2nd body 1 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,114,102,110);
  int t20=pb_bar(106,113,105,112);
  pb_flip_in(t20,11,2,0,"break c11 alt D3 term0: 3rd body 6 == 2nd body 6, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,115,102,111);
  int t21=pb_bar(107,113,105,112);
  pb_flip_in(t21,11,2,1,"break c11 alt D3 term1: 2nd body 7 == 1st body 7, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,111,102,110);
  int t22=pb_bar(106,111,105,111);
  pb_flip_in(t22,11,2,2,"break c11 alt D3 term2: neither upper shadow clears its ShadowShort average");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,114,103,113);
  int t30=pb_bar(114,131,113,122);
  pb_flip_in(t30,11,3,0,"break c11 alt D4 term0: 3rd body 8 == 2nd body 8, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,114,103,113);
  int t31=pb_bar(114,120,113,117);
  pb_flip_in(t31,11,3,1,"break c11 alt D4 term1: upper shadow 3 == ShadowLong 3 (its own body), the test is strict");
  pb_flat(8);

}

/* ---- Hard tier: CDLGAPSIDESIDEWHITE -------------------------------------- *
 *
 * Two white candles of near-equal size opening at near-equal prices, both
 * gapping the same way off a first candle. Seven conditions, and the last
 * pattern in the corpus whose disjunction was reachable by nothing.
 *
 *   c0  ( gapup(i-1,i-2) && gapup(i,i-2) ) || ( gapdown(i-1,i-2) && gapdown(i,i-2) )
 *   c1  color(i-1) == 1
 *   c2  color(i)   == 1
 *   c3  realbody(i) >= realbody(i-1) - avg(Near,  i-1)
 *   c4  realbody(i) <= realbody(i-1) + avg(Near,  i-1)
 *   c5  open(i)     >= open(i-1)     - avg(Equal, i-1)
 *   c6  open(i)     <= open(i-1)     + avg(Equal, i-1)
 *
 * FOUR OF THE SEVEN ARE INCLUSIVE, which is the whole character of this one:
 * c3..c6 are two tolerance BANDS, each spelled with >= and <=. An inclusive
 * bound is pinned by the control sitting exactly on the equality, never by the
 * flip -- there is no minimal falsifying value above a `<=` -- so all four
 * controls below sit on their own edge and all four flips sit strictly outside.
 * Relaxing any one of the four to its strict form is caught by its control, not
 * by its flip.
 *
 * The geometry keeps both bands exact. Near and Equal are HighLow-typed over
 * five bars ending at i-1, so their windows hold four primer bars plus the
 * FIRST candle; giving that candle the primer's own HighLow of 10 leaves
 * Near = 2 and Equal = 0.5 exactly, and the two bands land on whole and half
 * units. Its body is otherwise unconstrained -- no condition reads it -- which
 * is what makes that free to choose.
 *
 * c0's two alternatives are mutually exclusive (a pair of bodies cannot gap
 * both up and down off the same candle), so a sole-true case for each is just
 * a case of that gap direction. They are declared and covered anyway rather
 * than argued away: the exclusivity is a fact about the predicates, and the
 * check that reads it is the one this file now has for saying so.
 *
 * This is also a bi-signed pattern -- the firing arm is
 * `gapup(i-1,i-2) ? 100 : -100` -- so the two detects fire the two classes.
 */
static void cond_gapsidesidewhite( int i, int *c )
{
   double n = pb_avg(TA_Near,  i-1);
   double e = pb_avg(TA_Equal, i-1);
   c[0] = ( pb_bodylo(i-1) > pb_bodyhi(i-2) && pb_bodylo(i) > pb_bodyhi(i-2) )
       || ( pb_bodyhi(i-1) < pb_bodylo(i-2) && pb_bodyhi(i) < pb_bodylo(i-2) );
   c[1] = pb_white(i-1);
   c[2] = pb_white(i);
   c[3] = pb_body(i) >= pb_body(i-1) - n;
   c[4] = pb_body(i) <= pb_body(i-1) + n;
   c[5] = pbO[i]     >= pbO[i-1]     - e;
   c[6] = pbO[i]     <= pbO[i-1]     + e;
}
static void arm_gapsidesidewhite( int i, int cond, int arm, int *a )
{
   if( cond != 0 ) return;
   if( arm == 0 ) { a[0] = pb_bodylo(i-1) > pb_bodyhi(i-2);
                    a[1] = pb_bodylo(i)   > pb_bodyhi(i-2); }
   else           { a[0] = pb_bodyhi(i-1) < pb_bodylo(i-2);
                    a[1] = pb_bodyhi(i)   < pb_bodylo(i-2); }
}

static void build_gapsidesidewhite( void )
{
  pb_conditions(7);
  pb_signs(2);
  pb_disjuncts(0,2);
  pb_arm(0,0,2); pb_arm(0,1,2);
  pb_arm_model(arm_gapsidesidewhite);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);                  /* 1st: body 4, HighLow 10 -> Near 2, Equal 0.5 */
  pb_bar(110,117,109,116);                 /* 2nd: white body 6, gaps up over 104 */
  int d1=pb_bar(110,117,109,116);          /* 3rd: same body, same open */
  pb_detect(d1,100,"detect gap up: bodies 6 and 6, opens both 110, both gapping over 104");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(90,97,89,96);                     /* 2nd: white body 6, gaps down under 100 */
  int d2=pb_bar(90,97,89,96);
  pb_detect(d2,-100,"detect gap down: the mirror, firing the other output class");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(104,117,103,110);                 /* body floor 104 == the 1st candle's ceiling */
  int f0=pb_bar(104,117,103,110);
  pb_flip(f0,0,"break c0: body floor 104 == the first candle's ceiling 104, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k0=pb_bar(110,117,109,116);
  pb_control(k0,100,0,"restore c0: body floor 110 > 104");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,109.75);              /* 2nd BLACK, body 0.25 */
  int f1=pb_bar(110,117,109,110.25);
  pb_flip(f1,1,"break c1: the second candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k1=pb_bar(110,117,109,116);
  pb_control(k1,100,1,"restore c1: the second candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,110.25);
  int f2=pb_bar(110,117,109,109.75);       /* 3rd BLACK, body 0.25 */
  pb_flip(f2,2,"break c2: the third candle is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k2=pb_bar(110,117,109,116);
  pb_control(k2,100,2,"restore c2: the third candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int f3=pb_bar(110,117,109,113.5);        /* body 3.5, below 6 - Near 2 */
  pb_flip(f3,3,"break c3: third body 3.5 below the band floor 6 - 2");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k3=pb_bar(110,117,109,114);          /* body 4 == the floor */
  pb_control(k3,100,3,"restore c3: third body 4 == 6 - 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int f4=pb_bar(110,119,109,118.5);        /* body 8.5, above 6 + Near 2 */
  pb_flip(f4,4,"break c4: third body 8.5 above the band ceiling 6 + 2");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k4=pb_bar(110,119,109,118);          /* body 8 == the ceiling */
  pb_control(k4,100,4,"restore c4: third body 8 == 6 + 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int f5=pb_bar(109.25,117,108,115.25);    /* open 109.25, below 110 - Equal 0.5 */
  pb_flip(f5,5,"break c5: third open 109.25 below the band floor 110 - 0.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k5=pb_bar(109.5,117,108,115.5);      /* open 109.5 == the floor */
  pb_control(k5,100,5,"restore c5: third open 109.5 == 110 - 0.5, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int f6=pb_bar(110.75,118,109,116.75);    /* open 110.75, above 110 + Equal 0.5 */
  pb_flip(f6,6,"break c6: third open 110.75 above the band ceiling 110 + 0.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int k6=pb_bar(110.5,118,109,116.5);      /* open 110.5 == the ceiling */
  pb_control(k6,100,6,"restore c6: third open 110.5 == 110 + 0.5, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int a0=pb_bar(110,117,109,116);
  pb_sole(a0,100,0,0,"c0 alt0 alone: both bodies gap UP, so the gap-down alternative cannot hold");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(90,97,89,96);
  int a1=pb_bar(90,97,89,96);
  pb_sole(a1,-100,0,1,"c0 alt1 alone: both bodies gap DOWN, so the gap-up alternative cannot hold");
  pb_flat(8);

  /* Both terms of an arm are gap tests, and c5/c6 hold the two opens within
   * Equal of each other -- so the two body edges they compare move together and
   * can only be separated by straddling the first candle's edge inside that
   * half-point band. That coupling is why a single flip of c0 breaks both terms
   * at once and names neither. */
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(104,117,103,110);
  int g00=pb_bar(104.25,117,103,110.25);
  pb_flip_in(g00,0,0,0,"break c0 alt0 term0: 2nd body floor 104 == the 1st candle's ceiling 104, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(104.25,117,103,110.25);
  int g01=pb_bar(104,117,103,110);
  pb_flip_in(g01,0,0,1,"break c0 alt0 term1: 3rd body floor 104 == the 1st candle's ceiling 104, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(94,97,89,100);
  int g10=pb_bar(93.75,97,89,99.75);
  pb_flip_in(g10,0,1,0,"break c0 alt1 term0: 2nd body ceiling 100 == the 1st candle's floor 100, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(93.75,97,89,99.75);
  int g11=pb_bar(94,97,89,100);
  pb_flip_in(g11,0,1,1,"break c0 alt1 term1: 3rd body ceiling 100 == the 1st candle's floor 100, the gap test is strict");
  pb_flat(8);

}

/* ---- Hard tier: CDLRISEFALL3METHODS -------------------------------------- *
 *
 * A long candle, three small ones of the opposite colour holding inside its
 * range and moving against it, then a long candle of the first one's colour
 * carrying past its close. Nineteen conditions over five bars -- the largest
 * conjunction in the corpus -- and the first builder to face three structures
 * at once.
 *
 * A FIVE-DEEP WINDOW CASCADE. BodyLong and BodyShort both default to
 * (RealBody, 10, 1.0), which is why one BodyPeriodTotal[] serves both, and the
 * five reads sit at i-4..i so each window swallows one more scenario bar than
 * the last:
 *
 *   avg(i-4) = P                          all primer
 *   avg(i-3) = (9P + A4)/10
 *   avg(i-2) = (8P + A4 + A3)/10
 *   avg(i-1) = (7P + A4 + A3 + A2)/10
 *   avg(i)   = (6P + A4 + A3 + A2 + A1)/10
 *
 * so every bar placed shifts every threshold after it. P=2 with bodies
 * 12, 2, 2, 2, 12 lands the five on 2, 3, 3, 3, 3 exactly, and that is the
 * geometry every scenario returns to.
 *
 * THE COLOUR CHAIN CANNOT BE BROKEN ONE LINK AT A TIME. c0..c3 compare
 * adjacent colours, so each interior bar appears in two of them and changing
 * one bar's colour falsifies both. Breaking exactly one link takes TWO colour
 * changes -- c1's flip is white, black, white, white, black, which leaves c0,
 * c2 and c3 satisfied and only c1 broken. The controls for those four are the
 * detect itself rather than the flip bars with one value moved back, because
 * restoring one link forces a second bar to move as well; there is no
 * single-value control to write.
 *
 * SIX CONDITIONS ARE ENTAILED IN ONE DIRECTION AND FREE IN THE OTHER, which is
 * why this pattern needs both branches and not merely for the output class.
 * On the rising side the 2nd to 4th are black, so a body's floor is its close
 * and c10/c11 chain them: close(i-1) < close(i-2) < close(i-3) < high(i-4)
 * makes c6 and c8 entailed. On the falling side those bars are white, the floor
 * is the open, and the chain says nothing about it -- so c6 and c8 are flipped
 * there. c7 and c9 are the same argument mirrored, entailed falling and flipped
 * rising. Nothing is waived: every condition is flipped, just not always on the
 * side you would first reach for.
 */
static void cond_risefall3methods( int i, int *c )
{
   int k4 = pb_white(i-4) ? 1 : -1;
   int k3 = pb_white(i-3) ? 1 : -1;
   int k2 = pb_white(i-2) ? 1 : -1;
   int k1 = pb_white(i-1) ? 1 : -1;
   int k0 = pb_white(i)   ? 1 : -1;
   double H4 = pbH[i-4], L4 = pbL[i-4];
   c[0]  = k4 == -k3;
   c[1]  = k3 ==  k2;
   c[2]  = k2 ==  k1;
   c[3]  = k1 == -k0;
   c[4]  = pb_bodylo(i-3) < H4;
   c[5]  = pb_bodyhi(i-3) > L4;
   c[6]  = pb_bodylo(i-2) < H4;
   c[7]  = pb_bodyhi(i-2) > L4;
   c[8]  = pb_bodylo(i-1) < H4;
   c[9]  = pb_bodyhi(i-1) > L4;
   c[10] = pbC[i-2]*k4 < pbC[i-3]*k4;
   c[11] = pbC[i-1]*k4 < pbC[i-2]*k4;
   c[12] = pbO[i]  *k4 > pbC[i-1]*k4;
   c[13] = pbC[i]  *k4 > pbC[i-4]*k4;
   c[14] = pb_body(i-4) > pb_avg(TA_BodyLong,  i-4);
   c[15] = pb_body(i-3) < pb_avg(TA_BodyShort, i-3);
   c[16] = pb_body(i-2) < pb_avg(TA_BodyShort, i-2);
   c[17] = pb_body(i-1) < pb_avg(TA_BodyShort, i-1);
   c[18] = pb_body(i)   > pb_avg(TA_BodyLong,  i);
}

static void build_risefall3methods( void )
{
  pb_conditions(19);
  pb_signs(2);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s1=pb_bar(105,118,104,117);
  pb_detect(s1,100,"detect rising: white, 3 falling black inside its range, white closing above it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(102,105,101,104);
  pb_bar(104,107,103,106);
  pb_bar(106,109,105,108);
  int s2=pb_bar(107,108,95,96);
  pb_detect(s2,-100,"detect falling: the mirror, firing the other output class");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(106,109,105,108);
  pb_bar(104,107,103,106);
  pb_bar(102,105,101,104);
  int s3=pb_bar(125,126,112,113);
  pb_flip(s3,0,"break c0: the 1st and 2nd are both white -- the 2nd AND 3rD are moved, because one colour change breaks two links of the chain");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s4=pb_bar(105,118,104,117);
  pb_control(s4,100,0,"restore c0: white, black, black, black, white -- the whole chain, since one link cannot be restored alone");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(104,107,103,106);
  pb_bar(102,105,101,104);
  int s5=pb_bar(125,126,112,113);
  pb_flip(s5,1,"break c1: the 2nd is black and the 3rd white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s6=pb_bar(105,118,104,117);
  pb_control(s6,100,1,"restore c1: the 2nd and 3rd are both black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(102,105,101,104);
  int s7=pb_bar(125,126,112,113);
  pb_flip(s7,2,"break c2: the 3rd is black and the 4th white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s8=pb_bar(105,118,104,117);
  pb_control(s8,100,2,"restore c2: the 3rd and 4th are both black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s9=pb_bar(125,126,112,113);
  pb_flip(s9,3,"break c3: the 4th and 5th are both black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s10=pb_bar(105,118,104,117);
  pb_control(s10,100,3,"restore c3: the 4th is black and the 5th white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(115,116,112,113);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s11=pb_bar(105,118,104,117);
  pb_flip(s11,4,"break c4: 2nd body floor 113 == the 1st's high 113, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(114,115,111,112);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s12=pb_bar(105,118,104,117);
  pb_control(s12,100,4,"restore c4: 2nd body floor 112 < 113");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,100,112);
  pb_bar(100,101,97,98);
  pb_bar(100.25,101,97,97.75);
  pb_bar(100.25,101,97,97.5);
  int s13=pb_bar(98,114,97,113);
  pb_flip(s13,5,"break c5: 2nd body ceiling 100 == the 1st's low 100, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,100,112);
  pb_bar(100.25,101,97,98.25);
  pb_bar(100.25,101,97,97.75);
  pb_bar(100.25,101,97,97.5);
  int s14=pb_bar(98,114,97,113);
  pb_control(s14,100,5,"restore c5: 2nd body ceiling 100.25 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(102,105,101,104);
  pb_bar(113,116,112,115);
  pb_bar(112.75,116,112,115.25);
  int s15=pb_bar(110,111,97,98);
  pb_flip(s15,6,"break c6: 3rd body floor 113 == the 1st's high 113 -- on the BEAR side, where it is not entailed");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(102,105,101,104);
  pb_bar(112.75,116,112,114.75);
  pb_bar(112.75,116,112,115.25);
  int s16=pb_bar(110,111,97,98);
  pb_control(s16,-100,6,"restore c6: 3rd body floor 112.75 < 113");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,100,112);
  pb_bar(101,102,98,99);
  pb_bar(100,101,97,98);
  pb_bar(100.25,101,97,97.75);
  int s17=pb_bar(98,114,97,113);
  pb_flip(s17,7,"break c7: 3rd body ceiling 100 == the 1st's low 100, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,100,112);
  pb_bar(101,102,98,99);
  pb_bar(100.25,101,97,98.25);
  pb_bar(100.25,101,97,97.75);
  int s18=pb_bar(98,114,97,113);
  pb_control(s18,100,7,"restore c7: 3rd body ceiling 100.25 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(102,105,101,104);
  pb_bar(104,107,103,106);
  pb_bar(113,116,112,115);
  int s19=pb_bar(110,111,97,98);
  pb_flip(s19,8,"break c8: 4th body floor 113 == the 1st's high 113 -- on the BEAR side, where it is not entailed");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(102,105,101,104);
  pb_bar(104,107,103,106);
  pb_bar(112.75,116,112,114.75);
  int s20=pb_bar(110,111,97,98);
  pb_control(s20,-100,8,"restore c8: 4th body floor 112.75 < 113");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,100,112);
  pb_bar(104,105,101,102);
  pb_bar(102,103,99,100);
  pb_bar(100,101,97,98);
  int s21=pb_bar(99,114,98,113);
  pb_flip(s21,9,"break c9: 4th body ceiling 100 == the 1st's low 100, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,100,112);
  pb_bar(104,105,101,102);
  pb_bar(102,103,99,100);
  pb_bar(100.25,101,97,98.25);
  int s22=pb_bar(99,114,98,113);
  pb_control(s22,100,9,"restore c9: 4th body ceiling 100.25 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(110,111,107,108);
  pb_bar(106,107,103,104);
  int s23=pb_bar(105,118,104,117);
  pb_flip(s23,10,"break c10: 3rd close 108 == the 2nd close, the fall is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s24=pb_bar(105,118,104,117);
  pb_control(s24,100,10,"restore c10: 3rd close 106 < the 2nd close 108");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(108,109,105,106);
  int s25=pb_bar(107,118,106,117);
  pb_flip(s25,11,"break c11: 4th close 106 == the 3rd close, the fall is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s26=pb_bar(105,118,104,117);
  pb_control(s26,100,11,"restore c11: 4th close 104 < the 3rd close 106");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s27=pb_bar(104,118,103,117);
  pb_flip(s27,12,"break c12: 5th opens 104 == the 4th close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s28=pb_bar(105,118,104,117);
  pb_control(s28,100,12,"restore c12: 5th opens 105 > the 4th close 104");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s29=pb_bar(105,113,104,112);
  pb_flip(s29,13,"break c13: 5th closes 112 == the 1st close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s30=pb_bar(105,118,104,117);
  pb_control(s30,100,13,"restore c13: 5th closes 117 > the 1st close 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,113,99,112);
  pb_bar(110,111,108,109);
  pb_bar(110,111,108,108.5);
  pb_bar(109.5,110,107,108);
  int s31=pb_bar(109,122,108,121);
  pb_flip(s31,14,"break c14: 1st body 2 == avg 2, the test is strict (the 3 small bodies shrink with it to stay under their own averages)");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(109,113,99,112);
  pb_bar(110,111,108,109);
  pb_bar(110,111,108,108.5);
  pb_bar(109.5,110,107,108);
  int s32=pb_bar(109,122,108,121);
  pb_control(s32,100,14,"restore c14: 1st body 3 > avg 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(111,112,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s33=pb_bar(105,118,104,117);
  pb_flip(s33,15,"break c15: 2nd body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s34=pb_bar(105,118,104,117);
  pb_control(s34,100,15,"restore c15: 2nd body 2 < avg 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(110,111,106,107);
  pb_bar(107,108,104,105);
  int s35=pb_bar(106,119,105,118);
  pb_flip(s35,16,"break c16: 3rd body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s36=pb_bar(105,118,104,117);
  pb_control(s36,100,16,"restore c16: 3rd body 2 < avg 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(108,109,104,105);
  int s37=pb_bar(106,119,105,118);
  pb_flip(s37,17,"break c17: 4th body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s38=pb_bar(105,118,104,117);
  pb_control(s38,100,17,"restore c17: 4th body 2 < avg 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s39=pb_bar(110,114,109,113);
  pb_flip(s39,18,"break c18: 5th body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(110,111,107,108);
  pb_bar(108,109,105,106);
  pb_bar(106,107,103,104);
  int s40=pb_bar(110,115,109,114);
  pb_control(s40,100,18,"restore c18: 5th body 4 > avg 3");
  pb_flat(8);

}

/* ---- Hard tier: CDLBREAKAWAY --------------------------------------------- *
 *
 * A long candle, a gap, then three candles walking further in the gap's
 * direction, and a fifth closing back INSIDE the gap. Five conditions -- and
 * the pattern that motivated pb_arm(), because c4's two alternatives hold
 * EIGHT conjuncts each and every one of them was reachable by nothing.
 *
 *   c0  color(i-4) == color(i-3)
 *   c1  color(i-3) == color(i-1)
 *   c2  color(i-1) == -color(i)
 *   c3  realbody(i-4) > avg(BodyLong, i-4)
 *   c4  alt0 (1st black) || alt1 (1st white), each of them
 *         term0  the 1st candle's colour -- the arm selector
 *         term1  the 2nd gaps away from the 1st
 *         term2  the 3rd's high steps past the 2nd's
 *         term3  the 3rd's low steps past the 2nd's
 *         term4  the 4th's high steps past the 3rd's
 *         term5  the 4th's low steps past the 3rd's
 *         term6  the 5th closes past the 2nd's open
 *         term7  the 5th closes short of the 1st's close
 *
 * A flip of c4 falsifies both alternatives at once and names no term in
 * either, so twelve of these sixteen would have gone untested while the gate
 * printed "declared 5, source has 5 -- ok". They are attributed instead, one
 * case per term per arm, each leaving the other seven terms of its arm true
 * and the opposite arm false.
 *
 * TWO TERMS PER ARM ARE WAIVED, and for the two shapes worth telling apart.
 * term0 is the arm's own selector: it is what decides which alternative is
 * live, and the class it decides is fired by pb_signs(2). term1 is entailed --
 * terms 6 and 7 place close(5th) strictly between close(1st) and open(2nd),
 * which forces open(2nd) past close(1st), and c0 makes the 1st and 2nd the
 * same colour, so for that colour those two prices ARE the body edges the gap
 * test compares. A price-ordering derivation, not a settings-average one.
 *
 * THE GEOMETRY SEPARATES THE HIGHS AND LOWS FROM THE BODIES. terms 2..5 compare
 * highs and lows while terms 6..7 and c3 compare bodies, so the bars are cut
 * with each high and low one unit clear of its neighbour and well outside every
 * body. That is what lets a single high move to its boundary without dragging a
 * body across one of the other tests -- the coupling that made CDLADVANCEBLOCK
 * and CDLRISEFALL3METHODS need re-cut bars on three flips apiece.
 *
 * c0..c2 are a colour chain, so the same rule as CDLRISEFALL3METHODS applies:
 * one link cannot be broken by moving one candle, and each flip moves two.
 */
static void cond_breakaway( int i, int *c )
{
   int k4 = pb_white(i-4) ? 1 : -1, k3 = pb_white(i-3) ? 1 : -1;
   int k1 = pb_white(i-1) ? 1 : -1, k0 = pb_white(i)   ? 1 : -1;
   c[0] = k4 == k3;
   c[1] = k3 == k1;
   c[2] = k1 == -k0;
   c[3] = pb_body(i-4) > pb_avg(TA_BodyLong, i-4);
   c[4] = ( k4 == -1 && pb_bodyhi(i-3) < pb_bodylo(i-4) &&
            pbH[i-2] < pbH[i-3] && pbL[i-2] < pbL[i-3] &&
            pbH[i-1] < pbH[i-2] && pbL[i-1] < pbL[i-2] &&
            pbC[i] > pbO[i-3] && pbC[i] < pbC[i-4] )
       || ( k4 ==  1 && pb_bodylo(i-3) > pb_bodyhi(i-4) &&
            pbH[i-2] > pbH[i-3] && pbL[i-2] > pbL[i-3] &&
            pbH[i-1] > pbH[i-2] && pbL[i-1] > pbL[i-2] &&
            pbC[i] < pbO[i-3] && pbC[i] > pbC[i-4] );
}
static void arm_breakaway( int i, int cond, int arm, int *a )
{
   if( cond != 4 ) return;
   if( arm == 0 )
   {
      a[0] = !pb_white(i-4);
      a[1] = pb_bodyhi(i-3) < pb_bodylo(i-4);
      a[2] = pbH[i-2] < pbH[i-3];  a[3] = pbL[i-2] < pbL[i-3];
      a[4] = pbH[i-1] < pbH[i-2];  a[5] = pbL[i-1] < pbL[i-2];
      a[6] = pbC[i] > pbO[i-3];    a[7] = pbC[i] < pbC[i-4];
   }
   else
   {
      a[0] = pb_white(i-4);
      a[1] = pb_bodylo(i-3) > pb_bodyhi(i-4);
      a[2] = pbH[i-2] > pbH[i-3];  a[3] = pbL[i-2] > pbL[i-3];
      a[4] = pbH[i-1] > pbH[i-2];  a[5] = pbL[i-1] > pbL[i-2];
      a[6] = pbC[i] < pbO[i-3];    a[7] = pbC[i] > pbC[i-4];
   }
}

static void build_breakaway( void )
{
  pb_conditions(5);
  pb_signs(2);
  pb_arm(4,0,8); pb_arm(4,1,8);
  pb_arm_model(arm_breakaway);
  pb_waive_arm(4,0,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(4,0,1,"entailed by terms 6 and 7: they put close(5th) strictly between close(1st) and open(2nd), so open(2nd) > close(1st); c0 makes the 1st and 2nd the same colour, and for that colour those two prices ARE the body edges the gap test compares");
  pb_waive_arm(4,1,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(4,1,1,"entailed by terms 6 and 7: they put close(5th) strictly between close(1st) and open(2nd), so open(2nd) > close(1st); c0 makes the 1st and 2nd the same colour, and for that colour those two prices ARE the body edges the gap test compares");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b1=pb_bar(119,120,110,111);
  pb_detect(b1,-100,"detect gap up: white 1st, 2nd gaps above it, 3rd and 4th step higher, black 5th closing back inside the gap");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,89,93);
  pb_bar(93,96,88,91);
  int b2=pb_bar(89,99,88,99);
  pb_detect(b2,100,"detect gap down: the mirror, firing the other output class");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(114,120,112,112);
  pb_bar(115,121,113,116);
  pb_bar(118,122,114,117);
  int b3=pb_bar(112,113,110,113);
  pb_flip(b3,0,"break c0: 1st white, 2nd black -- the 4th and 5th move with it, since one colour change would break c1 or c2 as well");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b4=pb_bar(119,120,110,111);
  pb_control(b4,-100,0,"restore c0: 1st and 2nd both white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(118,122,114,117);
  int b5=pb_bar(110.5,111,110,111);
  pb_flip(b5,1,"break c1: 2nd white, 4th black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b6=pb_bar(119,120,110,111);
  pb_control(b6,-100,1,"restore c1: 2nd and 4th both white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b7=pb_bar(110.5,111,110,111);
  pb_flip(b7,2,"break c2: 4th and 5th both white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b8=pb_bar(119,120,110,111);
  pb_control(b8,-100,2,"restore c2: 4th white, 5th black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b9=pb_bar(119,120,110,111);
  pb_flip(b9,3,"break c3: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(107,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b10=pb_bar(119,120,110,111);
  pb_control(b10,-100,3,"restore c3: 1st body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,98,89,93);
  pb_bar(93,96,88,91);
  int b11=pb_bar(89,99,88,99);
  pb_flip_in(b11,4,0,2,"break c4 alt0 term2: 3rd high 98 == the 2nd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,90,93);
  pb_bar(93,96,88,91);
  int b12=pb_bar(89,99,88,99);
  pb_flip_in(b12,4,0,3,"break c4 alt0 term3: 3rd low 90 == the 2nd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,89,93);
  pb_bar(93,97,88,91);
  int b13=pb_bar(89,99,88,99);
  pb_flip_in(b13,4,0,4,"break c4 alt0 term4: 4th high 97 == the 3rd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,89,93);
  pb_bar(93,96,89,91);
  int b14=pb_bar(89,99,88,99);
  pb_flip_in(b14,4,0,5,"break c4 alt0 term5: 4th low 89 == the 3rd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,89,93);
  pb_bar(93,96,88,91);
  int b15=pb_bar(89,99,88,98);
  pb_flip_in(b15,4,0,6,"break c4 alt0 term6: 5th closes 98 == the 2nd open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,89,93);
  pb_bar(93,96,88,91);
  int b16=pb_bar(89,100,88,100);
  pb_flip_in(b16,4,0,7,"break c4 alt0 term7: 5th closes 100 == the 1st close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,120,113,116);
  pb_bar(117,122,114,118);
  int b17=pb_bar(119,120,110,111);
  pb_flip_in(b17,4,1,2,"break c4 alt1 term2: 3rd high 120 == the 2nd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,112,116);
  pb_bar(117,122,114,118);
  int b18=pb_bar(119,120,110,111);
  pb_flip_in(b18,4,1,3,"break c4 alt1 term3: 3rd low 112 == the 2nd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,121,114,118);
  int b19=pb_bar(119,120,110,111);
  pb_flip_in(b19,4,1,4,"break c4 alt1 term4: 4th high 121 == the 3rd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,113,118);
  int b20=pb_bar(119,120,110,111);
  pb_flip_in(b20,4,1,5,"break c4 alt1 term5: 4th low 113 == the 3rd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b21=pb_bar(119,120,110,112);
  pb_flip_in(b21,4,1,6,"break c4 alt1 term6: 5th closes 112 == the 2nd open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int b22=pb_bar(119,120,109,110);
  pb_flip_in(b22,4,1,7,"break c4 alt1 term7: 5th closes 110 == the 1st close, the test is strict");
  pb_flat(8);


  /* c4's paired controls, one per alternative: the attributed flips above all
   * assert a zero, and a control is what shows the zero belongs to the term
   * that moved rather than to the geometry those bars happen to sit in. */
  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  pb_bar(112,120,112,114);
  pb_bar(115,121,113,116);
  pb_bar(117,122,114,118);
  int c4w=pb_bar(119,120,110,111);
  pb_control(c4w,-100,4,"restore c4 via alt1: every term of the gap-up alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  pb_bar(98,98,90,96);
  pb_bar(95,97,89,93);
  pb_bar(93,96,88,91);
  int c4b=pb_bar(89,99,88,99);
  pb_control(c4b,100,4,"restore c4 via alt0: every term of the gap-down alternative holds");
  pb_flat(8);

}

/* ---- Hard tier: CDLABANDONEDBABY ----------------------------------------- *
 *
 * A long candle, a doji abandoned on a gap, then a candle gapping back the
 * other way and closing deep into the first. Four conditions, and the first
 * builder to run BOTH mechanisms at once: it registers through
 * pb_check_mcdc_p, because it takes optInPenetration, and c3's alternatives
 * hold five terms each, so it needs pb_arm as well.
 *
 *   c0  realbody(i-2) >  avg(BodyLong,  i-2)
 *   c1  realbody(i-1) <= avg(BodyDoji,  i-1)      inclusive
 *   c2  realbody(i)   >  avg(BodyShort, i)
 *   c3  alt0 (bearish) || alt1 (bullish), each of them
 *         term0  the 1st candle's colour
 *         term1  the 3rd candle's colour
 *         term2  the 3rd closes past close(1st) -/+ realbody(1st) * penetration
 *         term3  the doji gaps away from the 1st
 *         term4  the 3rd gaps back past the doji
 *
 * NOTHING HERE IS WAIVED, which is the difference from CDLBREAKAWAY and worth
 * the note. There, an arm's single colour test was its selector and could not
 * be broken without handing the decision to the other arm. Here each arm
 * carries TWO colour tests -- the 1st candle's and the 3rd's -- so breaking
 * either one leaves the other still holding the opposite arm false, and both
 * are ordinary flips. An arm selector is only unflippable when it is the arm's
 * only selector.
 *
 * THE PENETRATION ARM IS FUSED ON ONE SIDE AND NOT THE OTHER, the same split
 * unit 5 found: the bullish term2 is emitted as fma(rb, pen, close) and the
 * bearish one as close - rb * pen. The two boundary flips are written as the
 * arithmetic -- `104.0-4.0*0.3` and `100.0+4.0*0.3` -- rather than as decimals,
 * and the fused and two-step forms were measured equal at (4, 0.3, 104) rather
 * than assumed, so both land ON the boundary instead of near it.
 *
 * The geometry keeps all three averages exact. BodyLong's window at i-2 is all
 * primer, so it is the primer body 2. BodyDoji's at i-1 carries the first
 * candle's HighLow, so that is held at the primer's 10 and the doji threshold
 * is 1. BodyShort's at i carries the first two bodies, and 4 + 0 puts it on 2
 * exactly -- which is why the 1st candle is body 4 in a HighLow of 10 rather
 * than the longer body the pattern's name suggests.
 */
static void cond_abandonedbaby( int i, int *c )
{
   c[0] = pb_body(i-2) >  pb_avg(TA_BodyLong,  i-2);
   c[1] = pb_body(i-1) <= pb_avg(TA_BodyDoji,  i-1);
   c[2] = pb_body(i)   >  pb_avg(TA_BodyShort, i);
   c[3] = (  pb_white(i-2) && !pb_white(i) &&
             pbC[i] < pbC[i-2] - pb_body(i-2) * 0.3 &&
             pbL[i-1] > pbH[i-2] && pbH[i] < pbL[i-1] )
       || ( !pb_white(i-2) &&  pb_white(i) &&
             pbC[i] > pbC[i-2] + pb_body(i-2) * 0.3 &&
             pbH[i-1] < pbL[i-2] && pbL[i] > pbH[i-1] );
}
static void arm_abandonedbaby( int i, int cond, int arm, int *a )
{
   if( cond != 3 ) return;
   if( arm == 0 )
   {
      a[0] =  pb_white(i-2);
      a[1] = !pb_white(i);
      a[2] = pbC[i] < pbC[i-2] - pb_body(i-2) * 0.3;
      a[3] = pbL[i-1] > pbH[i-2];
      a[4] = pbH[i]   < pbL[i-1];
   }
   else
   {
      a[0] = !pb_white(i-2);
      a[1] =  pb_white(i);
      a[2] = pbC[i] > pbC[i-2] + pb_body(i-2) * 0.3;
      a[3] = pbH[i-1] < pbL[i-2];
      a[4] = pbL[i]   > pbH[i-1];
   }
}

static void build_abandonedbaby( void )
{
  pb_conditions(4);
  pb_signs(2);
  pb_arm(3,0,5); pb_arm(3,1,5);
  pb_arm_model(arm_abandonedbaby);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p1=pb_bar(106,107,99,100);
  pb_detect(p1,-100,"detect bearish: long white, doji abandoned above it, black 3rd gapping back down and closing past the penetration line");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,93,94);
  int p2=pb_bar(98,105,96,104);
  pb_detect(p2,100,"detect bullish: the mirror, firing the other output class");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,106,96,104);
  pb_bar(110,111,108,110);
  int p3=pb_bar(106,107,99,100);
  pb_flip(p3,0,"break c0: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(101,106,96,104);
  pb_bar(110,111,108,110);
  int p4=pb_bar(106,107,99,100);
  pb_control(p4,-100,0,"restore c0: 1st body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,112,108,111.5);
  int p5=pb_bar(106,107,99,100);
  pb_flip(p5,1,"break c1: doji body 1.5 above the BodyDoji average 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,112,108,111);
  int p6=pb_bar(106,107,99,100);
  pb_control(p6,-100,1,"restore c1: doji body 1 == avg 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p7=pb_bar(102,107,99,100);
  pb_flip(p7,2,"break c2: 3rd body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p8=pb_bar(103,107,99,100);
  pb_control(p8,-100,2,"restore c2: 3rd body 3 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(110,111,108,110);
  int p9=pb_bar(106,107,97,98);
  pb_flip_in(p9,3,0,0,"break c3 alt0 term0: 1st is black; the 3rd stays black, so the bullish alternative cannot take over");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p10=pb_bar(99,107,98,102);
  pb_flip_in(p10,3,0,1,"break c3 alt0 term1: 3rd is white; the 1st stays white, so the bullish alternative cannot take over");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p11=pb_bar(106,107,102,104.0-4.0*0.3);
  pb_flip_in(p11,3,0,2,"break c3 alt0 term2: 3rd closes 104 - 4*0.3 exactly, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,106,110);
  int p12=pb_bar(105,105,99,100);
  pb_flip_in(p12,3,0,3,"break c3 alt0 term3: doji low 106 == the 1st high, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p13=pb_bar(106,108,99,100);
  pb_flip_in(p13,3,0,4,"break c3 alt0 term4: 3rd high 108 == the doji low, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(94,95,93,94);
  int p14=pb_bar(98,107,96,106);
  pb_flip_in(p14,3,1,0,"break c3 alt1 term0: 1st is white; the 3rd stays white, so the bearish alternative cannot take over");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,93,94);
  int p15=pb_bar(105,106,96,102);
  pb_flip_in(p15,3,1,1,"break c3 alt1 term1: 3rd is black; the 1st stays black, so the bearish alternative cannot take over");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,93,94);
  int p16=pb_bar(98,105,96,100.0+4.0*0.3);
  pb_flip_in(p16,3,1,2,"break c3 alt1 term2: 3rd closes 100 + 4*0.3 exactly, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,96,93,94);
  int p17=pb_bar(98,105,97,104);
  pb_flip_in(p17,3,1,3,"break c3 alt1 term3: doji high 96 == the 1st low, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,93,94);
  int p18=pb_bar(98,105,95,104);
  pb_flip_in(p18,3,1,4,"break c3 alt1 term4: 3rd low 95 == the doji high, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,111,108,110);
  int p19=pb_bar(106,107,99,100);
  pb_control(p19,-100,3,"restore c3 via alt0: every term of the bearish alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,93,94);
  int p20=pb_bar(98,105,96,104);
  pb_control(p20,100,3,"restore c3 via alt1: every term of the bullish alternative holds");
  pb_flat(8);

}

/* ---- Hard tier: CDLMATHOLD ----------------------------------------------- *
 *
 * A long white candle, a gap, three small "reaction days" that give some of it
 * back without giving back too much, and a white 5th closing above all of
 * them. Sixteen conditions over five bars, a penetration parameter, and a
 * four-deep body cascade -- the combination CDLRISEFALL3METHODS and the
 * penetration patterns each had half of.
 *
 *   c0..c2   colours: 1st white, 2nd black, 5th white
 *   c3       the 2nd gaps up off the 1st
 *   c4,c5    the 3rd and 4th body floors stay below close(1st)
 *   c6,c7    ...and above close(1st) - realbody(1st) * penetration
 *   c8,c9    the 3rd's ceiling is under open(2nd), the 4th's under the 3rd's
 *   c10      the 5th opens above close(4th)
 *   c11      the 5th closes above every reaction-day high
 *   c12..c15 the 1st is long; the 2nd, 3rd and 4th are short
 *
 * THE REACTION DAYS LIVE IN A BAND WITH BOTH EDGES DERIVED FROM THE FIRST
 * CANDLE. c4/c6 and c5/c7 are two-sided: a body floor must sit below
 * close(1st) and above close(1st) - realbody(1st) * penetration. Both edges
 * move when the first candle's body moves, which is what makes c12's flip the
 * awkward one -- dropping that body to its BodyLong boundary of 2 raises the
 * penetration line from 106 to 111 and squeezes the band to (111, 112), so the
 * three small bodies have to be re-cut to fit inside a one-point corridor.
 *
 * c8 is the other one worth reading. Its boundary puts the 3rd's ceiling
 * exactly at open(2nd), while c4 keeps the 3rd's floor below close(1st) and
 * c14 keeps its body under 3 -- so open(2nd) can be at most close(1st) + 3, and
 * the 2nd candle has to be lowered from the detect's layout before the boundary
 * is reachable at all. Two conditions bounding one bar from opposite sides, with
 * a third bounding its size, is the shape to look for when a boundary seems
 * unreachable.
 *
 * The cascade is the familiar one: BodyLong at i-4 on an all-primer window,
 * then BodyShort at i-3, i-2 and i-1 each swallowing one more scenario body.
 * P=2 with bodies 12, 2, 2, 2 puts the four thresholds on 2, 3, 3, 3 exactly.
 * There is no body test on the 5th candle at all, which is why its body is free
 * to grow whenever a flip needs its close pushed past the reaction highs.
 */
static void cond_mathold( int i, int *c )
{
   double line = pbC[i-4] - pb_body(i-4) * 0.5;
   c[0]  = pb_white(i-4);
   c[1]  = !pb_white(i-3);
   c[2]  = pb_white(i);
   c[3]  = pb_bodylo(i-3) > pb_bodyhi(i-4);
   c[4]  = pb_bodylo(i-2) < pbC[i-4];
   c[5]  = pb_bodylo(i-1) < pbC[i-4];
   c[6]  = pb_bodylo(i-2) > line;
   c[7]  = pb_bodylo(i-1) > line;
   c[8]  = pb_bodyhi(i-2) < pbO[i-3];
   c[9]  = pb_bodyhi(i-1) < pb_bodyhi(i-2);
   c[10] = pbO[i] > pbC[i-1];
   c[11] = pbC[i] > (pbH[i-3] > pbH[i-2] ? (pbH[i-3] > pbH[i-1] ? pbH[i-3] : pbH[i-1])
                                         : (pbH[i-2] > pbH[i-1] ? pbH[i-2] : pbH[i-1]));
   c[12] = pb_body(i-4) > pb_avg(TA_BodyLong,  i-4);
   c[13] = pb_body(i-3) < pb_avg(TA_BodyShort, i-3);
   c[14] = pb_body(i-2) < pb_avg(TA_BodyShort, i-2);
   c[15] = pb_body(i-1) < pb_avg(TA_BodyShort, i-1);
}

static void build_mathold( void )
{
  pb_conditions(16);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m1=pb_bar(109,119,108,118);
  pb_detect(m1,100,"detect: long white, a gap, three small reaction days holding inside the first body, white 5th closing above them all");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(116,117,113,114);
  pb_bar(99,100,96,97);
  pb_bar(98,99,95,96);
  int m2=pb_bar(97,119,96,118);
  pb_flip(m2,0,"break c0: the 1st candle is black -- the whole layout drops with it, since close(1st) sets both the penetration line and the band the reaction days sit in");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m3=pb_bar(109,119,108,118);
  pb_control(m3,100,0,"restore c0: the 1st candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(114,117,113,116);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m4=pb_bar(109,119,108,118);
  pb_flip(m4,1,"break c1: the 2nd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m5=pb_bar(109,119,108,118);
  pb_control(m5,100,1,"restore c1: the 2nd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m6=pb_bar(120,121,108,118);
  pb_flip(m6,2,"break c2: the 5th candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m7=pb_bar(109,119,108,118);
  pb_control(m7,100,2,"restore c2: the 5th candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(114,117,112,112);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m8=pb_bar(109,119,108,118);
  pb_flip(m8,3,"break c3: 2nd body floor 112 == the 1st body ceiling 112, the gap test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m9=pb_bar(109,119,108,118);
  pb_control(m9,100,3,"restore c3: 2nd body floor 114 > 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(114,115,111,112);
  pb_bar(110,111,107,108);
  int m10=pb_bar(109,119,108,118);
  pb_flip(m10,4,"break c4: 3rd body floor 112 == close(1st), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m11=pb_bar(109,119,108,118);
  pb_control(m11,100,4,"restore c4: 3rd body floor 109 < close(1st) 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(118,119,113,116);
  pb_bar(113,114,110,111);
  pb_bar(112.5,113,111,112);
  int m12=pb_bar(113,121,112,120);
  pb_flip(m12,5,"break c5: 4th body floor 112 == close(1st), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(118,119,113,116);
  pb_bar(113,114,110,111);
  pb_bar(112.5,113,111,111.5);
  int m13=pb_bar(113,121,111,120);
  pb_control(m13,100,5,"restore c5: 4th body floor 111.5 < close(1st) 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(108,109,105,106);
  pb_bar(107.5,108,105,106.5);
  int m14=pb_bar(107,119,106,118);
  pb_flip(m14,6,"break c6: 3rd body floor 106 == the penetration line 112 - 12*0.5, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(108.5,109,105,106.5);
  pb_bar(107.5,108,105,106.5);
  int m15=pb_bar(107,119,106,118);
  pb_control(m15,100,6,"restore c6: 3rd body floor 106.5 > the penetration line 106");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(108,109,105,106);
  int m16=pb_bar(107,119,106,118);
  pb_flip(m16,7,"break c7: 4th body floor 106 == the penetration line, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m17=pb_bar(109,119,108,118);
  pb_control(m17,100,7,"restore c7: 4th body floor 108 > 106");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(114.5,117,112,112.5);
  pb_bar(114.5,115,111,111.75);
  pb_bar(110,111,107,108);
  int m18=pb_bar(109,119,108,118);
  pb_flip(m18,8,"break c8: 3rd body ceiling 114.5 == open(2nd), the test is strict -- the 2nd is lowered to make room, because c4 and c14 together cap open(2nd) at close(1st) + 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(114.5,117,112,112.5);
  pb_bar(114,115,111,111.25);
  pb_bar(110,111,107,108);
  int m19=pb_bar(109,119,107,118);
  pb_control(m19,100,8,"restore c8: 3rd body ceiling 114 < open(2nd) 114.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(111,112,108,109);
  int m20=pb_bar(110,119,108,118);
  pb_flip(m20,9,"break c9: 4th body ceiling 111 == the 3rd's, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m21=pb_bar(109,119,108,118);
  pb_control(m21,100,9,"restore c9: 4th body ceiling 110 < the 3rd's 111");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m22=pb_bar(108,119,107,118);
  pb_flip(m22,10,"break c10: 5th opens 108 == close(4th), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m23=pb_bar(109,119,108,118);
  pb_control(m23,100,10,"restore c10: 5th opens 109 > close(4th) 108");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m24=pb_bar(109,119,108,117);
  pb_flip(m24,11,"break c11: 5th closes 117 == the highest reaction-day high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m25=pb_bar(109,119,108,118);
  pb_control(m25,100,11,"restore c11: 5th closes 118 > the highest reaction-day high 117");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,113,99,112);
  pb_bar(115,117,113,114);
  pb_bar(112.5,113,111,111.5);
  pb_bar(112.25,113,111,111.25);
  int m26=pb_bar(112,119,111,118);
  pb_flip(m26,12,"break c12: 1st body 2 == avg 2, the test is strict -- the three small bodies shrink with it and the reaction band narrows to (111,112), because the penetration line rises with the shorter body");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(109,113,99,112);
  pb_bar(115,117,113,114);
  pb_bar(112.5,113,111,111.5);
  pb_bar(112.25,113,111,111.25);
  int m27=pb_bar(112,119,111,118);
  pb_control(m27,100,12,"restore c12: 1st body 3 > avg 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(117,118,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m28=pb_bar(109,120,108,119);
  pb_flip(m28,13,"break c13: 2nd body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m29=pb_bar(109,119,108,118);
  pb_control(m29,100,13,"restore c13: 2nd body 2 < avg 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(112,113,108,109);
  pb_bar(110,111,107,108);
  int m30=pb_bar(109,119,108,118);
  pb_flip(m30,14,"break c14: 3rd body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m31=pb_bar(109,119,108,118);
  pb_control(m31,100,14,"restore c14: 3rd body 2 < avg 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,106,107);
  int m32=pb_bar(109,119,106,118);
  pb_flip(m32,15,"break c15: 4th body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  pb_bar(111,112,108,109);
  pb_bar(110,111,107,108);
  int m33=pb_bar(109,119,108,118);
  pb_control(m33,100,15,"restore c15: 4th body 2 < avg 3");
  pb_flat(8);

}

/* ---- Hard tier: CDLSTALLEDPATTERN ---------------------------------------- *
 *
 * Three rising white candles where the third stalls -- small, riding the second
 * candle's shoulder. Twelve conditions, and the densest settings coupling in
 * the corpus: FOUR settings read at SIX places across three bars, on two window
 * lengths and two range types.
 *
 *   BodyLong  at i-2 (all primer) and at i-1 (carries the 1st body)
 *   BodyShort at i   (carries the 1st and 2nd bodies)
 *   ShadowVeryShort at i-1, HighLow-typed, carrying the 1st candle's range
 *   Near      at i-2 (all primer) and at i-1 (carries the 1st candle's range)
 *
 * The geometry has to satisfy all six at once. Holding HighLow(i-2) at the
 * primer's 10 fixes both Near reads and the ShadowVeryShort read; the two
 * RealBody cascades then want the two long bodies chosen so that (18 + A2)/10
 * and (16 + A2 + A1)/10 both land exactly, which 7 and 7 do -- giving 2, 2.5
 * and 3 for BodyLong(i-2), BodyLong(i-1) and BodyShort(i). A first body of 12,
 * the natural choice everywhere else in this file, is not available here: it
 * would need a HighLow above 10 and move both Near reads off their exact
 * values.
 *
 * TWO CONDITIONS ARE ENTAILED UNDER THE FILE'S USUAL PRIMER AND FLIPPABLE UNDER
 * ANOTHER, which is the same trap CDLADVANCEBLOCK's c0 carries and the reason
 * this builder runs two primers. c8 and c9 bound open(i-1) into
 * (open(i-2), close(i-2) + Near], so a black 1st candle needs its body under
 * Near while c5 needs it over BodyLong -- satisfiable only when Near exceeds
 * BodyLong, which pb_primer(12,100,2,4) does not do (both are 2). c1 is the
 * same argument one bar along, needing BodyLong(i-1) below Near. Both flips run
 * on hr=5, where Near is 2.4.
 *
 * c11 IS WAIVED, and its derivation does not depend on the primer. For a white
 * 3rd candle realbody(i) is close(i) - open(i), so
 *   open(i) >= close(i-1) - realbody(i) - Near
 * cancels to close(i) >= close(i-1) - Near. c3 already puts close(i) above
 * close(i-1), and Near cannot be negative -- it is a HighLow range times a
 * positive factor -- so c2 and c3 entail it for every valid input. Unlike a
 * threshold comparison, that is a settings reference the builder cannot vary:
 * it uses only the sign of Near, not its value.
 */
static void cond_stalledpattern( int i, int *c )
{
   c[0]  = pb_white(i-2);
   c[1]  = pb_white(i-1);
   c[2]  = pb_white(i);
   c[3]  = pbC[i]   > pbC[i-1];
   c[4]  = pbC[i-1] > pbC[i-2];
   c[5]  = pb_body(i-2) > pb_avg(TA_BodyLong, i-2);
   c[6]  = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[7]  = pb_upsh(i-1) < pb_avg(TA_ShadowVeryShort, i-1);
   c[8]  = pbO[i-1] > pbO[i-2];
   c[9]  = pbO[i-1] <= pbC[i-2] + pb_avg(TA_Near, i-2);
   c[10] = pb_body(i) < pb_avg(TA_BodyShort, i);
   c[11] = pbO[i] >= pbC[i-1] - pb_body(i) - pb_avg(TA_Near, i-1);
}

static void build_stalledpattern( void )
{
  pb_conditions(12);

  pb_waive(11, "for a white 3rd candle realbody(i) is close(i) - open(i), so the test reduces to close(i) >= close(i-1) - avg(Near, i-1); c3 already puts close(i) above close(i-1), and Near is a HighLow range times a positive factor and so is never negative. c2 and c3 therefore entail it for every valid input, whatever the primer -- a 200k-sample random search over all three bars found no case breaking it alone");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q1=pb_bar(111,114,110,113);
  pb_detect(q1,-100,"detect: three rising whites, the first two long with a very short 2nd upper shadow, a small 3rd riding the 2nd's shoulder");
  pb_flat(8);

  pb_primer(12,100,2,5);
  pb_bar(102.25,103,91,100);
  pb_bar(102.375,106.5,102,106);
  int q2=pb_bar(105,108,104,107);
  pb_flip(q2,0,"break c0: the 1st candle is black -- on hr=5, where Near 2.4 clears BodyLong 2 and a black 1st candle exists at all");
  pb_flat(8);

  pb_primer(12,100,2,5);
  pb_bar(100,103,91,102.25);
  pb_bar(102,106.5,101,106);
  int q3=pb_bar(105,108,104,107);
  pb_control(q3,-100,0,"restore c0: the 1st candle is white");
  pb_flat(8);

  pb_primer(12,100,2,5);
  pb_bar(100,103,91,102.25);
  pb_bar(104.5,105,102,102.4);
  int q4=pb_bar(101,104,100,103);
  pb_flip(q4,1,"break c1: the 2nd candle is black -- same hr=5 layout, with a 1st body short enough that BodyLong(i-1) stays under Near");
  pb_flat(8);

  pb_primer(12,100,2,5);
  pb_bar(100,103,91,102.25);
  pb_bar(102,106.5,101,106);
  int q5=pb_bar(105,108,104,107);
  pb_control(q5,-100,1,"restore c1: the 2nd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q6=pb_bar(114,115,112,113);
  pb_flip(q6,2,"break c2: the 3rd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q7=pb_bar(111,114,110,113);
  pb_control(q7,-100,2,"restore c2: the 3rd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q8=pb_bar(111,114,110,112);
  pb_flip(q8,3,"break c3: 3rd close 112 == the 2nd close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q9=pb_bar(111,114,110,113);
  pb_control(q9,-100,3,"restore c3: 3rd close 113 > the 2nd close 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,107.5,103,107);
  int q10=pb_bar(107,110,106,109);
  pb_flip(q10,4,"break c4: 2nd close 107 == the 1st close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(104,108.5,103,108);
  int q11=pb_bar(108,111,107,110);
  pb_control(q11,-100,4,"restore c4: 2nd close 108 > the 1st close 107");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,103,93,102);
  pb_bar(103,110.5,102,110);
  int q12=pb_bar(109,112,108,111);
  pb_flip(q12,5,"break c5: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,104,94,103);
  pb_bar(103,110.5,102,110);
  int q13=pb_bar(109,112,108,111);
  pb_control(q13,-100,5,"restore c5: 1st body 3 > avg 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,108,104,107.5);
  int q14=pb_bar(107,110,106,109);
  pb_flip(q14,6,"break c6: 2nd body 2.5 == avg 2.5, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,108.5,104,108);
  int q15=pb_bar(107,110,106,109);
  pb_control(q15,-100,6,"restore c6: 2nd body 3 > avg 2.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,113,104,112);
  int q16=pb_bar(111,114,110,113);
  pb_flip(q16,7,"break c7: 2nd upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q17=pb_bar(111,114,110,113);
  pb_control(q17,-100,7,"restore c7: 2nd upper shadow 0.5 < avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(100,112.5,99,112);
  int q18=pb_bar(111,114,110,113);
  pb_flip(q18,8,"break c8: 2nd opens 100 == the 1st open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q19=pb_bar(111,114,110,113);
  pb_control(q19,-100,8,"restore c8: 2nd opens 105 > the 1st open 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(109.5,113.5,108,113);
  int q20=pb_bar(113,116,112,115);
  pb_flip(q20,9,"break c9: 2nd opens 109.5, above close(1st) 107 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(109,113.5,108,113);
  int q21=pb_bar(113,116,112,115);
  pb_control(q21,-100,9,"restore c9: 2nd opens 109 == close(1st) 107 + Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q22=pb_bar(111,115,110,114);
  pb_flip(q22,10,"break c10: 3rd body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,108,98,107);
  pb_bar(105,112.5,104,112);
  int q23=pb_bar(111,114,110,113);
  pb_control(q23,-100,10,"restore c10: 3rd body 2 < avg 3");
  pb_flat(8);

}

/* ---- Hard tier: CDL3LINESTRIKE ------------------------------------------- *
 *
 * Three candles of one colour stepping in one direction, each opening near the
 * previous body, and a fourth of the opposite colour that swallows all three.
 * Eight conditions over FOUR bars, with c7 a two-way alternative of five terms
 * each.
 *
 *   c0,c1    the first three share a colour
 *   c2       the 4th is the opposite colour
 *   c3,c4    the 2nd opens within Near of the 1st body, either side
 *   c5,c6    the 3rd opens within Near of the 2nd body, either side
 *   c7       three-white || three-black, each of them
 *              term0  the 3rd candle's colour -- the arm selector
 *              term1  the 3rd close steps past the 2nd
 *              term2  the 2nd close steps past the 1st
 *              term3  the 4th opens past the 3rd close
 *              term4  the 4th closes past the 1st open
 *
 * ONE SETTING, READ TWICE, AND FOUR INCLUSIVE BOUNDS. Near is the only setting
 * here, at i-3 (all primer) and at i-2 (carrying the 1st candle's range), so
 * holding HighLow(i-3) at the primer's 10 puts both on 2 exactly. c3..c6 are
 * two bands spelled with >= and <=, so all four controls sit on their own edge
 * and all four flips strictly outside.
 *
 * c2 IS WAIVED AND c7 IS WHAT FORCES IT. Take the three-white alternative: it
 * puts open(i) above close(i-1), c0/c1 make all three the same colour so
 * close(i-1) > close(i-2) > close(i-3) >= open(i-3), and its last term puts
 * close(i) below open(i-3). Chain those and open(i) > close(i) -- the 4th is
 * black, which is exactly what c2 asks against a white 3rd. Price ordering
 * only: no threshold appears in it, so unlike CDLSTALLEDPATTERN's c0 no choice
 * of primer makes it flippable.
 *
 * BOTH ARM SELECTORS ARE WAIVED for a reason worth separating from
 * CDLBREAKAWAY's. There the selector was unflippable because it was the arm's
 * only one. Here it is worse: col(i-1) is read by c1 and c2 as well, so moving
 * it breaks three conditions at once and can never be the single false term
 * whatever the rest of the layout does.
 */
static void cond_3linestrike( int i, int *c )
{
   int k3 = pb_white(i-3) ? 1 : -1, k2 = pb_white(i-2) ? 1 : -1;
   int k1 = pb_white(i-1) ? 1 : -1, k0 = pb_white(i)   ? 1 : -1;
   double n3 = pb_avg(TA_Near, i-3), n2 = pb_avg(TA_Near, i-2);
   c[0] = k3 == k2;
   c[1] = k2 == k1;
   c[2] = k0 == -k1;
   c[3] = pbO[i-2] >= pb_bodylo(i-3) - n3;
   c[4] = pbO[i-2] <= pb_bodyhi(i-3) + n3;
   c[5] = pbO[i-1] >= pb_bodylo(i-2) - n2;
   c[6] = pbO[i-1] <= pb_bodyhi(i-2) + n2;
   c[7] = (  pb_white(i-1) && pbC[i-1] > pbC[i-2] && pbC[i-2] > pbC[i-3] &&
             pbO[i] > pbC[i-1] && pbC[i] < pbO[i-3] )
       || ( !pb_white(i-1) && pbC[i-1] < pbC[i-2] && pbC[i-2] < pbC[i-3] &&
             pbO[i] < pbC[i-1] && pbC[i] > pbO[i-3] );
}
static void arm_3linestrike( int i, int cond, int arm, int *a )
{
   if( cond != 7 ) return;
   if( arm == 0 )
   {
      a[0] = pb_white(i-1);
      a[1] = pbC[i-1] > pbC[i-2];  a[2] = pbC[i-2] > pbC[i-3];
      a[3] = pbO[i]   > pbC[i-1];  a[4] = pbC[i]   < pbO[i-3];
   }
   else
   {
      a[0] = !pb_white(i-1);
      a[1] = pbC[i-1] < pbC[i-2];  a[2] = pbC[i-2] < pbC[i-3];
      a[3] = pbO[i]   < pbC[i-1];  a[4] = pbC[i]   > pbO[i-3];
   }
}

static void build_3linestrike( void )
{
  pb_conditions(8);
  pb_signs(2);
  pb_arm(7,0,5); pb_arm(7,1,5);
  pb_arm_model(arm_3linestrike);

  pb_waive(2, "c7 forces it. Take the three-white alternative: it puts open(i) above close(i-1), and c0/c1 make all three the same colour, so close(i-1) > close(i-2) > close(i-3) >= open(i-3) and its last term puts close(i) below open(i-3). So open(i) > close(i) and the 4th candle is black, which is exactly what c2 asks of it against a white 3rd. The three-black alternative is the same derivation mirrored. Price ordering only -- no threshold is involved, so no primer changes it; a 200k-sample random search over all four bars found no case breaking it alone");
  pb_waive_arm(7,0,0,"the arm's own colour selector, and col(i-1) is read by c1 and c2 as well -- moving it breaks those too, so it can never be the single false term. The class it selects is fired by pb_signs(2)");
  pb_waive_arm(7,1,0,"the arm's own colour selector, and col(i-1) is read by c1 and c2 as well -- moving it breaks those too, so it can never be the single false term. The class it selects is fired by pb_signs(2)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r1=pb_bar(111,112,98,99);
  pb_detect(r1,100,"detect three white: three rising whites each opening near the previous body, a black 4th opening above and closing below the 1st open");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(101,102,96,97);
  pb_bar(98,99,93,94);
  int r2=pb_bar(93,106,92,105);
  pb_detect(r2,-100,"detect three black: the mirror, firing the other output class");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r3=pb_bar(111,112,98,99);
  pb_flip(r3,0,"break c0: the 1st candle is black -- its body edges are unchanged, so only the colour test moves");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r4=pb_bar(111,112,98,99);
  pb_control(r4,100,0,"restore c0: the 1st and 2nd are both white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(105,106,100,101);
  pb_bar(104,111,103,110);
  int r5=pb_bar(111,112,98,99);
  pb_flip(r5,1,"break c1: the 1st and 2nd are black and the 3rd white -- two candles move, because changing one would take c0 or c2 with it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r6=pb_bar(111,112,98,99);
  pb_control(r6,100,1,"restore c1: the 2nd and 3rd are both white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(97.5,108,96,107);
  pb_bar(106,111,105,110);
  int r7=pb_bar(111,112,98,99);
  pb_flip(r7,3,"break c3: 2nd opens 97.5, below the 1st body floor 100 - Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(98,108,97,107);
  pb_bar(106,111,105,110);
  int r8=pb_bar(111,112,98,99);
  pb_control(r8,100,3,"restore c3: 2nd opens 98 == 100 - Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(106.5,108,105,107);
  pb_bar(106,111,105,110);
  int r9=pb_bar(111,112,98,99);
  pb_flip(r9,4,"break c4: 2nd opens 106.5, above the 1st body ceiling 104 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(106,108,105,107);
  pb_bar(106,111,105,110);
  int r10=pb_bar(111,112,98,99);
  pb_control(r10,100,4,"restore c4: 2nd opens 106 == 104 + Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(100.5,111,99,110);
  int r11=pb_bar(111,112,98,99);
  pb_flip(r11,5,"break c5: 3rd opens 100.5, below the 2nd body floor 103 - Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(101,111,100,110);
  int r12=pb_bar(111,112,98,99);
  pb_control(r12,100,5,"restore c5: 3rd opens 101 == 103 - Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(109.5,111,108,110);
  int r13=pb_bar(111,112,98,99);
  pb_flip(r13,6,"break c6: 3rd opens 109.5, above the 2nd body ceiling 107 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(109,111,108,110);
  int r14=pb_bar(111,112,98,99);
  pb_control(r14,100,6,"restore c6: 3rd opens 109 == 107 + Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,107);
  int r15=pb_bar(111,112,98,99);
  pb_flip_in(r15,7,0,1,"break c7 alt0 term1: 3rd close 107 == the 2nd close, the rise is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,104);
  pb_bar(106,111,105,110);
  int r16=pb_bar(111,112,98,99);
  pb_flip_in(r16,7,0,2,"break c7 alt0 term2: 2nd close 104 == the 1st close, the rise is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r17=pb_bar(110,112,98,99);
  pb_flip_in(r17,7,0,3,"break c7 alt0 term3: 4th opens 110 == the 3rd close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r18=pb_bar(111,112,99,100);
  pb_flip_in(r18,7,0,4,"break c7 alt0 term4: 4th closes 100 == the 1st open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(101,102,96,97);
  pb_bar(98,99,96,97);
  int r19=pb_bar(93,106,92,105);
  pb_flip_in(r19,7,1,1,"break c7 alt1 term1: 3rd close 97 == the 2nd close, the fall is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(101,102,99,100);
  pb_bar(98,99,93,94);
  int r20=pb_bar(93,106,92,105);
  pb_flip_in(r20,7,1,2,"break c7 alt1 term2: 2nd close 100 == the 1st close, the fall is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(101,102,96,97);
  pb_bar(98,99,93,94);
  int r21=pb_bar(94,106,92,105);
  pb_flip_in(r21,7,1,3,"break c7 alt1 term3: 4th opens 94 == the 3rd close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(101,102,96,97);
  pb_bar(98,99,93,94);
  int r22=pb_bar(93,106,92,104);
  pb_flip_in(r22,7,1,4,"break c7 alt1 term4: 4th closes 104 == the 1st open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(103,108,102,107);
  pb_bar(106,111,105,110);
  int r23=pb_bar(111,112,98,99);
  pb_control(r23,100,7,"restore c7 via alt0: every term of the three-white alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(101,102,96,97);
  pb_bar(98,99,93,94);
  int r24=pb_bar(93,106,92,105);
  pb_control(r24,-100,7,"restore c7 via alt1: every term of the three-black alternative holds");
  pb_flat(8);

}

/* ---- Hard tier: CDLIDENTICAL3CROWS --------------------------------------- *
 *
 * Three declining black candles with no lower shadow, each opening at very
 * nearly the previous close. Twelve conditions over three bars, five settings
 * reads, and FOUR of the twelve inclusive -- two Equal bands, each spelled as a
 * <= and a >=, so four of the controls sit on their own edge.
 *
 *   c0,c2,c4   the three candles are black
 *   c1,c3,c5   each has a lower shadow under ShadowVeryShort
 *   c6,c7      the closes decline
 *   c8,c9      the 2nd opens within Equal of close(1st)
 *   c10,c11    the 3rd opens within Equal of close(2nd)
 *
 * NOTHING IS WAIVED HERE, which is unusual for a twelve-condition pattern and
 * worth knowing why: the three colour tests are INDEPENDENT. Unlike
 * CDL3LINESTRIKE, where c0..c2 compare adjacent colours and each candle sits in
 * two of them, these each assert one candle's colour on its own. So each flips
 * by recolouring one bar, and none is entangled with its neighbours.
 *
 * The geometry is the easiest in the hard tier once one thing is seen: holding
 * HighLow at 10 on the 1st and 2nd candles puts ALL FIVE reads on exact values
 * -- ShadowVeryShort 1 at each of the three bars, Equal 0.5 at both. The bodies
 * are then free, because no body test exists in this pattern at all. Bars with
 * the low ON the close (lower shadow 0) satisfy c1/c3/c5 with room to spare and
 * leave the high as the only degree of freedom needed to hold HighLow at 10.
 *
 * c0's flip is the one that moves three bars rather than one. Recolouring the
 * 1st candle moves close(1st), which is the centre of the band c8/c9 hold
 * open(2nd) in -- so the 2nd has to follow it, and the 3rd follows the 2nd
 * through c10/c11. The colour tests are independent of each other; they are not
 * independent of the bands.
 */
static void cond_identical3crows( int i, int *c )
{
   c[0]  = !pb_white(i-2);
   c[1]  = pb_losh(i-2) < pb_avg(TA_ShadowVeryShort, i-2);
   c[2]  = !pb_white(i-1);
   c[3]  = pb_losh(i-1) < pb_avg(TA_ShadowVeryShort, i-1);
   c[4]  = !pb_white(i);
   c[5]  = pb_losh(i)   < pb_avg(TA_ShadowVeryShort, i);
   c[6]  = pbC[i-2] > pbC[i-1];
   c[7]  = pbC[i-1] > pbC[i];
   c[8]  = pbO[i-1] <= pbC[i-2] + pb_avg(TA_Equal, i-2);
   c[9]  = pbO[i-1] >= pbC[i-2] - pb_avg(TA_Equal, i-2);
   c[10] = pbO[i]   <= pbC[i-1] + pb_avg(TA_Equal, i-1);
   c[11] = pbO[i]   >= pbC[i-1] - pb_avg(TA_Equal, i-1);
}

static void build_identical3crows( void )
{
  pb_conditions(12);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u1=pb_bar(99,104,94,94);
  pb_detect(u1,-100,"detect: three black candles, each with no lower shadow, declining, each opening within Equal of the previous close");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,114,104,110);
  pb_bar(110,115,105,105);
  int u2=pb_bar(105,110,100,100);
  pb_flip(u2,0,"break c0: the 1st candle is white -- the other two move with it, because close(1st) is the centre of the band c8/c9 hold open(2nd) in");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u3=pb_bar(99,104,94,94);
  pb_control(u3,-100,0,"restore c0: the 1st candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,113,103,104);
  pb_bar(104,109,99,99);
  int u4=pb_bar(99,104,94,94);
  pb_flip(u4,1,"break c1: 1st lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u5=pb_bar(99,104,94,94);
  pb_control(u5,-100,1,"restore c1: 1st lower shadow 0 < avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(103.5,113,103,103.75);
  int u6=pb_bar(103.5,108.5,98.5,98.5);
  pb_flip(u6,2,"break c2: the 2nd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u7=pb_bar(99,104,94,94);
  pb_control(u7,-100,2,"restore c2: the 2nd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,108,98,99);
  int u8=pb_bar(99,104,94,94);
  pb_flip(u8,3,"break c3: 2nd lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u9=pb_bar(99,104,94,94);
  pb_control(u9,-100,3,"restore c3: 2nd lower shadow 0 < avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u10=pb_bar(98.5,108,98,98.75);
  pb_flip(u10,4,"break c4: the 3rd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u11=pb_bar(99,104,94,94);
  pb_control(u11,-100,4,"restore c4: the 3rd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u12=pb_bar(99,103,93,94);
  pb_flip(u12,5,"break c5: 3rd lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u13=pb_bar(99,104,94,94);
  pb_control(u13,-100,5,"restore c5: 3rd lower shadow 0 < avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104.5,114,104,104);
  int u14=pb_bar(104,109,99,99);
  pb_flip(u14,6,"break c6: 2nd close 104 == the 1st close, the decline is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104.5,114,103.5,103.5);
  int u15=pb_bar(103.5,109,99,99);
  pb_control(u15,-100,6,"restore c6: 2nd close 103.5 < the 1st close 104");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u16=pb_bar(99.5,109,99,99);
  pb_flip(u16,7,"break c7: 3rd close 99 == the 2nd close, the decline is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u17=pb_bar(99.5,109,98.5,98.5);
  pb_control(u17,-100,7,"restore c7: 3rd close 98.5 < the 2nd close 99");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(105,109,99,99);
  int u18=pb_bar(99,104,94,94);
  pb_flip(u18,8,"break c8: 2nd opens 105, above close(1st) 104 + Equal 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104.5,109,99,99);
  int u19=pb_bar(99,104,94,94);
  pb_control(u19,-100,8,"restore c8: 2nd opens 104.5 == 104 + Equal 0.5, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(103,109,99,99);
  int u20=pb_bar(99,104,94,94);
  pb_flip(u20,9,"break c9: 2nd opens 103, below close(1st) 104 - Equal 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(103.5,109,99,99);
  int u21=pb_bar(99,104,94,94);
  pb_control(u21,-100,9,"restore c9: 2nd opens 103.5 == 104 - Equal 0.5, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u22=pb_bar(100,104,94,94);
  pb_flip(u22,10,"break c10: 3rd opens 100, above close(2nd) 99 + Equal 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u23=pb_bar(99.5,104,94,94);
  pb_control(u23,-100,10,"restore c10: 3rd opens 99.5 == 99 + Equal 0.5, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u24=pb_bar(98,104,94,94);
  pb_flip(u24,11,"break c11: 3rd opens 98, below close(2nd) 99 - Equal 0.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,114,104,104);
  pb_bar(104,109,99,99);
  int u25=pb_bar(98.5,104,94,94);
  pb_control(u25,-100,11,"restore c11: 3rd opens 98.5 == 99 - Equal 0.5, inclusive");
  pb_flat(8);

}

/* ---- Hard tier: CDLLADDERBOTTOM ------------------------------------------ *
 *
 * Three black candles stepping down, a fourth black one with a long upper
 * shadow, and a white fifth that closes above that shadow. Twelve conditions
 * over five bars and only ONE settings read -- ShadowVeryShort at i-1 -- which
 * makes this the least settings-coupled pattern in the hard tier and the
 * geometry correspondingly free.
 *
 * That one read still spans three of the five bars: its window ends at i-1, so
 * it carries the 1st, 2nd and 3rd candles. Giving each of those a HighLow of 10
 * puts it on 1 exactly, and every flip that moves one of those bodies keeps
 * that HighLow so the threshold does not drift underneath it.
 *
 * c8 IS THE ONE READ THAT POINTS THE OTHER WAY. Everywhere else in this file a
 * ShadowVeryShort test asks for a shadow UNDER the average; here the 4th
 * candle's upper shadow must EXCEED it. The boundary is the same equality and
 * the flip sits on it, but the control is above rather than below -- worth
 * checking the direction before copying a shadow scenario from another builder.
 *
 * c1's flip is the only one that re-cuts the whole ladder. A white 2nd candle
 * needs open(2nd) below close(2nd), while c4 puts open(2nd) above open(3rd) and
 * c5 puts close(2nd) below close(1st) -- so the four prices have to interleave
 * as open(3rd) < open(2nd) < close(2nd) < close(1st), which the detect's layout
 * does not leave room for. Widening the 1st candle and dropping the 3rd is what
 * makes the gap. c0 and c2, the other two colour tests, each move one bar: the
 * outer two are not squeezed from both sides the way the middle one is.
 */
static void cond_ladderbottom( int i, int *c )
{
   c[0]  = !pb_white(i-4);
   c[1]  = !pb_white(i-3);
   c[2]  = !pb_white(i-2);
   c[3]  = pbO[i-4] > pbO[i-3];
   c[4]  = pbO[i-3] > pbO[i-2];
   c[5]  = pbC[i-4] > pbC[i-3];
   c[6]  = pbC[i-3] > pbC[i-2];
   c[7]  = !pb_white(i-1);
   c[8]  = pb_upsh(i-1) > pb_avg(TA_ShadowVeryShort, i-1);
   c[9]  = pb_white(i);
   c[10] = pbO[i] > pbO[i-1];
   c[11] = pbC[i] > pbH[i-1];
}

static void build_ladderbottom( void )
{
  pb_conditions(12);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v1=pb_bar(113,117,112,116);
  pb_detect(v1,100,"detect: three black candles with descending opens and closes, a black 4th with a long upper shadow, a white 5th closing above that shadow");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(119,121,111,120);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v2=pb_bar(113,117,112,116);
  pb_flip(v2,0,"break c0: the 1st candle is white -- its open still leads the 2nd's, so only the colour test moves");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v3=pb_bar(113,117,112,116);
  pb_control(v3,100,0,"restore c0: the 1st candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(125,126,116,120);
  pb_bar(115,119,109,118);
  pb_bar(114,115,105,110);
  pb_bar(108,110,103,104);
  int v4=pb_bar(109,113,108,112);
  pb_flip(v4,1,"break c1: the 2nd candle is white -- the whole ladder is re-cut, because a white 2nd needs open(2nd) below close(2nd) while c4 puts it above open(3rd) and c5 puts close(2nd) below close(1st)");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(125,126,116,120);
  pb_bar(118,119,109,115);
  pb_bar(114,115,105,110);
  pb_bar(108,110,103,104);
  int v5=pb_bar(109,113,108,112);
  pb_control(v5,100,1,"restore c1: the 2nd candle is black, same layout");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(110,113,103,112);
  pb_bar(112,114,107,108);
  int v6=pb_bar(113,117,112,116);
  pb_flip(v6,2,"break c2: the 3rd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v7=pb_bar(113,117,112,116);
  pb_control(v7,100,2,"restore c2: the 3rd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(118,119,109,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v8=pb_bar(113,117,112,116);
  pb_flip(v8,3,"break c3: 1st opens 118 == the 2nd open, the descent is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(119,120,110,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v9=pb_bar(113,117,112,116);
  pb_control(v9,100,3,"restore c3: 1st opens 119 > the 2nd open 118");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(116,117,107,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v10=pb_bar(113,117,112,116);
  pb_flip(v10,4,"break c4: 2nd opens 116 == the 3rd open, the descent is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v11=pb_bar(113,117,112,116);
  pb_control(v11,100,4,"restore c4: 2nd opens 118 > the 3rd open 116");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,113);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v12=pb_bar(113,117,112,116);
  pb_flip(v12,5,"break c5: 1st closes 113 == the 2nd close, the descent is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,114);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v13=pb_bar(113,117,112,116);
  pb_control(v13,100,5,"restore c5: 1st closes 114 > the 2nd close 113");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,111);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v14=pb_bar(113,117,112,116);
  pb_flip(v14,6,"break c6: 2nd closes 111 == the 3rd close, the descent is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v15=pb_bar(113,117,112,116);
  pb_control(v15,100,6,"restore c6: 2nd closes 113 > the 3rd close 111");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(108,114,107,112);
  int v16=pb_bar(113,117,112,116);
  pb_flip(v16,7,"break c7: the 4th candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v17=pb_bar(113,117,112,116);
  pb_control(v17,100,7,"restore c7: the 4th candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,113,107,108);
  int v18=pb_bar(113,117,112,116);
  pb_flip(v18,8,"break c8: 4th upper shadow 1 == avg 1, and this test is strict in the OTHER direction -- the shadow must exceed the average, not fall under it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v19=pb_bar(113,117,112,116);
  pb_control(v19,100,8,"restore c8: 4th upper shadow 2 > avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v20=pb_bar(117,118,114,115);
  pb_flip(v20,9,"break c9: the 5th candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v21=pb_bar(113,117,112,116);
  pb_control(v21,100,9,"restore c9: the 5th candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v22=pb_bar(112,117,111,116);
  pb_flip(v22,10,"break c10: 5th opens 112 == the 4th open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v23=pb_bar(113,117,112,116);
  pb_control(v23,100,10,"restore c10: 5th opens 113 > the 4th open 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v24=pb_bar(113,115,112,114);
  pb_flip(v24,11,"break c11: 5th closes 114 == the 4th high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(120,121,111,115);
  pb_bar(118,119,109,113);
  pb_bar(116,117,107,111);
  pb_bar(112,114,107,108);
  int v25=pb_bar(113,117,112,116);
  pb_control(v25,100,11,"restore c11: 5th closes 116 > the 4th high 114");
  pb_flat(8);

}

/* ---- Hard tier: CDLUNIQUE3RIVER ------------------------------------------ *
 *
 * A long black candle, a black harami making a lower low, and a short white
 * third opening above that low. Nine conditions over three bars with two
 * settings reads -- BodyLong at i-2 on an all-primer window and BodyShort at i
 * carrying the first two bodies -- so 12 and 2 put them on 2 and 3 exactly.
 *
 * c0 IS WAIVED and the derivation needs no threshold. Were the 1st candle
 * white, open(1st) <= close(1st); c4 puts open(2nd) at or below open(1st) and
 * c3 puts close(2nd) above close(1st), so
 *   open(2nd) <= open(1st) <= close(1st) < close(2nd)
 * and the 2nd candle is white -- which c1 forbids. Price ordering only, so
 * unlike CDLSTALLEDPATTERN's c0 no choice of primer makes this one flippable.
 *
 * c7's flip is the one that moves a second bar, and for a reason worth naming
 * because it is the mirror of the usual cascade problem. Shrinking the 1st body
 * to its BodyLong boundary does not lower a threshold here -- it RAISES
 * open(1st), because the close is what stays fixed. c4 holds open(2nd) at or
 * below open(1st), so the 2nd candle has to come down to follow it, and the
 * BodyShort window at i then carries two smaller bodies and drops from 3 to
 * 1.9. Both effects come from one bar's body moving, in opposite directions.
 */
static void cond_unique3river( int i, int *c )
{
   c[0] = !pb_white(i-2);
   c[1] = !pb_white(i-1);
   c[2] = pb_white(i);
   c[3] = pbC[i-1] > pbC[i-2];
   c[4] = pbO[i-1] <= pbO[i-2];
   c[5] = pbL[i-1] < pbL[i-2];
   c[6] = pbO[i]   > pbL[i-1];
   c[7] = pb_body(i-2) > pb_avg(TA_BodyLong,  i-2);
   c[8] = pb_body(i)   < pb_avg(TA_BodyShort, i);
}

static void build_unique3river( void )
{
  pb_conditions(9);

  pb_waive(0, "were the 1st candle white, open(1st) <= close(1st); c4 puts open(2nd) at or below open(1st) and c3 puts close(2nd) above close(1st), so open(2nd) <= open(1st) <= close(1st) < close(2nd) and the 2nd candle is white -- which c1 forbids. So c1, c3 and c4 entail a black 1st candle. Price ordering only, with no threshold in it, so no choice of primer makes it flippable; a 300k-sample random search over all three bars found no case breaking it alone");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y1=pb_bar(105,108,104,107);
  pb_detect(y1,100,"detect: a long black candle, a black harami with a lower low, and a short white 3rd opening above that low");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(101,111,97,102);
  int y2=pb_bar(105,108,104,107);
  pb_flip(y2,1,"break c1: the 2nd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y3=pb_bar(105,108,104,107);
  pb_control(y3,100,1,"restore c1: the 2nd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y4=pb_bar(107,108,104,105);
  pb_flip(y4,2,"break c2: the 3rd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y5=pb_bar(105,108,104,107);
  pb_control(y5,100,2,"restore c2: the 3rd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,100);
  int y6=pb_bar(105,108,104,107);
  pb_flip(y6,3,"break c3: 2nd closes 100 == the 1st close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,101);
  int y7=pb_bar(105,108,104,107);
  pb_control(y7,100,3,"restore c3: 2nd closes 101 > the 1st close 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(112.5,113,97,108);
  int y8=pb_bar(105,108,104,107);
  pb_flip(y8,4,"break c4: 2nd opens 112.5, above the 1st open 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(112,113,97,108);
  int y9=pb_bar(105,108,104,107);
  pb_control(y9,100,4,"restore c4: 2nd opens 112 == the 1st open, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,98,108);
  int y10=pb_bar(105,108,104,107);
  pb_flip(y10,5,"break c5: 2nd low 98 == the 1st low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97.5,108);
  int y11=pb_bar(105,108,104,107);
  pb_control(y11,100,5,"restore c5: 2nd low 97.5 < the 1st low 98");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y12=pb_bar(97,108,96,99);
  pb_flip(y12,6,"break c6: 3rd opens 97 == the 2nd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y13=pb_bar(97.5,108,96,99);
  pb_control(y13,100,6,"restore c6: 3rd opens 97.5 > the 2nd low 97");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,113,98,100);
  pb_bar(102,111,97,101);
  int y14=pb_bar(105,108,104,106.5);
  pb_flip(y14,7,"break c7: 1st body 2 == avg 2, the test is strict -- the 2nd has to come down with it, because c4 holds open(2nd) under open(1st) and shrinking the 1st body raises that open");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(103,113,98,100);
  pb_bar(102,111,97,101);
  int y15=pb_bar(105,108,104,106.5);
  pb_control(y15,100,7,"restore c7: 1st body 3 > avg 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y16=pb_bar(105,109,104,108);
  pb_flip(y16,8,"break c8: 3rd body 3 == avg 3, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,98,100);
  pb_bar(110,111,97,108);
  int y17=pb_bar(105,109,104,107.5);
  pb_control(y17,100,8,"restore c8: 3rd body 2.5 < avg 3");
  pb_flat(8);

}

/* ---- Hard tier: CDLUPSIDEGAP2CROWS --------------------------------------- *
 *
 * A long white candle, a short black one gapping above it, and a black third
 * that engulfs the second's body while still closing above the first. Nine
 * conditions over three bars, two settings reads -- BodyLong at i-2 on an
 * all-primer window, BodyShort at i-1 carrying the first body -- so a first
 * body of 12 puts them on 2 and 3 exactly.
 *
 * TWO OF THE NINE ARE ENTAILED, AND ONE OF THEM IS THE GAP. That is the part
 * worth reading, because a gap test is the last thing that looks redundant.
 *
 *   c4 (the 2nd gaps above the 1st). c0 makes the 1st white, so its body
 *   ceiling is close(1st); c2 makes the 2nd black, so its body floor is
 *   close(2nd). c8 puts close(3rd) above close(1st) and c7 puts it below
 *   close(2nd). Chain those: close(2nd) > close(3rd) > close(1st) -- which IS
 *   the gap. The two conditions describing where the THIRD candle closes pin
 *   the relationship between the first two.
 *
 *   c5 (the 3rd is black). c2 gives open(2nd) > close(2nd), c6 puts open(3rd)
 *   above open(2nd) and c7 puts close(3rd) below close(2nd), so
 *   open(3rd) > open(2nd) > close(2nd) > close(3rd).
 *
 * Both derivations are price ordering with no threshold in them, so no choice
 * of primer makes either flippable -- the same class as CDL3LINESTRIKE's c2 and
 * CDLUNIQUE3RIVER's c0, rather than CDLSTALLEDPATTERN's primer-dependent pair.
 * A 300k-sample random search over all three bars found no case breaking either
 * one alone.
 */
static void cond_upsidegap2crows( int i, int *c )
{
   c[0] = pb_white(i-2);
   c[1] = pb_body(i-2) > pb_avg(TA_BodyLong,  i-2);
   c[2] = !pb_white(i-1);
   c[3] = pb_body(i-1) <= pb_avg(TA_BodyShort, i-1);
   c[4] = pb_bodylo(i-1) > pb_bodyhi(i-2);
   c[5] = !pb_white(i);
   c[6] = pbO[i] > pbO[i-1];
   c[7] = pbC[i] < pbC[i-1];
   c[8] = pbC[i] > pbC[i-2];
}

static void build_upsidegap2crows( void )
{
  pb_conditions(9);

  pb_waive(4, "c0 makes the 1st candle white, so its body ceiling is close(1st); c2 makes the 2nd black, so its body floor is close(2nd). c8 puts close(3rd) above close(1st) and c7 puts it below close(2nd), so close(2nd) > close(3rd) > close(1st) -- which IS the gap this condition tests. c0, c2, c7 and c8 therefore entail it. Price ordering only; a 300k-sample random search found no case breaking it alone");
  pb_waive(5, "c2 makes the 2nd candle black, so open(2nd) > close(2nd); c6 puts open(3rd) above open(2nd) and c7 puts close(3rd) below close(2nd), so open(3rd) > open(2nd) > close(2nd) > close(3rd) and the 3rd candle is black by construction. Price ordering only; same 300k-sample search found no case breaking it alone");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z1=pb_bar(117,118,112,113);
  pb_detect(z1,-100,"detect: a long white candle, a short black one gapping above it, and a black 3rd engulfing that body while closing above the 1st");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  pb_bar(116,117,113,114);
  int z2=pb_bar(117,118,112,113);
  pb_flip(z2,0,"break c0: the 1st candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z3=pb_bar(117,118,112,113);
  pb_control(z3,-100,0,"restore c0: the 1st candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,113,99,112);
  pb_bar(116,117,113,114);
  int z4=pb_bar(117,118,112,113);
  pb_flip(z4,1,"break c1: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(109,113,99,112);
  pb_bar(116,117,113,114);
  int z5=pb_bar(117,118,112,113);
  pb_control(z5,-100,1,"restore c1: 1st body 3 > avg 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(114,117,113,116);
  int z6=pb_bar(117,118,112,113);
  pb_flip(z6,2,"break c2: the 2nd candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z7=pb_bar(117,118,112,113);
  pb_control(z7,-100,2,"restore c2: the 2nd candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(117,118,113,113.5);
  int z8=pb_bar(118,119,112,113);
  pb_flip(z8,3,"break c3: 2nd body 3.5 above the BodyShort average 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(117,118,113,114);
  int z9=pb_bar(118,119,112,113);
  pb_control(z9,-100,3,"restore c3: 2nd body 3 == avg 3, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z10=pb_bar(116,118,112,113);
  pb_flip(z10,6,"break c6: 3rd opens 116 == the 2nd open, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z11=pb_bar(116.5,118,112,113);
  pb_control(z11,-100,6,"restore c6: 3rd opens 116.5 > the 2nd open 116");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z12=pb_bar(117,118,112,114);
  pb_flip(z12,7,"break c7: 3rd closes 114 == the 2nd close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z13=pb_bar(117,118,112,113.5);
  pb_control(z13,-100,7,"restore c7: 3rd closes 113.5 < the 2nd close 114");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z14=pb_bar(117,118,111,112);
  pb_flip(z14,8,"break c8: 3rd closes 112 == the 1st close, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  pb_bar(116,117,113,114);
  int z15=pb_bar(117,118,112,112.5);
  pb_control(z15,-100,8,"restore c8: 3rd closes 112.5 > the 1st close 112");
  pb_flat(8);

}

/* ---- Hard tier: CDLTASUKIGAP --------------------------------------------- *
 *
 * A gap, a candle that opens inside the second body and closes back into the
 * gap without filling it. The decision is a TOP-LEVEL disjunction -- upside gap
 * or downside gap -- which is why check_mcdc_conditions.py used to decline on
 * it and why it sat blocked while the rest of the tier was built.
 *
 * IT NEEDED NO NEW AXIS. A decision that is itself a disjunction is ONE
 * condition whose alternatives are conjunctions, which is exactly what pb_arm()
 * declares, pb_flip_in() attributes a case to, and pb_signs() covers the
 * selection of where the arms are colour-gated. The only change was letting the
 * static count through: a top-level `||` now reads as one condition with its
 * arms, the same reading a nested one already got. What looked like a missing
 * branch mechanism was the arm mechanism one level up.
 *
 * SIX OF THE SIXTEEN TERMS ARE ENTAILED, three per arm, and every derivation
 * comes from two other terms of the SAME arm:
 *
 *   term1 (the 2nd candle's colour) from terms 3 and 4. They put open(3rd)
 *   below close(2nd) and above open(2nd), so close(2nd) > open(2nd).
 *
 *   term2 (the 3rd candle's colour) from terms 4 and 5. open(3rd) is above
 *   open(2nd) and close(3rd) below it, so open(3rd) > close(3rd).
 *
 *   term0 (the gap itself) from terms 5 and 6. With term1 fixing the 2nd
 *   candle's colour, its body floor is open(2nd); term 5 puts close(3rd) below
 *   that and term 6 puts it above the 1st candle's body ceiling, so
 *   open(2nd) > close(3rd) > ceiling(1st) -- which IS the gap.
 *
 * That last one is the same shape CDLUPSIDEGAP2CROWS turned up: the gap between
 * the first two candles pinned by where the third one closes. Two patterns now,
 * so it is worth checking rather than assuming a gap test is independent.
 *
 * Every derivation is price ordering with no threshold in it, so no choice of
 * primer makes any of them flippable. A 120k-sample random search per term
 * found no case breaking any of the six alone.
 */
static void cond_tasukigap( int i, int *c )
{
   double n = pb_avg(TA_Near, i-1);
   c[0] = (  pb_bodylo(i-1) > pb_bodyhi(i-2) && pb_white(i-1) && !pb_white(i) &&
             pbO[i] < pbC[i-1] && pbO[i] > pbO[i-1] &&
             pbC[i] < pbO[i-1] && pbC[i] > pb_bodyhi(i-2) &&
             pb_abs(pb_body(i-1) - pb_body(i)) < n )
       || (  pb_bodyhi(i-1) < pb_bodylo(i-2) && !pb_white(i-1) && pb_white(i) &&
             pbO[i] < pbO[i-1] && pbO[i] > pbC[i-1] &&
             pbC[i] > pbO[i-1] && pbC[i] < pb_bodylo(i-2) &&
             pb_abs(pb_body(i-1) - pb_body(i)) < n );
}
static void arm_tasukigap( int i, int cond, int arm, int *a )
{
   double n = pb_avg(TA_Near, i-1);
   if( cond != 0 ) return;
   if( arm == 0 )
   {
      a[0] = pb_bodylo(i-1) > pb_bodyhi(i-2);
      a[1] = pb_white(i-1);   a[2] = !pb_white(i);
      a[3] = pbO[i] < pbC[i-1];  a[4] = pbO[i] > pbO[i-1];
      a[5] = pbC[i] < pbO[i-1];  a[6] = pbC[i] > pb_bodyhi(i-2);
      a[7] = pb_abs(pb_body(i-1) - pb_body(i)) < n;
   }
   else
   {
      a[0] = pb_bodyhi(i-1) < pb_bodylo(i-2);
      a[1] = !pb_white(i-1);  a[2] = pb_white(i);
      a[3] = pbO[i] < pbO[i-1];  a[4] = pbO[i] > pbC[i-1];
      a[5] = pbC[i] > pbO[i-1];  a[6] = pbC[i] < pb_bodylo(i-2);
      a[7] = pb_abs(pb_body(i-1) - pb_body(i)) < n;
   }
}

static void build_tasukigap( void )
{
  pb_conditions(1);
  pb_signs(2);
  pb_arm(0,0,8); pb_arm(0,1,8);
  pb_arm_model(arm_tasukigap);

  pb_waive_arm(0,0,0,"entailed by terms 5 and 6. Term 1 makes the 2nd candle white, so its body floor is open(2nd); term 5 puts close(3rd) below open(2nd) and term 6 puts it above the 1st candle's body ceiling, so open(2nd) > close(3rd) > ceiling(1st) -- which IS this gap");
  pb_waive_arm(0,0,1,"entailed by terms 3 and 4: term 3 puts open(3rd) below close(2nd) and term 4 puts it above open(2nd), so close(2nd) > open(2nd) and the 2nd candle is white");
  pb_waive_arm(0,0,2,"entailed by terms 4 and 5: open(3rd) is above open(2nd) and close(3rd) is below it, so open(3rd) > close(3rd) and the 3rd candle is black");
  pb_waive_arm(0,1,0,"entailed by terms 5 and 6, mirrored. Term 1 makes the 2nd candle black, so its body ceiling is open(2nd); term 5 puts close(3rd) above open(2nd) and term 6 puts it below the 1st candle's body floor, so open(2nd) < close(3rd) < floor(1st)");
  pb_waive_arm(0,1,1,"entailed by terms 3 and 4, mirrored: open(3rd) is below open(2nd) and above close(2nd), so open(2nd) > close(2nd) and the 2nd candle is black");
  pb_waive_arm(0,1,2,"entailed by terms 4 and 5, mirrored: open(3rd) is above close(2nd) and close(3rd) is above open(2nd) with open(3rd) below open(2nd), so close(3rd) > open(3rd)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t1=pb_bar(114,115,107,108);
  pb_detect(t1,100,"detect upside gap: a white 2nd gapping above the 1st, and a black 3rd opening inside it and closing back into the gap");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t2=pb_bar(90,97,89,96);
  pb_detect(t2,-100,"detect downside gap: the mirror, firing the other output class");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t3=pb_bar(116,117,108,109);
  pb_flip_in(t3,0,0,3,"break c0 alt0 term3: 3rd opens 116 == close(2nd), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t4=pb_bar(110,111,104,105);
  pb_flip_in(t4,0,0,4,"break c0 alt0 term4: 3rd opens 110 == open(2nd), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t5=pb_bar(115,116,109,110);
  pb_flip_in(t5,0,0,5,"break c0 alt0 term5: 3rd closes 110 == open(2nd), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t6=pb_bar(111,112,103,104);
  pb_flip_in(t6,0,0,6,"break c0 alt0 term6: 3rd closes 104 == the 1st body ceiling, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t7=pb_bar(114,115,105,106);
  pb_flip_in(t7,0,0,7,"break c0 alt0 term7: the two bodies differ by 2 == Near 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t8=pb_bar(94,100,93,99);
  pb_flip_in(t8,0,1,3,"break c0 alt1 term3: 3rd opens 94 == open(2nd), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t9=pb_bar(88,96,87,95);
  pb_flip_in(t9,0,1,4,"break c0 alt1 term4: 3rd opens 88 == close(2nd), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t10=pb_bar(89,95,88,94);
  pb_flip_in(t10,0,1,5,"break c0 alt1 term5: 3rd closes 94 == open(2nd), the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t11=pb_bar(93,101,92,100);
  pb_flip_in(t11,0,1,6,"break c0 alt1 term6: 3rd closes 100 == the 1st body floor, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t12=pb_bar(90,99,89,98);
  pb_flip_in(t12,0,1,7,"break c0 alt1 term7: the two bodies differ by 2 == Near 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,106,96,104);
  pb_bar(110,117,109,116);
  int t13=pb_bar(114,115,107,108);
  pb_control(t13,100,0,"restore c0 via alt0: every term of the upside-gap alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(104,106,96,100);
  pb_bar(94,95,87,88);
  int t14=pb_bar(90,97,89,96);
  pb_control(t14,-100,0,"restore c0 via alt1: every term of the downside-gap alternative holds");
  pb_flat(8);

}

/* ---- Hard tier: CDLENGULFING -------------------------------------------- *
 *
 * A candle whose body swallows the previous one. #219 listed it as needing a
 * decision rather than a builder -- "12 conditions, 0 independently flippable".
 * That estimate was made before the arm axis existed. Read through it the
 * pattern is ordinary: ONE condition (white-engulfs-black || black-engulfs-
 * white), three terms per arm, of which two flip and one is entailed.
 *
 * TWO DECISIONS, AND ONLY THE OUTER ONE FIRES. The inner if chooses between
 * +/-100 and +/-80 -- both non-zero -- so it selects an output CLASS and the
 * outer if is what decides firing. check_mcdc_conditions.py now walks out to
 * the outer if in exactly that case, and still declines when the outer block
 * also assigns zero (CDLTRISTAR), where the outer if does NOT decide firing.
 * CDLHARAMI and CDLHARAMICROSS keep declining for that same reason.
 *
 * THE +/-80 CASES ARE WHAT REACH INSIDE THE ENGULF TERM, and this is the part
 * worth reading. Each arm's term 2 is itself a disjunction:
 *
 *    ( close(i) >= open(i-1) && open(i) <  close(i-1) )
 * || ( close(i) >  open(i-1) && open(i) <= close(i-1) )
 *
 * -- the same engulfing test written twice, differing only in which end is
 * allowed to touch. Both alternatives hold whenever neither end touches, which
 * is every +/-100 case. Exactly one holds when one end meets exactly, and that
 * is precisely when the pattern emits +/-80. So the output class and the
 * alternative are the same distinction seen from two sides, and the four
 * detects below are also the four sole-true cases for the two arms' interiors.
 *
 * WHAT THE GATE ENFORCES HERE IS LESS THAN WHAT IS WRITTEN. pb_signs(4) asks
 * for one case per class, which is four; covering both alternatives of both
 * arms needs those same four, but nothing makes the two +/-80 cases differ in
 * WHICH end touches. Two of the four detects are therefore volunteered rather
 * than required. That is one level deeper than pb_arm reaches -- a disjunction
 * inside an arm term rather than inside a condition -- and CDLENGULFING is the
 * only pattern in the corpus with one. A third level of the same machinery
 * would close it; one pattern did not seem worth the axis.
 *
 * The pattern reads no candle settings at all, so nothing here depends on the
 * primer.
 */
static void cond_engulfing( int i, int *c )
{
   c[0] = (  pb_white(i) && !pb_white(i-1) &&
             ( ( pbC[i] >= pbO[i-1] && pbO[i] <  pbC[i-1] ) ||
               ( pbC[i] >  pbO[i-1] && pbO[i] <= pbC[i-1] ) ) )
       || ( !pb_white(i) &&  pb_white(i-1) &&
             ( ( pbO[i] >= pbC[i-1] && pbC[i] <  pbO[i-1] ) ||
               ( pbO[i] >  pbC[i-1] && pbC[i] <= pbO[i-1] ) ) );
}
static void arm_engulfing( int i, int cond, int arm, int *a )
{
   if( cond != 0 ) return;
   if( arm == 0 )
   {
      a[0] =  pb_white(i);
      a[1] = !pb_white(i-1);
      a[2] = ( pbC[i] >= pbO[i-1] && pbO[i] <  pbC[i-1] ) ||
             ( pbC[i] >  pbO[i-1] && pbO[i] <= pbC[i-1] );
   }
   else
   {
      a[0] = !pb_white(i);
      a[1] =  pb_white(i-1);
      a[2] = ( pbO[i] >= pbC[i-1] && pbC[i] <  pbO[i-1] ) ||
             ( pbO[i] >  pbC[i-1] && pbC[i] <= pbO[i-1] );
   }
}

static void build_engulfing( void )
{
  pb_conditions(1);
  pb_signs(4);
  pb_arm(0,0,3); pb_arm(0,1,3);
  pb_arm_model(arm_engulfing);

  pb_waive_arm(0,0,0,"entailed by terms 1 and 2. Term 1 makes the prior candle black, so open(i-1) > close(i-1); term 2 puts close(i) at or above open(i-1) and open(i) at or below close(i-1), so close(i) >= open(i-1) > close(i-1) >= open(i) and this candle is white by construction");
  pb_waive_arm(0,1,0,"entailed by terms 1 and 2, mirrored: the prior candle is white, and term 2 puts open(i) at or above close(i-1) and close(i) at or below open(i-1), so open(i) >= close(i-1) > open(i-1) >= close(i)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  int e1=pb_bar(99,112,98,111);
  pb_detect(e1,100,"detect +100: a white candle engulfing a black one on both ends, neither end equal");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  int e2=pb_bar(111,112,98,99);
  pb_detect(e2,-100,"detect -100: the mirror");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  int e3=pb_bar(99,111,98,110);
  pb_detect(e3,80,"detect +80: the close meets the prior open exactly -- only the first alternative of the engulf term holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  int e4=pb_bar(100,112,99,111);
  pb_detect(e4,80,"detect +80 again, the OTHER way: the open meets the prior close exactly, so only the second alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  int e5=pb_bar(110,111,98,99);
  pb_detect(e5,-80,"detect -80: the open meets the prior close exactly -- only the first alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  int e6=pb_bar(111,112,99,100);
  pb_detect(e6,-80,"detect -80 again, the other way: only the second alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  int e7=pb_bar(99,112,98,111);
  pb_flip_in(e7,0,0,1,"break c0 alt0 term1: the prior candle is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  int e8=pb_bar(100,111,99,110);
  pb_flip_in(e8,0,0,2,"break c0 alt0 term2: both ends meet exactly -- the one engulfing shape the pattern excludes");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  int e9=pb_bar(111,112,98,99);
  pb_flip_in(e9,0,1,1,"break c0 alt1 term1: the prior candle is black");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  int e10=pb_bar(110,111,99,100);
  pb_flip_in(e10,0,1,2,"break c0 alt1 term2: both ends meet exactly");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,111,99,100);
  int e11=pb_bar(99,112,98,111);
  pb_control(e11,100,0,"restore c0 via alt0: a white candle engulfing a black one");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,111,99,110);
  int e12=pb_bar(111,112,98,99);
  pb_control(e12,-100,0,"restore c0 via alt1: a black candle engulfing a white one");
  pb_flat(8);

}

/* ---- The three patterns whose firing decision spans nested ifs ----------- *
 *
 * CDLHARAMI, CDLHARAMICROSS and CDLTRISTAR were the last uncounted patterns.
 * All three write their non-zero output from inside more than one `if`, which
 * the conjunct counter used to decline on rather than describe. It now derives
 * the firing decision instead: collect the guard chain of every non-zero
 * assignment, factor the common prefix into conjuncts, and OR the remainders.
 * That one rule covers a flat decision, CDLENGULFING's if/else over two
 * non-zero values, the harami pair's else-if chain and CDLTRISTAR's sequential
 * ifs alike.
 *
 *   CDLHARAMI / CDLHARAMICROSS
 *     if(long 1st) { if(short 2nd) { if(strict inside) x100 else if(inside) x80 } }
 *     firing = long && short && (strict || loose)     -- 3 conditions, arms [2,2]
 *
 *   CDLTRISTAR
 *     if(3 doji) { =0; if(gap up && ...) -100; if(gap down && ...) +100 }
 *     firing = doji && doji && doji && (up || down)   -- 4 conditions, arms [2,2]
 *
 * THE HARAMI PAIR'S STRICT ALTERNATIVE CAN NEVER BE SOLE-TRUE, and that is the
 * one waiver here. Its two alternatives are the same containment test written
 * with < > and with <= >=, so the strict form implies the loose one. Whenever
 * strict holds, loose holds too. What the loose form alone means is exactly the
 * +/-80 output -- one end of the body meeting the other's -- so that case is
 * covered as alt1's sole-true and the strict alternative is waived. The four
 * output classes and the two alternatives are again the same distinction seen
 * from two sides, as in CDLENGULFING.
 *
 * CDLTRISTAR needs no such waiver: its alternatives are a gap up and a gap down
 * off the same candle, which are mutually exclusive, so each is sole-true in
 * its own direction.
 */
static void cond_harami( int i, int *c )
{
   c[0] = pb_body(i-1) > pb_avg(TA_BodyLong,  i-1);
   c[1] = pb_body(i)  <= pb_avg(TA_BodyShort, i);
   c[2] = ( pb_bodyhi(i) <  pb_bodyhi(i-1) && pb_bodylo(i) >  pb_bodylo(i-1) )
       || ( pb_bodyhi(i) <= pb_bodyhi(i-1) && pb_bodylo(i) >= pb_bodylo(i-1) );
}
static void arm_harami( int i, int cond, int arm, int *a )
{
   if( cond != 2 ) return;
   if( arm == 0 ) { a[0] = pb_bodyhi(i) <  pb_bodyhi(i-1);
                    a[1] = pb_bodylo(i) >  pb_bodylo(i-1); }
   else           { a[0] = pb_bodyhi(i) <= pb_bodyhi(i-1);
                    a[1] = pb_bodylo(i) >= pb_bodylo(i-1); }
}
static void cond_haramicross( int i, int *c )
{
   c[0] = pb_body(i-1) > pb_avg(TA_BodyLong, i-1);
   c[1] = pb_body(i)  <= pb_avg(TA_BodyDoji, i);
   c[2] = ( pb_bodyhi(i) <  pb_bodyhi(i-1) && pb_bodylo(i) >  pb_bodylo(i-1) )
       || ( pb_bodyhi(i) <= pb_bodyhi(i-1) && pb_bodylo(i) >= pb_bodylo(i-1) );
}
static void arm_haramicross( int i, int cond, int arm, int *a )
{
   arm_harami(i, cond, arm, a);
}
static void cond_tristar( int i, int *c )
{
   double d = pb_avg(TA_BodyDoji, i-2);
   c[0] = pb_body(i-2) <= d;
   c[1] = pb_body(i-1) <= d;
   c[2] = pb_body(i)   <= d;
   c[3] = ( pb_bodylo(i-1) > pb_bodyhi(i-2) && pb_bodyhi(i) < pb_bodyhi(i-1) )
       || ( pb_bodyhi(i-1) < pb_bodylo(i-2) && pb_bodylo(i) > pb_bodylo(i-1) );
}
static void arm_tristar( int i, int cond, int arm, int *a )
{
   if( cond != 3 ) return;
   if( arm == 0 ) { a[0] = pb_bodylo(i-1) > pb_bodyhi(i-2);
                    a[1] = pb_bodyhi(i)   < pb_bodyhi(i-1); }
   else           { a[0] = pb_bodyhi(i-1) < pb_bodylo(i-2);
                    a[1] = pb_bodylo(i)   > pb_bodylo(i-1); }
}

static void build_harami( void )
{
  pb_conditions(3);
  pb_signs(4);
  pb_disjuncts(2,2);
  pb_arm(2,0,2); pb_arm(2,1,2);
  pb_arm_model(arm_harami);
  pb_waive_disjunct(2,0,"the strict form implies the loose one, so it can never be the only alternative true; the loose form alone is what the +/-80 output means, and that case is covered");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha1=pb_bar(104,105,101,102);
  pb_detect(ha1,100,"detect +100: a short body strictly inside a long black one");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  int ha2=pb_bar(104,105,101,102);
  pb_detect(ha2,-100,"detect -100: the mirror over a long white first candle");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha3=pb_bar(112,113,109,110);
  pb_detect(ha3,80,"detect +80: the body ceilings meet exactly, so only the loose alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,113,99,112);
  int ha4=pb_bar(112,113,109,110);
  pb_detect(ha4,-80,"detect -80: the mirror");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha5=pb_bar(112,113,109,110);
  pb_sole(ha5,80,2,1,"c2 alt1 alone: ceilings equal, so the strict alternative is false and the loose one carries it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,113,99,100);
  int ha6=pb_bar(101,102,100.5,101);
  pb_flip(ha6,0,"break c0: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha7=pb_bar(104,105,101,102);
  pb_control(ha7,100,0,"restore c0: 1st body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha8=pb_bar(106,107,101,102);
  pb_flip(ha8,1,"break c1: 2nd body 4 above the BodyShort average 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha9=pb_bar(105,106,101,102);
  pb_control(ha9,100,1,"restore c1: 2nd body 3 == avg 3, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha10=pb_bar(114,115,111,112);
  pb_flip_in(ha10,2,1,0,"break c2 alt1 term0: 2nd body ceiling 114 above the 1st's 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha11=pb_bar(99,100,96,97);
  pb_flip_in(ha11,2,1,1,"break c2 alt1 term1: 2nd body floor 97 below the 1st's 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha12=pb_bar(114,115,111,112);
  pb_flip_in(ha12,2,0,0,"break c2 alt0 term0: same bars, filed against the strict alternative");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha13=pb_bar(99,100,96,97);
  pb_flip_in(ha13,2,0,1,"break c2 alt0 term1: same bars, filed against the strict alternative");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha14=pb_bar(104,105,101,102);
  pb_control(ha14,100,2,"restore c2: the 2nd body is strictly inside the 1st");
  pb_flat(8);


  /* The other +/-80: the body FLOORS meet rather than the ceilings. Without
   * it nothing sits on the loose alternative's second bound, and relaxing
   * that bound to strict passes. */
  pb_primer(12,100,2,4);
  pb_bar(112,113,99,100);
  int ha90=pb_bar(102,103,99,100);
  pb_sole(ha90,80,2,1,"c2 alt1 alone, the floor side: floors equal, so the strict alternative is false");
  pb_flat(8);

}

static void build_haramicross( void )
{
  pb_conditions(3);
  pb_signs(4);
  pb_disjuncts(2,2);
  pb_arm(2,0,2); pb_arm(2,1,2);
  pb_arm_model(arm_haramicross);
  pb_waive_disjunct(2,0,"the strict form implies the loose one, so it can never be the only alternative true; the loose form alone is what the +/-80 output means, and that case is covered");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha1=pb_bar(104,105,103,104);
  pb_detect(ha1,100,"detect +100: a doji strictly inside a long black candle");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);
  int ha2=pb_bar(104,105,103,104);
  pb_detect(ha2,-100,"detect -100: the mirror");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha3=pb_bar(108,109,107,108);
  pb_detect(ha3,80,"detect +80: the ceilings meet exactly");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,109,99,108);
  int ha4=pb_bar(108,109,107,108);
  pb_detect(ha4,-80,"detect -80: the mirror");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha5=pb_bar(108,109,107,108);
  pb_sole(ha5,80,2,1,"c2 alt1 alone: only the loose alternative holds");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(102,109,99,100);
  int ha6=pb_bar(101,102,100.5,101);
  pb_flip(ha6,0,"break c0: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha7=pb_bar(104,105,103,104);
  pb_control(ha7,100,0,"restore c0: 1st body 8 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha8=pb_bar(105,106,103,106.5);
  pb_flip(ha8,1,"break c1: 2nd body 1.5 above the BodyDoji average 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha9=pb_bar(104,105,103,105);
  pb_control(ha9,100,1,"restore c1: 2nd body 1 == avg 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha10=pb_bar(110,111,109,110);
  pb_flip_in(ha10,2,1,0,"break c2 alt1 term0: 2nd body ceiling 110 above the 1st's 108");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha11=pb_bar(98,99,97,98);
  pb_flip_in(ha11,2,1,1,"break c2 alt1 term1: 2nd body floor 98 below the 1st's 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha12=pb_bar(110,111,109,110);
  pb_flip_in(ha12,2,0,0,"break c2 alt0 term0: same bars, filed against the strict alternative");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha13=pb_bar(98,99,97,98);
  pb_flip_in(ha13,2,0,1,"break c2 alt0 term1: same bars, filed against the strict alternative");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha14=pb_bar(104,105,103,104);
  pb_control(ha14,100,2,"restore c2: the doji is strictly inside the 1st body");
  pb_flat(8);


  /* The floor-side +/-80, same reason as CDLHARAMI's. */
  pb_primer(12,100,2,4);
  pb_bar(108,109,99,100);
  int ha91=pb_bar(101,102,99,100);
  pb_sole(ha91,80,2,1,"c2 alt1 alone, the floor side");
  pb_flat(8);

}

static void build_tristar( void )
{
  pb_conditions(4);
  pb_signs(2);
  pb_disjuncts(3,2);
  pb_arm(3,0,2); pb_arm(3,1,2);
  pb_arm_model(arm_tristar);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,106);
  int tr1=pb_bar(102,103,101,102);
  pb_detect(tr1,-100,"detect -100: three doji with the middle one gapping up");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(94,95,93,94);
  int tr2=pb_bar(98,99,97,98);
  pb_detect(tr2,100,"detect +100: the middle one gapping down");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,106);
  int tr3=pb_bar(102,103,101,102);
  pb_sole(tr3,-100,3,0,"c3 alt0 alone: the middle doji gaps UP, so the gap-down alternative cannot hold");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(94,95,93,94);
  int tr4=pb_bar(98,99,97,98);
  pb_sole(tr4,100,3,1,"c3 alt1 alone: the middle doji gaps DOWN");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101.5,99,101.5);
  pb_bar(106,107,105,106);
  int tr5=pb_bar(102,103,101,102);
  pb_flip(tr5,0,"break c0: 1st body 1.5 above the BodyDoji average 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,101);
  pb_bar(106,107,105,106);
  int tr6=pb_bar(102,103,101,102);
  pb_control(tr6,-100,0,"restore c0: 1st body 1 == avg 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107.5,105,107.5);
  int tr7=pb_bar(102,103,101,102);
  pb_flip(tr7,1,"break c1: 2nd body 1.5 above avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,107);
  int tr8=pb_bar(102,103,101,102);
  pb_control(tr8,-100,1,"restore c1: 2nd body 1 == avg 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,106);
  int tr9=pb_bar(102,103.5,101,103.5);
  pb_flip(tr9,2,"break c2: 3rd body 1.5 above avg 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,106);
  int tr10=pb_bar(102,103,101,103);
  pb_control(tr10,-100,2,"restore c2: 3rd body 1 == avg 1, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(100,101,99,100);
  int tr11=pb_bar(99,100,98,99);
  pb_flip_in(tr11,3,0,0,"break c3 alt0 term0: 2nd body floor 100 == the 1st's ceiling, the gap is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,106);
  int tr12=pb_bar(106,107,105,106);
  pb_flip_in(tr12,3,0,1,"break c3 alt0 term1: 3rd body ceiling 106 == the 2nd's, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(100,101,99,100);
  int tr13=pb_bar(101,102,100,101);
  pb_flip_in(tr13,3,1,0,"break c3 alt1 term0: 2nd body ceiling 100 == the 1st's floor, the gap is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(94,95,93,94);
  int tr14=pb_bar(94,95,93,94);
  pb_flip_in(tr14,3,1,1,"break c3 alt1 term1: 3rd body floor 94 == the 2nd's, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,101,99,100);
  pb_bar(106,107,105,106);
  int tr15=pb_bar(102,103,101,102);
  pb_control(tr15,-100,3,"restore c3 via alt0: the gap-up alternative holds");
  pb_flat(8);

}

/* ------------------------------------------------------------------ *
 * CDLHIKKAKE / CDLHIKKAKEMOD -- the DETECTION decision.
 *
 * These two are the only patterns in the corpus whose output does not come
 * from one per-bar decision. Each has two:
 *
 *   detection    -- a pure function of the pattern window, emitting +/-100.
 *                   That is what the builders below cover, and it is an
 *                   ordinary MC/DC decision like every other pattern's.
 *   confirmation -- emitting +/-200 up to three bars later, from a countdown
 *                   and a cached high/low set at the detection bar. Its truth
 *                   at bar i depends on which bar last detected, so it is not
 *                   a decision over bar i's window at all and the pb_* model
 *                   (conditions evaluated at one index) cannot express it.
 *
 * The confirmation half stays where it already is, on
 * test_hikkake_predicate_coverage's legacy scenarios, which walk the countdown
 * and both confirmation directions. check-mcdc's counter reads the detection
 * decision only, for the same reason: its firing-expression derivation drops
 * any path guarded by loop-carried state.
 *
 * The bars use no candle setting except HIKKAKEMOD's Near, so the geometry is
 * free: highs and lows are placed directly on the strict inequalities. Near's
 * window at the 2nd candle is four primer bars plus the 1st candle, so holding
 * the 1st at a HighLow of 10 puts the threshold on exactly 2.
 * ------------------------------------------------------------------ */
static void cond_hikkake( int i, int *c )
{
   c[0] = pbH[i-1] < pbH[i-2];
   c[1] = pbL[i-1] > pbL[i-2];
   c[2] = ( pbH[i] < pbH[i-1] && pbL[i] < pbL[i-1] )
       || ( pbH[i] > pbH[i-1] && pbL[i] > pbL[i-1] );
}
static void arm_hikkake( int i, int cond, int arm, int *a )
{
   if( cond != 2 ) return;
   if( arm == 0 )
   {
      a[0] = pbH[i] < pbH[i-1];
      a[1] = pbL[i] < pbL[i-1];
   }
   else
   {
      a[0] = pbH[i] > pbH[i-1];
      a[1] = pbL[i] > pbL[i-1];
   }
}

static void build_hikkake( void )
{
  pb_conditions(3);
  pb_signs(2);
  pb_disjuncts(2,2);
  pb_arm(2,0,2); pb_arm(2,1,2);
  pb_arm_model(arm_hikkake);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk1=pb_bar(100,106,91,100);
  pb_detect(hk1,100,"detect +100: an inside bar, then a lower high AND lower low");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk2=pb_bar(100,110,93,100);
  pb_detect(hk2,-100,"detect -100: the mirror, a higher high AND higher low");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk3=pb_bar(100,106,91,100);
  pb_sole(hk3,100,2,0,"c2 alt0 alone: the 3rd breaks DOWN, so the up alternative cannot hold");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk4=pb_bar(100,110,93,100);
  pb_sole(hk4,-100,2,1,"c2 alt1 alone: the 3rd breaks UP");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,110,92,100);
  int hk5=pb_bar(100,106,91,100);
  pb_flip(hk5,0,"break c0: 2nd high 110 == the 1st high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk6=pb_bar(100,106,91,100);
  pb_control(hk6,100,0,"restore c0: 2nd high 108 < 110");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,90,100);
  int hk7=pb_bar(95,106,89,95);
  pb_flip(hk7,1,"break c1: 2nd low 90 == the 1st low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk8=pb_bar(95,106,89,95);
  pb_control(hk8,100,1,"restore c1: 2nd low 92 > 90, the 3rd left exactly as the flip had it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk9=pb_bar(100,108,91,100);
  pb_flip_in(hk9,2,0,0,"break c2 alt0 term0: 3rd high 108 == the 2nd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk10=pb_bar(100,106,92,100);
  pb_flip_in(hk10,2,0,1,"break c2 alt0 term1: 3rd low 92 == the 2nd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk11=pb_bar(100,108,93,100);
  pb_flip_in(hk11,2,1,0,"break c2 alt1 term0: 3rd high 108 == the 2nd high");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk12=pb_bar(100,110,92,100);
  pb_flip_in(hk12,2,1,1,"break c2 alt1 term1: 3rd low 92 == the 2nd low");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk13=pb_bar(100,106,91,100);
  pb_control(hk13,100,2,"restore c2 on the bull geometry: the minimal pair for the alt0 flips");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110,90,100);
  pb_bar(100,108,92,100);
  int hk14=pb_bar(100,110,93,100);
  pb_control(hk14,-100,2,"restore c2 on the bear geometry: the minimal pair for the alt1 flips");
  pb_flat(8);

}


static void cond_hikkakemod( int i, int *c )
{
   c[0] = pbH[i-2] < pbH[i-3];
   c[1] = pbL[i-2] > pbL[i-3];
   c[2] = pbH[i-1] < pbH[i-2];
   c[3] = pbL[i-1] > pbL[i-2];
   c[4] = ( pbH[i] < pbH[i-1] && pbL[i] < pbL[i-1] &&
            pbC[i-2] <= pbL[i-2] + pb_avg(TA_Near, i-2) )
       || ( pbH[i] > pbH[i-1] && pbL[i] > pbL[i-1] &&
            pbC[i-2] >= pbH[i-2] - pb_avg(TA_Near, i-2) );
}
static void arm_hikkakemod( int i, int cond, int arm, int *a )
{
   if( cond != 4 ) return;
   if( arm == 0 )
   {
      a[0] = pbH[i] < pbH[i-1];
      a[1] = pbL[i] < pbL[i-1];
      a[2] = pbC[i-2] <= pbL[i-2] + pb_avg(TA_Near, i-2);
   }
   else
   {
      a[0] = pbH[i] > pbH[i-1];
      a[1] = pbL[i] > pbL[i-1];
      a[2] = pbC[i-2] >= pbH[i-2] - pb_avg(TA_Near, i-2);
   }
}

static void build_hikkakemod( void )
{
  pb_conditions(5);
  pb_signs(2);
  pb_disjuncts(4,2);
  pb_arm(4,0,3); pb_arm(4,1,3);
  pb_arm_model(arm_hikkakemod);

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm1=pb_bar(96,102,96,96);
  pb_detect(hm1,100,"detect +100: two inside bars, the 2nd closing near its low, then a break down");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,103);
  pb_bar(100,103,97,100);
  int hm2=pb_bar(100,105,98,100);
  pb_detect(hm2,-100,"detect -100: the 2nd closing near its high, then a break up");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm3=pb_bar(96,102,96,96);
  pb_sole(hm3,100,4,0,"c4 alt0 alone: the 4th breaks DOWN");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,103);
  pb_bar(100,103,97,100);
  int hm4=pb_bar(100,105,98,100);
  pb_sole(hm4,-100,4,1,"c4 alt1 alone: the 4th breaks UP");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,105,96,97);
  pb_bar(100,103,97,100);
  int hm5=pb_bar(96,102,96,96);
  pb_flip(hm5,0,"break c0: 2nd high 105 == the 1st high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm6=pb_bar(96,102,96,96);
  pb_control(hm6,100,0,"restore c0: 2nd high 104 < 105");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,95,97);
  pb_bar(100,103,97,100);
  int hm7=pb_bar(96,102,96,96);
  pb_flip(hm7,1,"break c1: 2nd low 95 == the 1st low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm8=pb_bar(96,102,96,96);
  pb_control(hm8,100,1,"restore c1: 2nd low 96 > 95");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,104,97,100);
  int hm9=pb_bar(96,102,96,96);
  pb_flip(hm9,2,"break c2: 3rd high 104 == the 2nd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm10=pb_bar(96,102,96,96);
  pb_control(hm10,100,2,"restore c2: 3rd high 103 < 104");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,96,100);
  int hm11=pb_bar(95,102,95,95);
  pb_flip(hm11,3,"break c3: 3rd low 96 == the 2nd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm12=pb_bar(95,102,95,95);
  pb_control(hm12,100,3,"restore c3: 3rd low 97 > 96, the 4th left exactly as the flip had it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm13=pb_bar(96,103,96,96);
  pb_flip_in(hm13,4,0,0,"break c4 alt0 term0: 4th high 103 == the 3rd high, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm14=pb_bar(98,102,97,98);
  pb_flip_in(hm14,4,0,1,"break c4 alt0 term1: 4th low 97 == the 3rd low, the test is strict");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,99);
  pb_bar(100,103,97,100);
  int hm15=pb_bar(96,102,96,96);
  pb_flip_in(hm15,4,0,2,"break c4 alt0 term2: 2nd closes 99, above its low 96 + Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,98);
  pb_bar(100,103,97,100);
  int hm16=pb_bar(96,102,96,96);
  pb_control(hm16,100,4,"restore c4 alt0 term2: 2nd closes 98 == 96 + Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,103);
  pb_bar(100,103,97,100);
  int hm17=pb_bar(100,103,98,100);
  pb_flip_in(hm17,4,1,0,"break c4 alt1 term0: 4th high 103 == the 3rd high");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,103);
  pb_bar(100,103,97,100);
  int hm18=pb_bar(100,105,97,100);
  pb_flip_in(hm18,4,1,1,"break c4 alt1 term1: 4th low 97 == the 3rd low");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,101);
  pb_bar(100,103,97,100);
  int hm19=pb_bar(100,105,98,100);
  pb_flip_in(hm19,4,1,2,"break c4 alt1 term2: 2nd closes 101, below its high 104 - Near 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,102);
  pb_bar(100,103,97,100);
  int hm20=pb_bar(100,105,98,100);
  pb_control(hm20,-100,4,"restore c4 alt1 term2: 2nd closes 102 == 104 - Near 2, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(97,104,96,97);
  pb_bar(100,103,97,100);
  int hm21=pb_bar(96,102,96,96);
  pb_control(hm21,100,4,"restore c4 on the bull geometry: the minimal pair for the alt0 term0/term1 flips");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105,95,100);
  pb_bar(103,104,96,103);
  pb_bar(100,103,97,100);
  int hm22=pb_bar(100,105,98,100);
  pb_control(hm22,-100,4,"restore c4 on the bear geometry: the minimal pair for the alt1 term0/term1 flips");
  pb_flat(8);

}

/* ---- Moderate tier, unit 6: the last seven buildable patterns ------------- *
 *
 * 52 conjuncts across seven, five bi-signed, and the end of the buildable
 * non-hard set. Two things about this unit are new.
 *
 * KICKING and KICKINGBYLENGTH HAVE IDENTICAL DECISIONS. All eight conjuncts
 * are the same source text; they differ only in the value emitted, KICKING
 * taking this candle's colour and KICKINGBYLENGTH the colour of whichever of
 * the two has the longer body. So no flip can separate them -- every condition
 * scenario behaves identically in both -- and the separation has to come from
 * the output-class axis: a scenario where the longer candle is NOT the current
 * one, and the two disagree.
 *
 * INCLUSIVE SITES, COUNTED BEFORE THE SCENARIOS WERE WRITTEN, because unit 5
 * shipped four of five right and reported five. Five across the seven:
 * CDL3INSIDE 1, CDLSEPARATINGLINES 2, CDLSTICKSANDWICH 2, and none in the
 * other four. Each of those five gets its control on the equality; every other
 * comparison here is strict and gets its flip there instead.
 *
 * The two KICKINGs and CONCEALBABYSWALL read one setting at several windows
 * (BodyLong and ShadowVeryShort at i-1 and i; ShadowVeryShort at i-3, i-2 and
 * i-1). That wiring is check_candle_windows.py's business as of 8c74ae3e7, not
 * these builders' -- pb_primer lays down identical bars, so no scenario here
 * could tell one window from another anyway.
 */

/* CDLSEPARATINGLINES -- two opposite candles opening at the same price, the
 * second a belt hold.
 *
 *   c0  color(i-1) == -color(i)
 *   c1  open(i) <= open(i-1) + avg(Equal, i-1)
 *   c2  open(i) >= open(i-1) - avg(Equal, i-1)
 *   c3  realbody(i) > avg(BodyLong, i)
 *   c4  ( white(i) && lowershadow(i) < avg(ShadowVeryShort, i) )
 *       || ( black(i) && uppershadow(i) < avg(ShadowVeryShort, i) )
 *
 * c1 and c2 are the two inclusive sites; each is pinned by a control sitting
 * exactly on its edge. c4 is BELTHOLD's disjunction, and the colour that
 * selects it also sets the output sign, so the black-arm scenario reaches the
 * second disjunct and the second class together.
 */
static void cond_separatinglines( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-1);
   double vs = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_white(i-1) != pb_white(i);
   c[1] = pbO[i] <= pbO[i-1] + eq;
   c[2] = pbO[i] >= pbO[i-1] - eq;
   c[3] = pb_body(i) > pb_avg(TA_BodyLong, i);
   c[4] = (  pb_white(i) && pb_losh(i) < vs )
       || ( !pb_white(i) && pb_upsh(i) < vs );
}

/* c4's two alternatives are a colour selector and a shadow test each -- the
 * BELTHOLD shape. The selector term is waived for the same reason it is there:
 * it is what chooses the arm, and the class it chooses is fired by pb_signs(2). */
static void arm_separatinglines( int i, int cond, int arm, int *a )
{
   double vs = pb_avg(TA_ShadowVeryShort, i);
   if( cond != 4 ) return;
   if( arm == 0 ) { a[0] =  pb_white(i); a[1] = pb_losh(i) < vs; }
   else           { a[0] = !pb_white(i); a[1] = pb_upsh(i) < vs; }
}

static void build_separatinglines( void )
{
  pb_conditions(5);
  pb_signs(2);
  pb_arm(4,0,2); pb_arm(4,1,2);
  pb_arm_model(arm_separatinglines);
  pb_waive_arm(4,0,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(4,1,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");

  /* The prior bar must satisfy two settings at once, and they pull in opposite
   * directions. BodyLong is RealBody-typed: avg(BodyLong, i) = (9*2 + A)/10 is
   * exact only for A congruent to 2 mod 10, so A = 12. ShadowVeryShort is
   * HighLow-typed: avg(SVS, i) = 0.1 * (9*10 + R)/10 is exact only for
   * R in {10, 110, 210, ...}. R = 10 is arithmetically impossible -- a body of
   * 12 needs a range of at least 12 -- so the smallest workable prior bar is
   * body 12 inside a range of 110, giving thresholds of 3.0 and 2.0.
   *
   * pb_bar clamps, which is what rules R = 16 out as an accident rather than a
   * choice: (106,110,100,94) looks like a range of 10 and is stored as a range
   * of 16, because the low is pulled down to the close. Both averages have to
   * be read off the bar as STORED.
   */
  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);                   /* prior: black, body 12, range 110, open 106 */
  int d=pb_bar(106,112,105,111);           /* white, body 5 > 3, lower shadow 1 < 2 */
  pb_detect(d,100,"detect: opposite, open 106 == prior open, body 5 > 3, lower shadow 1 < 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int f0=pb_bar(106,107,100,101);          /* BLACK like the prior; upper shadow 1 keeps c4 true */
  pb_flip(f0,0,"break c0: both candles are black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int k0=pb_bar(106,112,105,111);
  pb_control(k0,100,0,"restore c0: opposite colours");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int f1=pb_bar(106.6,112,105.6,111);      /* open 106.6 above the band top 106.5 */
  pb_flip(f1,1,"break c1: open 106.6 > band top 106.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int k1=pb_bar(106.5,112,105.5,111);      /* open exactly on the band top */
  pb_control(k1,100,1,"restore c1: open 106.5 == band top, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int f2=pb_bar(105.4,112,104.4,111);      /* open 105.4 below the band bottom 105.5 */
  pb_flip(f2,2,"break c2: open 105.4 < band bottom 105.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int k2=pb_bar(105.5,112,104.5,111);      /* open exactly on the band bottom */
  pb_control(k2,100,2,"restore c2: open 105.5 == band bottom, inclusive");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int f3=pb_bar(106,112,105,109);          /* body 3 == avg */
  pb_flip(f3,3,"break c3: body 3 == avg 3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int k3=pb_bar(106,112,105,111);
  pb_control(k3,100,3,"restore c3: body 5 > 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int f4=pb_bar(106,112,104,111);          /* lower shadow 2 == avg */
  pb_flip_in(f4,4,0,1,"break c4 alt0 term1: lower shadow 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,200,90,94);
  int k4=pb_bar(106,112,105,111);
  pb_control(k4,100,4,"restore c4: lower shadow 1 < 2");
  pb_flat(8);

  /* The black arm: the other disjunct of c4 and the other output class. Black
   * reads the UPPER shadow where white reads the lower. */
  pb_primer(12,100,2,4);
  pb_bar(94,200,90,106);                   /* prior: white, body 12, range 110, open 94 */
  int db=pb_bar(94,95,88,89);              /* black, body 5 > 3, upper shadow 1 < 2 */
  pb_detect(db,-100,"detect black: opposite, open 94 == prior open, body 5 > 3, upper shadow 1 < 2");
  pb_flat(8);

  /* The black arm's own term. Without this the arm has a firing case but no
   * case that asks for its shadow test alone -- pb_signs would be satisfied and
   * the test could be relaxed with the tier green. */
  pb_primer(12,100,2,4);
  pb_bar(94,200,90,106);
  int f4b=pb_bar(94,96,88,89);             /* upper shadow 2 == avg 2 */
  pb_flip_in(f4b,4,1,1,"break c4 alt1 term1: upper shadow 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(94,200,90,106);
  int k4b=pb_bar(94,95,88,89);
  pb_control(k4b,-100,4,"restore c4 alt1: upper shadow 1 < 2");
  pb_flat(8);
}

/* CDLKICKING -- two opposite marubozus with a gap between them.
 *
 *   c0  color(i-1) == -color(i)
 *   c1  realbody(i-1)    >  avg(BodyLong, i-1)
 *   c2  uppershadow(i-1) <  avg(ShadowVeryShort, i-1)
 *   c3  lowershadow(i-1) <  avg(ShadowVeryShort, i-1)
 *   c4  realbody(i)      >  avg(BodyLong, i)
 *   c5  uppershadow(i)   <  avg(ShadowVeryShort, i)
 *   c6  lowershadow(i)   <  avg(ShadowVeryShort, i)
 *   c7  ( black(i-1) && candlegapup(low(i), high(i-1)) )
 *       || ( white(i-1) && candlegapdown(high(i), low(i-1)) )
 *
 * Both bars must be marubozus, so both need a range barely larger than their
 * body, and that range enters avg(ShadowVeryShort, i) through the HighLow
 * window. A first bar of body 12 inside a range of 13.125 puts that average on
 * 1.03125, against 1.0 at i-1 where the window is all primer -- so the two bars
 * are measured against DIFFERENT shadow thresholds and each flip is placed on
 * its own.
 *
 * 13.125 rather than a round 13, because THE THRESHOLD HAS TO BE DYADIC. What
 * the library compares is not a literal, it is a subtraction of two prices, and
 * `high - max(open,close)` can only reproduce the threshold's bits when the
 * threshold fits the price's exponent. With a range of 13 the average is 1.03,
 * and 113.03 - 112 is 1.0300000000000011 -- five ULP above it, because 112 +
 * 1.03 is not representable and the reconstruction drops six bits. Both `<` and
 * `<=` are then false, so the flip still reports 0 and the boundary it claims to
 * sit on is not tested: relaxing c5 and c6 to `<=` in ta_CDLKICKING.c passed the
 * entire suite.
 *
 * A dyadic threshold does exist here, which the first version of this comment
 * denied. avg(SVS,i) is 0.1*((9*10 + R)/10), dyadic when 25 divides 90 + R, and
 * R = 13.125 gives exactly 1.03125 = 33/32. The marubozu survives it: body 12,
 * upper shadow 0.5, lower shadow 0.625, both still under the 1.0 threshold at
 * i-1. The body stays 12, so avg(BodyLong, i) stays 3.0 and c4's flip is
 * untouched. 113.03125 - 112 and 108 - 106.96875 are then both bitwise 1.03125.
 */
static void cond_kicking( int i, int *c )
{
   double vs1 = pb_avg(TA_ShadowVeryShort, i-1);
   double vs0 = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_white(i-1) != pb_white(i);
   c[1] = pb_body(i-1) >  pb_avg(TA_BodyLong, i-1);
   c[2] = pb_upsh(i-1) <  vs1;
   c[3] = pb_losh(i-1) <  vs1;
   c[4] = pb_body(i)   >  pb_avg(TA_BodyLong, i);
   c[5] = pb_upsh(i)   <  vs0;
   c[6] = pb_losh(i)   <  vs0;
   c[7] = ( !pb_white(i-1) && pbL[i] > pbH[i-1] )
       || (  pb_white(i-1) && pbH[i] < pbL[i-1] );
}

/* The scenario the two KICKINGs share. CDLKICKING reports this candle's
 * colour, CDLKICKINGBYLENGTH the colour of whichever body is longer, so a
 * builder whose longer body is always the SAME candle cannot tell that
 * comparison from a constant. The detects here keep the first body longer
 * (12 against 5), which is what makes the two functions disagree and gives each
 * its own expected value; CDLKICKINGBYLENGTH carries one more with the second
 * body longer, so its selector is reached on both branches. The flips do not
 * all preserve that ordering -- they are placed on their own boundaries and
 * expect 0, where the emitted value plays no part.
 */
/* c7's alternatives are a colour selector plus a gap test. The selector is
 * waived as in BELTHOLD; the gap test is the term the boundary flip attacks. */
static void arm_kicking( int i, int cond, int arm, int *a )
{
   if( cond != 7 ) return;
   if( arm == 0 ) { a[0] = !pb_white(i-1); a[1] = pbL[i] > pbH[i-1]; }
   else           { a[0] =  pb_white(i-1); a[1] = pbH[i] < pbL[i-1]; }
}

static void build_kicking( void )
{
  pb_conditions(8);
  pb_signs(2);
  pb_arm(7,0,2); pb_arm(7,1,2);
  pb_arm_model(arm_kicking);
  pb_waive_arm(7,0,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(7,1,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);               /* 1st: black marubozu, body 12, range 13.125 */
  int d=pb_bar(107,112.5,107,112);       /* 2nd: white marubozu, body 5, gaps up */
  pb_detect(d,100,"detect: opposite marubozus, 2nd gaps up over 106.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f0=pb_bar(112,112.5,107,107.5);      /* BLACK like the 1st, low 107 keeps the gap */
  pb_flip(f0,0,"break c0: both candles are black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k0=pb_bar(107,112.5,107,112);
  pb_control(k0,100,0,"restore c0: opposite colours");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,96.5,93.5,94);                 /* 1st body 2 == avg at i-1 */
  int f1=pb_bar(107,112.5,107,112);
  pb_flip(f1,1,"break c1: first body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k1=pb_bar(107,112.5,107,112);
  pb_control(k1,100,1,"restore c1: first body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,107,93.5,94);                 /* 1st upper shadow 1 == avg at i-1 */
  int f2=pb_bar(108,113,107.5,112.5);
  pb_flip(f2,2,"break c2: first upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k2=pb_bar(107,112.5,107,112);
  pb_control(k2,100,2,"restore c2: first upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93,94);                 /* 1st lower shadow 1 == avg at i-1 */
  int f3=pb_bar(107,112.5,107,112);
  pb_flip(f3,3,"break c3: first lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k3=pb_bar(107,112.5,107,112);
  pb_control(k3,100,3,"restore c3: first lower shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f4=pb_bar(107,110.5,107,110);      /* 2nd body 3 == avg at i */
  pb_flip(f4,4,"break c4: second body 3 == avg 3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k4=pb_bar(107,112.5,107,112);
  pb_control(k4,100,4,"restore c4: second body 5 > 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f5=pb_bar(107,113.03125,107,112);     /* 2nd upper shadow 1.03125 == avg at i */
  pb_flip(f5,5,"break c5: second upper shadow 1.03125 == avg 1.03125, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k5=pb_bar(107,112.5,107,112);
  pb_control(k5,100,5,"restore c5: second upper shadow 0.5 < 1.03125");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f6=pb_bar(108,112,106.96875,111.5);     /* 2nd lower shadow 1.03125 == avg at i */
  pb_flip(f6,6,"break c6: second lower shadow 1.03125 == avg 1.03125, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k6=pb_bar(107,112.5,107,112);
  pb_control(k6,100,6,"restore c6: second lower shadow 0.5 < 1.03125");
  pb_flat(8);

  /* c7 moves the SECOND bar, never the first. The first bar's range is what
   * sets avg(ShadowVeryShort, i) -- dropping its high from 106.5 to 106 shifts
   * that threshold from 1.03125 to 1.02625, which would quietly move c5's and c6's
   * flips off the boundary they are placed on a few lines above. Same coupling
   * as the prior bar entering the window at i: changing one bar's geometry
   * re-prices the other bar's conditions, and nothing goes red when it does. */
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f7=pb_bar(107,112.5,106.5,112);    /* low 106.5 == prior high: no gap */
  pb_flip_in(f7,7,0,1,"break c7 alt0 term1: low(i) 106.5 == high(i-1) 106.5, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k7=pb_bar(107,112.5,107,112);        /* low 107 clears the prior high 106.5 */
  pb_control(k7,100,7,"restore c7: low 107 > prior high 106.5");
  pb_flat(8);
  /* The other output class: a WHITE first marubozu with a black second one
   * gapping DOWN -- c7's second disjunct. Both bodies keep the first longer,
   * which is also what will separate KICKINGBYLENGTH from this function. */
  pb_primer(12,100,2,4);
  pb_bar(94,106.5,93.5,106);               /* 1st: white marubozu, body 12, shadows 0.5 */
  int db=pb_bar(92,92.5,86,86.5);          /* 2nd: black marubozu, gaps down under 93.5 */
  pb_detect(db,-100,"detect white-first: opposite marubozus, 2nd gaps down under 93.5");
  pb_flat(8);

  /* The white-first arm's gap term, on its own boundary: high(i) exactly at
   * low(i-1) makes candlegapdown false while the colour selector stays true.
   * On the SAME first bar as the detect, so the control below differs from this
   * flip in one number -- the second bar's high -- rather than in a bespoke
   * first bar as well. A control that is not its flip's minimal pair cannot
   * show which of the two bars carried the difference. */
  pb_primer(12,100,2,4);
  pb_bar(94,106.5,93.5,106);
  int f7b=pb_bar(93,93.5,88,88.5);         /* high 93.5 == prior low 93.5: no gap */
  pb_flip_in(f7b,7,1,1,"break c7 alt1 term1: high(i) 93.5 == low(i-1) 93.5, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(94,106.5,93.5,106);
  int k7b=pb_bar(93,93.25,88,88.5);        /* only the high moves: 93.25 < 93.5 */
  pb_control(k7b,-100,7,"restore c7 alt1: high 93.25 < prior low 93.5");
  pb_flat(8);
}
static void cond_kickingbylength( int i, int *c )
{
   double vs1 = pb_avg(TA_ShadowVeryShort, i-1);
   double vs0 = pb_avg(TA_ShadowVeryShort, i);
   c[0] = pb_white(i-1) != pb_white(i);
   c[1] = pb_body(i-1) >  pb_avg(TA_BodyLong, i-1);
   c[2] = pb_upsh(i-1) <  vs1;
   c[3] = pb_losh(i-1) <  vs1;
   c[4] = pb_body(i)   >  pb_avg(TA_BodyLong, i);
   c[5] = pb_upsh(i)   <  vs0;
   c[6] = pb_losh(i)   <  vs0;
   c[7] = ( !pb_white(i-1) && pbL[i] > pbH[i-1] )
       || (  pb_white(i-1) && pbH[i] < pbL[i-1] );
}

/* The scenario the two KICKINGs share. CDLKICKING reports this candle's
 * colour, CDLKICKINGBYLENGTH the colour of whichever body is longer, so a
 * builder whose longer body is always the SAME candle cannot tell that
 * comparison from a constant. The detects here keep the first body longer
 * (12 against 5), which is what makes the two functions disagree and gives each
 * its own expected value; CDLKICKINGBYLENGTH carries one more with the second
 * body longer, so its selector is reached on both branches. The flips do not
 * all preserve that ordering -- they are placed on their own boundaries and
 * expect 0, where the emitted value plays no part.
 */
/* c7's alternatives are a colour selector plus a gap test. The selector is
 * waived as in BELTHOLD; the gap test is the term the boundary flip attacks. */
static void arm_kickingbylength( int i, int cond, int arm, int *a )
{
   if( cond != 7 ) return;
   if( arm == 0 ) { a[0] = !pb_white(i-1); a[1] = pbL[i] > pbH[i-1]; }
   else           { a[0] =  pb_white(i-1); a[1] = pbH[i] < pbL[i-1]; }
}

static void build_kickingbylength( void )
{
  pb_conditions(8);
  pb_signs(2);
  pb_arm(7,0,2); pb_arm(7,1,2);
  pb_arm_model(arm_kickingbylength);
  pb_waive_arm(7,0,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(7,1,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);               /* 1st: black marubozu, body 12, range 13.125 */
  int d=pb_bar(107,112.5,107,112);       /* 2nd: white marubozu, body 5, gaps up */
  pb_detect(d,-100,"detect: opposite marubozus, 2nd gaps up over 106.5");
  pb_flat(8);

  /* The one scenario CDLKICKING has no use for, and the only thing separating
   * this function from it: the SECOND body is the longer one, so the length
   * comparison has to take its other branch. Without this every scenario keeps
   * the first body longer, the selector always resolves to i-1, and deleting
   * the comparison outright -- reporting i-1's colour unconditionally -- passes
   * the whole suite. Here the longer body is white, so this reports +100 where
   * a deleted comparison would report the first candle's black -100. */
  pb_primer(12,100,2,4);
  pb_bar(97,97.5,93.375,94);               /* 1st: black marubozu, body 3 */
  int dl=pb_bar(98,110.5,98,110);          /* 2nd: white marubozu, body 12, gaps up over 97.5 */
  pb_detect(dl,100,"detect longer-second: the 2nd body 12 beats the 1st body 3, so the sign follows the 2nd");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f0=pb_bar(112,112.5,107,107.5);      /* BLACK like the 1st, low 107 keeps the gap */
  pb_flip(f0,0,"break c0: both candles are black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k0=pb_bar(107,112.5,107,112);
  pb_control(k0,-100,0,"restore c0: opposite colours");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(96,96.5,93.5,94);                 /* 1st body 2 == avg at i-1 */
  int f1=pb_bar(107,112.5,107,112);
  pb_flip(f1,1,"break c1: first body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k1=pb_bar(107,112.5,107,112);
  pb_control(k1,-100,1,"restore c1: first body 12 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,107,93.5,94);                 /* 1st upper shadow 1 == avg at i-1 */
  int f2=pb_bar(108,113,107.5,112.5);
  pb_flip(f2,2,"break c2: first upper shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k2=pb_bar(107,112.5,107,112);
  pb_control(k2,-100,2,"restore c2: first upper shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93,94);                 /* 1st lower shadow 1 == avg at i-1 */
  int f3=pb_bar(107,112.5,107,112);
  pb_flip(f3,3,"break c3: first lower shadow 1 == avg 1, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k3=pb_bar(107,112.5,107,112);
  pb_control(k3,-100,3,"restore c3: first lower shadow 0.5 < 1");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f4=pb_bar(107,110.5,107,110);      /* 2nd body 3 == avg at i */
  pb_flip(f4,4,"break c4: second body 3 == avg 3, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k4=pb_bar(107,112.5,107,112);
  pb_control(k4,-100,4,"restore c4: second body 5 > 3");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f5=pb_bar(107,113.03125,107,112);     /* 2nd upper shadow 1.03125 == avg at i */
  pb_flip(f5,5,"break c5: second upper shadow 1.03125 == avg 1.03125, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k5=pb_bar(107,112.5,107,112);
  pb_control(k5,-100,5,"restore c5: second upper shadow 0.5 < 1.03125");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f6=pb_bar(108,112,106.96875,111.5);     /* 2nd lower shadow 1.03125 == avg at i */
  pb_flip(f6,6,"break c6: second lower shadow 1.03125 == avg 1.03125, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k6=pb_bar(107,112.5,107,112);
  pb_control(k6,-100,6,"restore c6: second lower shadow 0.5 < 1.03125");
  pb_flat(8);

  /* c7 moves the SECOND bar, never the first. The first bar's range is what
   * sets avg(ShadowVeryShort, i) -- dropping its high from 106.5 to 106 shifts
   * that threshold from 1.03125 to 1.02625, which would quietly move c5's and c6's
   * flips off the boundary they are placed on a few lines above. Same coupling
   * as the prior bar entering the window at i: changing one bar's geometry
   * re-prices the other bar's conditions, and nothing goes red when it does. */
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int f7=pb_bar(107,112.5,106.5,112);    /* low 106.5 == prior high: no gap */
  pb_flip_in(f7,7,0,1,"break c7 alt0 term1: low(i) 106.5 == high(i-1) 106.5, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(106,106.5,93.375,94);
  int k7=pb_bar(107,112.5,107,112);        /* low 107 clears the prior high 106.5 */
  pb_control(k7,-100,7,"restore c7: low 107 > prior high 106.5");
  pb_flat(8);
  /* The other output class: a WHITE first marubozu with a black second one
   * gapping DOWN -- c7's second disjunct. Both bodies keep the first longer,
   * which is also what will separate KICKINGBYLENGTH from this function. */
  pb_primer(12,100,2,4);
  pb_bar(94,106.5,93.5,106);               /* 1st: white marubozu, body 12, shadows 0.5 */
  int db=pb_bar(92,92.5,86,86.5);          /* 2nd: black marubozu, gaps down under 93.5 */
  pb_detect(db,100,"detect white-first: opposite marubozus, 2nd gaps down under 93.5");
  pb_flat(8);

  /* The white-first arm's gap term, on its own boundary: high(i) exactly at
   * low(i-1) makes candlegapdown false while the colour selector stays true.
   * On the SAME first bar as the detect, so the control below differs from this
   * flip in one number -- the second bar's high -- rather than in a bespoke
   * first bar as well. A control that is not its flip's minimal pair cannot
   * show which of the two bars carried the difference. */
  pb_primer(12,100,2,4);
  pb_bar(94,106.5,93.5,106);
  int f7b=pb_bar(93,93.5,88,88.5);         /* high 93.5 == prior low 93.5: no gap */
  pb_flip_in(f7b,7,1,1,"break c7 alt1 term1: high(i) 93.5 == low(i-1) 93.5, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(94,106.5,93.5,106);
  int k7b=pb_bar(93,93.25,88,88.5);        /* only the high moves: 93.25 < 93.5 */
  pb_control(k7b,100,7,"restore c7 alt1: high 93.25 < prior low 93.5");
  pb_flat(8);
}

/* CDL3INSIDE -- a harami that resolves: a long body, a short one engulfed by
 * it, then a third candle of the first's opposite colour closing past the
 * first's open.
 *
 *   c0  bodyhi(i-1) < bodyhi(i-2)
 *   c1  bodylo(i-1) > bodylo(i-2)
 *   c2  ( white(i-2) && black(i) && close(i) < open(i-2) )
 *       || ( black(i-2) && white(i) && close(i) > open(i-2) )
 *   c3  realbody(i-2) > avg(BodyLong, i-2)
 *   c4  realbody(i-1) <= avg(BodyShort, i-1)
 *
 * The first pattern here whose disjuncts carry THREE terms rather than two, so
 * the arm axis has real work: past the waived colour selector, each arm still
 * holds the third candle's colour and its close-out, and the two have to be
 * broken one at a time. They are not independent by construction -- turning the
 * third candle white to break the colour term will also lift its close above
 * open(i-2) unless the whole candle is moved down first, which is why the two
 * flips below sit at different price levels rather than being one bar edited
 * two ways.
 *
 * c3 and c4 read averages over two different windows, and the first candle's
 * body is inside the second window: avg(BodyShort, i-1) is (9*primer + body(i-2))/10.
 * So c3's flip, which shortens the first body, moves c4's threshold as well.
 * The second body is kept far below that threshold in that scenario so the
 * move cannot break c4 as collateral.
 *
 * c4 is the inclusive site and cannot be pinned by a flip -- there is no
 * minimal violating value above a <= -- so the control sits exactly on the
 * edge instead, at a second body of 3.0 against a threshold of 3.0.
 */
static void cond_3inside( int i, int *c )
{
   c[0] = pb_bodyhi(i-1) < pb_bodyhi(i-2);
   c[1] = pb_bodylo(i-1) > pb_bodylo(i-2);
   c[2] = (  pb_white(i-2) && !pb_white(i) && pbC[i] < pbO[i-2] )
       || ( !pb_white(i-2) &&  pb_white(i) && pbC[i] > pbO[i-2] );
   c[3] = pb_body(i-2) >  pb_avg(TA_BodyLong,  i-2);
   c[4] = pb_body(i-1) <= pb_avg(TA_BodyShort, i-1);
}

static void arm_3inside( int i, int cond, int arm, int *a )
{
   if( cond != 2 ) return;
   if( arm == 0 ) { a[0] =  pb_white(i-2); a[1] = !pb_white(i); a[2] = pbC[i] < pbO[i-2]; }
   else           { a[0] = !pb_white(i-2); a[1] =  pb_white(i); a[2] = pbC[i] > pbO[i-2]; }
}

static void build_3inside( void )
{
  pb_conditions(5);
  pb_signs(2);
  pb_arm(2,0,3); pb_arm(2,1,3);
  pb_arm_model(arm_3inside);
  pb_waive_arm(2,0,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(2,1,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");

  /* avg(BodyLong, i-2) is 2 -- ten primer bars. avg(BodyShort, i-1) is
   * (9*2 + body(i-2))/10, which is 3.0 exactly while the first body is 12. */
  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);              /* 1st: white, body 12 > 2 */
  pb_bar(105,106.5,104.5,106);             /* 2nd: body 1 <= 3, inside 100..112 */
  int d=pb_bar(99,99.5,94.5,95);           /* 3rd: black, closes 95 below open 100 */
  pb_detect(d,-100,"detect: white long, engulfed short, black closing out below");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(111,112.5,110.5,112);             /* 2nd top 112 == 1st top */
  int f0=pb_bar(99,99.5,94.5,95);
  pb_flip(f0,0,"break c0: 2nd body top 112 == 1st body top, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(111,112.5,110.5,111.5);           /* top 111.5, one step inside */
  int k0=pb_bar(99,99.5,94.5,95);
  pb_control(k0,-100,0,"restore c0: 2nd body top 111.5 < 112");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(100,101.5,99.5,101);              /* 2nd bottom 100 == 1st bottom */
  int f1=pb_bar(99,99.5,94.5,95);
  pb_flip(f1,1,"break c1: 2nd body bottom 100 == 1st body bottom, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(100.5,101.5,99.5,101);
  int k1=pb_bar(99,99.5,94.5,95);
  pb_control(k1,-100,1,"restore c1: 2nd body bottom 100.5 > 100");
  pb_flat(8);

  /* The third candle's colour, with the close-out held: moving the whole
   * candle down to 94..96 keeps close below open(i-2) while it turns white. */
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(105,106.5,104.5,106);
  int f2a=pb_bar(94,96.5,93.5,96);
  pb_flip_in(f2a,2,0,1,"break c2 alt0 term1: 3rd is white, and still closes at 96 below open 100");
  pb_flat(8);
  /* The close-out, with the colour held: black, but closing exactly ON
   * open(i-2) rather than below it. */
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(105,106.5,104.5,106);
  int f2b=pb_bar(105,105.5,99.5,100);
  pb_flip_in(f2b,2,0,2,"break c2 alt0 term2: 3rd is black but closes at 100 == open(i-2), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(105,106.5,104.5,106);
  int k2=pb_bar(105,105.5,99,99.5);
  pb_control(k2,-100,2,"restore c2: 3rd black closing at 99.5 below 100");
  pb_flat(8);

  /* c3 shortens the first body onto its threshold. That also drops
   * avg(BodyShort, i-1) from 3.0 to 2.0, so the second body is 0.5 here --
   * well under either -- and c4 survives the move. */
  pb_primer(12,100,2,4);
  pb_bar(100,102.5,99.5,102);              /* 1st body 2 == avg 2 */
  pb_bar(101,101.8,100.7,101.5);           /* body 0.5, inside 100..102 */
  int f3=pb_bar(99,99.5,94.5,95);
  pb_flip(f3,3,"break c3: 1st body 2 == avg 2, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,103,99.5,102.5);              /* 1st body 2.5 */
  pb_bar(101,101.8,100.7,101.5);
  int k3=pb_bar(99,99.5,94.5,95);
  pb_control(k3,-100,3,"restore c3: 1st body 2.5 > 2");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(105,109.5,104.5,109);             /* 2nd body 4 > 3 */
  int f4=pb_bar(99,99.5,94.5,95);
  pb_flip(f4,4,"break c4: 2nd body 4 > avg 3");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,112.5,99.5,112);
  pb_bar(105,108.5,104.5,108);             /* 2nd body 3.0 == avg 3.0 */
  int k4=pb_bar(99,99.5,94.5,95);
  pb_control(k4,-100,4,"restore c4 on the edge: 2nd body 3.0 == avg 3.0, and <= admits it");
  pb_flat(8);

  /* The BLACK-first arm. Everything above is white-first, which leaves the
   * other half of c2 and the +100 class untouched. */
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);              /* 1st: black, body 12 */
  pb_bar(106,106.5,104.5,105);             /* 2nd: body 1, inside 100..112 */
  int db=pb_bar(113,118.5,112.5,118);      /* 3rd: white, closes 118 above open 112 */
  pb_detect(db,100,"detect black-first: the mirror, output +100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(106,106.5,104.5,105);
  int f2c=pb_bar(120,120.5,117.5,118);
  pb_flip_in(f2c,2,1,1,"break c2 alt1 term1: 3rd is black, and still closes at 118 above open 112");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(106,106.5,104.5,105);
  int f2d=pb_bar(110,112.5,109.5,112);
  pb_flip_in(f2d,2,1,2,"break c2 alt1 term2: 3rd is white but closes at 112 == open(i-2), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(106,106.5,104.5,105);
  int k2b=pb_bar(112,113,109.5,112.5);
  pb_control(k2b,100,2,"restore c2 black-first: 3rd white closing at 112.5 above 112");
  pb_flat(8);
}

/* CDLSTICKSANDWICH -- two black candles closing at the same price with a
 * white one trapped between them.
 *
 *   c0  black(i-2)
 *   c1  white(i-1)
 *   c2  black(i)
 *   c3  low(i-1) > close(i-2)
 *   c4  close(i) <= close(i-2) + avg(Equal, i-2)
 *   c5  close(i) >= close(i-2) - avg(Equal, i-2)
 *
 * avg(Equal) is 0.05 of a HighLow range over five bars, and it is read at i-2,
 * so it depends on the five PRIMER bars alone -- none of the three pattern
 * candles is inside that window. The primers give a range of 10, so the band
 * is 0.5 wide and its edges land on 100.5 and 99.5 exactly. That is what makes
 * c4 and c5 pinnable: both are inclusive sites, neither can be pinned by a
 * flip, and each has a control sitting exactly on its edge instead.
 *
 * Because eq does not depend on the pattern candles, the c0 scenario can turn
 * the first candle white while leaving its CLOSE at 100 -- so c3, c4 and c5 all
 * keep reading the same numbers and only the colour moves. The minimal white
 * candle is a doji, since the library's colour test is close >= open, so the
 * flip is open == close rather than an obviously white body.
 */
static void cond_sticksandwich( int i, int *c )
{
   double eq = pb_avg(TA_Equal, i-2);
   c[0] = !pb_white(i-2);
   c[1] =  pb_white(i-1);
   c[2] = !pb_white(i);
   c[3] = pbL[i-1] > pbC[i-2];
   c[4] = pbC[i] <= pbC[i-2] + eq;
   c[5] = pbC[i] >= pbC[i-2] - eq;
}

static void build_sticksandwich( void )
{
  pb_conditions(6);

  pb_flat(6);
  pb_primer(12,100,2,4);                   /* HighLow range 10 -> avg(Equal) 0.5 */
  pb_bar(112,112.5,99.5,100);              /* 1st: black, closes 100 */
  pb_bar(102,106.5,101.5,106);             /* 2nd: white, low 101.5 above 100 */
  int d=pb_bar(105,105.5,99,100);          /* 3rd: black, closes 100 -- dead centre */
  pb_detect(d,100,"detect: black/white/black, 3rd closing on the 1st's close");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,100.5,99.5,100);              /* 1st is a doji: close == open is WHITE */
  pb_bar(102,106.5,101.5,106);
  int f0=pb_bar(105,105.5,99,100);
  pb_flip(f0,0,"break c0: 1st is a doji, and close >= open counts as white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100.5,100.5,99.5,100);            /* one step black, same close */
  pb_bar(102,106.5,101.5,106);
  int k0=pb_bar(105,105.5,99,100);
  pb_control(k0,100,0,"restore c0: open 100.5 above close 100, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(106,106.5,101.5,102);             /* 2nd black, low unchanged at 101.5 */
  int f1=pb_bar(105,105.5,99,100);
  pb_flip(f1,1,"break c1: 2nd is black, its low still above the 1st close");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,102);             /* doji: white by the same rule as c0 */
  int k1=pb_bar(105,105.5,99,100);
  pb_control(k1,100,1,"restore c1: 2nd is a doji, which is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,106);
  int f2=pb_bar(100,100.5,99.5,100);       /* 3rd doji -> white, close still 100 */
  pb_flip(f2,2,"break c2: 3rd is a doji, and close >= open counts as white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,106);
  int k2=pb_bar(100.5,100.5,99.5,100);
  pb_control(k2,100,2,"restore c2: open 100.5 above close 100, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,100,106);               /* 2nd low 100 == the 1st close */
  int f3=pb_bar(105,105.5,99,100);
  pb_flip(f3,3,"break c3: 2nd low 100 == the 1st close, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,100.5,106);
  int k3=pb_bar(105,105.5,99,100);
  pb_control(k3,100,3,"restore c3: 2nd low 100.5 > 100");
  pb_flat(8);

  /* c4 and c5 are the band's two inclusive edges. A flip can only land clear of
   * the edge, never on it; the controls are what sit on 100.5 and 99.5. */
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,106);
  int f4=pb_bar(105,105.5,99,100.75);      /* above the band top 100.5 */
  pb_flip(f4,4,"break c4: 3rd closes 100.75, above the band top 100.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,106);
  int k4=pb_bar(105,105.5,99,100.5);
  pb_control(k4,100,4,"restore c4 on the edge: 3rd closes 100.5 == the band top, and <= admits it");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,106);
  int f5=pb_bar(105,105.5,99,99.25);       /* below the band bottom 99.5 */
  pb_flip(f5,5,"break c5: 3rd closes 99.25, below the band bottom 99.5");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(112,112.5,99.5,100);
  pb_bar(102,106.5,101.5,106);
  int k5=pb_bar(105,105.5,99,99.5);
  pb_control(k5,100,5,"restore c5 on the edge: 3rd closes 99.5 == the band bottom, and >= admits it");
  pb_flat(8);
}

/* CDLXSIDEGAP3METHODS -- two candles of one colour with a real-body gap
 * between them, then a third of the opposite colour reaching back across it.
 *
 *   c0  color(i-2) == color(i-1)
 *   c1  color(i-1) == -color(i)
 *   c2  open(i)  < bodyhi(i-1)
 *   c3  open(i)  > bodylo(i-1)
 *   c4  close(i) < bodyhi(i-2)
 *   c5  close(i) > bodylo(i-2)
 *   c6  ( white(i-2) && realbodygapup(i-1 over i-2) )
 *       || ( black(i-2) && realbodygapdown(i-1 under i-2) )
 *
 * c1 is waived, and the argument is the reason this pattern needs one at all.
 * c2 and c3 put open(i) inside the 2nd real body, c4 and c5 put close(i)
 * inside the 1st, and c6 puts one body entirely clear of the other. In the
 * white arm that chains to
 *
 *    open(i) > bodylo(i-1) > bodyhi(i-2) > close(i)
 *
 * so open(i) > close(i) and the 3rd candle is black by construction; the black
 * arm is the mirror and forces it white. Either way color(i) == -color(i-2),
 * so c1 reads color(i-1) == color(i-2), which is c0 written out again. Exactly
 * one of the two carries information once the others hold, and the third
 * candle's colour is the one the geometry already decided.
 *
 * A search over a coarse grid agrees: of 531441 assignments there is not one
 * that falsifies c0 alone, and not one that falsifies c1 alone.
 *
 * The colour conditions here have no boundary case to sit on. A doji would be
 * the minimal white candle, but c2 and c3 need the 2nd real body to be wide
 * enough to hold open(i) strictly inside it, so a degenerate body breaks them
 * instead. The c0 flip is an ordinary black body, and it is the one flip in
 * this builder that is not pinned to an edge.
 */
static void cond_xsidegap3methods( int i, int *c )
{
   c[0] = pb_white(i-2) == pb_white(i-1);
   c[1] = pb_white(i-1) != pb_white(i);
   c[2] = pbO[i] < pb_bodyhi(i-1);
   c[3] = pbO[i] > pb_bodylo(i-1);
   c[4] = pbC[i] < pb_bodyhi(i-2);
   c[5] = pbC[i] > pb_bodylo(i-2);
   c[6] = (  pb_white(i-2) && pb_bodylo(i-1) > pb_bodyhi(i-2) )
       || ( !pb_white(i-2) && pb_bodyhi(i-1) < pb_bodylo(i-2) );
}

static void arm_xsidegap3methods( int i, int cond, int arm, int *a )
{
   if( cond != 6 ) return;
   if( arm == 0 ) { a[0] =  pb_white(i-2); a[1] = pb_bodylo(i-1) > pb_bodyhi(i-2); }
   else           { a[0] = !pb_white(i-2); a[1] = pb_bodyhi(i-1) < pb_bodylo(i-2); }
}

static void build_xsidegap3methods( void )
{
  pb_conditions(7);
  pb_signs(2);
  pb_arm(6,0,2); pb_arm(6,1,2);
  pb_arm_model(arm_xsidegap3methods);
  /* c0 and c1 are the same predicate once the others hold, so NEITHER can be
   * broken alone and both are waived. Deleting either conjunct from the library
   * would preserve the function exactly; deleting BOTH would not, and that is
   * what the zero-valued case below is for -- it is not an MC/DC case and makes
   * no independence claim, it just refuses to let the pair vanish unnoticed. */
  pb_waive(0, "the mirror of c1's waiver: given c2..c6 the 3rd candle's colour is fixed at "
              "-color(i-2), so c1 reads color(i-1) == color(i-2), which is c0. The two are "
              "one predicate written twice and neither can be falsified without the other");
  pb_waive(1, "c2 and c3 put open(i) inside the 2nd real body, c4 and c5 put close(i) "
              "inside the 1st, and c6 separates the two bodies. In the white arm that "
              "gives open(i) > bodylo(i-1) > bodyhi(i-2) > close(i), so the 3rd candle "
              "is black by construction, and the black arm forces it white. So "
              "color(i) == -color(i-2) always, and c1 restates c0");
  pb_waive_arm(6,0,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");
  pb_waive_arm(6,1,0,"the arm's own colour selector -- it chooses the arm, and the class it chooses is fired by pb_signs(2)");

  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);              /* 1st: white, body 100..105 */
  pb_bar(110,115.5,109.5,115);             /* 2nd: white, body 110..115, clear above */
  int d=pb_bar(113,113.5,101.5,102);       /* 3rd: black, opens in the 2nd, closes in the 1st */
  pb_detect(d,100,"detect: two whites gapped apart, a black reaching back across");
  pb_flat(8);

  /* The 2nd candle black: c0 and c1 both false, c2..c6 all true. Not a flip --
   * it breaks two conditions and the harness is right to refuse it as one --
   * but it is the case that fails if both conjuncts are removed. */
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(115,115.5,109.5,110);             /* 2nd black: same body extent, gap intact */
  int z0=pb_bar(113,113.5,101.5,102);
  pb_expect(z0,0,"the colour pair is load-bearing: 2nd black, everything else intact");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int f2=pb_bar(115,115.5,101.5,102);      /* open 115 == bodyhi(i-1) */
  pb_flip(f2,2,"break c2: open 115 == the 2nd body top, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int k2=pb_bar(114.5,114.5,101.5,102);
  pb_control(k2,100,2,"restore c2: open 114.5 < 115");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int f3=pb_bar(110,110.5,101.5,102);      /* open 110 == bodylo(i-1) */
  pb_flip(f3,3,"break c3: open 110 == the 2nd body bottom, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int k3=pb_bar(110.5,110.5,101.5,102);
  pb_control(k3,100,3,"restore c3: open 110.5 > 110");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int f4=pb_bar(113,113.5,104.5,105);      /* close 105 == bodyhi(i-2) */
  pb_flip(f4,4,"break c4: close 105 == the 1st body top, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int k4=pb_bar(113,113.5,104,104.5);
  pb_control(k4,100,4,"restore c4: close 104.5 < 105");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int f5=pb_bar(113,113.5,99.5,100);       /* close 100 == bodylo(i-2) */
  pb_flip(f5,5,"break c5: close 100 == the 1st body bottom, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(110,115.5,109.5,115);
  int k5=pb_bar(113,113.5,100,100.5);
  pb_control(k5,100,5,"restore c5: close 100.5 > 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(105,115.5,104.5,115);             /* 2nd body bottom 105 == the 1st body top */
  int f6=pb_bar(113,113.5,101.5,102);
  pb_flip_in(f6,6,0,1,"break c6 alt0 term1: the 2nd body bottom 105 == the 1st body top, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,105.5,99.5,105);
  pb_bar(105.5,115.5,105,115);
  int k6=pb_bar(113,113.5,101.5,102);
  pb_control(k6,100,6,"restore c6: the 2nd body bottom 105.5 > 105");
  pb_flat(8);

  /* The BLACK arm: the gap points the other way and the output flips sign. */
  pb_primer(12,100,2,4);
  pb_bar(105,105.5,99.5,100);              /* 1st: black, body 100..105 */
  pb_bar(95,95.5,89.5,90);                 /* 2nd: black, body 90..95, clear below */
  int db=pb_bar(93,102.5,92.5,102);        /* 3rd: white, opens in the 2nd, closes in the 1st */
  pb_detect(db,-100,"detect black-first: the mirror, output -100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(105,105.5,99.5,100);
  pb_bar(100,100.5,89.5,90);               /* 2nd body top 100 == the 1st body bottom */
  int f6b=pb_bar(93,102.5,92.5,102);
  pb_flip_in(f6b,6,1,1,"break c6 alt1 term1: the 2nd body top 100 == the 1st body bottom, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(105,105.5,99.5,100);
  pb_bar(99.5,99.5,89.5,90);
  int k6b=pb_bar(93,102.5,92.5,102);
  pb_control(k6b,-100,6,"restore c6 black arm: the 2nd body top 99.5 < 100");
  pb_flat(8);
}

/* CDLCONCEALBABYSWALL -- four black candles: two marubozus, a third that gaps
 * down but throws a shadow back up into the second's body, and a fourth that
 * swallows the third whole.
 *
 *   c0..c3   black(i-3), black(i-2), black(i-1), black(i)
 *   c4,c5    the 1st candle's two shadows < avg(ShadowVeryShort, i-3)
 *   c6,c7    the 2nd candle's two shadows < avg(ShadowVeryShort, i-2)
 *   c8       bodyhi(i-1) < bodylo(i-2)                     -- gap down
 *   c9       uppershadow(i-1) > avg(ShadowVeryShort, i-1)  -- but a real shadow
 *   c10      high(i-1) > close(i-2)                        -- reaching the body
 *   c11,c12  high(i) > high(i-1) and low(i) < low(i-1)     -- engulfed
 *
 * Thirteen conditions, and the settings average is read at THREE different
 * lags -- i-3, i-2 and i-1 -- off a rolling array the library carries per bar.
 * The windows overlap: the one at i-2 contains the 1st candle, the one at i-1
 * contains the 1st and the 2nd.
 *
 * The three thresholds are 1.0, 1.25 and 1.5 here, and they are DIFFERENT ON
 * PURPOSE. My first draft gave the first two candles the primers' own range,
 * which left all three averages at 1.0 -- every flip still pinned, every
 * condition still covered, and the gate completely blind to which window each
 * test reads. Rewiring c6 to read the i-3 window, or c9 to read the i-2 one,
 * changed six and three call sites and the suite stayed green. Equal
 * thresholds make the wiring unobservable no matter how exact they are.
 *
 * Giving the 1st and 2nd candles a HighLow range of 35 against the primers' 10
 * separates them: avg(i-3) stays 1.0 on primers alone, avg(i-2) becomes 1.25,
 * avg(i-1) becomes 1.5. All three are dyadic, so a shadow can still be placed
 * bitwise ON each of them -- a threshold that only prints as a short decimal is
 * not enough, since the shadow is a difference of two prices and has to land on
 * the same double the library computes.
 *
 * The controls for c6 and c7 sit at 1.0, which is deliberately the value of the
 * NEIGHBOURING window: they fire only if the test reads i-2, and fail if it
 * reads i-3.
 *
 * The four colour flips are dojis, and a doji cannot hold a range of 35 while
 * keeping both shadows small, so those scenarios do move the later thresholds.
 * Everything reading them has slack far wider than the move.
 */
static void cond_concealbabyswall( int i, int *c )
{
   c[0]  = !pb_white(i-3);
   c[1]  = !pb_white(i-2);
   c[2]  = !pb_white(i-1);
   c[3]  = !pb_white(i);
   c[4]  = pb_losh(i-3) < pb_avg(TA_ShadowVeryShort, i-3);
   c[5]  = pb_upsh(i-3) < pb_avg(TA_ShadowVeryShort, i-3);
   c[6]  = pb_losh(i-2) < pb_avg(TA_ShadowVeryShort, i-2);
   c[7]  = pb_upsh(i-2) < pb_avg(TA_ShadowVeryShort, i-2);
   c[8]  = pb_bodyhi(i-1) < pb_bodylo(i-2);
   c[9]  = pb_upsh(i-1) > pb_avg(TA_ShadowVeryShort, i-1);
   c[10] = pbH[i-1] > pbC[i-2];
   c[11] = pbH[i] > pbH[i-1];
   c[12] = pbL[i] < pbL[i-1];
}

static void build_concealbabyswall( void )
{
  pb_conditions(13);

  pb_flat(6);
  pb_primer(12,100,2,4);                   /* primer range 10 -> avg(i-3) = 1.0 */
  pb_bar(135,135.5,100.5,101);            /* 1st: black marubozu, range 35 -> avg(i-2) = 1.25 */
  pb_bar(100,100.5,65.5,66);            /* 2nd: black marubozu, range 35 -> avg(i-1) = 1.5  */
  pb_bar(65,69,62.5,63);                 /* 3rd: gaps below 66, high 69 reaches back up */
  int d=pb_bar(70,70.5,60.5,61);        /* 4th: swallows the 3rd, shadows included */
  pb_detect(d,100,"detect: two marubozus, a gapped third with a shadow, a fourth engulfing it");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(105,105.5,104.5,105);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int f0=pb_bar(70,70.5,60.5,61);
  pb_flip(f0,0,"break c0: 1st is a doji, and close >= open counts as white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(105.5,105.5,104.5,105);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int k0=pb_bar(70,70.5,60.5,61);
  pb_control(k0,100,0,"restore c0: 1st open 105.5 above its close, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(67,67.5,66.5,67);
  pb_bar(65,69,62.5,63);
  int f1=pb_bar(70,70.5,60.5,61);
  pb_flip(f1,1,"break c1: 2nd is a doji, placed at 67 so the gap and the 3rd high both survive");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(67.5,67.5,66.5,67);
  pb_bar(65,69,62.5,63);
  int k1=pb_bar(70,70.5,60.5,61);
  pb_control(k1,100,1,"restore c1: 2nd open 67.5 above its close, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,65);
  int f2=pb_bar(70,70.5,60.5,61);
  pb_flip(f2,2,"break c2: 3rd is a doji, its body top and shadow unchanged");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65.5,69,62.5,65);
  int k2=pb_bar(70,70.5,60.5,61);
  pb_control(k2,100,2,"restore c2: 3rd open 65.5 above its close, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int f3=pb_bar(70,70.5,60.5,70);
  pb_flip(f3,3,"break c3: 4th is a doji, its high and low unchanged");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int k3=pb_bar(70.5,70.5,60.5,70);
  pb_control(k3,100,3,"restore c3: 4th open 70.5 above its close, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int f4=pb_bar(70,70.5,60.5,61);
  pb_flip(f4,4,"break c4: 1st lower shadow 1.0 == avg(i-3) 1.0, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.25,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int k4=pb_bar(70,70.5,60.5,61);
  pb_control(k4,100,4,"restore c4: 1st lower shadow 0.75 < 1.0");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,136,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int f5=pb_bar(70,70.5,60.5,61);
  pb_flip(f5,5,"break c5: 1st upper shadow 1.0 == avg(i-3) 1.0, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.75,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int k5=pb_bar(70,70.5,60.5,61);
  pb_control(k5,100,5,"restore c5: 1st upper shadow 0.75 < 1.0");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,64.75,66);
  pb_bar(65,69,62.5,63);
  int f6=pb_bar(70,70.5,60.5,61);
  pb_flip(f6,6,"break c6: 2nd lower shadow 1.25 == avg(i-2) 1.25, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65,66);
  pb_bar(65,69,62.5,63);
  int k6=pb_bar(70,70.5,60.5,61);
  pb_control(k6,100,6,"restore c6: 2nd lower shadow 1.0 < 1.25 -- and 1.0 is avg(i-3), so a window mix-up fails here");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,101.25,65.5,66);
  pb_bar(65,69,62.5,63);
  int f7=pb_bar(70,70.5,60.5,61);
  pb_flip(f7,7,"break c7: 2nd upper shadow 1.25 == avg(i-2) 1.25, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,101,65.5,66);
  pb_bar(65,69,62.5,63);
  int k7=pb_bar(70,70.5,60.5,61);
  pb_control(k7,100,7,"restore c7: 2nd upper shadow 1.0 < 1.25 -- likewise pinned between the two windows");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(66,69,62.5,63);
  int f8=pb_bar(70,70.5,60.5,61);
  pb_flip(f8,8,"break c8: 3rd body top 66 == the 2nd body bottom, the gap test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65.75,69,62.5,63);
  int k8=pb_bar(70,70.5,60.5,61);
  pb_control(k8,100,8,"restore c8: 3rd body top 65.75 < 66");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,66.5,62.5,63);
  int f9=pb_bar(70,70.5,60.5,61);
  pb_flip(f9,9,"break c9: 3rd upper shadow 1.5 == avg(i-1) 1.5, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,66.75,62.5,63);
  int k9=pb_bar(70,70.5,60.5,61);
  pb_control(k9,100,9,"restore c9: 3rd upper shadow 1.75 > 1.5");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(64,66,62.5,63);
  int f10=pb_bar(70,70.5,60.5,61);
  pb_flip(f10,10,"break c10: 3rd high 66 == the 2nd close, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(64,66.25,62.5,63);
  int k10=pb_bar(70,70.5,60.5,61);
  pb_control(k10,100,10,"restore c10: 3rd high 66.25 > the 2nd close 66");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int f11=pb_bar(69,69,60.5,61);
  pb_flip(f11,11,"break c11: 4th high 69 == the 3rd high, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int k11=pb_bar(69.25,69.25,60.5,61);
  pb_control(k11,100,11,"restore c11: 4th high 69.25 > 69");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int f12=pb_bar(70,70.5,62.5,63);
  pb_flip(f12,12,"break c12: 4th low 62.5 == the 3rd low, the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(135,135.5,100.5,101);
  pb_bar(100,100.5,65.5,66);
  pb_bar(65,69,62.5,63);
  int k12=pb_bar(70,70.5,62.25,63);
  pb_control(k12,100,12,"restore c12: 4th low 62.25 < 62.5");
  pb_flat(8);
}

/* CDL3OUTSIDE -- an engulfing that resolves: a real body swallowing the one
 * before it, then a third candle carrying on past the second's close. The last
 * candlestick without MC/DC coverage.
 *
 *   c0  ( white(i-1) && black(i-2) && close(i-1) > open(i-2)
 *                    && open(i-1) < close(i-2) && close(i) > close(i-1) )
 *    || ( black(i-1) && white(i-2) && open(i-1) > close(i-2)
 *                    && close(i-1) < open(i-2) && close(i) < close(i-1) )
 *
 * ONE top-level condition, so there is nothing for pb_flip to break: every flip
 * lives on the arm-term axis and the two output classes come from pb_signs(2).
 * That is CDLENGULFING's shape, with five terms per alternative instead of
 * three.
 *
 * The pattern reads NO candle setting -- there is not one TA_CANDLEAVERAGE in
 * it. Every threshold is therefore a price against another price, and an
 * equality between two short decimals is already exact, so each flip below sits
 * bitwise on its boundary with nothing to derive. This is the one builder in
 * the tier where that question does not arise.
 *
 * Each arm's colour selector is entailed, and waived on that ground. Term 1
 * makes the 1st candle black, so close(i-2) < open(i-2); terms 2 and 3 put
 * open(i-1) under close(i-2) and close(i-1) over open(i-2). Chaining them,
 *
 *      open(i-1) < close(i-2) < open(i-2) < close(i-1)
 *
 * so close(i-1) > open(i-1) and the 2nd candle is white by construction. The
 * black arm is the mirror. That colour is also the emitted value, so the class
 * each selector chooses is the one pb_signs(2) fires.
 *
 * Every control is its flip's minimal pair: one price moves, and it is the one
 * the flip pinned.
 */
static void cond_3outside( int i, int *c )
{
   c[0] = (  pb_white(i-1) && !pb_white(i-2) &&
             pbC[i-1] > pbO[i-2] && pbO[i-1] < pbC[i-2] && pbC[i] > pbC[i-1] )
       || ( !pb_white(i-1) &&  pb_white(i-2) &&
             pbO[i-1] > pbC[i-2] && pbC[i-1] < pbO[i-2] && pbC[i] < pbC[i-1] );
}

static void arm_3outside( int i, int cond, int arm, int *a )
{
   if( cond != 0 ) return;
   if( arm == 0 )
   {
      a[0] =  pb_white(i-1);
      a[1] = !pb_white(i-2);
      a[2] = pbC[i-1] > pbO[i-2];
      a[3] = pbO[i-1] < pbC[i-2];
      a[4] = pbC[i]   > pbC[i-1];
   }
   else
   {
      a[0] = !pb_white(i-1);
      a[1] =  pb_white(i-2);
      a[2] = pbO[i-1] > pbC[i-2];
      a[3] = pbC[i-1] < pbO[i-2];
      a[4] = pbC[i]   < pbC[i-1];
   }
}

static void build_3outside( void )
{
  pb_conditions(1);
  pb_signs(2);
  pb_arm(0,0,5); pb_arm(0,1,5);
  pb_arm_model(arm_3outside);
  pb_waive_arm(0,0,0,"entailed by terms 1..3: term 1 makes the 1st candle black, so close(i-2) < open(i-2), and terms 2 and 3 give open(i-1) < close(i-2) < open(i-2) < close(i-1), so the 2nd candle is white by construction. It is also the emitted colour, and that class is fired by pb_signs(2)");
  pb_waive_arm(0,1,0,"the mirror: term 1 makes the 1st candle white, so open(i-2) <= close(i-2), and terms 2 and 3 give close(i-1) < open(i-2) <= close(i-2) < open(i-1), so the 2nd candle is black by construction");

  /* The WHITE arm, output +100. The primer only supplies lookback here; no
   * average is read, so its geometry sets no threshold. */
  pb_flat(6);
  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);              /* 1st: black, body 110..100 */
  pb_bar(99,111.5,98.5,111);               /* 2nd: white, engulfing it at both ends */
  int d=pb_bar(112,115.5,111.5,115);       /* 3rd: closes 115, above the 2nd close 111 */
  pb_detect(d,100,"detect: a white body engulfing a black one, then a higher close");
  pb_flat(8);

  /* term1, the 1st candle's colour. The minimal violation is a doji, since the
   * library's test is close >= open -- and the doji keeps its close at 100, so
   * terms 2 and 3 go on reading the same two prices. */
  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,100);              /* 1st is a doji, which counts as white */
  pb_bar(99,111.5,98.5,111);
  int f1=pb_bar(112,115.5,111.5,115);
  pb_flip_in(f1,0,0,1,"break c0 alt0 term1: the 1st is a doji, and close >= open counts as white");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100.5,110.5,99.5,100);            /* one step black, same close */
  pb_bar(99,111.5,98.5,111);
  int k1=pb_bar(112,115.5,111.5,115);
  pb_control(k1,100,0,"restore c0 alt0 term1: open 100.5 above close 100, black again");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);
  pb_bar(99,111,98.5,110);                 /* 2nd close 110 == the 1st open */
  int f2=pb_bar(112,115.5,111.5,115);
  pb_flip_in(f2,0,0,2,"break c0 alt0 term2: the 2nd closes at 110 == open(i-2), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);
  pb_bar(99,111,98.5,110.5);               /* only the close moves */
  int k2=pb_bar(112,115.5,111.5,115);
  pb_control(k2,100,0,"restore c0 alt0 term2: the 2nd closes 110.5 above 110");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);
  pb_bar(100,111.5,98.5,111);              /* 2nd open 100 == the 1st close */
  int f3=pb_bar(112,115.5,111.5,115);
  pb_flip_in(f3,0,0,3,"break c0 alt0 term3: the 2nd opens at 100 == close(i-2), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);
  pb_bar(99.5,111.5,98.5,111);             /* only the open moves */
  int k3=pb_bar(112,115.5,111.5,115);
  pb_control(k3,100,0,"restore c0 alt0 term3: the 2nd opens 99.5 below 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);
  pb_bar(99,111.5,98.5,111);
  int f4=pb_bar(112,115.5,110.5,111);      /* 3rd closes 111 == the 2nd close */
  pb_flip_in(f4,0,0,4,"break c0 alt0 term4: the 3rd closes at 111 == close(i-1), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,100);
  pb_bar(99,111.5,98.5,111);
  int k4=pb_bar(112,115.5,110.5,111.5);    /* only the close moves */
  pb_control(k4,100,0,"restore c0 alt0 term4: the 3rd closes 111.5 above 111");
  pb_flat(8);

  /* The BLACK arm, output -100: the mirror, and the second output class. */
  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);              /* 1st: white, body 100..110 */
  pb_bar(111,111.5,98.5,99);               /* 2nd: black, engulfing it */
  int db=pb_bar(98,98.5,94.5,95);          /* 3rd: closes 95, below the 2nd close 99 */
  pb_detect(db,-100,"detect black-first: the mirror, output -100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(110.5,110.5,99.5,110);            /* 1st black, same close */
  pb_bar(111,111.5,98.5,99);
  int fb1=pb_bar(98,98.5,94.5,95);
  pb_flip_in(fb1,0,1,1,"break c0 alt1 term1: the 1st opens 110.5 above its close, so it is black");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(110,110.5,99.5,110);              /* one step back: a doji, which is white */
  pb_bar(111,111.5,98.5,99);
  int kb1=pb_bar(98,98.5,94.5,95);
  pb_control(kb1,-100,0,"restore c0 alt1 term1: the 1st is a doji, which is white");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);
  pb_bar(110,110.5,98.5,99);               /* 2nd open 110 == the 1st close */
  int fb2=pb_bar(98,98.5,94.5,95);
  pb_flip_in(fb2,0,1,2,"break c0 alt1 term2: the 2nd opens at 110 == close(i-2), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);
  pb_bar(110.5,110.5,98.5,99);             /* only the open moves */
  int kb2=pb_bar(98,98.5,94.5,95);
  pb_control(kb2,-100,0,"restore c0 alt1 term2: the 2nd opens 110.5 above 110");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);
  pb_bar(111,111.5,99,100);                /* 2nd close 100 == the 1st open */
  int fb3=pb_bar(98,98.5,94.5,95);
  pb_flip_in(fb3,0,1,3,"break c0 alt1 term3: the 2nd closes at 100 == open(i-2), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);
  pb_bar(111,111.5,99,99.5);               /* only the close moves */
  int kb3=pb_bar(98,98.5,94.5,95);
  pb_control(kb3,-100,0,"restore c0 alt1 term3: the 2nd closes 99.5 below 100");
  pb_flat(8);

  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);
  pb_bar(111,111.5,98.5,99);
  int fb4=pb_bar(98,99.5,94.5,99);         /* 3rd closes 99 == the 2nd close */
  pb_flip_in(fb4,0,1,4,"break c0 alt1 term4: the 3rd closes at 99 == close(i-1), the test is strict");
  pb_flat(8);
  pb_primer(12,100,2,4);
  pb_bar(100,110.5,99.5,110);
  pb_bar(111,111.5,98.5,99);
  int kb4=pb_bar(98,99.5,94.5,98.5);       /* only the close moves */
  pb_control(kb4,-100,0,"restore c0 alt1 term4: the 3rd closes 98.5 below 99");
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
   pb_reset(); build_hammer();              e = pb_check_mcdc("CDLHAMMER",              TA_CDLHAMMER,              cond_hammer);              if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_hangingman();          e = pb_check_mcdc("CDLHANGINGMAN",          TA_CDLHANGINGMAN,          cond_hangingman);          if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_invertedhammer();      e = pb_check_mcdc("CDLINVERTEDHAMMER",      TA_CDLINVERTEDHAMMER,      cond_invertedhammer);      if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_shootingstar();        e = pb_check_mcdc("CDLSHOOTINGSTAR",        TA_CDLSHOOTINGSTAR,        cond_shootingstar);        if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_matchinglow();         e = pb_check_mcdc("CDLMATCHINGLOW",         TA_CDLMATCHINGLOW,         cond_matchinglow);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_dojistar();            e = pb_check_mcdc("CDLDOJISTAR",            TA_CDLDOJISTAR,            cond_dojistar);            if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_inneck();              e = pb_check_mcdc("CDLINNECK",              TA_CDLINNECK,              cond_inneck);              if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_onneck();              e = pb_check_mcdc("CDLONNECK",              TA_CDLONNECK,              cond_onneck);              if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_thrusting();           e = pb_check_mcdc("CDLTHRUSTING",           TA_CDLTHRUSTING,           cond_thrusting);           if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_piercing();            e = pb_check_mcdc("CDLPIERCING",            TA_CDLPIERCING,            cond_piercing);            if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_homingpigeon();        e = pb_check_mcdc("CDLHOMINGPIGEON",        TA_CDLHOMINGPIGEON,        cond_homingpigeon);        if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_counterattack();       e = pb_check_mcdc("CDLCOUNTERATTACK",       TA_CDLCOUNTERATTACK,       cond_counterattack);       if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_darkcloudcover();      e = pb_check_mcdc_p("CDLDARKCLOUDCOVER",    TA_CDLDARKCLOUDCOVER,    0.5, cond_darkcloudcover);  if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_morningstar();         e = pb_check_mcdc_p("CDLMORNINGSTAR",       TA_CDLMORNINGSTAR,       0.3, cond_morningstar);     if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_eveningstar();         e = pb_check_mcdc_p("CDLEVENINGSTAR",       TA_CDLEVENINGSTAR,       0.3, cond_eveningstar);     if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_morningdojistar();     e = pb_check_mcdc_p("CDLMORNINGDOJISTAR",   TA_CDLMORNINGDOJISTAR,   0.3, cond_morningdojistar); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_eveningdojistar();     e = pb_check_mcdc_p("CDLEVENINGDOJISTAR",   TA_CDLEVENINGDOJISTAR,   0.3, cond_eveningdojistar); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_advanceblock();        e = pb_check_mcdc("CDLADVANCEBLOCK",        TA_CDLADVANCEBLOCK,        cond_advanceblock);        if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_gapsidesidewhite();    e = pb_check_mcdc("CDLGAPSIDESIDEWHITE",    TA_CDLGAPSIDESIDEWHITE,    cond_gapsidesidewhite);    if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_risefall3methods();    e = pb_check_mcdc("CDLRISEFALL3METHODS",    TA_CDLRISEFALL3METHODS,    cond_risefall3methods);    if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_breakaway();           e = pb_check_mcdc("CDLBREAKAWAY",           TA_CDLBREAKAWAY,           cond_breakaway);           if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_abandonedbaby();       e = pb_check_mcdc_p("CDLABANDONEDBABY",     TA_CDLABANDONEDBABY,     0.3, cond_abandonedbaby);   if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_mathold();             e = pb_check_mcdc_p("CDLMATHOLD",           TA_CDLMATHOLD,           0.5, cond_mathold);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_stalledpattern();      e = pb_check_mcdc("CDLSTALLEDPATTERN",     TA_CDLSTALLEDPATTERN,      cond_stalledpattern);      if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3linestrike();         e = pb_check_mcdc("CDL3LINESTRIKE",        TA_CDL3LINESTRIKE,         cond_3linestrike);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_identical3crows();     e = pb_check_mcdc("CDLIDENTICAL3CROWS",   TA_CDLIDENTICAL3CROWS,     cond_identical3crows);     if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_ladderbottom();        e = pb_check_mcdc("CDLLADDERBOTTOM",       TA_CDLLADDERBOTTOM,        cond_ladderbottom);        if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_unique3river();        e = pb_check_mcdc("CDLUNIQUE3RIVER",       TA_CDLUNIQUE3RIVER,        cond_unique3river);        if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_upsidegap2crows();     e = pb_check_mcdc("CDLUPSIDEGAP2CROWS",   TA_CDLUPSIDEGAP2CROWS,     cond_upsidegap2crows);     if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_tasukigap();           e = pb_check_mcdc("CDLTASUKIGAP",          TA_CDLTASUKIGAP,           cond_tasukigap);           if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_engulfing();           e = pb_check_mcdc("CDLENGULFING",          TA_CDLENGULFING,           cond_engulfing);           if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_harami();              e = pb_check_mcdc("CDLHARAMI",             TA_CDLHARAMI,              cond_harami);              if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_haramicross();         e = pb_check_mcdc("CDLHARAMICROSS",        TA_CDLHARAMICROSS,         cond_haramicross);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_tristar();             e = pb_check_mcdc("CDLTRISTAR",            TA_CDLTRISTAR,             cond_tristar);             if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_hikkake();             e = pb_check_mcdc("CDLHIKKAKE",            TA_CDLHIKKAKE,             cond_hikkake);             if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_hikkakemod();          e = pb_check_mcdc("CDLHIKKAKEMOD",         TA_CDLHIKKAKEMOD,          cond_hikkakemod);          if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_separatinglines(); e = pb_check_mcdc("CDLSEPARATINGLINES", TA_CDLSEPARATINGLINES, cond_separatinglines); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_kicking();         e = pb_check_mcdc("CDLKICKING",         TA_CDLKICKING,         cond_kicking);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_kickingbylength(); e = pb_check_mcdc("CDLKICKINGBYLENGTH", TA_CDLKICKINGBYLENGTH, cond_kickingbylength); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3inside();         e = pb_check_mcdc("CDL3INSIDE",         TA_CDL3INSIDE,         cond_3inside);         if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_sticksandwich();   e = pb_check_mcdc("CDLSTICKSANDWICH",   TA_CDLSTICKSANDWICH,   cond_sticksandwich);   if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_xsidegap3methods(); e = pb_check_mcdc("CDLXSIDEGAP3METHODS", TA_CDLXSIDEGAP3METHODS, cond_xsidegap3methods); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_concealbabyswall(); e = pb_check_mcdc("CDLCONCEALBABYSWALL", TA_CDLCONCEALBABYSWALL, cond_concealbabyswall); if( e != TA_TEST_PASS ) return e;
   pb_reset(); build_3outside();        e = pb_check_mcdc("CDL3OUTSIDE",         TA_CDL3OUTSIDE,         cond_3outside);        if( e != TA_TEST_PASS ) return e;
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
