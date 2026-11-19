/* ta_fuzz: libFuzzer harness calling every function of the abstract API.
 *
 * Each input picks a function, its optional parameters (in range, at the
 * bounds, just outside them, NaN and infinite reals), an index range (valid
 * and rejected) and the settings it runs under, then builds its input series
 * from the remaining bytes. Every buffer is its own allocation sized exactly
 * to the contract, so an access the contract does not allow lands in a redzone.
 * Any broken promise aborts.
 *
 * Series stay inside the documented input domain: finite, within
 * [TA_REAL_MIN, TA_REAL_MAX], well-formed bars (low <= open, close <= high,
 * volume >= 0) and positive where TA_FUNC_FLG_POSITIVE_INPUT is set. Out of
 * that domain nothing is promised, so a finding there is not a bug.
 *
 * TA_FUZZ_STATS set in the environment prints, at exit, how many functions
 * were called, how many returned at least one value, and how the calls ended.
 *
 * Built with -DTA_FUZZ_SELFTEST, the harness reads one element past an output
 * buffer of its own after any call that returned a value, so a build can show
 * the sanitizer and the fuzzer report a fault. Never set it otherwise.
 */

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ta_libc.h"

#define MAX_BARS    400
#define MAX_FUNCS   1024
#define MAX_SERIES  16
#define MAX_OUTPUTS 16

static const TA_FuncInfo *funcs[MAX_FUNCS];
static unsigned int nbFuncs;
static unsigned int savedUnstable[TA_FUNC_UNST_COUNT];
static unsigned long statCalled[MAX_FUNCS];
static unsigned long statProduced[MAX_FUNCS];
static unsigned long statEmpty, statBadParam, statBadStart, statBadEnd;

/* Past the end of the input, bytes continue from a generator seeded by the
 * whole input, so a short input still yields a long series that is not flat. */
typedef struct
{
   const uint8_t *p;
   size_t left;
   uint32_t tail;
} Bytes;

static Bytes bytes_of( const uint8_t *data, size_t size )
{
   Bytes b;
   size_t i;
   b.p = data;
   b.left = size;
   b.tail = 2166136261u;
   for( i = 0; i < size; i++ )
      b.tail = (b.tail ^ data[i]) * 16777619u;
   if( b.tail == 0 )
      b.tail = 1;
   return b;
}

static unsigned int u8( Bytes *b )
{
   if( b->left == 0 )
   {
      b->tail ^= b->tail << 13;
      b->tail ^= b->tail >> 17;
      b->tail ^= b->tail << 5;
      return b->tail >> 24;
   }
   b->left--;
   return *b->p++;
}

static unsigned int u16( Bytes *b )
{
   unsigned int lo = u8( b );
   return lo | (u8( b ) << 8);
}

static const char *curName = "?";

#define REQUIRE( cond, ... ) do { \
   if( !(cond) ) { \
      fprintf( stderr, "ta_fuzz: %s: ", curName ); \
      fprintf( stderr, __VA_ARGS__ ); \
      fputc( '\n', stderr ); \
      abort(); \
   } } while( 0 )

static void collect( const TA_FuncInfo *info, void *opaque )
{
   (void)opaque;
   if( nbFuncs == MAX_FUNCS )
   {
      fprintf( stderr, "ta_fuzz: more than %d functions, raise MAX_FUNCS\n", MAX_FUNCS );
      abort();
   }
   funcs[nbFuncs++] = info;
}

static void print_stats( void )
{
   unsigned int i, called = 0, produced = 0;
   for( i = 0; i < nbFuncs; i++ )
   {
      called += statCalled[i] != 0;
      produced += statProduced[i] != 0;
   }
   fprintf( stderr, "ta_fuzz: %u functions listed, %u called, %u returned values\n",
            nbFuncs, called, produced );
   fprintf( stderr, "ta_fuzz: calls with no value %lu, TA_BAD_PARAM %lu, "
            "TA_OUT_OF_RANGE_START_INDEX %lu, TA_OUT_OF_RANGE_END_INDEX %lu\n",
            statEmpty, statBadParam, statBadStart, statBadEnd );
}

