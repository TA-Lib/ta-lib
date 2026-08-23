/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  BT       BobTrader (TADoc.org forum user).
 *  MW       github @mw66
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  062704 MF    Prevent divide by zero.
 *  121705 MF    Java port related changes.
 *  060907 MF,BT Fix #1727704. MFI logic bug when no price movement
 *  070726 MW,CC Fix #4. MFI has no unstable period; drop the unstable-period
 *               term (and the now-dead unstable-skip loop) so
 *               TA_SetUnstablePeriod is a no-op for it.
 *  071026 MF,CC Fix #107. Classify money-flow direction with a magnitude-scaled
 *               dead-zone (TA_IS_ZERO_SCALED), not an exact sign test, so an
 *               epsilon-flat typical price is "no movement", not a spurious move.
 *  082326 MF,CC Fix #244. Detect an empty window by counting bars, not by
 *               testing the money-flow sum against a literal 1.0; classify
 *               branchlessly; clamp the emitted ratio into [0,100].
 */

int mfi_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode mfi(int startIdx, int endIdx,
   const double inHigh[],
   const double inLow[],
   const double inClose[],
   const double inVolume[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double posSumMF, negSumMF, prevValue;
   double tempValue1, tempValue2, tempValue3;
   double moneyFlow, posFlow, negFlow, posClamped;
   int lookbackTotal, outIdx, i, today, nullRun;

   typedef struct { double positive; double negative; } MoneyFlow;
   CIRCBUF_PROLOG_CLASS( mflow, MoneyFlow, 50 ); /* Id, Type, Static Size */

   CIRCBUF_INIT_CLASS( mflow, MoneyFlow, optInTimePeriod );

   *outBegIdx = 0;
   *outNBElement = 0;

   /* Adjust startIdx to account for the lookback period. */
   lookbackTotal = optInTimePeriod;

   if( startIdx < lookbackTotal )
      startIdx = lookbackTotal;

   /* Make sure there is still something to evaluate. */
   if( startIdx > endIdx )
   {
      CIRCBUF_DESTROY(mflow);
      return TA_SUCCESS;
   }

   outIdx = 0; /* Index into the output. */

   /* Accumulate the positive and negative money flow
    * among the initial period.
    */
   today = startIdx-lookbackTotal;
   prevValue = (inHigh[today]+inLow[today]+inClose[today])/3.0;

   posSumMF = 0.0;
   negSumMF = 0.0;
   /* Consecutive bars that put nothing into the window, counted so that an
    * empty window can be recognized exactly (issue #244).  The running sums
    * cannot answer that question themselves: they are maintained by
    * add-then-subtract, so when the window empties they hold rounding
    * residue of arbitrary sign, not zero.
    */
   nullRun = 0;
   today++;
   for( i=optInTimePeriod; i > 0; i-- )
   {
      tempValue1 = (inHigh[today]+inLow[today]+inClose[today])/3.0;
      tempValue2 = tempValue1 - prevValue;
      /* Dead-zone scaled to the two typical prices being compared (issue #107).
       * Captured before prevValue/tempValue1 are repurposed below. */
      tempValue3 = fabs(tempValue1) + fabs(prevValue);
      prevValue  = tempValue1;
      tempValue1 *= inVolume[today++];
      /* This bar's money flow, and its split into the positive and negative
       * sums.  Selects rather than a three-arm branch: the direction of a
       * price move is a coin flip, so that branch mispredicted on roughly
       * every other bar and dominated the cost of the function.  Adding the
       * unused side's 0.0 to a sum is an exact no-op, so this reproduces the
       * branching form bit for bit.
       *
       * The three quantities are named rather than folded back into
       * tempValue1/2 deliberately, at a known cost: every local in a step body
       * becomes a field of the stream handle, so each name is another store
       * per bar (~10% of MFI's streaming Update, +32 handle bytes).  That is
       * the generator's to fix -- issue #252, which counts 436 such fields
       * across 125 streaming functions -- not something to obfuscate an
       * indicator body over.
       */
      moneyFlow = TA_IS_ZERO_SCALED(tempValue2,tempValue3) ? 0.0 : tempValue1;
      posFlow   = tempValue2 < 0.0 ? 0.0 : moneyFlow;
      negFlow   = tempValue2 < 0.0 ? moneyFlow : 0.0;

      mflow_positive[mflow_Idx] = posFlow;
      mflow_negative[mflow_Idx] = negFlow;
      posSumMF += posFlow;
      negSumMF += negFlow;

      /* A bar contributes nothing when the typical price did not move, or
       * when it moved but carried no volume.  Once a whole period of those
       * has gone by, every slot of the ring is 0.0, so the sums are known to
       * be exactly zero and the residue can be dropped.
       */
      nullRun = moneyFlow == 0.0 ? nullRun+1 : 0;
      if( nullRun >= optInTimePeriod )
      {
         nullRun  = optInTimePeriod;
         posSumMF = 0.0;
         negSumMF = 0.0;
      }

      CIRCBUF_NEXT(mflow);
   }

   /* The following two equations are equivalent:
    *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
    *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
    * The second equation is used here for speed optimization.
    *
    * Both sums are non-negative, so the total is zero only for a window that
    * received no money flow at all -- 0/0, reported as 0.0.  The test is on
    * the total itself, not on a fixed threshold: money flow is a price times
    * a volume, so any constant compared against it is a constant in some
    * arbitrary unit, and would zero a healthy index for any instrument
    * quoted small enough to fall under it (issue #244).
    *
    * Clamping the numerator into [0,total] keeps the result inside the
    * documented 0-100 range: the sums drift by a few ulp as the window
    * slides, and a sum whose true value is near zero can drift negative.
    */
   /* The first full window is complete: emit its output for startIdx here,
    * then slide the window over the remaining bars below.
    */
   tempValue1 = posSumMF+negSumMF;
   posClamped = posSumMF < 0.0 ? 0.0 : (posSumMF > tempValue1 ? tempValue1 : posSumMF);
   if( tempValue1 <= 0.0 )
      outReal[outIdx++] = 0.0;
   else
      outReal[outIdx++] = 100.0*(posClamped/tempValue1);

   /* Now continue processing the remaining bars. */
   while( today <= endIdx )
   {
      posSumMF -= mflow_positive[mflow_Idx];
      negSumMF -= mflow_negative[mflow_Idx];

      tempValue1 = (inHigh[today]+inLow[today]+inClose[today])/3.0;
      tempValue2 = tempValue1 - prevValue;
      /* Dead-zone scaled to the two typical prices being compared (issue #107).
       * Captured before prevValue/tempValue1 are repurposed below. */
      tempValue3 = fabs(tempValue1) + fabs(prevValue);
      prevValue  = tempValue1;
      tempValue1 *= inVolume[today++];

      moneyFlow = TA_IS_ZERO_SCALED(tempValue2,tempValue3) ? 0.0 : tempValue1;
      posFlow   = tempValue2 < 0.0 ? 0.0 : moneyFlow;
      negFlow   = tempValue2 < 0.0 ? moneyFlow : 0.0;

      mflow_positive[mflow_Idx] = posFlow;
      mflow_negative[mflow_Idx] = negFlow;
      posSumMF += posFlow;
      negSumMF += negFlow;

      nullRun = moneyFlow == 0.0 ? nullRun+1 : 0;
      if( nullRun >= optInTimePeriod )
      {
         nullRun  = optInTimePeriod;
         posSumMF = 0.0;
         negSumMF = 0.0;
      }

      tempValue1 = posSumMF+negSumMF;
      posClamped = posSumMF < 0.0 ? 0.0 : (posSumMF > tempValue1 ? tempValue1 : posSumMF);
      if( tempValue1 <= 0.0 )
         outReal[outIdx++] = 0.0;
      else
         outReal[outIdx++] = 100.0*(posClamped/tempValue1);

      CIRCBUF_NEXT(mflow);
   }

   CIRCBUF_DESTROY(mflow);

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
