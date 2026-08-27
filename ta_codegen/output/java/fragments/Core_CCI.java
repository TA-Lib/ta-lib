/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AF       Alexander Trufanov (github @trufanov-nok)
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  031202 MF     Template creation.
 *  052603 MF     Port to managed C++. Change to use CIRCBUF macros.
 *  061704 MF     Lower limit for period to 2, and correct algorithm
 *                to avoid cummulative error when value are close to
 *                the floating point epsilon.
 *  070626 AF,CC  Guard the final division with TA_IS_ZERO instead of an exact
 *                "!= 0.0" check: identical prices over the period leave
 *                sub-epsilon residue that the exact check divided into a
 *                spurious value (issue #7 / SF bug #107). Now returns 0.0.
 *  082326 MF,CC  Fix #253. Scale that flatness test to the window's own price
 *                level: the fixed band zeroed the whole output for any
 *                instrument quoted small enough to fall under it.
 */

   /**
    * Number of leading input bars {@link Core#CCI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the averaging/deviation window
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CCI_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode CCI_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double tempReal3 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* This ptr will points on a circular buffer of
       * at least "optInTimePeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Allocate a circular buffer equal to the requested
       * period.
       */
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value.
       * Fill up the circular buffer at the same time.
       */
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         /* Calculate the average for the whole period. */
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         /* Do the summation of the ABS(TypePrice-average)
          * for the whole period, then its mean.
          */
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal2 /= optInTimePeriod;
         /* And finally, the CCI... */
         tempReal = lastValue - theAverage;
         /* Both tests are relative to the window's own price level (issue #253).
          * They ask "is this window flat?", and flatness is a property of the
          * prices relative to each other -- but a deviation carries the quote
          * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
          * every window of an instrument quoted below it and zeroed the whole
          * output. The band is still wide enough (~90 ulp of the average) to
          * absorb the sub-epsilon residue an identical-price window leaves in the
          * average, which is what it was widened for in the first place (#7).
          */
         tempReal3 = Math.abs(theAverage);
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) ) {
            outReal[outIdx++] = tempReal / (0.015 * tempReal2);
         } else {
            outReal[outIdx++] = 0.0;
         }
         /* Move forward the circular buffer indexes. */
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Free the circular buffer if it was dynamically allocated. */
      return RetCode.Success ;
   }
   RetCode CCI_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double tempReal3 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
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
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      outIdx = 0;
      do {
         lastValue = ((double)inHigh[i] + (double)inLow[i] + (double)inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal2 /= optInTimePeriod;
         tempReal = lastValue - theAverage;
         tempReal3 = Math.abs(theAverage);
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) ) {
            outReal[outIdx++] = tempReal / (0.015 * tempReal2);
         } else {
            outReal[outIdx++] = 0.0;
         }
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Commodity Channel Index: measures the current typical price relative to
    * its simple moving average, scaled by mean absolute deviation. Momentum
    * oscillator flagging overbought/oversold extremes. CCI &gt; +100
    * overbought; CCI &lt; -100 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP_i = (High_i + Low_i + Close_i)/3
    * SMA = (1/N) * sum(TP over N bars)
    * meanDev = (1/N) * sum(|TP - SMA| over N bars)
    * CCI = (TP_last - SMA) / (0.015 * meanDev)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CCI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars in the averaging/deviation window
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal CCI value per bar. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#TYPPRICE
    * @see Core#SMA
    */
   public OutRange CCI( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("CCI", startIdx, endIdx);
      int guardStart = clampedStart("CCI", startIdx, CCI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CCI", "inHigh", inHigh, guardInLen);
      requireLength("CCI", "inLow", inLow, guardInLen);
      requireLength("CCI", "inClose", inClose, guardInLen);
      requireLength("CCI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CCI_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CCI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Commodity Channel Index: measures the current typical price relative to
    * its simple moving average, scaled by mean absolute deviation. Momentum
    * oscillator flagging overbought/oversold extremes. CCI &gt; +100
    * overbought; CCI &lt; -100 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * TP_i = (High_i + Low_i + Close_i)/3
    * SMA = (1/N) * sum(TP over N bars)
    * meanDev = (1/N) * sum(|TP - SMA| over N bars)
    * CCI = (TP_last - SMA) / (0.015 * meanDev)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CCI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars in the averaging/deviation window
    *        (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal CCI value per bar. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#TYPPRICE
    * @see Core#SMA
    */
   public OutRange CCI( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("CCI", startIdx, endIdx);
      int guardStart = clampedStart("CCI", startIdx, CCI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CCI", "inHigh", inHigh, guardInLen);
      requireLength("CCI", "inLow", inLow, guardInLen);
      requireLength("CCI", "inClose", inClose, guardInLen);
      requireLength("CCI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CCI_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CCI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CCI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CCI} over the same series.
    * Open with {@link Core#CCI_Open}; there is no close — the handle is
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
   public static final class CCI_Stream {
      Core core;
      int optInTimePeriod;
      int circBuffer_Idx;
      int maxIdx_circBuffer;
      int cbSize_circBuffer;
      double[] cb_circBuffer;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      CCI_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CCI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CCI_Stream( CCI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.circBuffer_Idx = other.circBuffer_Idx;
         this.maxIdx_circBuffer = other.maxIdx_circBuffer;
         this.cbSize_circBuffer = other.cbSize_circBuffer;
         this.cb_circBuffer = other.cb_circBuffer.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( CCI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.circBuffer_Idx = other.circBuffer_Idx;
         this.maxIdx_circBuffer = other.maxIdx_circBuffer;
         this.cbSize_circBuffer = other.cbSize_circBuffer;
         if( this.cb_circBuffer != null && this.cb_circBuffer.length == other.cb_circBuffer.length ) {
            System.arraycopy( other.cb_circBuffer, 0, this.cb_circBuffer, 0, other.cb_circBuffer.length );
         } else {
            this.cb_circBuffer = other.cb_circBuffer.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the handle is left exactly as it was —
       * the stream stays usable, so skip the bar or re-open on a clean
       * history. This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CCI update: BadParam", RetCode.BadParam);
         core.CCI_StepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("CCI updateAndFill", "inHigh", inHigh);
         requireArgument("CCI updateAndFill", "inLow", inLow);
         requireArgument("CCI updateAndFill", "inClose", inClose);
         requireArgument("CCI updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("CCI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("CCI updateAndFill: BadParam", RetCode.BadParam);
            core.CCI_StepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("CCI peek: BadParam", RetCode.BadParam);
         CCI_Stream scratch = new CCI_Stream(this);
         core.CCI_StepImpl(scratch, inHigh, inLow, inClose);
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
      public CCI_Stream copy() {
         return new CCI_Stream(this);
      }
   }
   void CCI_StepImpl( CCI_Stream sp, double inHigh, double inLow, double inClose )
   {
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      double tempReal3 = 0.0;
      double theAverage = 0.0;
      double lastValue = 0.0;
      int j = 0;
      lastValue = (inHigh + inLow + inClose) / 3;
      sp.cb_circBuffer[sp.circBuffer_Idx] = lastValue;
      /* Calculate the average for the whole period. */
      theAverage = 0;
      for( j = 0; j < sp.optInTimePeriod; j += 1 ) {
         theAverage += sp.cb_circBuffer[j];
      }
      theAverage /= sp.optInTimePeriod;
      /* Do the summation of the ABS(TypePrice-average)
       * for the whole period, then its mean.
       */
      tempReal2 = 0;
      for( j = 0; j < sp.optInTimePeriod; j += 1 ) {
         tempReal2 += Math.abs(sp.cb_circBuffer[j] - theAverage);
      }
      tempReal2 /= sp.optInTimePeriod;
      /* And finally, the CCI... */
      tempReal = lastValue - theAverage;
      /* Both tests are relative to the window's own price level (issue #253).
       * They ask "is this window flat?", and flatness is a property of the
       * prices relative to each other -- but a deviation carries the quote
       * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
       * every window of an instrument quoted below it and zeroed the whole
       * output. The band is still wide enough (~90 ulp of the average) to
       * absorb the sub-epsilon residue an identical-price window leaves in the
       * average, which is what it was widened for in the first place (#7).
       */
      tempReal3 = Math.abs(theAverage);
      if( !(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) ) {
         sp.cur_outReal = tempReal / (0.015 * tempReal2);
      } else {
         sp.cur_outReal = 0.0;
      }
      /* Move forward the circular buffer indexes. */
      sp.circBuffer_Idx = sp.circBuffer_Idx + 1;
      if( sp.circBuffer_Idx > sp.maxIdx_circBuffer ) {
         sp.circBuffer_Idx = 0;
      }
   }
   private RetCode CCI_OpenImpl( CCI_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double tempReal = 0;
      double tempReal2 = 0;
      double tempReal3 = 0;
      double theAverage = 0;
      double lastValue = 0;
      int i = 0;
      int j = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double[] circBuffer;
      int circBuffer_Idx = 0;
      int maxIdx_circBuffer = (30)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* This ptr will points on a circular buffer of
       * at least "optInTimePeriod" element.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Allocate a circular buffer equal to the requested
       * period.
       */
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      circBuffer = new double[optInTimePeriod];
      maxIdx_circBuffer = (optInTimePeriod)-1;
      circBuffer_Idx = 0;
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value.
       * Fill up the circular buffer at the same time.
       */
      i = startIdx - lookbackTotal;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            circBuffer[circBuffer_Idx] = (inHigh[i] + inLow[i] + inClose[i]) / 3;
            i += 1;
            circBuffer_Idx++;
            if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the inReal and
       * outReal to be the same buffer.
       */
      outIdx = 0;
      do {
         lastValue = (inHigh[i] + inLow[i] + inClose[i]) / 3;
         circBuffer[circBuffer_Idx] = lastValue;
         /* Calculate the average for the whole period. */
         theAverage = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            theAverage += circBuffer[j];
         }
         theAverage /= optInTimePeriod;
         /* Do the summation of the ABS(TypePrice-average)
          * for the whole period, then its mean.
          */
         tempReal2 = 0;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            tempReal2 += Math.abs(circBuffer[j] - theAverage);
         }
         tempReal2 /= optInTimePeriod;
         /* And finally, the CCI... */
         tempReal = lastValue - theAverage;
         /* Both tests are relative to the window's own price level (issue #253).
          * They ask "is this window flat?", and flatness is a property of the
          * prices relative to each other -- but a deviation carries the quote
          * unit, so the fixed TA_IS_ZERO band these used to be answered "flat" for
          * every window of an instrument quoted below it and zeroed the whole
          * output. The band is still wide enough (~90 ulp of the average) to
          * absorb the sub-epsilon residue an identical-price window leaves in the
          * average, which is what it was widened for in the first place (#7).
          */
         tempReal3 = Math.abs(theAverage);
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (tempReal3)) && !(Math.abs(tempReal2) <= 0.00000000000001 * (tempReal3)) ) {
            outReal[outIdx++ * outStride] = tempReal / (0.015 * tempReal2);
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
         /* Move forward the circular buffer indexes. */
         circBuffer_Idx++;
         if( circBuffer_Idx > maxIdx_circBuffer ) { circBuffer_Idx = 0; }
         i += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Free the circular buffer if it was dynamically allocated. */
      /* Capture the live batch state into the handle. */
      int capCb_circBuffer = maxIdx_circBuffer + 1;
      if( capCb_circBuffer > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.circBuffer_Idx = circBuffer_Idx;
      sp.maxIdx_circBuffer = maxIdx_circBuffer;
      sp.cbSize_circBuffer = capCb_circBuffer;
      sp.cb_circBuffer = circBuffer;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* CCI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CCI_Stream CCI_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CCI_Stream sp = new CCI_Stream(this);
      RetCode retCode = CCI_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CCI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CCI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CCI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind CCI_Open (composition seam). */
   CCI_Stream CCI_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      CCI_Stream sp = new CCI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = CCI_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CCI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CCI open: internal error", retCode);
      }
      throw new TaLibArgumentException("CCI open: " + retCode, retCode);
   }
   /**
    * Open a live CCI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CCI} at that bar.
    * <p>The history must hold at least {@code CCI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CCI_Stream CCI_Open( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("CCI open", "inHigh", inHigh);
      requireHistory("CCI open", inHigh.length);
      requireArgument("CCI open", "inLow", inLow);
      requireArgument("CCI open", "inClose", inClose);
      requireHistoryLength("CCI open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CCI open", "inClose", inClose.length, inHigh.length);
      return CCI_OpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#CCI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CCI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CCI_Stream#outRange()}.
    */
   public CCI_Stream CCI_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CCI openAndFill", "inHigh", inHigh);
      requireHistory("CCI openAndFill", inHigh.length);
      requireArgument("CCI openAndFill", "inLow", inLow);
      requireArgument("CCI openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("CCI openAndFill", inHigh.length, CCI_Lookback(optInTimePeriod));
      requireHistoryLength("CCI openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("CCI openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("CCI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("CCI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return CCI_OpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
