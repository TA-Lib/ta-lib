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
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#ADOSC} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast A/D EMA (default 3; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow A/D EMA (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ADOSC_Lookback( int optInFastPeriod, int optInSlowPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      int slowestPeriod;
      /* Use the slowest EMA period to evaluate the total lookback. */
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      /* Adjust startIdx to account for the lookback period. */
      return EMA_Lookback(slowestPeriod) ;

   }
   RetCode ADOSC_Impl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       double inVolume[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Implementation Note:
       *     The fastEMA varaible is not neceseraly the
       *     fastest EMA.
       *     In the same way, slowEMA is not neceseraly the
       *     slowest EMA.
       *
       *     The ADOSC is always the (fastEMA - slowEMA) regardless
       *     of the period specified. In other word:
       *
       *     ADOSC(3,10) = EMA(3,AD) - EMA(10,AD)
       *
       *        while
       *
       *     ADOSC(10,3) = EMA(10,AD)- EMA(3,AD)
       *
       *     In the first case the EMA(3) is truly a faster EMA,
       *     while in the second case, the EMA(10) is still call
       *     fastEMA in the algorithm, even if it is in fact slower.
       *
       *     This gives more flexibility to the user if they want to
       *     experiment with unusual parameter settings.
       */
      /* Identify the slowest period.
       * This infomration is used soleley to bootstrap
       * the algorithm (skip the lookback period).
       */
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = EMA_Lookback(slowestPeriod);
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
      today = startIdx - lookbackTotal;
      /* The following variables are used to
       * calculate the "ad".
       */
      ad = 0.0;
      /* Constants for EMA */
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      /* Initialize the two EMA
       *
       * Use the same range of initialization inputs for
       * both EMA and simply seed with the first A/D value.
       *
       * Note: Metastock do the same.
       */
      high = inHigh[today];
      low = inLow[today];
      tmp = high - low;
      close = inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      /* Initialize the EMA and skip the unstable period. */
      while( today < startIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      /* Perform the calculation for the requested range */
      outIdx = 0;
      while( today <= endIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode ADOSC_Impl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       float inVolume[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      lookbackTotal = EMA_Lookback(slowestPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      ad = 0.0;
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      high = (double)inHigh[today];
      low = (double)inLow[today];
      tmp = high - low;
      close = (double)inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      while( today < startIdx ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         tmp = high - low;
         close = (double)inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      outIdx = 0;
      while( today <= endIdx ) {
         high = (double)inHigh[today];
         low = (double)inLow[today];
         tmp = high - low;
         close = (double)inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Chaikin A/D Oscillator: the difference between a fast and a slow EMA of
    * the Accumulation/Distribution line. Highlights momentum in
    * accumulation/distribution volume flow. Positive/rising suggests
    * accumulation; negative/falling suggests distribution.
    * <p><b>Formula</b>
    * <pre>{@code
    * ad += ((close-low)-(high-close))/(high-low) * volume   (only when high>low)
    * fastEMA = fastk*ad + (1-fastk)*fastEMA,  fastk = 2/(optInFastPeriod+1)
    * slowEMA = slowk*ad + (1-slowk)*slowEMA,  slowk = 2/(optInSlowPeriod+1)
    * ADOSC = fastEMA - slowEMA
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADOSC_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInFastPeriod Period of the fast A/D EMA (default 3; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow A/D EMA (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Fast-EMA minus slow-EMA of the A/D line. Must hold at least
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
    * @see Core#AD
    * @see Core#EMA
    */
   public OutRange ADOSC( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          double inVolume[],
                          int optInFastPeriod,
                          int optInSlowPeriod,
                          double outReal[] )
   {
      requireIndexRange("ADOSC", startIdx, endIdx);
      int guardStart = clampedStart("ADOSC", startIdx, ADOSC_Lookback(optInFastPeriod, optInSlowPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADOSC", "inHigh", inHigh, guardInLen);
      requireLength("ADOSC", "inLow", inLow, guardInLen);
      requireLength("ADOSC", "inClose", inClose, guardInLen);
      requireLength("ADOSC", "inVolume", inVolume, guardInLen);
      requireLength("ADOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADOSC_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chaikin A/D Oscillator: the difference between a fast and a slow EMA of
    * the Accumulation/Distribution line. Highlights momentum in
    * accumulation/distribution volume flow. Positive/rising suggests
    * accumulation; negative/falling suggests distribution.
    * <p><b>Formula</b>
    * <pre>{@code
    * ad += ((close-low)-(high-close))/(high-low) * volume   (only when high>low)
    * fastEMA = fastk*ad + (1-fastk)*fastEMA,  fastk = 2/(optInFastPeriod+1)
    * slowEMA = slowk*ad + (1-slowk)*slowEMA,  slowk = 2/(optInSlowPeriod+1)
    * ADOSC = fastEMA - slowEMA
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ADOSC_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInFastPeriod Period of the fast A/D EMA (default 3; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow A/D EMA (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Fast-EMA minus slow-EMA of the A/D line. Must hold at least
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
    * @see Core#AD
    * @see Core#EMA
    */
   public OutRange ADOSC( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          float inVolume[],
                          int optInFastPeriod,
                          int optInSlowPeriod,
                          double outReal[] )
   {
      requireIndexRange("ADOSC", startIdx, endIdx);
      int guardStart = clampedStart("ADOSC", startIdx, ADOSC_Lookback(optInFastPeriod, optInSlowPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ADOSC", "inHigh", inHigh, guardInLen);
      requireLength("ADOSC", "inLow", inLow, guardInLen);
      requireLength("ADOSC", "inClose", inClose, guardInLen);
      requireLength("ADOSC", "inVolume", inVolume, guardInLen);
      requireLength("ADOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ADOSC_Impl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ADOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ADOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ADOSC} over the same series.
    * Open with {@link Core#ADOSC_Open}; there is no close — the handle is
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
   public static final class ADOSC_Stream {
      Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      double slowEMA;
      double slowk;
      double one_minus_slowk;
      double fastEMA;
      double fastk;
      double one_minus_fastk;
      double ad;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      ADOSC_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ADOSC} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      ADOSC_Stream( ADOSC_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.slowEMA = other.slowEMA;
         this.slowk = other.slowk;
         this.one_minus_slowk = other.one_minus_slowk;
         this.fastEMA = other.fastEMA;
         this.fastk = other.fastk;
         this.one_minus_fastk = other.one_minus_fastk;
         this.ad = other.ad;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( ADOSC_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.slowEMA = other.slowEMA;
         this.slowk = other.slowk;
         this.one_minus_slowk = other.one_minus_slowk;
         this.fastEMA = other.fastEMA;
         this.fastk = other.fastk;
         this.one_minus_fastk = other.one_minus_fastk;
         this.ad = other.ad;
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
      public double update( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("ADOSC update: BadParam", RetCode.BadParam);
         core.ADOSC_StepImpl(this, inHigh, inLow, inClose, inVolume);
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], double outReal[] ) {
         requireArgument("ADOSC updateAndFill", "inHigh", inHigh);
         requireArgument("ADOSC updateAndFill", "inLow", inLow);
         requireArgument("ADOSC updateAndFill", "inClose", inClose);
         requireArgument("ADOSC updateAndFill", "inVolume", inVolume);
         requireArgument("ADOSC updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("ADOSC updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) )
               throw new TaLibArgumentException("ADOSC updateAndFill: BadParam", RetCode.BadParam);
            core.ADOSC_StepImpl(this, inHigh[i], inLow[i], inClose[i], inVolume[i]);
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
      public double peek( double inHigh, double inLow, double inClose, double inVolume ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("ADOSC peek: BadParam", RetCode.BadParam);
         ADOSC_Stream scratch = new ADOSC_Stream(this);
         core.ADOSC_StepImpl(scratch, inHigh, inLow, inClose, inVolume);
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
      public ADOSC_Stream copy() {
         return new ADOSC_Stream(this);
      }
   }
   void ADOSC_StepImpl( ADOSC_Stream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double high = 0.0;
      double low = 0.0;
      double close = 0.0;
      double tmp = 0.0;
      high = inHigh;
      low = inLow;
      tmp = high - low;
      close = inClose;
      if( tmp > 0.0 ) {
         sp.ad += (close - low - (high - close)) / tmp * (double)inVolume;
      }
      sp.fastEMA = Math.fma(sp.one_minus_fastk, sp.fastEMA, sp.fastk * sp.ad);
      sp.slowEMA = Math.fma(sp.one_minus_slowk, sp.slowEMA, sp.slowk * sp.ad);
      sp.cur_outReal = sp.fastEMA - sp.slowEMA;
   }
   private RetCode ADOSC_OpenImpl( ADOSC_Stream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int slowestPeriod = 0;
      double high = 0;
      double low = 0;
      double close = 0;
      double tmp = 0;
      double slowEMA = 0;
      double slowk = 0;
      double one_minus_slowk = 0;
      double fastEMA = 0;
      double fastk = 0;
      double one_minus_fastk = 0;
      double ad = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 3;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 10;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Implementation Note:
       *     The fastEMA varaible is not neceseraly the
       *     fastest EMA.
       *     In the same way, slowEMA is not neceseraly the
       *     slowest EMA.
       *
       *     The ADOSC is always the (fastEMA - slowEMA) regardless
       *     of the period specified. In other word:
       *
       *     ADOSC(3,10) = EMA(3,AD) - EMA(10,AD)
       *
       *        while
       *
       *     ADOSC(10,3) = EMA(10,AD)- EMA(3,AD)
       *
       *     In the first case the EMA(3) is truly a faster EMA,
       *     while in the second case, the EMA(10) is still call
       *     fastEMA in the algorithm, even if it is in fact slower.
       *
       *     This gives more flexibility to the user if they want to
       *     experiment with unusual parameter settings.
       */
      /* Identify the slowest period.
       * This infomration is used soleley to bootstrap
       * the algorithm (skip the lookback period).
       */
      if( optInFastPeriod < optInSlowPeriod ) {
         slowestPeriod = optInSlowPeriod;
      } else {
         slowestPeriod = optInFastPeriod;
      }
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = EMA_Lookback(slowestPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal;
      /* The following variables are used to
       * calculate the "ad".
       */
      ad = 0.0;
      /* Constants for EMA */
      fastk = 2.0 / ((double)optInFastPeriod + 1.0);
      one_minus_fastk = 1.0 - fastk;
      slowk = 2.0 / ((double)optInSlowPeriod + 1.0);
      one_minus_slowk = 1.0 - slowk;
      /* Initialize the two EMA
       *
       * Use the same range of initialization inputs for
       * both EMA and simply seed with the first A/D value.
       *
       * Note: Metastock do the same.
       */
      high = inHigh[today];
      low = inLow[today];
      tmp = high - low;
      close = inClose[today];
      if( tmp > 0.0 ) {
         ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
      }
      today += 1;
      fastEMA = ad;
      slowEMA = ad;
      /* Initialize the EMA and skip the unstable period. */
      while( today < startIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
      }
      /* Perform the calculation for the requested range */
      outIdx = 0;
      while( today <= endIdx ) {
         high = inHigh[today];
         low = inLow[today];
         tmp = high - low;
         close = inClose[today];
         if( tmp > 0.0 ) {
            ad += (close - low - (high - close)) / tmp * (double)inVolume[today];
         }
         today += 1;
         fastEMA = Math.fma(one_minus_fastk, fastEMA, fastk * ad);
         slowEMA = Math.fma(one_minus_slowk, slowEMA, slowk * ad);
         outReal[outIdx++ * outStride] = fastEMA - slowEMA;
      }
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.slowEMA = slowEMA;
      sp.slowk = slowk;
      sp.one_minus_slowk = one_minus_slowk;
      sp.fastEMA = fastEMA;
      sp.fastk = fastk;
      sp.one_minus_fastk = one_minus_fastk;
      sp.ad = ad;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* ADOSC_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ADOSC_Stream ADOSC_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      ADOSC_Stream sp = new ADOSC_Stream(this);
      RetCode retCode = ADOSC_OpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADOSC openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ADOSC openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind ADOSC_Open (composition seam). */
   ADOSC_Stream ADOSC_OpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod )
   {
      ADOSC_Stream sp = new ADOSC_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ADOSC_OpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ADOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ADOSC open: internal error", retCode);
      }
      throw new TaLibArgumentException("ADOSC open: " + retCode, retCode);
   }
   /**
    * Open a live ADOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ADOSC} at that bar.
    * <p>The history must hold at least {@code ADOSC_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ADOSC_Stream ADOSC_Open( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod )
   {
      requireArgument("ADOSC open", "inHigh", inHigh);
      requireHistory("ADOSC open", inHigh.length);
      requireArgument("ADOSC open", "inLow", inLow);
      requireArgument("ADOSC open", "inClose", inClose);
      requireArgument("ADOSC open", "inVolume", inVolume);
      requireHistoryLength("ADOSC open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ADOSC open", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("ADOSC open", "inVolume", inVolume.length, inHigh.length);
      return ADOSC_OpenInternal(inHigh, inLow, inClose, inVolume, 0, optInFastPeriod, optInSlowPeriod);
   }
   /**
    * {@link Core#ADOSC_Open} that also fills the output array(s) bit-identically
    * to {@link Core#ADOSC} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ADOSC_Stream#outRange()}.
    */
   public ADOSC_Stream ADOSC_OpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod, double outReal[] )
   {
      requireArgument("ADOSC openAndFill", "inHigh", inHigh);
      requireHistory("ADOSC openAndFill", inHigh.length);
      requireArgument("ADOSC openAndFill", "inLow", inLow);
      requireArgument("ADOSC openAndFill", "inClose", inClose);
      requireArgument("ADOSC openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("ADOSC openAndFill", inHigh.length, ADOSC_Lookback(optInFastPeriod, optInSlowPeriod));
      requireHistoryLength("ADOSC openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ADOSC openAndFill", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("ADOSC openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("ADOSC openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("ADOSC openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ADOSC_OpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, optInFastPeriod, optInSlowPeriod, outBegIdx, outNBElement, outReal);
   }
