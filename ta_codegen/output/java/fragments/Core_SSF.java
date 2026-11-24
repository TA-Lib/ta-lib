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
 *  112326 MF,CC  First version (issue #609).
 */

   /**
    * Number of leading input bars {@link Core#ssf} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Critical period: the wavelength that divides the
    *        pass band and the reject band (default 10; range 1..10000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ssfLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 10000 ) {
         return -1;
      }
      int structural;
      if( optInTimePeriod == 1 ) {
         return this.unstableCount(FuncUnstId.SSF.ordinal(), 0, 0) ;
      }
      /* The seed occupies three bars, so the first output is a recursion step
       * even at periods 2 and 3; from 4 up the lookback is EMA's P-1.
       *
       * Auto: the poles are a complex pair at radius exp(-1.414*3.14159/P), so an
       * e-fold costs P/4.44221 bars, taken as 7P/31. Two output slots carry the
       * seed, so the difference of two starts is R*r^k*cos(k*w + phi) with R and
       * phi free; against the largest difference it ever shows, the worst phase
       * leaves ln(R/S) at most 2.51 (P = 2), 1.39 at a long period. The fixed
       * part 2 covers it at every period with the 7/31 rounding (rules_check.py).
       */
      structural = optInTimePeriod - 1;
      if( structural < 3 ) {
         structural = 3;
      }
      return structural + this.unstableCount(FuncUnstId.SSF.ordinal(), (7 * (10 + 2) * optInTimePeriod + 30) / 31, (7 * (19 + 2) * optInTimePeriod + 30) / 31) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#ssf}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Critical period: the wavelength that divides the
    *        pass band and the reject band (default 10; range 1..10000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int ssfDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( ssfLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode ssfImpl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInTimePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double a1 = 0;
      double s = 0;
      double c1h = 0;
      double c2 = 0;
      double c3 = 0;
      double x0 = 0;
      double x1 = 0;
      double y = 0;
      double y1 = 0;
      double y2 = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ssfLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      /* No smoothing at period 1: the output is a copy of the input, as MA gives
       * for every MAType. The unstable period still delays the first output.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.SUCCESS ;
      }
      /* Ehlers' constants as he prints them: 1.414 and 3.14159 in the radius,
       * and Cosine(1.414*180/P) in degrees, so a full-precision pi in the angle.
       *
       * Keep the cosine as 1 - 2*sin(w/2)^2: cos(w) is close to 1 at a long
       * period, and a one-ULP libm difference in it moves c1 by about
       * 5.6e-18*P^2 relative; the half angle does not.
       */
      a1 = Math.exp(-1.414 * 3.14159 / (double)optInTimePeriod);
      s = Math.sin(1.414 * 3.141592653589793 / (2.0 * (double)optInTimePeriod));
      c2 = 2.0 * a1 * (1.0 - 2.0 * s * s);
      c3 = -(a1 * a1);
      /* c1 = 1 - c2 - c3 makes the DC gain 1 whatever the constants. Halving it
       * here is exact, so c1h*(x + x1) is c1*(x + x1)/2.
       */
      c1h = 0.5 * (1.0 - c2 - c3);
      /* Ehlers' guard: the first three bars of the history are the price. The
       * input slot is carried in a local and never re-read, because outReal may
       * alias inReal.
       */
      today = startIdx - lookbackTotal;
      y2 = inReal[today + 1];
      y1 = inReal[today + 2];
      x1 = y1;
      today += 3;
      /* Skip the rest of the lookback: run the recurrence, publish nothing. */
      i = lookbackTotal - 3;
      while( i != 0 ) {
         x0 = inReal[today];
         y = Math.fma(c2, y1, Math.fma(c3, y2, c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         today += 1;
         i -= 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
          * the bar rate at one fused step.
          */
         x0 = inReal[today];
         y = Math.fma(c2, y1, Math.fma(c3, y2, c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         outReal[outIdx] = y;
         outIdx = outIdx + 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode ssfImpl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInTimePeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double a1 = 0;
      double s = 0;
      double c1h = 0;
      double c2 = 0;
      double c3 = 0;
      double x0 = 0;
      double x1 = 0;
      double y = 0;
      double y1 = 0;
      double y2 = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ssfLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         while( today <= endIdx ) {
            outReal[outIdx++] = (double)inReal[today++];
         }
         outNBElement.value = outIdx;
         return RetCode.SUCCESS ;
      }
      a1 = Math.exp(-1.414 * 3.14159 / (double)optInTimePeriod);
      s = Math.sin(1.414 * 3.141592653589793 / (2.0 * (double)optInTimePeriod));
      c2 = 2.0 * a1 * (1.0 - 2.0 * s * s);
      c3 = -(a1 * a1);
      c1h = 0.5 * (1.0 - c2 - c3);
      today = startIdx - lookbackTotal;
      y2 = (double)inReal[today + 1];
      y1 = (double)inReal[today + 2];
      x1 = y1;
      today += 3;
      i = lookbackTotal - 3;
      while( i != 0 ) {
         x0 = (double)inReal[today];
         y = Math.fma(c2, y1, Math.fma(c3, y2, c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         today += 1;
         i -= 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         x0 = (double)inReal[today];
         y = Math.fma(c2, y1, Math.fma(c3, y2, c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         outReal[outIdx] = y;
         outIdx = outIdx + 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * John Ehlers' SuperSmoother: a two-pole low-pass smoother with a zero at
    * Nyquist, which he recommends for use instead of an EMA and builds most of
    * his later indicators on. Read it as a smoothed price line with less lag
    * than an EMA of comparable smoothness. Its DC gain is 1, so a flat market
    * returns the price itself and the line sits on the chart with price. The
    * critical period is the wavelength that divides the pass band from the
    * reject band: cycles longer than it pass, shorter ones fall off at 12 dB
    * per octave, and a two-bar alternation is removed entirely. The poles are a
    * complex pair, which no EMA of an EMA and none of the {@code SWAK_*} rows
    * can place.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ssf">ta-lib.org/functions/ssf</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, consistent with {@code MA(period=1)} for every MAType.</li>
    * <li>Ehlers' 2004 form averages no bars in the numerator ({@code c1 * x[i]}, no zero at Nyquist); he replaced it by 2013, and implementations still computing it differ from this function by up to a few percent.</li>
    * <li>Implementations with an exact {@code sqrt(2)} and {@code pi} in place of Ehlers' constants differ from this function in about the fifth significant digit.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ssfLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to smooth; Ehlers' examples use the close.
    * @param optInTimePeriod Critical period: the wavelength that divides the
    *        pass band and the reject band (default 10; range 1..10000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal The smoothed line, on the same scale as the input. Must
    *        hold at least {@code endIdx - max(startIdx, ssfLookback(...)) + 1} values,
    *        and never be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#ema
    * @see Core#swakButter
    * @see Core#swakGauss
    * @see Core#ma
    */
   public OutRange ssf( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("SSF", startIdx, endIdx);
      int guardStart = clampedStart("SSF", startIdx, ssfLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SSF", "inReal", inReal, guardInLen);
      requireLength("SSF", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ssfImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SSF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * John Ehlers' SuperSmoother: a two-pole low-pass smoother with a zero at
    * Nyquist, which he recommends for use instead of an EMA and builds most of
    * his later indicators on. Read it as a smoothed price line with less lag
    * than an EMA of comparable smoothness. Its DC gain is 1, so a flat market
    * returns the price itself and the line sits on the chart with price. The
    * critical period is the wavelength that divides the pass band from the
    * reject band: cycles longer than it pass, shorter ones fall off at 12 dB
    * per octave, and a two-bar alternation is removed entirely. The poles are a
    * complex pair, which no EMA of an EMA and none of the {@code SWAK_*} rows
    * can place.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ssf">ta-lib.org/functions/ssf</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input, consistent with {@code MA(period=1)} for every MAType.</li>
    * <li>Ehlers' 2004 form averages no bars in the numerator ({@code c1 * x[i]}, no zero at Nyquist); he replaced it by 2013, and implementations still computing it differ from this function by up to a few percent.</li>
    * <li>Implementations with an exact {@code sqrt(2)} and {@code pi} in place of Ehlers' constants differ from this function in about the fifth significant digit.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ssfLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to smooth; Ehlers' examples use the close.
    * @param optInTimePeriod Critical period: the wavelength that divides the
    *        pass band and the reject band (default 10; range 1..10000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal The smoothed line, on the same scale as the input. Must
    *        hold at least {@code endIdx - max(startIdx, ssfLookback(...)) + 1} values,
    *        and never be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#ema
    * @see Core#swakButter
    * @see Core#swakGauss
    * @see Core#ma
    */
   public OutRange ssf( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("SSF", startIdx, endIdx);
      int guardStart = clampedStart("SSF", startIdx, ssfLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SSF", "inReal", inReal, guardInLen);
      requireLength("SSF", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ssfImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SSF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SSF stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ssf} over the same series.
    * Open with {@link Core#ssfOpen}; there is no close — the handle is
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
   public static final class SsfStream {
      private Core core;
      private int optInTimePeriod;
      private double c1h;
      private double c2;
      private double c3;
      private double x1;
      private double y1;
      private double y2;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SsfStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ssf} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("SSF advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SsfStream( SsfStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.c1h = other.c1h;
         this.c2 = other.c2;
         this.c3 = other.c3;
         this.x1 = other.x1;
         this.y1 = other.y1;
         this.y2 = other.y2;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("SSF update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SSF update", "inReal");
         core.ssfStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SSF peek", "inReal");
         SsfStream sp = this;
         double x0 = 0.0;
         double y = 0.0;
         double cur_outReal = 0.0;
         double x1 = sp.x1;
         double y1 = sp.y1;
         double y2 = sp.y2;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
          * the bar rate at one fused step.
          */
         x0 = inReal;
         y = Math.fma(sp.c2, y1, Math.fma(sp.c3, y2, sp.c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         cur_outReal = y;
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
      public SsfStream clone() {
         return new SsfStream(this);
      }
   }
   private void ssfStepImpl( SsfStream sp, double inReal )
   {
      double x0 = 0.0;
      double y = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
       * the bar rate at one fused step.
       */
      x0 = inReal;
      y = Math.fma(sp.c2, sp.y1, Math.fma(sp.c3, sp.y2, sp.c1h * (x0 + sp.x1)));
      sp.x1 = x0;
      sp.y2 = sp.y1;
      sp.y1 = y;
      sp.cur_outReal = y;
   }
   private RetCode ssfOpenImpl( SsfStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double a1 = 0;
      double s = 0;
      double c1h = 0;
      double c2 = 0;
      double c3 = 0;
      double x0 = 0;
      double x1 = 0;
      double y = 0;
      double y1 = 0;
      double y2 = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx >= historyLen ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = ssfLookback(optInTimePeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.INSUFFICIENT_HISTORY;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.c1h = 0.0;
         sp.c2 = 0.0;
         sp.c3 = 0.0;
         sp.x1 = 0.0;
         sp.y1 = 0.0;
         sp.y2 = 0.0;
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.SUCCESS;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = ssfLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Ehlers' constants as he prints them: 1.414 and 3.14159 in the radius,
       * and Cosine(1.414*180/P) in degrees, so a full-precision pi in the angle.
       *
       * Keep the cosine as 1 - 2*sin(w/2)^2: cos(w) is close to 1 at a long
       * period, and a one-ULP libm difference in it moves c1 by about
       * 5.6e-18*P^2 relative; the half angle does not.
       */
      a1 = Math.exp(-1.414 * 3.14159 / (double)optInTimePeriod);
      s = Math.sin(1.414 * 3.141592653589793 / (2.0 * (double)optInTimePeriod));
      c2 = 2.0 * a1 * (1.0 - 2.0 * s * s);
      c3 = -(a1 * a1);
      /* c1 = 1 - c2 - c3 makes the DC gain 1 whatever the constants. Halving it
       * here is exact, so c1h*(x + x1) is c1*(x + x1)/2.
       */
      c1h = 0.5 * (1.0 - c2 - c3);
      /* Ehlers' guard: the first three bars of the history are the price. The
       * input slot is carried in a local and never re-read, because outReal may
       * alias inReal.
       */
      today = startIdx - lookbackTotal;
      y2 = inReal[today + 1];
      y1 = inReal[today + 2];
      x1 = y1;
      today += 3;
      /* Skip the rest of the lookback: run the recurrence, publish nothing. */
      i = lookbackTotal - 3;
      while( i != 0 ) {
         x0 = inReal[today];
         y = Math.fma(c2, y1, Math.fma(c3, y2, c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         today += 1;
         i -= 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         /* c2*y1 stays the outermost term: y1 is the newest output, so that keeps
          * the bar rate at one fused step.
          */
         x0 = inReal[today];
         y = Math.fma(c2, y1, Math.fma(c3, y2, c1h * (x0 + x1)));
         x1 = x0;
         y2 = y1;
         y1 = y;
         outReal[outIdx * outStride] = y;
         outIdx = outIdx + 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.c1h = c1h;
      sp.c2 = c2;
      sp.c3 = c3;
      sp.x1 = x1;
      sp.y1 = y1;
      sp.y2 = y2;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* ssfOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SsfStream ssfOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SsfStream sp = new SsfStream(this);
      RetCode retCode = ssfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SSF openAndFill", inReal.length, startIdx, ssfLookback(optInTimePeriod));
      }
      throw streamFailure("SSF openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind ssfOpen (composition seam). */
   SsfStream ssfOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      SsfStream sp = new SsfStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ssfOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SSF open", inReal.length, startIdx, ssfLookback(optInTimePeriod));
      }
      throw streamFailure("SSF open", retCode);
   }
   /**
    * Open a live SSF stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ssf} at that bar.
    * <p>The history must hold at least {@code ssfLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public SsfStream ssfOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("SSF open", "inReal", inReal);
      requireHistory("SSF open", inReal.length);
      return ssfOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#ssfOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ssf} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SsfStream#outRange()}.
    */
   public SsfStream ssfOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("SSF openAndFill", "inReal", inReal);
      requireHistory("SSF openAndFill", inReal.length);
      int guardOutLen = openFillCount("SSF openAndFill", inReal.length, ssfLookback(optInTimePeriod));
      requireLength("SSF openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("SSF openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ssfOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
   private double ssfStepTape( SsfStream sp, double[] tape, int tapeBase, int tapeMask, double inReal )
   {
      ssfStepImpl(sp, inReal);
      sp.outRangeCount++;
      return sp.cur_outReal;
   }
   private double ssfPeekTape( SsfStream sp, double[] tape, int tapeBase, int tapeMask, double inReal )
   {
      return sp.peek(inReal);
   }
   private int ssfTapeDetach( SsfStream sp )
   {
      return 0;
   }
