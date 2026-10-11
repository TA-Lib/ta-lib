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
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#487).
 *  101126 MF,CC  Per-bar table from LEAN; exact-threshold leg; the edge,
 *                tie and alias legs compare what they count (#487).
 */

/* Description:
 *
 *   Test TA_ZIGZAG.
 *
 *   TA_ZIGZAG emits per-bar state; talipp, the card's oracle, emits a pivot
 *   LIST. The card gives the bridge, and this file uses it: a pivot is confirmed
 *   at bar i whenever outTrend[i] differs from outTrend[i-1], and it is
 *   (outPivotIdx[i-1], outZigZag[i-1]); the last, still-open pivot is the final
 *   row's. When outTrend at the first output bar is +1 a reversal fired there,
 *   and the pivot it confirmed is the seed -- which the gate makes exact: any
 *   move of the seed would have reset the counter and left fewer than m bars, so
 *   a reversal at the first output bar proves the seed never moved.
 *
 *   Legs:
 *     1. GOLDEN. talipp 2.7.0's pivot list over the 252-bar corpus at five
 *        parameter sets, compared as a list. Also the leg that drives
 *        server_verify.
 *     2. THE GATE ACTUALLY BINDING. Whether optInMinTrendLength does anything at
 *        all depends on where the sensitivity sits against the data, so the
 *        parameter sets are MEASURED rather than chosen. On this corpus:
 *          s=10%: m=1:15 m=2:15 m=4:15 m=8:13 m=15:7
 *          s= 5%: m=1:50 m=2:42 m=4:30 m=8:15 m=15:11
 *          s= 2%: m=1:155 m=2:67 m=4:40 m=8:17 m=15:11
 *        At 10% the gate does nothing through m=4. Two of the five golden
 *        sets are therefore pairs differing only in m, and this leg asserts
 *        their tables really do differ -- a pair that happened to land where the
 *        gate is inert would prove nothing while still passing.
 *     3. ORDERING. A bar that both makes a new extreme and clears the threshold
 *        must REVERSE the leg, not extend it. The two answers differ by a whole
 *        leg, not by a rounding, so no tolerance could absorb a mix-up.
 *     4. TIES. A flat stretch at an unchanged price: outPivotIdx advances and
 *        outZigZag does not. That movement is the only thing the index output
 *        carries, so the leg compares it on every bar of a flat run.
 *     5. LOOKBACK AND THE SEED. The lookback is optInMinTrendLength and does not
 *        move with the sensitivity; the first output lands on it; and a reversal
 *        CAN fire on the first output bar, which is the ruling this card makes
 *        (the gate counts the seed bar) and the one place the other reading
 *        would show.
 *     6. PATH DEPENDENCE. Two different startIdx disagree at a shared bar, and
 *        then stop. The disagreement begins on the later run's FIRST output
 *        bar, so it is entirely about the seed. Pinning only "they differ"
 *        would also be satisfied by a machine that never re-synchronises.
 *     7. PARAMETER EDGES. Sensitivity 0 and 100 are degenerate but defined.
 *     8. ALIASING. outZigZag over each input in turn.
 *     9. PER BAR. Every bar's three outputs against LEAN's ZigZag at four
 *        parameter sets. The pivot list of leg 1 holds one row per leg; this
 *        holds every extension inside a leg too.
 *    10. EXACT THRESHOLD. A move of exactly the sensitivity reverses, in both
 *        directions; the only leg that separates >= from >.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations.    ****/

#define ZZ_NB_BAR 252

typedef struct { int bar; double price; int trend; } ZzPivot;
typedef struct { double sensitivity; int minTrendLength; int off; int nb; } ZzCase;

static const ZzPivot zzGolden[] =
{
   {   8,  89.440000, -1 },
   {  11,  99.625000,  1 },
   {  25,  80.875000, -1 },
   {  46,  92.970000,  1 },
   {  54,  82.000000, -1 },
   {  66,  94.030000,  1 },
   {  73,  81.500000, -1 },
   {  90, 123.000000,  1 },
   { 103, 109.440000, -1 },
   { 131, 139.190000,  1 },
   { 148, 117.560000, -1 },
   { 173, 137.690000,  1 },
   { 202,  89.000000, -1 },
   { 236, 122.120000,  1 },
   { 240, 104.500000, -1 },
   {  25,  80.875000, -1 },
   { 131, 139.190000,  1 },
   { 148, 117.560000, -1 },
   { 173, 137.690000,  1 },
   { 202,  89.000000, -1 },
   { 236, 122.120000,  1 },
   { 251, 106.620000, -1 },
   {   0,  90.750000, -1 },
   {   4,  96.000000,  1 },
   {   8,  89.440000, -1 },
   {  12,  99.125000,  1 },
   {  25,  80.875000, -1 },
   {  34,  90.000000,  1 },
   {  40,  82.530000, -1 },
   {  46,  92.970000,  1 },
   {  54,  82.000000, -1 },
   {  66,  94.030000,  1 },
   {  73,  81.500000, -1 },
   {  90, 123.000000,  1 },
   { 103, 109.440000, -1 },
   { 108, 120.870000,  1 },
   { 112, 114.750000, -1 },
   { 131, 139.190000,  1 },
   { 148, 117.560000, -1 },
   { 157, 129.630000,  1 },
   { 162, 120.870000, -1 },
   { 173, 137.690000,  1 },
   { 202,  89.000000, -1 },
   { 208,  99.000000,  1 },
   { 212,  90.000000, -1 },
   { 217,  97.500000,  1 },
   { 221,  92.620000, -1 },
   { 225, 109.870000,  1 },
   { 229, 102.120000, -1 },
   { 236, 122.120000,  1 },
   { 240, 104.500000, -1 },
   { 248, 110.750000,  1 },
   {  25,  80.875000, -1 },
   {  46,  92.970000,  1 },
   {  54,  82.000000, -1 },
   {  66,  94.030000,  1 },
   {  74,  82.565000, -1 },
   {  90, 123.000000,  1 },
   { 103, 109.440000, -1 },
   { 131, 139.190000,  1 },
   { 148, 117.560000, -1 },
   { 157, 129.630000,  1 },
   { 165, 122.690000, -1 },
   { 173, 137.690000,  1 },
   { 202,  89.000000, -1 },
   { 236, 122.120000,  1 },
   { 251, 106.620000, -1 },
   {  25,  80.875000, -1 },
   {  34,  90.000000,  1 },
   {  42,  86.875000, -1 },
   {  50,  90.000000,  1 },
   {  58,  87.190000, -1 },
   {  66,  94.030000,  1 },
   {  74,  82.565000, -1 },
   {  90, 123.000000,  1 },
   { 103, 109.440000, -1 },
   { 131, 139.190000,  1 },
   { 148, 117.560000, -1 },
   { 157, 129.630000,  1 },
   { 165, 122.690000, -1 },
   { 173, 137.690000,  1 },
   { 202,  89.000000, -1 },
   { 236, 122.120000,  1 },
   { 251, 106.620000, -1 },
};
#define ZZ_NB_GOLDEN ((int)(sizeof(zzGolden)/sizeof(zzGolden[0])))

static const ZzCase zzCases[] =
{
   {  10.0,   1,   0,  15 },
   {  10.0,  15,  15,   7 },
   {   5.0,   4,  22,  30 },
   {   5.0,   8,  52,  15 },
   {   2.0,   8,  67,  17 },
};
#define ZZ_NB_CASE ((int)(sizeof(zzCases)/sizeof(zzCases[0])))

/* LEAN ZigZag (QuantConnect.Indicators 2.5.18090, commit 5b0c9975d318), fed one
 * bar High = Low = 1e15 before bar 0, which stands in for the seed bar LEAN does
 * not count toward the first gate. Captured by ta-lib-oracles
 * capture_487_zigzag.py; talipp 2.7.0 gives the same pivot lists.
 *
 * One cell per bar: outTrend * (outPivotIdx + 1), 0 before the first output.
 * outZigZag is the pivot bar's high on an up leg and its low on a down leg.
 */
static const ZzCase zzBarCases[] =
{
   {  5.0,  1, 0, 0 },
   {  2.0,  1, 0, 0 },
   {  5.0,  8, 0, 0 },
   { 10.0, 15, 0, 0 },
};
#define ZZ_NB_BAR_CASE ((int)(sizeof(zzBarCases)/sizeof(zzBarCases[0])))

static const short zzPerBar[ZZ_NB_BAR_CASE][ZZ_NB_BAR] =
{
   { /* sensitivity 5, minTrendLength 1 */
      0,  -1,   3,   3,   3,   3,   3,  -8,  -9,  -9,  11,  12,
     12, -14, -15,  16,  17,  17,  17,  17, -21, -22, -23, -24,
    -25, -26, -26,  28,  28,  28, -31, -31, -31,  34,  35,  35,
    -37, -38, -39, -39, -41,  42,  43,  44,  45,  45,  47,  47,
     47,  47,  47, -52, -53, -54, -55, -55,  57,  58,  59,  60,
     61,  61,  63,  64,  65,  66,  67,  67,  67, -70, -71, -72,
    -73, -74,  75,  76,  77,  78,  79, -80, -81, -82, -82,  84,
     84,  84,  87,  88,  89,  90,  91,  91, -93, -93, -93, -93,
    -97, -98, -99, 100, 100, 100,-103,-104, 105, 106, 107, 107,
    107,-110,-111,-111, 113, 114, 115, 115, 117, 118, 118, 118,
    118, 118, 118, 124, 124, 126, 127, 127, 129, 130, 131, 132,
    132, 132, 132, 132,-137,-137,-139,-140,-140,-140,-140,-140,
   -140,-146,-147,-148, 149, 150, 150,-152,-152,-152,-152, 156,
    157, 158,-159,-160,-160,-162,-163,-163,-163,-163,-163, 168,
    168, 170, 171, 172, 173, 174, 174, 174, 174,-178,-179, 180,
    180, 180,-183,-184, 185, 185,-187,-188,-189,-189, 191, 191,
   -193,-194, 195,-196,-197,-198,-199,-199,-199, 202,-203, 204,
    205, 206, 206, 206, 209, 209,-211,-212,-213,-213,-213, 216,
    217, 218, 218, 218, 218,-222, 223, 224, 225, 226,-227,-227,
   -227,-227,-227,-227, 233, 234, 235, 236,-237,-238,-239,-239,
   -241,-241, 243, 243, 243, 243, 243, 243, 243, 243, 243, 243
   },
   { /* sensitivity 2, minTrendLength 1 */
      0,   2,   3,  -4,   5,  -6,  -7,  -8,   9,  10,  11,  12,
    -13, -14,  15,  16, -17, -18,  19, -20, -21, -22, -23, -24,
     25, -26,  27,  28, -29,  30, -31,  32, -33,  34,  35, -36,
    -37, -38, -39,  40, -41,  42,  43,  44,  45, -46,  47, -48,
     49,  50, -51, -52,  53, -54, -55,  56,  57,  58,  59,  60,
    -61, -62,  63,  64, -65,  66, -67, -68, -69,  70, -71,  72,
    -73,  74, -75,  76, -77,  78,  79, -80,  81, -82,  83,  84,
    -85,  86,  87,  88,  89, -90,  91, -92, -93,  94, -95, -96,
    -97, -98, -99, 100,-101, 102,-103, 104,-105, 106, 107,-108,
    109,-110, 111,-112, 113, 114,-115, 116, 117,-118,-119, 120,
    121,-122, 123, 124,-125, 126, 127,-128, 129, 130, 131,-132,
    133,-134, 135,-136,-137, 138,-139,-140, 141, 142, 143,-144,
   -145,-146,-147, 148,-149, 150,-151,-152, 153,-154, 155, 156,
    157,-158,-159, 160, 161,-162, 163, 164,-165, 166,-167, 168,
   -169, 170, 171, 172, 173, 174,-175,-176, 177,-178,-179, 180,
   -181,-182, 183,-184, 185,-186,-187,-188,-189, 190, 191,-192,
   -193,-194, 195,-196,-197, 198,-199, 200,-201, 202,-203, 204,
   -205, 206,-207, 208, 209,-210,-211, 212,-213, 214,-215, 216,
    217,-218,-219, 220,-221, 222, 223, 224, 225,-226,-227, 228,
   -229,-230, 231, 232, 233, 234, 235,-236, 237,-238, 239,-240,
   -241, 242, 243,-244, 245, 246, 247,-248, 249, 249, 249,-252
   },
   { /* sensitivity 5, minTrendLength 8 */
      0,   0,   0,   0,   0,   0,   0,   0,  -9,  -9,  -9,  -9,
     -9, -14, -15, -15, -15, -15, -15, -15, -15, -22, -23, -24,
    -25, -26, -26, -26, -26, -26, -26, -26, -26,  34,  35,  35,
     35,  35,  35,  35,  35,  35,  35,  44,  45,  45,  47,  47,
     47,  47,  47,  47,  47,  47, -55, -55, -55, -55, -55, -55,
    -55, -55,  63,  64,  65,  66,  67,  67,  67,  67,  67,  67,
     67,  67, -75, -75, -75, -75, -75, -75, -75, -75,  83,  84,
     84,  84,  87,  88,  89,  90,  91,  91,  91,  91,  91,  91,
     91,  91, -99, -99, -99, -99, -99,-104,-104,-104,-104,-104,
   -104,-104,-104, 112, 113, 114, 115, 115, 117, 118, 118, 118,
    118, 118, 118, 124, 124, 126, 127, 127, 129, 130, 131, 132,
    132, 132, 132, 132, 132, 132, 132,-140,-140,-140,-140,-140,
   -140,-146,-147,-148,-149,-149,-149,-149,-149,-149,-149,-149,
    157, 158, 158, 158, 158, 158, 158, 158, 158,-166,-166,-166,
   -166,-166,-166,-166,-166, 174, 174, 174, 174, 174, 174, 174,
    174,-182,-183,-184,-184,-184,-187,-188,-189,-189,-189,-189,
   -193,-194,-194,-196,-197,-198,-199,-199,-199,-199,-203,-203,
   -203,-203,-203,-203,-203,-203, 211, 211, 211, 211, 211, 211,
    217, 218, 218, 218, 218, 218, 223, 224, 225, 226, 226, 226,
    226, 226, 226, 226, 233, 234, 235, 236, 237, 237, 237, 237,
    237, 237, 237, 237,-245,-246,-246,-246,-246,-246,-246,-252
   },
   { /* sensitivity 10, minTrendLength 15 */
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0, -15, -15, -15, -15, -15, -15, -22, -23, -24,
    -25, -26, -26, -26, -26, -26, -26, -26, -26, -26, -26, -26,
    -26, -26, -26, -26, -26, -26,  43,  44,  45,  45,  47,  47,
     47,  47,  47,  47,  47,  47,  47,  47,  47,  47,  47,  47,
     47,  47,  47,  64,  65,  66,  67,  67,  67,  67,  67,  67,
     67,  67,  67,  76,  77,  78,  79,  79,  79,  79,  79,  84,
     84,  84,  87,  88,  89,  90,  91,  91,  91,  91,  91,  91,
     91,  91,  91,  91,  91,  91,  91,  91,  91,  91,  91,  91,
     91,  91,  91,  91,  91,  91,  91,  91, 117, 118, 118, 118,
    118, 118, 118, 124, 124, 126, 127, 127, 129, 130, 131, 132,
    132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132,
    132, 132,-147,-148,-149,-149,-149,-149,-149,-149,-149,-149,
   -149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,-149,
   -149, 170, 171, 172, 173, 174, 174, 174, 174, 174, 174, 174,
    174, 174, 174, 174, 174, 174, 174, 174,-189,-189,-189,-189,
   -193,-194,-194,-196,-197,-198,-199,-199,-199,-199,-203,-203,
   -203,-203,-203,-203,-203,-203,-203,-203,-203,-203,-203,-203,
   -203,-203,-203,-203,-203,-203, 223, 224, 225, 226, 226, 226,
    226, 226, 226, 226, 233, 234, 235, 236, 237, 237, 237, 237,
    237, 237, 237, 237, 237, 237, 237, 237, 237, 237, 237,-252
   },
};
#define ZZ_THRESH_CMP 2
#define ZZ_PERBAR_CMP 2949   /* (251 + 251 + 244 + 237) bars x 3 outputs */

/* Counts the legs compare, pinned so that a leg which stopped early reads
 * differently from one that passed. */
#define ZZ_GOLDEN_CMP    252   /* 84 pivot rows x 3 fields */
#define ZZ_GATE_CMP        2   /* two golden pairs differing only in m */
#define ZZ_ORDER_CMP       4   /* trend, price, pivotIdx at the outside bar, and the bar before it */
#define ZZ_TIE_CMP        63   /* every output bar of the 64-bar flat run */
#define ZZ_LOOKBACK_CMP    8   /* 4 values of m x (the lookback, then the seed reversal) */
#define ZZ_PATH_CMP        2   /* it diverges at the seed, and it re-converges */
#define ZZ_EDGE_CMP      255   /* 251 bars at s=0, 3 bars at s=100, m=0 rejected */
#define ZZ_ALIAS_CMP       2   /* outZigZag over each input */

static int g_zzGoldenCmp;
static int g_zzGateCmp;
static int g_zzOrderCmp;
static int g_zzTieCmp;
static int g_zzLookbackCmp;
static int g_zzPathCmp;
static int g_zzEdgeCmp;
static int g_zzAliasCmp;
static int g_zzPerBarCmp;
static int g_zzThreshCmp;

static TA_Real  zzOut[ZZ_NB_BAR];
static TA_Integer zzTrend[ZZ_NB_BAR];
static TA_Integer zzPivot[ZZ_NB_BAR];

/**** Local functions declarations.    ****/

static int zzRebuild( const TA_Real *low, int begIdx, int nbElement,
                      int minTrendLength, ZzPivot *out, int cap );
static ErrorNumber zzLegGolden  ( TA_History *history );
static ErrorNumber zzLegGate    ( void );
static ErrorNumber zzLegOrdering( void );
static ErrorNumber zzLegTies    ( void );
static ErrorNumber zzLegLookback( TA_History *history );
static ErrorNumber zzLegPath    ( TA_History *history );
static ErrorNumber zzLegEdges   ( TA_History *history );
static ErrorNumber zzLegAlias   ( TA_History *history );
static ErrorNumber zzLegPerBar  ( TA_History *history );
static ErrorNumber zzLegThreshold( void );

/**** Global functions definitions.   ****/

ErrorNumber test_func_zigzag( TA_History *history )
{
   ErrorNumber e;

   if( (int)history->nbBars != ZZ_NB_BAR )
   {
      printf( "Fail: ZIGZAG expects %d bars, history has %d\n",
              ZZ_NB_BAR, (int)history->nbBars );
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   }

   g_zzGoldenCmp = g_zzGateCmp = g_zzOrderCmp = g_zzTieCmp = 0;
   g_zzLookbackCmp = g_zzPathCmp = g_zzEdgeCmp = g_zzAliasCmp = 0;
   g_zzPerBarCmp = 0;
   g_zzThreshCmp = 0;

   if( (e = zzLegGolden  ( history )) != TA_TEST_PASS ) return e;
   if( (e = zzLegGate    (         )) != TA_TEST_PASS ) return e;
   if( (e = zzLegOrdering(         )) != TA_TEST_PASS ) return e;
   if( (e = zzLegTies    (         )) != TA_TEST_PASS ) return e;
   if( (e = zzLegLookback( history )) != TA_TEST_PASS ) return e;
   if( (e = zzLegPath    ( history )) != TA_TEST_PASS ) return e;
   if( (e = zzLegEdges   ( history )) != TA_TEST_PASS ) return e;
   if( (e = zzLegAlias   ( history )) != TA_TEST_PASS ) return e;
   if( (e = zzLegPerBar  ( history )) != TA_TEST_PASS ) return e;
   if( (e = zzLegThreshold(         )) != TA_TEST_PASS ) return e;

   if( g_zzGoldenCmp   != ZZ_GOLDEN_CMP   ||
       g_zzGateCmp     != ZZ_GATE_CMP     ||
       g_zzOrderCmp    != ZZ_ORDER_CMP    ||
       g_zzTieCmp      != ZZ_TIE_CMP      ||
       g_zzLookbackCmp != ZZ_LOOKBACK_CMP ||
       g_zzPathCmp     != ZZ_PATH_CMP     ||
       g_zzEdgeCmp     != ZZ_EDGE_CMP     ||
       g_zzAliasCmp    != ZZ_ALIAS_CMP )
   {
      printf( "Fail: ZIGZAG comparison counts %d/%d/%d/%d/%d/%d/%d/%d, expected "
              "%d/%d/%d/%d/%d/%d/%d/%d\n",
              g_zzGoldenCmp, g_zzGateCmp, g_zzOrderCmp, g_zzTieCmp,
              g_zzLookbackCmp, g_zzPathCmp, g_zzEdgeCmp, g_zzAliasCmp,
              ZZ_GOLDEN_CMP, ZZ_GATE_CMP, ZZ_ORDER_CMP, ZZ_TIE_CMP,
              ZZ_LOOKBACK_CMP, ZZ_PATH_CMP, ZZ_EDGE_CMP, ZZ_ALIAS_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( g_zzPerBarCmp != ZZ_PERBAR_CMP )
   {
      printf( "Fail: ZIGZAG per-bar leg compared %d values, expected %d\n",
              g_zzPerBarCmp, ZZ_PERBAR_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   if( g_zzThreshCmp != ZZ_THRESH_CMP )
   {
      printf( "Fail: ZIGZAG threshold leg compared %d reversals, expected %d\n",
              g_zzThreshCmp, ZZ_THRESH_CMP );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }

   return TA_TEST_PASS;
}

/**** Local functions definitions.     ****/

/* The card's bridge: per-bar state back to the pivot list talipp emits. */
static int zzRebuild( const TA_Real *low, int begIdx, int nbElement,
                      int minTrendLength, ZzPivot *out, int cap )
{
   int i, n = 0;

   if( nbElement <= 0 )
      return 0;

   /* A reversal at the FIRST output bar confirms the seed. It can only fire
    * there if the seed never moved: any move resets the counter and leaves
    * fewer than minTrendLength bars, so the gate would not have opened.
    */
   if( zzTrend[0] == 1 )
   {
      if( n >= cap ) return -1;
      out[n].bar = begIdx - minTrendLength;
      out[n].price = low[begIdx - minTrendLength];
      out[n].trend = -1;
      n++;
   }

   for( i = 1; i < nbElement; i++ )
   {
      if( zzTrend[i] != zzTrend[i-1] )
      {
         if( n >= cap ) return -1;
         out[n].bar = zzPivot[i-1];
         out[n].price = zzOut[i-1];
         out[n].trend = zzTrend[i-1];
         n++;
      }
   }

   /* The last, still-open pivot: the leg Achelis warns can still change. */
   if( n >= cap ) return -1;
   out[n].bar = zzPivot[nbElement-1];
   out[n].price = zzOut[nbElement-1];
   out[n].trend = zzTrend[nbElement-1];
   n++;

   return n;
}

/* Leg 1. talipp's pivot list, five parameter sets, compared as a list. */
static ErrorNumber zzLegGolden( TA_History *history )
{
   static ZzPivot mine[ZZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, i, n, nbBars;
   ErrorNumber e;

   nbBars = (int)history->nbBars;

   for( k = 0; k < ZZ_NB_CASE; k++ )
   {
      const ZzCase *c = &zzCases[k];
      const ZzPivot *want = &zzGolden[c->off];
      int cmpBefore;

      rc = TA_ZIGZAG( 0, nbBars-1, history->high, history->low,
                      c->sensitivity, c->minTrendLength,
                      &begIdx, &nbElement, zzOut, zzTrend, zzPivot );
      if( rc != TA_SUCCESS || begIdx != c->minTrendLength )
      {
         printf( "Fail: TA_ZIGZAG s=%g m=%d rc=%d begIdx=%d, expected %d\n",
                 c->sensitivity, c->minTrendLength, (int)rc, (int)begIdx,
                 c->minTrendLength );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      cmpBefore = server_verify_comparisons();
      if( server_verify_active() )
      {
         const double opt[2] = { c->sensitivity, (double)c->minTrendLength };
         e = server_verify( "ZIGZAG", 0, nbBars-1, nbBars,
                rc, begIdx, nbElement,
                (const TA_Real*[]){ history->high, history->low, NULL },
                opt, 2,
                (const TA_Real*[]){ zzOut, NULL },
                (const TA_Integer*[]){ zzTrend, zzPivot, NULL } );
         if( e != TA_TEST_PASS )
            return e;
         if( server_verify_comparisons() == cmpBefore )
         {
            printf( "ZIGZAG [s=%g m=%d]: compared no server despite a live "
                    "session\n", c->sensitivity, c->minTrendLength );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      n = zzRebuild( history->low, begIdx, nbElement, c->minTrendLength,
                     mine, ZZ_NB_BAR );
      if( n != c->nb )
      {
         printf( "Fail: TA_ZIGZAG s=%g m=%d rebuilt %d pivot(s), talipp gives "
                 "%d\n", c->sensitivity, c->minTrendLength, n, c->nb );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }

      for( i = 0; i < n; i++ )
      {
         if( mine[i].bar != want[i].bar )
         {
            printf( "Fail: TA_ZIGZAG s=%g m=%d pivot %d bar %d, talipp gives "
                    "%d\n", c->sensitivity, c->minTrendLength, i,
                    mine[i].bar, want[i].bar );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_zzGoldenCmp++;

         if( mine[i].price != want[i].price )
         {
            printf( "Fail: TA_ZIGZAG s=%g m=%d pivot %d price %.17g, talipp "
                    "gives %.17g\n", c->sensitivity, c->minTrendLength, i,
                    mine[i].price, want[i].price );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_zzGoldenCmp++;

         if( mine[i].trend != want[i].trend )
         {
            printf( "Fail: TA_ZIGZAG s=%g m=%d pivot %d trend %d, talipp gives "
                    "%d\n", c->sensitivity, c->minTrendLength, i,
                    mine[i].trend, want[i].trend );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_zzGoldenCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 2. The two golden pairs differ only in optInMinTrendLength. If their
 * tables were equal the pairing would prove nothing about the gate, which is
 * what MEASURED above says happens at a sensitivity of 10 for every m up to 4.
 * This leg is the assertion that the corpus really does exercise it.
 */
static ErrorNumber zzLegGate( void )
{
   int pairs[2][2] = { { 0, 1 }, { 2, 3 } };
   int p, i, same;

   for( p = 0; p < 2; p++ )
   {
      const ZzCase *a = &zzCases[pairs[p][0]];
      const ZzCase *b = &zzCases[pairs[p][1]];

      if( a->sensitivity != b->sensitivity ||
          a->minTrendLength == b->minTrendLength )
      {
         printf( "Fail: ZIGZAG gate leg, cases %d and %d are not a pair that "
                 "differs only in the trend gate\n", pairs[p][0], pairs[p][1] );
         return TA_TESTUTIL_TFRR_BAD_PARAM;
      }

      same = ( a->nb == b->nb );
      if( same )
      {
         for( i = 0; i < a->nb; i++ )
         {
            if( zzGolden[a->off+i].bar   != zzGolden[b->off+i].bar   ||
                zzGolden[a->off+i].price != zzGolden[b->off+i].price ||
                zzGolden[a->off+i].trend != zzGolden[b->off+i].trend )
            {
               same = 0;
               break;
            }
         }
      }

      if( same )
      {
         printf( "Fail: ZIGZAG at s=%g gives the same pivots at m=%d and m=%d, "
                 "so the golden pair tests nothing about the gate\n",
                 a->sensitivity, a->minTrendLength, b->minTrendLength );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzGateCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 3. Reversal before extension.
 *
 * Bar 2 is an outside bar: its high clears the threshold off the seed low AND
 * its low makes a new low. Testing extension first would move the pivot down
 * and leave the leg where it was; testing reversal first flips the leg and
 * leaves the old pivot alone. The two answers differ by a whole leg.
 */
static ErrorNumber zzLegOrdering( void )
{
   static TA_Real h[8], l[8];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;

   h[0] = 100.0; l[0] = 100.0;
   h[1] = 100.5; l[1] = 100.0;
   h[2] = 120.0; l[2] =  90.0;   /* clears +5% off 100 AND makes a new low */
   h[3] = 120.0; l[3] = 119.0;
   h[4] = 121.0; l[4] = 120.0;
   h[5] = 121.0; l[5] = 120.0;
   h[6] = 121.0; l[6] = 120.0;
   h[7] = 121.0; l[7] = 120.0;

   rc = TA_ZIGZAG( 0, 7, h, l, 5.0, 1, &begIdx, &nbElement,
                   zzOut, zzTrend, zzPivot );
   if( rc != TA_SUCCESS || begIdx != 1 )
   {
      printf( "Fail: ZIGZAG ordering rc=%d begIdx=%d\n", (int)rc, (int)begIdx );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   /* Bar 2 is index 1 of the output. */
   if( zzTrend[1] != 1 )
   {
      printf( "Fail: ZIGZAG ordering, bar 2 trend %d, expected +1: the outside "
              "bar must REVERSE the leg, not extend it\n", (int)zzTrend[1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_zzOrderCmp++;

   if( zzOut[1] != 120.0 )
   {
      printf( "Fail: ZIGZAG ordering, bar 2 price %.17g, expected 120 (the new "
              "high), not the new low\n", zzOut[1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_zzOrderCmp++;

   if( zzPivot[1] != 2 )
   {
      printf( "Fail: ZIGZAG ordering, bar 2 pivotIdx %d, expected 2\n",
              (int)zzPivot[1] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_zzOrderCmp++;

   /* And bar 1, before it, is still the down leg off the seed. */
   if( zzTrend[0] != -1 )
   {
      printf( "Fail: ZIGZAG ordering, bar 1 trend %d, expected -1\n",
              (int)zzTrend[0] );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_zzOrderCmp++;

   return TA_TEST_PASS;
}

/* Leg 4. Ties extend to the later bar.
 *
 * A long flat stretch at one price: outZigZag never moves and outPivotIdx
 * advances every bar. That movement is the only thing the index output carries
 * and the price output cannot, which is the card's reason for having it. The
 * leg counts the bars where it happens, so a corpus without ties cannot pass it
 * by comparing nothing.
 */
static ErrorNumber zzLegTies( void )
{
   static TA_Real h[64], l[64];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   for( i = 0; i < 64; i++ )
   {
      h[i] = 50.0;
      l[i] = 50.0;
   }

   rc = TA_ZIGZAG( 0, 63, h, l, 5.0, 1, &begIdx, &nbElement,
                   zzOut, zzTrend, zzPivot );
   if( rc != TA_SUCCESS || begIdx != 1 || nbElement != 63 )
   {
      printf( "Fail: ZIGZAG ties rc=%d begIdx=%d nbElement=%d\n", (int)rc,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < nbElement; i++ )
   {
      if( zzOut[i] != 50.0 || zzTrend[i] != -1 )
      {
         printf( "Fail: ZIGZAG ties bar %d gives %.17g/%d, expected 50/-1: a "
                 "flat series never clears the threshold\n",
                 (int)begIdx+i, zzOut[i], (int)zzTrend[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      if( zzPivot[i] != begIdx + i )
      {
         printf( "Fail: ZIGZAG ties bar %d pivotIdx %d, expected %d: an equal "
                 "low must move the pivot to the LATER bar\n",
                 (int)begIdx+i, (int)zzPivot[i], (int)begIdx+i );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzTieCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 5. The lookback, and the seed counted toward the first gate.
 *
 * The lookback is optInMinTrendLength and does not move with the sensitivity.
 * The series below puts a +10% bar exactly at the first output bar for each m,
 * so a reversal fires there -- which it can only do if the seed's own bar
 * counts toward the gate. The other reading of the rule delays it by one bar,
 * and this is the one place that shows.
 */
static ErrorNumber zzLegLookback( TA_History *history )
{
   static TA_Real h[64], l[64];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int m, i, got;
   double s;

   for( m = 1; m <= 4; m++ )
   {
      /* The seed is bar 0's low. Every later low sits ABOVE it so the tie rule
       * never moves the pivot -- if it did, the counter would reset and the
       * gate could not open at bar m, which is the whole point being measured.
       */
      h[0] = 100.0; l[0] = 100.0;
      for( i = 1; i < 64; i++ )
      {
         h[i] = 101.0;
         l[i] = 101.0;
      }
      /* The first output bar is m; put the breakout exactly there. */
      h[m] = 130.0;

      got = TA_ZIGZAG_Lookback( 5.0, m );
      if( got != m )
      {
         printf( "Fail: TA_ZIGZAG_Lookback(5,%d) = %d, expected %d\n",
                 m, got, m );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzLookbackCmp++;

      /* ... and it does not move with the sensitivity. */
      for( s = 0.0; s <= 100.0; s += 50.0 )
      {
         if( TA_ZIGZAG_Lookback( s, m ) != m )
         {
            printf( "Fail: TA_ZIGZAG_Lookback(%g,%d) moved with the "
                    "sensitivity\n", s, m );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }

      rc = TA_ZIGZAG( 0, 63, h, l, 5.0, m, &begIdx, &nbElement,
                      zzOut, zzTrend, zzPivot );
      if( rc != TA_SUCCESS || begIdx != m )
      {
         printf( "Fail: ZIGZAG seed m=%d rc=%d begIdx=%d, expected %d\n",
                 m, (int)rc, (int)begIdx, m );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      if( zzTrend[0] != 1 || zzPivot[0] != m || zzOut[0] != 130.0 )
      {
         printf( "Fail: ZIGZAG seed m=%d, first output gives %.17g/%d/%d, "
                 "expected 130/+1/%d: the reversal must fire ON the first "
                 "output bar, which it can only do if the seed's own bar counts "
                 "toward the gate\n", m, zzOut[0], (int)zzTrend[0],
                 (int)zzPivot[0], m );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzLookbackCmp++;
   }

   (void)history;
   return TA_TEST_PASS;
}

/* Leg 6. Path dependence.
 *
 * The disagreement between two starts begins on the later run's first output
 * bar, because it is the seed, and the machine then re-synchronises. The leg
 * pins both halves: "they differ" alone would also pass a function that never
 * re-synchronises.
 */
static ErrorNumber zzLegPath( TA_History *history )
{
   static TA_Real outB[ZZ_NB_BAR];
   static TA_Integer trendB[ZZ_NB_BAR], pivotB[ZZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begA, nbA, begB, nbB;
   int i, off, firstDiff = -1, lastDiff = -1, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_ZIGZAG( 0, nbBars-1, history->high, history->low, 2.0, 4,
                   &begA, &nbA, zzOut, zzTrend, zzPivot );
   if( rc != TA_SUCCESS ) goto pathFail;

   rc = TA_ZIGZAG( 5, nbBars-1, history->high, history->low, 2.0, 4,
                   &begB, &nbB, outB, trendB, pivotB );
   if( rc != TA_SUCCESS ) goto pathFail;

   off = begB - begA;
   for( i = 0; i < nbB; i++ )
   {
      if( zzOut[off+i]   != outB[i]   ||
          zzTrend[off+i] != trendB[i] ||
          zzPivot[off+i] != pivotB[i] )
      {
         if( firstDiff < 0 ) firstDiff = i;
         lastDiff = i;
      }
   }

   if( firstDiff != 0 )
   {
      printf( "Fail: ZIGZAG from 0 and from 5 first disagree at offset %d, "
              "expected 0: the divergence is the seed, so it can only start on "
              "the later run's first output bar\n", firstDiff );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_zzPathCmp++;

   if( lastDiff >= nbB - 1 )
   {
      printf( "Fail: ZIGZAG from 0 and from 5 still disagree at the last "
              "overlapping bar: the machine never re-synchronises, which is a "
              "stronger property than path_dependent claims\n" );
      return TA_TESTUTIL_TFRR_BAD_CALCULATION;
   }
   g_zzPathCmp++;

   return TA_TEST_PASS;

pathFail:
   printf( "Fail: ZIGZAG path leg rc=%d\n", (int)rc );
   return TA_TESTUTIL_TFRR_BAD_RETCODE;
}

/* Leg 7. Parameter edges. The ranges are inclusive, so 0 and 100 are accepted
 * and defined.
 *
 * At 0 with a gate of 1 every bar moves the pivot onto itself: a bar whose
 * high reaches a low pivot reverses, and otherwise its low is under the pivot
 * and extends it (mirrored on an up leg).
 *
 * At 100 the threshold off a high pivot is zero, so an up leg never turns back
 * on a positive price. The corpus never turns up at 100, hence the series
 * below: bar 1 doubles the seed and the lows then collapse.
 */
static ErrorNumber zzLegEdges( TA_History *history )
{
   static const TA_Real h[4] = { 100.0, 250.0, 250.0, 250.0 };
   static const TA_Real l[4] = { 100.0, 240.0,   1.0,   0.5 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_ZIGZAG( 0, nbBars-1, history->high, history->low, 0.0, 1,
                   &begIdx, &nbElement, zzOut, zzTrend, zzPivot );
   if( rc != TA_SUCCESS || begIdx != 1 || nbElement != nbBars-1 )
   {
      printf( "Fail: ZIGZAG s=0 rc=%d begIdx=%d nbElement=%d\n", (int)rc,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < nbElement; i++ )
   {
      if( zzPivot[i] != begIdx+i )
      {
         printf( "Fail: ZIGZAG s=0 bar %d pivotIdx %d: at a zero threshold "
                 "every bar is the pivot\n", (int)begIdx+i, (int)zzPivot[i] );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzEdgeCmp++;
   }

   rc = TA_ZIGZAG( 0, 3, h, l, 100.0, 1, &begIdx, &nbElement,
                   zzOut, zzTrend, zzPivot );
   if( rc != TA_SUCCESS || begIdx != 1 || nbElement != 3 )
   {
      printf( "Fail: ZIGZAG s=100 rc=%d begIdx=%d nbElement=%d\n", (int)rc,
              (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   for( i = 0; i < 3; i++ )
   {
      if( zzTrend[i] != 1 || zzOut[i] != 250.0 || zzPivot[i] != i+1 )
      {
         printf( "Fail: ZIGZAG s=100 bar %d gives %.17g/%d/%d, expected "
                 "250/1/%d: an up leg never turns back at 100\n", i+1,
                 zzOut[i], (int)zzTrend[i], (int)zzPivot[i], i+1 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzEdgeCmp++;
   }

   rc = TA_ZIGZAG( 0, nbBars-1, history->high, history->low, 5.0, 0,
                   &begIdx, &nbElement, zzOut, zzTrend, zzPivot );
   if( rc != TA_BAD_PARAM )
   {
      printf( "Fail: ZIGZAG m=0 rc=%d, expected TA_BAD_PARAM\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }
   g_zzEdgeCmp++;

   return TA_TEST_PASS;
}

/* Leg 8. Aliasing: outZigZag over each input in turn. */
static ErrorNumber zzLegAlias( TA_History *history )
{
   static TA_Real buf[ZZ_NB_BAR];
   static TA_Real baseOut[ZZ_NB_BAR];
   static TA_Integer baseTrend[ZZ_NB_BAR], basePivot[ZZ_NB_BAR];
   TA_RetCode rc;
   TA_Integer begIdx, nbElement, baseBeg, baseNb;
   int src, i, nbBars;

   nbBars = (int)history->nbBars;

   rc = TA_ZIGZAG( 0, nbBars-1, history->high, history->low, 2.0, 1,
                   &baseBeg, &baseNb, baseOut, baseTrend, basePivot );
   if( rc != TA_SUCCESS )
   {
      printf( "Fail: ZIGZAG alias baseline rc=%d\n", (int)rc );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( src = 0; src < 2; src++ )
   {
      memcpy( buf, src == 0 ? history->high : history->low,
              nbBars * sizeof(TA_Real) );

      rc = TA_ZIGZAG( 0, nbBars-1, src == 0 ? buf : history->high,
                      src == 0 ? history->low : buf, 2.0, 1,
                      &begIdx, &nbElement, buf, zzTrend, zzPivot );
      if( rc != TA_SUCCESS || begIdx != baseBeg || nbElement != baseNb )
      {
         printf( "Fail: ZIGZAG alias over input %d rc=%d\n", src, (int)rc );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         if( buf[i] != baseOut[i] || zzTrend[i] != baseTrend[i] ||
             zzPivot[i] != basePivot[i] )
         {
            printf( "Fail: ZIGZAG outZigZag aliased over input %d differs at "
                    "bar %d\n", src, (int)baseBeg+i );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
      }
      g_zzAliasCmp++;
   }

   return TA_TEST_PASS;
}

/* Leg 9. Every bar against LEAN. */
static ErrorNumber zzLegPerBar( TA_History *history )
{
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int k, i, nbBars, cell, wantTrend, wantIdx;
   TA_Real wantPrice;

   nbBars = (int)history->nbBars;

   for( k = 0; k < ZZ_NB_BAR_CASE; k++ )
   {
      const ZzCase *c = &zzBarCases[k];

      rc = TA_ZIGZAG( 0, nbBars-1, history->high, history->low,
                      c->sensitivity, c->minTrendLength,
                      &begIdx, &nbElement, zzOut, zzTrend, zzPivot );
      if( rc != TA_SUCCESS || begIdx != c->minTrendLength ||
          nbElement != nbBars - c->minTrendLength )
      {
         printf( "Fail: ZIGZAG per-bar s=%g m=%d rc=%d begIdx=%d nbElement=%d\n",
                 c->sensitivity, c->minTrendLength, (int)rc, (int)begIdx,
                 (int)nbElement );
         return TA_TESTUTIL_TFRR_BAD_RETCODE;
      }

      for( i = 0; i < nbElement; i++ )
      {
         cell = zzPerBar[k][begIdx+i];
         wantTrend = cell > 0 ? 1 : -1;
         wantIdx = cell * wantTrend - 1;
         if( cell == 0 || wantIdx > begIdx+i )
         {
            printf( "Fail: ZIGZAG per-bar table s=%g m=%d bar %d holds %d\n",
                    c->sensitivity, c->minTrendLength, (int)begIdx+i, cell );
            return TA_TESTUTIL_TFRR_BAD_PARAM;
         }
         wantPrice = wantTrend > 0 ? history->high[wantIdx] : history->low[wantIdx];

         if( zzTrend[i] != wantTrend )
         {
            printf( "Fail: ZIGZAG s=%g m=%d bar %d trend %d, LEAN gives %d\n",
                    c->sensitivity, c->minTrendLength, (int)begIdx+i,
                    (int)zzTrend[i], wantTrend );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_zzPerBarCmp++;

         if( zzPivot[i] != wantIdx )
         {
            printf( "Fail: ZIGZAG s=%g m=%d bar %d pivotIdx %d, LEAN gives %d\n",
                    c->sensitivity, c->minTrendLength, (int)begIdx+i,
                    (int)zzPivot[i], wantIdx );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_zzPerBarCmp++;

         if( zzOut[i] != wantPrice )
         {
            printf( "Fail: ZIGZAG s=%g m=%d bar %d price %.17g, LEAN gives %.17g\n",
                    c->sensitivity, c->minTrendLength, (int)begIdx+i,
                    zzOut[i], wantPrice );
            return TA_TESTUTIL_TFRR_BAD_CALCULATION;
         }
         g_zzPerBarCmp++;
      }
   }

   return TA_TEST_PASS;
}

/* Leg 10. A move of exactly the sensitivity reverses, in both directions.
 *
 * At 25 percent every product is exact in binary: 100*1.25 is 125 and 125*0.75
 * is 93.75, so each bar below sits on its threshold and not near it. No bar of
 * the 252-bar corpus does at the parameter sets of legs 1 and 9, which leaves
 * >= and > indistinguishable there.
 * LEAN (fed the sentinel bar) and talipp 2.7.0 give the same three pivots.
 */
static ErrorNumber zzLegThreshold( void )
{
   static const TA_Real h[3] = { 100.0, 125.0, 125.0  };
   static const TA_Real l[3] = { 100.0, 110.0,  93.75 };
   static const TA_Real wantPrice[2] = { 125.0, 93.75 };
   static const int     wantTrend[2] = { 1, -1 };
   TA_RetCode rc;
   TA_Integer begIdx, nbElement;
   int i;

   rc = TA_ZIGZAG( 0, 2, h, l, 25.0, 1, &begIdx, &nbElement,
                   zzOut, zzTrend, zzPivot );
   if( rc != TA_SUCCESS || begIdx != 1 || nbElement != 2 )
   {
      printf( "Fail: ZIGZAG threshold rc=%d begIdx=%d nbElement=%d\n",
              (int)rc, (int)begIdx, (int)nbElement );
      return TA_TESTUTIL_TFRR_BAD_RETCODE;
   }

   for( i = 0; i < 2; i++ )
   {
      if( zzOut[i] != wantPrice[i] || zzTrend[i] != wantTrend[i] ||
          zzPivot[i] != i + 1 )
      {
         printf( "Fail: ZIGZAG threshold bar %d gives %.17g/%d/%d, expected "
                 "%.17g/%d/%d: a move of exactly the sensitivity reverses\n",
                 i + 1, zzOut[i], (int)zzTrend[i], (int)zzPivot[i],
                 wantPrice[i], wantTrend[i], i + 1 );
         return TA_TESTUTIL_TFRR_BAD_CALCULATION;
      }
      g_zzThreshCmp++;
   }

   return TA_TEST_PASS;
}
