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
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090626 KL     First version (issue #389).
 */

/* Description:
 *
 *   A rejected Open or OpenAndFill leaves the caller's output buffers exactly
 *   as it found them. Corpus-wide, both compatibility modes, both entry points.
 *
 *   The property is stated in website/src/api/README.md 3.4 ("on anything else,
 *   treat outBegIdx and outNBElement as undefined and the output buffers as
 *   untouched") and had one test behind it: an undersized
 *   output handed to SMA in the Rust crate. That rejection is raised by the
 *   public frame BEFORE the transcribed body runs, so it structurally cannot
 *   write -- the suite asserted the property on the one rejection class that
 *   can never violate it. The class that CAN is a rejection raised from inside
 *   the body, after it has already written: TA_RSI_OpenAndFill and
 *   TA_CMO_OpenAndFill under Metastock return TA_INSUFFICIENT_HISTORY with a
 *   plausible RSI value already in outReal[0] (issue #389).
 *
 *   WHY A RAMP RATHER THAN A COMPUTED SHORT HISTORY. The leg walks historyLen
 *   up from 0 and stops at the first length that produces a value, so the
 *   last rejection it sees is the one at the boundary -- whatever the boundary
 *   turns out to be. That matters here: under Metastock TA_RSI_Lookback says
 *   13 while Open needs 15 bars, so a probe built from the lookback lands one
 *   bar away from the violating call. Nothing in the leg knows a lookback.
 *
 *   WHAT IS ASSERTED, AND WHAT IS ONLY RATCHETED. The output BUFFERS are the
 *   assertion. outBegIdx/outNBElement are not: the spec calls them undefined
 *   on a non-success return, and most bodies legitimately zero them on the
 *   no-data path. A rejection leaving outNBElement NON-ZERO is what turns a
 *   silent write into a value a caller trusting the count will read, so it
 *   carries a ceiling instead -- measured at exactly two over the whole
 *   corpus, and those two are the same pair listed below.
 *
 *   THE POSITIVE CONTROL IS PER FUNCTION, not a corpus total. At the first
 *   producing history the same buffers must come back CHANGED. Without it
 *   "nothing was written" is satisfied by a fixture whose pointers the
 *   function never writes through at all -- a mis-sized buffer, a thunk
 *   handed the wrong slot -- and the sweep would read green over 201
 *   functions while probing none of them. Every function must also reach that
 *   producing history inside OC_MAX_BARS, or the run fails rather than
 *   quietly skipping it.
 *
 *   RSI AND CMO UNDER METASTOCK ARE LISTED AS OPEN, and asserted to STILL
 *   violate. The fix is a decision about the shipped library rather than
 *   something to slip in under a gate -- issue #388 may delete the
 *   compatibility mode outright, and the alternative (buffering the fill and
 *   committing it on success) costs a copy on every successful call. The
 *   sweep says so out loud and cannot rot into a silent pass: a listed row
 *   that stops violating fails the run.
 *
 *   THE PARAMETER LEG CARRIES ITS OWN ASSERTION. Its rejections come from the
 *   public frame, before the body runs, so they are the class that cannot
 *   write -- having both classes here is what makes "only the in-body class
 *   writes" a measurement rather than a claim. For that to mean anything the
 *   probe has to actually be out of range, so a probe the tier ACCEPTS fails
 *   the run: today none is accepted, over the 1360 probes the corpus
 *   carries (170 parameters x both bounds x both entry points x both modes).
 *
 *   NOT COVERED HERE. The undersized-output rejection class has no C
 *   expression at all -- TA_<N>_Open takes a bare output pointer and carries
 *   no length -- so it stays where it already is, in the Rust crate's
 *   stream_open_contract.rs. Rust also cannot host what IS here: its
 *   Compatibility is pub(crate) and pinned to Default, with a unit test
 *   asserting there is no setter, so no Rust probe can reach the Metastock
 *   seeding path at all.
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

/* Anything but these in a buffer after a non-success return is a write. Both
 * are values no indicator produces on this series, so a violation reads as a
 * value rather than as a bit pattern. */
#define OC_SENT_REAL  (-777.0)
#define OC_SENT_INT   (-777)

/* Pre-set outBegIdx/outNBElement, so "written" and "written as zero" are
 * distinguishable from each other. */
#define OC_SENT_IDX   (-999)

typedef struct {
   const char   *name;        /* without the TA_ prefix */
   int           metastock;   /* 1 = only violates under Metastock */
} OcKnownOpen;

/* Issue #389. Both are the same emitter shape: the transcribed batch prologue
 * writes the Metastock seed output, and only then does the body discover it
 * has no bar left to continue from. Asserted to still fail -- see the header
 * comment for why they are listed rather than fixed. */
static const OcKnownOpen ocKnownOpen[] = {
   { "RSI", 1 },
   { "CMO", 1 },
};
#define OC_NB_KNOWN_OPEN ((int)(sizeof(ocKnownOpen)/sizeof(ocKnownOpen[0])))
static int ocKnownOpenHit[OC_NB_KNOWN_OPEN];

/* The two modes, in the order the sweep runs them. */
static const struct { TA_Compatibility mode; const char *name; } ocCompat[] = {
   { TA_COMPATIBILITY_DEFAULT,   "DEFAULT"   },
   { TA_COMPATIBILITY_METASTOCK, "METASTOCK" },
};
#define OC_NB_COMPAT ((int)(sizeof(ocCompat)/sizeof(ocCompat[0])))

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
   long    nbSuccessChecked;  /* producing calls whose buffers must have moved */
   long    nbCountLeaked;     /* rejections leaving outNBElement non-zero */
   long    nbParamRejects;    /* leg B probes that were rejected */
   int     nbParamAccepted;   /* leg B probes that were NOT */
   char    leaked[OC_NB_KNOWN_OPEN + 4][96];   /* who, for the ceiling's report */
   int     nbViolation;       /* buffer writes on a rejection, known-open aside */
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

/* Index of the first output slot that moved, or -1 when none did. */
static int oc_first_moved( const OcCtx *c, const TA_StreamEntry *e )
{
   int o;
   for( o = 0; o < e->nbOutput; o++ )
   {
      if( e->outIsInt[o] )
      {
         if( memcmp( c->outInt[o], c->refInt, sizeof(c->refInt) ) != 0 ) return o;
      }
      else
      {
         if( memcmp( c->outReal[o], c->refReal, sizeof(c->refReal) ) != 0 ) return o;
      }
   }
   return -1;
}

/* The offset and value of the first moved element of one output, for the
 * report: "wrote something" is not actionable, "wrote 66.6667 at [0]" is. */
static void oc_report_write( const OcCtx *c, const TA_StreamEntry *e,
                             const char *compat, const char *what,
                             int rc, int begIdx, int nbElement, int slot )
{
   int k;
   printf( "  OPEN-CONTRACT TA_%s %s %s: retCode=%d wrote the caller's buffer"
           " (outBegIdx=%d outNBElement=%d)\n",
           e->name, compat, what, rc, begIdx, nbElement );
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
 * range" probe lands back INSIDE the range -- 58 of the 116 probes were
 * no-ops before this. Integers and enums keep the exact +/-1, which is the
 * tightest probe and exact at their magnitudes. */
static double oc_out_of_range( const TA_VOptSpec *spec, int above )
{
   if( spec->kind == TA_VOPT_REAL )
      return above ? spec->maxValue + fabs( spec->maxValue ) * 0.5 + 1.0
                   : spec->minValue - fabs( spec->minValue ) * 0.5 - 1.0;
   return above ? spec->maxValue + 1.0 : spec->minValue - 1.0;
}

static int oc_known_open_idx( const char *name, int metastock )
{
   int i;
   for( i = 0; i < OC_NB_KNOWN_OPEN; i++ )
      if( strcmp( ocKnownOpen[i].name, name ) == 0 &&
          ocKnownOpen[i].metastock == metastock )
         return i;
   return -1;
}

/* Judge one call that came back non-success. */
static void oc_judge_reject( OcCtx *c, const TA_StreamEntry *e, int compatIdx,
                            const char *what, int rc, int begIdx, int nbElement )
{
   int slot, known;

   c->nbRejectChecked++;
   if( nbElement != 0 && nbElement != OC_SENT_IDX )
   {
      if( c->nbCountLeaked < (long)( sizeof(c->leaked) / sizeof(c->leaked[0]) ) )
         snprintf( c->leaked[c->nbCountLeaked], sizeof(c->leaked[0]),
                   "TA_%s %s %s (outNBElement=%d)",
                   e->name, ocCompat[compatIdx].name, what, nbElement );
      c->nbCountLeaked++;
   }

   slot = oc_first_moved( c, e );
   if( slot < 0 )
      return;

   known = oc_known_open_idx( e->name, compatIdx == 1 );
   if( known >= 0 )
   {
      ocKnownOpenHit[known] = 1;
      return;
   }

   if( c->nbReported < 12 )
   {
      c->nbReported++;
      oc_report_write( c, e, ocCompat[compatIdx].name, what, rc, begIdx, nbElement, slot );
   }
   c->nbViolation++;
}

/* One function, one compatibility mode. Returns the shortest history that
 * produced a value, or -1 when none did inside OC_MAX_BARS. */
static int oc_sweep_one( OcCtx *c, const TA_StreamEntry *e, int compatIdx )
{
   const double *in[OC_MAX_IN];
   double       *outReal[OC_MAX_OUT];
   TA_Integer   *outInt[OC_MAX_OUT];
   double        opt[OC_MAX_OPT];
   int i, h, firstOk = -1;

   for( i = 0; i < e->nbInput; i++ )
      in[i] = oc_series( c, e->inputKind[i], i );
   /* Both lists, always: a mixed-type function's thunk dereferences a slot in
    * each, and a homogeneous one (void)s the list it does not use. */
   for( i = 0; i < OC_MAX_OUT; i++ )
   {
      outReal[i] = c->outReal[i];
      outInt[i]  = c->outInt[i];
   }
   for( i = 0; i < e->nbOptInput; i++ )
      opt[i] = e->optInput[i].defValue;

   /* ---- Leg A: the history ramp, from an empty one up. ---- */
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
         oc_judge_reject( c, e, compatIdx, "OpenAndFill", (int)rc, (int)begIdx, (int)nbElement );
      }
      else if( nbElement > 0 )
      {
         /* The positive control: this call MUST have moved the buffers the
          * rejections above were checked against. */
         c->nbSuccessChecked++;
         if( oc_first_moved( c, e ) < 0 )
         {
            printf( "  OPEN-CONTRACT TA_%s %s: OpenAndFill reported %d elements at"
                    " historyLen=%d and wrote nothing -- the sweep is checking"
                    " buffers this function does not write.\n",
                    e->name, ocCompat[compatIdx].name, (int)nbElement, h );
            return -2;
         }
         firstOk = h;
      }

      /* Open: its own one-slot sink per output, on the same history. */
      stream = NULL;
      oc_arm( c, e );
      rc = e->open( &stream, in, h, opt, outReal, outInt );
      if( stream ) e->close( stream );
      if( rc != TA_SUCCESS )
         oc_judge_reject( c, e, compatIdx, "Open", (int)rc, OC_SENT_IDX, OC_SENT_IDX );

      if( firstOk >= 0 )
         break;
   }

   if( firstOk < 0 )
      return -1;

   /* ---- Leg B: a parameter outside its range, on a history that otherwise
    * produces. A different rejection class, answered by the public frame
    * before the body runs -- which is exactly why it belongs here: it is the
    * class the one pre-existing test already covered, and having both is what
    * makes "which class can write" a measurement rather than a claim. ---- */
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
         opt[i] = ( side == 0 ) ? oc_out_of_range( &e->optInput[i], 0 )
                                : oc_out_of_range( &e->optInput[i], 1 );

         oc_arm( c, e );
         rc = e->openAndFill( &stream, in, firstOk, opt, &begIdx, &nbElement, outReal, outInt );
         if( stream ) e->close( stream );
         if( rc != TA_SUCCESS )
         {
            c->nbParamRejects++;
            oc_judge_reject( c, e, compatIdx, "OpenAndFill/param", (int)rc, (int)begIdx, (int)nbElement );
         }
         else
         {
            /* The probe is only a probe while it is out of range. Reported
             * here rather than silently skipped: a bound that stopped
             * rejecting turns this whole leg into a no-op, which is what it
             * already was for every real-valued parameter until the probe
             * value stopped being `min - 1`. */
            c->nbParamAccepted++;
            printf( "  OPEN-CONTRACT TA_%s %s: %s=%g is outside its declared"
                    " [%g, %g] and OpenAndFill accepted it.\n",
                    e->name, ocCompat[compatIdx].name, e->optInput[i].name, opt[i],
                    e->optInput[i].minValue, e->optInput[i].maxValue );
         }

         stream = NULL;
         oc_arm( c, e );
         rc = e->open( &stream, in, firstOk, opt, outReal, outInt );
         if( stream ) e->close( stream );
         if( rc != TA_SUCCESS )
         {
            c->nbParamRejects++;
            oc_judge_reject( c, e, compatIdx, "Open/param", (int)rc, OC_SENT_IDX, OC_SENT_IDX );
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
   int f, m, i, firstOk;
   ErrorNumber errNb = TA_TEST_PASS;

   (void)history;   /* The lengths are the subject here, so the series is local. */

   memset( &ctx, 0, sizeof(ctx) );
   memset( ocKnownOpenHit, 0, sizeof(ocKnownOpenHit) );
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
         errNb = TA_OPEN_CONTRACT_VACUOUS;
         goto done;
      }

      ctx.nbFunc++;
      for( m = 0; m < OC_NB_COMPAT; m++ )
      {
         TA_SetCompatibility( ocCompat[m].mode );
         firstOk = oc_sweep_one( &ctx, e, m );
         if( firstOk == -2 )
         {
            errNb = TA_OPEN_CONTRACT_VACUOUS;
            goto done;
         }
         if( firstOk < 0 )
         {
            printf( "\nFail: TA_%s %s produced no value at any history up to %d bars,"
                    " so the sweep never reached its rejection boundary.\n",
                    e->name, ocCompat[m].name, OC_MAX_BARS );
            errNb = TA_OPEN_CONTRACT_VACUOUS;
            goto done;
         }
      }
   }

   if( ctx.nbViolation > 0 )
   {
      printf( "\nFail: %d Open/OpenAndFill rejection%s wrote the caller's output"
              " buffer (issue #389).\n",
              ctx.nbViolation, ctx.nbViolation == 1 ? "" : "s" );
      errNb = TA_OPEN_CONTRACT_WROTE;
      goto done;
   }

   /* A listed row that stopped violating is the fix landing, and the entry has
    * to come out with it -- otherwise the list slowly becomes a description of
    * defects that no longer exist. */
   for( i = 0; i < OC_NB_KNOWN_OPEN; i++ )
      if( !ocKnownOpenHit[i] )
      {
         printf( "\nFail: TA_%s is listed as a known open write-on-rejection (#389)"
                 " under %s, but the sweep no longer sees it.\n"
                 "      If that is the fix landing, delete the entry from"
                 " ocKnownOpen so the row stops claiming an open defect.\n",
                 ocKnownOpen[i].name,
                 ocKnownOpen[i].metastock ? "Metastock" : "the default mode" );
         errNb = TA_OPEN_CONTRACT_STALE;
         goto done;
      }

   /* The count, as a CEILING rather than an assertion. Over 201 functions x 2
    * modes exactly two rejections come back with outNBElement non-zero, and
    * they are the two rows above -- so the ceiling is the row count itself and
    * drops with the list rather than being a second constant to forget. It is
    * a ratchet, not a rule: the spec calls the indices undefined on a
    * non-success return, so a third one is reported as a new instance of the
    * trap for someone to judge, not as a violated contract. */
   if( ctx.nbCountLeaked > OC_NB_KNOWN_OPEN )
   {
      printf( "\nFail: %ld rejections reported a non-zero outNBElement, above the"
              " %d this corpus is known to have (#389):\n",
              ctx.nbCountLeaked, OC_NB_KNOWN_OPEN );
      for( i = 0; i < (int)( sizeof(ctx.leaked) / sizeof(ctx.leaked[0]) ) &&
                  i < (int)ctx.nbCountLeaked; i++ )
         printf( "      %s\n", ctx.leaked[i] );
      errNb = TA_OPEN_CONTRACT_WROTE;
      goto done;
   }

   if( ctx.nbParamAccepted > 0 )
   {
      printf( "\nFail: %d out-of-range parameter probe%s were accepted by the"
              " streaming open tier.\n",
              ctx.nbParamAccepted, ctx.nbParamAccepted == 1 ? "" : "s" );
      errNb = TA_OPEN_CONTRACT_VACUOUS;
      goto done;
   }

   /* Vacuity floors. Each has been a real failure mode of a sweep in this
    * suite: a corpus that shrank to nothing, a leg whose rejections all became
    * successes, and a control arm that stopped comparing. */
   if( ctx.nbFunc < TA_STREAM_TABLE_SIZE || ctx.nbRejectChecked == 0 ||
       ctx.nbParamRejects == 0 ||
       ctx.nbSuccessChecked < ctx.nbFunc * OC_NB_COMPAT )
   {
      printf( "\nFail: open-contract sweep ran thin -- %d functions,"
              " %ld rejections checked (%ld of them parameter probes),"
              " %ld producing calls controlled"
              " (expected %d functions and at least %d controls).\n",
              ctx.nbFunc, ctx.nbRejectChecked, ctx.nbParamRejects,
              ctx.nbSuccessChecked,
              TA_STREAM_TABLE_SIZE, TA_STREAM_TABLE_SIZE * OC_NB_COMPAT );
      errNb = TA_OPEN_CONTRACT_VACUOUS;
      goto done;
   }

done:
   TA_SetCompatibility( TA_COMPATIBILITY_DEFAULT );
   return errNb;
}
