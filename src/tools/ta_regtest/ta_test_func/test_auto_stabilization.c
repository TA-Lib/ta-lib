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
 *  100526 MF,CC  First version (#492).
 */

/* Description:
 *
 *   The Auto levels of the unstable period (TA_UNSTABLE_AUTO_PREC_4 / _8).
 *
 *   SERVER_VERIFY: in-process only. The count is the lookback, which the
 *   cross-language lookback legs compare for every server.
 *
 *   Two legs, over every function through the abstraction layer, and rL8:
 *   under a level a lookback at a period of 1 stays 0.
 *
 *   1. The count. With one id on a level, its owner's lookback minus its
 *      lookback at 0 equals the rule, computed here from this file's own copy
 *      of the rule table. This is what holds each rule exactly: leg 2 would
 *      pass a linear kernel's rule a third too short. The owner then reports
 *      the bits it reports at 0, starting that many bars later.
 *
 *   2. Two starts. The same call on a series and on that series with its first
 *      bars removed, compared at every bar both report:
 *        - a window (Auto adds nothing to its lookback): within
 *          T = max(1e-10, 1e-7 * range), integers equal, an index output
 *          rebased by the bars removed;
 *        - a converging call: with S the largest difference at a setting of 0,
 *          within e^-(K-3) * S on the level whose e-folds are K, and no
 *          tighter than T at PREC_4 or the committed floor at PREC_8;
 *        - a path-dependent function is not compared, and fails if it meets
 *          the converging criterion everywhere: its flag is then wrong.
 *
 *   The corpus is three synthetic series, built here with integer arithmetic
 *   only so that every platform sees the same bars. Prices are whole cents
 *   with bar ranges small against the price: the candle functions that keep a
 *   running average sum exactly on such bars, which is what lets their
 *   integer outputs be compared for equality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_utility.h"
#include "ta_memory.h"

#define AW_N          8192
#define AW_NB_SERIES  3
#define AW_SERIES_WALK   0
#define AW_SERIES_TREND  1
#define AW_SERIES_RANGE  2
#define AW_NB_START   6
#define AW_MAX_OUT    4
#define AW_MAX_OPT    12
#define AW_MAX_VEC    96
#define AW_MIN_BARS   1024
#define AW_MAX_REPORT 40

static const int awStart[AW_NB_START] = { 1, 2, 5, 33, 250, 1000 };
static const char *awSeriesName[AW_NB_SERIES] = { "walk", "trend", "range" };

typedef struct
{
   double open[AW_N], high[AW_N], low[AW_N], close[AW_N], volume[AW_N], oi[AW_N];
} AwSeries;

typedef struct
{
   double real[AW_MAX_OUT][AW_N];
   int    integer[AW_MAX_OUT][AW_N];
   int    beg, nb, lookback;
   TA_RetCode retCode;
} AwRun;

static AwSeries *awSeries;      /* [AW_NB_SERIES] */
static AwRun *awFull, *awLate;

static int awNbReport;
static unsigned int awNbWindow, awNbConverging, awNbPathDep, awNbCount, awNbCountNonZero, awNbFloorHeld;
static unsigned int awNbSameValues, awNbPeriod1, awNbOffset, awNbMonotone;

/* --- corpus ---------------------------------------------------------- */

static unsigned int awRng;
static int awDraw( int span )   /* uniform in [-span, span] */
{
   awRng = awRng * 1664525u + 1013904223u;
   return (int)( (awRng >> 8) % (unsigned int)(2*span + 1) ) - span;
}

static void awBuildSeries( void )
{
   int s, i, cents, prev, body, vol;

   for( s=0; s < AW_NB_SERIES; s++ )
   {
      AwSeries *x = &awSeries[s];
      awRng = 12345u + 977u * (unsigned int)s;
      cents = 50000;
      vol = 500000;
      for( i=0; i < AW_N; i++ )
      {
         prev = cents;
         if( s == AW_SERIES_WALK )
            cents += awDraw( 150 );
         else if( s == AW_SERIES_TREND )
            cents += ( ((i / 1024) & 1) ? -40 : 40 ) + awDraw( 60 );
         else
            cents += ( (i & 1) ? -120 : 120 ) + ( (i % 16) == 0 ? 3 : 0 ) + awDraw( 4 );
         if( cents < 20000 ) cents = 20000 + (20000 - cents);
         if( cents > 90000 ) cents = 90000 - (cents - 90000);

         body = cents > prev ? cents - prev : prev - cents;
         x->close[i] = (double)cents / 100.0;
         x->open[i]  = (double)prev / 100.0;
         x->high[i]  = (double)( (cents > prev ? cents : prev) + 5 + (awDraw(20)+20) + body/4 ) / 100.0;
         x->low[i]   = (double)( (cents < prev ? cents : prev) - 5 - (awDraw(20)+20) - body/4 ) / 100.0;

         vol += awDraw( 9000 );
         if( vol < 100000 ) vol = 100000 + (100000 - vol);
         x->volume[i] = (double)vol;
         x->oi[i]     = (double)( 2000 + (i % 97) );
      }
   }
}

/* --- this file's copy of the rule table ------------------------------- */

static int awCeilDiv( int a, int b ) { return (a + b - 1) / b; }

static int awISqrt( int n )
{
   int r = 0;
   while( (r+1)*(r+1) <= n ) r++;
   return r;
}

/* The Auto count of `name` for integer parameters p0, p1 and real parameters
 * r0, r1, at e-folds K and digits X. -1: the function owns no rule.
 */
static int awRule( const char *name, int K, int X, int p0, int p1, double r0, double r1 )
{
   int hilbert = 80 + 50*X;
   double big;
   long long v;

   if( !strcmp(name,"EMA") ) return p0 > 1 ? awCeilDiv( K*p0, 2 ) : 0;
   /* The DM and DI functions answer a period of 1 without their id. */
   if( !strcmp(name,"RMA") || !strcmp(name,"ATR") ||
       !strcmp(name,"PLUS_DM") || !strcmp(name,"MINUS_DM") )
      return p0 > 1 ? awCeilDiv( K*(2*p0-1), 2 ) : 0;
   if( !strcmp(name,"NATR") || !strcmp(name,"RSI") || !strcmp(name,"CMO") ||
       !strcmp(name,"DX") || !strcmp(name,"RVI") ||
       !strcmp(name,"PLUS_DI") || !strcmp(name,"MINUS_DI") )
      return p0 > 1 ? K*p0 : 0;
   if( !strcmp(name,"ADX") ) return (K+6)*p0;
   if( !strcmp(name,"T3") ) return p0 > 1 ? awCeilDiv( 11*(X+4)*p0, 8 ) : 0;
   if( !strcmp(name,"HA") ) return awCeilDiv( 13*K, 9 );
   if( !strcmp(name,"SWAK_HP") ) return awCeilDiv( K*p0, 6 );
   if( !strcmp(name,"SWAK_GAUSS") || !strcmp(name,"SWAK_BUTTER") || !strcmp(name,"SWAK_2PHP") )
      return awCeilDiv( (K+3)*(p0+2), 9 );
   if( !strcmp(name,"SWAK_BP") ) return (int)ceil( (double)((K+1)*p0) / (6.0*r0) );
   if( !strcmp(name,"KAMA") ) return p0 > 1 ? 25*X*awISqrt(p0) : 0;
   if( !strcmp(name,"FRAMA") )
      return 9*(X+4)*(awISqrt(p0)+2)/2 < 99*K ? 9*(X+4)*(awISqrt(p0)+2)/2 : 99*K;
   if( !strcmp(name,"VIDYA") )
   {
      if( p0 <= 1 ) return 0;
      v = 2LL*X*(p0+1)*awISqrt(p1);
      return v > TA_INDEX_MAX ? TA_INDEX_MAX : (int)v;
   }
   if( !strcmp(name,"MCGD") ) return 5*X*p0;
   if( !strcmp(name,"HT_TRENDLINE") || !strcmp(name,"HT_TRENDMODE") ) return 120 + 20*X;
   if( !strncmp(name,"HT_",3) ) return hilbert;
   if( !strcmp(name,"MAMA") )
   {
      big = r0 > r1 ? r0 : r1;
      return hilbert + (int)ceil( (double)(2*K) / big );
   }
   if( !strcmp(name,"FISHER") ) return awCeilDiv( 5*(K+6), 2 );
   /* STC: p0 = fast, p1 = slow; on top of the EMA count it inherits. */
   if( !strcmp(name,"STC") ) return awCeilDiv( 5*K, 2 ) + 3*( (p0 > p1 ? p0 : p1) + 1 );
   return -1;
}

/* --- calling a function ----------------------------------------------- */

typedef struct
{
   int    nbOpt;
   double val[AW_MAX_OPT];
   int    hasMaType[3];   /* [0] KAMA, [1] VIDYA, [2] MAMA, on any MA-type slot */
   const char *label;
} AwVector;

static char awMsg[256];

static void awFail( const char *name, const char *what )
{
   if( awNbReport < AW_MAX_REPORT )
      printf( "\n  auto-stabilization: %s %s: %s", name, what, awMsg );
   awNbReport++;
}

/* Run `funcInfo` with `vec` on series `s` with its first `skip` bars removed.
 * Outputs land at their bar index within the removed series.
 */
static void awCall( const TA_FuncInfo *funcInfo, const AwVector *vec, int s, int skip, AwRun *run )
{
   TA_ParamHolder *h;
   const TA_InputParameterInfo *in;
   const TA_OptInputParameterInfo *opt;
   const TA_OutputParameterInfo *out;
   const AwSeries *x = &awSeries[s];
   unsigned int i, nbReal = 0;

   run->beg = run->nb = 0;
   run->lookback = -1;
   run->retCode = TA_ParamHolderAlloc( funcInfo->handle, &h );
   if( run->retCode != TA_SUCCESS ) return;

   for( i=0; i < funcInfo->nbInput && run->retCode == TA_SUCCESS; i++ )
   {
      TA_GetInputParameterInfo( funcInfo->handle, i, &in );
      if( in->type == TA_Input_Price )
         run->retCode = TA_SetInputParamPricePtr( h, i, x->open+skip, x->high+skip, x->low+skip,
                                                  x->close+skip, x->volume+skip, x->oi+skip );
      else if( in->type == TA_Input_Real )
      {
         run->retCode = TA_SetInputParamRealPtr( h, i, (nbReal == 0 ? x->close : x->open) + skip );
         nbReal++;
      }
      else
         run->retCode = TA_BAD_PARAM;   /* an integer input: not swept here */
   }
   for( i=0; i < funcInfo->nbOptInput && run->retCode == TA_SUCCESS; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( opt->type == TA_OptInput_RealRange || opt->type == TA_OptInput_RealList )
         run->retCode = TA_SetOptInputParamReal( h, i, vec->val[i] );
      else
         run->retCode = TA_SetOptInputParamInteger( h, i, (TA_Integer)vec->val[i] );
   }
   for( i=0; i < funcInfo->nbOutput && run->retCode == TA_SUCCESS; i++ )
   {
      TA_GetOutputParameterInfo( funcInfo->handle, i, &out );
      if( out->type == TA_Output_Real )
         run->retCode = TA_SetOutputParamRealPtr( h, i, run->real[i] );
      else
         run->retCode = TA_SetOutputParamIntegerPtr( h, i, run->integer[i] );
   }
   if( run->retCode == TA_SUCCESS )
      run->retCode = TA_GetLookback( h, &run->lookback );
   if( run->retCode == TA_SUCCESS )
      run->retCode = TA_CallFunc( h, 0, AW_N-1-skip, &run->beg, &run->nb );
   TA_ParamHolderFree( h );
}

static int awLookback( const TA_FuncInfo *funcInfo, const AwVector *vec )
{
   TA_ParamHolder *h;
   const TA_OptInputParameterInfo *opt;
   TA_Integer lookback = -1;
   unsigned int i;

   if( TA_ParamHolderAlloc( funcInfo->handle, &h ) != TA_SUCCESS ) return -1;
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( opt->type == TA_OptInput_RealRange || opt->type == TA_OptInput_RealList )
         TA_SetOptInputParamReal( h, i, vec->val[i] );
      else
         TA_SetOptInputParamInteger( h, i, (TA_Integer)vec->val[i] );
   }
   if( TA_GetLookback( h, &lookback ) != TA_SUCCESS ) lookback = -1;
   TA_ParamHolderFree( h );
   return lookback;
}

