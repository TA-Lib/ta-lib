/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100126 KL,CC  Creation (#486).
 *  100326 MF,CC  The newest output on one fused step (#486).
 */

   /**
    * Number of leading input bars {@link Core#swakBp} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Centre period of the band; the filter passes this
    *        one untouched (default 20; range 5..2000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInDelta Half-bandwidth as a fraction of the centre period;
    *        smaller is a narrower band and a longer settling transient (default 0.1;
    *        range 0.05..0.5; {@link Core#REAL_DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int swakBpLookback( int optInTimePeriod, double optInDelta )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 5 || optInTimePeriod > 2000 ) {
         return -1;
      }
      if( optInDelta == REAL_DEFAULT ) {
         optInDelta = 1e-1;
      } else if( !(optInDelta >= 5e-2 && optInDelta <= 5e-1) ) {
         return -1;
      }
      /* No structural lookback: the two input slots and the two output slots are
       * seeded from the first bar rather than read from before it, and there is
       * no callee whose lookback could be inherited.
       */
      return this.unstablePeriod[FuncUnstId.SWAK_BP.ordinal()] ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#swakBp}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Centre period of the band; the filter passes this
    *        one untouched (default 20; range 5..2000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInDelta Half-bandwidth as a fraction of the centre period;
    *        smaller is a narrower band and a longer settling transient (default 0.1;
    *        range 0.05..0.5; {@link Core#REAL_DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int swakBpDisplayShift( int optInTimePeriod, double optInDelta, int outputIdx )
   {
      if( swakBpLookback( optInTimePeriod, optInDelta ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode swakBpImpl( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double optInDelta,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double w = 0;
      double beta = 0;
      double t = 0;
      double abp = 0;
      double c0 = 0;
      double a1 = 0;
      double a2 = 0;
      double x0 = 0;
      double x1 = 0;
      double x2 = 0;
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
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 5 || optInTimePeriod > 2000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInDelta == REAL_DEFAULT ) {
         optInDelta = 1e-1;
      } else if( !(optInDelta >= 5e-2 && optInDelta <= 5e-1) ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = swakBpLookback(optInTimePeriod, optInDelta);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      /* The band-pass row (Ehlers, Stocks & Commodities January 2006, Figure 5).
       * The paper's 360/P is a full turn, so 2*pi/P; beta is the cosine of the
       * centre frequency and t is the half-bandwidth angle.
       */
      w = 2.0 * 3.141592653589793 / (double)optInTimePeriod;
      beta = Math.cos(w);
      t = 4.0 * 3.141592653589793 * optInDelta / (double)optInTimePeriod;
      /* abp is written as (1 - sin t)/cos t rather than the paper's
       * gamma - sqrt(gamma^2 - 1) with gamma = 1/cos t. The two are equal for
       * 0 < t < pi/2, which this function's ranges guarantee (t <= 0.4*pi), but
       * the published form cancels as t -> 0 because gamma^2 - 1 goes as t^2.
       * Against a 60-digit reference the published form is already 4.5e-13 off
       * at the period cap; this one is not, and that is what lets the cap be
       * 2000 rather than 1000.
       */
      abp = (1.0 - Math.sin(t)) / Math.cos(t);
      /* The numerator (1, 0, -1) is zero both on a constant and on a Nyquist
       * alternation, so this row answers 0 at DC and at Nyquist, and exactly 1
       * with zero phase at the centre period itself.
       */
      c0 = (1.0 - abp) / 2.0;
      a1 = beta * (1.0 + abp);
      a2 = -abp;
      today = startIdx - lookbackTotal;
      /* Start in the steady state of a constant input equal to the first bar: a
       * band-pass of a constant is 0, so both output slots start there while both
       * input slots hold the bar. The input slots are carried in locals and never
       * re-read from inReal, because outReal may alias it.
       */
      x1 = inReal[today];
      x2 = x1;
      y1 = 0.0;
      y2 = 0.0;
      /* Skip the unstable period: run the recurrence but publish nothing. */
      i = lookbackTotal;
      while( i != 0 ) {
         x0 = inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - x2)));
         x2 = x1;
         x1 = x0;
         y2 = y1;
         y1 = y;
         today += 1;
         i -= 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
          * rate is the latency of whatever y1 crosses to become y: outermost, that
          * is one fused step. Nested inside, it is three, and every backend's last
          * bit moves with it.
          */
         x0 = inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - x2)));
         x2 = x1;
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
   RetCode swakBpImpl( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double optInDelta,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double w = 0;
      double beta = 0;
      double t = 0;
      double abp = 0;
      double c0 = 0;
      double a1 = 0;
      double a2 = 0;
      double x0 = 0;
      double x1 = 0;
      double x2 = 0;
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
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 5 || optInTimePeriod > 2000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInDelta == REAL_DEFAULT ) {
         optInDelta = 1e-1;
      } else if( !(optInDelta >= 5e-2 && optInDelta <= 5e-1) ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = swakBpLookback(optInTimePeriod, optInDelta);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      w = 2.0 * 3.141592653589793 / (double)optInTimePeriod;
      beta = Math.cos(w);
      t = 4.0 * 3.141592653589793 * optInDelta / (double)optInTimePeriod;
      abp = (1.0 - Math.sin(t)) / Math.cos(t);
      c0 = (1.0 - abp) / 2.0;
      a1 = beta * (1.0 + abp);
      a2 = -abp;
      today = startIdx - lookbackTotal;
      x1 = (double)inReal[today];
      x2 = x1;
      y1 = 0.0;
      y2 = 0.0;
      i = lookbackTotal;
      while( i != 0 ) {
         x0 = (double)inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - x2)));
         x2 = x1;
         x1 = x0;
         y2 = y1;
         y1 = y;
         today += 1;
         i -= 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         x0 = (double)inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - x2)));
         x2 = x1;
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
    * The band-pass row of John Ehlers' Swiss Army Knife filter: a cycle
    * extractor that keeps a band of periods around a chosen centre and removes
    * everything on both sides of it. Read it as an oscillator, not as price. It
    * answers zero on a constant, zero on a bar-to-bar alternation, and exactly
    * the input — same amplitude, no phase shift — on a sine wave at the centre
    * period. Between those it tapers, with the half-power points near
    * {@code P(1 ± delta)}: at a centre of 20 bars and a delta of 0.1 the band
    * is roughly 20 ± 2 bars. What separates it from the high-pass rows is that
    * it rejects the fast end too. A detrender keeps everything above its
    * cutoff, including the bar-to-bar noise; this keeps only the band asked
    * for.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/swak_bp">ta-lib.org/functions/swak_bp</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#swakBpLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to filter; Ehlers' default is the bar midpoint
    *        {@code (H+L)/2}, which the caller passes as {@code TA_MEDPRICE} output.
    * @param optInTimePeriod Centre period of the band; the filter passes this
    *        one untouched (default 20; range 5..2000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInDelta Half-bandwidth as a fraction of the centre period;
    *        smaller is a narrower band and a longer settling transient (default 0.1;
    *        range 0.05..0.5; {@link Core#REAL_DEFAULT} selects the default).
    * @param outReal The extracted cycle, centred on zero. Must hold at least
    *        {@code endIdx - max(startIdx, swakBpLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#swakHp
    * @see Core#swak2php
    * @see Core#medprice
    */
   public OutRange swakBp( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double optInDelta,
                           double outReal[] )
   {
      requireIndexRange("SWAK_BP", startIdx, endIdx);
      int guardStart = clampedStart("SWAK_BP", startIdx, swakBpLookback(optInTimePeriod, optInDelta));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SWAK_BP", "inReal", inReal, guardInLen);
      requireLength("SWAK_BP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = swakBpImpl(startIdx, endIdx, inReal, optInTimePeriod, optInDelta, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SWAK_BP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The band-pass row of John Ehlers' Swiss Army Knife filter: a cycle
    * extractor that keeps a band of periods around a chosen centre and removes
    * everything on both sides of it. Read it as an oscillator, not as price. It
    * answers zero on a constant, zero on a bar-to-bar alternation, and exactly
    * the input — same amplitude, no phase shift — on a sine wave at the centre
    * period. Between those it tapers, with the half-power points near
    * {@code P(1 ± delta)}: at a centre of 20 bars and a delta of 0.1 the band
    * is roughly 20 ± 2 bars. What separates it from the high-pass rows is that
    * it rejects the fast end too. A detrender keeps everything above its
    * cutoff, including the bar-to-bar noise; this keeps only the band asked
    * for.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/swak_bp">ta-lib.org/functions/swak_bp</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#swakBpLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to filter; Ehlers' default is the bar midpoint
    *        {@code (H+L)/2}, which the caller passes as {@code TA_MEDPRICE} output.
    * @param optInTimePeriod Centre period of the band; the filter passes this
    *        one untouched (default 20; range 5..2000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInDelta Half-bandwidth as a fraction of the centre period;
    *        smaller is a narrower band and a longer settling transient (default 0.1;
    *        range 0.05..0.5; {@link Core#REAL_DEFAULT} selects the default).
    * @param outReal The extracted cycle, centred on zero. Must hold at least
    *        {@code endIdx - max(startIdx, swakBpLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#swakHp
    * @see Core#swak2php
    * @see Core#medprice
    */
   public OutRange swakBp( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double optInDelta,
                           double outReal[] )
   {
      requireIndexRange("SWAK_BP", startIdx, endIdx);
      int guardStart = clampedStart("SWAK_BP", startIdx, swakBpLookback(optInTimePeriod, optInDelta));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SWAK_BP", "inReal", inReal, guardInLen);
      requireLength("SWAK_BP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = swakBpImpl(startIdx, endIdx, inReal, optInTimePeriod, optInDelta, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SWAK_BP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SWAK_BP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#swakBp} over the same series.
    * Open with {@link Core#swakBpOpen}; there is no close — the handle is
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
   public static final class SwakBpStream {
      private Core core;
      private int optInTimePeriod;
      private double optInDelta;
      private double c0;
      private double a1;
      private double a2;
      private double x1;
      private double x2;
      private double y1;
      private double y2;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SwakBpStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#swakBp} reports over the same bars: the
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
            throw failure("SWAK_BP advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SwakBpStream( SwakBpStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInDelta = other.optInDelta;
         this.c0 = other.c0;
         this.a1 = other.a1;
         this.a2 = other.a2;
         this.x1 = other.x1;
         this.x2 = other.x2;
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
            throw failure("SWAK_BP update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SWAK_BP update", "inReal");
         core.swakBpStepImpl(this, inReal);
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
            throw nonFiniteBar("SWAK_BP peek", "inReal");
         SwakBpStream sp = this;
         double x0 = 0.0;
         double y = 0.0;
         double cur_outReal = 0.0;
         double x1 = sp.x1;
         double x2 = sp.x2;
         double y1 = sp.y1;
         double y2 = sp.y2;
         /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
          * rate is the latency of whatever y1 crosses to become y: outermost, that
          * is one fused step. Nested inside, it is three, and every backend's last
          * bit moves with it.
          */
         x0 = inReal;
         y = Math.fma(sp.a1, y1, Math.fma(sp.a2, y2, sp.c0 * (x0 - x2)));
         x2 = x1;
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
      public SwakBpStream clone() {
         return new SwakBpStream(this);
      }
   }
   private void swakBpStepImpl( SwakBpStream sp, double inReal )
   {
      double x0 = 0.0;
      double y = 0.0;
      /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
       * rate is the latency of whatever y1 crosses to become y: outermost, that
       * is one fused step. Nested inside, it is three, and every backend's last
       * bit moves with it.
       */
      x0 = inReal;
      y = Math.fma(sp.a1, sp.y1, Math.fma(sp.a2, sp.y2, sp.c0 * (x0 - sp.x2)));
      sp.x2 = sp.x1;
      sp.x1 = x0;
      sp.y2 = sp.y1;
      sp.y1 = y;
      sp.cur_outReal = y;
   }
   private RetCode swakBpOpenImpl( SwakBpStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInDelta, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double w = 0;
      double beta = 0;
      double t = 0;
      double abp = 0;
      double c0 = 0;
      double a1 = 0;
      double a2 = 0;
      double x0 = 0;
      double x1 = 0;
      double x2 = 0;
      double y = 0;
      double y1 = 0;
      double y2 = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 5 || optInTimePeriod > 2000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInDelta == REAL_DEFAULT ) {
         optInDelta = 1e-1;
      } else if( !(optInDelta >= 5e-2 && optInDelta <= 5e-1) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = swakBpLookback(optInTimePeriod, optInDelta);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* The band-pass row (Ehlers, Stocks & Commodities January 2006, Figure 5).
       * The paper's 360/P is a full turn, so 2*pi/P; beta is the cosine of the
       * centre frequency and t is the half-bandwidth angle.
       */
      w = 2.0 * 3.141592653589793 / (double)optInTimePeriod;
      beta = Math.cos(w);
      t = 4.0 * 3.141592653589793 * optInDelta / (double)optInTimePeriod;
      /* abp is written as (1 - sin t)/cos t rather than the paper's
       * gamma - sqrt(gamma^2 - 1) with gamma = 1/cos t. The two are equal for
       * 0 < t < pi/2, which this function's ranges guarantee (t <= 0.4*pi), but
       * the published form cancels as t -> 0 because gamma^2 - 1 goes as t^2.
       * Against a 60-digit reference the published form is already 4.5e-13 off
       * at the period cap; this one is not, and that is what lets the cap be
       * 2000 rather than 1000.
       */
      abp = (1.0 - Math.sin(t)) / Math.cos(t);
      /* The numerator (1, 0, -1) is zero both on a constant and on a Nyquist
       * alternation, so this row answers 0 at DC and at Nyquist, and exactly 1
       * with zero phase at the centre period itself.
       */
      c0 = (1.0 - abp) / 2.0;
      a1 = beta * (1.0 + abp);
      a2 = -abp;
      today = startIdx - lookbackTotal;
      /* Start in the steady state of a constant input equal to the first bar: a
       * band-pass of a constant is 0, so both output slots start there while both
       * input slots hold the bar. The input slots are carried in locals and never
       * re-read from inReal, because outReal may alias it.
       */
      x1 = inReal[today];
      x2 = x1;
      y1 = 0.0;
      y2 = 0.0;
      /* Skip the unstable period: run the recurrence but publish nothing. */
      i = lookbackTotal;
      while( i != 0 ) {
         x0 = inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - x2)));
         x2 = x1;
         x1 = x0;
         y2 = y1;
         y1 = y;
         today += 1;
         i -= 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
          * rate is the latency of whatever y1 crosses to become y: outermost, that
          * is one fused step. Nested inside, it is three, and every backend's last
          * bit moves with it.
          */
         x0 = inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - x2)));
         x2 = x1;
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
      sp.optInDelta = optInDelta;
      sp.c0 = c0;
      sp.a1 = a1;
      sp.a2 = a2;
      sp.x1 = x1;
      sp.x2 = x2;
      sp.y1 = y1;
      sp.y2 = y2;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* swakBpOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SwakBpStream swakBpOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInDelta, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SwakBpStream sp = new SwakBpStream(this);
      RetCode retCode = swakBpOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInDelta, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SWAK_BP openAndFill", inReal.length, startIdx, swakBpLookback(optInTimePeriod, optInDelta));
      }
      throw streamFailure("SWAK_BP openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind swakBpOpen (composition seam). */
   SwakBpStream swakBpOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInDelta )
   {
      SwakBpStream sp = new SwakBpStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = swakBpOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInDelta, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SWAK_BP open", inReal.length, startIdx, swakBpLookback(optInTimePeriod, optInDelta));
      }
      throw streamFailure("SWAK_BP open", retCode);
   }
   /**
    * Open a live SWAK_BP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#swakBp} at that bar.
    * <p>The history must hold at least {@code swakBpLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SwakBpStream swakBpOpen( double inReal[], int optInTimePeriod, double optInDelta )
   {
      requireArgument("SWAK_BP open", "inReal", inReal);
      requireHistory("SWAK_BP open", inReal.length);
      return swakBpOpenInternal(inReal, 0, optInTimePeriod, optInDelta);
   }
   /**
    * {@link Core#swakBpOpen} that also fills the output array(s) bit-identically
    * to {@link Core#swakBp} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SwakBpStream#outRange()}.
    */
   public SwakBpStream swakBpOpenAndFill( double inReal[], int optInTimePeriod, double optInDelta, double outReal[] )
   {
      requireArgument("SWAK_BP openAndFill", "inReal", inReal);
      requireHistory("SWAK_BP openAndFill", inReal.length);
      int guardOutLen = openFillCount("SWAK_BP openAndFill", inReal.length, swakBpLookback(optInTimePeriod, optInDelta));
      requireLength("SWAK_BP openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("SWAK_BP openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return swakBpOpenAndFillInternal(inReal, 0, optInTimePeriod, optInDelta, outBegIdx, outNBElement, outReal);
   }
