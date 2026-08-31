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
 *  082326 MF,CC  Fix #253. Test the close exactly instead of against the fixed
 *                TA_IS_ZERO band, which zeroed the output for any instrument
 *                quoted small enough to fall under it.
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
         /* NATR is the ATR as a percentage of the close, so it is scale-free and
          * the divisor only has to be non-zero. An exact test, not the fixed
          * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
          * band zeroed the whole output for any instrument quoted below it (#253).
          */
         tempValue = inClose[startIdx];
         if( tempValue != 0.0 ) {
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
            if( tempValue != 0.0 ) {
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
         if( tempValue != 0.0 ) {
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
            if( tempValue != 0.0 ) {
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("NATR", startIdx, NATR_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("NATR", startIdx, NATR_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#natrOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class NatrStream {
      Core core;
      int optInTimePeriod;
      double prevATR;
      double lag1_inClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      NatrStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#NATR} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      NatrStream( NatrStream other ) {
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
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("NATR update: BadParam", RetCode.BadParam);
         }
         core.natrStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("NATR updateAndFill", "inHigh", inHigh);
         requireArgument("NATR updateAndFill", "inLow", inLow);
         requireArgument("NATR updateAndFill", "inClose", inClose);
         requireArgument("NATR updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("NATR updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("NATR updateAndFill: BadParam", RetCode.BadParam);
            }
            core.natrStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("NATR peek: BadParam", RetCode.BadParam);
         NatrStream sp = this;
         double tempValue = 0.0;
         double val2 = 0.0;
         double val3 = 0.0;
         double greatest = 0.0;
         double tempCY = 0.0;
         double tempLT = 0.0;
         double tempHT = 0.0;
         double cur_outReal = sp.cur_outReal;
         double lag1_inClose = sp.lag1_inClose;
         double prevATR = sp.prevATR;
         /* Find the greatest of the 3 values. */
         tempLT = inLow;
         tempHT = inHigh;
         tempCY = lag1_inClose;
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
         prevATR *= sp.optInTimePeriod - 1;
         prevATR += greatest;
         prevATR /= sp.optInTimePeriod;
         if( sp.optInTimePeriod <= 1 ) {
            /* No smoothing: emit the raw True Range (unnormalized). */
            cur_outReal = prevATR;
         } else {
            tempValue = inClose;
            if( tempValue != 0.0 ) {
               cur_outReal = prevATR / tempValue * 100.0;
            } else {
               cur_outReal = 0.0;
            }
         }
         lag1_inClose = inClose;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public NatrStream clone() {
         return new NatrStream(this);
      }
   }
   void natrStepImpl( NatrStream sp, double inHigh, double inLow, double inClose )
   {
      double tempValue = 0.0;
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
      if( sp.optInTimePeriod <= 1 ) {
         /* No smoothing: emit the raw True Range (unnormalized). */
         sp.cur_outReal = sp.prevATR;
      } else {
         tempValue = inClose;
         if( tempValue != 0.0 ) {
            sp.cur_outReal = sp.prevATR / tempValue * 100.0;
         } else {
            sp.cur_outReal = 0.0;
         }
      }
      sp.lag1_inClose = inClose;
   }
   private RetCode natrOpenImpl( NatrStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
         /* NATR is the ATR as a percentage of the close, so it is scale-free and
          * the divisor only has to be non-zero. An exact test, not the fixed
          * TA_IS_ZERO band it used to be: a close carries the quote unit, and that
          * band zeroed the whole output for any instrument quoted below it (#253).
          */
         tempValue = inClose[startIdx];
         if( tempValue != 0.0 ) {
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
            if( tempValue != 0.0 ) {
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
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* natrOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   NatrStream natrOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      NatrStream sp = new NatrStream(this);
      RetCode retCode = natrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
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
   /* Internal startIdx-anchored open behind natrOpen (composition seam). */
   NatrStream natrOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      NatrStream sp = new NatrStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = natrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
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
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public NatrStream natrOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("NATR open", "inHigh", inHigh);
      requireHistory("NATR open", inHigh.length);
      requireArgument("NATR open", "inLow", inLow);
      requireArgument("NATR open", "inClose", inClose);
      requireHistoryLength("NATR open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("NATR open", "inClose", inClose.length, inHigh.length);
      return natrOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#natrOpen} that also fills the output array(s) bit-identically
    * to {@link Core#NATR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link NatrStream#outRange()}.
    */
   public NatrStream natrOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("NATR openAndFill", "inHigh", inHigh);
      requireHistory("NATR openAndFill", inHigh.length);
      requireArgument("NATR openAndFill", "inLow", inLow);
      requireArgument("NATR openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("NATR openAndFill", inHigh.length, NATR_Lookback(optInTimePeriod));
      requireHistoryLength("NATR openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("NATR openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("NATR openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("NATR openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return natrOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