/* --- vectors ----------------------------------------------------------- */

static int awIsMaType( const TA_OptInputParameterInfo *opt )
{
   return opt->type == TA_OptInput_IntegerList && strstr( opt->paramName, "MAType" ) != NULL;
}

static void awNoteMaType( AwVector *v, const TA_FuncInfo *funcInfo )
{
   const TA_OptInputParameterInfo *opt;
   unsigned int i;
   v->hasMaType[0] = v->hasMaType[1] = v->hasMaType[2] = 0;
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( !awIsMaType( opt ) ) continue;
      if( (int)v->val[i] == TA_MAType_KAMA )  v->hasMaType[0] = 1;
      if( (int)v->val[i] == TA_MAType_VIDYA ) v->hasMaType[1] = 1;
      if( (int)v->val[i] == TA_MAType_MAMA )  v->hasMaType[2] = 1;
   }
}

static int awBuildVectors( const TA_FuncInfo *funcInfo, AwVector *vec )
{
   static const double mamaLimits[4][2] = { {0.01,0.01}, {0.99,0.99}, {0.99,0.01}, {0.01,0.99} };
   static const double bpDelta[2] = { 0.05, 0.5 };
   const TA_OptInputParameterInfo *opt;
   AwVector def;
   int n = 0, tripled;
   unsigned int i, j;

   if( funcInfo->nbOptInput > AW_MAX_OPT ) return 0;
   memset( &def, 0, sizeof(def) );
   def.nbOpt = (int)funcInfo->nbOptInput;
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      def.val[i] = opt->defaultValue;
   }

   vec[n] = def; vec[n].label = "defaults"; n++;

   vec[n] = def; vec[n].label = "integer minima";
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( opt->type == TA_OptInput_IntegerRange )
         vec[n].val[i] = (double)((const TA_IntegerRange *)opt->dataSet)->min;
   }
   n++;

   vec[n] = def; vec[n].label = "periods tripled";
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( opt->type == TA_OptInput_IntegerRange )
      {
         tripled = 3 * (int)opt->defaultValue;
         if( tripled > ((const TA_IntegerRange *)opt->dataSet)->max )
            tripled = ((const TA_IntegerRange *)opt->dataSet)->max;
         vec[n].val[i] = (double)tripled;
      }
   }
   n++;

   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      const TA_IntegerList *list;
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( !awIsMaType( opt ) ) continue;
      list = (const TA_IntegerList *)opt->dataSet;
      for( j=0; j < list->nbElement && n < AW_MAX_VEC; j++ )
      {
         vec[n] = def; vec[n].label = "MA type";
         vec[n].val[i] = (double)list->data[j].value;
         n++;
      }
   }

   if( !strcmp( funcInfo->name, "SWAK_BP" ) )
      for( j=0; j < 2; j++ )
      {
         vec[n] = def; vec[n].label = "delta bound"; vec[n].val[1] = bpDelta[j]; n++;
      }
   if( !strcmp( funcInfo->name, "MAMA" ) )
      for( j=0; j < 4; j++ )
      {
         vec[n] = def; vec[n].label = "limits";
         vec[n].val[0] = mamaLimits[j][0]; vec[n].val[1] = mamaLimits[j][1]; n++;
      }

   if( !strcmp( funcInfo->name, "ADOSC" ) )
   {
      vec[n] = def; vec[n].label = "fast above slow"; vec[n].val[0] = 20.0; vec[n].val[1] = 5.0; n++;
   }
   if( !strcmp( funcInfo->name, "FRAMA" ) )
   {
      vec[n] = def; vec[n].label = "capped count"; vec[n].val[0] = 1024.0; n++;
   }

   for( j=0; j < (unsigned int)n; j++ )
      awNoteMaType( &vec[j], funcInfo );
   return n;
}

