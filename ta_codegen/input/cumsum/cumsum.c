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
 *  090526 KL     First version (issue #372).
 */

int cumsum_lookback(void)
{
   return 0;
}

TA_RetCode cumsum(int startIdx, int endIdx,
   const double inReal[],
   int *outBegIdx,
   int *outNBElement,
   double outReal[])
{
   double total;
   int i, outIdx;

   /* Running total from the ANCHOR bar forward: the accumulator re-seeds at
    * startIdx, exactly as every shipped path-dependent accumulator does
    * (ad.c anchors at startIdx with ad = 0.0, obv.c seeds at
    * inVolume[startIdx]) -- TA_CUMSUM(3, 7, x)[0] is x[3], NOT sum(x[0..3]).
    * "From bar 0" could only mean "from the start of whatever buffer the
    * caller passed", under which a composed call handed inReal + off silently
    * changes answer; the path_dependent flag exists to declare this class
    * (issue #372). Left-to-right, one double, no compensation -- ad.c's own
    * plain += convention, and what both external oracles compute bit-exactly.
    *
    * Two statements, not outReal[outIdx] = (total += ...): a value-producing
    * compound assignment appears in zero input files, and Rust has no
    * assignment expression, so the one-liner cannot lower to a required
    * backend.
    */
   total = 0.0;

   for( i = startIdx, outIdx = 0; i <= endIdx; i++, outIdx++ )
   {
      total += inReal[i];
      outReal[outIdx] = total;
   }

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
