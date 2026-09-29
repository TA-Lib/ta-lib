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
 *  092926 MF,CC  Creation (#465).
 */

int emv_lookback(int optInTimePeriod, double optInVolumeDivisor)
{
   /* One bar is consumed forming the first midpoint change, then the SMA's own
    * warm-up on top:
    *    1 + sma_lookback(optInTimePeriod) = 1 + (optInTimePeriod - 1)
    * which is efi_lookback's derivation with a finite window in place of the
    * EMA, so there is no unstable period to add.
    *
    * The divisor scales the output and cannot move the first valid bar.
    */
   (void)optInVolumeDivisor;
   return optInTimePeriod;
}

TA_RetCode emv(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inVolume[],
   int optInTimePeriod,
   double optInVolumeDivisor,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double prevMid, mid, range, boxRatio, raw, sumRaw, tempReal;
   int lookbackTotal, outIdx, i, today;

   /* The window of raw values is carried here rather than recomputed from the
    * inputs at the trailing index: once a bar has been consumed it is never
    * read again, which is what makes outReal safe to alias any input
    * (cmf.c:32-38).
    */
   CIRCBUF_PROLOG(rawRing,double,50);

   *outBegIdx = 0;
   *outNBElement = 0;

   lookbackTotal = emv_lookback( optInTimePeriod, optInVolumeDivisor );

   /* Move up the start index if there is not enough initial data. */
   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   CIRCBUF_INIT( rawRing, double, optInTimePeriod );

   /* The running sum is seeded with the window's first optInTimePeriod-1 raw
    * values and each output bar then adds its own before dividing, which is
    * sma.c:57-81's order applied to the raw series rather than to an input
    * array. At optInTimePeriod 1 the seed loop does not run and the body
    * reduces to 0 + raw, then raw - raw, so the output is the raw kernel bit
    * for bit.
    */
   today = startIdx - lookbackTotal + 1;
   prevMid = (inHigh[today-1] + inLow[today-1]) / 2.0;
   sumRaw = 0.0;
   i = optInTimePeriod - 1;
   while( i-- > 0 )
   {
      mid = (inHigh[today] + inLow[today]) / 2.0;
      range = inHigh[today] - inLow[today];

      /* A bar with no volume, or no range, has no boxRatio to divide by. The tests
       * are exact rather than TA_IS_ZERO's band: a near-zero range is a real
       * boxRatio, and the result has to be a plain 0.0 with no sign, as in
       * marketfi.c and roc.c:85-88.
       *
       * The boxRatio itself is tested, not just its operands. optInVolumeDivisor
       * goes up to TA_REAL_MAX, so the scaling can reach zero from a volume
       * that is not zero: at inVolume 5e-324 and a divisor of 2 -- both inside
       * their declared ranges -- inVolume/optInVolumeDivisor rounds to 0.0 and
       * the quotient below would be infinite. Testing the operands alone let
       * that through.
       */
      if( inVolume[today] != 0.0 && range != 0.0 )
      {
         boxRatio = (inVolume[today] / optInVolumeDivisor) / range;
         if( boxRatio != 0.0 )
            raw = (mid - prevMid) / boxRatio;
         else
            raw = 0.0;
      }
      else
         raw = 0.0;

      /* The midpoint moves on even for a guarded bar: the next bar's change is
       * measured from the bar immediately before it, never from the last bar
       * that happened to produce a value.
       */
      prevMid = mid;

      rawRing[rawRing_Idx] = raw;
      sumRaw += raw;
      CIRCBUF_NEXT(rawRing);
      today = today + 1;
   }

   outIdx = 0;
   while( today <= endIdx )
   {
      mid = (inHigh[today] + inLow[today]) / 2.0;
      range = inHigh[today] - inLow[today];
      if( inVolume[today] != 0.0 && range != 0.0 )
      {
         boxRatio = (inVolume[today] / optInVolumeDivisor) / range;
         if( boxRatio != 0.0 )
            raw = (mid - prevMid) / boxRatio;
         else
            raw = 0.0;
      }
      else
         raw = 0.0;
      prevMid = mid;

      /* Today's raw value enters the window at its own slot, and the bar
       * leaving the window is read only after the ring has advanced onto it.
       * Every input read for this bar is done above, so the store into
       * outReal is safe when the caller aliases it over an input.
       */
      rawRing[rawRing_Idx] = raw;
      sumRaw += raw;
      tempReal = sumRaw;
      CIRCBUF_NEXT(rawRing);
      sumRaw -= rawRing[rawRing_Idx];

      outReal[outIdx] = tempReal / (double)optInTimePeriod;
      outIdx = outIdx + 1;
      today = today + 1;
   }

   CIRCBUF_DESTROY(rawRing);

   *outNBElement = outIdx;
   *outBegIdx    = startIdx;

   return TA_SUCCESS;
}