/* --- leg 1: the count -------------------------------------------------- */

static const struct { unsigned int level; int K, X; } awLevel[2] = {
   { TA_UNSTABLE_AUTO_PREC_4, 10, 4 },
   { TA_UNSTABLE_AUTO_PREC_8, 19, 8 }
};

static TA_FuncUnstId awOwnId( const char *name )
{
   static const struct { const char *name; TA_FuncUnstId id; } own[] = {
      { "ADX", TA_FUNC_UNST_ADX },
      { "ATR", TA_FUNC_UNST_ATR },
      { "CMO", TA_FUNC_UNST_CMO },
      { "DX", TA_FUNC_UNST_DX },
      { "EMA", TA_FUNC_UNST_EMA },
      { "HT_DCPERIOD", TA_FUNC_UNST_HT_DCPERIOD },
      { "HT_DCPHASE", TA_FUNC_UNST_HT_DCPHASE },
      { "HT_PHASOR", TA_FUNC_UNST_HT_PHASOR },
      { "HT_SINE", TA_FUNC_UNST_HT_SINE },
      { "HT_TRENDLINE", TA_FUNC_UNST_HT_TRENDLINE },
      { "HT_TRENDMODE", TA_FUNC_UNST_HT_TRENDMODE },
      { "KAMA", TA_FUNC_UNST_KAMA },
      { "MAMA", TA_FUNC_UNST_MAMA },
      { "MINUS_DI", TA_FUNC_UNST_MINUS_DI },
      { "MINUS_DM", TA_FUNC_UNST_MINUS_DM },
      { "NATR", TA_FUNC_UNST_NATR },
      { "PLUS_DI", TA_FUNC_UNST_PLUS_DI },
      { "PLUS_DM", TA_FUNC_UNST_PLUS_DM },
      { "RSI", TA_FUNC_UNST_RSI },
      { "T3", TA_FUNC_UNST_T3 },
      { "RMA", TA_FUNC_UNST_RMA },
      { "FRAMA", TA_FUNC_UNST_FRAMA },
      { "MCGD", TA_FUNC_UNST_MCGD },
      { "HA", TA_FUNC_UNST_HA },
      { "VIDYA", TA_FUNC_UNST_VIDYA },
      { "RVI", TA_FUNC_UNST_RVI },
      { "STC", TA_FUNC_UNST_STC },
      { "SWAK_GAUSS", TA_FUNC_UNST_SWAK_GAUSS },
      { "SWAK_BUTTER", TA_FUNC_UNST_SWAK_BUTTER },
      { "SWAK_HP", TA_FUNC_UNST_SWAK_HP },
      { "SWAK_2PHP", TA_FUNC_UNST_SWAK_2PHP },
      { "FISHER", TA_FUNC_UNST_FISHER },
      { "SWAK_BP", TA_FUNC_UNST_SWAK_BP },
   };
   unsigned int i;
   for( i=0; i < sizeof(own)/sizeof(own[0]); i++ )
      if( !strcmp( own[i].name, name ) ) return own[i].id;
   return TA_FUNC_UNST_ALL;
}