static void init_once( void )
{
   static int done;
   unsigned int i;

   if( done )
      return;
   done = 1;
   REQUIRE( TA_Initialize() == TA_SUCCESS, "TA_Initialize failed" );
   REQUIRE( TA_ForEachFunc( collect, NULL ) == TA_SUCCESS, "TA_ForEachFunc failed" );
   REQUIRE( nbFuncs > 0, "the abstract API lists no function" );
   for( i = 0; i < TA_FUNC_UNST_COUNT; i++ )
      savedUnstable[i] = TA_GetUnstablePeriod( (TA_FuncUnstId)i );
   if( getenv( "TA_FUZZ_STATS" ) )
      atexit( print_stats );
}

/* Settings are process-wide, so every input restores them before it returns:
 * otherwise a crash would depend on the inputs that ran before it and would
 * not reproduce from its own bytes. */
static void apply_settings( Bytes *b )
{
   unsigned int mode = u8( b ) % 8;
   unsigned int k = u8( b );

   switch( mode )
   {
   case 4:
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, k % 64 );
      break;
   case 5:
      TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, (k & 1) ? TA_UNSTABLE_AUTO_PREC_8
                                                       : TA_UNSTABLE_AUTO_PREC_4 );
      break;
   case 6:
   {
      unsigned int range = u8( b ) % 3;
      unsigned int avg = u8( b ) % 32;
      double factor = u8( b ) / 16.0;
      REQUIRE( TA_SetCandleSettings( (TA_CandleSettingType)(k % TA_AllCandleSettings),
                                     (TA_RangeType)range, (int)avg, factor ) == TA_SUCCESS,
               "TA_SetCandleSettings rejected an accepted value" );
      break;
   }
   default:
      break;
   }
}

static void restore_settings( void )
{
   unsigned int i;
   for( i = 0; i < TA_FUNC_UNST_COUNT; i++ )
      TA_SetUnstablePeriod( (TA_FuncUnstId)i, savedUnstable[i] );
   TA_RestoreCandleDefaultSettings( TA_AllCandleSettings );
}

/* Binds optional parameter i and returns 1 when the value bound is outside
 * the declared domain, which the call must reject. A value equal to the
 * default sentinel selects the default, so it never counts as outside. */
