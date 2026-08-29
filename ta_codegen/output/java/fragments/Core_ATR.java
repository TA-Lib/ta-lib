/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 */

   /**
    * Number of leading input bars {@link Core#ATR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Smoothing period (default 14; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ATR_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* The ATR lookback is the sum of:
       *    1 + (optInTimePeriod - 1)
       *
       * Where 1 is for the True Range, and
       * (optInTimePeriod-1) is for the simple
       * moving average.
       */
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.ATR.ordinal()] ;

   }
   RetCode ATR_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Average True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * These value are averaged for the specified period using
       * Wilder method. This method have an unstable period comparable
       * to and Exponential Moving Average (EMA).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = ATR_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Period 1 needs no smoothing: the Wilder recursion below degenerates
       * to the raw True Range at every bar (prevATR = (prevATR*0 + TR)/1 = TR),
       * so the single general path handles every period >= 1.
       */
      /* The True Range of each bar is computed inline in a single
       * pass. No temporary buffer is needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - True Range: start from high-low, then compare/replace
       *    with the two previous-close distances, in that order.
       *  - Seed: the first 'period' True Range values are summed,
       *    accumulated from 0.0 in input order, then divided by
       *    the period.
       *  - Wilder smoothing: multiply by period-1, add the True
       *    Range, divide by period, as three separate statements.
       *
       * In-place (outReal being one of the input arrays) is
       * supported: each output is written only after every input
       * read at or before its bar, and the output index is always
       * smaller than the bar index of any remaining read.
       */
      /* The first True Range needs the two price bars at
       * startIdx-lookbackTotal+1 (a previous close is consumed).
       */
      today = startIdx - lookbackTotal + 1;
      /* Seed the ATR with a simple average of the True Range
       * for the first 'period' bars.
       */
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      /* Subsequent value are smoothed using the
       * previous ATR value (Wilder's approach).
       *  1) Multiply the previous ATR by 'period-1'.
       *  2) Add today TR value.
       *  3) Divide by 'period'.
       */
      /* Skip the unstable period. */
      i = this.unstablePeriod[FuncUnstId.ATR.ordinal()];
      while( i != 0 ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR *= optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= optInTimePeriod;
         today += 1;
         i -= 1;
      }
      /* Now start to write the final ATR in the caller
       * provided outReal.
       */
      outIdx = 1;
      outReal[0] = prevATR;
      /* Now do the number of requested ATR. */
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR *= optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= optInTimePeriod;
         outReal[outIdx++] = prevATR;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode ATR_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ATR_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      i = this.unstablePeriod[FuncUnstId.ATR.ordinal()];
      while( i != 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR *= optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= optInTimePeriod;
         today += 1;
         i -= 1;
      }
      outIdx = 1;
      outReal[0] = prevATR;
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR *= optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= optInTimePeriod;
         outReal[outIdx++] = prevATR;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Wilder-smoothed average of the True Range over a period, measuring price
    * volatility regardless of direction. Higher ATR means greater volatility;
    * no directional bias.
    * <p><b>Formula</b>
    * <pre>{@code
    * TR_t = max(high-low, |prevClose-high|, |prevClose-low|)
    * ATR seed = simple average of first `period` TR values
    * ATR_t = (ATR_{t-1} * (period-1) + TR_t) / period
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ATR_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period (default 14; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Average True Range value. Must hold at least
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
    * @see Core#TRANGE
    * @see Core#NATR
    * @see Core#SMA
    * @see Core#EMA
    */
   public OutRange ATR( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("ATR", startIdx, endIdx);
      int guardStart = clampedStart("ATR", startIdx, ATR_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ATR", "inHigh", inHigh, guardInLen);
      requireLength("ATR", "inLow", inLow, guardInLen);
      requireLength("ATR", "inClose", inClose, guardInLen);
      requireLength("ATR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ATR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ATR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Wilder-smoothed average of the True Range over a period, measuring price
    * volatility regardless of direction. Higher ATR means greater volatility;
    * no directional bias.
    * <p><b>Formula</b>
    * <pre>{@code
    * TR_t = max(high-low, |prevClose-high|, |prevClose-low|)
    * ATR seed = simple average of first `period` TR values
    * ATR_t = (ATR_{t-1} * (period-1) + TR_t) / period
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ATR_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period (default 14; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Average True Range value. Must hold at least
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
    * @see Core#TRANGE
    * @see Core#NATR
    * @see Core#SMA
    * @see Core#EMA
    */
   public OutRange ATR( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("ATR", startIdx, endIdx);
      int guardStart = clampedStart("ATR", startIdx, ATR_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ATR", "inHigh", inHigh, guardInLen);
      requireLength("ATR", "inLow", inLow, guardInLen);
      requireLength("ATR", "inClose", inClose, guardInLen);
      requireLength("ATR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ATR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ATR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ATR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ATR} over the same series.
    * Open with {@link Core#atrOpen}; there is no close — the handle is
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
   public static final class AtrStream {
      Core core;
      int optInTimePeriod;
      double prevATR;
      double lag1_inClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AtrStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ATR} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AtrStream( AtrStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevATR = other.prevATR;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AtrStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevATR = other.prevATR;
         this.lag1_inClose = other.lag1_inClose;
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
            throw new TaLibArgumentException("ATR update: BadParam", RetCode.BadParam);
         core.atrStepImpl(this, inHigh, inLow, inClose);
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
         requireArgument("ATR updateAndFill", "inHigh", inHigh);
         requireArgument("ATR updateAndFill", "inLow", inLow);
         requireArgument("ATR updateAndFill", "inClose", inClose);
         requireArgument("ATR updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("ATR updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("ATR updateAndFill: BadParam", RetCode.BadParam);
            core.atrStepImpl(this, inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("ATR peek: BadParam", RetCode.BadParam);
         AtrStream scratch = new AtrStream(this);
         core.atrStepImpl(scratch, inHigh, inLow, inClose);
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
      public AtrStream copy() {
         return new AtrStream(this);
      }
   }
   void atrStepImpl( AtrStream sp, double inHigh, double inLow, double inClose )
   {
      double val2 = 0.0;
      double val3 = 0.0;
      double greatest = 0.0;
      double tempCY = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      /* Find the greatest of the 3 values. */
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      greatest = tempHT - tempLT;
      /* val1 */
      val2 = Math.abs(tempCY - tempHT);
      if( val2 > greatest ) {
         greatest = val2;
      }
      val3 = Math.abs(tempCY - tempLT);
      if( val3 > greatest ) {
         greatest = val3;
      }
      sp.prevATR *= sp.optInTimePeriod - 1;
      sp.prevATR += greatest;
      sp.prevATR /= sp.optInTimePeriod;
      sp.cur_outReal = sp.prevATR;
      sp.lag1_inClose = inClose;
   }
   private RetCode atrOpenImpl( AtrStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
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
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Average True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * These value are averaged for the specified period using
       * Wilder method. This method have an unstable period comparable
       * to and Exponential Moving Average (EMA).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = ATR_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* Period 1 needs no smoothing: the Wilder recursion below degenerates
       * to the raw True Range at every bar (prevATR = (prevATR*0 + TR)/1 = TR),
       * so the single general path handles every period >= 1.
       */
      /* The True Range of each bar is computed inline in a single
       * pass. No temporary buffer is needed.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - True Range: start from high-low, then compare/replace
       *    with the two previous-close distances, in that order.
       *  - Seed: the first 'period' True Range values are summed,
       *    accumulated from 0.0 in input order, then divided by
       *    the period.
       *  - Wilder smoothing: multiply by period-1, add the True
       *    Range, divide by period, as three separate statements.
       *
       * In-place (outReal being one of the input arrays) is
       * supported: each output is written only after every input
       * read at or before its bar, and the output index is always
       * smaller than the bar index of any remaining read.
       */
      /* The first True Range needs the two price bars at
       * startIdx-lookbackTotal+1 (a previous close is consumed).
       */
      today = startIdx - lookbackTotal + 1;
      /* Seed the ATR with a simple average of the True Range
       * for the first 'period' bars.
       */
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      /* Subsequent value are smoothed using the
       * previous ATR value (Wilder's approach).
       *  1) Multiply the previous ATR by 'period-1'.
       *  2) Add today TR value.
       *  3) Divide by 'period'.
       */
      /* Skip the unstable period. */
      i = this.unstablePeriod[FuncUnstId.ATR.ordinal()];
      while( i != 0 ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR *= optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= optInTimePeriod;
         today += 1;
         i -= 1;
      }
      /* Now start to write the final ATR in the caller
       * provided outReal.
       */
      outIdx = 1;
      outReal[0 * outStride] = prevATR;
      /* Now do the number of requested ATR. */
      nbATR = endIdx - startIdx + 1;
      while( --nbATR != 0 ) {
         /* Find the greatest of the 3 values. */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR *= optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= optInTimePeriod;
         outReal[outIdx++ * outStride] = prevATR;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevATR = prevATR;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* atrOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AtrStream atrOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AtrStream sp = new AtrStream(this);
      RetCode retCode = atrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ATR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ATR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ATR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind atrOpen (composition seam). */
   AtrStream atrOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      AtrStream sp = new AtrStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = atrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ATR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ATR open: internal error", retCode);
      }
      throw new TaLibArgumentException("ATR open: " + retCode, retCode);
   }
   /**
    * Open a live ATR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ATR} at that bar.
    * <p>The history must hold at least {@code ATR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AtrStream atrOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("ATR open", "inHigh", inHigh);
      requireHistory("ATR open", inHigh.length);
      requireArgument("ATR open", "inLow", inLow);
      requireArgument("ATR open", "inClose", inClose);
      requireHistoryLength("ATR open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ATR open", "inClose", inClose.length, inHigh.length);
      return atrOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#atrOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ATR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AtrStream#outRange()}.
    */
   public AtrStream atrOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("ATR openAndFill", "inHigh", inHigh);
      requireHistory("ATR openAndFill", inHigh.length);
      requireArgument("ATR openAndFill", "inLow", inLow);
      requireArgument("ATR openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("ATR openAndFill", inHigh.length, ATR_Lookback(optInTimePeriod));
      requireHistoryLength("ATR openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ATR openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("ATR openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("ATR openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return atrOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
