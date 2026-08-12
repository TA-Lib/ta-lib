/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version.
 *
 */

int marketfi_lookback(void)
{
   /* Each output depends only on its own bar, so nothing is consumed
    * before the first one can be produced.
    */
   return 0;
}

TA_RetCode marketfi(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inVolume[],
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx, i;

   /* Bill Williams' Market Facilitation Index: the price range a bar
    * travelled per unit of volume traded, i.e. how much movement the
    * market "facilitated" per tick.
    *
    *      MARKETFI = ( High - Low ) / Volume
    *
    * Stateless and per-bar: no seeding, no smoothing, no accumulator and
    * no unstable period, so the output for a bar never depends on where
    * the caller started the range.
    *
    * Retail material often abbreviates this "MFI" or "BW MFI". TA_MFI is
    * already the Money Flow Index, so this carries the name Tulip and
    * pandas-ta-classic use.
    */
   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
   {
      /* A zero-volume bar would divide by zero. Neither reference guards
       * it -- they emit +/-Inf, or NaN when the range is zero too -- but
       * issue #112 settled that a successful call never emits NaN or Inf,
       * so an untraded bar facilitated no movement and reports 0.
       *
       * The comparison is an exact != 0.0 rather than TA_IS_ZERO, whose
       * 1e-14 band is an absolute threshold and meaningless against an
       * unbounded volume scale. Same reasoning as the prevClose guard in
       * ta_codegen/input/nvi/nvi.c.
       */
      if( inVolume[i] != 0.0 )
         outReal[outIdx++] = (inHigh[i] - inLow[i]) / inVolume[i];
      else
         outReal[outIdx++] = 0.0;
   }

   *outBegIdx    = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
