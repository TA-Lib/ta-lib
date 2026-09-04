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
 *  090426 MF,CC  First version. See issue #345.
 */

   /**
    * Number of leading input bars {@link Core#FOSC} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the regression window (default 5;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int FOSC_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode FOSC_Impl( int startIdx,
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
      double m = 0;
      double b = 0;
      double closeValue = 0;
      int i = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double trailingValue = 0;
      double weightedTrailing = 0;
      double sumAbs = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* The forecast compared against the close is the one made one bar EARLIER:
       * FOSC[t] = 100*(inReal[t] - TSF[t-1])/inReal[t]. The regression window
       * ends at t-1, so the lookback is period, not period-1.
       *
       * The window arithmetic below is TA_TSF's verbatim -- priming scan, O(1)
       * recurrence, both re-anchor triggers (#254) and the fused
       * `b + m*(double)optInTimePeriod` -- which is what makes FOSC bit-identical
       * to a TA_TSF call anchored one bar earlier. Reshaping any of it breaks
       * that silently.
       *
       * trailingValue -- the value the NEXT bar's window drops -- is read before
       * the output write because with outReal==inReal (#130) that write lands on
       * exactly that cell whenever startIdx is the clamped minimum. closeValue
       * carries no such constraint: it sits startIdx bars ahead of the cursor.
       */
      lookbackTotal = FOSC_Lookback(optInTimePeriod);
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
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      sumAbs = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - 1 - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += Math.abs(tempValue1);
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = inReal[trailingIdx];
      trailingIdx += 1;
      closeValue = inReal[today];
      if( closeValue != 0.0 ) {
         outReal[outIdx++] = 100.0 * (closeValue - (Math.fma(m, (double)optInTimePeriod, b))) / closeValue;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         weightedTrailing = (double)optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + inReal[today - 1];
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today - 1]);
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)(optInTimePeriod - 1);
            for( j = windowStart; j < today; j += 1 ) {
               tempValue1 = inReal[j];
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         trailingValue = inReal[trailingIdx];
         trailingIdx += 1;
         closeValue = inReal[today];
         if( closeValue != 0.0 ) {
            outReal[outIdx++] = 100.0 * (closeValue - (Math.fma(m, (double)optInTimePeriod, b))) / closeValue;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode FOSC_Impl( int startIdx,
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
      double m = 0;
      double b = 0;
      double closeValue = 0;
      int i = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double trailingValue = 0;
      double weightedTrailing = 0;
      double sumAbs = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = FOSC_Lookback(optInTimePeriod);
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
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      sumAbs = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = (double)inReal[today - 1 - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += Math.abs(tempValue1);
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = (double)inReal[trailingIdx];
      trailingIdx += 1;
      closeValue = (double)inReal[today];
      if( closeValue != 0.0 ) {
         outReal[outIdx++] = 100.0 * (closeValue - (Math.fma(m, (double)optInTimePeriod, b))) / closeValue;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         weightedTrailing = (double)optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + (double)inReal[today - 1];
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs((double)inReal[today - 1]);
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)(optInTimePeriod - 1);
            for( j = windowStart; j < today; j += 1 ) {
               tempValue1 = (double)inReal[j];
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         trailingValue = (double)inReal[trailingIdx];
         trailingIdx += 1;
         closeValue = (double)inReal[today];
         if( closeValue != 0.0 ) {
            outReal[outIdx++] = 100.0 * (closeValue - (Math.fma(m, (double)optInTimePeriod, b))) / closeValue;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Forecast Oscillator: the percentage by which the close deviates from the
    * Time Series Forecast that was made one bar earlier for the current bar.
    * Positive means price came in above what the regression projected, negative
    * below; the value oscillates around zero and crosses it whenever price
    * meets its own forecast. Persistent readings far from zero say the trend is
    * running ahead of, or lagging, its regression line.
    * <p><b>Formula</b>
    * <pre>{@code
    * FOSC[t] = 100 * (P[t] - TSF[t-1]) / P[t], where TSF[t-1] is the Time Series Forecast fitted over the N bars ending at t-1 and evaluated one x-step beyond that window — the forecast for bar t made without seeing it.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Several vendors publish a "Chande Forecast Oscillator (CFO)" that compares the close to the regression value of the window *ending at the same bar*, with no lag. FOSC is the lagged form Chande and Achelis describe.</li>
    * <li>The default window is Chande's own suggestion, shorter than the one TA-Lib's TSF and LINEARREG default to.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#FOSC_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Number of bars in the regression window (default 5;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Percentage deviation of the close from the previous bar's
    *        forecast. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#TSF
    * @see Core#LINEARREG
    * @see Core#CMOU
    */
   public OutRange FOSC( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("FOSC", startIdx, endIdx);
      int guardStart = clampedStart("FOSC", startIdx, FOSC_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FOSC", "inReal", inReal, guardInLen);
      requireLength("FOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = FOSC_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("FOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Forecast Oscillator: the percentage by which the close deviates from the
    * Time Series Forecast that was made one bar earlier for the current bar.
    * Positive means price came in above what the regression projected, negative
    * below; the value oscillates around zero and crosses it whenever price
    * meets its own forecast. Persistent readings far from zero say the trend is
    * running ahead of, or lagging, its regression line.
    * <p><b>Formula</b>
    * <pre>{@code
    * FOSC[t] = 100 * (P[t] - TSF[t-1]) / P[t], where TSF[t-1] is the Time Series Forecast fitted over the N bars ending at t-1 and evaluated one x-step beyond that window — the forecast for bar t made without seeing it.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Several vendors publish a "Chande Forecast Oscillator (CFO)" that compares the close to the regression value of the window *ending at the same bar*, with no lag. FOSC is the lagged form Chande and Achelis describe.</li>
    * <li>The default window is Chande's own suggestion, shorter than the one TA-Lib's TSF and LINEARREG default to.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#FOSC_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Number of bars in the regression window (default 5;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Percentage deviation of the close from the previous bar's
    *        forecast. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#TSF
    * @see Core#LINEARREG
    * @see Core#CMOU
    */
   public OutRange FOSC( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("FOSC", startIdx, endIdx);
      int guardStart = clampedStart("FOSC", startIdx, FOSC_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FOSC", "inReal", inReal, guardInLen);
      requireLength("FOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = FOSC_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("FOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live FOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#FOSC} over the same series.
    * Open with {@link Core#foscOpen}; there is no close — the handle is
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
   public static final class FoscStream {
      Core core;
      int optInTimePeriod;
      int lookbackTotal;
      int trailingIdx;
      double SumX;
      double SumXY;
      double SumY;
      double Divisor;
      int barsSinceReseed;
      double trailingValue;
      double sumAbs;
      int j;
      int today;
      double lag1_inReal;
      int xMask;
      double[] x_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      FoscStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#FOSC} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      FoscStream( FoscStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lookbackTotal = other.lookbackTotal;
         this.trailingIdx = other.trailingIdx;
         this.SumX = other.SumX;
         this.SumXY = other.SumXY;
         this.SumY = other.SumY;
         this.Divisor = other.Divisor;
         this.barsSinceReseed = other.barsSinceReseed;
         this.trailingValue = other.trailingValue;
         this.sumAbs = other.sumAbs;
         this.j = other.j;
         this.today = other.today;
         this.lag1_inReal = other.lag1_inReal;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("FOSC update: BadParam", RetCode.BadParam);
         }
         core.foscStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("FOSC updateAndFill", "inReal", inReal);
         requireArgument("FOSC updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("FOSC updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("FOSC updateAndFill: BadParam", RetCode.BadParam);
            }
            core.foscStepImpl(this, inReal[i]);
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
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("FOSC peek: BadParam", RetCode.BadParam);
         FoscStream sp = this;
         double m = 0.0;
         double b = 0.0;
         double closeValue = 0.0;
         int windowStart = 0;
         double tempValue1 = 0.0;
         double tempValue2 = 0.0;
         double weightedTrailing = 0.0;
         double SumXY = sp.SumXY;
         double SumY = sp.SumY;
         int barsSinceReseed = sp.barsSinceReseed;
         double cur_outReal = 0.0;
         int j = sp.j;
         double sumAbs = sp.sumAbs;
         int today = sp.today;
         int trailingIdx = sp.trailingIdx;
         double trailingValue = sp.trailingValue;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( today >= 1073741824 ) {
            int rebaseShift = trailingIdx & ~sp.xMask;
            today -= rebaseShift;
            trailingIdx -= rebaseShift;
            j -= rebaseShift;
         }
         pkSlot0 = today & sp.xMask;
         pkVal0 = inReal;
         weightedTrailing = (double)sp.optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + ((((today - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(today - 1) & sp.xMask] : pkVal0);
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs((((today - 1) & sp.xMask) != pkSlot0) ? sp.x_inReal[(today - 1) & sp.xMask] : pkVal0);
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = today - sp.lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)(sp.optInTimePeriod - 1);
            for( j = windowStart; j < today; j += 1 ) {
               tempValue1 = ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         m = (sp.optInTimePeriod * SumXY - sp.SumX * SumY) / sp.Divisor;
         b = (SumY - m * sp.SumX) / (double)sp.optInTimePeriod;
         trailingValue = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
         trailingIdx += 1;
         closeValue = ((today & sp.xMask) != pkSlot0) ? sp.x_inReal[today & sp.xMask] : pkVal0;
         if( closeValue != 0.0 ) {
            cur_outReal = 100.0 * (closeValue - (Math.fma(m, (double)sp.optInTimePeriod, b))) / closeValue;
         } else {
            cur_outReal = 0.0;
         }
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
      public FoscStream clone() {
         return new FoscStream(this);
      }
   }
   void foscStepImpl( FoscStream sp, double inReal )
   {
      double m = 0.0;
      double b = 0.0;
      double closeValue = 0.0;
      int windowStart = 0;
      double tempValue1 = 0.0;
      double tempValue2 = 0.0;
      double weightedTrailing = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.j -= rebaseShift;
      }
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      weightedTrailing = (double)sp.optInTimePeriod * sp.trailingValue;
      sp.SumXY = sp.SumXY + sp.SumY - weightedTrailing;
      sp.SumY = sp.SumY - sp.trailingValue + sp.x_inReal[(sp.today - 1) & sp.xMask];
      sp.sumAbs = sp.sumAbs - Math.abs(sp.trailingValue) + Math.abs(sp.x_inReal[(sp.today - 1) & sp.xMask]);
      sp.barsSinceReseed -= 1;
      if( sp.barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sp.sumAbs ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         windowStart = sp.today - sp.lookbackTotal;
         sp.SumY = 0;
         sp.SumXY = 0;
         sp.sumAbs = 0;
         tempValue2 = (double)(sp.optInTimePeriod - 1);
         for( sp.j = windowStart; sp.j < sp.today; sp.j += 1 ) {
            tempValue1 = sp.x_inReal[sp.j & sp.xMask];
            sp.SumY += tempValue1;
            sp.SumXY += tempValue2 * tempValue1;
            sp.sumAbs += Math.abs(tempValue1);
            tempValue2 -= 1.0;
         }
      }
      m = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
      b = (sp.SumY - m * sp.SumX) / (double)sp.optInTimePeriod;
      sp.trailingValue = sp.x_inReal[sp.trailingIdx & sp.xMask];
      sp.trailingIdx += 1;
      closeValue = sp.x_inReal[sp.today & sp.xMask];
      if( closeValue != 0.0 ) {
         sp.cur_outReal = 100.0 * (closeValue - (Math.fma(m, (double)sp.optInTimePeriod, b))) / closeValue;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.today += 1;
      sp.lag1_inReal = inReal;
   }
   private RetCode foscOpenImpl( FoscStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      double m = 0;
      double b = 0;
      double closeValue = 0;
      int i = 0;
      int j = 0;
      int windowStart = 0;
      int barsSinceReseed = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double trailingValue = 0;
      double weightedTrailing = 0;
      double sumAbs = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* The forecast compared against the close is the one made one bar EARLIER:
       * FOSC[t] = 100*(inReal[t] - TSF[t-1])/inReal[t]. The regression window
       * ends at t-1, so the lookback is period, not period-1.
       *
       * The window arithmetic below is TA_TSF's verbatim -- priming scan, O(1)
       * recurrence, both re-anchor triggers (#254) and the fused
       * `b + m*(double)optInTimePeriod` -- which is what makes FOSC bit-identical
       * to a TA_TSF call anchored one bar earlier. Reshaping any of it breaks
       * that silently.
       *
       * trailingValue -- the value the NEXT bar's window drops -- is read before
       * the output write because with outReal==inReal (#130) that write lands on
       * exactly that cell whenever startIdx is the clamped minimum. closeValue
       * carries no such constraint: it sits startIdx bars ahead of the cursor.
       */
      lookbackTotal = FOSC_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      SumXY = 0;
      SumY = 0;
      sumAbs = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - 1 - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += Math.abs(tempValue1);
      }
      m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      b = (SumY - m * SumX) / (double)optInTimePeriod;
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = inReal[trailingIdx];
      trailingIdx += 1;
      closeValue = inReal[today];
      if( closeValue != 0.0 ) {
         outReal[outIdx++ * outStride] = 100.0 * (closeValue - (Math.fma(m, (double)optInTimePeriod, b))) / closeValue;
      } else {
         outReal[outIdx++ * outStride] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         weightedTrailing = (double)optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + inReal[today - 1];
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today - 1]);
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)(optInTimePeriod - 1);
            for( j = windowStart; j < today; j += 1 ) {
               tempValue1 = inReal[j];
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         m = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         b = (SumY - m * SumX) / (double)optInTimePeriod;
         trailingValue = inReal[trailingIdx];
         trailingIdx += 1;
         closeValue = inReal[today];
         if( closeValue != 0.0 ) {
            outReal[outIdx++ * outStride] = 100.0 * (closeValue - (Math.fma(m, (double)optInTimePeriod, b))) / closeValue;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lookbackTotal = lookbackTotal;
      sp.trailingIdx = trailingIdx;
      sp.SumX = SumX;
      sp.SumXY = SumXY;
      sp.SumY = SumY;
      sp.Divisor = Divisor;
      sp.barsSinceReseed = barsSinceReseed;
      sp.trailingValue = trailingValue;
      sp.sumAbs = sumAbs;
      sp.j = j;
      sp.today = today;
      sp.lag1_inReal = inReal[historyLen - 1];
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* foscOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   FoscStream foscOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      FoscStream sp = new FoscStream(this);
      RetCode retCode = foscOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("FOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("FOSC openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("FOSC openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind foscOpen (composition seam). */
   FoscStream foscOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      FoscStream sp = new FoscStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = foscOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("FOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("FOSC open: internal error", retCode);
      }
      throw new TaLibArgumentException("FOSC open: " + retCode, retCode);
   }
   /**
    * Open a live FOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#FOSC} at that bar.
    * <p>The history must hold at least {@code FOSC_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public FoscStream foscOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("FOSC open", "inReal", inReal);
      requireHistory("FOSC open", inReal.length);
      return foscOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#foscOpen} that also fills the output array(s) bit-identically
    * to {@link Core#FOSC} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link FoscStream#outRange()}.
    */
   public FoscStream foscOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("FOSC openAndFill", "inReal", inReal);
      requireHistory("FOSC openAndFill", inReal.length);
      int guardOutLen = openFillCount("FOSC openAndFill", inReal.length, FOSC_Lookback(optInTimePeriod));
      requireLength("FOSC openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("FOSC openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return foscOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
