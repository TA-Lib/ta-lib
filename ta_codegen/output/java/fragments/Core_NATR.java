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
 *  060306 MF     Initial Version
 *  070526 MF,CC  Fix #98: partial-range calls normalized with a close
 *                from the wrong bar (TR-buffer-relative index).
 *  070626 MF,CC  Speed optimization: True Range computed inline in a
 *                single pass (bit-exact, no temporary buffer).
 */

   /**
    * Number of leading input bars {@link Core#NATR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Smoothing period for the true range average
    *        (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int NATR_Lookback( int optInTimePeriod )
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
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.NATR.ordinal()] ;

   }
   RetCode NATR_Impl( int startIdx,
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
      double tempValue = 0;
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
      /* This function is very similar as ATR, except
       * it is being normalized as follow:
       *
       *    NATR = (ATR(period) / Close) * 100
       *
       *
       * Normalization make the ATR function more relevant
       * in the folllowing scenario:
       *    - Long term analysis where the price changes drastically.
       *    - Cross-market or cross-security ATR comparison.
       *
       * More Info:
       *      Technical Analysis of Stock & Commodities (TASC)
       *      May 2006 by John Forman
       */
      /* Average True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * These value are averaged for the specified period using
       * Wilder method. This method have an unstable period comparable
       * to an Exponential Moving Average (EMA).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = NATR_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Period 1 needs no smoothing: the Wilder recursion below degenerates
       * to the raw True Range at every bar (prevATR = (prevATR*0 + TR)/1 = TR).
       * At period 1 the output is left as that raw True Range (unnormalized),
       * matching the historical TRANGE-delegation behavior; every period > 1 is
       * normalized by the close. The single general path handles all period >= 1.
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
       * Each output is normalized by the close of its own bar; a
       * close of zero yields 0.0.
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
      i = this.unstablePeriod[FuncUnstId.NATR.ordinal()];
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
      /* Now start to write the final NATR in the caller
       * provided outReal.
       */
      outIdx = 1;
      if( optInTimePeriod <= 1 ) {
         /* No smoothing: emit the raw True Range (unnormalized). */
         outReal[0] = prevATR;
      } else {
         tempValue = inClose[startIdx];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[0] = prevATR / tempValue * 100.0;
         } else {
            outReal[0] = 0.0;
         }
      }
      /* Now do the number of requested NATR. */
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
         if( optInTimePeriod <= 1 ) {
            /* No smoothing: emit the raw True Range (unnormalized). */
            outReal[outIdx] = prevATR;
         } else {
            tempValue = inClose[today];
            if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
               outReal[outIdx] = prevATR / tempValue * 100.0;
            } else {
               outReal[outIdx] = 0.0;
            }
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode NATR_Impl( int startIdx,
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
      double tempValue = 0;
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
      lookbackTotal = NATR_Lookback(optInTimePeriod);
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
      i = this.unstablePeriod[FuncUnstId.NATR.ordinal()];
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
      if( optInTimePeriod <= 1 ) {
         outReal[0] = prevATR;
      } else {
         tempValue = (double)inClose[startIdx];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[0] = prevATR / tempValue * 100.0;
         } else {
            outReal[0] = 0.0;
         }
      }
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
         if( optInTimePeriod <= 1 ) {
            outReal[outIdx] = prevATR;
         } else {
            tempValue = (double)inClose[today];
            if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
               outReal[outIdx] = prevATR / tempValue * 100.0;
            } else {
               outReal[outIdx] = 0.0;
            }
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Average True Range expressed as a percentage of the current close, making
    * volatility comparable across price levels and securities. Same computation
    * as ATR, then normalized by close. Higher values mean greater relative
    * volatility; unit is percent of price.
    * <p><b>Formula</b>
    * <pre>{@code
    * NATR = (ATR / Close) * 100
    * ATR: first value = SMA of TRANGE over period; then Wilder smoothing ATR_t = (ATR_{t-1}*(period-1) + TR_t) / period
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#NATR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period for the true range average
    *        (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal ATR as a percentage of the close. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#ATR
    * @see Core#TRANGE
    * @see Core#SMA
    */
   public OutRange NATR( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         double inClose[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("NATR", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, NATR_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("NATR", "inHigh", inHigh, guardInLen);
      requireLength("NATR", "inLow", inLow, guardInLen);
      requireLength("NATR", "inClose", inClose, guardInLen);
      requireLength("NATR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = NATR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("NATR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Average True Range expressed as a percentage of the current close, making
    * volatility comparable across price levels and securities. Same computation
    * as ATR, then normalized by close. Higher values mean greater relative
    * volatility; unit is percent of price.
    * <p><b>Formula</b>
    * <pre>{@code
    * NATR = (ATR / Close) * 100
    * ATR: first value = SMA of TRANGE over period; then Wilder smoothing ATR_t = (ATR_{t-1}*(period-1) + TR_t) / period
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#NATR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period for the true range average
    *        (default 14; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal ATR as a percentage of the close. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#ATR
    * @see Core#TRANGE
    * @see Core#SMA
    */
   public OutRange NATR( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         float inClose[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("NATR", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, NATR_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("NATR", "inHigh", inHigh, guardInLen);
      requireLength("NATR", "inLow", inLow, guardInLen);
      requireLength("NATR", "inClose", inClose, guardInLen);
      requireLength("NATR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = NATR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("NATR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live NATR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#NATR} over the same series.
    * Open with {@link Core#NATR_Open}; there is no close — the handle is
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
   public static final class NATR_Stream {
      Core core;
      int optInTimePeriod;
      double prevATR;
      double tempValue;
      double val3;
      double lag1_inClose;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      NATR_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#NATR_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      NATR_Stream( NATR_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevATR = other.prevATR;
         this.tempValue = other.tempValue;
         this.val3 = other.val3;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( NATR_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevATR = other.prevATR;
         this.tempValue = other.tempValue;
         this.val3 = other.val3;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
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
            throw new TaLibArgumentException("NATR update: BadParam", RetCode.BadParam);
         core.NATR_StreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
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
            throw new TaLibArgumentException("NATR peek: BadParam", RetCode.BadParam);
         NATR_Stream scratch = new NATR_Stream(this);
         core.NATR_StreamStep(scratch, inHigh, inLow, inClose);
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
      public NATR_Stream copy() {
         return new NATR_Stream(this);
      }
   }
   void NATR_StreamStep( NATR_Stream sp, double inHigh, double inLow, double inClose )
   {
      double val2 = 0.0;
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
      sp.val3 = Math.abs(tempCY - tempLT);
      if( sp.val3 > greatest ) {
         greatest = sp.val3;
      }
      sp.prevATR *= sp.optInTimePeriod - 1;
      sp.prevATR += greatest;
      sp.prevATR /= sp.optInTimePeriod;
      if( sp.optInTimePeriod <= 1 ) {
         /* No smoothing: emit the raw True Range (unnormalized). */
         sp.cur_outReal = sp.prevATR;
      } else {
         sp.tempValue = inClose;
         if( !((-0.00000000000001 < sp.tempValue) && (sp.tempValue < 0.00000000000001)) ) {
            sp.cur_outReal = sp.prevATR / sp.tempValue * 100.0;
         } else {
            sp.cur_outReal = 0.0;
         }
      }
      sp.lag1_inClose = inClose;
   }
   private RetCode NATR_OpenPass( NATR_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int nbATR = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double tempValue = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* This function is very similar as ATR, except
       * it is being normalized as follow:
       *
       *    NATR = (ATR(period) / Close) * 100
       *
       *
       * Normalization make the ATR function more relevant
       * in the folllowing scenario:
       *    - Long term analysis where the price changes drastically.
       *    - Cross-market or cross-security ATR comparison.
       *
       * More Info:
       *      Technical Analysis of Stock & Commodities (TASC)
       *      May 2006 by John Forman
       */
      /* Average True Range is the greatest of the following:
       *
       *  val1 = distance from today's high to today's low.
       *  val2 = distance from yesterday's close to today's high.
       *  val3 = distance from yesterday's close to today's low.
       *
       * These value are averaged for the specified period using
       * Wilder method. This method have an unstable period comparable
       * to an Exponential Moving Average (EMA).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = NATR_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* Period 1 needs no smoothing: the Wilder recursion below degenerates
       * to the raw True Range at every bar (prevATR = (prevATR*0 + TR)/1 = TR).
       * At period 1 the output is left as that raw True Range (unnormalized),
       * matching the historical TRANGE-delegation behavior; every period > 1 is
       * normalized by the close. The single general path handles all period >= 1.
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
       * Each output is normalized by the close of its own bar; a
       * close of zero yields 0.0.
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
      i = this.unstablePeriod[FuncUnstId.NATR.ordinal()];
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
      /* Now start to write the final NATR in the caller
       * provided outReal.
       */
      outIdx = 1;
      if( optInTimePeriod <= 1 ) {
         /* No smoothing: emit the raw True Range (unnormalized). */
         outReal[0 * outStride] = prevATR;
      } else {
         tempValue = inClose[startIdx];
         if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
            outReal[0 * outStride] = prevATR / tempValue * 100.0;
         } else {
            outReal[0 * outStride] = 0.0;
         }
      }
      /* Now do the number of requested NATR. */
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
         if( optInTimePeriod <= 1 ) {
            /* No smoothing: emit the raw True Range (unnormalized). */
            outReal[outIdx * outStride] = prevATR;
         } else {
            tempValue = inClose[today];
            if( !((-0.00000000000001 < tempValue) && (tempValue < 0.00000000000001)) ) {
               outReal[outIdx * outStride] = prevATR / tempValue * 100.0;
            } else {
               outReal[outIdx * outStride] = 0.0;
            }
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevATR = prevATR;
      sp.tempValue = tempValue;
      sp.val3 = val3;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode NATR_OpenImpl( NATR_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return NATR_OpenPass( sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode NATR_OpenAndFillImpl( NATR_Stream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return NATR_OpenPass( sp, inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode NATR_OpenAndFillInternalImpl( NATR_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return NATR_OpenPass(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* NATR_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   NATR_Stream NATR_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      NATR_Stream sp = new NATR_Stream(this);
      RetCode retCode = NATR_OpenAndFillInternalImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("NATR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("NATR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("NATR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind NATR_Open (composition seam). */
   NATR_Stream NATR_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      NATR_Stream sp = new NATR_Stream(this);
      RetCode retCode = NATR_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("NATR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("NATR open: internal error", retCode);
      }
      throw new TaLibArgumentException("NATR open: " + retCode, retCode);
   }
   /**
    * Open a live NATR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#NATR} at that bar.
    * <p>The history must hold at least {@code NATR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public NATR_Stream NATR_Open( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      return NATR_OpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#NATR_Open} that also fills the output array(s) bit-identically
    * to {@link Core#NATR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link NATR_Stream#fillRange()}.
    */
   public NATR_Stream NATR_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      NATR_Stream sp = new NATR_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = NATR_OpenAndFillImpl(sp, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("NATR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("NATR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("NATR openAndFill: " + retCode, retCode);
   }
