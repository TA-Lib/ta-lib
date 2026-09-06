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
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #389).
 *  090626 MF,CC  Compatibility dimension dropped with #388.
 */

/* Description:
 *
 *   A rejected Open or OpenAndFill leaves the caller's output buffers exactly
 *   as it found them (website/src/api/README.md 3.4). Corpus-wide, both entry
 *   points.
 *
 *   WHY A RAMP RATHER THAN A COMPUTED SHORT HISTORY. The leg walks historyLen
 *   up from 0 and stops at the first length that produces a value, so the last
 *   rejection it sees is the one at the boundary -- whatever the boundary turns
 *   out to be. A probe built from TA_<N>_Lookback assumes the two agree, and
 *   #389 is a case where they did not. Nothing here knows a lookback.
 *
 *   It runs twice: once at the default parameters and once with every integer
 *   and enum parameter at its declared minimum, which is a different boundary
 *   and often a different code path (a period-1 arm rather than the general
 *   loop). A sweep that only ever probes the default period is the #147 trap.
 *
 *   THE POSITIVE CONTROL IS PER FUNCTION, PER ENTRY POINT, AND EVERY SLOT. At
 *   the first producing history the same buffers must come back CHANGED --
 *   all of them, because "one of them moved" is satisfied by a fixture that
 *   mis-binds the others, which is the thunk mistake the control exists to
 *   catch. Without it "nothing was written" is satisfied by pointers the
 *   function never writes through, and the sweep reads green over the whole
 *   corpus while probing none of it. Every function must also reach a producing
 *   history inside OC_MAX_BARS, or the run fails rather than skipping it.
 *
 *   TA_ALLOC_ERR IS OUT OF SCOPE rather than a hole: nothing past it is defined
 *   (docs/error-handling-spec.md, rule B7), and nothing here could provoke one
 *   anyway. Every rejection this sweep drives is a defined one.
 *
 *   The undersized-output class has no C expression at all -- TA_<N>_Open takes
 *   a bare output pointer and carries no length -- so it stays in the Rust
 *   crate's stream_open_contract.rs.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_test_func.h"
#include "ta_libc.h"
#include "ta_stream_frame.h"

/**** Local declarations. ****/

/* Long enough that every streaming function in the corpus produces a value at
 * its default parameters; the run fails naming any that does not, so this
 * cannot silently become too small. */
#define OC_MAX_BARS   320
#define OC_PAD        8      /* Slots past the longest legal fill. */
#define OC_BUF        (OC_MAX_BARS + OC_PAD)

#define OC_MAX_IN     8      /* Widest flattened input list (4 today). */
#define OC_MAX_OUT    8      /* Widest output list (4 today). */
#define OC_MAX_OPT    8

/* Leg A's parameter passes: the defaults, then the minimum integer/enum ones. */
#define OC_NB_PASS    2

/* Anything but these in a buffer after a non-success return is a write. Both
 * are values no indicator produces on this series, so a violation reads as a
 * value rather than as a bit pattern. */
#define OC_SENT_REAL  (-777.0)
#define OC_SENT_INT   (-777)

/* Pre-set outBegIdx/outNBElement, so "written" and "written as zero" are
 * distinguishable from each other. */
