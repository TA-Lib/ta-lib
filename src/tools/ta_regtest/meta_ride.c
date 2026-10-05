#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_libc.h"
#include "ta_frame_priv.h"
#include "meta_ride.h"

#define TA_META_FRAME_IMPL
#include "ta_meta_frame.h"

#define MAX_OUT 10
#define MAX_IN  12
#define MAX_PRINTED 20

/* Value classes of one output: a sign or non-finite for a real or a plain
 * integer output, 0 or a level or C_OTHER for a pattern output. */
#define C_ZERO      0x001u
#define C_POS       0x002u
#define C_NEG       0x004u
#define C_NONFINITE 0x008u
#define C_P100      0x010u
#define C_N100      0x020u
#define C_P80       0x040u
#define C_N80       0x080u
#define C_P200      0x100u
#define C_N200      0x200u
#define C_OTHER     0x400u
#define C_NONE      0x800u /* declared by a pattern output with no value flag */
/* What a flag permits and a call did: TA_OUT_DISPLAY_SHIFT, TA_OUT_NULLABLE. */
#define C_SHIFTED   0x1000u
#define C_DECLINED  0x2000u

#define PATTERN_FLAGS (TA_OUT_PATTERN_BOOL | TA_OUT_PATTERN_BULL_BEAR)
#define SIGN_FLAGS    (TA_OUT_POSITIVE | TA_OUT_NEGATIVE | TA_OUT_ZERO)

/* A product of four inputs this large is still finite. Past it a non-finite
 * output is overflow, which TA_FUNC_FLG_NAN_INF_OUT does not describe. */
#define ORDINARY_MAX 1e75

typedef struct
{
   const TA_FuncInfo *info;
   const TA_OutputParameterInfo *out[MAX_OUT];
   int pattern[MAX_OUT];           /* a pattern output of rule rW8 */
   unsigned int declared[MAX_OUT]; /* 0: the output declares no value or sign */
   unsigned int required[MAX_OUT]; /* what some call of the run must show */
   unsigned int seen[MAX_OUT];
   unsigned long long calls;
} Site;

static Site sites[TA_META_FRAME_SIZE];
static int  resolved;
static long mismatches;

static unsigned long long finiteJudged, nonFiniteExcused;
static unsigned long long shiftJudged, patternJudged, pointerJudged;
static unsigned long long rangeJudged, rangeEmpty;

static int report( const Site *s )
{
   if( mismatches++ >= MAX_PRINTED )
      return 0;
   printf( "\nMETA RIDE [TA_%s]: ", s->info->name );
   return 1;
}

static int is_pattern( const TA_FuncInfo *func, const TA_OutputParameterInfo *o )
{
   if( o->type != TA_Output_Integer )
      return 0;
   return (o->flags & PATTERN_FLAGS) ||
          ((func->flags & TA_FUNC_FLG_CANDLESTICK) &&
           (o->flags & SIGN_FLAGS) == SIGN_FLAGS);
}

static unsigned int declared_classes( const TA_OutputParameterInfo *o, int pattern )
{
   unsigned int c = 0;
   int f = o->flags;

   if( f & TA_OUT_ZERO )
      c |= C_ZERO;
   if( pattern )
   {
      if( f & TA_OUT_POSITIVE )
         c |= C_P100 | ((f & TA_OUT_PATTERN_WEAK) ? C_P80 : 0)
                     | ((f & TA_OUT_PATTERN_CONFIRM) ? C_P200 : 0);
      if( f & TA_OUT_NEGATIVE )
         c |= C_N100 | ((f & TA_OUT_PATTERN_WEAK) ? C_N80 : 0)
                     | ((f & TA_OUT_PATTERN_CONFIRM) ? C_N200 : 0);
      return c ? c : C_NONE;
   }
   if( f & TA_OUT_POSITIVE ) c |= C_POS;
   if( f & TA_OUT_NEGATIVE ) c |= C_NEG;
   return c;
}