static int bind_opt( TA_ParamHolder *ph, unsigned int i, const TA_OptInputParameterInfo *info,
                     Bytes *b )
{
   unsigned int mode = u8( b ) % 16;

   if( mode == 0 )
      return 0;

   if( info->type == TA_OptInput_IntegerRange )
   {
      const TA_IntegerRange *r = (const TA_IntegerRange *)info->dataSet;
      long long v, span = (long long)r->max - r->min;
      switch( mode )
      {
      case 1:  v = TA_INTEGER_DEFAULT; break;
      case 2:  v = r->min; break;
      case 3:  v = r->max; break;
      case 4:  v = (long long)r->min - 1; break;
      case 5:  v = (long long)r->max + 1; break;
      case 13: case 14: case 15:
               v = r->min + (long long)(u16( b ) % (unsigned long long)(span + 1)); break;
      default: v = r->min + (long long)(u8( b ) % 32); if( v > r->max ) v = r->max; break;
      }
      if( v < INT_MIN || v > INT_MAX )
         v = r->max;
      REQUIRE( TA_SetOptInputParamInteger( ph, i, (TA_Integer)v ) == TA_SUCCESS,
               "TA_SetOptInputParamInteger(%u) refused", i );
      return v != TA_INTEGER_DEFAULT && (v < r->min || v > r->max);
   }

   if( info->type == TA_OptInput_IntegerList )
   {
      const TA_IntegerList *l = (const TA_IntegerList *)info->dataSet;
      unsigned int j;
      long long v, lo = INT_MAX, hi = INT_MIN;
      int listed = 0;
      for( j = 0; j < l->nbElement; j++ )
      {
         if( l->data[j].value < lo ) lo = l->data[j].value;
         if( l->data[j].value > hi ) hi = l->data[j].value;
      }
      switch( mode )
      {
      case 1:  v = TA_INTEGER_DEFAULT; break;
      case 2:  v = lo - 1; break;
      case 3:  v = hi + 1; break;
      default: v = l->nbElement ? l->data[u8( b ) % l->nbElement].value : TA_INTEGER_DEFAULT; break;
      }
      if( v < INT_MIN || v > INT_MAX )
         v = TA_INTEGER_DEFAULT;
      for( j = 0; j < l->nbElement; j++ )
         listed |= l->data[j].value == v;
      REQUIRE( TA_SetOptInputParamInteger( ph, i, (TA_Integer)v ) == TA_SUCCESS,
               "TA_SetOptInputParamInteger(%u) refused", i );
      return v != TA_INTEGER_DEFAULT && !listed;
   }

   if( info->type == TA_OptInput_RealRange )
   {
      const TA_RealRange *r = (const TA_RealRange *)info->dataSet;
      double v, f = u16( b ) / 65535.0;
      switch( mode )
      {
      case 1:  v = TA_REAL_DEFAULT; break;
      case 2:  v = r->min; break;
      case 3:  v = r->max; break;
      case 4:  v = nextafter( r->min, -HUGE_VAL ); break;
      case 5:  v = nextafter( r->max, HUGE_VAL ); break;
      case 6:  v = NAN; break;
      case 7:  v = (f < 0.5) ? -HUGE_VAL : HUGE_VAL; break;
      case 8: case 9: case 10:
               v = r->min + (r->max - r->min) * f; break;
      default: v = r->suggested_start + (r->suggested_end - r->suggested_start) * f;
               if( v < r->min ) v = r->min;
               if( v > r->max ) v = r->max;
               break;
      }
      REQUIRE( TA_SetOptInputParamReal( ph, i, v ) == TA_SUCCESS,
               "TA_SetOptInputParamReal(%u) refused", i );
      return v != TA_REAL_DEFAULT && !(v >= r->min && v <= r->max);
   }

   if( info->type == TA_OptInput_RealList )
   {
      const TA_RealList *l = (const TA_RealList *)info->dataSet;
      if( l->nbElement == 0 )
         return 0;
      REQUIRE( TA_SetOptInputParamReal( ph, i, l->data[u8( b ) % l->nbElement].value ) == TA_SUCCESS,
               "TA_SetOptInputParamReal(%u) refused", i );
      return 0;
   }

   REQUIRE( 0, "optional parameter %u has unknown type %d", i, (int)info->type );
   return 0;
}

typedef struct
{
   double center, amp, w;
   int shape;
} Walk;

/* Magnitudes stay at or below 1e37, so a bar's high or low, at most
 * 1.8 * amp from the center, never leaves [TA_REAL_MIN, TA_REAL_MAX]. */
static const double kMag[] = { 1e-9, 1e-3, 0.5, 1.0, 37.25, 1e3, 1e6, 1e12, 1e24, 1e37 };
#define NB_MAG (sizeof(kMag) / sizeof(kMag[0]))

static void walk_init( Walk *w, Bytes *b, int positive )
{
   static const double kRel[] = { 0.0, 1e-6, 0.01, 0.1, 0.5 };
   unsigned int c = u8( b );
   w->shape = (int)(u8( b ) % 4);
   w->w = 0.0;
   w->center = kMag[c % NB_MAG];
   if( positive )
   {
      w->amp = w->center * kRel[(c / NB_MAG) % 5];
      return;
   }
   switch( (c / NB_MAG) % 4 )
   {
   case 0:  w->center = 0.0; break;
   case 1:  w->center = -w->center; break;
   default: break;
   }
   w->amp = kMag[u8( b ) % NB_MAG];
}

/* A value in [center - amp, center + amp]. */
static double walk_next( Walk *w, Bytes *b )
{
   int s = (int)(signed char)u8( b );
   switch( w->shape )
   {
   case 0:  w->w += s / 512.0; break;
   case 1:  w->w = s / 128.0; break;
   case 2:  if( s > 120 || s < -120 ) w->w = s / 128.0; break;
   default: w->w += (s & 1) ? 1.0 / 64 : -1.0 / 64; break;
   }
   if( w->w > 1.0 ) w->w = 1.0;
   if( w->w < -1.0 ) w->w = -1.0;
   return w->center + w->amp * w->w;
}