/* A function that adjusts the count it inherits: `offset` more bars under a
 * level of `source`, and none under a count.
 */
static void awCheckOffset( const TA_FuncInfo *funcInfo, const AwVector *vec )
{
   const TA_OptInputParameterInfo *opt;
   TA_FuncUnstId source;
   int l, at0, got, base, offset, p0 = 0, p1 = 0, nbInt = 0;
   unsigned int i;

   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( opt->type != TA_OptInput_IntegerRange ) continue;
      if( nbInt == 0 ) p0 = (int)vec->val[i]; else if( nbInt == 1 ) p1 = (int)vec->val[i];
      nbInt++;
   }
   if( !strcmp( funcInfo->name, "CKSP" ) ) source = TA_FUNC_UNST_ATR;
   else if( !strcmp( funcInfo->name, "ADOSC" ) ) source = TA_FUNC_UNST_EMA;
   else return;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   at0 = awLookback( funcInfo, vec );
   if( at0 < 0 ) return;
   TA_SetUnstablePeriod( source, 30 );
   got = awLookback( funcInfo, vec ) - at0;
   TA_SetUnstablePeriod( source, 0 );
   if( got != 30 )
   {
      sprintf( awMsg, "a count of 30 on the id it inherits adds %d bars", got );
      awFail( funcInfo->name, vec->label );
   }
   for( l=0; l < 2; l++ )
   {
      if( source == TA_FUNC_UNST_ATR )
      {
         base   = awRule( "ATR", awLevel[l].K, awLevel[l].X, p0, 0, 0.0, 0.0 );
         offset = p1 - 1;
      }
      else
      {
         int slowest = p0 > p1 ? p0 : p1, fastest = p0 > p1 ? p1 : p0, j, sum = 0;
         for( j=1; j <= 16; j++ )
            sum += (slowest >> j) < fastest ? (slowest >> j) : fastest;
         base   = awRule( "EMA", awLevel[l].K, awLevel[l].X, slowest, 0, 0.0, 0.0 );
         offset = awCeilDiv( 15*fastest + 3*sum, 8 );
      }
      TA_SetUnstablePeriod( source, awLevel[l].level );
      got = awLookback( funcInfo, vec ) - at0;
      TA_SetUnstablePeriod( source, 0 );
      if( got != base + offset )
      {
         sprintf( awMsg, "PREC_%d adds %d bars, its source's %d and its own %d say %d",
                  awLevel[l].X, got, base, offset, base + offset );
         awFail( funcInfo->name, vec->label );
      }
      if( offset > 0 ) awNbOffset++;
   }
}

