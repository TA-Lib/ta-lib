/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  JP       John Price <jp_talib@gcfl.net>
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  070203 JP     Initial.
 *  071326 MF,CC  O(period) per-bar rescan -> O(1) sliding-sum recurrence
 *                (numerics-changing). See issue #103.
 */

   public int linearRegSlopeLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   public RetCode linearRegSlope( int startIdx,
                                  int endIdx,
                                  double inReal[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
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
      /* Linear Regression is a concept also known as the
       * "least squares method" or "best fit." Linear
       * Regression attempts to fit a straight line between
       * several data points in such a way that distance
       * between each data point and the line is minimized.
       *
       * For each point, a straight line over the specified
       * previous bar period is determined in terms
       * of y = b + m*x:
       *
       * TA_LINEARREG          : Returns b+m*(period-1)
       * TA_LINEARREG_SLOPE    : Returns 'm'
       * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
       * TA_LINEARREG_INTERCEPT: Returns 'b'
       * TA_TSF                : Returns b+m*(period)
       */
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = linearRegSlopeLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      /* Index into the output. */
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       */
      while( today <= endIdx ) {
         trailingValue = inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegSlopeUnguarded( int startIdx,
                                           int endIdx,
                                           double inReal[],
                                           int optInTimePeriod,
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      lookbackTotal = linearRegSlopeLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      while( today <= endIdx ) {
         trailingValue = inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegSlope( int startIdx,
                                  int endIdx,
                                  float inReal[],
                                  int optInTimePeriod,
                                  MInteger outBegIdx,
                                  MInteger outNBElement,
                                  double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
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
      lookbackTotal = linearRegSlopeLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = (double)inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      while( today <= endIdx ) {
         trailingValue = (double)inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + (double)inReal[today];
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode linearRegSlopeUnguarded( int startIdx,
                                           int endIdx,
                                           float inReal[],
                                           int optInTimePeriod,
                                           MInteger outBegIdx,
                                           MInteger outNBElement,
                                           double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      lookbackTotal = linearRegSlopeLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = (double)inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      while( today <= endIdx ) {
         trailingValue = (double)inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + (double)inReal[today];
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live LINEARREG_SLOPE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#linearRegSlope} over the same series.
    * Open with {@link Core#linearRegSlopeOpen}; there is no close — the handle is
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
   public static final class LinearRegSlopeStream {
      final Core core;
      int optInTimePeriod;
      double SumX;
      double SumXY;
      double SumY;
      double Divisor;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;

      LinearRegSlopeStream( Core core ) { this.core = core; }

      LinearRegSlopeStream( LinearRegSlopeStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.SumX = other.SumX;
         this.SumXY = other.SumXY;
         this.SumY = other.SumY;
         this.Divisor = other.Divisor;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.linearRegSlopeStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         LinearRegSlopeStream scratch = new LinearRegSlopeStream(this);
         core.linearRegSlopeStreamStep(scratch, inReal);
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
      public LinearRegSlopeStream copy() {
         return new LinearRegSlopeStream(this);
      }
   }
   void linearRegSlopeStreamStep( LinearRegSlopeStream sp, double inReal )
   {
      double trailingValue = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      trailingValue = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      sp.SumXY = sp.SumXY + sp.SumY - (double)sp.optInTimePeriod * trailingValue;
      sp.SumY = sp.SumY - trailingValue + inReal;
      sp.cur_outReal = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode linearRegSlopeOpenBody( LinearRegSlopeStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Linear Regression is a concept also known as the
       * "least squares method" or "best fit." Linear
       * Regression attempts to fit a straight line between
       * several data points in such a way that distance
       * between each data point and the line is minimized.
       *
       * For each point, a straight line over the specified
       * previous bar period is determined in terms
       * of y = b + m*x:
       *
       * TA_LINEARREG          : Returns b+m*(period-1)
       * TA_LINEARREG_SLOPE    : Returns 'm'
       * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
       * TA_LINEARREG_INTERCEPT: Returns 'b'
       * TA_TSF                : Returns b+m*(period)
       */
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = linearRegSlopeLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      /* Index into the output. */
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      lastValue_outReal = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       */
      while( today <= endIdx ) {
         trailingValue = inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         lastValue_outReal = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.SumX = SumX;
      sp.SumXY = SumXY;
      sp.SumY = SumY;
      sp.Divisor = Divisor;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode linearRegSlopeOpenAndFillBody( LinearRegSlopeStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int trailingIdx = 0;
      double SumX = 0;
      double SumXY = 0;
      double SumY = 0;
      double SumXSqr = 0;
      double Divisor = 0;
      int i = 0;
      double tempValue1 = 0;
      double trailingValue = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      /* Linear Regression is a concept also known as the
       * "least squares method" or "best fit." Linear
       * Regression attempts to fit a straight line between
       * several data points in such a way that distance
       * between each data point and the line is minimized.
       *
       * For each point, a straight line over the specified
       * previous bar period is determined in terms
       * of y = b + m*x:
       *
       * TA_LINEARREG          : Returns b+m*(period-1)
       * TA_LINEARREG_SLOPE    : Returns 'm'
       * TA_LINEARREG_ANGLE    : Returns 'm' in degree.
       * TA_LINEARREG_INTERCEPT: Returns 'b'
       * TA_TSF                : Returns b+m*(period)
       */
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = linearRegSlopeLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      /* Index into the output. */
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
      }
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       */
      while( today <= endIdx ) {
         trailingValue = inReal[trailingIdx++];
         SumXY = SumXY + SumY - (double)optInTimePeriod * trailingValue;
         SumY = SumY - trailingValue + inReal[today];
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.SumX = SumX;
      sp.SumXY = SumXY;
      sp.SumY = SumY;
      sp.Divisor = Divisor;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind linearRegSlopeOpen (composition seam). */
   LinearRegSlopeStream linearRegSlopeOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      LinearRegSlopeStream sp = new LinearRegSlopeStream(this);
      RetCode retCode = linearRegSlopeOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_LINEARREG_SLOPE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_LINEARREG_SLOPE open: internal error");
      }
      throw new IllegalArgumentException("TA_LINEARREG_SLOPE open: " + retCode);
   }
   /**
    * Open a live LINEARREG_SLOPE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#linearRegSlope} at that bar.
    * <p>The history must hold at least {@code linearRegSlopeLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public LinearRegSlopeStream linearRegSlopeOpen( double inReal[], int optInTimePeriod )
   {
      return linearRegSlopeOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#linearRegSlopeOpen} that also fills the output array(s) bit-identically
    * to {@link Core#linearRegSlope} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public LinearRegSlopeStream linearRegSlopeOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      LinearRegSlopeStream sp = new LinearRegSlopeStream(this);
      RetCode retCode = linearRegSlopeOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_LINEARREG_SLOPE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_LINEARREG_SLOPE openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_LINEARREG_SLOPE openAndFill: " + retCode);
   }
