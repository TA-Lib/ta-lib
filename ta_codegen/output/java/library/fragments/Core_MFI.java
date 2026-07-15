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
 */

   public int mfiLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   public RetCode mfi( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       double inVolume[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Id, Type, Static Size */
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Accumulate the positive and negative money flow
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      /* The following two equations are equivalent:
       *    MFI = 100 - (100 / 1 + (posSumMF/negSumMF))
       *    MFI = 100 * (posSumMF/(posSumMF+negSumMF))
       * The second equation is used here for speed optimization.
       */
      /* The first full window is complete: emit its output for startIdx here,
       * then slide the window over the remaining bars below.
       */
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
      }
      /* Now continue processing the remaining bars. */
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         /* Dead-zone scaled to the two typical prices being compared (issue #107).
          * Captured before prevValue/tempValue1 are repurposed below.
          */
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode mfiUnguarded( int startIdx,
                                int endIdx,
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                double inVolume[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx - lookbackTotal;
      prevValue = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
      }
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = (inHigh[today] + inLow[today] + inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode mfi( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       float inVolume[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx - lookbackTotal;
      prevValue = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
      }
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode mfiUnguarded( int startIdx,
                                int endIdx,
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                float inVolume[],
                                int optInTimePeriod,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double posSumMF = 0;
      double negSumMF = 0;
      double prevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int today = 0;
      double[] mflow_positive;
      double[] mflow_negative;
      int mflow_Idx = 0;
      int maxIdx_mflow = (50)-1;
      if( optInTimePeriod < 1 ) return RetCode.AllocErr;
      mflow_positive = new double[optInTimePeriod];
      mflow_negative = new double[optInTimePeriod];
      maxIdx_mflow = (optInTimePeriod)-1;
      mflow_Idx = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = optInTimePeriod;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx - lookbackTotal;
      prevValue = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
      posSumMF = 0.0;
      negSumMF = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      tempValue1 = posSumMF + negSumMF;
      if( tempValue1 < 1.0 ) {
         outReal[outIdx++] = 0.0;
      } else {
         outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
      }
      while( today <= endIdx ) {
         posSumMF -= mflow_positive[mflow_Idx];
         negSumMF -= mflow_negative[mflow_Idx];
         tempValue1 = ((double)inHigh[today] + (double)inLow[today] + (double)inClose[today]) / 3.0;
         tempValue2 = tempValue1 - prevValue;
         tempValue3 = Math.abs(tempValue1) + Math.abs(prevValue);
         prevValue = tempValue1;
         tempValue1 *= (double)inVolume[today++];
         if( (Math.abs(tempValue2) <= 0.00000000000001 * (tempValue3)) ) {
            mflow_positive[mflow_Idx] = 0.0;
            mflow_negative[mflow_Idx] = 0.0;
         } else if( tempValue2 < 0 ) {
            mflow_negative[mflow_Idx] = tempValue1;
            negSumMF += tempValue1;
            mflow_positive[mflow_Idx] = 0.0;
         } else {
            mflow_positive[mflow_Idx] = tempValue1;
            posSumMF += tempValue1;
            mflow_negative[mflow_Idx] = 0.0;
         }
         tempValue1 = posSumMF + negSumMF;
         if( tempValue1 < 1.0 ) {
            outReal[outIdx++] = 0.0;
         } else {
            outReal[outIdx++] = 100.0 * (posSumMF / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