static void fill_real( double *x, int n, Bytes *b, int positive )
{
   Walk w;
   int i;
   walk_init( &w, b, positive );
   for( i = 0; i < n; i++ )
      x[i] = walk_next( &w, b );
}

/* Spreads are at most 0.8 * amp below the lower of open and close, and amp is
 * at most half the center on a positive series, so its low stays above 0. */
static void fill_price( double *const comp[6], int n, Bytes *b, int positive )
{
   static const double kVol[] = { 0.0, 1.0, 1e3, 1e6, 1e9 };
   Walk w;
   double prevClose, vol;
   int i, ownOpen;

   walk_init( &w, b, positive );
   ownOpen = (int)(u8( b ) & 1);
   vol = kVol[u8( b ) % 5];
   prevClose = w.center;
   for( i = 0; i < n; i++ )
   {
      double c = walk_next( &w, b );
      double o = ownOpen ? w.center + w.amp * ((int)(signed char)u8( b ) / 128.0) : prevClose;
      double hi = (o > c ? o : c) + w.amp * 0.8 * (u8( b ) / 255.0);
      double lo = (o < c ? o : c) - w.amp * 0.8 * (u8( b ) / 255.0);
      if( comp[0] ) comp[0][i] = o;
      if( comp[1] ) comp[1][i] = hi;
      if( comp[2] ) comp[2][i] = lo;
      if( comp[3] ) comp[3][i] = c;
      if( comp[4] ) comp[4][i] = vol * (u8( b ) / 255.0);
      if( comp[5] ) comp[5][i] = vol * (u8( b ) / 255.0);
      prevClose = c;
   }
}

typedef struct
{
   void *buf;
   void *copy;
   size_t bytes;
} Series;

static void *xalloc( size_t bytes )
{
   void *p = malloc( bytes ? bytes : 1 );
   REQUIRE( p != NULL, "out of memory" );
   return p;
}

static Series *new_series( Series *all, int *nb, size_t bytes )
{
   Series *s;
   REQUIRE( *nb < MAX_SERIES, "more than %d input series", MAX_SERIES );
   s = &all[(*nb)++];
   s->bytes = bytes;
   s->buf = xalloc( bytes );
   s->copy = NULL;
   return s;
}

static const uint64_t kRealSentinel = 0x7FF4DEADBEEF0001ull;
static const int32_t kIntSentinel = 0x5EEDF00D;

static int untouched( const void *buf, int isReal, int from, int size )
{
   int i;
   for( i = from; i < size; i++ )
   {
      if( isReal )
      {
         uint64_t bits;
         memcpy( &bits, (const double *)buf + i, sizeof bits );
         if( bits != kRealSentinel )
            return 0;
      }
      else if( ((const int *)buf)[i] != kIntSentinel )
         return 0;
   }
   return 1;
}

