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
 *  072026 MF,CC  Read the departing value before the output write so in-place
 *                (outReal==inReal) calls stay correct. See issue #130.
 *  082426 MF,CC  Fix #254. Re-anchor the running sums: every 32*period bars,
 *                and on the bar a large value leaves the window.
 */

   /**
    * Number of leading input bars {@link Core#LINEARREG_SLOPE} consumes before
    * it can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the regression window (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int LINEARREG_SLOPE_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode LINEARREG_SLOPE_Impl( int startIdx,
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
      lookbackTotal = LINEARREG_SLOPE_Lookback(optInTimePeriod);
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
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      sumAbs = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += Math.abs(tempValue1);
      }
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = inReal[trailingIdx];
      trailingIdx += 1;
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       * Each departing value is read before the output write of the same bar:
       * with outReal==inReal (in-place, #130) that write lands on the cell the
       * next iteration departs from.
       */
      while( today <= endIdx ) {
         weightedTrailing = (double)optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + inReal[today];
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today]);
         /* Re-anchor: rebuild both sums from the window itself. #103 left them as
          * running totals that are never rebuilt, so each bar's rounding joins a
          * residue no later bar can subtract -- unbounded in the length of the
          * call, and scaled by the largest value the sums have EVER held rather
          * than by what the window holds now. Two triggers, and they cover
          * different failures (issue #254):
          *
          *   - every 32*period bars, so a slow drift stays bounded however long
          *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
          *
          *   - when the value the window just dropped carries more weight than
          *     everything left in it. That is the one the interval cannot cover:
          *     one large print inflates the residue for up to 32*period bars
          *     after it is gone (measured 31x at period 5), and this rebuilds on
          *     the bar it leaves instead.
          *
          * The threshold compares two DEGREE-1 quantities, which is why it is 100
          * and not TA_CORREL's 1e6 -- that guard weighs a squared deviation
          * against a sum of squares. On ordinary prices the ratio is ~1 and this
          * never fires; it is a compare, not work. The constant is 100 rather than
          * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
          * measured accuracy gain.
          *
          * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
          * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
          * while the departing value does not, so |weightedTrailing|/|SumY| is
          * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
          * series measured 10.9x slower at period 30, which is precisely the
          * O(n*period) cost #103 removed. Same shape of error as #242's absolute
          * guard on a quartic quantity: a ratio test is ill-posed when its
          * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
          * when every value in the window is 0 -- and then the numerator is 0 too
          * and the test is false. There is no window it can misjudge.
          *
          * It is also the RIGHT quantity on the merits, not just the safe one: a
          * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
          * term against sum|y| asks exactly "would rebuilding beat what we are
          * carrying?".
          *
          * Carrying it is free in practice. Measured on the shipped libta-lib.a it
          * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
          * ns/bar at period 14) because the update is INDEPENDENT of the serial
          * SumXY -> SumY dependency chain and fills slots that were idle. The
          * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
          * once per `period` bars -- bounded the cliff at 1.2x rather than removing
          * it, and silently dropped any print departing within `period` bars of a
          * rebuild (~3% of them).
          *
          * The scan walks the window oldest-first with the weight counting DOWN,
          * which is the priming scan's order and weighting -- so a reseeded bar is
          * bit-identical to the same bar computed by a call that started there.
          * That identity is the whole point: it is what the range-stability
          * contract measures.
          *
          * Reading the window is safe when outReal aliases inReal (#130): the
          * outputs written so far occupy [0, outIdx-1], and windowStart is
          * today-lookbackTotal, which is >= outIdx because startIdx was clamped
          * to at least lookbackTotal.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)lookbackTotal;
            for( j = windowStart; j <= today; j += 1 ) {
               tempValue1 = inReal[j];
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         trailingValue = inReal[trailingIdx];
         trailingIdx += 1;
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode LINEARREG_SLOPE_Impl( int startIdx,
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
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = LINEARREG_SLOPE_Lookback(optInTimePeriod);
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
         tempValue1 = (double)inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += Math.abs(tempValue1);
      }
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = (double)inReal[trailingIdx];
      trailingIdx += 1;
      outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      while( today <= endIdx ) {
         weightedTrailing = (double)optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + (double)inReal[today];
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs((double)inReal[today]);
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)lookbackTotal;
            for( j = windowStart; j <= today; j += 1 ) {
               tempValue1 = (double)inReal[j];
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         trailingValue = (double)inReal[trailingIdx];
         trailingIdx += 1;
         outReal[outIdx++] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last
    * optInTimePeriod bars. Reports the per-bar rate of change of the fitted
    * trend line. Positive slope = rising trend, negative = falling; magnitude
    * is price change per bar.
    * <p><b>Formula</b>
    * <pre>{@code
    * m = (n·SumXY − SumX·SumY) / Divisor
    * SumX = n(n−1)/2,  SumXSqr = n(n−1)(2n−1)/6,  Divisor = SumX² − n·SumXSqr
    * SumXY = Σ i·y[today−i],  SumY = Σ y[today−i],  i=0..n−1,  n=period,  y=inReal
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#LINEARREG_SLOPE_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data series to fit.
    * @param optInTimePeriod Number of bars in the regression window (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Slope m of the fitted line. Must hold at least
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
    * @see Core#LINEARREG
    * @see Core#LINEARREG_INTERCEPT
    * @see Core#LINEARREG_ANGLE
    * @see Core#TSF
    */
   public OutRange LINEARREG_SLOPE( int startIdx,
                                    int endIdx,
                                    double inReal[],
                                    int optInTimePeriod,
                                    double outReal[] )
   {
      requireIndexRange("LINEARREG_SLOPE", startIdx, endIdx);
      int guardStart = clampedStart("LINEARREG_SLOPE", startIdx, LINEARREG_SLOPE_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("LINEARREG_SLOPE", "inReal", inReal, guardInLen);
      requireLength("LINEARREG_SLOPE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = LINEARREG_SLOPE_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("LINEARREG_SLOPE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last
    * optInTimePeriod bars. Reports the per-bar rate of change of the fitted
    * trend line. Positive slope = rising trend, negative = falling; magnitude
    * is price change per bar.
    * <p><b>Formula</b>
    * <pre>{@code
    * m = (n·SumXY − SumX·SumY) / Divisor
    * SumX = n(n−1)/2,  SumXSqr = n(n−1)(2n−1)/6,  Divisor = SumX² − n·SumXSqr
    * SumXY = Σ i·y[today−i],  SumY = Σ y[today−i],  i=0..n−1,  n=period,  y=inReal
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#LINEARREG_SLOPE_Lookback} is a
    * <b>success with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data series to fit.
    * @param optInTimePeriod Number of bars in the regression window (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Slope m of the fitted line. Must hold at least
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
    * @see Core#LINEARREG
    * @see Core#LINEARREG_INTERCEPT
    * @see Core#LINEARREG_ANGLE
    * @see Core#TSF
    */
   public OutRange LINEARREG_SLOPE( int startIdx,
                                    int endIdx,
                                    float inReal[],
                                    int optInTimePeriod,
                                    double outReal[] )
   {
      requireIndexRange("LINEARREG_SLOPE", startIdx, endIdx);
      int guardStart = clampedStart("LINEARREG_SLOPE", startIdx, LINEARREG_SLOPE_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("LINEARREG_SLOPE", "inReal", inReal, guardInLen);
      requireLength("LINEARREG_SLOPE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = LINEARREG_SLOPE_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("LINEARREG_SLOPE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live LINEARREG_SLOPE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#LINEARREG_SLOPE} over the same series.
    * Open with {@link Core#linearregSlopeOpen}; there is no close — the handle is
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
   public static final class LinearregSlopeStream {
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
      int xMask;
      double[] x_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      LinearregSlopeStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#LINEARREG_SLOPE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      LinearregSlopeStream( LinearregSlopeStream other ) {
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
            throw new TaLibArgumentException("LINEARREG_SLOPE update: BadParam", RetCode.BadParam);
         }
         core.linearregSlopeStepImpl(this, inReal);
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
         requireArgument("LINEARREG_SLOPE updateAndFill", "inReal", inReal);
         requireArgument("LINEARREG_SLOPE updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("LINEARREG_SLOPE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("LINEARREG_SLOPE updateAndFill: BadParam", RetCode.BadParam);
            }
            core.linearregSlopeStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("LINEARREG_SLOPE peek: BadParam", RetCode.BadParam);
         LinearregSlopeStream sp = this;
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
         SumY = SumY - trailingValue + (((today & sp.xMask) != pkSlot0) ? sp.x_inReal[today & sp.xMask] : pkVal0);
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(((today & sp.xMask) != pkSlot0) ? sp.x_inReal[today & sp.xMask] : pkVal0);
         /* Re-anchor: rebuild both sums from the window itself. #103 left them as
          * running totals that are never rebuilt, so each bar's rounding joins a
          * residue no later bar can subtract -- unbounded in the length of the
          * call, and scaled by the largest value the sums have EVER held rather
          * than by what the window holds now. Two triggers, and they cover
          * different failures (issue #254):
          *
          *   - every 32*period bars, so a slow drift stays bounded however long
          *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
          *
          *   - when the value the window just dropped carries more weight than
          *     everything left in it. That is the one the interval cannot cover:
          *     one large print inflates the residue for up to 32*period bars
          *     after it is gone (measured 31x at period 5), and this rebuilds on
          *     the bar it leaves instead.
          *
          * The threshold compares two DEGREE-1 quantities, which is why it is 100
          * and not TA_CORREL's 1e6 -- that guard weighs a squared deviation
          * against a sum of squares. On ordinary prices the ratio is ~1 and this
          * never fires; it is a compare, not work. The constant is 100 rather than
          * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
          * measured accuracy gain.
          *
          * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
          * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
          * while the departing value does not, so |weightedTrailing|/|SumY| is
          * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
          * series measured 10.9x slower at period 30, which is precisely the
          * O(n*period) cost #103 removed. Same shape of error as #242's absolute
          * guard on a quartic quantity: a ratio test is ill-posed when its
          * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
          * when every value in the window is 0 -- and then the numerator is 0 too
          * and the test is false. There is no window it can misjudge.
          *
          * It is also the RIGHT quantity on the merits, not just the safe one: a
          * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
          * term against sum|y| asks exactly "would rebuilding beat what we are
          * carrying?".
          *
          * Carrying it is free in practice. Measured on the shipped libta-lib.a it
          * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
          * ns/bar at period 14) because the update is INDEPENDENT of the serial
          * SumXY -> SumY dependency chain and fills slots that were idle. The
          * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
          * once per `period` bars -- bounded the cliff at 1.2x rather than removing
          * it, and silently dropped any print departing within `period` bars of a
          * rebuild (~3% of them).
          *
          * The scan walks the window oldest-first with the weight counting DOWN,
          * which is the priming scan's order and weighting -- so a reseeded bar is
          * bit-identical to the same bar computed by a call that started there.
          * That identity is the whole point: it is what the range-stability
          * contract measures.
          *
          * Reading the window is safe when outReal aliases inReal (#130): the
          * outputs written so far occupy [0, outIdx-1], and windowStart is
          * today-lookbackTotal, which is >= outIdx because startIdx was clamped
          * to at least lookbackTotal.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * sp.optInTimePeriod;
            windowStart = today - sp.lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)sp.lookbackTotal;
            for( j = windowStart; j <= today; j += 1 ) {
               tempValue1 = ((j & sp.xMask) != pkSlot0) ? sp.x_inReal[j & sp.xMask] : pkVal0;
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         trailingValue = ((trailingIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[trailingIdx & sp.xMask] : pkVal0;
         trailingIdx += 1;
         cur_outReal = (sp.optInTimePeriod * SumXY - sp.SumX * SumY) / sp.Divisor;
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
      public LinearregSlopeStream clone() {
         return new LinearregSlopeStream(this);
      }
   }
   void linearregSlopeStepImpl( LinearregSlopeStream sp, double inReal )
   {
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
      sp.SumY = sp.SumY - sp.trailingValue + sp.x_inReal[sp.today & sp.xMask];
      sp.sumAbs = sp.sumAbs - Math.abs(sp.trailingValue) + Math.abs(sp.x_inReal[sp.today & sp.xMask]);
      /* Re-anchor: rebuild both sums from the window itself. #103 left them as
       * running totals that are never rebuilt, so each bar's rounding joins a
       * residue no later bar can subtract -- unbounded in the length of the
       * call, and scaled by the largest value the sums have EVER held rather
       * than by what the window holds now. Two triggers, and they cover
       * different failures (issue #254):
       *
       *   - every 32*period bars, so a slow drift stays bounded however long
       *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
       *
       *   - when the value the window just dropped carries more weight than
       *     everything left in it. That is the one the interval cannot cover:
       *     one large print inflates the residue for up to 32*period bars
       *     after it is gone (measured 31x at period 5), and this rebuilds on
       *     the bar it leaves instead.
       *
       * The threshold compares two DEGREE-1 quantities, which is why it is 100
       * and not TA_CORREL's 1e6 -- that guard weighs a squared deviation
       * against a sum of squares. On ordinary prices the ratio is ~1 and this
       * never fires; it is a compare, not work. The constant is 100 rather than
       * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
       * measured accuracy gain.
       *
       * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
       * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
       * while the departing value does not, so |weightedTrailing|/|SumY| is
       * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
       * series measured 10.9x slower at period 30, which is precisely the
       * O(n*period) cost #103 removed. Same shape of error as #242's absolute
       * guard on a quartic quantity: a ratio test is ill-posed when its
       * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
       * when every value in the window is 0 -- and then the numerator is 0 too
       * and the test is false. There is no window it can misjudge.
       *
       * It is also the RIGHT quantity on the merits, not just the safe one: a
       * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
       * term against sum|y| asks exactly "would rebuilding beat what we are
       * carrying?".
       *
       * Carrying it is free in practice. Measured on the shipped libta-lib.a it
       * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
       * ns/bar at period 14) because the update is INDEPENDENT of the serial
       * SumXY -> SumY dependency chain and fills slots that were idle. The
       * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
       * once per `period` bars -- bounded the cliff at 1.2x rather than removing
       * it, and silently dropped any print departing within `period` bars of a
       * rebuild (~3% of them).
       *
       * The scan walks the window oldest-first with the weight counting DOWN,
       * which is the priming scan's order and weighting -- so a reseeded bar is
       * bit-identical to the same bar computed by a call that started there.
       * That identity is the whole point: it is what the range-stability
       * contract measures.
       *
       * Reading the window is safe when outReal aliases inReal (#130): the
       * outputs written so far occupy [0, outIdx-1], and windowStart is
       * today-lookbackTotal, which is >= outIdx because startIdx was clamped
       * to at least lookbackTotal.
       */
      sp.barsSinceReseed -= 1;
      if( sp.barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sp.sumAbs ) {
         sp.barsSinceReseed = 32 * sp.optInTimePeriod;
         windowStart = sp.today - sp.lookbackTotal;
         sp.SumY = 0;
         sp.SumXY = 0;
         sp.sumAbs = 0;
         tempValue2 = (double)sp.lookbackTotal;
         for( sp.j = windowStart; sp.j <= sp.today; sp.j += 1 ) {
            tempValue1 = sp.x_inReal[sp.j & sp.xMask];
            sp.SumY += tempValue1;
            sp.SumXY += tempValue2 * tempValue1;
            sp.sumAbs += Math.abs(tempValue1);
            tempValue2 -= 1.0;
         }
      }
      sp.trailingValue = sp.x_inReal[sp.trailingIdx & sp.xMask];
      sp.trailingIdx += 1;
      sp.cur_outReal = (sp.optInTimePeriod * sp.SumXY - sp.SumX * sp.SumY) / sp.Divisor;
      sp.today += 1;
   }
   private RetCode linearregSlopeOpenImpl( LinearregSlopeStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
      lookbackTotal = LINEARREG_SLOPE_Lookback(optInTimePeriod);
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
      /* Index into the output. */
      today = startIdx;
      trailingIdx = startIdx - lookbackTotal;
      SumX = (double)optInTimePeriod * (optInTimePeriod - 1) * 0.5;
      SumXSqr = (double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0;
      Divisor = SumX * SumX - optInTimePeriod * SumXSqr;
      /* Prime the two data-dependent window sums for the first output with a
       * one-time full-window scan. SumX/SumXSqr/Divisor are period-only constants;
       * SumY = sum of the window, SumXY = sum of i*value (i the reversed
       * 0..period-1 position).
       */
      SumXY = 0;
      SumY = 0;
      sumAbs = 0;
      for( i = optInTimePeriod; i-- != 0;  ) {
         tempValue1 = inReal[today - i];
         SumY += tempValue1;
         SumXY += (double)i * tempValue1;
         sumAbs += Math.abs(tempValue1);
      }
      barsSinceReseed = 32 * optInTimePeriod;
      trailingValue = inReal[trailingIdx];
      trailingIdx += 1;
      outReal[outIdx++ * outStride] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
      today += 1;
      /* Slide the window one bar at a time, keeping both sums in O(1): advancing
       * the window raises every retained value's weight by 1 (adds SumY) and drops
       * the departing value at full weight (subtracts period*trailingValue). Same
       * incremental identity as WMA/CORREL; the output arithmetic is unchanged.
       * (perf #103 -- numerics-changing: running total vs per-bar fresh sum.)
       * Each departing value is read before the output write of the same bar:
       * with outReal==inReal (in-place, #130) that write lands on the cell the
       * next iteration departs from.
       */
      while( today <= endIdx ) {
         weightedTrailing = (double)optInTimePeriod * trailingValue;
         SumXY = SumXY + SumY - weightedTrailing;
         SumY = SumY - trailingValue + inReal[today];
         sumAbs = sumAbs - Math.abs(trailingValue) + Math.abs(inReal[today]);
         /* Re-anchor: rebuild both sums from the window itself. #103 left them as
          * running totals that are never rebuilt, so each bar's rounding joins a
          * residue no later bar can subtract -- unbounded in the length of the
          * call, and scaled by the largest value the sums have EVER held rather
          * than by what the window holds now. Two triggers, and they cover
          * different failures (issue #254):
          *
          *   - every 32*period bars, so a slow drift stays bounded however long
          *     the series runs. Same interval as TA_VAR / TA_CORREL / TA_BETA.
          *
          *   - when the value the window just dropped carries more weight than
          *     everything left in it. That is the one the interval cannot cover:
          *     one large print inflates the residue for up to 32*period bars
          *     after it is gone (measured 31x at period 5), and this rebuilds on
          *     the bar it leaves instead.
          *
          * The threshold compares two DEGREE-1 quantities, which is why it is 100
          * and not TA_CORREL's 1e6 -- that guard weighs a squared deviation
          * against a sum of squares. On ordinary prices the ratio is ~1 and this
          * never fires; it is a compare, not work. The constant is 100 rather than
          * 10 because at 10 a zero-mean oscillator rebuilds on 8.8% of bars for no
          * measured accuracy gain.
          *
          * THE DENOMINATOR IS sumAbs, NOT SumY, AND THAT IS THE WHOLE POINT.
          * SumY is a CANCELLING sum: on a zero-mean window it collapses toward 0
          * while the departing value does not, so |weightedTrailing|/|SumY| is
          * unbounded and the rebuild fires on EVERY bar -- an alternating +/-1
          * series measured 10.9x slower at period 30, which is precisely the
          * O(n*period) cost #103 removed. Same shape of error as #242's absolute
          * guard on a quartic quantity: a ratio test is ill-posed when its
          * denominator can cancel. sumAbs is a sum of magnitudes, so it is 0 only
          * when every value in the window is 0 -- and then the numerator is 0 too
          * and the test is false. There is no window it can misjudge.
          *
          * It is also the RIGHT quantity on the merits, not just the safe one: a
          * fresh rebuild's own error is ~eps*sum|y|, so comparing the departing
          * term against sum|y| asks exactly "would rebuilding beat what we are
          * carrying?".
          *
          * Carrying it is free in practice. Measured on the shipped libta-lib.a it
          * costs nothing against the |SumY| form on a price series (1.541 vs 1.605
          * ns/bar at period 14) because the update is INDEPENDENT of the serial
          * SumXY -> SumY dependency chain and fills slots that were idle. The
          * rejected alternative -- keeping |SumY| and rate-limiting the trigger to
          * once per `period` bars -- bounded the cliff at 1.2x rather than removing
          * it, and silently dropped any print departing within `period` bars of a
          * rebuild (~3% of them).
          *
          * The scan walks the window oldest-first with the weight counting DOWN,
          * which is the priming scan's order and weighting -- so a reseeded bar is
          * bit-identical to the same bar computed by a call that started there.
          * That identity is the whole point: it is what the range-stability
          * contract measures.
          *
          * Reading the window is safe when outReal aliases inReal (#130): the
          * outputs written so far occupy [0, outIdx-1], and windowStart is
          * today-lookbackTotal, which is >= outIdx because startIdx was clamped
          * to at least lookbackTotal.
          */
         barsSinceReseed -= 1;
         if( barsSinceReseed <= 0 || Math.abs(weightedTrailing) > 100.0 * sumAbs ) {
            barsSinceReseed = 32 * optInTimePeriod;
            windowStart = today - lookbackTotal;
            SumY = 0;
            SumXY = 0;
            sumAbs = 0;
            tempValue2 = (double)lookbackTotal;
            for( j = windowStart; j <= today; j += 1 ) {
               tempValue1 = inReal[j];
               SumY += tempValue1;
               SumXY += tempValue2 * tempValue1;
               sumAbs += Math.abs(tempValue1);
               tempValue2 -= 1.0;
            }
         }
         trailingValue = inReal[trailingIdx];
         trailingIdx += 1;
         outReal[outIdx++ * outStride] = (optInTimePeriod * SumXY - SumX * SumY) / Divisor;
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
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* linearregSlopeOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   LinearregSlopeStream linearregSlopeOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      LinearregSlopeStream sp = new LinearregSlopeStream(this);
      RetCode retCode = linearregSlopeOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("LINEARREG_SLOPE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("LINEARREG_SLOPE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("LINEARREG_SLOPE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind linearregSlopeOpen (composition seam). */
   LinearregSlopeStream linearregSlopeOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      LinearregSlopeStream sp = new LinearregSlopeStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = linearregSlopeOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("LINEARREG_SLOPE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("LINEARREG_SLOPE open: internal error", retCode);
      }
      throw new TaLibArgumentException("LINEARREG_SLOPE open: " + retCode, retCode);
   }
   /**
    * Open a live LINEARREG_SLOPE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#LINEARREG_SLOPE} at that bar.
    * <p>The history must hold at least {@code LINEARREG_SLOPE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public LinearregSlopeStream linearregSlopeOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("LINEARREG_SLOPE open", "inReal", inReal);
      requireHistory("LINEARREG_SLOPE open", inReal.length);
      return linearregSlopeOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#linearregSlopeOpen} that also fills the output array(s) bit-identically
    * to {@link Core#LINEARREG_SLOPE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link LinearregSlopeStream#outRange()}.
    */
   public LinearregSlopeStream linearregSlopeOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("LINEARREG_SLOPE openAndFill", "inReal", inReal);
      requireHistory("LINEARREG_SLOPE openAndFill", inReal.length);
      int guardOutLen = openFillCount("LINEARREG_SLOPE openAndFill", inReal.length, LINEARREG_SLOPE_Lookback(optInTimePeriod));
      requireLength("LINEARREG_SLOPE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("LINEARREG_SLOPE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return linearregSlopeOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