#define OC_SENT_IDX   (-999)
typedef struct {
   /* Input series, OC_MAX_BARS long. */
   double  open[OC_MAX_BARS];
   double  high[OC_MAX_BARS];
   double  low[OC_MAX_BARS];
   double  close[OC_MAX_BARS];
   double  volume[OC_MAX_BARS];
   double  openInterest[OC_MAX_BARS];
   double  periods[OC_MAX_BARS];

   /* Output buffers and the pristine copy every check compares against. */
   double  outReal[OC_MAX_OUT][OC_BUF];
   TA_Integer outInt[OC_MAX_OUT][OC_BUF];
   double  refReal[OC_BUF];
   TA_Integer refInt[OC_BUF];

   /* Counters, incremented AT the assertion rather than derived from a loop
    * bound: a count taken from the trip count stays healthy while the checks
    * inside are deleted. */
   long    nbRejectChecked;   /* rejections whose buffers were compared */
   long    nbSuccessChecked;  /* producing OpenAndFill calls, every slot moved */
   long    nbOpenControlled;  /* producing Open calls, every slot moved */
   long    nbParamRejects;    /* leg B probes answered TA_BAD_PARAM */
   long    nbParamAccepted;   /* leg B probes that were not rejected at all */
   long    nbParamMiscoded;   /* leg B probes rejected for some other reason */
   long    nbMinParamRejects; /* leg A rejections seen on a differing pass 1 */
   int     nbMinParamPasses;  /* functions whose pass 1 actually differed */
   int     nbMinParamFuncs;   /* functions whose table row says it should */
   int     nbViolation;       /* buffer writes on a rejection */
   int     nbCountLeaked;     /* rejections leaving outNBElement non-zero */
   int     nbFunc;
   int     nbReported;
} OcCtx;

/**** Local functions. ****/

static void oc_build_series( OcCtx *c )
{
   int i;
   for( i = 0; i < OC_MAX_BARS; i++ )
   {
      double base = 100.0 + 8.0 * sin( i * 0.11 ) + 0.03 * i;
      c->open[i]   = base;
      c->high[i]   = base + 1.25 + 0.5 * cos( i * 0.37 );
      c->low[i]    = base - 1.25 - 0.5 * cos( i * 0.23 );
      c->close[i]  = base + 0.4 * sin( i * 0.71 );
      if( c->high[i] < c->open[i]  ) c->high[i] = c->open[i];
      if( c->low[i]  > c->close[i] ) c->low[i]  = c->close[i];
      c->volume[i] = 10000.0 + 250.0 * ( 1.0 + sin( i * 0.29 ) );
      c->openInterest[i] = 5000.0 + 100.0 * i;
      /* MAVP's per-element period series: prices here would drive every
       * period out of range and make the function a no-op. */
      c->periods[i] = (double)( 2 + ( i % 25 ) );
   }
   for( i = 0; i < OC_BUF; i++ )
   {
      c->refReal[i] = OC_SENT_REAL;
      c->refInt[i]  = OC_SENT_INT;
   }
}

static const double *oc_series( const OcCtx *c, TA_VInputKind kind, int slot )
{
   switch( kind )
   {
   case TA_VIN_OPEN:         return c->open;
   case TA_VIN_HIGH:         return c->high;
   case TA_VIN_LOW:          return c->low;
   case TA_VIN_CLOSE:        return c->close;
   case TA_VIN_VOLUME:       return c->volume;
   case TA_VIN_OPENINTEREST: return c->openInterest;
   case TA_VIN_PERIODS:      return c->periods;
   case TA_VIN_REAL:
   default:
      /* Same convention as test_codegen.c and test_variants.c: the first real
       * input is the close series, a second one is volume. */
      return ( slot == 0 ) ? c->close : c->volume;
   }
}

/* Repaint every output buffer with the sentinel. */
static void oc_arm( OcCtx *c, const TA_StreamEntry *e )
{
   int o;
   for( o = 0; o < e->nbOutput; o++ )
   {
      if( e->outIsInt[o] ) memcpy( c->outInt[o],  c->refInt,  sizeof(c->refInt) );
      else                 memcpy( c->outReal[o], c->refReal, sizeof(c->refReal) );
   }
}

static int oc_slot_moved( const OcCtx *c, const TA_StreamEntry *e, int o )
{
   if( e->outIsInt[o] ) return memcmp( c->outInt[o],  c->refInt,  sizeof(c->refInt)  ) != 0;
   return                      memcmp( c->outReal[o], c->refReal, sizeof(c->refReal) ) != 0;
}