static void resolve( void )
{
   int i;
   unsigned int o;

   for( i = 0; i < TA_META_FRAME_SIZE; i++ )
   {
      const TA_FuncHandle *handle;
      Site *s = &sites[i];

      if( TA_GetFuncHandle( TA_MetaFrameName[i], &handle ) != TA_SUCCESS ||
          TA_GetFuncInfo( handle, &s->info ) != TA_SUCCESS ||
          s->info->nbOutput > MAX_OUT )
      {
         printf( "\nMETA RIDE: no usable metadata for TA_%s\n", TA_MetaFrameName[i] );
         s->info = NULL;
         mismatches++;
         continue;
      }
      for( o = 0; o < s->info->nbOutput; o++ )
      {
         TA_GetOutputParameterInfo( handle, o, &s->out[o] );
         s->pattern[o] = is_pattern( s->info, s->out[o] );
         s->declared[o] = declared_classes( s->out[o], s->pattern[o] );
         s->required[o] = s->declared[o] & (C_OTHER - 1);
         if( s->out[o]->flags & TA_OUT_DISPLAY_SHIFT ) s->required[o] |= C_SHIFTED;
         if( s->out[o]->flags & TA_OUT_NULLABLE )      s->required[o] |= C_DECLINED;
      }
   }
   resolved = 1;
}

static unsigned int real_classes( const double *v, int nb )
{
   unsigned int c = 0;
   int i;

   for( i = 0; i < nb; i++ )
   {
      double x = v[i];
      if( x > 0.0 )       c |= C_POS;
      else if( x < 0.0 )  c |= C_NEG;
      else if( x == 0.0 ) c |= C_ZERO;
      if( x - x != 0.0 )  c |= C_NONFINITE;
   }
   return c;
}

/* The exponent field is all ones exactly when adding one to it carries into
 * the sign bit, so the loop is three integer operations and vectorizes. */
static int any_non_finite( const double *v, int nb )
{
   unsigned long long acc = 0, bits;
   int i;

   for( i = 0; i < nb; i++ )
   {
      memcpy( &bits, &v[i], sizeof(bits) );
      acc |= (bits & 0x7FF0000000000000ULL) + 0x0010000000000000ULL;
   }
   return (int)(acc >> 63);
}

static unsigned int int_class( int x, int pattern )
{
   if( x == 0 )
      return C_ZERO;
   if( pattern )
      switch( x )
      {
      case  100: return C_P100;
      case -100: return C_N100;
      case   80: return C_P80;
      case  -80: return C_N80;
      case  200: return C_P200;
      case -200: return C_N200;
      default:   return C_OTHER;
      }
   return x > 0 ? C_POS : C_NEG;
}

/* 1 when every input bar the call could read is finite and of ordinary size. */
static int inputs_ordinary( int single, const void *const in[], int nbIn,
                            int first, int last )
{
   int k, i;

   for( k = 0; k < nbIn; k++ )
   {
      if( !in[k] )
         continue;
      for( i = first; i <= last; i++ )
      {
         double x = single ? (double)((const float *)in[k])[i]
                           : ((const double *)in[k])[i];
         if( !(fabs( x ) <= ORDINARY_MAX) )
            return 0;
      }
   }
   return 1;
}

