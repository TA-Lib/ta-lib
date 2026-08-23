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
       * values; it then seeds the recursion. This is ema.c's CLASSIC seeding
       * applied to the force series rather than to the input array.
       *
       * TA_GetCompatibility() is deliberately NOT consulted. ema.c still carries
       * a TA_COMPATIBILITY_METASTOCK seeding arm, but that capability is being
       * deprecated: it is preserved for the functions that already shipped with
       * it and dropped from new ones, and it is not reachable at all from the
       * Rust, Java and C# APIs, which expose no TA_SetCompatibility. Honouring it
       * here would make EFI's C output diverge from the other three backends for
       * a setting they cannot even read.
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
    * Alexander Elder's Force Index (*Trading for a Living*, 1993):
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
      int guardStart = clampedStart(startIdx, endIdx, EFI_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Alexander Elder's Force Index (*Trading for a Living*, 1993):
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
      int guardStart = clampedStart(startIdx, endIdx, EFI_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#EFI_Open}; there is no close — the handle is
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
   public static final class EFI_Stream {
      Core core;
      int optInTimePeriod;
      double prevClose;
      double optInK_1;
      double prevMA;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      EFI_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#EFI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      EFI_Stream( EFI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevClose = other.prevClose;
         this.optInK_1 = other.optInK_1;
         this.prevMA = other.prevMA;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( EFI_Stream other ) {
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
      public double update( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("EFI update: BadParam", RetCode.BadParam);
         core.EFI_StepImpl(this, inClose, inVolume);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inClose.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inClose[], double inVolume[], double outReal[] ) {
         final int barCount = inClose.length;
         if( inVolume.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume )
            throw new TaLibArgumentException("EFI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inClose[i]) || !Double.isFinite(inVolume[i]) )
               throw new TaLibArgumentException("EFI updateAndFill: BadParam", RetCode.BadParam);
            core.EFI_StepImpl(this, inClose[i], inVolume[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inClose, double inVolume ) {
         if( !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw new TaLibArgumentException("EFI peek: BadParam", RetCode.BadParam);
         EFI_Stream scratch = new EFI_Stream(this);
         core.EFI_StepImpl(scratch, inClose, inVolume);
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
      public EFI_Stream copy() {
         return new EFI_Stream(this);
      }
   }
   void EFI_StepImpl( EFI_Stream sp, double inClose, double inVolume )
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
   private RetCode EFI_OpenImpl( EFI_Stream sp, double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int historyLen = inClose.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inVolume.length != inClose.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
          * values; it then seeds the recursion. This is ema.c's CLASSIC seeding
          * applied to the force series rather than to the input array.
          *
          * TA_GetCompatibility() is deliberately NOT consulted. ema.c still carries
          * a TA_COMPATIBILITY_METASTOCK seeding arm, but that capability is being
          * deprecated: it is preserved for the functions that already shipped with
          * it and dropped from new ones, and it is not reachable at all from the
          * Rust, Java and C# APIs, which expose no TA_SetCompatibility. Honouring it
          * here would make EFI's C output diverge from the other three backends for
          * a setting they cannot even read.
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
   /* EFI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   EFI_Stream EFI_OpenAndFillInternal( double inClose[], double inVolume[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      EFI_Stream sp = new EFI_Stream(this);
      RetCode retCode = EFI_OpenImpl(sp, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
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
   /* Internal startIdx-anchored open behind EFI_Open (composition seam). */
   EFI_Stream EFI_OpenInternal( double inClose[], double inVolume[], int startIdx, int optInTimePeriod )
   {
      EFI_Stream sp = new EFI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = EFI_OpenImpl(sp, inClose, inVolume, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
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
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public EFI_Stream EFI_Open( double inClose[], double inVolume[], int optInTimePeriod )
   {
      return EFI_OpenInternal(inClose, inVolume, 0, optInTimePeriod);
   }
   /**
    * {@link Core#EFI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#EFI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link EFI_Stream#outRange()}.
    */
   public EFI_Stream EFI_OpenAndFill( double inClose[], double inVolume[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inClose || (Object)outReal == (Object)inVolume ) {
         throw new TaLibArgumentException("EFI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return EFI_OpenAndFillInternal(inClose, inVolume, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
