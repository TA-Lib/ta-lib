/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF,CC    Mario Fortier, Claude Code
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  080326 MF,CC Creation (synthetic gate: integer local typing, #158)
 *  080426 MF,CC Int-array element typing and cast placement (#159, #163)
 *  080426 MF,CC Signed locals inside expressions (#165)
 *
 * SYNTHETIC GATE FUNCTION - never shipped; see input_synth/README.md.
 * What this fixture covers, and what would silently reduce that coverage,
 * is in synth3.md — one copy, so there is one thing to keep true.
 */

int synth3_lookback(int optInTimePeriod)
{
   int k, w;

   /* An integer local compound-assigned an integer optional parameter,
    * inside a LOOKBACK body. This rendered `k += ((optInTimePeriod) as f64)`
    * onto a `usize` declaration before #158. */
   k = optInTimePeriod;
   k += optInTimePeriod;
   k -= optInTimePeriod;

   /* A SIGNED local in a lookback body: the `< 0` compare elects it, so the
    * declaration emitter must render it i32 rather than usize. No shipped
    * lookback has one, so this fixture is that branch's only coverage. */
   w = 0 - optInTimePeriod;
   if( w < 0 )
      w += optInTimePeriod;

   return (k - optInTimePeriod) + w;   /* always 0 */
}

TA_RetCode synth3( int    startIdx,
                   int    endIdx,
                   const double inReal[],
                   int    optInTimePeriod,
                   int   *outBegIdx,
                   int   *outNBElement,
                   int    outInteger[] )
{
   int outIdx, i, k, slot, lag, head, hits;
   int ring[4];
   double barVal;

   /* Unsigned index domain: never negative, so it stays usize in Rust and
    * the i32 parameter has to be cast INTO it. */
   slot = optInTimePeriod;
   slot += optInTimePeriod;

   /* Written before every read below, but seeded anyway so no slot is ever
    * read uninitialised if this body is restructured. */
   ring[0] = 0;
   ring[1] = 0;
   ring[2] = 0;
   ring[3] = 0;

   outIdx = 0;
   for( i = startIdx; i <= endIdx; i++ )
   {
      /* Fold every bar outside [0, 1e6) to zero, so the (int) cast is in
       * range and non-negative in all four backends (see synth1/synth2). */
      barVal = inReal[i];
      if( !(barVal > 0.0) || !(barVal < 1000000.0) )
         barVal = 0.0;

      /* Whole-RHS (int) cast of a double: a SIGNED local, per #160. */
      lag = (int)barVal;

      /* "If the index went negative, wrap it by one period" — the
       * ring-buffer bookkeeping #158 was found in. The `< 0` compare is
       * what makes `k` signed, and the compound assign of the integer
       * parameter must then take no cast at all. */
      k = 0 - optInTimePeriod;
      if( k < 0 )
         k += optInTimePeriod;

      /* Signed target, unsigned index-domain right-hand side: the mirror
       * cast, which had no branch at all before #158. */
      k += slot;

      /* Signed target, signed right-hand side: no cast. */
      k += lag;

      /* #159 / #163: an int-ARRAY element against the unsigned index domain.
       * `slot` is 2*optInTimePeriod and the stored value is in [0,7], so at
       * the low end of the parameter range every comparison below straddles
       * rather than answering the same way on every bar.
       *
       * Both operand positions and every arithmetic operator the predicate
       * accepts are covered, because the first cut of #163 fired on some and
       * not others: it accepted an integer LITERAL as the other operand but
       * not an integer PARAMETER, so `ring[head] + optInTimePeriod` was the
       * one that still did not compile.
       */
      /* #165A: a usize subscript assigned a BinOp over a SIGNED local. The
       * single-variable form `head = lag;` always took its cast; the masked
       * form took none, because the ctx-aware i32 predicate folded over the
       * four arithmetic operators only. */
      head = lag & 3;
      ring[head] = lag & 7;

      hits = 0;
      if( ring[head] < slot )                   /* bare subscript (#159) */
         hits += 1;
      if( ring[head] + 1 < slot )               /* + literal */
         hits += 2;
      if( ring[head] + optInTimePeriod < slot ) /* + integer PARAMETER */
         hits += 4;
      if( slot < ring[head] + 1 )               /* mirror: compound on the right */
         hits += 8;
      if( ring[head] * 2 < slot )               /* * */
         hits += 16;
      if( ring[head] << 1 < slot )              /* shift, non-negative operand */
         hits += 32;
      if( (ring[head] | 1) < slot )             /* bitwise */
         hits += 64;
      if( ring[head] + 1 <= slot )              /* `<=` parses bare; typing must still fire */
         hits += 128;

      /*  */
      /* A comment whose content reduces to NOTHING used to abort `generate`
       * outright: `block_comment` indexed `lines[1..]` on an empty slice. The
       * two spellings are the empty comment above and the bare `*` labelling
       * the multiply case, whose lone asterisk is eaten as a continuation
       * prefix. Both are ordinary C. They live in a fixture rather than only
       * in a unit test because the crash is a `generate` abort, and this is
       * the gate that runs `generate` for all four languages.
       */

      /* #165B: an i32 local and a usize local in one bitwise expression.
       * `k` is elected signed (it is compared `< 0` above), `hits` is not, so
       * this used to emit `i32 | usize`. It is reconciled by the SAME sentinel
       * arm that #165A unlocked, not by a second rule — which is why the two
       * halves of that issue had one fix. Packing rather than folding also
       * keeps k and hits independently visible in the output. */
      outInteger[outIdx] = (k & 65535) | (hits << 16);
      outIdx++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