void meta_ride_check( int site, int single, int startIdx, int endIdx,
                      const void *const in[], int nbIn,
                      int outBegIdx, int outNBElement, int lookback,
                      const int shift[], const void *const out[] )
{
   Site *s = &sites[site];
   int first = startIdx > lookback ? startIdx : lookback;
   int nb = outNBElement;
   unsigned int o;
   int i;

   if( !resolved )
      resolve();
   if( !s->info )
      return;
   s->calls++;

   rangeJudged++;
   if( first > endIdx )
   {
      rangeEmpty++;
      if( (nb != 0 || outBegIdx != 0) && report( s ) )
         printf( "range (%d, %d) with lookback %d answered outBegIdx %d, "
                 "outNBElement %d; nothing fits\n",
                 startIdx, endIdx, lookback, outBegIdx, nb );
   }
   else if( outBegIdx != first || nb != endIdx - first + 1 )
   {
      if( report( s ) )
         printf( "range (%d, %d) with lookback %d answered outBegIdx %d, "
                 "outNBElement %d; expected %d, %d\n",
                 startIdx, endIdx, lookback, outBegIdx, nb, first, endIdx - first + 1 );
      return;
   }

   for( o = 0; o < s->info->nbOutput; o++ )
   {
      const TA_OutputParameterInfo *info = s->out[o];
      int isInt = info->type == TA_Output_Integer;
      int pattern = s->pattern[o];
      unsigned int declared = s->declared[o];
      unsigned int c = 0, bad;

      shiftJudged++;
      if( shift[o] != 0 )
      {
         if( info->flags & TA_OUT_DISPLAY_SHIFT )
            s->seen[o] |= C_SHIFTED;
         else if( report( s ) )
            printf( "%s has display shift %d without TA_OUT_DISPLAY_SHIFT\n",
                    info->paramName, shift[o] );
      }

      pointerJudged++;
      if( !out[o] )
      {
         if( info->flags & TA_OUT_NULLABLE )
            s->seen[o] |= C_DECLINED;
         else if( report( s ) )
            printf( "%s was NULL in a successful call without TA_OUT_NULLABLE\n",
                    info->paramName );
         continue;
      }
      if( nb <= 0 || (isInt && !declared) )
         continue;

      if( isInt )
         for( i = 0; i < nb; i++ )
            c |= int_class( ((const int *)out[o])[i], pattern );
      else if( declared )
         c = real_classes( (const double *)out[o], nb );
      else if( any_non_finite( (const double *)out[o], nb ) )
         c = C_NONFINITE;
      s->seen[o] |= c & ~C_NONFINITE;

      if( !isInt )
      {
         int flagged = (s->info->flags & TA_FUNC_FLG_NAN_INF_OUT) != 0;

         if( !flagged )
            finiteJudged += nb;
         if( c & C_NONFINITE )
         {
            int from = outBegIdx - lookback;
            int inPlace = 0, k;
            unsigned int oo;

            /* Computed in place, the inputs are gone and nothing is judged. */
            for( k = 0; k < nbIn; k++ )
               for( oo = 0; oo < s->info->nbOutput; oo++ )
                  inPlace |= in[k] && in[k] == out[oo];
            if( inPlace ||
                !inputs_ordinary( single, in, nbIn, from > 0 ? from : 0, endIdx ) )
               nonFiniteExcused++;
            else if( flagged )
               s->seen[o] |= C_NONFINITE;
            else if( report( s ) )
            {
               const double *v = (const double *)out[o];
               for( i = 0; v[i] - v[i] == 0.0; i++ ) {}
               printf( "%s[%d] is %g, from ordinary inputs, without "
                       "TA_FUNC_FLG_NAN_INF_OUT (range %d, %d)\n",
                       info->paramName, i, v[i], startIdx, endIdx );
            }
         }
      }

      if( !declared )
         continue;
      if( pattern )
         patternJudged += nb;

      bad = c & ~declared & ~C_NONFINITE;
      if( !bad )
         continue;
      for( i = 0; i < nb; i++ )
      {
         unsigned int ci = isInt ? int_class( ((const int *)out[o])[i], pattern )
                                 : real_classes( &((const double *)out[o])[i], 1 );
         if( ci & bad )
            break;
      }
      if( i < nb && report( s ) )
      {
         if( isInt )
            printf( "%s[%d] is %d, ", info->paramName, i, ((const int *)out[o])[i] );
         else
            printf( "%s[%d] is %g, ", info->paramName, i, ((const double *)out[o])[i] );
         printf( "which its flags 0x%x do not declare (range %d, %d)\n",
                 (unsigned int)info->flags, startIdx, endIdx );
      }
   }
}

void meta_ride_call( const TA_ParamHolder *params, int startIdx, int endIdx,
                     int outBegIdx, int outNBElement )
{
   static int last;
   const TA_ParamHolderPriv *priv = (const TA_ParamHolderPriv *)params->hiddenData;
   const TA_FuncInfo *info = priv->funcInfo;
   const void *in[MAX_IN];
   const void *out[MAX_OUT];
   int shift[MAX_OUT];
   TA_Integer lookback, oneShift;
   int nbIn = 0;
   unsigned int i;

   if( !resolved )
      resolve();
   if( sites[last].info != info )
   {
      for( last = 0; last < TA_META_FRAME_SIZE && sites[last].info != info; last++ ) {}
      if( last == TA_META_FRAME_SIZE )
      {
         last = 0;
         if( mismatches++ < MAX_PRINTED )
            printf( "\nMETA RIDE [TA_%s]: no row in ta_meta_frame.h\n", info->name );
         return;
      }
   }

   for( i = 0; i < info->nbInput && nbIn + 6 <= MAX_IN; i++ )
   {
      const TA_ParamHolderInput *h = &priv->in[i];
      if( h->inputInfo->type == TA_Input_Price )
      {
         int f = h->inputInfo->flags;
         if( f & TA_IN_PRICE_OPEN )         in[nbIn++] = h->data.inPrice.open;
         if( f & TA_IN_PRICE_HIGH )         in[nbIn++] = h->data.inPrice.high;
         if( f & TA_IN_PRICE_LOW )          in[nbIn++] = h->data.inPrice.low;
         if( f & TA_IN_PRICE_CLOSE )        in[nbIn++] = h->data.inPrice.close;
         if( f & TA_IN_PRICE_VOLUME )       in[nbIn++] = h->data.inPrice.volume;
         if( f & TA_IN_PRICE_OPENINTEREST ) in[nbIn++] = h->data.inPrice.openInterest;
      }
      else if( h->inputInfo->type == TA_Input_Real )
         in[nbIn++] = h->data.inReal;
   }
   for( i = 0; i < info->nbOutput && i < MAX_OUT; i++ )
   {
      out[i] = priv->out[i].outputInfo->type == TA_Output_Integer
                  ? (const void *)priv->out[i].data.outInteger
                  : (const void *)priv->out[i].data.outReal;
      shift[i] = TA_GetDisplayShift( params, i, &oneShift ) == TA_SUCCESS ? (int)oneShift : 0;
   }
   if( TA_GetLookback( params, &lookback ) != TA_SUCCESS )
      lookback = -1;

   meta_ride_check( last, 0, startIdx, endIdx, in, nbIn, outBegIdx, outNBElement,
                    (int)lookback, shift, out );
}

