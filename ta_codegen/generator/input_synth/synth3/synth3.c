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
 *
 * SYNTHETIC GATE FUNCTION - never shipped (see input_synth/README.md).
 * Regression driver for issue #158: an integer local must be typed by its
 * DECLARATION, never by its name. Every local below is named so that the
 * Rust backend's naming heuristics get it wrong if they are consulted:
 * `k` is on that backend's hard-coded float-name list (it is EMA's k
 * factor), and `slot` / `lag` / `barVal` are on no list at all. The
 * lookback body carries the same shape, because that context used to be
 * rendered with no type information whatsoever.
 *
 * Every value here is bar-local, so the function is not path-dependent and
 * batch and streaming agree bar for bar.
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
   int outIdx, i, k, slot, lag;
   double barVal;

   /* Unsigned index domain: never negative, so it stays usize in Rust and
    * the i32 parameter has to be cast INTO it. */
   slot = optInTimePeriod;
   slot += optInTimePeriod;

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

      outInteger[outIdx] = k & 65535;
      outIdx++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
