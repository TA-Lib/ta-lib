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
 */

int mfi_lookback(int optInTimePeriod)
{
   return optInTimePeriod;
}

TA_RetCode mfi(int startIdx, int endIdx, const double inHigh[], const double inLow[], const double inClose[], const double inVolume[], int optInTimePeriod, int *outBegIdx, int *outNBElement, double outReal[])
{
   double posSumMF, negSumMF, prevValue;
   double tempValue1, tempValue2;
   int lookbackTotal, outIdx, i, today;

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
   today++;
   for( i=optInTimePeriod; i > 0; i-- )
   {
      tempValue1 = (inHigh[today]+inLow[today]+inClose[today])/3.0;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      tempValue1 *= inVolume[today++];
      if( tempValue2 < 0 )
      {
         mflow_negative[mflow_Idx] = tempValue1;
         negSumMF += tempValue1;
         mflow_positive[mflow_Idx] = 0.0;
      }
      else if( tempValue2 > 0 )
      {
         mflow_positive[mflow_Idx] = tempValue1;
         posSumMF += tempValue1;
         mflow_negative[mflow_Idx] = 0.0;
      }
      else
      {
         mflow_positive[mflow_Idx] = 0.0;
         mflow_negative[mflow_Idx] = 0.0;
      }

      CIRCBUF_NEXT(mflow);
   }

   /* The following two equations are equivalent:
    *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
    *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
    * The second equation is used here for speed optimization.
    */
   /* The first full window is complete: emit its output for startIdx here,
    * then slide the window over the remaining bars below.
    */
   tempValue1 = posSumMF+negSumMF;
   if( tempValue1 < 1.0 )
      outReal[outIdx++] = 0.0;
   else
      outReal[outIdx++] = 100.0*(posSumMF/tempValue1);

   /* Now continue processing the remaining bars. */
   while( today <= endIdx )
   {
      posSumMF -= mflow_positive[mflow_Idx];
      negSumMF -= mflow_negative[mflow_Idx];

      tempValue1 = (inHigh[today]+inLow[today]+inClose[today])/3.0;
      tempValue2 = tempValue1 - prevValue;
      prevValue  = tempValue1;
      tempValue1 *= inVolume[today++];
      if( tempValue2 < 0 )
      {
         mflow_negative[mflow_Idx] = tempValue1;
         negSumMF += tempValue1;
         mflow_positive[mflow_Idx] = 0.0;
      }
      else if( tempValue2 > 0 )
      {
         mflow_positive[mflow_Idx] = tempValue1;
         posSumMF += tempValue1;
         mflow_negative[mflow_Idx] = 0.0;
      }
      else
      {
         mflow_positive[mflow_Idx] = 0.0;
         mflow_negative[mflow_Idx] = 0.0;
      }

      tempValue1 = posSumMF+negSumMF;
      if( tempValue1 < 1.0 )
         outReal[outIdx++] = 0.0;
      else
         outReal[outIdx++] = 100.0*(posSumMF/tempValue1);

      CIRCBUF_NEXT(mflow);
   }

   CIRCBUF_DESTROY(mflow);

   *outBegIdx = startIdx;
   *outNBElement = outIdx;

   return TA_SUCCESS;
}