long meta_ride_mismatches( void )
{
   return mismatches;
}

/* What an output declares that no call of the run showed. */
static const char *unreached( unsigned int c )
{
   switch( c )
   {
   case C_ZERO: return "0 and no call wrote it";
   case C_POS:  return "a positive value and no call wrote one";
   case C_NEG:  return "a negative value and no call wrote one";
   case C_P100: return "+100 and no call wrote it";
   case C_N100: return "-100 and no call wrote it";
   case C_P80:  return "+80 and no call wrote it";
   case C_N80:  return "-80 and no call wrote it";
   case C_P200: return "+200 and no call wrote it";
   case C_N200: return "-200 and no call wrote it";
   case C_SHIFTED: return "TA_OUT_DISPLAY_SHIFT and no call had a non-zero shift";
   default:     return "TA_OUT_NULLABLE and no call passed NULL";
   }
}

ErrorNumber meta_ride_whole_run( void )
{
   const struct { const char *what; unsigned long long count; } floors[] = {
      { "real output values held finite", finiteJudged },
      { "non-finite outputs excused by their inputs", nonFiniteExcused },
      { "display shifts", shiftJudged },
      { "pattern output values", patternJudged },
      { "output pointers", pointerJudged },
      { "output ranges", rangeJudged },
      { "ranges too short for the lookback", rangeEmpty },
   };
   ErrorNumber retValue = TA_TEST_PASS;
   unsigned int k, o, bit;
   int i;

   if( !resolved )
      resolve();

   for( k = 0; k < sizeof(floors) / sizeof(floors[0]); k++ )
      if( floors[k].count == 0 )
      {
         printf( "\nMETA RIDE: the suite produced no %s\n", floors[k].what );
         retValue = TA_META_RIDE_VACUOUS;
      }

   for( i = 0; i < TA_META_FRAME_SIZE; i++ )
   {
      const Site *s = &sites[i];
      if( !s->info )
         continue;
      if( s->calls == 0 )
      {
         printf( "\nMETA RIDE [TA_%s]: never called\n", s->info->name );
         retValue = TA_META_RIDE_UNREACHED;
      }
      if( s->info->flags & TA_FUNC_FLG_NAN_INF_OUT )
      {
         unsigned int seen = 0;
         for( o = 0; o < s->info->nbOutput; o++ )
            seen |= s->seen[o];
         if( !(seen & C_NONFINITE) )
         {
            printf( "\nMETA RIDE [TA_%s]: declares TA_FUNC_FLG_NAN_INF_OUT and no call "
                    "wrote a non-finite value from ordinary inputs\n", s->info->name );
            retValue = TA_META_RIDE_UNREACHED;
         }
      }
      for( o = 0; o < s->info->nbOutput; o++ )
         for( bit = 1; bit <= C_DECLINED; bit <<= 1 )
            if( (s->required[o] & bit) && !(s->seen[o] & bit) )
            {
               printf( "\nMETA RIDE [TA_%s]: %s declares %s\n",
                       s->info->name, s->out[o]->paramName, unreached( bit ) );
               retValue = TA_META_RIDE_UNREACHED;
            }
   }
   return retValue;
}
