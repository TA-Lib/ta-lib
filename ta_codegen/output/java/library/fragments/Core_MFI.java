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
/**** Streaming API *****/

   /**
    * A live MFI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#mfi} over the same series.
    * Open with {@link Core#mfiOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class MfiStream {
      final Core core;
      int optInTimePeriod;
      double posSumMF;
      double negSumMF;
      double prevValue;
      double tempValue1;
      double tempValue2;
      double tempValue3;
      int mflow_Idx;
      int maxIdx_mflow;
      int cbSize_mflow;
      double[] cb_mflow_positive;
      double[] cb_mflow_negative;
      double cur_outReal;

      MfiStream( Core core ) { this.core = core; }

      MfiStream( MfiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.posSumMF = other.posSumMF;
         this.negSumMF = other.negSumMF;
         this.prevValue = other.prevValue;
         this.tempValue1 = other.tempValue1;
         this.tempValue2 = other.tempValue2;
         this.tempValue3 = other.tempValue3;
         this.mflow_Idx = other.mflow_Idx;
         this.maxIdx_mflow = other.maxIdx_mflow;
         this.cbSize_mflow = other.cbSize_mflow;
         this.cb_mflow_positive = other.cb_mflow_positive.clone();
         this.cb_mflow_negative = other.cb_mflow_negative.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         core.mfiStreamStep(this, inHigh, inLow, inClose, inVolume);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         MfiStream scratch = new MfiStream(this);
         core.mfiStreamStep(scratch, inHigh, inLow, inClose, inVolume);
         return scratch.cur_outReal;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MfiStream copy() {
         return new MfiStream(this);
      }
   }
   void mfiStreamStep( MfiStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      sp.posSumMF -= sp.cb_mflow_positive[sp.mflow_Idx];
      sp.negSumMF -= sp.cb_mflow_negative[sp.mflow_Idx];
      sp.tempValue1 = (inHigh + inLow + inClose) / 3.0;
      sp.tempValue2 = sp.tempValue1 - sp.prevValue;
      /* Dead-zone scaled to the two typical prices being compared (issue #107).
       * Captured before prevValue/tempValue1 are repurposed below.
       */
      sp.tempValue3 = Math.abs(sp.tempValue1) + Math.abs(sp.prevValue);
      sp.prevValue = sp.tempValue1;
      sp.tempValue1 *= inVolume;
      if( (Math.abs(sp.tempValue2) <= 0.00000000000001 * (sp.tempValue3)) ) {
         sp.cb_mflow_positive[sp.mflow_Idx] = 0.0;
         sp.cb_mflow_negative[sp.mflow_Idx] = 0.0;
      } else if( sp.tempValue2 < 0 ) {
         sp.cb_mflow_negative[sp.mflow_Idx] = sp.tempValue1;
         sp.negSumMF += sp.tempValue1;
         sp.cb_mflow_positive[sp.mflow_Idx] = 0.0;
      } else {
         sp.cb_mflow_positive[sp.mflow_Idx] = sp.tempValue1;
         sp.posSumMF += sp.tempValue1;
         sp.cb_mflow_negative[sp.mflow_Idx] = 0.0;
      }
      sp.tempValue1 = sp.posSumMF + sp.negSumMF;
      if( sp.tempValue1 < 1.0 ) {
         sp.cur_outReal = 0.0;
      } else {
         sp.cur_outReal = 100.0 * (sp.posSumMF / sp.tempValue1);
      }
      sp.mflow_Idx = sp.mflow_Idx + 1;
      if( sp.mflow_Idx > sp.maxIdx_mflow ) {
         sp.mflow_Idx = 0;
      }
   }
   private RetCode mfiOpenBody( MfiStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
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
         return RetCode.OutOfRangeEndIndex ;
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
         lastValue_outReal = 0.0;
      } else {
         lastValue_outReal = 100.0 * (posSumMF / tempValue1);
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
            lastValue_outReal = 0.0;
         } else {
            lastValue_outReal = 100.0 * (posSumMF / tempValue1);
         }
         mflow_Idx++;
         if( mflow_Idx > maxIdx_mflow ) { mflow_Idx = 0; }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capCb_mflow = maxIdx_mflow + 1;
      if( capCb_mflow > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.posSumMF = posSumMF;
      sp.negSumMF = negSumMF;
      sp.prevValue = prevValue;
      sp.tempValue1 = tempValue1;
      sp.tempValue2 = tempValue2;
      sp.tempValue3 = tempValue3;
      sp.mflow_Idx = mflow_Idx;
      sp.maxIdx_mflow = maxIdx_mflow;
      sp.cbSize_mflow = capCb_mflow;
      sp.cb_mflow_positive = mflow_positive;
      sp.cb_mflow_negative = mflow_negative;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode mfiOpenAndFillBody( MfiStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
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
         return RetCode.OutOfRangeEndIndex ;
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
      /* Capture the live batch state into the handle. */
      int capCb_mflow = maxIdx_mflow + 1;
      if( capCb_mflow > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.posSumMF = posSumMF;
      sp.negSumMF = negSumMF;
      sp.prevValue = prevValue;
      sp.tempValue1 = tempValue1;
      sp.tempValue2 = tempValue2;
      sp.tempValue3 = tempValue3;
      sp.mflow_Idx = mflow_Idx;
      sp.maxIdx_mflow = maxIdx_mflow;
      sp.cbSize_mflow = capCb_mflow;
      sp.cb_mflow_positive = mflow_positive;
      sp.cb_mflow_negative = mflow_negative;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind mfiOpen (composition seam). */
   MfiStream mfiOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      MfiStream sp = new MfiStream(this);
      RetCode retCode = mfiOpenBody(sp, inHigh, inLow, inClose, inVolume, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MFI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MFI open: internal error");
      }
      throw new IllegalArgumentException("TA_MFI open: " + retCode);
   }
   /**
    * Open a live MFI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#mfi} at that bar.
    * <p>The history must hold at least {@code mfiLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MfiStream mfiOpen( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod )
   {
      return mfiOpenInternal(inHigh, inLow, inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#mfiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#mfi} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public MfiStream mfiOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MfiStream sp = new MfiStream(this);
      RetCode retCode = mfiOpenAndFillBody(sp, inHigh, inLow, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MFI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MFI openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MFI openAndFill: " + retCode);
   }
