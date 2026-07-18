/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  101003 MF   Initial Coding
 *  062804 MF   Resolve div by zero bug on limit case.
 */

   public int correlLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode correl( int startIdx,
                          int endIdx,
                          double inReal0[],
                          double inReal1[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      /* Calculate the initial values. */
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      /* Write the first output.
       * Save first the trailing values since the input
       * and output might be the same array,
       */
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      /* Tight loop to do subsequent values. */
      outIdx = 1;
      while( today <= endIdx ) {
         /* Remove trailing values */
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         /* Add new values */
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         /* Output new coefficient.
          * Save first the trailing values since the input
          * and output might be the same array,
          */
         trailingX = inReal0[trailingIdx];
         trailingY = inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode correlUnguarded( int startIdx,
                                   int endIdx,
                                   double inReal0[],
                                   double inReal1[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today <= endIdx ) {
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         trailingX = inReal0[trailingIdx];
         trailingY = inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode correl( int startIdx,
                          int endIdx,
                          float inReal0[],
                          float inReal1[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      trailingX = (double)inReal0[trailingIdx];
      trailingY = (double)inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today <= endIdx ) {
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         trailingX = (double)inReal0[trailingIdx];
         trailingY = (double)inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode correlUnguarded( int startIdx,
                                   int endIdx,
                                   float inReal0[],
                                   float inReal1[],
                                   int optInTimePeriod,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      trailingX = (double)inReal0[trailingIdx];
      trailingY = (double)inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      outIdx = 1;
      while( today <= endIdx ) {
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         x = (double)inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = (double)inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         trailingX = (double)inReal0[trailingIdx];
         trailingY = (double)inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CORREL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#correl} over the same series.
    * Open with {@link Core#correlOpen}; there is no close — the handle is
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
   public static final class CorrelStream {
      final Core core;
      int optInTimePeriod;
      double sumXY;
      double sumX;
      double sumY;
      double sumX2;
      double sumY2;
      double x;
      double y;
      double trailingX;
      double trailingY;
      double tempReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal0;
      double[] ring_trailingIdx_inReal1;
      double cur_outReal;

      CorrelStream( Core core ) { this.core = core; }

      CorrelStream( CorrelStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.sumXY = other.sumXY;
         this.sumX = other.sumX;
         this.sumY = other.sumY;
         this.sumX2 = other.sumX2;
         this.sumY2 = other.sumY2;
         this.x = other.x;
         this.y = other.y;
         this.trailingX = other.trailingX;
         this.trailingY = other.trailingY;
         this.tempReal = other.tempReal;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal0 = other.ring_trailingIdx_inReal0.clone();
         this.ring_trailingIdx_inReal1 = other.ring_trailingIdx_inReal1.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal0, double inReal1 ) {
         core.correlStreamStep(this, inReal0, inReal1);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal0, double inReal1 ) {
         CorrelStream scratch = new CorrelStream(this);
         core.correlStreamStep(scratch, inReal0, inReal1);
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
      public CorrelStream copy() {
         return new CorrelStream(this);
      }
   }
   void correlStreamStep( CorrelStream sp, double inReal0, double inReal1 )
   {
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal0[0] = inReal0;
         sp.ring_trailingIdx_inReal1[0] = inReal1;
      }
      /* Remove trailing values */
      sp.sumX -= sp.trailingX;
      sp.sumX2 -= sp.trailingX * sp.trailingX;
      sp.sumXY -= sp.trailingX * sp.trailingY;
      sp.sumY -= sp.trailingY;
      sp.sumY2 -= sp.trailingY * sp.trailingY;
      /* Add new values */
      sp.x = inReal0;
      sp.sumX += sp.x;
      sp.sumX2 += sp.x * sp.x;
      sp.y = inReal1;
      sp.sumXY += sp.x * sp.y;
      sp.sumY += sp.y;
      sp.sumY2 += sp.y * sp.y;
      /* Output new coefficient.
       * Save first the trailing values since the input
       * and output might be the same array,
       */
      sp.trailingX = sp.ring_trailingIdx_inReal0[sp.ringPos_trailingIdx];
      sp.trailingY = sp.ring_trailingIdx_inReal1[sp.ringPos_trailingIdx];
      sp.tempReal = (sp.sumX2 - sp.sumX * sp.sumX / sp.optInTimePeriod) * (sp.sumY2 - sp.sumY * sp.sumY / sp.optInTimePeriod);
      if( !(sp.tempReal < 0.00000000000001) ) {
         sp.cur_outReal = (sp.sumXY - sp.sumX * sp.sumY / sp.optInTimePeriod) / Math.sqrt(sp.tempReal);
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.ring_trailingIdx_inReal0[sp.ringPos_trailingIdx] = inReal0;
      sp.ring_trailingIdx_inReal1[sp.ringPos_trailingIdx] = inReal1;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode correlOpenBody( CorrelStream sp, double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      /* Calculate the initial values. */
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      /* Write the first output.
       * Save first the trailing values since the input
       * and output might be the same array,
       */
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         lastValue_outReal = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         lastValue_outReal = 0.0;
      }
      /* Tight loop to do subsequent values. */
      outIdx = 1;
      while( today <= endIdx ) {
         /* Remove trailing values */
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         /* Add new values */
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         /* Output new coefficient.
          * Save first the trailing values since the input
          * and output might be the same array,
          */
         trailingX = inReal0[trailingIdx];
         trailingY = inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            lastValue_outReal = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            lastValue_outReal = 0.0;
         }
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal0 = new double[allocN_trailingIdx];
      System.arraycopy(inReal0, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal0, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inReal1 = new double[allocN_trailingIdx];
      System.arraycopy(inReal1, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal1, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumXY = sumXY;
      sp.sumX = sumX;
      sp.sumY = sumY;
      sp.sumX2 = sumX2;
      sp.sumY2 = sumY2;
      sp.x = x;
      sp.y = y;
      sp.trailingX = trailingX;
      sp.trailingY = trailingY;
      sp.tempReal = tempReal;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal0 = capRing_trailingIdx_inReal0;
      sp.ring_trailingIdx_inReal1 = capRing_trailingIdx_inReal1;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode correlOpenAndFillBody( CorrelStream sp, double inReal0[], double inReal1[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double sumXY = 0;
      double sumX = 0;
      double sumY = 0;
      double sumX2 = 0;
      double sumY2 = 0;
      double x = 0;
      double y = 0;
      double trailingX = 0;
      double trailingY = 0;
      double tempReal = 0;
      int lookbackTotal = 0;
      int today = 0;
      int trailingIdx = 0;
      int outIdx = 0;
      int historyLen = inReal0.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inReal1.length != inReal0.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal0 || (Object)outReal == (Object)inReal1 ) {
         return RetCode.BadParam;
      }
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outBegIdx.value = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      /* Calculate the initial values. */
      sumY2 = 0.0;
      sumX2 = sumY2;
      sumY = sumX2;
      sumX = sumY;
      sumXY = sumX;
      for( today = trailingIdx; today <= startIdx; today += 1 ) {
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
      }
      /* Write the first output.
       * Save first the trailing values since the input
       * and output might be the same array,
       */
      trailingX = inReal0[trailingIdx];
      trailingY = inReal1[trailingIdx++];
      tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
      if( !(tempReal < 0.00000000000001) ) {
         outReal[0] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
      } else {
         outReal[0] = 0.0;
      }
      /* Tight loop to do subsequent values. */
      outIdx = 1;
      while( today <= endIdx ) {
         /* Remove trailing values */
         sumX -= trailingX;
         sumX2 -= trailingX * trailingX;
         sumXY -= trailingX * trailingY;
         sumY -= trailingY;
         sumY2 -= trailingY * trailingY;
         /* Add new values */
         x = inReal0[today];
         sumX += x;
         sumX2 += x * x;
         y = inReal1[today++];
         sumXY += x * y;
         sumY += y;
         sumY2 += y * y;
         /* Output new coefficient.
          * Save first the trailing values since the input
          * and output might be the same array,
          */
         trailingX = inReal0[trailingIdx];
         trailingY = inReal1[trailingIdx++];
         tempReal = (sumX2 - sumX * sumX / optInTimePeriod) * (sumY2 - sumY * sumY / optInTimePeriod);
         if( !(tempReal < 0.00000000000001) ) {
            outReal[outIdx++] = (sumXY - sumX * sumY / optInTimePeriod) / Math.sqrt(tempReal);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal0 = new double[allocN_trailingIdx];
      System.arraycopy(inReal0, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal0, 0, cap_trailingIdx);
      double[] capRing_trailingIdx_inReal1 = new double[allocN_trailingIdx];
      System.arraycopy(inReal1, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal1, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.sumXY = sumXY;
      sp.sumX = sumX;
      sp.sumY = sumY;
      sp.sumX2 = sumX2;
      sp.sumY2 = sumY2;
      sp.x = x;
      sp.y = y;
      sp.trailingX = trailingX;
      sp.trailingY = trailingY;
      sp.tempReal = tempReal;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal0 = capRing_trailingIdx_inReal0;
      sp.ring_trailingIdx_inReal1 = capRing_trailingIdx_inReal1;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind correlOpen (composition seam). */
   CorrelStream correlOpenInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
   {
      CorrelStream sp = new CorrelStream(this);
      RetCode retCode = correlOpenBody(sp, inReal0, inReal1, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CORREL open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CORREL open: internal error");
      }
      throw new IllegalArgumentException("TA_CORREL open: " + retCode);
   }
   /**
    * Open a live CORREL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#correl} at that bar.
    * <p>The history must hold at least {@code correlLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CorrelStream correlOpen( double inReal0[], double inReal1[], int optInTimePeriod )
   {
      return correlOpenInternal(inReal0, inReal1, 0, optInTimePeriod);
   }
   /**
    * {@link Core#correlOpen} that also fills the output array(s) bit-identically
    * to {@link Core#correl} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CorrelStream correlOpenAndFill( double inReal0[], double inReal1[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CorrelStream sp = new CorrelStream(this);
      RetCode retCode = correlOpenAndFillBody(sp, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CORREL openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CORREL openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CORREL openAndFill: " + retCode);
   }
