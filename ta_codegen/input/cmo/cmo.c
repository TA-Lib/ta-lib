/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  BT       Barry Tsung
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  112605 MF      Initial version.
 *  021806 MF,BT   Fix #1434450 reported by BT.
 *  082326 MF,CC   Fix #253. Test the gain+loss total exactly instead of against
 *                 the fixed TA_IS_ZERO band, which zeroed the oscillator for any
 *                 instrument quoted small enough to fall under it.
 *  091326 MF,CC   #411 Wilder step without a divide or a branch.
 */

int cmo_lookback(int optInTimePeriod)
{
   int retValue;

   retValue = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_CMO);

   return retValue;
}

TA_RetCode cmo(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;

   int today, lookbackTotal, i;
   double gainDelta;
   double prevGain, prevLoss, invPeriod, prevValue;
   double tempValue1, tempValue2;

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = cmo_lookback( optInTimePeriod );

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
      return TA_SUCCESS;

   outIdx = 0; /* Index into the output. */

   /* Trap special case where the period is '1'.
    * In that case, just copy the input into the
    * output for the requested range (as-is !)
    */
   if( optInTimePeriod == 1 )
   {
      *outBegIdx = startIdx;
      i = (endIdx-startIdx)+1;
      *outNBElement = i;
      /* Element loop, not a block copy: the C single-precision variant reads a
       * float array, so a double-sized byte copy would reinterpret and
       * over-read it (#137). Forward order keeps the in-place case correct (#94). */
      today = startIdx;
      for( outIdx = 0; outIdx < i; outIdx++ )
         outReal[outIdx] = inReal[today++];
      return TA_SUCCESS;
   }

   /* The declaration order above sets invPeriod's place in the stream state,
    * and that place is load-bearing: a layout that lets Update load it paired
    * with a field the previous bar stored stalls every call. Re-measure Update
    * in C and Rust before reordering those declarations.
    */
   invPeriod = 1.0 / (double)optInTimePeriod;

   /* Accumulate Wilder's "Average Gain" and "Average Loss"
    * among the initial period.
    */
   today = startIdx-lookbackTotal;
   prevValue = inReal[today];

   prevGain = 0.0;
   prevLoss = 0.0;
   today++;
   for( i=optInTimePeriod; i > 0; i-- )
   {
      tempValue1 = inReal[today++];
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      gainDelta = tempValue2 > 0.0 ? tempValue2 : 0.0;
      prevGain += gainDelta;
      prevLoss += gainDelta - tempValue2;
   }

   /* Subsequent prevLoss and prevGain are smoothed
    * using the previous values (Wilder's approach):
    *    prev += (today - prev) / 'period'
    * gainDelta - tempValue2 is the exact loss delta for every finite
    * tempValue2, so both accumulators step without a branch. Keep the step a
    * difference of two products: prev - (prev - today)*k lets the zero arm fold
    * to prev, which gcc compiles back into a branch, and prev + (today - prev)*k
    * becomes a fused multiply-add.
    */
   prevLoss /= optInTimePeriod;
   prevGain /= optInTimePeriod;

   /* Often documentation present the RSI calculation as follow:
    *    RSI = 100 - (100 / 1 + (prevGain/prevLoss))
    *
    * The following is equivalent:
    *    RSI = 100 * (prevGain/(prevGain+prevLoss))
    *
    * The second equation is used here for speed optimization.
    *
    * prevGain+prevLoss is a sum of non-negative magnitudes, so it is zero only
    * when every change since the seed was exactly zero -- test it exactly, never
    * against a fixed band. A gain carries the quote unit, so a constant put
    * against it zeroes a healthy oscillator for an instrument quoted below it
    * (issue #253).
    */
   if( today > startIdx )
   {
      tempValue1 = prevGain+prevLoss;
      if( tempValue1 > 0.0 )
         outReal[outIdx++] = 100.0*((prevGain-prevLoss)/tempValue1);
      else
         outReal[outIdx++] = 0.0;
   }
   else
   {
      /* Skip the unstable period. Do the processing
       * but do not write it in the output.
       */
      while( today < startIdx )
      {
         tempValue1 = inReal[today];
         tempValue2 = tempValue1 - prevValue;
         prevValue  = tempValue1;

         gainDelta = tempValue2 > 0.0 ? tempValue2 : 0.0;
         prevGain += gainDelta*invPeriod - prevGain*invPeriod;
         prevLoss += (gainDelta - tempValue2)*invPeriod - prevLoss*invPeriod;

         today++;
      }
   }

   /* Unstable period skipped... now continue
    * processing if needed.
    */
   while( today <= endIdx )
   {
      tempValue1 = inReal[today++];
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;

      gainDelta = tempValue2 > 0.0 ? tempValue2 : 0.0;
      prevGain += gainDelta*invPeriod - prevGain*invPeriod;
      prevLoss += (gainDelta - tempValue2)*invPeriod - prevLoss*invPeriod;
      tempValue1 = prevGain+prevLoss;
      if( tempValue1 > 0.0 )
         outReal[outIdx++] = 100.0*((prevGain-prevLoss)/tempValue1);
      else
         outReal[outIdx++] = 0.0;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