/* Index of the first output slot that moved, or -1 when none did. */
static int oc_first_moved( const OcCtx *c, const TA_StreamEntry *e )
{
   int o;
   for( o = 0; o < e->nbOutput; o++ )
      if( oc_slot_moved( c, e, o ) ) return o;
   return -1;
}

/* Index of the first output slot that did NOT move, or -1 when all did. The
 * control has to be every slot: "one of them moved" is satisfied by a fixture
 * that mis-binds all the others, which is the thunk mistake it exists to
 * catch. */
static int oc_first_still( const OcCtx *c, const TA_StreamEntry *e )
{
   int o;
   for( o = 0; o < e->nbOutput; o++ )
      if( !oc_slot_moved( c, e, o ) ) return o;
   return -1;
}

/* The offset and value of the first moved element of one output, for the
 * report: "wrote something" is not actionable, "wrote 66.6667 at [0]" is. */
static void oc_report_write( const OcCtx *c, const TA_StreamEntry *e,
                             const char *what,
                             int rc, int begIdx, int nbElement, int slot )
{
   int k;
   printf( "  OPEN-CONTRACT TA_%s %s: retCode=%d wrote the caller's buffer"
           " (outBegIdx=%d outNBElement=%d)\n",
           e->name, what, rc, begIdx, nbElement );
   for( k = 0; k < OC_BUF; k++ )
   {
      if( e->outIsInt[slot] )
      {
         if( c->outInt[slot][k] != OC_SENT_INT )
         {
            printf( "     output %d [%d] = %d\n", slot, k, (int)c->outInt[slot][k] );
            return;
         }
      }
      else if( c->outReal[slot][k] != OC_SENT_REAL )
      {
         printf( "     output %d [%d] = %g\n", slot, k, c->outReal[slot][k] );
         return;
      }
   }
}

/* A value strictly outside one parameter's domain.
 *
 * Not `min - 1` for a real: a real parameter's declared bounds are
 * -/+TA_REAL_MAX (3e37), where adding one is the identity and the "out of
 * range" probe lands back INSIDE the range. Integers and enums keep the exact
 * +/-1, which is the tightest probe and exact at their magnitudes. */
static double oc_out_of_range( const TA_VOptSpec *spec, int above )
{
   if( spec->kind == TA_VOPT_REAL )
      return above ? spec->maxValue + fabs( spec->maxValue ) * 0.5 + 1.0
                   : spec->minValue - fabs( spec->minValue ) * 0.5 - 1.0;
   return above ? spec->maxValue + 1.0 : spec->minValue - 1.0;
}

/* An out-of-range probe run at the shortest producing history trips the
 * insufficient-history guard too, so only the CODE says which one answered.
 * Accepting any rejection would let a deleted bound stay green. */
static void oc_param_judge_code( OcCtx *c, const TA_StreamEntry *e, int p,
                                 double value, const char *what, int rc )
{
   if( rc == TA_BAD_PARAM )
   {
      c->nbParamRejects++;
      return;
   }
   c->nbParamMiscoded++;
   if( c->nbReported < 12 )
   {
      c->nbReported++;
      printf( "  OPEN-CONTRACT TA_%s %s: %s=%g is outside [%g, %g] but the"
              " rejection was retCode=%d, not TA_BAD_PARAM.\n",
              e->name, what, e->optInput[p].name, value,
              e->optInput[p].minValue, e->optInput[p].maxValue, rc );
   }
}