/* MACD, STC, MAVP and the like index a shorter-period leg on a count that
 * does not fall as a period grows. ADOSC's is the one written in two periods.
 */
static void awAdoscMonotone( void )
{
   int l, f, s, prev, lb;

   for( l=0; l < 2; l++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, awLevel[l].level );
      for( s=2; s <= 100000; s = s < 300 ? s+1 : s + s/7 )
      {
         prev = 0;
         for( f=2; f <= 100000; f = f < 300 ? f+1 : f + f/7 )
         {
            lb = TA_ADOSC_Lookback( f, s );
            if( lb < prev )
            {
               sprintf( awMsg, "PREC_%d lookback falls to %d at fast %d, slow %d", awLevel[l].X, lb, f, s );
               awFail( "ADOSC", "" );
            }
            if( lb != TA_ADOSC_Lookback( s, f ) )
            {
               sprintf( awMsg, "lookback differs with the two periods swapped at %d, %d", f, s );
               awFail( "ADOSC", "" );
            }
            prev = lb;
            awNbMonotone++;
         }
      }
   }
   TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 0 );
}

/* No level takes bars away: with every id on a level a lookback is at least
 * what it is at 0, and PREC_8 at least PREC_4.
 */
static void awCheckLevels( const TA_FuncInfo *funcInfo, const AwVector *vec )
{
   int at0, at4, at8;

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   at0 = awLookback( funcInfo, vec );
   if( at0 < 0 ) return;
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, TA_UNSTABLE_AUTO_PREC_4 );
   at4 = awLookback( funcInfo, vec );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, TA_UNSTABLE_AUTO_PREC_8 );
   at8 = awLookback( funcInfo, vec );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   if( at4 < at0 || at8 < at4 )
   {
      sprintf( awMsg, "lookback is %d at 0, %d under PREC_4 and %d under PREC_8", at0, at4, at8 );
      awFail( funcInfo->name, vec->label );
   }
}

static void awCheckCount( const TA_FuncInfo *funcInfo, const AwVector *vec, TA_FuncUnstId own )
{
   int l, at0, atLevel, want, p0 = 0, p1 = 0, nbInt = 0, nbReal = 0;
   double r0 = 0.0, r1 = 0.0;
   const TA_OptInputParameterInfo *opt;
   unsigned int i;

   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( opt->type == TA_OptInput_IntegerRange )
      {
         if( nbInt == 0 ) p0 = (int)vec->val[i]; else if( nbInt == 1 ) p1 = (int)vec->val[i];
         nbInt++;
      }
      else if( opt->type == TA_OptInput_RealRange )
      {
         if( nbReal == 0 ) r0 = vec->val[i]; else if( nbReal == 1 ) r1 = vec->val[i];
         nbReal++;
      }
   }

   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   at0 = awLookback( funcInfo, vec );
   if( at0 < 0 ) return;
   for( l=0; l < 2; l++ )
   {
      want = awRule( funcInfo->name, awLevel[l].K, awLevel[l].X, p0, p1, r0, r1 );
      if( want < 0 )
      {
         sprintf( awMsg, "owns an unstable id but this file has no rule for it" );
         awFail( funcInfo->name, vec->label );
         return;
      }
      TA_SetUnstablePeriod( own, awLevel[l].level );
      atLevel = awLookback( funcInfo, vec );
      TA_SetUnstablePeriod( own, 0 );
      if( TA_GetUnstablePeriod( own ) != 0 ) atLevel = -1;
      if( atLevel - at0 != want )
      {
         sprintf( awMsg, "PREC_%d count is %d, the rule says %d", awLevel[l].X, atLevel - at0, want );
         awFail( funcInfo->name, vec->label );
      }
      awNbCount++;
      if( want > 0 ) awNbCountNonZero++;

      /* A level changes when its owner starts reporting, never what it reports:
       * the same bits as at 0, minus the bars the count discards.
       */
      if( atLevel - at0 == want && at0 + want < AW_N - AW_MIN_BARS )
      {
         const TA_OutputParameterInfo *out;
         int same = 1;
         awCall( funcInfo, vec, AW_SERIES_WALK, 0, awFull );
         TA_SetUnstablePeriod( own, awLevel[l].level );
         awCall( funcInfo, vec, AW_SERIES_WALK, 0, awLate );
         TA_SetUnstablePeriod( own, 0 );
         if( awFull->retCode != TA_SUCCESS || awLate->retCode != TA_SUCCESS ||
             awLate->beg != awFull->beg + want || awLate->nb != awFull->nb - want || awLate->nb <= 0 )
            same = 0;
         for( i=0; same && i < funcInfo->nbOutput; i++ )
         {
            TA_GetOutputParameterInfo( funcInfo->handle, i, &out );
            if( out->type == TA_Output_Real )
               same = memcmp( awFull->real[i] + want, awLate->real[i], sizeof(double) * (size_t)awLate->nb ) == 0;
            else
               same = memcmp( awFull->integer[i] + want, awLate->integer[i], sizeof(int) * (size_t)awLate->nb ) == 0;
         }
         if( !same )
         {
            sprintf( awMsg, "PREC_%d moves a value its owner reports from bar 0, or the range it reports", awLevel[l].X );
            awFail( funcInfo->name, vec->label );
         }
         awNbSameValues++;
      }
   }
}

