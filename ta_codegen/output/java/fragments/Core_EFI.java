/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version (#206).
 */

   /**
    * Number of leading input bars {@link Core#EFI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod EMA period applied to the force series (default 13;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int EFI_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* One bar is consumed forming the first close-to-close change, then the
       * EMA's own warm-up on top:
       *    1 + ema_lookback(optInTimePeriod)
       *  = 1 + (optInTimePeriod - 1) + TA_GetUnstablePeriod(TA_FUNC_UNST_EMA)
       */
      return optInTimePeriod + this.unstablePeriod[FuncUnstId.EMA.ordinal()] ;

   }
   RetCode EFI_Impl( int startIdx,
                     int endIdx,
                     double inClose[],
                     double inVolume[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      double prevClose = 0;
      double force = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
       * close-to-close move weighted by that bar's volume, then smoothed with an
       * EMA. Elder's 2-period reading is the short-term form and 13 the
       * intermediate-term one -- that is the parameter, not a second formula.
       *
       *    force[t] = ( close[t] - close[t-1] ) * volume[t]
       *    EFI      = EMA( force, optInTimePeriod )
       *
       * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
       * in exactly that shape on purpose: the seed accumulates from 0.0 in the
       * same order, and the recurrence is (x - prevMA)*k + prevMA rather than the
       * algebraically equal k*x + (1-k)*prevMA. That order IS the bit-exactness
       * contract against the composed reference in test_composite.c -- MOM, then
       * MULT, then EMA -- so do not tidy it. TRIX carries the same warning.
       *
       * Nothing on the data path divides by an input, so issue #112 is satisfied
       * structurally: a flat close gives force exactly 0.0 and output exactly
       * 0.0, and zero volume likewise. The only division is by the period, a
       * positive integer parameter.
       *
       * prevClose is carried in a scalar rather than re-read from inClose[t-1]
       * because the C API allows outReal to alias an input: at bar t the slot
       * holding close[t-1] may already have been overwritten by the output
       * written a bar earlier. cmou.c carries its trailing value for the same
       * reason.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = EFI_Lookback(optInTimePeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* No smoothing at a period of 1: the output is the raw Force Index.
       * Explicit for the reason spelled out in ema.c -- at period 1 optInK_1 is
       * exactly 1.0, so the recursion reduces to (x-prev)+prev, which returns x
       * only while consecutive values stay within a factor of two of each other.
       * Force values swing by orders of magnitude, far more than the prices EMA
       * warns about.
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         prevClose = inClose[today - 1];
         while( today <= endIdx ) {
            force = (inClose[today] - prevClose) * inVolume[today];
            prevClose = inClose[today];
            outReal[outIdx] = force;
            outIdx = outIdx + 1;
            today = today + 1;
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      /* The first EMA value is a simple average of the first 'period' force
       * values; it then seeds the recursion. This is ema.c's seeding applied
       * to the force series rather than to the input array.
       */
      today = startIdx - lookbackTotal + 1;
      prevClose = inClose[today - 1];
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         force = (inClose[today] - prevClose) * inVolume[today];
         prevClose = inClose[today];
         tempReal += force;
         today = today + 1;
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         force = (inClose[today] - prevClose) * inVolume[today];
         prevClose = inClose[today];
         prevMA = Math.fma(force - prevMA, optInK_1, prevMA);
         today = today + 1;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         force = (inClose[today] - prevClose) * inVolume[today];
         prevClose = inClose[today];
         prevMA = Math.fma(force - prevMA, optInK_1, prevMA);
         outReal[outIdx] = prevMA;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode EFI_Impl( int startIdx,
                     int endIdx,
                     float inClose[],
                     float inVolume[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double optInK_1 = 0;
      double tempReal = 0;
      double prevMA = 0;
      double prevClose = 0;
      double force = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
      lookbackTotal = EFI_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         prevClose = (double)inClose[today - 1];
         while( today <= endIdx ) {
            force = ((double)inClose[today] - prevClose) * (double)inVolume[today];
            prevClose = (double)inClose[today];
            outReal[outIdx] = force;
            outIdx = outIdx + 1;
            today = today + 1;
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      today = startIdx - lookbackTotal + 1;
      prevClose = (double)inClose[today - 1];
      i = optInTimePeriod;
      tempReal = 0.0;
      while( i-- > 0 ) {
         force = ((double)inClose[today] - prevClose) * (double)inVolume[today];
         prevClose = (double)inClose[today];
         tempReal += force;
         today = today + 1;
      }
      prevMA = tempReal / optInTimePeriod;
      while( today <= startIdx ) {
         force = ((double)inClose[today] - prevClose) * (double)inVolume[today];
         prevClose = (double)inClose[today];
         prevMA = Math.fma(force - prevMA, optInK_1, prevMA);
         today = today + 1;
      }
      outReal[0] = prevMA;
      outIdx = 1;
      while( today <= endIdx ) {
         force = ((double)inClose[today] - prevClose) * (double)inVolume[today];
         prevClose = (double)inClose[today];
         prevMA = Math.fma(force - prevMA, optInK_1, prevMA);
         outReal[outIdx] = prevMA;
         outIdx = outIdx + 1;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Alexander Elder's Force Index (<i>Trading for a Living</i>, 1993):
    * volume-weighted momentum. Each bar's close-to-close move is weighted by
    * that bar's volume, and the result is smoothed with an exponential moving
    * average. The sign is the direction of the move; the size combines how far
    * price travelled with how much volume stood behind it. Elder reads two
    * settings — 2 for the short term, which he pairs with a 22-period EMA of
    * price to mark corrections against an established trend, and 13 for the
    * intermediate term, the default here. A divergence against price can be
    * confirmed by a zero-line cross. Beyond Elder, much longer settings are
    * also in use, 100 or so, for the longer-term balance between buyers and
    * sellers. Nothing normalises the result, so it scales with the instrument's
    * own volume: read its sign and its shape over time, not its level against
    * another instrument.
    * <p><b>Formula</b>
    * <pre>{@code
    * force_t = ( close_t - close_{t-1} ) * volume_t; EFI = EMA( force, optInTimePeriod )
    * The EMA is TA-Lib's, seeded with a simple average of the first `optInTimePeriod` force values. A period of 1 leaves the raw one-bar Force Index.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#EFI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod EMA period applied to the force series (default 13;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Smoothed force. Must hold at least
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
    * @see Core#AD
    * @see Core#EMA
    * @see Core#MFI
    * @see Core#OBV
    * @see Core#PVO
    */
   public OutRange EFI( int startIdx,
                        int endIdx,
                        double inClose[],
                        double inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("EFI", startIdx, endIdx);
      int guardStart = clampedStart("EFI", startIdx, EFI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("EFI", "inClose", inClose, guardInLen);
      requireLength("EFI", "inVolume", inVolume, guardInLen);
      requireLength("EFI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = EFI_Impl(startIdx, endIdx, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("EFI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Alexander Elder's Force Index (<i>Trading for a Living</i>, 1993):
    * volume-weighted momentum. Each bar's close-to-close move is weighted by
    * that bar's volume, and the result is smoothed with an exponential moving
    * average. The sign is the direction of the move; the size combines how far
    * price travelled with how much volume stood behind it. Elder reads two
    * settings — 2 for the short term, which he pairs with a 22-period EMA of
    * price to mark corrections against an established trend, and 13 for the
    * intermediate term, the default here. A divergence against price can be
    * confirmed by a zero-line cross. Beyond Elder, much longer settings are
    * also in use, 100 or so, for the longer-term balance between buyers and
    * sellers. Nothing normalises the result, so it scales with the instrument's
    * own volume: read its sign and its shape over time, not its level against
    * another instrument.
    * <p><b>Formula</b>
    * <pre>{@code
    * force_t = ( close_t - close_{t-1} ) * volume_t; EFI = EMA( force, optInTimePeriod )
    * The EMA is TA-Lib's, seeded with a simple average of the first `optInTimePeriod` force values. A period of 1 leaves the raw one-bar Force Index.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#EFI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inClose Close price of each bar.
    * @param inVolume Volume of each bar.
    * @param optInTimePeriod EMA period applied to the force series (default 13;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Smoothed force. Must hold at least
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
    * @see Core#AD
    * @see Core#EMA
    * @see Core#MFI
    * @see Core#OBV
    * @see Core#PVO
    */
   public OutRange EFI( int startIdx,
                        int endIdx,
                        float inClose[],
                        float inVolume[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("EFI", startIdx, endIdx);
      int guardStart = clampedStart("EFI", startIdx, EFI_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("EFI", "inClose", inClose, guardInLen);
      requireLength("EFI", "inVolume", inVolume, guardInLen);
      requireLength("EFI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = EFI_Impl(startIdx, endIdx, inClose, inVolume, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("EFI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live EFI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#EFI} over the same series.
    * Open with {@link Core#efiOpen}; there is no close — the handle is
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
   public static final class EfiStream {
      Core core;
      int optInTimePeriod;
      double prevClose;
      double optInK_1;
      double prevMA;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      EfiStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#EFI} reports over the same bars: the
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
            throw failure("EFI advance", RetCode.OutOfRangeEndIndex);
         this.outRangeCount++;
      }

      EfiStream( EfiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevClose = other.prevClose;
         this.optInK_1 = other.optInK_1;
         this.prevMA = other.prevMA;
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
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inClose, double inVolume ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("EFI update", RetCode.OutOfRangeEndIndex);
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("EFI update: BadParam", RetCode.BadParam);
         core.efiStepImpl(this, inClose, inVolume);
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
      public double peek( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("EFI peek: BadParam", RetCode.BadParam);
         EfiStream sp = this;
         double cur_outReal = 0.0;
         if( sp.optInTimePeriod == 1 ) {
            double force = 0.0;
            double prevClose = sp.prevClose;
            force = (inClose - prevClose) * inVolume;
            prevClose = inClose;
            cur_outReal = force;
         } else {
            double force = 0.0;
            double prevClose = sp.prevClose;
            double prevMA = sp.prevMA;
            force = (inClose - prevClose) * inVolume;
            prevClose = inClose;
            prevMA = Math.fma(force - prevMA, sp.optInK_1, prevMA);
            cur_outReal = prevMA;
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
      public EfiStream clone() {
         return new EfiStream(this);
      }
   }
   void efiStepImpl( EfiStream sp, double inClose, double inVolume )
   {
      if( sp.optInTimePeriod == 1 ) {
         double force = 0.0;
         force = (inClose - sp.prevClose) * inVolume;
         sp.prevClose = inClose;
         sp.cur_outReal = force;
      } else {
         double force = 0.0;
         force = (inClose - sp.prevClose) * inVolume;
         sp.prevClose = inClose;
         sp.prevMA = Math.fma(force - sp.prevMA, sp.optInK_1, sp.prevMA);
         sp.cur_outReal = sp.prevMA;
      }
   }
   private RetCode efiOpenImpl( EfiStream sp, double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inVolume.length != inClose.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 13;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         double optInK_1 = 0;
         double tempReal = 0;
         double prevMA = 0;
         double prevClose = 0;
         double force = 0;
         int i = 0;
         int today = 0;
         int outIdx = 0;
         int lookbackTotal = 0;
         optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
         /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
          * close-to-close move weighted by that bar's volume, then smoothed with an
          * EMA. Elder's 2-period reading is the short-term form and 13 the
          * intermediate-term one -- that is the parameter, not a second formula.
          *
          *    force[t] = ( close[t] - close[t-1] ) * volume[t]
          *    EFI      = EMA( force, optInTimePeriod )
          *
          * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
          * in exactly that shape on purpose: the seed accumulates from 0.0 in the
          * same order, and the recurrence is (x - prevMA)*k + prevMA rather than the
          * algebraically equal k*x + (1-k)*prevMA. That order IS the bit-exactness
          * contract against the composed reference in test_composite.c -- MOM, then
          * MULT, then EMA -- so do not tidy it. TRIX carries the same warning.
          *
          * Nothing on the data path divides by an input, so issue #112 is satisfied
          * structurally: a flat close gives force exactly 0.0 and output exactly
          * 0.0, and zero volume likewise. The only division is by the period, a
          * positive integer parameter.
          *
          * prevClose is carried in a scalar rather than re-read from inClose[t-1]
          * because the C API allows outReal to alias an input: at bar t the slot
          * holding close[t-1] may already have been overwritten by the output
          * written a bar earlier. cmou.c carries its trailing value for the same
          * reason.
          */
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
         lookbackTotal = EFI_Lookback(optInTimePeriod);
         /* Move up the start index if there is not
          * enough initial data.
          */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.InsufficientHistory ;
         }
         /* No smoothing at a period of 1: the output is the raw Force Index.
          * Explicit for the reason spelled out in ema.c -- at period 1 optInK_1 is
          * exactly 1.0, so the recursion reduces to (x-prev)+prev, which returns x
          * only while consecutive values stay within a factor of two of each other.
          * Force values swing by orders of magnitude, far more than the prices EMA
          * warns about.
          */
         outBegIdx.value = startIdx;
         outIdx = 0;
         today = startIdx;
         prevClose = inClose[today - 1];
         while( today <= endIdx ) {
            force = (inClose[today] - prevClose) * inVolume[today];
            prevClose = inClose[today];
            outReal[outIdx * outStride] = force;
            outIdx = outIdx + 1;
            today = today + 1;
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevClose = prevClose;
         sp.optInK_1 = optInK_1;
         sp.prevMA = prevMA;
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      } else {
         double optInK_1 = 0;
         double tempReal = 0;
         double prevMA = 0;
         double prevClose = 0;
         double force = 0;
         int i = 0;
         int today = 0;
         int outIdx = 0;
         int lookbackTotal = 0;
         optInK_1 = 2.0 / (double)(optInTimePeriod + 1);
         /* Alexander Elder's Force Index (Trading for a Living, 1993): the one-bar
          * close-to-close move weighted by that bar's volume, then smoothed with an
          * EMA. Elder's 2-period reading is the short-term form and 13 the
          * intermediate-term one -- that is the parameter, not a second formula.
          *
          *    force[t] = ( close[t] - close[t-1] ) * volume[t]
          *    EFI      = EMA( force, optInTimePeriod )
          *
          * The arithmetic below is ema.c's with inReal[t] replaced by force[t], kept
          * in exactly that shape on purpose: the seed accumulates from 0.0 in the
          * same order, and the recurrence is (x - prevMA)*k + prevMA rather than the
          * algebraically equal k*x + (1-k)*prevMA. That order IS the bit-exactness
          * contract against the composed reference in test_composite.c -- MOM, then
          * MULT, then EMA -- so do not tidy it. TRIX carries the same warning.
          *
          * Nothing on the data path divides by an input, so issue #112 is satisfied
          * structurally: a flat close gives force exactly 0.0 and output exactly
          * 0.0, and zero volume likewise. The only division is by the period, a
          * positive integer parameter.
          *
          * prevClose is carried in a scalar rather than re-read from inClose[t-1]
          * because the C API allows outReal to alias an input: at bar t the slot
          * holding close[t-1] may already have been overwritten by the output
          * written a bar earlier. cmou.c carries its trailing value for the same
          * reason.
          */
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
         lookbackTotal = EFI_Lookback(optInTimePeriod);
         /* Move up the start index if there is not
          * enough initial data.
          */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.InsufficientHistory ;
         }
         /* No smoothing at a period of 1: the output is the raw Force Index.
          * Explicit for the reason spelled out in ema.c -- at period 1 optInK_1 is
          * exactly 1.0, so the recursion reduces to (x-prev)+prev, which returns x
          * only while consecutive values stay within a factor of two of each other.
          * Force values swing by orders of magnitude, far more than the prices EMA
          * warns about.
          */
         outBegIdx.value = startIdx;
         /* The first EMA value is a simple average of the first 'period' force
          * values; it then seeds the recursion. This is ema.c's seeding applied
          * to the force series rather than to the input array.
          */
         today = startIdx - lookbackTotal + 1;
         prevClose = inClose[today - 1];
         i = optInTimePeriod;
         tempReal = 0.0;
         while( i-- > 0 ) {
            force = (inClose[today] - prevClose) * inVolume[today];
            prevClose = inClose[today];
            tempReal += force;
            today = today + 1;
         }
         prevMA = tempReal / optInTimePeriod;
         while( today <= startIdx ) {
            force = (inClose[today] - prevClose) * inVolume[today];
            prevClose = inClose[today];
            prevMA = Math.fma(force - prevMA, optInK_1, prevMA);
            today = today + 1;
         }
         outReal[0 * outStride] = prevMA;
         outIdx = 1;
         while( today <= endIdx ) {
            force = (inClose[today] - prevClose) * inVolume[today];
            prevClose = inClose[today];
            prevMA = Math.fma(force - prevMA, optInK_1, prevMA);
            outReal[outIdx * outStride] = prevMA;
            outIdx = outIdx + 1;
            today = today + 1;
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevClose = prevClose;
         sp.optInK_1 = optInK_1;
         sp.prevMA = prevMA;
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
   }
   /* efiOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   EfiStream efiOpenAndFillInternal( double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      EfiStream sp = new EfiStream(this);
      RetCode retCode = efiOpenImpl(sp, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("EFI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("EFI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("EFI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind efiOpen (composition seam). */
   EfiStream efiOpenInternal( double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      EfiStream sp = new EfiStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = efiOpenImpl(sp, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("EFI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("EFI open: internal error", retCode);
      }
      throw new TaLibArgumentException("EFI open: " + retCode, retCode);
   }
   /**
    * Open a live EFI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#EFI} at that bar.
    * <p>The history must hold at least {@code EFI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public EfiStream efiOpen( double inClose[], double inVolume[], int optInTimePeriod )
   {
      requireArgument("EFI open", "inClose", inClose);
      requireHistory("EFI open", inClose.length);
      requireArgument("EFI open", "inVolume", inVolume);
      requireHistoryLength("EFI open", "inVolume", inVolume.length, inClose.length);
      return efiOpenInternal(inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#efiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#EFI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link EfiStream#outRange()}.
    */
   public EfiStream efiOpenAndFill( double inClose[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("EFI openAndFill", "inClose", inClose);
      requireHistory("EFI openAndFill", inClose.length);
      requireArgument("EFI openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("EFI openAndFill", inClose.length, EFI_Lookback(optInTimePeriod));
      requireHistoryLength("EFI openAndFill", "inVolume", inVolume.length, inClose.length);
      requireLength("EFI openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("EFI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return efiOpenAndFillInternal(inClose, inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