/* Judge one call that came back non-success. */
static void oc_judge_reject( OcCtx *c, const TA_StreamEntry *e, int minParams,
                             const char *what, int rc, int begIdx, int nbElement )
{
   int slot;

   c->nbRejectChecked++;
   if( minParams ) c->nbMinParamRejects++;

   /* A count is what turns a write a caller cannot see into a value it reads.
    * 3.4 leaves the indices undefined on a rejection and most bodies zero them,
    * so only a NON-ZERO one is a finding. */
   if( nbElement != 0 && nbElement != OC_SENT_IDX )
   {
      if( c->nbReported < 12 )
      {
         c->nbReported++;
         printf( "  OPEN-CONTRACT TA_%s %s: retCode=%d reported outNBElement=%d\n",
                 e->name, what, rc, nbElement );
      }
      c->nbCountLeaked++;
   }

   slot = oc_first_moved( c, e );
   if( slot < 0 )
      return;

   if( c->nbReported < 12 )
   {
      c->nbReported++;
      oc_report_write( c, e, what, rc, begIdx, nbElement, slot );
   }
   c->nbViolation++;
}

/* One function. Returns the shortest history that produced a value, -1 when
 * none did inside OC_MAX_BARS, -2 when the positive control failed. */
static int oc_sweep_one( OcCtx *c, const TA_StreamEntry *e )
{
   const double *in[OC_MAX_IN];
   double       *outReal[OC_MAX_OUT];
   TA_Integer   *outInt[OC_MAX_OUT];
   double        opt[OC_MAX_OPT];
   int i, h, pass, firstOk = -1;

   for( i = 0; i < e->nbInput; i++ )
      in[i] = oc_series( c, e->inputKind[i], i );
   /* Both lists, always: a mixed-type function's thunk dereferences a slot in
    * each, and a homogeneous one (void)s the list it does not use. */
   for( i = 0; i < OC_MAX_OUT; i++ )
   {
      outReal[i] = c->outReal[i];
      outInt[i]  = c->outInt[i];
   }
   /* ---- Leg A: the history ramp, from an empty one up. Once at the default
    * parameters, once at the minimum integer/enum ones. Reals keep their
    * default: their declared floor is -TA_REAL_MAX, which is leg B's probe
    * rather than a second boundary. ---- */
  for( pass = 0; pass < OC_NB_PASS; pass++ )
  {
   int firstOkPass = -1, differs = 0;
   for( i = 0; i < e->nbOptInput; i++ )
   {
      opt[i] = e->optInput[i].defValue;
      if( pass == 1 && e->optInput[i].kind != TA_VOPT_REAL )
         opt[i] = e->optInput[i].minValue;
      /* Read back what was actually assigned: a selector that quietly stopped
       * selecting would still satisfy a test written against the table. */
      if( opt[i] != e->optInput[i].defValue )
         differs = 1;
   }
   /* A pass that would repeat pass 0 is not run: duplicated work reads as
    * coverage on any counter derived from the trip count. */
   if( pass == 1 )
   {
      if( !differs ) continue;
      c->nbMinParamPasses++;
   }

   for( h = 0; h <= OC_MAX_BARS; h++ )
   {
      void *stream = NULL;
      TA_Integer begIdx = OC_SENT_IDX, nbElement = OC_SENT_IDX;
      TA_RetCode rc;

      /* OpenAndFill: the entry that is handed the caller's whole output. */
      oc_arm( c, e );
      rc = e->openAndFill( &stream, in, h, opt, &begIdx, &nbElement, outReal, outInt );
      if( stream ) e->close( stream );

      if( rc != TA_SUCCESS )
      {
         oc_judge_reject( c, e, pass == 1, "OpenAndFill", (int)rc, (int)begIdx, (int)nbElement );
      }
      else if( nbElement > 0 )
      {
         /* The positive control: this call MUST have moved the buffers the
          * rejections above were checked against. */
         int still;
         c->nbSuccessChecked++;
         still = oc_first_still( c, e );
         if( still >= 0 )
         {
            printf( "  OPEN-CONTRACT TA_%s: OpenAndFill reported %d elements at"
                    " historyLen=%d (pass %d) and left output %d untouched -- the"
                    " sweep is checking a buffer this function does not write.\n",
                    e->name, (int)nbElement, h, pass, still );
            return -2;
         }
         firstOkPass = h;
      }

      /* Open: its own one-slot sink per output, on the same history. */
      stream = NULL;
      oc_arm( c, e );
      rc = e->open( &stream, in, h, opt, outReal, outInt );
      if( stream ) e->close( stream );
      if( rc != TA_SUCCESS )
         oc_judge_reject( c, e, pass == 1, "Open", (int)rc, OC_SENT_IDX, OC_SENT_IDX );
      else if( firstOkPass >= 0 )
      {
         /* Open's own control, on its own counter: it writes one slot per
          * output rather than a range, and nothing else here would notice a
          * thunk that handed it the wrong buffer. */
         int still = oc_first_still( c, e );
         c->nbOpenControlled++;
         if( still >= 0 )
         {
            printf( "  OPEN-CONTRACT TA_%s: Open succeeded at historyLen=%d"
                    " (pass %d) and left output %d untouched -- the sweep is"
                    " checking a buffer this function does not write.\n",
                    e->name, h, pass, still );
            return -2;
         }
      }

      if( firstOkPass >= 0 )
         break;
   }

   if( firstOkPass < 0 )
   {
      printf( "  OPEN-CONTRACT TA_%s: no history up to %d bars produced a value"
              " on pass %d, so the sweep never reached a rejection boundary.\n",
              e->name, OC_MAX_BARS, pass );
      return -1;
   }
   if( pass == 0 )
      firstOk = firstOkPass;
  }

   /* ---- Leg B: a parameter outside its range, on a history that otherwise
    * produces. A probe the tier ACCEPTS fails the run: a probe that landed back
    * inside the range would make the whole leg a no-op. ---- */
   for( i = 0; i < e->nbOptInput; i++ )
   {
      int k, side;
      for( side = 0; side < 2; side++ )
      {
         void *stream = NULL;
         TA_Integer begIdx = OC_SENT_IDX, nbElement = OC_SENT_IDX;
         TA_RetCode rc;

         for( k = 0; k < e->nbOptInput; k++ )
            opt[k] = e->optInput[k].defValue;
         opt[i] = oc_out_of_range( &e->optInput[i], side );

         oc_arm( c, e );
         rc = e->openAndFill( &stream, in, firstOk, opt, &begIdx, &nbElement, outReal, outInt );
         if( stream ) e->close( stream );
         if( rc != TA_SUCCESS )
         {
            oc_param_judge_code( c, e, i, opt[i], "OpenAndFill", (int)rc );
            oc_judge_reject( c, e, 0, "OpenAndFill/param", (int)rc, (int)begIdx, (int)nbElement );
         }
         else
         {
            c->nbParamAccepted++;
            printf( "  OPEN-CONTRACT TA_%s: %s=%g is outside its declared"
                    " [%g, %g] and OpenAndFill accepted it.\n",
                    e->name, e->optInput[i].name, opt[i],
                    e->optInput[i].minValue, e->optInput[i].maxValue );
         }

         stream = NULL;
         oc_arm( c, e );
         rc = e->open( &stream, in, firstOk, opt, outReal, outInt );
         if( stream ) e->close( stream );
         if( rc != TA_SUCCESS )
         {
            oc_param_judge_code( c, e, i, opt[i], "Open", (int)rc );
            oc_judge_reject( c, e, 0, "Open/param", (int)rc, OC_SENT_IDX, OC_SENT_IDX );
         }
         else
            c->nbParamAccepted++;
      }
   }
   return firstOk;
}