/* --- leg 2: two starts -------------------------------------------------- */

/* Compares every bar both runs report (the last quarter of them when
 * `tailOnly`). perOut[o] is the largest |full - late| of output o and range[o]
 * the range of `full` there; *firstBad is the first bar where an output passes
 * its limitOf[o], -1 if none or if limitOf is NULL. Two NaN agree. Returns the
 * number of bars compared.
 */
static int awCompare( const TA_FuncInfo *funcInfo, const AwRun *full, const AwRun *late, int skip,
                      int tailOnly, const double limitOf[AW_MAX_OUT],
                      double perOut[AW_MAX_OUT], double range[AW_MAX_OUT], int *firstBad )
{
   const TA_OutputParameterInfo *out;
   unsigned int o;
   int bar, first, last, isIndex;
   double a, b, d, lo, hi;

   /* An index counts from the first bar handed in. */
   isIndex = !strcmp(funcInfo->name,"MAXINDEX") || !strcmp(funcInfo->name,"MININDEX") ||
             !strcmp(funcInfo->name,"MINMAXINDEX");
   *firstBad = -1;
   first = late->beg + skip;
   if( full->beg > first ) first = full->beg;
   last = full->beg + full->nb - 1;
   if( late->beg + skip + late->nb - 1 < last ) last = late->beg + skip + late->nb - 1;
   if( last < first ) return 0;
   if( tailOnly ) first = last - (last - first)/4;

   for( o=0; o < funcInfo->nbOutput; o++ )
   {
      TA_GetOutputParameterInfo( funcInfo->handle, o, &out );
      lo = hi = 0.0;
      perOut[o] = 0.0;
      for( bar=first; bar <= last; bar++ )
      {
         if( out->type == TA_Output_Real )
         {
            a = full->real[o][bar - full->beg];
            b = late->real[o][bar - skip - late->beg];
         }
         else
         {
            a = (double)full->integer[o][bar - full->beg];
            b = (double)late->integer[o][bar - skip - late->beg];
            if( isIndex ) b += skip;
         }
         if( bar == first || a < lo ) lo = a;
         if( bar == first || a > hi ) hi = a;
         d = ( a != a && b != b ) ? 0.0 : fabs( a - b );
         if( !(d <= perOut[o]) ) perOut[o] = d;   /* a NaN on one side lands here */
         if( limitOf && !(d <= limitOf[o]) && ( *firstBad < 0 || bar < *firstBad ) ) *firstBad = bar;
      }
      range[o] = hi - lo;
   }
   return last - first + 1;
}

static double awMax( const TA_FuncInfo *funcInfo, const double perOut[AW_MAX_OUT] )
{
   double m = 0.0;
   unsigned int o;
   for( o=0; o < funcInfo->nbOutput; o++ )
      if( !(perOut[o] <= m) ) m = perOut[o];
   return m;
}

/* --- the PREC_8 floors -------------------------------------------------- */

/* At PREC_8 the threshold e^-16 * S is the size of the window tolerance T, so
 * with T as floor that pass would show nothing. Its floor is instead a
 * committed number per function and output: 4 times the largest two-start
 * difference over the last quarter of each series at a setting of 0, where
 * only rounding is left. Measured once (TA_AUTO_STABILIZATION_FLOORS=1 prints the
 * table), never in the run it gates: a floor measured in the run would follow
 * a regression. An output absent from the table has a floor of 0.
 */
typedef struct { const char *name; int output; double floor; } AwFloor;
static const AwFloor awFloor[] = {
#include "test_auto_stabilization_floors.h"
   { NULL, 0, 0.0 }
};

static double awFloorOf( const char *name, int output )
{
   const AwFloor *f;
   for( f=awFloor; f->name; f++ )
      if( f->output == output && !strcmp( f->name, name ) ) return f->floor;
   return 0.0;
}

static int awPrintFloors;
static double awTail[AW_MAX_OUT];   /* this function's measured tails, when printing */

static int awSkipSeries( const TA_FuncInfo *funcInfo, const AwVector *vec, int s )
{
   const char *n = funcInfo->name;
   /* Sized for a market that trends or wanders: design, ruling D3. */
   if( s == AW_SERIES_RANGE &&
       ( !strcmp(n,"KAMA") || !strcmp(n,"FRAMA") || !strcmp(n,"VIDYA") ||
         vec->hasMaType[0] || vec->hasMaType[1] ) )
      return 1;
   return 0;
}

static int awSkipCall( const TA_FuncInfo *funcInfo, const AwVector *vec, int autoCount )
{
   const char *n = funcInfo->name;
   /* FastK rests at 0 or 100: MAMA does not converge there, VIDYA's coefficient is 0. */
   if( !strcmp(n,"STOCHRSI") && ( vec->hasMaType[1] || vec->hasMaType[2] ) ) return 1;
   /* Its streak restarts a difference at the first reversal, whatever the warm-up. */
   if( !strcmp(n,"CRSI") && autoCount > 0 ) return 1;
   return 0;
}

