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
 *  112726 MF,CC  First version (issue #622).
 */

/* Description:
 *
 *   TA_SetEMAVariation(TA_EMAVariation_FIRST_BAR_SEED), across functions.
 *
 *   Legs:
 *     1. Scope, over the whole abstract table: at the default integer
 *        parameters, and at each value of each MAType parameter, the output
 *        moves under the setting exactly when the function is in evInline,
 *        or is in evByMaType with an EMA, DEMA, TEMA or ZLEMA MA type. Every
 *        other cell is bit-identical. A function that starts reading the
 *        setting fails until it is added; a row that stops moving fails.
 *     2. On every call of leg 1, and at off-default integer parameters:
 *        lookback, retCode, outBegIdx and outNBElement are the default's.
 *     3. Range independence of the single-stage functions: the value at a bar
 *        does not depend on startIdx, bit for bit. The chained ones do depend
 *        on it (a stage fed by another seeds on the first value that one
 *        publishes in the call), and so do the single-stage ones under the
 *        default; both are counted, so the probe is shown to see a start.
 *     4. MACD's line is TA_EMA(fast) - TA_EMA(slow) from bar 0, bit for bit.
 *     5. Goldens from the seed rule for the functions no frozen release
 *        reproduces, TRIX past its lookback among them. Each reference below is written from the rule in plain
 *        loops, not from the bodies and not from an outside product; stages
 *        the setting does not touch (ATR, RSI, VAR, non-EMA MA types) come
 *        from the library at the window the function hands them. Routed
 *        through server_verify under the setting.
 *
 *   The frozen-release oracle (0.6.4 under TA_COMPATIBILITY_METASTOCK) is the
 *   --ref leg; the hand-written MetaStock rows are in test_ma.c, test_macd.c,
 *   test_po.c and test_bbands.c.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "server_verify.h"

/**** Local declarations. ****/
#define EV_N      252
#define EV_MAXOUT 8
#define EV_MAXOPT 16
#define EV_LATE   100

/* Of the output's range over the call, floored at 1, against the rule's plain
 * loops: 3x the worst measured over the cases below, 2.2e-14 (TRIX(30), a
 * one-bar rate of change of its third stage). */
#define EV_GOLD_TOL 7e-14

typedef struct
{
   TA_RetCode   rc;
   TA_Integer   beg, nb, lookback;
   unsigned int nbOut;
   int          isInt[EV_MAXOUT];
   double       real[EV_MAXOUT][EV_N];
   int          integer[EV_MAXOUT][EV_N];
} EvRun;

/* The functions whose own body follows the setting at their default
 * parameters, whatever their MA types. */
static const char *const evInline[] = {
   "CVI", "DEMA", "DOSC", "EFI", "EMA", "ERI", "KC", "MACD", "MACDFIX", "MACDV",
   "MASSI", "PSO", "SMI", "STC", "TEMA", "TRIX", "TSI", "WAVETREND", "ZLEMA"
};
/* The functions that follow it through an MA type in evEmaFamily. */
static const char *const evByMaType[] = {
   "APO", "BBANDS", "BBW", "KDJ", "KSTEXT", "MA", "MACDEXT", "MAENV", "MAVP",
   "PERCENTB", "PPO", "PVO", "STOCH", "STOCHF", "STOCHRSI"
};
static const int evEmaFamily[] = { TA_MAType_EMA, TA_MAType_DEMA, TA_MAType_TEMA, TA_MAType_ZLEMA };

#define EV_NB_INLINE  ((int)(sizeof(evInline)/sizeof(evInline[0])))
#define EV_NB_BYTYPE  ((int)(sizeof(evByMaType)/sizeof(evByMaType[0])))

static const TA_History *g_evHist;
static double g_evPeriods[EV_N];
static EvRun  g_evA, g_evB, g_evC;

static int  g_evInlineSeen[EV_NB_INLINE], g_evInlineMoved[EV_NB_INLINE];
static int  g_evByTypeSeen[EV_NB_BYTYPE], g_evByTypeMoved[EV_NB_BYTYPE];
static long g_evScopeCells, g_evScopeStill, g_evShapeCalls, g_evOffDefault;
static long g_evIndepCmp, g_evIndepDefaultMoved, g_evChainMoved, g_evIdentCmp, g_evGoldCmp;
static int  g_evFuncs;

static int ev_find( const char *const *list, int n, const char *name )
{
   int i;
   for( i = 0; i < n; i++ )
      if( strcmp( list[i], name ) == 0 )
         return i;
   return -1;
}

static int ev_is_ema_family( int t )
{
   unsigned int i;
   for( i = 0; i < sizeof(evEmaFamily)/sizeof(evEmaFamily[0]); i++ )
      if( evEmaFamily[i] == t )
         return 1;
   return 0;
}

static int ev_is_matype( const TA_OptInputParameterInfo *oi )
{
   return oi->type == TA_OptInput_IntegerList && strstr( oi->paramName, "MAType" ) != NULL;
}

static void ev_defaults( const TA_FuncInfo *fi, double *opt )
{
   unsigned int i;
   for( i = 0; i < fi->nbOptInput && i < EV_MAXOPT; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_GetOptInputParameterInfo( fi->handle, i, &oi );
      opt[i] = oi->defaultValue;
   }
}

/* TA_FALSE when the function has an input this file does not feed. */
static int ev_feedable( const TA_FuncInfo *fi )
{
   unsigned int i;
   if( fi->nbOptInput > EV_MAXOPT || fi->nbOutput > EV_MAXOUT )
      return 0;
   for( i = 0; i < fi->nbInput; i++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo( fi->handle, i, &ii );
      if( ii->type == TA_Input_Integer )
         return 0;
   }
   return 1;
}

static const double *ev_real_input( const TA_FuncInfo *fi, int nth )
{
   if( nth == 0 )
      return g_evHist->close;
   return strcmp( fi->name, "MAVP" ) == 0 ? g_evPeriods : g_evHist->high;
}

/* The whole series, startIdx..EV_N-1, through the abstract interface. */
static void ev_call( const TA_FuncInfo *fi, const double *opt, int start, EvRun *r )
{
   TA_ParamHolder *ph;
   unsigned int i;
   int nReal = 0;

   memset( r, 0, sizeof(*r) );
   r->lookback = -1;
   r->nbOut = fi->nbOutput;
   if( TA_ParamHolderAlloc( fi->handle, &ph ) != TA_SUCCESS )
   {
      r->rc = TA_ALLOC_ERR;
      return;
   }
   for( i = 0; i < fi->nbInput; i++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo( fi->handle, i, &ii );
      if( ii->type == TA_Input_Price )
         TA_SetInputParamPricePtr( ph, i, g_evHist->open, g_evHist->high, g_evHist->low,
                                   g_evHist->close, g_evHist->volume, g_evHist->volume );
      else
         TA_SetInputParamRealPtr( ph, i, ev_real_input( fi, nReal++ ) );
   }
   for( i = 0; i < fi->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_RetCode rc;
      TA_GetOptInputParameterInfo( fi->handle, i, &oi );
      if( oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList )
         rc = TA_SetOptInputParamReal( ph, i, opt[i] );
      else
         rc = TA_SetOptInputParamInteger( ph, i, (TA_Integer)opt[i] );
      if( rc != TA_SUCCESS )
      {
         r->rc = rc;
         TA_ParamHolderFree( ph );
         return;
      }
   }
   for( i = 0; i < fi->nbOutput; i++ )
   {
      const TA_OutputParameterInfo *oo;
      TA_GetOutputParameterInfo( fi->handle, i, &oo );
      r->isInt[i] = oo->type == TA_Output_Integer;
      if( r->isInt[i] )
         TA_SetOutputParamIntegerPtr( ph, i, r->integer[i] );
      else
         TA_SetOutputParamRealPtr( ph, i, r->real[i] );
   }
   if( TA_GetLookback( ph, &r->lookback ) != TA_SUCCESS )
      r->lookback = -1;
   r->rc = TA_CallFunc( ph, start, EV_N-1, &r->beg, &r->nb );
   TA_ParamHolderFree( ph );
}

static void ev_call_under( const TA_FuncInfo *fi, const double *opt, int start,
                           TA_EMAVariation v, EvRun *r )
{
   TA_SetEMAVariation( v );
   ev_call( fi, opt, start, r );
   TA_SetEMAVariation( TA_EMAVariation_SMA_SEED );
}

static int ev_same_shape( const EvRun *a, const EvRun *b )
{
   return a->rc == b->rc && a->lookback == b->lookback
          && (a->rc != TA_SUCCESS || (a->beg == b->beg && a->nb == b->nb));
}

static int ev_same_values( const EvRun *a, const EvRun *b )
{
   unsigned int o;
   if( a->rc != TA_SUCCESS )
      return 1;
   for( o = 0; o < a->nbOut; o++ )
   {
      size_t bytes = (size_t)a->nb * (a->isInt[o] ? sizeof(int) : sizeof(double));
      const void *x = a->isInt[o] ? (const void *)a->integer[o] : (const void *)a->real[o];
      const void *y = b->isInt[o] ? (const void *)b->integer[o] : (const void *)b->real[o];
      if( memcmp( x, y, bytes ) != 0 )
         return 0;
   }
   return 1;
}

static void ev_print_vec( const TA_FuncInfo *fi, const double *opt )
{
   unsigned int i;
   for( i = 0; i < fi->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      TA_GetOptInputParameterInfo( fi->handle, i, &oi );
      printf( " %s=%.17g", oi->paramName, opt[i] );
   }
}

/* Leg 2 on one call: the setting moves no lookback and no range. */
static ErrorNumber ev_check_shape( const TA_FuncInfo *fi, const double *opt, int start,
                                   const EvRun *a, const EvRun *b )
{
   g_evShapeCalls++;
   if( ev_same_shape( a, b ) )
      return TA_TEST_PASS;
   printf( "\nEMA VARIATION SHAPE [TA_%s] start=%d:", fi->name, start );
   ev_print_vec( fi, opt );
   printf( "\n  SMA_SEED rc=%d lookback=%d (%d,%d), FIRST_BAR_SEED rc=%d lookback=%d (%d,%d)\n",
           (int)a->rc, a->lookback, a->beg, a->nb, (int)b->rc, b->lookback, b->beg, b->nb );
   return TA_EMAV_SHAPE;
}

/* Whether the cell must move: by name, then by the effective MA types. */
static int ev_expected( const TA_FuncInfo *fi, const double *opt )
{
   unsigned int i;
   if( ev_find( evInline, EV_NB_INLINE, fi->name ) >= 0 )
      return 1;
   if( ev_find( evByMaType, EV_NB_BYTYPE, fi->name ) < 0 )
      return 0;
   for( i = 0; i < fi->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      int t;
      TA_GetOptInputParameterInfo( fi->handle, i, &oi );
      if( !ev_is_matype( oi ) )
         continue;
      t = (int)opt[i];
      if( t == TA_MAType_DEFAULT )
         t = (int)oi->defaultValue;
      if( ev_is_ema_family( t ) )
         return 1;
   }
   return 0;
}

static ErrorNumber ev_scope_cell( const TA_FuncInfo *fi, const double *opt )
{
   static const int starts[2] = { 0, EV_LATE };
   int expected = ev_expected( fi, opt ), moved = 0, s, k;
   ErrorNumber e;

   for( s = 0; s < 2; s++ )
   {
      ev_call_under( fi, opt, starts[s], TA_EMAVariation_SMA_SEED, &g_evA );
      ev_call_under( fi, opt, starts[s], TA_EMAVariation_FIRST_BAR_SEED, &g_evB );
      e = ev_check_shape( fi, opt, starts[s], &g_evA, &g_evB );
      if( e != TA_TEST_PASS )
         return e;
      if( g_evA.rc == TA_SUCCESS && g_evA.nb > 0 && !ev_same_values( &g_evA, &g_evB ) )
         moved = 1;
   }
   if( g_evA.rc != TA_SUCCESS )
   {
      printf( "\nEMA VARIATION SCOPE [TA_%s]: rc=%d at", fi->name, (int)g_evA.rc );
      ev_print_vec( fi, opt );
      printf( "\n" );
      return TA_EMAV_SCOPE;
   }
   g_evScopeCells++;
   if( moved != expected )
   {
      printf( "\nEMA VARIATION SCOPE [TA_%s]: %s under FIRST_BAR_SEED at", fi->name,
              moved ? "moved" : "did not move" );
      ev_print_vec( fi, opt );
      printf( "\n  %s\n", moved ? "add it to evInline or evByMaType, with what it seeds"
                                : "a row that no longer follows the setting: why?" );
      return TA_EMAV_SCOPE;
   }
   if( !moved )
      g_evScopeStill++;
   else if( (k = ev_find( evInline, EV_NB_INLINE, fi->name )) >= 0 )
      g_evInlineMoved[k]++;
   else if( (k = ev_find( evByMaType, EV_NB_BYTYPE, fi->name )) >= 0 )
      g_evByTypeMoved[k]++;
   return TA_TEST_PASS;
}

static ErrorNumber ev_off_call( const TA_FuncInfo *fi, const double *opt )
{
   ev_call_under( fi, opt, 0, TA_EMAVariation_SMA_SEED, &g_evA );
   ev_call_under( fi, opt, 0, TA_EMAVariation_FIRST_BAR_SEED, &g_evB );
   g_evOffDefault++;
   return ev_check_shape( fi, opt, 0, &g_evA, &g_evB );
}

/* Leg 2 off the defaults: each integer range over its minimum, one above, 3,
 * 7, its default and the default plus 5, every pair of values when the
 * function has at most two ranges, else one range at a time. */
static ErrorNumber ev_off_default( const TA_FuncInfo *fi, const double *dflt )
{
   double opt[EV_MAXOPT];
   long grid[EV_MAXOPT][6];
   int at[EV_MAXOPT], nGrid[EV_MAXOPT], nInt = 0, j, g, h;
   unsigned int i;
   ErrorNumber e = TA_TEST_PASS;

   for( i = 0; i < fi->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      const TA_IntegerRange *r;
      long cand[6];
      int c;
      TA_GetOptInputParameterInfo( fi->handle, i, &oi );
      if( oi->type != TA_OptInput_IntegerRange || !oi->dataSet )
         continue;
      r = (const TA_IntegerRange *)oi->dataSet;
      cand[0] = r->min; cand[1] = (long)r->min + 1; cand[2] = 3; cand[3] = 7;
      cand[4] = (long)oi->defaultValue; cand[5] = (long)oi->defaultValue + 5;
      at[nInt] = (int)i;
      nGrid[nInt] = 0;
      for( c = 0; c < 6; c++ )
      {
         if( cand[c] < (long)r->min || cand[c] > (long)r->max )
            continue;
         for( g = 0; g < nGrid[nInt] && grid[nInt][g] != cand[c]; g++ ) {}
         if( g == nGrid[nInt] )
            grid[nInt][nGrid[nInt]++] = cand[c];
      }
      nInt++;
   }
   if( nInt <= 2 )
   {
      for( g = 0; nInt > 0 && g < nGrid[0] && e == TA_TEST_PASS; g++ )
         for( h = 0; h < (nInt == 2 ? nGrid[1] : 1) && e == TA_TEST_PASS; h++ )
         {
            memcpy( opt, dflt, sizeof(opt) );
            opt[at[0]] = (double)grid[0][g];
            if( nInt == 2 )
               opt[at[1]] = (double)grid[1][h];
            if( memcmp( opt, dflt, sizeof(opt) ) != 0 )
               e = ev_off_call( fi, opt );
         }
      return e;
   }
   for( j = 0; j < nInt && e == TA_TEST_PASS; j++ )
      for( g = 0; g < nGrid[j] && e == TA_TEST_PASS; g++ )
      {
         memcpy( opt, dflt, sizeof(opt) );
         opt[at[j]] = (double)grid[j][g];
         if( memcmp( opt, dflt, sizeof(opt) ) != 0 )
            e = ev_off_call( fi, opt );
      }
   return e;
}

typedef struct { ErrorNumber err; } EvScopeCtx;

static void ev_scope_one( const TA_FuncInfo *fi, void *opaque )
{
   EvScopeCtx *ctx = (EvScopeCtx *)opaque;
   double dflt[EV_MAXOPT], opt[EV_MAXOPT];
   unsigned int i, j;
   int k;

   if( ctx->err != TA_TEST_PASS )
      return;
   if( !ev_feedable( fi ) )
   {
      printf( "\nEMA VARIATION SCOPE [TA_%s]: an input or a count this sweep does not "
              "handle; it would go unswept\n", fi->name );
      ctx->err = TA_EMAV_SCOPE;
      return;
   }
   g_evFuncs++;
   if( (k = ev_find( evInline, EV_NB_INLINE, fi->name )) >= 0 ) g_evInlineSeen[k] = 1;
   if( (k = ev_find( evByMaType, EV_NB_BYTYPE, fi->name )) >= 0 ) g_evByTypeSeen[k] = 1;

   memset( dflt, 0, sizeof(dflt) );
   ev_defaults( fi, dflt );
   ctx->err = ev_scope_cell( fi, dflt );
   for( i = 0; i < fi->nbOptInput && ctx->err == TA_TEST_PASS; i++ )
   {
      const TA_OptInputParameterInfo *oi;
      const TA_IntegerList *list;
      TA_GetOptInputParameterInfo( fi->handle, i, &oi );
      if( !ev_is_matype( oi ) || !oi->dataSet )
         continue;
      list = (const TA_IntegerList *)oi->dataSet;
      for( j = 0; j < list->nbElement && ctx->err == TA_TEST_PASS; j++ )
      {
         if( list->data[j].value == (int)oi->defaultValue )
            continue;
         memcpy( opt, dflt, sizeof(opt) );
         opt[i] = (double)list->data[j].value;
         ctx->err = ev_scope_cell( fi, opt );
      }
   }
   if( ctx->err == TA_TEST_PASS )
      ctx->err = ev_off_default( fi, dflt );
}

static ErrorNumber test_ema_variation_scope( void )
{
   EvScopeCtx ctx;
   int k;

   ctx.err = TA_TEST_PASS;
   memset( g_evInlineSeen, 0, sizeof(g_evInlineSeen) );
   memset( g_evInlineMoved, 0, sizeof(g_evInlineMoved) );
   memset( g_evByTypeSeen, 0, sizeof(g_evByTypeSeen) );
   memset( g_evByTypeMoved, 0, sizeof(g_evByTypeMoved) );
   TA_ForEachFunc( ev_scope_one, &ctx );
   if( ctx.err != TA_TEST_PASS )
      return ctx.err;

   for( k = 0; k < EV_NB_INLINE; k++ )
      if( !g_evInlineSeen[k] || !g_evInlineMoved[k] )
      {
         printf( "\nEMA VARIATION SCOPE: evInline names %s, which %s\n", evInline[k],
                 g_evInlineSeen[k] ? "never moved" : "is not a function" );
         return TA_EMAV_VACUOUS;
      }
   for( k = 0; k < EV_NB_BYTYPE; k++ )
      if( !g_evByTypeSeen[k] || !g_evByTypeMoved[k] )
      {
         printf( "\nEMA VARIATION SCOPE: evByMaType names %s, which %s\n", evByMaType[k],
                 g_evByTypeSeen[k] ? "never moved" : "is not a function" );
         return TA_EMAV_VACUOUS;
      }
   if( g_evScopeStill == 0 || g_evOffDefault == 0 || g_evShapeCalls < 2 * g_evScopeCells )
   {
      printf( "\nEMA VARIATION SCOPE VACUOUS: %ld still cell(s), %ld off-default vector(s), "
              "%ld shape check(s) for %ld cell(s)\n", g_evScopeStill, g_evOffDefault,
              g_evShapeCalls, g_evScopeCells );
      return TA_EMAV_VACUOUS;
   }
   return TA_TEST_PASS;
}

/* ---- Leg 3 ---- */

typedef struct
{
   const char *name;
   int         maType;  /* -1: the function's own; else every MAType parameter */
   int         out;     /* -1: every output */
   int         chained; /* 1: a stage fed by another, so the start shows */
} EvIndep;

static const EvIndep evIndep[] = {
   { "EMA",       -1,               -1, 0 },
   { "CVI",       -1,               -1, 0 },
   { "EFI",       -1,               -1, 0 },
   { "ERI",       -1,               -1, 0 },
   { "ZLEMA",     -1,               -1, 0 },
   { "MA",        TA_MAType_EMA,    -1, 0 },
   { "MA",        TA_MAType_ZLEMA,  -1, 0 },
   { "MAENV",     TA_MAType_EMA,    -1, 0 },
   { "MAENV",     TA_MAType_ZLEMA,  -1, 0 },
   { "KC",        -1,                1, 0 },  /* the middle band; ATR's bands are Wilder */
   { "MACD",      -1,                0, 0 },  /* the line: two input-fed legs */
   { "DEMA",      -1,               -1, 1 },
   { "TEMA",      -1,               -1, 1 },
   { "TRIX",      -1,               -1, 1 },
   { "MACD",      -1,                1, 1 },
   { "TSI",       -1,               -1, 1 },
   { "MASSI",     -1,               -1, 1 },
   { "SMI",       -1,               -1, 1 },
   { "MA",        TA_MAType_DEMA,   -1, 1 },
   { "STOCH",     TA_MAType_EMA,    -1, 1 },  /* seeded on the call's first Fast-K (rT14) */
   { "STOCHF",    TA_MAType_EMA,    -1, 1 },
};

/* 1 when every compared bar of the late call equals the full call's. */
static int ev_bars_agree( const EvRun *full, const EvRun *late, int out )
{
   unsigned int o;
   int t;
   for( o = 0; o < full->nbOut; o++ )
   {
      if( out >= 0 && (int)o != out )
         continue;
      for( t = late->beg; t < late->beg + late->nb; t++ )
         if( memcmp( &full->real[o][t - full->beg], &late->real[o][t - late->beg], sizeof(double) ) != 0 )
            return 0;
   }
   return 1;
}

static ErrorNumber test_ema_variation_independence( void )
{
   unsigned int c, i;
   int s;

   for( c = 0; c < sizeof(evIndep)/sizeof(evIndep[0]); c++ )
   {
      const EvIndep *k = &evIndep[c];
      const TA_FuncHandle *h;
      const TA_FuncInfo *fi;
      double opt[EV_MAXOPT];
      int firstMoved = 0, defaultMoved = 0;

      if( TA_GetFuncHandle( k->name, &h ) != TA_SUCCESS || TA_GetFuncInfo( h, &fi ) != TA_SUCCESS )
      {
         printf( "\nEMA VARIATION RANGE: no function %s\n", k->name );
         return TA_EMAV_INDEPENDENCE;
      }
      memset( opt, 0, sizeof(opt) );
      ev_defaults( fi, opt );
      if( k->maType >= 0 )
         for( i = 0; i < fi->nbOptInput; i++ )
         {
            const TA_OptInputParameterInfo *oi;
            TA_GetOptInputParameterInfo( fi->handle, i, &oi );
            if( ev_is_matype( oi ) )
               opt[i] = (double)k->maType;
         }

      for( s = 0; s < 3; s++ )
      {
         int pass;
         for( pass = 0; pass < 2; pass++ )
         {
            TA_EMAVariation v = pass ? TA_EMAVariation_FIRST_BAR_SEED : TA_EMAVariation_SMA_SEED;
            int start;
            ev_call_under( fi, opt, 0, v, &g_evA );
            start = g_evA.lookback + (s == 0 ? 1 : s == 1 ? 17 : 90);
            ev_call_under( fi, opt, start, v, &g_evC );
            if( g_evA.rc != TA_SUCCESS || start >= EV_N - 1 || g_evC.rc != TA_SUCCESS
                || g_evC.beg != start || g_evC.nb != EV_N - start )
            {
               printf( "\nEMA VARIATION RANGE [TA_%s]: no output from startIdx %d (rc %d, %d)\n",
                       k->name, start, (int)g_evC.rc, g_evC.nb );
               return TA_EMAV_INDEPENDENCE;
            }
            if( ev_bars_agree( &g_evA, &g_evC, k->out ) )
               continue;
            if( !pass )
               defaultMoved = 1;
            else if( k->chained )
               firstMoved = 1;
            else
            {
               printf( "\nEMA VARIATION RANGE [TA_%s maType %d out %d]: from startIdx %d the "
                       "values differ from the whole-series call under FIRST_BAR_SEED\n",
                       k->name, k->maType, k->out, start );
               return TA_EMAV_INDEPENDENCE;
            }
         }
         if( !k->chained )
            g_evIndepCmp++;
      }
      if( !k->chained && !defaultMoved )
      {
         printf( "\nEMA VARIATION RANGE VACUOUS [TA_%s maType %d]: no start moved it under "
                 "SMA_SEED either, so the probe sees no start\n", k->name, k->maType );
         return TA_EMAV_VACUOUS;
      }
      if( k->chained && !firstMoved )
      {
         printf( "\nEMA VARIATION RANGE [TA_%s maType %d out %d]: a chained stage no longer "
                 "depends on startIdx under FIRST_BAR_SEED\n", k->name, k->maType, k->out );
         return TA_EMAV_INDEPENDENCE;
      }
      g_evIndepDefaultMoved += defaultMoved;
      g_evChainMoved += firstMoved;
   }
   return TA_TEST_PASS;
}

/* ---- Leg 4 ---- */

static ErrorNumber test_ema_variation_macd_identity( const TA_History *history )
{
   static const int cfg[][3] = { { 12, 26, 9 }, { 5, 14, 3 }, { 26, 12, 9 }, { 2, 30, 1 } };
   static double ef[EV_N], es[EV_N], mac[EV_N], sig[EV_N], hist[EV_N];
   const double *c = history->close;
   TA_Integer b1, n1, b2, n2, b, n;
   unsigned int k;
   int s, t, ext;

   TA_SetEMAVariation( TA_EMAVariation_FIRST_BAR_SEED );
   for( k = 0; k < sizeof(cfg)/sizeof(cfg[0]); k++ )
   {
      int fast = cfg[k][0] < cfg[k][1] ? cfg[k][0] : cfg[k][1];
      int slow = cfg[k][0] < cfg[k][1] ? cfg[k][1] : cfg[k][0];
      int lb = TA_MACD_Lookback( cfg[k][0], cfg[k][1], cfg[k][2] );
      int starts[3];
      starts[0] = 0; starts[1] = lb; starts[2] = EV_LATE;
      if( TA_EMA( 0, EV_N-1, c, fast, &b1, &n1, ef ) != TA_SUCCESS
          || TA_EMA( 0, EV_N-1, c, slow, &b2, &n2, es ) != TA_SUCCESS )
         goto fail;
      for( s = 0; s < 3; s++ )
      for( ext = 0; ext < 2; ext++ )
      {
         TA_RetCode rc = ext
            ? TA_MACDEXT( starts[s], EV_N-1, c, cfg[k][0], TA_MAType_EMA, cfg[k][1], TA_MAType_EMA,
                          cfg[k][2], TA_MAType_EMA, &b, &n, mac, sig, hist )
            : TA_MACD( starts[s], EV_N-1, c, cfg[k][0], cfg[k][1], cfg[k][2], &b, &n, mac, sig, hist );
         if( rc != TA_SUCCESS || n <= 0 )
            goto fail;
         for( t = b; t < b + n; t++ )
         {
            double line = ef[t - b1] - es[t - b2];
            if( memcmp( &line, &mac[t - b], sizeof(double) ) != 0 )
            {
               printf( "\nEMA VARIATION IDENTITY [TA_%s(%d,%d,%d) start %d] bar %d: %.17g is not "
                       "TA_EMA(%d) - TA_EMA(%d) from bar 0, %.17g\n", ext ? "MACDEXT" : "MACD",
                       cfg[k][0], cfg[k][1], cfg[k][2], starts[s], t, mac[t - b], fast, slow, line );
               goto fail;
            }
            g_evIdentCmp++;
         }
      }
   }
   TA_SetEMAVariation( TA_EMAVariation_SMA_SEED );
   return TA_TEST_PASS;
fail:
   TA_SetEMAVariation( TA_EMAVariation_SMA_SEED );
   return TA_EMAV_IDENTITY;
}

/* ---- Leg 5: the seed rule, in plain loops ----
 *
 *   rule 1: a stage fed by the input or a per-bar transform of it is seeded on
 *           the first bar where that series exists and runs from there;
 *   rule 2: legs of different periods start on that same bar;
 *   rule 3: a stage fed by another is seeded on the first value that one
 *           publishes in the call;
 *   rule 4: the lookback, first bar and count are the default's.
 */

static double g_evO[3][EV_N];

static int ev_lbE( int p ) { return p - 1; }

#define EV_EPS 0.00000000000001
static int ev_zs( double v, double scale ) { return fabs( v ) <= EV_EPS * scale; }

static void ev_ema_from( const double *x, int from, int p, double *e )
{
   double k = 2.0 / (p + 1.0);
   int t;
   e[from] = x[from];
   for( t = from + 1; t < EV_N; t++ )
      e[t] = k * x[t] + (1.0 - k) * e[t - 1];
}

/* An EMA-family MA (EMA, DEMA, TEMA, ZLEMA) over y[0..n-1] called at relative
 * start sr; any other type is the library's. out[j] is y-index *beg + j. */
static int ev_ma_model( int type, int p, const double *y, int n, int sr, double *out, int *beg )
{
   static double e1[EV_N], e2[EV_N], e3[EV_N], d[EV_N];
   double k = 2.0 / (p + 1.0), b = 1.0 - k;
   int L, s, i, b2, b3;

   if( !ev_is_ema_family( type ) )
   {
      int nb;
      TA_RetCode rc = TA_MA( sr, n - 1, y, p, (TA_MAType)type, beg, &nb, out );
      return rc == TA_SUCCESS ? nb : -1;
   }
   if( type == TA_MAType_EMA )        L = ev_lbE( p );
   else if( type == TA_MAType_DEMA )  L = 2 * ev_lbE( p );
   else if( type == TA_MAType_TEMA )  L = 3 * ev_lbE( p );
   else                               L = (p - 1) / 2 + ev_lbE( p );
   s = sr < L ? L : sr;
   if( s > n - 1 ) { *beg = 0; return 0; }
   *beg = s;
   if( p == 1 )
   {
      for( i = s; i < n; i++ ) out[i - s] = y[i];
      return n - s;
   }
   if( type == TA_MAType_ZLEMA )
   {
      int lag = (p - 1) / 2;
      for( i = lag; i < n; i++ ) d[i] = 2.0 * y[i] - y[i - lag];
      e1[lag] = d[lag];
      for( i = lag + 1; i < n; i++ ) e1[i] = k * d[i] + b * e1[i - 1];
      for( i = s; i < n; i++ ) out[i - s] = e1[i];
      return n - s;
   }
   e1[0] = y[0];
   for( i = 1; i < n; i++ ) e1[i] = k * y[i] + b * e1[i - 1];
   if( type == TA_MAType_EMA )
   {
      for( i = s; i < n; i++ ) out[i - s] = e1[i];
      return n - s;
   }
   b2 = type == TA_MAType_DEMA ? s - ev_lbE( p ) : s - 2 * ev_lbE( p );
   e2[b2] = e1[b2];
   for( i = b2 + 1; i < n; i++ ) e2[i] = k * e1[i] + b * e2[i - 1];
   if( type == TA_MAType_DEMA )
   {
      for( i = s; i < n; i++ ) out[i - s] = 2.0 * e1[i] - e2[i];
      return n - s;
   }
   b3 = s - ev_lbE( p );
   e3[b3] = e2[b3];
   for( i = b3 + 1; i < n; i++ ) e3[i] = k * e2[i] + b * e3[i - 1];
   for( i = s; i < n; i++ ) out[i - s] = 3.0 * e1[i] - 3.0 * e2[i] + e3[i];
   return n - s;
}

typedef struct
{
   const char *name;
   int         ip[11];
   double      dp;
} EvGold;

static const EvGold evGold[] = {
   { "TRIX",   { 30 }, 0 },
   { "TRIX",   { 9 }, 0 },
   { "ERI",    { 13 }, 0 },
   { "ERI",    { 3 }, 0 },
   { "PVO",    { 12, 26, TA_MAType_EMA }, 0 },
   { "PVO",    { 26, 5, TA_MAType_EMA }, 0 },
   { "EFI",    { 13 }, 0 },
   { "EFI",    { 2 }, 0 },
   { "CVI",    { 10, 10 }, 0 },
   { "CVI",    { 14, 5 }, 0 },
   { "TSI",    { 25, 13 }, 0 },
   { "TSI",    { 5, 14 }, 0 },
   { "MASSI",  { 9, 25 }, 0 },
   { "MASSI",  { 14, 5 }, 0 },
   { "SMI",    { 13, 2, 25, 9 }, 0 },
   { "SMI",    { 5, 14, 3, 5 }, 0 },
   { "MACDV",  { 12, 26, 9 }, 0 },
   { "MACDV",  { 26, 5, 2 }, 0 },
   { "ZLEMA",  { 30 }, 0 },
   { "ZLEMA",  { 3 }, 0 },
   { "KC",     { 20, 10 }, 2.0 },
   { "KC",     { 14, 30 }, 1.5 },
   { "PSO",    { 8, 5 }, 0 },
   { "PSO",    { 14, 2 }, 0 },
   { "WAVETREND", { 10, 21, 4 }, 0 },
   { "DOSC",   { 14, 5, 3, 9 }, 0 },
   { "KSTEXT", { 10, 15, 20, 30, 10, 10, 10, 15, 9, TA_MAType_EMA, TA_MAType_SMA }, 0 },
   { "KSTEXT", { 10, 15, 20, 30, 10, 10, 10, 15, 9, TA_MAType_SMA, TA_MAType_EMA }, 0 },
   { "KSTEXT", { 5, 7, 9, 11, 3, 4, 5, 6, 4, TA_MAType_DEMA, TA_MAType_ZLEMA }, 0 },
   { "KSTEXT", { 5, 7, 9, 11, 3, 4, 5, 6, 4, TA_MAType_TEMA, TA_MAType_TEMA }, 0 },
};

#define EV_IS(x) (strcmp( k->name, x ) == 0)

/* Fills g_evO[q][t - s] for t in [s, EV_N-1]; returns s, or < 0 when the
 * model cannot run the case. */
static int ev_model( const EvGold *k, int s0, int L )
{
   static double x[EV_N], e1[EV_N], e2[EV_N], e3[EV_N], g1[EV_N], g2[EV_N];
   static double tmp[EV_N], tmp2[EV_N], line[EV_N];
   const int *p = k->ip;
   const double *c = g_evHist->close, *h = g_evHist->high, *l = g_evHist->low;
   const double *vol = g_evHist->volume;
   int s = s0 < L ? L : s0, t, i, beg, nb;

   if( s > EV_N - 1 )
      return -1;
   memset( g_evO, 0, sizeof(g_evO) );
   if( EV_IS("TRIX") )
   {
      int b3 = s - 1 - ev_lbE( p[0] ), b2 = b3 - ev_lbE( p[0] );
      double kk = 2.0 / (p[0] + 1.0);
      ev_ema_from( c, 0, p[0], e1 );
      e2[b2] = e1[b2];
      for( t = b2 + 1; t < EV_N; t++ ) e2[t] = kk * e1[t] + (1.0 - kk) * e2[t - 1];
      e3[b3] = e2[b3];
      for( t = b3 + 1; t < EV_N; t++ ) e3[t] = kk * e2[t] + (1.0 - kk) * e3[t - 1];
      for( t = s; t < EV_N; t++ ) g_evO[0][t - s] = e3[t - 1] != 0.0 ? (e3[t] / e3[t - 1] - 1.0) * 100.0 : 0.0;
      return s;
   }
   if( EV_IS("ERI") )
   {
      ev_ema_from( c, 0, p[0], e1 );
      for( t = s; t < EV_N; t++ ) { g_evO[0][t - s] = h[t] - e1[t]; g_evO[1][t - s] = l[t] - e1[t]; }
      return s;
   }
   if( EV_IS("PVO") )
   {
      ev_ema_from( vol, 0, p[0] < p[1] ? p[0] : p[1], e1 );
      ev_ema_from( vol, 0, p[0] < p[1] ? p[1] : p[0], e2 );
      for( t = s; t < EV_N; t++ ) g_evO[0][t - s] = e2[t] != 0.0 ? (e1[t] - e2[t]) / e2[t] * 100.0 : 0.0;
      return s;
   }
   if( EV_IS("EFI") )
   {
      if( p[0] == 1 ) return -1;
      for( t = 1; t < EV_N; t++ ) x[t] = (c[t] - c[t - 1]) * vol[t];
      ev_ema_from( x, 1, p[0], e1 );
      for( t = s; t < EV_N; t++ ) g_evO[0][t - s] = e1[t];
      return s;
   }
   if( EV_IS("ZLEMA") )
   {
      nb = ev_ma_model( TA_MAType_ZLEMA, p[0], c, EV_N, s, g_evO[0], &beg );
      return nb > 0 ? beg : -1;
   }
   if( EV_IS("CVI") )
   {
      for( t = 0; t < EV_N; t++ ) x[t] = h[t] - l[t];
      ev_ema_from( x, 0, p[0], e1 );
      for( t = s; t < EV_N; t++ )
      {
         double lag = e1[t - p[1]];
         g_evO[0][t - s] = lag != 0.0 ? 100.0 * ((e1[t] - lag) / lag) : 0.0;
      }
      return s;
   }
   if( EV_IS("MASSI") )
   {
      int b2 = s - L + ev_lbE( p[0] );
      double kk = 2.0 / (p[0] + 1.0);
      for( t = 0; t < EV_N; t++ ) x[t] = h[t] - l[t];
      ev_ema_from( x, 0, p[0], e1 );
      e2[b2] = e1[b2];
      for( t = b2 + 1; t < EV_N; t++ ) e2[t] = kk * e1[t] + (1.0 - kk) * e2[t - 1];
      for( t = s; t < EV_N; t++ )
      {
         double sum = 0.0;
         for( i = t - p[1] + 1; i <= t; i++ ) sum += e2[i] == 0.0 ? 1.0 : e1[i] / e2[i];
         g_evO[0][t - s] = sum;
      }
      return s;
   }
   if( EV_IS("TSI") )
   {
      int b2 = s - ev_lbE( p[1] );
      double kk = 2.0 / (p[1] + 1.0);
      for( t = 1; t < EV_N; t++ ) { x[t] = c[t] - c[t - 1]; tmp[t] = fabs( x[t] ); }
      ev_ema_from( x, 1, p[0], e1 );
      ev_ema_from( tmp, 1, p[0], g1 );
      e2[b2] = e1[b2]; g2[b2] = g1[b2];
      for( t = b2 + 1; t < EV_N; t++ )
      {
         e2[t] = kk * e1[t] + (1.0 - kk) * e2[t - 1];
         g2[t] = kk * g1[t] + (1.0 - kk) * g2[t - 1];
      }
      for( t = s; t < EV_N; t++ ) g_evO[0][t - s] = g2[t] > 0.0 ? 100.0 * e2[t] / g2[t] : 0.0;
      return s;
   }
   if( EV_IS("SMI") )
   {
      int tp = p[0], b2 = s - ev_lbE( p[1] ) - ev_lbE( p[3] ), b3 = s - ev_lbE( p[3] );
      double kf = 2.0 / (p[1] + 1.0), ks = 2.0 / (p[3] + 1.0);
      for( t = tp - 1; t < EV_N; t++ )
      {
         double hh = h[t - tp + 1], ll = l[t - tp + 1];
         for( i = t - tp + 2; i <= t; i++ ) { if( h[i] > hh ) hh = h[i]; if( l[i] < ll ) ll = l[i]; }
         x[t] = c[t] - (hh + ll) * 0.5;
         tmp[t] = hh - ll;
      }
      ev_ema_from( x, tp - 1, p[2], e1 );
      ev_ema_from( tmp, tp - 1, p[2], g1 );
      e2[b2] = e1[b2]; g2[b2] = g1[b2];
      for( t = b2 + 1; t < EV_N; t++ )
      {
         e2[t] = kf * e1[t] + (1.0 - kf) * e2[t - 1];
         g2[t] = kf * g1[t] + (1.0 - kf) * g2[t - 1];
      }
      for( t = b3; t < EV_N; t++ ) { double hd = 0.5 * g2[t]; line[t] = hd > 0.0 ? 100.0 * e2[t] / hd : 0.0; }
      e3[b3] = line[b3];
      for( t = b3 + 1; t < EV_N; t++ ) e3[t] = ks * line[t] + (1.0 - ks) * e3[t - 1];
      for( t = s; t < EV_N; t++ ) { g_evO[0][t - s] = line[t]; g_evO[1][t - s] = e3[t]; }
      return s;
   }
   if( EV_IS("WAVETREND") )
   {
      int lbC = ev_lbE( p[0] ), b2 = s - L + lbC, b3 = b2 + lbC, sig = p[2];
      double kc = 2.0 / (p[0] + 1.0), ka = 2.0 / (p[1] + 1.0);
      for( t = 0; t < EV_N; t++ ) x[t] = (h[t] + l[t] + c[t]) / 3.0;
      ev_ema_from( x, 0, p[0], e1 );
      for( t = b2; t < EV_N; t++ )
      {
         double num = x[t] - e1[t];
         if( t >= 1 && x[t] == x[t - 1] && e1[t] == e1[t - 1] ) num = 0.0;
         tmp[t] = num; tmp2[t] = fabs( num );
      }
      e2[b2] = tmp2[b2];
      for( t = b2 + 1; t < EV_N; t++ ) e2[t] = kc * tmp2[t] + (1.0 - kc) * e2[t - 1];
      for( t = b3; t < EV_N; t++ ) { double sd = 0.015 * e2[t]; g1[t] = sd > 0.0 ? tmp[t] / sd : 0.0; }
      if( p[1] == 1 ) { for( t = b3; t < EV_N; t++ ) e3[t] = g1[t]; }
      else { e3[b3] = g1[b3]; for( t = b3 + 1; t < EV_N; t++ ) e3[t] = ka * g1[t] + (1.0 - ka) * e3[t - 1]; }
      for( t = s; t < EV_N; t++ )
      {
         double sum = 0.0;
         for( i = t - sig + 1; i <= t; i++ ) sum += e3[i];
         g_evO[0][t - s] = e3[t]; g_evO[1][t - s] = sum / sig;
      }
      return s;
   }
   if( EV_IS("DOSC") )
   {
      int r0 = s - L + TA_RSI_Lookback( p[0] ), b2 = r0 + ev_lbE( p[1] ), g = p[3];
      double k2 = 2.0 / (p[2] + 1.0);
      if( TA_RSI( r0, EV_N - 1, c, p[0], &beg, &nb, tmp ) != TA_SUCCESS || beg != r0 ) return -2;
      for( t = r0; t < EV_N; t++ ) x[t] = tmp[t - r0];
      ev_ema_from( x, r0, p[1], e1 );
      e2[b2] = e1[b2];
      for( t = b2 + 1; t < EV_N; t++ ) e2[t] = k2 * e1[t] + (1.0 - k2) * e2[t - 1];
      for( t = s; t < EV_N; t++ )
      {
         double sum = 0.0;
         for( i = t - g + 1; i <= t; i++ ) sum += e2[i];
         g_evO[0][t - s] = e2[t] - sum / g;
      }
      return s;
   }
   if( EV_IS("MACDV") )
   {
      int fast = p[0], slow = p[1], sig = p[2], lineIdx;
      double ks;
      if( slow < fast ) { int z = slow; slow = fast; fast = z; }
      lineIdx = s - ev_lbE( sig );
      ks = 2.0 / (sig + 1.0);
      ev_ema_from( c, 0, fast, e1 );
      ev_ema_from( c, 0, slow, e2 );
      if( TA_ATR( lineIdx, EV_N - 1, h, l, c, slow, &beg, &nb, tmp ) != TA_SUCCESS || beg != lineIdx ) return -2;
      for( t = lineIdx; t < EV_N; t++ )
      {
         double sp = e1[t] - e2[t], atr = tmp[t - lineIdx];
         double bs = fmax( fabs( e1[t] ), fabs( e2[t] ) ) * (slow + 1.0);
         line[t] = (!ev_zs( sp, bs ) && !ev_zs( atr, bs ) && atr >= DBL_MIN) ? sp / atr * 100.0 : 0.0;
      }
      e3[lineIdx] = line[lineIdx];
      for( t = lineIdx + 1; t < EV_N; t++ ) e3[t] = sig == 1 ? line[t] : ks * line[t] + (1.0 - ks) * e3[t - 1];
      for( t = s; t < EV_N; t++ )
      {
         g_evO[0][t - s] = line[t]; g_evO[1][t - s] = e3[t]; g_evO[2][t - s] = line[t] - e3[t];
      }
      return s;
   }
   if( EV_IS("PSO") )
   {
      int K = p[0], b2 = s - ev_lbE( p[1] );
      double ke = 2.0 / (p[1] + 1.0);
      for( t = K - 1; t < EV_N; t++ )
      {
         double hh = h[t - K + 1], ll = l[t - K + 1], fk;
         for( i = t - K + 2; i <= t; i++ ) { if( h[i] > hh ) hh = h[i]; if( l[i] < ll ) ll = l[i]; }
         fk = !ev_zs( hh - ll, fabs( hh ) + fabs( ll ) ) ? (c[t] - ll) / (hh - ll) * 100.0 : 50.0;
         x[t] = 0.1 * (fk - 50.0);
      }
      ev_ema_from( x, K - 1, p[1], e1 );
      e2[b2] = e1[b2];
      for( t = b2 + 1; t < EV_N; t++ ) e2[t] = ke * e1[t] + (1.0 - ke) * e2[t - 1];
      for( t = s; t < EV_N; t++ ) g_evO[0][t - s] = tanh( 0.5 * e2[t] );
      return s;
   }
   if( EV_IS("KC") )
   {
      for( t = 0; t < EV_N; t++ ) x[t] = (h[t] + l[t] + c[t]) / 3.0;
      ev_ema_from( x, 0, p[0], e1 );
      if( TA_ATR( s, EV_N - 1, h, l, c, p[1], &beg, &nb, tmp ) != TA_SUCCESS || beg != s ) return -2;
      for( t = s; t < EV_N; t++ )
      {
         double w = tmp[t - s] * k->dp;
         g_evO[0][t - s] = e1[t] + w; g_evO[1][t - s] = e1[t]; g_evO[2][t - s] = e1[t] - w;
      }
      return s;
   }
   if( EV_IS("KSTEXT") )
   {
      int lbS = TA_MA_Lookback( p[8], (TA_MAType)p[10] ), sigStart = s - lbS, leg;
      for( t = sigStart; t < EV_N; t++ ) line[t] = 0.0;
      for( leg = 0; leg < 4; leg++ )
      {
         int n = p[leg];
         for( t = n; t < EV_N; t++ ) tmp[t - n] = c[t - n] != 0.0 ? (c[t] / c[t - n] - 1.0) * 100.0 : 0.0;
         nb = ev_ma_model( p[9], p[4 + leg], tmp, EV_N - n, sigStart - n, tmp2, &beg );
         if( nb <= 0 || beg + n != sigStart ) return -3;
         for( t = sigStart; t < EV_N; t++ ) line[t] += (leg + 1.0) * tmp2[t - sigStart];
      }
      nb = ev_ma_model( p[10], p[8], line + sigStart, EV_N - sigStart, 0, tmp2, &beg );
      if( nb <= 0 || beg + sigStart != s ) return -3;
      for( t = s; t < EV_N; t++ ) { g_evO[0][t - s] = line[t]; g_evO[1][t - s] = tmp2[t - s]; }
      return s;
   }
   return -9;
}

/* The inputs server_verify expects: price components in flag order, then the
 * real arrays, in the function's input order. */
static ErrorNumber ev_server_verify( const TA_FuncInfo *fi, const double *opt, int start, const EvRun *r )
{
   const TA_Real *inputs[16];
   const TA_Real *outReal[EV_MAXOUT + 1];
   const TA_Integer *outInt[EV_MAXOUT + 1];
   unsigned int i;
   int nIn = 0, nR = 0, nI = 0, nReal = 0;

   for( i = 0; i < fi->nbInput; i++ )
   {
      const TA_InputParameterInfo *ii;
      TA_GetInputParameterInfo( fi->handle, i, &ii );
      if( ii->type == TA_Input_Price )
      {
         if( ii->flags & TA_IN_PRICE_OPEN )         inputs[nIn++] = g_evHist->open;
         if( ii->flags & TA_IN_PRICE_HIGH )         inputs[nIn++] = g_evHist->high;
         if( ii->flags & TA_IN_PRICE_LOW )          inputs[nIn++] = g_evHist->low;
         if( ii->flags & TA_IN_PRICE_CLOSE )        inputs[nIn++] = g_evHist->close;
         if( ii->flags & TA_IN_PRICE_VOLUME )       inputs[nIn++] = g_evHist->volume;
         if( ii->flags & TA_IN_PRICE_OPENINTEREST ) inputs[nIn++] = g_evHist->volume;
      }
      else
         inputs[nIn++] = ev_real_input( fi, nReal++ );
   }
   inputs[nIn] = NULL;
   for( i = 0; i < r->nbOut; i++ )
   {
      if( r->isInt[i] ) outInt[nI++] = r->integer[i];
      else              outReal[nR++] = r->real[i];
   }
   outReal[nR] = NULL;
   outInt[nI] = NULL;
   return server_verify( fi->name, start, EV_N - 1, EV_N, r->rc, r->beg, r->nb,
                         inputs, opt, (int)fi->nbOptInput, outReal, nI ? outInt : NULL );
}

static ErrorNumber test_ema_variation_goldens( void )
{
   static const int starts[2] = { 0, 150 };
   unsigned int c, i;
   int s, o, t;
   ErrorNumber e = TA_TEST_PASS;

   TA_SetEMAVariation( TA_EMAVariation_FIRST_BAR_SEED );
   for( c = 0; c < sizeof(evGold)/sizeof(evGold[0]) && e == TA_TEST_PASS; c++ )
   {
      const EvGold *k = &evGold[c];
      const TA_FuncHandle *h;
      const TA_FuncInfo *fi;
      double opt[EV_MAXOPT];
      int nInt = 0;

      if( TA_GetFuncHandle( k->name, &h ) != TA_SUCCESS || TA_GetFuncInfo( h, &fi ) != TA_SUCCESS )
      { e = TA_EMAV_GOLDEN; break; }
      for( i = 0; i < fi->nbOptInput; i++ )
      {
         const TA_OptInputParameterInfo *oi;
         TA_GetOptInputParameterInfo( fi->handle, i, &oi );
         opt[i] = (oi->type == TA_OptInput_RealRange || oi->type == TA_OptInput_RealList)
                  ? k->dp : (double)k->ip[nInt++];
      }
      for( s = 0; s < 2 && e == TA_TEST_PASS; s++ )
      {
         int ms;
         ev_call( fi, opt, starts[s], &g_evA );
         ms = ev_model( k, starts[s], g_evA.lookback );
         if( g_evA.rc != TA_SUCCESS || ms < 0 || ms != g_evA.beg || g_evA.nb != EV_N - ms )
         {
            printf( "\nEMA VARIATION GOLDEN [TA_%s] start %d: rc=%d (%d,%d), the rule's model "
                    "answered %d\n", k->name, starts[s], (int)g_evA.rc, g_evA.beg, g_evA.nb, ms );
            e = TA_EMAV_GOLDEN;
            break;
         }
         for( o = 0; o < (int)fi->nbOutput && o < 3; o++ )
         {
            double lo = g_evO[o][0], hi = lo, scale;
            for( t = 0; t < g_evA.nb; t++ )
            {
               if( g_evO[o][t] < lo ) lo = g_evO[o][t];
               if( g_evO[o][t] > hi ) hi = g_evO[o][t];
            }
            scale = hi - lo > 1.0 ? hi - lo : 1.0;
            for( t = 0; t < g_evA.nb; t++ )
            {
               if( !(fabs( g_evA.real[o][t] - g_evO[o][t] ) <= EV_GOLD_TOL * scale) )
               {
                  printf( "\nEMA VARIATION GOLDEN [TA_%s] start %d out %d bar %d: %.17g, the "
                          "seed rule gives %.17g (bound %.1e of %.4g)\n", k->name, starts[s], o,
                          g_evA.beg + t, g_evA.real[o][t], g_evO[o][t], EV_GOLD_TOL, scale );
                  e = TA_EMAV_GOLDEN;
                  break;
               }
               g_evGoldCmp++;
            }
            if( e != TA_TEST_PASS ) break;
         }
         if( e == TA_TEST_PASS && server_verify_active() )
            e = ev_server_verify( fi, opt, starts[s], &g_evA );
      }
   }
   TA_SetEMAVariation( TA_EMAVariation_SMA_SEED );
   return e;
}

ErrorNumber test_func_ema_variation( TA_History *history )
{
   ErrorNumber e;
   int i;

   if( (int)history->nbBars < EV_N )
      return TA_TESTUTIL_TFRR_BAD_PARAM;
   g_evHist = history;
   for( i = 0; i < EV_N; i++ )
      g_evPeriods[i] = (double)(2 + (i * 7) % 29);
   g_evFuncs = 0;
   g_evScopeCells = g_evScopeStill = g_evShapeCalls = g_evOffDefault = 0;
   g_evIndepCmp = g_evIndepDefaultMoved = g_evChainMoved = g_evIdentCmp = g_evGoldCmp = 0;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   TA_SetEMAVariation( TA_EMAVariation_SMA_SEED );

   e = test_ema_variation_scope();
   if( e == TA_TEST_PASS ) e = test_ema_variation_independence();
   if( e == TA_TEST_PASS ) e = test_ema_variation_macd_identity( history );
   if( e == TA_TEST_PASS ) e = test_ema_variation_goldens();
   TA_SetEMAVariation( TA_EMAVariation_SMA_SEED );
   if( e != TA_TEST_PASS )
      return e;

   if( g_evIndepCmp == 0 || g_evChainMoved == 0 || g_evIdentCmp == 0 || g_evGoldCmp == 0 )
   {
      printf( "\nEMA VARIATION VACUOUS: %ld independence, %ld chained, %ld identity, "
              "%ld golden comparison(s)\n", g_evIndepCmp, g_evChainMoved, g_evIdentCmp, g_evGoldCmp );
      return TA_EMAV_VACUOUS;
   }
   return TA_TEST_PASS;
}