/**** Global functions. ****/

ErrorNumber test_func_open_contract( TA_History *history )
{
   static OcCtx ctx;   /* ~40 KB of buffers; not a stack frame. */
   int f, i, firstOk;

   (void)history;   /* The lengths are the subject here, so the series is local. */

   memset( &ctx, 0, sizeof(ctx) );
   oc_build_series( &ctx );

   for( f = 0; f < TA_STREAM_TABLE_SIZE; f++ )
   {
      const TA_StreamEntry *e = &TA_StreamTable[f];

      if( e->nbInput > OC_MAX_IN || e->nbOutput > OC_MAX_OUT ||
          e->nbOptInput > OC_MAX_OPT )
      {
         printf( "\nFail: TA_%s outgrew the open-contract sweep's fixture"
                 " (%d inputs, %d outputs, %d parameters).\n",
                 e->name, e->nbInput, e->nbOutput, e->nbOptInput );
         return TA_OPEN_CONTRACT_VACUOUS;
      }

      ctx.nbFunc++;
      for( i = 0; i < e->nbOptInput; i++ )
         if( e->optInput[i].kind != TA_VOPT_REAL &&
             e->optInput[i].minValue != e->optInput[i].defValue )
         {
            ctx.nbMinParamFuncs++;
            break;
         }

      firstOk = oc_sweep_one( &ctx, e );
      if( firstOk == -2 )
         return TA_OPEN_CONTRACT_VACUOUS;
      if( firstOk < 0 )
         return TA_OPEN_CONTRACT_VACUOUS;
   }

   if( ctx.nbViolation > 0 || ctx.nbCountLeaked > 0 )
   {
      printf( "\nFail: %d Open/OpenAndFill rejection%s wrote the caller's output"
              " buffer and %d reported a non-zero outNBElement (issue #389).\n",
              ctx.nbViolation, ctx.nbViolation == 1 ? "" : "s", ctx.nbCountLeaked );
      return TA_OPEN_CONTRACT_WROTE;
   }

   if( ctx.nbParamMiscoded > 0 )
   {
      printf( "\nFail: %ld out-of-range parameter probe%s were rejected for some"
              " reason other than the parameter, so they prove nothing about the"
              " bound.\n",
              ctx.nbParamMiscoded, ctx.nbParamMiscoded == 1 ? "" : "s" );
      return TA_OPEN_CONTRACT_VACUOUS;
   }

   if( ctx.nbParamAccepted > 0 )
   {
      printf( "\nFail: %ld out-of-range parameter probe%s were accepted by the"
              " streaming open tier.\n",
              ctx.nbParamAccepted, ctx.nbParamAccepted == 1 ? "" : "s" );
      return TA_OPEN_CONTRACT_VACUOUS;
   }

   /* Vacuity floors. Each has been a real failure mode of a sweep in this
    * suite: a corpus that shrank to nothing, a leg whose rejections all became
    * successes, and a control arm that stopped comparing. */
   {
      int wantControls = TA_STREAM_TABLE_SIZE + ctx.nbMinParamFuncs;
      if( ctx.nbFunc < TA_STREAM_TABLE_SIZE || ctx.nbRejectChecked == 0 ||
          ctx.nbParamRejects == 0 || ctx.nbMinParamRejects == 0 ||
          ctx.nbMinParamPasses != ctx.nbMinParamFuncs || ctx.nbMinParamFuncs == 0 ||
          ctx.nbSuccessChecked < wantControls || ctx.nbOpenControlled < wantControls )
      {
         printf( "\nFail: open-contract sweep ran thin -- %d functions,"
                 " %ld rejections checked (%ld parameter probes, %ld on a"
                 " minimum-parameter pass), %d of %d minimum-parameter passes run,"
                 " %ld OpenAndFill and %ld Open producing calls controlled"
                 " (expected %d functions and %d controls of each).\n",
                 ctx.nbFunc, ctx.nbRejectChecked, ctx.nbParamRejects,
                 ctx.nbMinParamRejects, ctx.nbMinParamPasses, ctx.nbMinParamFuncs,
                 ctx.nbSuccessChecked, ctx.nbOpenControlled, TA_STREAM_TABLE_SIZE,
                 wantControls );
         return TA_OPEN_CONTRACT_VACUOUS;
      }
   }

   return TA_TEST_PASS;
}
