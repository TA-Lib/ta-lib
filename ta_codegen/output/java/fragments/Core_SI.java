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
 *  092726 MF,CC  Initial version (#451).
 */

   /**
    * Number of leading input bars {@link Core#si} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInLimitMove Limit move, the largest one-bar price move the index
    *        is scaled against, in price units (default 3; minimum 0.00000001;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int siLookback( double optInLimitMove )
   {
      if( optInLimitMove == REAL_DEFAULT ) {
         optInLimitMove = 3e0;
      } else if( !(optInLimitMove >= 1e-8 && optInLimitMove <= REAL_MAX) ) {
         return -1;
      }
      return 1 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#si}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInLimitMove Limit move, the largest one-bar price move the index
    *        is scaled against, in price units (default 3; minimum 0.00000001;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int siDisplayShift( double optInLimitMove, int outputIdx )
   {
      if( siLookback( optInLimitMove ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode siImpl( int startIdx,
                   int endIdx,
                   double inOpen[],
                   double inHigh[],
                   double inLow[],
                   double inClose[],
                   double optInLimitMove,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double prevClose = 0;
      double prevBody = 0;
      double tempOpen = 0;
      double tempHigh = 0;
      double tempLow = 0;
      double tempClose = 0;
      double body = 0;
      double n = 0;
      double up = 0;
      double dn = 0;
      double rg = 0;
      double k = 0;
      double r = 0;
      double swing = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInLimitMove == REAL_DEFAULT ) {
         optInLimitMove = 3e0;
      } else if( !(optInLimitMove >= 1e-8 && optInLimitMove <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = siLookback(optInLimitMove);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      prevClose = inClose[startIdx - 1];
      prevBody = prevClose - inOpen[startIdx - 1];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempOpen = inOpen[i];
         tempHigh = inHigh[i];
         tempLow = inLow[i];
         tempClose = inClose[i];
         body = tempClose - tempOpen;
         n = tempClose - prevClose;
         n += 0.5 * body;
         n += 0.25 * prevBody;
         up = Math.abs(tempHigh - prevClose);
         dn = Math.abs(tempLow - prevClose);
         rg = Math.abs(tempHigh - tempLow);
         k = Math.max(up, dn);
         /* Wilder's three cases of R are this one max, since rg is up + dn or
          * |up - dn|. Keep it branch-free: the case chain mispredicts on most
          * bars of real data. The four backends' max builtins agree only while
          * no operand is -0.0 or NaN, which holds while up and dn are finite.
          */
         r = Math.max(up - 0.5 * dn, dn - 0.5 * up);
         r = Math.max(r, rg);
         r += 0.25 * Math.abs(prevBody);
         if( r == 0.0 ) {
            swing = 0.0;
         } else {
            swing = 50.0 * (n / r) * (k / optInLimitMove);
         }
         outReal[outIdx] = swing;
         outIdx += 1;
         prevClose = tempClose;
         prevBody = body;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode siImpl( int startIdx,
                   int endIdx,
                   float inOpen[],
                   float inHigh[],
                   float inLow[],
                   float inClose[],
                   double optInLimitMove,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outReal[] )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double prevClose = 0;
      double prevBody = 0;
      double tempOpen = 0;
      double tempHigh = 0;
      double tempLow = 0;
      double tempClose = 0;
      double body = 0;
      double n = 0;
      double up = 0;
      double dn = 0;
      double rg = 0;
      double k = 0;
      double r = 0;
      double swing = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInLimitMove == REAL_DEFAULT ) {
         optInLimitMove = 3e0;
      } else if( !(optInLimitMove >= 1e-8 && optInLimitMove <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = siLookback(optInLimitMove);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      prevClose = (double)inClose[startIdx - 1];
      prevBody = prevClose - (double)inOpen[startIdx - 1];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempOpen = (double)inOpen[i];
         tempHigh = (double)inHigh[i];
         tempLow = (double)inLow[i];
         tempClose = (double)inClose[i];
         body = tempClose - tempOpen;
         n = tempClose - prevClose;
         n += 0.5 * body;
         n += 0.25 * prevBody;
         up = Math.abs(tempHigh - prevClose);
         dn = Math.abs(tempLow - prevClose);
         rg = Math.abs(tempHigh - tempLow);
         k = Math.max(up, dn);
         r = Math.max(up - 0.5 * dn, dn - 0.5 * up);
         r = Math.max(r, rg);
         r += 0.25 * Math.abs(prevBody);
         if( r == 0.0 ) {
            swing = 0.0;
         } else {
            swing = 50.0 * (n / r) * (k / optInLimitMove);
         }
         outReal[outIdx] = swing;
         outIdx += 1;
         prevClose = tempClose;
         prevBody = body;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Wilder's Swing Index: one number rating each bar against the bar before
    * it, from both bars' open and close and the current bar's high and low,
    * signed by the direction of the swing. A strong close above the prior close
    * on a wide range reads high and positive, a strong down swing reads high
    * and negative. It is scaled against the limit move, the largest one-bar
    * move the market allows, and stays between -100 and +100 while every move
    * is within that limit. J. Welles Wilder Jr. introduced it in 1978 and
    * trades its running total, ASI.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/si">ta-lib.org/functions/si</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder rounds each value to a whole number when working by hand; the output is not rounded.</li>
    * <li>A bar that moves more than the limit move can read beyond -100 or +100; the output is not clamped.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#siLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInLimitMove Limit move, the largest one-bar price move the index
    *        is scaled against, in price units (default 3; minimum 0.00000001;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param outReal Swing index of the bar against the previous bar. Must hold
    *        at least {@code endIdx - max(startIdx, siLookback(...)) + 1} values, and
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
    * @see Core#asi
    * @see Core#wad
    * @see Core#trange
    * @see Core#bop
    */
   public OutRange si( int startIdx,
                       int endIdx,
                       double inOpen[],
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       double optInLimitMove,
                       double outReal[] )
   {
      requireIndexRange("SI", startIdx, endIdx);
      int guardStart = clampedStart("SI", startIdx, siLookback(optInLimitMove));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SI", "inOpen", inOpen, guardInLen);
      requireLength("SI", "inHigh", inHigh, guardInLen);
      requireLength("SI", "inLow", inLow, guardInLen);
      requireLength("SI", "inClose", inClose, guardInLen);
      requireLength("SI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = siImpl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInLimitMove, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Wilder's Swing Index: one number rating each bar against the bar before
    * it, from both bars' open and close and the current bar's high and low,
    * signed by the direction of the swing. A strong close above the prior close
    * on a wide range reads high and positive, a strong down swing reads high
    * and negative. It is scaled against the limit move, the largest one-bar
    * move the market allows, and stays between -100 and +100 while every move
    * is within that limit. J. Welles Wilder Jr. introduced it in 1978 and
    * trades its running total, ASI.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/si">ta-lib.org/functions/si</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Wilder rounds each value to a whole number when working by hand; the output is not rounded.</li>
    * <li>A bar that moves more than the limit move can read beyond -100 or +100; the output is not clamped.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#siLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInLimitMove Limit move, the largest one-bar price move the index
    *        is scaled against, in price units (default 3; minimum 0.00000001;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param outReal Swing index of the bar against the previous bar. Must hold
    *        at least {@code endIdx - max(startIdx, siLookback(...)) + 1} values, and
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
    * @see Core#asi
    * @see Core#wad
    * @see Core#trange
    * @see Core#bop
    */
   public OutRange si( int startIdx,
                       int endIdx,
                       float inOpen[],
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       double optInLimitMove,
                       double outReal[] )
   {
      requireIndexRange("SI", startIdx, endIdx);
      int guardStart = clampedStart("SI", startIdx, siLookback(optInLimitMove));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SI", "inOpen", inOpen, guardInLen);
      requireLength("SI", "inHigh", inHigh, guardInLen);
      requireLength("SI", "inLow", inLow, guardInLen);
      requireLength("SI", "inClose", inClose, guardInLen);
      requireLength("SI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = siImpl(startIdx, endIdx, inOpen, inHigh, inLow, inClose, optInLimitMove, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#si} over the same series.
    * Open with {@link Core#siOpen}; there is no close — the handle is
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
   public static final class SiStream {
      private Core core;
      private double optInLimitMove;
      private double prevClose;
      private double prevBody;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#si} reports over the same bars: the
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
            throw failure("SI advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SiStream( SiStream other ) {
         this.core = other.core;
         this.optInLimitMove = other.optInLimitMove;
         this.prevClose = other.prevClose;
         this.prevBody = other.prevBody;
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
      public double update( double inOpen, double inHigh, double inLow, double inClose ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("SI update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("SI update", !Double.isFinite(inOpen) ? "inOpen" : !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.siStepImpl(this, inOpen, inHigh, inLow, inClose);
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
      public double peek( double inOpen, double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("SI peek", !Double.isFinite(inOpen) ? "inOpen" : !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         SiStream sp = this;
         double tempOpen = 0.0;
         double tempHigh = 0.0;
         double tempLow = 0.0;
         double tempClose = 0.0;
         double body = 0.0;
         double n = 0.0;
         double up = 0.0;
         double dn = 0.0;
         double rg = 0.0;
         double k = 0.0;
         double r = 0.0;
         double swing = 0.0;
         double cur_outReal = 0.0;
         tempOpen = inOpen;
         tempHigh = inHigh;
         tempLow = inLow;
         tempClose = inClose;
         body = tempClose - tempOpen;
         n = tempClose - sp.prevClose;
         n += 0.5 * body;
         n += 0.25 * sp.prevBody;
         up = Math.abs(tempHigh - sp.prevClose);
         dn = Math.abs(tempLow - sp.prevClose);
         rg = Math.abs(tempHigh - tempLow);
         k = Math.max(up, dn);
         /* Wilder's three cases of R are this one max, since rg is up + dn or
          * |up - dn|. Keep it branch-free: the case chain mispredicts on most
          * bars of real data. The four backends' max builtins agree only while
          * no operand is -0.0 or NaN, which holds while up and dn are finite.
          */
         r = Math.max(up - 0.5 * dn, dn - 0.5 * up);
         r = Math.max(r, rg);
         r += 0.25 * Math.abs(sp.prevBody);
         if( r == 0.0 ) {
            swing = 0.0;
         } else {
            swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
         }
         cur_outReal = swing;
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
      public SiStream clone() {
         return new SiStream(this);
      }
   }
   private void siStepImpl( SiStream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      double tempOpen = 0.0;
      double tempHigh = 0.0;
      double tempLow = 0.0;
      double tempClose = 0.0;
      double body = 0.0;
      double n = 0.0;
      double up = 0.0;
      double dn = 0.0;
      double rg = 0.0;
      double k = 0.0;
      double r = 0.0;
      double swing = 0.0;
      tempOpen = inOpen;
      tempHigh = inHigh;
      tempLow = inLow;
      tempClose = inClose;
      body = tempClose - tempOpen;
      n = tempClose - sp.prevClose;
      n += 0.5 * body;
      n += 0.25 * sp.prevBody;
      up = Math.abs(tempHigh - sp.prevClose);
      dn = Math.abs(tempLow - sp.prevClose);
      rg = Math.abs(tempHigh - tempLow);
      k = Math.max(up, dn);
      /* Wilder's three cases of R are this one max, since rg is up + dn or
       * |up - dn|. Keep it branch-free: the case chain mispredicts on most
       * bars of real data. The four backends' max builtins agree only while
       * no operand is -0.0 or NaN, which holds while up and dn are finite.
       */
      r = Math.max(up - 0.5 * dn, dn - 0.5 * up);
      r = Math.max(r, rg);
      r += 0.25 * Math.abs(sp.prevBody);
      if( r == 0.0 ) {
         swing = 0.0;
      } else {
         swing = 50.0 * (n / r) * (k / sp.optInLimitMove);
      }
      sp.cur_outReal = swing;
      sp.prevClose = tempClose;
      sp.prevBody = body;
   }
   private RetCode siOpenImpl( SiStream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, double optInLimitMove, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      double prevClose = 0;
      double prevBody = 0;
      double tempOpen = 0;
      double tempHigh = 0;
      double tempLow = 0;
      double tempClose = 0;
      double body = 0;
      double n = 0;
      double up = 0;
      double dn = 0;
      double rg = 0;
      double k = 0;
      double r = 0;
      double swing = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLimitMove == REAL_DEFAULT ) {
         optInLimitMove = 3e0;
      } else if( !(optInLimitMove >= 1e-8 && optInLimitMove <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      lookbackTotal = siLookback(optInLimitMove);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      prevClose = inClose[startIdx - 1];
      prevBody = prevClose - inOpen[startIdx - 1];
      outIdx = 0;
      for( i = startIdx; i <= endIdx; i += 1 ) {
         tempOpen = inOpen[i];
         tempHigh = inHigh[i];
         tempLow = inLow[i];
         tempClose = inClose[i];
         body = tempClose - tempOpen;
         n = tempClose - prevClose;
         n += 0.5 * body;
         n += 0.25 * prevBody;
         up = Math.abs(tempHigh - prevClose);
         dn = Math.abs(tempLow - prevClose);
         rg = Math.abs(tempHigh - tempLow);
         k = Math.max(up, dn);
         /* Wilder's three cases of R are this one max, since rg is up + dn or
          * |up - dn|. Keep it branch-free: the case chain mispredicts on most
          * bars of real data. The four backends' max builtins agree only while
          * no operand is -0.0 or NaN, which holds while up and dn are finite.
          */
         r = Math.max(up - 0.5 * dn, dn - 0.5 * up);
         r = Math.max(r, rg);
         r += 0.25 * Math.abs(prevBody);
         if( r == 0.0 ) {
            swing = 0.0;
         } else {
            swing = 50.0 * (n / r) * (k / optInLimitMove);
         }
         outReal[outIdx * outStride] = swing;
         outIdx += 1;
         prevClose = tempClose;
         prevBody = body;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInLimitMove = optInLimitMove;
      sp.prevClose = prevClose;
      sp.prevBody = prevBody;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* siOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SiStream siOpenAndFillInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, double optInLimitMove, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      SiStream sp = new SiStream(this);
      RetCode retCode = siOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SI openAndFill", inOpen.length, startIdx, siLookback(optInLimitMove));
      }
      throw streamFailure("SI openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind siOpen (composition seam). */
   SiStream siOpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx, double optInLimitMove )
   {
      SiStream sp = new SiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = siOpenImpl(sp, inOpen, inHigh, inLow, inClose, startIdx, optInLimitMove, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SI open", inOpen.length, startIdx, siLookback(optInLimitMove));
      }
      throw streamFailure("SI open", retCode);
   }
   /**
    * Open a live SI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#si} at that bar.
    * <p>The history must hold at least {@code siLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Core#REAL_DEFAULT} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SiStream siOpen( double inOpen[], double inHigh[], double inLow[], double inClose[], double optInLimitMove )
   {
      requireArgument("SI open", "inOpen", inOpen);
      requireHistory("SI open", inOpen.length);
      requireArgument("SI open", "inHigh", inHigh);
      requireArgument("SI open", "inLow", inLow);
      requireArgument("SI open", "inClose", inClose);
      requireHistoryLength("SI open", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("SI open", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("SI open", "inClose", inClose.length, inOpen.length);
      return siOpenInternal(inOpen, inHigh, inLow, inClose, 0, optInLimitMove);
   }
   /**
    * {@link Core#siOpen} that also fills the output array(s) bit-identically
    * to {@link Core#si} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SiStream#outRange()}.
    */
   public SiStream siOpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], double optInLimitMove, double outReal[] )
   {
      requireArgument("SI openAndFill", "inOpen", inOpen);
      requireHistory("SI openAndFill", inOpen.length);
      requireArgument("SI openAndFill", "inHigh", inHigh);
      requireArgument("SI openAndFill", "inLow", inLow);
      requireArgument("SI openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("SI openAndFill", inOpen.length, siLookback(optInLimitMove));
      requireHistoryLength("SI openAndFill", "inHigh", inHigh.length, inOpen.length);
      requireHistoryLength("SI openAndFill", "inLow", inLow.length, inOpen.length);
      requireHistoryLength("SI openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("SI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw streamFailure("SI openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return siOpenAndFillInternal(inOpen, inHigh, inLow, inClose, 0, optInLimitMove, outBegIdx, outNBElement, outReal);
   }
