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

   /**
    * Number of leading input bars {@link Core#CORREL} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Rolling window length (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CORREL_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode CORREL_Internal( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
   RetCode CORREL_Internal( int startIdx,
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
   /**
    * Pearson's correlation coefficient (r) between two input series over a
    * rolling window of optInTimePeriod bars. Measures how linearly the two
    * series move together. r near +1: strong positive co-movement; near -1:
    * strong inverse; near 0: no linear relationship.
    * <p><b>Formula</b>
    * <pre>{@code
    * r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CORREL_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 First data series (X)
    * @param inReal1 Second data series (Y)
    * @param optInTimePeriod Rolling window length (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Correlation coefficient r in [-1, 1]. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#BETA
    * @see Core#STDDEV
    * @see Core#VAR
    */
   public OutRange CORREL( int startIdx,
                           int endIdx,
                           double inReal0[],
                           double inReal1[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CORREL_Internal(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CORREL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Pearson's correlation coefficient (r) between two input series over a
    * rolling window of optInTimePeriod bars. Measures how linearly the two
    * series move together. r near +1: strong positive co-movement; near -1:
    * strong inverse; near 0: no linear relationship.
    * <p><b>Formula</b>
    * <pre>{@code
    * r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CORREL_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal0 First data series (X)
    * @param inReal1 Second data series (Y)
    * @param optInTimePeriod Rolling window length (default 30; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Correlation coefficient r in [-1, 1]. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#BETA
    * @see Core#STDDEV
    * @see Core#VAR
    */
   public OutRange CORREL( int startIdx,
                           int endIdx,
                           float inReal0[],
                           float inReal1[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CORREL_Internal(startIdx, endIdx, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CORREL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CORREL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CORREL} over the same series.
    * Open with {@link Core#CORREL_Open}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class CORREL_Stream {
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
      OutRange fillRange = OutRange.EMPTY;

      CORREL_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CORREL_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CORREL_Stream( CORREL_Stream other ) {
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
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal0, double inReal1 ) {
         core.CORREL_StreamStep(this, inReal0, inReal1);
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
         CORREL_Stream scratch = new CORREL_Stream(this);
         core.CORREL_StreamStep(scratch, inReal0, inReal1);
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
      public CORREL_Stream copy() {
         return new CORREL_Stream(this);
      }
   }
   void CORREL_StreamStep( CORREL_Stream sp, double inReal0, double inReal1 )
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
   private RetCode CORREL_OpenBody( CORREL_Stream sp, double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
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
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
   private RetCode CORREL_OpenAndFillBody( CORREL_Stream sp, double inReal0[], double inReal1[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
   /* Internal startIdx-anchored open behind CORREL_Open (composition seam). */
   CORREL_Stream CORREL_OpenInternal( double inReal0[], double inReal1[], int startIdx, int optInTimePeriod )
   {
      CORREL_Stream sp = new CORREL_Stream(this);
      RetCode retCode = CORREL_OpenBody(sp, inReal0, inReal1, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CORREL open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CORREL open: internal error");
      }
      throw new IllegalArgumentException("CORREL open: " + retCode);
   }
   /**
    * Open a live CORREL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CORREL} at that bar.
    * <p>The history must hold at least {@code CORREL_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CORREL_Stream CORREL_Open( double inReal0[], double inReal1[], int optInTimePeriod )
   {
      return CORREL_OpenInternal(inReal0, inReal1, 0, optInTimePeriod);
   }
   /**
    * {@link Core#CORREL_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CORREL} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CORREL_Stream#fillRange()}.
    */
   public CORREL_Stream CORREL_OpenAndFill( double inReal0[], double inReal1[], int optInTimePeriod, double outReal[] )
   {
      CORREL_Stream sp = new CORREL_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CORREL_OpenAndFillBody(sp, inReal0, inReal1, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CORREL openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CORREL openAndFill: internal error");
      }
      throw new IllegalArgumentException("CORREL openAndFill: " + retCode);
   }
