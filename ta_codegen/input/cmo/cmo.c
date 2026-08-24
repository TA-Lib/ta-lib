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
 */

int cmo_lookback(int optInTimePeriod)
{
   int retValue;

   retValue = optInTimePeriod + TA_GetUnstablePeriod(TA_FUNC_UNST_CMO);
   if( TA_GetCompatibility() == TA_COMPATIBILITY_METASTOCK )
      retValue--;

   return retValue;
}

TA_RetCode cmo(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int outIdx;

   int today, lookbackTotal, unstablePeriod, i;
   double prevGain, prevLoss, prevValue, savePrevValue;
   double tempValue1, tempValue2, tempValue3, tempValue4;

   /* CMO calculation is mostly identical to RSI.
    *
    * The only difference is in the last step of calculation:
    *
    *   RSI = gain / (gain+loss)
    *   CMO = (gain-loss) / (gain+loss)
    *
    * See the RSI function for potentially some more info
    * on this algo.
    */

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

   /* Accumulate Wilder's "Average Gain" and "Average Loss"
    * among the initial period.
    */
   today = startIdx-lookbackTotal;
   prevValue = inReal[today];

   unstablePeriod = TA_GetUnstablePeriod(TA_FUNC_UNST_CMO);

   /* If there is no unstable period,
    * calculate the 'additional' initial
    * price bar who is particuliar to
    * metastock.
    * If there is an unstable period,
    * no need to calculate since this
    * first value will be surely skip.
    */
   if( (unstablePeriod == 0) &&
      (TA_GetCompatibility() == TA_COMPATIBILITY_METASTOCK))
   {
      /* Preserve prevValue because it may get
       * overwritten by the output.
       *(because output ptr could be the same as input ptr).
       */
      savePrevValue = prevValue;

      /* No unstable period, so must calculate first output
       * particular to Metastock.
       * (Metastock re-use the first price bar, so there
       *  is no loss/gain at first. Beats me why they
       *  are doing all this).
       */
      prevGain = 0.0;
      prevLoss = 0.0;
      for( i=optInTimePeriod; i > 0; i-- )
      {
         tempValue1 = inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue  = tempValue1;
         if( tempValue2 < 0 )
            prevLoss -= tempValue2;
         else
            prevGain += tempValue2;
      }

      tempValue1 = prevLoss/optInTimePeriod;
      tempValue2 = prevGain/optInTimePeriod;
      tempValue3 = tempValue2-tempValue1;
      tempValue4 = tempValue1+tempValue2;

      /* Write the output.
       *
       * Both halves are averages of non-negative magnitudes, so the total is
       * zero only when every change since the seed was exactly zero -- test it
       * exactly, do not compare it to a fixed band.  A gain carries the quote
       * unit, so any constant put against it is a constant in some arbitrary
       * unit, and zeroes a healthy oscillator for an instrument quoted below it
       * (issue #253).  Wilder's smoothing only ever adds non-negative terms, so
       * unlike a sliding sum this total cannot hold cancellation residue.
       */
      if( tempValue4 > 0.0 )
         outReal[outIdx++] = 100*(tempValue3/tempValue4);
      else
         outReal[outIdx++] = 0.0;

      /* Are we done? */
      if( today > endIdx )
      {
         *outBegIdx    = startIdx;
         *outNBElement = outIdx;
         return TA_SUCCESS;
      }

      /* Start over for the next price bar. */
      today -= optInTimePeriod;
      prevValue = savePrevValue;
   }

   /* Remaining of the processing is identical
    * for both Classic calculation and Metastock.
    */
   prevGain = 0.0;
   prevLoss = 0.0;
   today++;
   for( i=optInTimePeriod; i > 0; i-- )
   {
      tempValue1 = inReal[today++];
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      if( tempValue2 < 0 )
         prevLoss -= tempValue2;
      else
         prevGain += tempValue2;
   }

   /* Subsequent prevLoss and prevGain are smoothed
    * using the previous values (Wilder's approach).
    *  1) Multiply the previous by 'period-1'.
    *  2) Add today value.
    *  3) Divide by 'period'.
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

         prevLoss *= (optInTimePeriod-1);
         prevGain *= (optInTimePeriod-1);
         if( tempValue2 < 0 )
            prevLoss -= tempValue2;
         else
            prevGain += tempValue2;

         prevLoss /= optInTimePeriod;
         prevGain /= optInTimePeriod;

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

      prevLoss *= (optInTimePeriod-1);
      prevGain *= (optInTimePeriod-1);
      if( tempValue2 < 0 )
         prevLoss -= tempValue2;
      else
         prevGain += tempValue2;

      prevLoss /= optInTimePeriod;
      prevGain /= optInTimePeriod;
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