static void awTwoStarts( const TA_FuncInfo *funcInfo, const AwVector *vec )
{
   int s, k, l, nb, nbAt0, bad, autoCount, lb0, lbAuto, pathDep, met, sawSeed;
   double diff, seed, T, perOut[AW_MAX_OUT], range[AW_MAX_OUT], limitOf[AW_MAX_OUT];
   unsigned int o;
   char what[96];

   pathDep = (funcInfo->flags & TA_FUNC_FLG_PATH_DEP) != 0;
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   lb0 = awLookback( funcInfo, vec );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, TA_UNSTABLE_AUTO_PREC_4 );
   lbAuto = awLookback( funcInfo, vec );
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
   if( lb0 < 0 || lbAuto < 0 )
   {
      sprintf( awMsg, "no lookback (%d at 0, %d under PREC_4)", lb0, lbAuto );
      awFail( funcInfo->name, vec->label );
      return;
   }
   autoCount = lbAuto - lb0;
   if( awSkipCall( funcInfo, vec, autoCount ) ) return;

   met = 1; sawSeed = 0;
   for( s=0; s < AW_NB_SERIES; s++ )
   {
      if( awSkipSeries( funcInfo, vec, s ) ) continue;

      /* Setting 0: the seed difference S and the window tolerance T. */
      awCall( funcInfo, vec, s, 0, awFull );
      if( awFull->retCode != TA_SUCCESS )
      {
         sprintf( awMsg, "the call at a setting of 0 answered %d", (int)awFull->retCode );
         awFail( funcInfo->name, vec->label );
         return;
      }
      seed = 0.0; T = 1e-10; nbAt0 = 0;
      for( k=0; k < AW_NB_START; k++ )
      {
         awCall( funcInfo, vec, s, awStart[k], awLate );
         nb = awCompare( funcInfo, awFull, awLate, awStart[k], 0, NULL, perOut, range, &bad );
         if( awLate->retCode != TA_SUCCESS ) nb = 0;
         if( k == 0 || nb < nbAt0 ) nbAt0 = nb;
         if( nb == 0 ) continue;
         diff = awMax( funcInfo, perOut );
         if( !(diff <= seed) ) seed = diff;
         for( o=0; o < funcInfo->nbOutput; o++ )
            if( 1e-7 * range[o] > T ) T = 1e-7 * range[o];
         if( awPrintFloors && !pathDep && autoCount > 0 )
         {
            awCompare( funcInfo, awFull, awLate, awStart[k], 1, NULL, perOut, range, &bad );
            for( o=0; o < funcInfo->nbOutput; o++ )
               if( !(perOut[o] <= awTail[o]) ) awTail[o] = perOut[o];
         }
      }

      if( !pathDep && autoCount == 0 )
      {
         if( nbAt0 < AW_MIN_BARS )
         {
            sprintf( what, "(%s, %s) window", vec->label, awSeriesName[s] );
            sprintf( awMsg, "only %d bars compared, floor %d", nbAt0, AW_MIN_BARS );
            awFail( funcInfo->name, what );
         }
         if( !(seed <= T) )
         {
            sprintf( what, "(%s, %s) window", vec->label, awSeriesName[s] );
            sprintf( awMsg, "two starts differ by %g, tolerance %g", seed, T );
            awFail( funcInfo->name, what );
         }
         awNbWindow++;
         continue;
      }
      if( !(seed > T) ) continue;   /* nothing to converge from: not a comparison */
      sawSeed = 1;

      for( l=0; l < 2; l++ )
      {
         for( o=0; o < funcInfo->nbOutput; o++ )
         {
            /* PREC_4 holds T as its floor; PREC_8 the committed one. */
            limitOf[o] = exp( -(double)(awLevel[l].K - 3) ) * seed;
            if( l == 0 && limitOf[o] < T ) limitOf[o] = T;
            if( l == 1 && limitOf[o] < awFloorOf( funcInfo->name, (int)o ) )
            {
               limitOf[o] = awFloorOf( funcInfo->name, (int)o );
               awNbFloorHeld++;
            }
         }
         TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, awLevel[l].level );
         awCall( funcInfo, vec, s, 0, awFull );
         if( awFull->retCode != TA_SUCCESS )
         {
            sprintf( what, "(%s, %s) PREC_%d", vec->label, awSeriesName[s], awLevel[l].X );
            sprintf( awMsg, "the call answered %d", (int)awFull->retCode );
            awFail( funcInfo->name, what );
            TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
            continue;
         }
         for( k=0; k < AW_NB_START; k++ )
         {
            awCall( funcInfo, vec, s, awStart[k], awLate );
            nb = awCompare( funcInfo, awFull, awLate, awStart[k], 0, limitOf, perOut, range, &bad );
            if( pathDep )
            {
               if( nb == 0 || bad >= 0 ) met = 0;
               continue;
            }
            if( nb < AW_MIN_BARS )
            {
               sprintf( what, "(%s, %s, start %d) PREC_%d", vec->label, awSeriesName[s], awStart[k], awLevel[l].X );
               sprintf( awMsg, "only %d bars compared, floor %d", nb, AW_MIN_BARS );
               awFail( funcInfo->name, what );
            }
            else if( bad >= 0 )
            {
               sprintf( what, "(%s, %s, start %d, bar %d) PREC_%d", vec->label, awSeriesName[s], awStart[k], bad, awLevel[l].X );
               sprintf( awMsg, "two starts differ by up to %g, limit %g, seed difference %g",
                        awMax( funcInfo, perOut ), limitOf[0], seed );
               awFail( funcInfo->name, what );
            }
         }
         TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
         if( !pathDep ) awNbConverging++;
      }
   }

   if( pathDep )
   {
      if( met && sawSeed )
      {
         sprintf( awMsg, "meets the converging criterion on every series and start: the path_dependent flag is wrong" );
         awFail( funcInfo->name, vec->label );
      }
      awNbPathDep++;
   }
}