int LLVMFuzzerTestOneInput( const uint8_t *data, size_t size )
{
   Bytes b = bytes_of( data, size );
   const TA_FuncInfo *fi;
   TA_ParamHolder *ph = NULL;
   Series series[MAX_SERIES];
   void *outBuf[MAX_OUTPUTS];
   int outIsReal[MAX_OUTPUTS];
   int nbSeries = 0, startIdx, endIdx, len, outBeg = -1, outNb = -1;
   int paramOut = 0, startBad, endBad, lbRejected, beg, expected, outSize, k;
   unsigned int fIdx, i, idxMode;
   TA_Integer lookback = 0;
   TA_RetCode rc;

   init_once();

   fIdx = u16( &b ) % nbFuncs;
   fi = funcs[fIdx];
   curName = fi->name;
   REQUIRE( fi->nbInput <= MAX_SERIES && fi->nbOutput <= MAX_OUTPUTS, "too many inputs or outputs" );
   statCalled[fIdx]++;

   /* The inputs hold exactly endIdx + 1 bars, the least the contract allows. */
   len = (int)(u16( &b ) % (MAX_BARS + 1));
   idxMode = u8( &b ) % 32;
   if( idxMode >= 16 )
      idxMode %= 12;
   endIdx = len - 1;
   if( len == 0 )
   {
      endIdx = (idxMode & 1) ? -1 : INT_MIN;
      startIdx = (idxMode & 2) ? 0 : -1;
   }
   else
   {
      switch( idxMode )
      {
      case 0: case 1: case 2: case 3: startIdx = 0; break;
      case 4: case 5:                 startIdx = endIdx; break;
      case 6: case 7: case 8: case 9: startIdx = (int)(u16( &b ) % (unsigned int)len); break;
      case 10: case 11:               startIdx = endIdx - (int)(u8( &b ) % 8); break;
      case 12:                        startIdx = -1; break;
      case 13:                        startIdx = INT_MIN; break;
      case 14:                        startIdx = TA_INDEX_MAX + 1; break;
      default:                        startIdx = endIdx + 1; break;
      }
      if( idxMode >= 10 && idxMode <= 11 && startIdx < 0 )
         startIdx = 0;
   }
   startBad = startIdx < 0 || startIdx > TA_INDEX_MAX;
   endBad = endIdx < 0 || endIdx > TA_INDEX_MAX || endIdx < startIdx;

   apply_settings( &b );

   REQUIRE( TA_ParamHolderAlloc( fi->handle, &ph ) == TA_SUCCESS && ph, "TA_ParamHolderAlloc failed" );

   for( i = 0; i < fi->nbOptInput; i++ )
   {
      const TA_OptInputParameterInfo *info;
      REQUIRE( TA_GetOptInputParameterInfo( fi->handle, i, &info ) == TA_SUCCESS, "opt info %u", i );
      paramOut |= bind_opt( ph, i, info, &b );
   }

   for( i = 0; i < fi->nbInput; i++ )
   {
      const TA_InputParameterInfo *info;
      int positive = (fi->flags & TA_FUNC_FLG_POSITIVE_INPUT) != 0;
      REQUIRE( TA_GetInputParameterInfo( fi->handle, i, &info ) == TA_SUCCESS, "input info %u", i );
      if( info->type == TA_Input_Real )
      {
         Series *s = new_series( series, &nbSeries, (size_t)len * sizeof(double) );
         fill_real( (double *)s->buf, len, &b, positive );
         REQUIRE( TA_SetInputParamRealPtr( ph, i, (const double *)s->buf ) == TA_SUCCESS, "bind input %u", i );
      }
      else if( info->type == TA_Input_Integer )
      {
         Series *s = new_series( series, &nbSeries, (size_t)len * sizeof(int) );
         for( k = 0; k < len; k++ )
            ((int *)s->buf)[k] = (int)(signed char)u8( &b );
         REQUIRE( TA_SetInputParamIntegerPtr( ph, i, (const int *)s->buf ) == TA_SUCCESS, "bind input %u", i );
      }
      else
      {
         static const TA_InputFlags kComp[6] = { TA_IN_PRICE_OPEN, TA_IN_PRICE_HIGH, TA_IN_PRICE_LOW,
                                                 TA_IN_PRICE_CLOSE, TA_IN_PRICE_VOLUME,
                                                 TA_IN_PRICE_OPENINTEREST };
         double *comp[6];
         int c;
         for( c = 0; c < 6; c++ )
            comp[c] = (info->flags & kComp[c])
                    ? (double *)new_series( series, &nbSeries, (size_t)len * sizeof(double) )->buf
                    : NULL;
         fill_price( comp, len, &b, positive );
         REQUIRE( TA_SetInputParamPricePtr( ph, i, comp[0], comp[1], comp[2], comp[3], comp[4],
                                            comp[5] ) == TA_SUCCESS, "bind price input %u", i );
      }
   }
   for( k = 0; k < nbSeries; k++ )
   {
      series[k].copy = xalloc( series[k].bytes );
      memcpy( series[k].copy, series[k].buf, series[k].bytes );
   }

   REQUIRE( TA_GetLookback( ph, &lookback ) == TA_SUCCESS, "TA_GetLookback failed" );
   lbRejected = lookback < 0;
   REQUIRE( !paramOut || lbRejected, "lookback %d accepts a parameter outside its declared range",
            (int)lookback );

   /* Outputs hold max(1, count), count being what rW1 says the call returns. */
   beg = (startIdx > lookback) ? startIdx : lookback;
   expected = (!lbRejected && !startBad && !endBad && endIdx >= beg) ? endIdx - beg + 1 : 0;
   outSize = expected > 0 ? expected : 1;
   for( i = 0; i < fi->nbOutput; i++ )
   {
      const TA_OutputParameterInfo *info;
      REQUIRE( TA_GetOutputParameterInfo( fi->handle, i, &info ) == TA_SUCCESS, "output info %u", i );
      outIsReal[i] = info->type == TA_Output_Real;
      if( outIsReal[i] )
      {
         double *o = (double *)xalloc( (size_t)outSize * sizeof(double) );
         for( k = 0; k < outSize; k++ )
            memcpy( &o[k], &kRealSentinel, sizeof(double) );
         outBuf[i] = o;
         REQUIRE( TA_SetOutputParamRealPtr( ph, i, o ) == TA_SUCCESS, "bind output %u", i );
      }
      else
      {
         int *o = (int *)xalloc( (size_t)outSize * sizeof(int) );
         for( k = 0; k < outSize; k++ )
            o[k] = kIntSentinel;
         outBuf[i] = o;
         REQUIRE( TA_SetOutputParamIntegerPtr( ph, i, o ) == TA_SUCCESS, "bind output %u", i );
      }
   }

   rc = TA_CallFunc( ph, startIdx, endIdx, &outBeg, &outNb );

#define CTX_FMT "start=%d end=%d lookback=%d rc=%d outBeg=%d outNb=%d"
#define CTX_ARGS startIdx, endIdx, (int)lookback, (int)rc, outBeg, outNb

   if( !startBad && !endBad )
   {
      REQUIRE( rc == (lbRejected ? TA_BAD_PARAM : TA_SUCCESS),
               "valid range, lookback %s, unexpected code: " CTX_FMT,
               lbRejected ? "rejects" : "accepts", CTX_ARGS );
   }
   else
   {
      REQUIRE( (startBad && rc == TA_OUT_OF_RANGE_START_INDEX) ||
               (endBad && rc == TA_OUT_OF_RANGE_END_INDEX) ||
               (lbRejected && rc == TA_BAD_PARAM),
               "rejected range, code is none of its faults: " CTX_FMT, CTX_ARGS );
   }

   if( rc == TA_SUCCESS )
   {
      REQUIRE( outNb == expected, "count differs from endIdx - max(startIdx, lookback) + 1: " CTX_FMT, CTX_ARGS );
      REQUIRE( expected == 0 || outBeg == beg, "outBegIdx is not max(startIdx, lookback): " CTX_FMT, CTX_ARGS );
      for( i = 0; i < fi->nbOutput; i++ )
         REQUIRE( untouched( outBuf[i], outIsReal[i], outNb, outSize ),
                  "output %u written past the count: " CTX_FMT, i, CTX_ARGS );
      if( outNb > 0 )
         statProduced[fIdx]++;
      else
         statEmpty++;
#ifdef TA_FUZZ_SELFTEST
      if( outNb > 0 )
      {
         volatile double past = outIsReal[0] ? ((const double *)outBuf[0])[outSize]
                                             : ((const int *)outBuf[0])[outSize];
         (void)past;
      }
#endif
   }
   else
   {
      statBadParam += rc == TA_BAD_PARAM;
      statBadStart += rc == TA_OUT_OF_RANGE_START_INDEX;
      statBadEnd += rc == TA_OUT_OF_RANGE_END_INDEX;
      for( i = 0; i < fi->nbOutput; i++ )
         REQUIRE( untouched( outBuf[i], outIsReal[i], 0, outSize ),
                  "output %u written by a rejected call: " CTX_FMT, i, CTX_ARGS );
   }
   for( k = 0; k < nbSeries; k++ )
      REQUIRE( memcmp( series[k].buf, series[k].copy, series[k].bytes ) == 0,
               "input series %d modified: " CTX_FMT, k, CTX_ARGS );
#undef CTX_FMT
#undef CTX_ARGS

   for( i = 0; i < fi->nbOutput; i++ )
      free( outBuf[i] );
   for( k = 0; k < nbSeries; k++ )
   {
      free( series[k].buf );
      free( series[k].copy );
   }
   TA_ParamHolderFree( ph );
   restore_settings();
   return 0;
}
