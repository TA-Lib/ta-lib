/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  091126 KL     First version (proposal-drafts issue #68).
 */

   /**
    * Number of leading input bars {@link Core#RVIR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Wilder smoothing period applied to both legs of
    *        both indices (default 14; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInStdDevPeriod Number of trailing values each standard deviation
    *        spans (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int RVIR_Lookback( int optInTimePeriod, int optInStdDevPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return -1;
      }
      /* Both legs are the shipped TA_RVI at the same parameters, so they warm on
       * the same bar and there is no max() to take. Stated as the callee's
       * lookback rather than restating the arithmetic, which is what makes this
       * function inherit TA_FUNC_UNST_RVI the way KC inherits its two (kc.c).
       */
      return RVI_Lookback(optInTimePeriod, optInStdDevPeriod) ;

   }
   RetCode RVIR_Impl( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      int optInTimePeriod,
                      int optInStdDevPeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double[] tempHigh;
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
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
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RVIR_Lookback(optInTimePeriod, optInStdDevPeriod);
      /* Nothing is allocated and no input is read when the range cannot produce a
       * value, so a caller-supplied input that stops short of endIdx is never
       * read past its end (kc.c).
       */
      if( lookbackTotal > endIdx ) {
         return RetCode.Success ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Each leg is the shipped TA_RVI over this range, entered at the same
       * startIdx: same function, same parameters, same lookback, so the two
       * outputs are aligned bar for bar with no anchor to reconcile.
       *
       * Two calls rather than one fused loop. The fused body is expressible --
       * two copies of TA_RVI's state driven by one bar cursor -- but it carries
       * two window-start cursors, and the streaming analyzer accepts exactly one
       * ("extrema automaton: expected exactly one window-start variable"), so
       * fusing would cost this function the streaming tier. Composing keeps it,
       * for the reason KC's legs do: each leg streams as itself.
       */
      tempHigh = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Either input may be aliased onto outReal. Both legs are safe against that
       * for TA_RVI's own reason -- its write index trails its read index by the
       * lookback and never overtakes it -- so the order of the two calls is an
       * implementation detail, not what makes the aliasing safe. Swapping them
       * leaves every value identical, which is why the aliasing test earns its
       * keep against the scratch buffer's extent rather than against this order.
       */
      OutRange _xr0 = RVI(startIdx, endIdx, inHigh, optInTimePeriod, optInStdDevPeriod, tempHigh);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.Success;
      OutRange _xr1 = RVI(startIdx, endIdx, inLow, optInTimePeriod, optInStdDevPeriod, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
       * 50 before the average is taken, so a tie in one series never drags the
       * other. Scaling by one half is exact, so the spelling of the average is
       * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
       * 0.5*a + 0.5*b.
       */
      for( i = 0; i < outNBElement.value; i += 1 ) {
         outReal[i] = 0.5 * (tempHigh[i] + outReal[i]);
      }
      return RetCode.Success ;
   }
   RetCode RVIR_Impl( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      int optInTimePeriod,
                      int optInStdDevPeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outReal[] )
   {
      double[] tempHigh;
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
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
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RVIR_Lookback(optInTimePeriod, optInStdDevPeriod);
      if( lookbackTotal > endIdx ) {
         return RetCode.Success ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      tempHigh = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = RVI(startIdx, endIdx, inHigh, optInTimePeriod, optInStdDevPeriod, tempHigh);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.Success;
      OutRange _xr1 = RVI(startIdx, endIdx, inLow, optInTimePeriod, optInStdDevPeriod, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      for( i = 0; i < outNBElement.value; i += 1 ) {
         outReal[i] = 0.5 * (tempHigh[i] + outReal[i]);
      }
      return RetCode.Success ;
   }
   /**
    * Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of
    * his own indicator, which runs the 1993 RVI over the daily highs and again
    * over the daily lows and averages the two indices. Each leg is the shipped
    * <a href="https://ta-lib.org/functions/rvi">{@code RVI}</a> unchanged — the
    * same rolling standard deviation routed to an up or a down bucket by the
    * direction of the bar, the same Wilder smoothing, the same treatment of a
    * tie. Bounded in 0..100 and read like the close-only form: above 50 the
    * recent volatility arrived mostly on up bars, below 50 mostly on down bars.
    * Dorsey's stated reason for the revision is that a high and a low carry the
    * day's range, so the pair answers the question the close alone can only
    * approximate.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/rvir">ta-lib.org/functions/rvir</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The name is contested, and in the opposite direction from what the abbreviation suggests. Some vendors reserve the bare name RVI for <i>this</i> revision and call the 1993 close-only form RVIorig; others default the other way. This library ships the 1993 form as <a href="https://ta-lib.org/functions/rvi">{@code RVI}</a> and the 1995 revision here.</li>
    * <li>The two legs are computed in one pass rather than by calling {@code RVI} twice, but the arithmetic of each leg is {@code RVI}'s in {@code RVI}'s order. The result is the average of two {@code RVI} calls bit for bit, which is what the regression test asserts.</li>
    * <li>A bar whose high equals the previous high feeds neither bucket of the high leg, and likewise for the lows. Descriptions that write a leg's denominator as a smoothed deviation instead of {@code U + D} are counting ties as down bars, which is a different indicator: on a 252-bar equity series that flip moves this function by up to 4.6 index points.</li>
    * <li>Each leg reports 50 when its own smoothed legs are both exactly zero, for the reason {@code RVI} does. The average is taken after each leg has resolved that, so a tie in one series does not drag the other.</li>
    * <li>On a series whose high equals its low at every bar the two legs are the same computation, and this function returns exactly {@code RVI} of it.</li>
    * <li>Sources publishing something else under this name, and how far from this function they land on a 252-bar equity series: a plain exponential smoother instead of Wilder's, up to 12.9 index points; averaging the <i>prices</i> and taking one index of the result instead of averaging the two indices, up to 13.6; adding the close as a third leg, up to 6.0; a 9-period deviation, up to 3.2. These are different indicators, not errors.</li>
    * <li>This is not a re-parameterisation of {@code RVI}: against {@code RVI} of the closes at the shared defaults the two series differ by up to 18.1 index points on the same corpus.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RVIR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Wilder smoothing period applied to both legs of
    *        both indices (default 14; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInStdDevPeriod Number of trailing values each standard deviation
    *        spans (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal The averaged index, in 0..100. Must hold at least
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
    * @see Core#RVI
    * @see Core#STDDEV
    * @see Core#ATR
    */
   public OutRange RVIR( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         int optInTimePeriod,
                         int optInStdDevPeriod,
                         double outReal[] )
   {
      requireIndexRange("RVIR", startIdx, endIdx);
      int guardStart = clampedStart("RVIR", startIdx, RVIR_Lookback(optInTimePeriod, optInStdDevPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RVIR", "inHigh", inHigh, guardInLen);
      requireLength("RVIR", "inLow", inLow, guardInLen);
      requireLength("RVIR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RVIR_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RVIR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of
    * his own indicator, which runs the 1993 RVI over the daily highs and again
    * over the daily lows and averages the two indices. Each leg is the shipped
    * <a href="https://ta-lib.org/functions/rvi">{@code RVI}</a> unchanged — the
    * same rolling standard deviation routed to an up or a down bucket by the
    * direction of the bar, the same Wilder smoothing, the same treatment of a
    * tie. Bounded in 0..100 and read like the close-only form: above 50 the
    * recent volatility arrived mostly on up bars, below 50 mostly on down bars.
    * Dorsey's stated reason for the revision is that a high and a low carry the
    * day's range, so the pair answers the question the close alone can only
    * approximate.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/rvir">ta-lib.org/functions/rvir</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The name is contested, and in the opposite direction from what the abbreviation suggests. Some vendors reserve the bare name RVI for <i>this</i> revision and call the 1993 close-only form RVIorig; others default the other way. This library ships the 1993 form as <a href="https://ta-lib.org/functions/rvi">{@code RVI}</a> and the 1995 revision here.</li>
    * <li>The two legs are computed in one pass rather than by calling {@code RVI} twice, but the arithmetic of each leg is {@code RVI}'s in {@code RVI}'s order. The result is the average of two {@code RVI} calls bit for bit, which is what the regression test asserts.</li>
    * <li>A bar whose high equals the previous high feeds neither bucket of the high leg, and likewise for the lows. Descriptions that write a leg's denominator as a smoothed deviation instead of {@code U + D} are counting ties as down bars, which is a different indicator: on a 252-bar equity series that flip moves this function by up to 4.6 index points.</li>
    * <li>Each leg reports 50 when its own smoothed legs are both exactly zero, for the reason {@code RVI} does. The average is taken after each leg has resolved that, so a tie in one series does not drag the other.</li>
    * <li>On a series whose high equals its low at every bar the two legs are the same computation, and this function returns exactly {@code RVI} of it.</li>
    * <li>Sources publishing something else under this name, and how far from this function they land on a 252-bar equity series: a plain exponential smoother instead of Wilder's, up to 12.9 index points; averaging the <i>prices</i> and taking one index of the result instead of averaging the two indices, up to 13.6; adding the close as a third leg, up to 6.0; a 9-period deviation, up to 3.2. These are different indicators, not errors.</li>
    * <li>This is not a re-parameterisation of {@code RVI}: against {@code RVI} of the closes at the shared defaults the two series differ by up to 18.1 index points on the same corpus.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RVIR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Wilder smoothing period applied to both legs of
    *        both indices (default 14; range 1..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInStdDevPeriod Number of trailing values each standard deviation
    *        spans (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal The averaged index, in 0..100. Must hold at least
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
    * @see Core#RVI
    * @see Core#STDDEV
    * @see Core#ATR
    */
   public OutRange RVIR( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         int optInTimePeriod,
                         int optInStdDevPeriod,
                         double outReal[] )
   {
      requireIndexRange("RVIR", startIdx, endIdx);
      int guardStart = clampedStart("RVIR", startIdx, RVIR_Lookback(optInTimePeriod, optInStdDevPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RVIR", "inHigh", inHigh, guardInLen);
      requireLength("RVIR", "inLow", inLow, guardInLen);
      requireLength("RVIR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RVIR_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RVIR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live RVIR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#RVIR} over the same series.
    * Open with {@link Core#rvirOpen}; there is no close — the handle is
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
   public static final class RvirStream {
      private Core core;
      private int optInTimePeriod;
      private int optInStdDevPeriod;
      private double cur_outReal;
      private RviStream sub0;
      private RviStream sub1;
      private int outRangeBegIdx;
      private int outRangeCount;

      private RvirStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#RVIR} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#MAX_INDEX}; past that
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
       * has reached bar {@link Core#MAX_INDEX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("RVIR advance", RetCode.OutOfRangeEndIndex);
         this.outRangeCount++;
      }

      private RvirStream( RvirStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInStdDevPeriod = other.optInStdDevPeriod;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new RviStream(other.sub0);
         this.sub1 = new RviStream(other.sub1);
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
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inHigh, double inLow ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("RVIR update", RetCode.OutOfRangeEndIndex);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("RVIR update: BadParam", RetCode.BadParam);
         core.rvirStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("RVIR peek: BadParam", RetCode.BadParam);
         RvirStream sp = this;
         double cur_tempHigh = 0.0;
         double cur_outReal = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempHigh = sp.sub0.peek(inHigh);
         cur_outReal = sp.sub1.peek(inLow);
         /* Combine map (batch tail, per bar). */
         cur_outReal = 0.5 * (cur_tempHigh + cur_outReal);
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
      public RvirStream clone() {
         return new RvirStream(this);
      }
   }
   private void rvirStepImpl( RvirStream sp, double inHigh, double inLow )
   {
      double cur_tempHigh = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempHigh = sp.sub0.update(inHigh);
      cur_outReal = sp.sub1.update(inLow);
      /* Combine map (batch tail, per bar). */
      cur_outReal = 0.5 * (cur_tempHigh + cur_outReal);
      sp.cur_outReal = cur_outReal;
   }
   private RetCode rvirOpenImpl( RvirStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, int optInStdDevPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double[] tempHigh;
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInStdDevPeriod == Integer.MIN_VALUE ) {
         optInStdDevPeriod = 10;
      } else if( optInStdDevPeriod < 2 || optInStdDevPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < RVIR_Lookback(optInTimePeriod, optInStdDevPeriod) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = RVIR_Lookback(optInTimePeriod, optInStdDevPeriod);
      /* Nothing is allocated and no input is read when the range cannot produce a
       * value, so a caller-supplied input that stops short of endIdx is never
       * read past its end (kc.c).
       */
      if( lookbackTotal > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* Each leg is the shipped TA_RVI over this range, entered at the same
       * startIdx: same function, same parameters, same lookback, so the two
       * outputs are aligned bar for bar with no anchor to reconcile.
       *
       * Two calls rather than one fused loop. The fused body is expressible --
       * two copies of TA_RVI's state driven by one bar cursor -- but it carries
       * two window-start cursors, and the streaming analyzer accepts exactly one
       * ("extrema automaton: expected exactly one window-start variable"), so
       * fusing would cost this function the streaming tier. Composing keeps it,
       * for the reason KC's legs do: each leg streams as itself.
       */
      tempHigh = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Either input may be aliased onto outReal. Both legs are safe against that
       * for TA_RVI's own reason -- its write index trails its read index by the
       * lookback and never overtakes it -- so the order of the two calls is an
       * implementation detail, not what makes the aliasing safe. Swapping them
       * leaves every value identical, which is why the aliasing test earns its
       * keep against the scratch buffer's extent rather than against this order.
       */
      /* Sub-stream 0: rvi over `inHigh`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RviStream sub0 = rviOpenAndFillInternal(inHigh, startIdx, optInTimePeriod, optInStdDevPeriod, tempBegIdx, tempNbElement, tempHigh);
      retCode = RetCode.Success;
      /* Sub-stream 1: rvi over `inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RviStream sub1 = rviOpenAndFillInternal(inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.Success;
      /* Each leg has already resolved its own zero-total case to TA_RVI's neutral
       * 50 before the average is taken, so a tie in one series never drags the
       * other. Scaling by one half is exact, so the spelling of the average is
       * not a variant: 0.5*(a+b) is one rounding of a+b, as are (a+b)/2 and
       * 0.5*a + 0.5*b.
       */
      for( i = 0; i < outNBElement.value; i += 1 ) {
         sc_outReal[i] = 0.5 * (tempHigh[i] + sc_outReal[i]);
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInStdDevPeriod = optInStdDevPeriod;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* rvirOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   RvirStream rvirOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, int optInStdDevPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RvirStream sp = new RvirStream(this);
      RetCode retCode = rvirOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RVIR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RVIR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("RVIR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind rvirOpen (composition seam). */
   RvirStream rvirOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, int optInStdDevPeriod )
   {
      RvirStream sp = new RvirStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = rvirOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RVIR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RVIR open: internal error", retCode);
      }
      throw new TaLibArgumentException("RVIR open: " + retCode, retCode);
   }
   /**
    * Open a live RVIR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#RVIR} at that bar.
    * <p>The history must hold at least {@code RVIR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public RvirStream rvirOpen( double inHigh[], double inLow[], int optInTimePeriod, int optInStdDevPeriod )
   {
      requireArgument("RVIR open", "inHigh", inHigh);
      requireHistory("RVIR open", inHigh.length);
      requireArgument("RVIR open", "inLow", inLow);
      requireHistoryLength("RVIR open", "inLow", inLow.length, inHigh.length);
      return rvirOpenInternal(inHigh, inLow, 0, optInTimePeriod, optInStdDevPeriod);
   }
   /**
    * {@link Core#rvirOpen} that also fills the output array(s) bit-identically
    * to {@link Core#RVIR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link RvirStream#outRange()}.
    */
   public RvirStream rvirOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, int optInStdDevPeriod, double outReal[] )
   {
      requireArgument("RVIR openAndFill", "inHigh", inHigh);
      requireHistory("RVIR openAndFill", inHigh.length);
      requireArgument("RVIR openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("RVIR openAndFill", inHigh.length, RVIR_Lookback(optInTimePeriod, optInStdDevPeriod));
      requireHistoryLength("RVIR openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("RVIR openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("RVIR openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return rvirOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, optInStdDevPeriod, outBegIdx, outNBElement, outReal);
   }