/* --- driver ------------------------------------------------------------- */

/* rL8: under a level a lookback at a period of 1 stays 0, for the owner of an
 * id and for a function that only inherits one.
 */
static void awPeriod1( const TA_FuncInfo *funcInfo, const AwVector *def )
{
   const TA_OptInputParameterInfo *opt;
   AwVector v = *def;
   unsigned int i;
   int l, lb, found = 0;

   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &opt );
      if( !strcmp( opt->paramName, "optInTimePeriod" ) ) { v.val[i] = 1.0; found = 1; }
   }
   if( !found )
   {
      sprintf( awMsg, "is flagged identity at a period of 1 and has no optInTimePeriod" );
      awFail( funcInfo->name, "" );
      return;
   }
   for( l=0; l < 2; l++ )
   {
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, awLevel[l].level );
      lb = awLookback( funcInfo, &v );
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );
      if( lb != 0 )
      {
         sprintf( awMsg, "lookback at a period of 1 is %d under PREC_%d, not 0", lb, awLevel[l].X );
         awFail( funcInfo->name, "" );
      }
      awNbPeriod1++;
   }
}

static void awOneFunction( const TA_FuncInfo *funcInfo, void *opaque )
{
   static AwVector vec[AW_MAX_VEC];
   TA_FuncUnstId own = TA_FUNC_UNST_ALL;
   int n, v;

   (void)opaque;
   if( funcInfo->nbOutput > AW_MAX_OUT || funcInfo->nbOptInput > AW_MAX_OPT )
   {
      sprintf( awMsg, "has %u outputs and %u optional inputs, more than this file holds",
               funcInfo->nbOutput, funcInfo->nbOptInput );
      awFail( funcInfo->name, "" );
      return;
   }
   n = awBuildVectors( funcInfo, vec );
   if( funcInfo->flags & TA_FUNC_FLG_UNST_PER )
   {
      own = awOwnId( funcInfo->name );
      if( own == TA_FUNC_UNST_ALL )
      {
         sprintf( awMsg, "is flagged as owning an unstable id, which this file does not list" );
         awFail( funcInfo->name, "" );
      }
   }
   if( funcInfo->flags & TA_FUNC_FLG_PERIOD1_IDENTITY )
      for( v=0; v < n; v++ )
         awPeriod1( funcInfo, &vec[v] );
   memset( awTail, 0, sizeof(awTail) );
   for( v=0; v < n; v++ )
   {
      if( own != TA_FUNC_UNST_ALL )
         awCheckCount( funcInfo, &vec[v], own );
      awCheckOffset( funcInfo, &vec[v] );
      awCheckLevels( funcInfo, &vec[v] );
      awTwoStarts( funcInfo, &vec[v] );
   }
   if( awPrintFloors )
      for( v=0; v < (int)funcInfo->nbOutput; v++ )
         if( awTail[v] > 0.0 )
            printf( "\n   { \"%s\", %d, %.3e },", funcInfo->name, v, 4.0 * awTail[v] );
}

ErrorNumber test_func_auto_stabilization( TA_History *history )
{
   (void)history;

   awSeries = (AwSeries *)malloc( sizeof(AwSeries) * AW_NB_SERIES );
   awFull   = (AwRun *)malloc( sizeof(AwRun) );
   awLate   = (AwRun *)malloc( sizeof(AwRun) );
   if( !awSeries || !awFull || !awLate )
   {
      free( awSeries ); free( awFull ); free( awLate );
      return TA_AUTO_STABILIZATION_FAIL;
   }
   awNbReport = 0;
   awNbWindow = awNbConverging = awNbPathDep = awNbCount = awNbCountNonZero = awNbFloorHeld = 0;
   awNbSameValues = awNbPeriod1 = awNbOffset = awNbMonotone = 0;
   awPrintFloors = getenv( "TA_AUTO_STABILIZATION_FLOORS" ) != NULL;
   awBuildSeries();

   TA_ForEachFunc( awOneFunction, NULL );
   awAdoscMonotone();
   TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 0 );

   free( awSeries ); free( awFull ); free( awLate );
   awSeries = NULL; awFull = awLate = NULL;

   if( awNbReport != 0 )
   {
      printf( "\n  auto-stabilization: %d failure(s)\n", awNbReport );
      return TA_AUTO_STABILIZATION_FAIL;
   }
   if( awNbWindow < 1600 || awNbConverging < 1600 || awNbPathDep < 30 ||
       awNbCount < 200 || awNbCountNonZero < 175 || awNbSameValues < 200 || awNbPeriod1 < 100 || awNbOffset < 8 || awNbMonotone < 100000 ||
       ( !awPrintFloors && awNbFloorHeld < 60 ) )
   {
      printf( "\n  auto-stabilization: vacuous: %u window, %u converging, %u path-dependent, "
              "%u counts (%u above 0), %u owner value checks, %u period-1 lookbacks, "
              "%u outputs held to a floor\n",
              awNbWindow, awNbConverging, awNbPathDep, awNbCount, awNbCountNonZero,
              awNbSameValues, awNbPeriod1, awNbFloorHeld );
      return TA_AUTO_STABILIZATION_VACUOUS;
   }
   return TA_TEST_PASS;
}
