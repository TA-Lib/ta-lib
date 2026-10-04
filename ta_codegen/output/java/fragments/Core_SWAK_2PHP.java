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
    * Number of leading input bars {@link Core#swak2php} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Cutoff period; cycles longer than it are removed,
    *        cycles shorter pass (default 20; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int swak2phpLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return -1;
      }
      /* No structural lookback: the two input slots and the two output slots are
       * seeded from the first bar rather than read from before it, and there is
       * no callee whose lookback could be inherited.
       */
      return this.unstablePeriod[FuncUnstId.SWAK_2PHP.ordinal()] ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#swak2php}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Cutoff period; cycles longer than it are removed,
    *        cycles shorter pass (default 20; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int swak2phpDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( swak2phpLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode swak2phpImpl( int startIdx,
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
      double w = 0;
      double b2p = 0;
      double a2p = 0;
      double om = 0;
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
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = swak2phpLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      /* The two-pole alpha of Ehlers' "Swiss Army Knife Indicator", Figure 5. The
       * paper's 360/P is a full turn, so 2*pi/P.
       */
      w = 2.0 * 3.141592653589793 / (double)optInTimePeriod;
      b2p = 2.415 * (1.0 - Math.cos(w));
      a2p = -b2p + Math.sqrt(Math.fma(b2p, b2p, 2.0 * b2p));
      /* The two-pole high-pass row: the same double real pole as the Gaussian
       * and Butterworth rows, with a (1, -2, 1) numerator instead. That
       * numerator is zero on a constant, so the DC gain is 0 and the line is
       * centred on zero; at Nyquist it weighs 4, and c0 is exactly what divides
       * that back to unity -- ((2-a2p)/2)^2 times 4/(2-a2p)^2 is 1. Rolling off
       * twice as steeply as the one-pole row is the whole reason to pay for the
       * second pole.
       */
      om = 1.0 - a2p;
      c0 = (1.0 - a2p / 2.0) * (1.0 - a2p / 2.0);
      a1 = 2.0 * om;
      a2 = -(om * om);
      today = startIdx - lookbackTotal;
      /* Start in the steady state of a constant input equal to the first bar: a
       * DC-gain-0 row answers 0 for a constant, so both output slots start there
       * while both input slots hold the bar. The input slots are carried in
       * locals and never re-read from inReal, because outReal may alias it.
       */
      x1 = inReal[today];
      x2 = x1;
      y1 = 0.0;
      y2 = 0.0;
      /* Skip the unstable period: run the recurrence but publish nothing. */
      i = lookbackTotal;
      while( i != 0 ) {
         x0 = inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - 2.0 * x1 + x2)));
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
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - 2.0 * x1 + x2)));
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
   RetCode swak2phpImpl( int startIdx,
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
      double w = 0;
      double b2p = 0;
      double a2p = 0;
      double om = 0;
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
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = swak2phpLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      w = 2.0 * 3.141592653589793 / (double)optInTimePeriod;
      b2p = 2.415 * (1.0 - Math.cos(w));
      a2p = -b2p + Math.sqrt(Math.fma(b2p, b2p, 2.0 * b2p));
      om = 1.0 - a2p;
      c0 = (1.0 - a2p / 2.0) * (1.0 - a2p / 2.0);
      a1 = 2.0 * om;
      a2 = -(om * om);
      today = startIdx - lookbackTotal;
      x1 = (double)inReal[today];
      x2 = x1;
      y1 = 0.0;
      y2 = 0.0;
      i = lookbackTotal;
      while( i != 0 ) {
         x0 = (double)inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - 2.0 * x1 + x2)));
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
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - 2.0 * x1 + x2)));
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
    * The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the
    * same double real pole the Gaussian and Butterworth rows use, with a
    * detrending numerator instead of a smoothing one. It removes what is slower
    * than the cutoff period and rolls off twice as steeply as the one-pole row.
    * Read it as an oscillator, not as price. Its DC gain is 0, so a flat market
    * returns zero and a trend returns its departure from itself. Against
    * {@code TA_SWAK_HP} the difference is the slope of the transition: the
    * second pole buys a sharper separation between what is kept and what is
    * removed.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/swak_2php">ta-lib.org/functions/swak_2php</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#swak2phpLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to detrend; Ehlers' default is the bar midpoint
    *        {@code (H+L)/2}, which the caller passes as {@code TA_MEDPRICE} output.
    * @param optInTimePeriod Cutoff period; cycles longer than it are removed,
    *        cycles shorter pass (default 20; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal The detrended line, centred on zero. Must hold at least
    *        {@code endIdx - max(startIdx, swak2phpLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
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
    * @see Core#swakBp
    * @see Core#medprice
    */
   public OutRange swak2php( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("SWAK_2PHP", startIdx, endIdx);
      int guardStart = clampedStart("SWAK_2PHP", startIdx, swak2phpLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SWAK_2PHP", "inReal", inReal, guardInLen);
      requireLength("SWAK_2PHP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = swak2phpImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SWAK_2PHP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the
    * same double real pole the Gaussian and Butterworth rows use, with a
    * detrending numerator instead of a smoothing one. It removes what is slower
    * than the cutoff period and rolls off twice as steeply as the one-pole row.
    * Read it as an oscillator, not as price. Its DC gain is 0, so a flat market
    * returns zero and a trend returns its departure from itself. Against
    * {@code TA_SWAK_HP} the difference is the slope of the transition: the
    * second pole buys a sharper separation between what is kept and what is
    * removed.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/swak_2php">ta-lib.org/functions/swak_2php</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#swak2phpLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to detrend; Ehlers' default is the bar midpoint
    *        {@code (H+L)/2}, which the caller passes as {@code TA_MEDPRICE} output.
    * @param optInTimePeriod Cutoff period; cycles longer than it are removed,
    *        cycles shorter pass (default 20; range 2..10000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal The detrended line, centred on zero. Must hold at least
    *        {@code endIdx - max(startIdx, swak2phpLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
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
    * @see Core#swakBp
    * @see Core#medprice
    */
   public OutRange swak2php( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("SWAK_2PHP", startIdx, endIdx);
      int guardStart = clampedStart("SWAK_2PHP", startIdx, swak2phpLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SWAK_2PHP", "inReal", inReal, guardInLen);
      requireLength("SWAK_2PHP", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = swak2phpImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SWAK_2PHP", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SWAK_2PHP stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#swak2php} over the same series.
    * Open with {@link Core#swak2phpOpen}; there is no close — the handle is
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
   public static final class Swak2phpStream {
      private Core core;
      private int optInTimePeriod;
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

      private Swak2phpStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#swak2php} reports over the same bars: the
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
            throw failure("SWAK_2PHP advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private Swak2phpStream( Swak2phpStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
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
            throw failure("SWAK_2PHP update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("SWAK_2PHP update", "inReal");
         core.swak2phpStepImpl(this, inReal);
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
            throw nonFiniteBar("SWAK_2PHP peek", "inReal");
         Swak2phpStream sp = this;
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
         y = Math.fma(sp.a1, y1, Math.fma(sp.a2, y2, sp.c0 * (x0 - 2.0 * x1 + x2)));
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
      public Swak2phpStream clone() {
         return new Swak2phpStream(this);
      }
   }
   private void swak2phpStepImpl( Swak2phpStream sp, double inReal )
   {
      double x0 = 0.0;
      double y = 0.0;
      /* a1*y1 stays the outermost term. y1 is the newest output, so the bar
       * rate is the latency of whatever y1 crosses to become y: outermost, that
       * is one fused step. Nested inside, it is three, and every backend's last
       * bit moves with it.
       */
      x0 = inReal;
      y = Math.fma(sp.a1, sp.y1, Math.fma(sp.a2, sp.y2, sp.c0 * (x0 - 2.0 * sp.x1 + sp.x2)));
      sp.x2 = sp.x1;
      sp.x1 = x0;
      sp.y2 = sp.y1;
      sp.y1 = y;
      sp.cur_outReal = y;
   }
   private RetCode swak2phpOpenImpl( Swak2phpStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      double w = 0;
      double b2p = 0;
      double a2p = 0;
      double om = 0;
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
      } else if( optInTimePeriod < 2 || optInTimePeriod > 10000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = swak2phpLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* The two-pole alpha of Ehlers' "Swiss Army Knife Indicator", Figure 5. The
       * paper's 360/P is a full turn, so 2*pi/P.
       */
      w = 2.0 * 3.141592653589793 / (double)optInTimePeriod;
      b2p = 2.415 * (1.0 - Math.cos(w));
      a2p = -b2p + Math.sqrt(Math.fma(b2p, b2p, 2.0 * b2p));
      /* The two-pole high-pass row: the same double real pole as the Gaussian
       * and Butterworth rows, with a (1, -2, 1) numerator instead. That
       * numerator is zero on a constant, so the DC gain is 0 and the line is
       * centred on zero; at Nyquist it weighs 4, and c0 is exactly what divides
       * that back to unity -- ((2-a2p)/2)^2 times 4/(2-a2p)^2 is 1. Rolling off
       * twice as steeply as the one-pole row is the whole reason to pay for the
       * second pole.
       */
      om = 1.0 - a2p;
      c0 = (1.0 - a2p / 2.0) * (1.0 - a2p / 2.0);
      a1 = 2.0 * om;
      a2 = -(om * om);
      today = startIdx - lookbackTotal;
      /* Start in the steady state of a constant input equal to the first bar: a
       * DC-gain-0 row answers 0 for a constant, so both output slots start there
       * while both input slots hold the bar. The input slots are carried in
       * locals and never re-read from inReal, because outReal may alias it.
       */
      x1 = inReal[today];
      x2 = x1;
      y1 = 0.0;
      y2 = 0.0;
      /* Skip the unstable period: run the recurrence but publish nothing. */
      i = lookbackTotal;
      while( i != 0 ) {
         x0 = inReal[today];
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - 2.0 * x1 + x2)));
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
         y = Math.fma(a1, y1, Math.fma(a2, y2, c0 * (x0 - 2.0 * x1 + x2)));
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
   /* swak2phpOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   Swak2phpStream swak2phpOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      Swak2phpStream sp = new Swak2phpStream(this);
      RetCode retCode = swak2phpOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SWAK_2PHP openAndFill", inReal.length, startIdx, swak2phpLookback(optInTimePeriod));
      }
      throw streamFailure("SWAK_2PHP openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind swak2phpOpen (composition seam). */
   Swak2phpStream swak2phpOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      Swak2phpStream sp = new Swak2phpStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = swak2phpOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SWAK_2PHP open", inReal.length, startIdx, swak2phpLookback(optInTimePeriod));
      }
      throw streamFailure("SWAK_2PHP open", retCode);
   }
   /**
    * Open a live SWAK_2PHP stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#swak2php} at that bar.
    * <p>The history must hold at least {@code swak2phpLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public Swak2phpStream swak2phpOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("SWAK_2PHP open", "inReal", inReal);
      requireHistory("SWAK_2PHP open", inReal.length);
      return swak2phpOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#swak2phpOpen} that also fills the output array(s) bit-identically
    * to {@link Core#swak2php} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link Swak2phpStream#outRange()}.
    */
   public Swak2phpStream swak2phpOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("SWAK_2PHP openAndFill", "inReal", inReal);
      requireHistory("SWAK_2PHP openAndFill", inReal.length);
      int guardOutLen = openFillCount("SWAK_2PHP openAndFill", inReal.length, swak2phpLookback(optInTimePeriod));
      requireLength("SWAK_2PHP openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("SWAK_2PHP openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return swak2phpOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
