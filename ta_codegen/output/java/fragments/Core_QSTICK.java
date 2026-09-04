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
 *  081226 KL   Initial version.
 */

   /**
    * Number of leading input bars {@link Core#QSTICK} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars averaged. Default 10, matching Tulip
    *        Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
    *        documents 1, and AmiBroker community code commonly uses 8 (default 10;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int QSTICK_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode QSTICK_Impl( int startIdx,
                        int endIdx,
                        double inOpen[],
                        double inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double periodTotal = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Qstick (Chande & Kroll, The New Technical Trader, 1994): a simple moving
       * average of the candle body, close minus open. Above zero means bodies
       * were predominantly bullish over the window; the zero crossings are the
       * signal.
       *
       * This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
       * (inClose[x] - inOpen[x]), and deliberately nothing else: the same
       * running-sum order, the same read-before-write of the trailing term, and
       * the same divide by the period. Keeping the arithmetic identical is what
       * makes the composed reference in test_composite.c -- TA_SUB followed by
       * TA_SMA -- bit-exact rather than merely close, so any future drift in
       * either path is a hard failure instead of a tolerance argument.
       *
       * In particular the last statement divides; it does NOT multiply by a
       * precomputed 1/period. Tulip's qstick.c multiplies, which costs it up to
       * one ULP against TA_SMA. Dividing buys the memcmp differential, which is
       * the stronger of the two gates.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      periodTotal = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            periodTotal += (double)(inClose[i] - inOpen[i]);
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows outReal to be the same
       * buffer as either input.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         periodTotal += (double)(inClose[i] - inOpen[i]);
         i = i + 1;
         tempReal = periodTotal;
         periodTotal -= (double)(inClose[trailingIdx] - inOpen[trailingIdx]);
         trailingIdx = trailingIdx + 1;
         outReal[outIdx] = tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode QSTICK_Impl( int startIdx,
                        int endIdx,
                        float inOpen[],
                        float inClose[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      double periodTotal = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = (int)(optInTimePeriod - 1);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      periodTotal = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            periodTotal += (double)((double)inClose[i] - (double)inOpen[i]);
            i = i + 1;
         }
      }
      outIdx = 0;
      while( i <= endIdx ) {
         periodTotal += (double)((double)inClose[i] - (double)inOpen[i]);
         i = i + 1;
         tempReal = periodTotal;
         periodTotal -= (double)((double)inClose[trailingIdx] - (double)inOpen[trailingIdx]);
         trailingIdx = trailingIdx + 1;
         outReal[outIdx] = tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Tushar Chande and Stanley Kroll's Qstick (*The New Technical Trader*,
    * 1994): a simple moving average of the candle body, close minus open. It
    * measures how bullish or bearish the bodies have been over the window,
    * independently of the wicks — above zero the bodies closed up on balance,
    * below zero they closed down, and the zero-line crossings are the signal.
    * <p><b>Formula</b>
    * <pre>{@code
    * body_t = close_t - open_t; QSTICK_t = ( Σ body over the last `optInTimePeriod` bars ) / optInTimePeriod
    * The moving average is a plain SMA, so there is no seeding convention and none of the cross-library divergence that comes with one. `optInTimePeriod` of 1 leaves the raw body.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#QSTICK_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars averaged. Default 10, matching Tulip
    *        Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
    *        documents 1, and AmiBroker community code commonly uses 8 (default 10;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Average candle body over the window. Must hold at least
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
    * @see Core#CMO
    * @see Core#IMI
    * @see Core#MOM
    * @see Core#SMA
    */
   public OutRange QSTICK( int startIdx,
                           int endIdx,
                           double inOpen[],
                           double inClose[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("QSTICK", startIdx, endIdx);
      int guardStart = clampedStart("QSTICK", startIdx, QSTICK_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("QSTICK", "inOpen", inOpen, guardInLen);
      requireLength("QSTICK", "inClose", inClose, guardInLen);
      requireLength("QSTICK", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = QSTICK_Impl(startIdx, endIdx, inOpen, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("QSTICK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Tushar Chande and Stanley Kroll's Qstick (*The New Technical Trader*,
    * 1994): a simple moving average of the candle body, close minus open. It
    * measures how bullish or bearish the bodies have been over the window,
    * independently of the wicks — above zero the bodies closed up on balance,
    * below zero they closed down, and the zero-line crossings are the signal.
    * <p><b>Formula</b>
    * <pre>{@code
    * body_t = close_t - open_t; QSTICK_t = ( Σ body over the last `optInTimePeriod` bars ) / optInTimePeriod
    * The moving average is a plain SMA, so there is no seeding convention and none of the cross-library divergence that comes with one. `optInTimePeriod` of 1 leaves the raw body.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#QSTICK_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Number of bars averaged. Default 10, matching Tulip
    *        Indicators and pandas-ta-classic. Other packages differ: TraderEvolution
    *        documents 1, and AmiBroker community code commonly uses 8 (default 10;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Average candle body over the window. Must hold at least
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
    * @see Core#CMO
    * @see Core#IMI
    * @see Core#MOM
    * @see Core#SMA
    */
   public OutRange QSTICK( int startIdx,
                           int endIdx,
                           float inOpen[],
                           float inClose[],
                           int optInTimePeriod,
                           double outReal[] )
   {
      requireIndexRange("QSTICK", startIdx, endIdx);
      int guardStart = clampedStart("QSTICK", startIdx, QSTICK_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("QSTICK", "inOpen", inOpen, guardInLen);
      requireLength("QSTICK", "inClose", inClose, guardInLen);
      requireLength("QSTICK", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = QSTICK_Impl(startIdx, endIdx, inOpen, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("QSTICK", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live QSTICK stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#QSTICK} over the same series.
    * Open with {@link Core#qstickOpen}; there is no close — the handle is
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
   public static final class QstickStream {
      Core core;
      int optInTimePeriod;
      double periodTotal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_derived;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      QstickStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#QSTICK} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      QstickStream( QstickStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.periodTotal = other.periodTotal;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_derived = other.ring_trailingIdx_derived.clone();
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
      public double update( double inOpen, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("QSTICK update: BadParam", RetCode.BadParam);
         }
         core.qstickStepImpl(this, inOpen, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inOpen.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inOpen[], double inClose[], double outReal[] ) {
         requireArgument("QSTICK updateAndFill", "inOpen", inOpen);
         requireArgument("QSTICK updateAndFill", "inClose", inClose);
         requireArgument("QSTICK updateAndFill", "outReal", outReal);
         final int barCount = inOpen.length;
         if( inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("QSTICK updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inOpen[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("QSTICK updateAndFill: BadParam", RetCode.BadParam);
            }
            core.qstickStepImpl(this, inOpen[i], inClose[i]);
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
      public double peek( double inOpen, double inClose ) {
         if( !Double.isFinite(inOpen) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("QSTICK peek: BadParam", RetCode.BadParam);
         QstickStream sp = this;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         double periodTotal = sp.periodTotal;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = (double)(inClose - inOpen);
         }
         periodTotal += (double)(inClose - inOpen);
         tempReal = periodTotal;
         periodTotal -= (sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] : pkVal0;
         cur_outReal = tempReal / (double)sp.optInTimePeriod;
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
      public QstickStream clone() {
         return new QstickStream(this);
      }
   }
   void qstickStepImpl( QstickStream sp, double inOpen, double inClose )
   {
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_derived[0] = (double)(inClose - inOpen);
      }
      sp.periodTotal += (double)(inClose - inOpen);
      tempReal = sp.periodTotal;
      sp.periodTotal -= sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx];
      sp.cur_outReal = tempReal / (double)sp.optInTimePeriod;
      sp.ring_trailingIdx_derived[sp.ringPos_trailingIdx] = (double)(inClose - inOpen);
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode qstickOpenImpl( QstickStream sp, double inOpen[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double periodTotal = 0;
      double tempReal = 0;
      int i = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Qstick (Chande & Kroll, The New Technical Trader, 1994): a simple moving
       * average of the candle body, close minus open. Above zero means bodies
       * were predominantly bullish over the window; the zero crossings are the
       * signal.
       *
       * This is ta_codegen/input/sma/sma.c with inReal[x] replaced by
       * (inClose[x] - inOpen[x]), and deliberately nothing else: the same
       * running-sum order, the same read-before-write of the trailing term, and
       * the same divide by the period. Keeping the arithmetic identical is what
       * makes the composed reference in test_composite.c -- TA_SUB followed by
       * TA_SMA -- bit-exact rather than merely close, so any future drift in
       * either path is a hard failure instead of a tolerance argument.
       *
       * In particular the last statement divides; it does NOT multiply by a
       * precomputed 1/period. Tulip's qstick.c multiplies, which costs it up to
       * one ULP against TA_SMA. Dividing buys the memcmp differential, which is
       * the stronger of the two gates.
       */
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = (int)(optInTimePeriod - 1);
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
      /* Do the MA calculation using tight loops. */
      /* Add-up the initial period, except for the last value. */
      periodTotal = 0.0;
      trailingIdx = startIdx - lookbackTotal;
      i = trailingIdx;
      if( optInTimePeriod > 1 ) {
         while( i < startIdx ) {
            periodTotal += (double)(inClose[i] - inOpen[i]);
            i = i + 1;
         }
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows outReal to be the same
       * buffer as either input.
       */
      outIdx = 0;
      while( i <= endIdx ) {
         periodTotal += (double)(inClose[i] - inOpen[i]);
         i = i + 1;
         tempReal = periodTotal;
         periodTotal -= (double)(inClose[trailingIdx] - inOpen[trailingIdx]);
         trailingIdx = trailingIdx + 1;
         outReal[outIdx * outStride] = tempReal / (double)optInTimePeriod;
         outIdx = outIdx + 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = i - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_derived = new double[allocN_trailingIdx];
      for( int fillJ = historyLen - cap_trailingIdx; fillJ < historyLen; fillJ++ ) {
         capRing_trailingIdx_derived[fillJ - (historyLen - cap_trailingIdx)] = (double)(inClose[fillJ] - inOpen[fillJ]);
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.periodTotal = periodTotal;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_derived = capRing_trailingIdx_derived;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* qstickOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   QstickStream qstickOpenAndFillInternal( double inOpen[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      QstickStream sp = new QstickStream(this);
      RetCode retCode = qstickOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("QSTICK openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("QSTICK openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("QSTICK openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind qstickOpen (composition seam). */
   QstickStream qstickOpenInternal( double inOpen[], double inClose[], int startIdx, int optInTimePeriod )
   {
      QstickStream sp = new QstickStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = qstickOpenImpl(sp, inOpen, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("QSTICK open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("QSTICK open: internal error", retCode);
      }
      throw new TaLibArgumentException("QSTICK open: " + retCode, retCode);
   }
   /**
    * Open a live QSTICK stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#QSTICK} at that bar.
    * <p>The history must hold at least {@code QSTICK_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public QstickStream qstickOpen( double inOpen[], double inClose[], int optInTimePeriod )
   {
      requireArgument("QSTICK open", "inOpen", inOpen);
      requireHistory("QSTICK open", inOpen.length);
      requireArgument("QSTICK open", "inClose", inClose);
      requireHistoryLength("QSTICK open", "inClose", inClose.length, inOpen.length);
      return qstickOpenInternal(inOpen, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#qstickOpen} that also fills the output array(s) bit-identically
    * to {@link Core#QSTICK} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link QstickStream#outRange()}.
    */
   public QstickStream qstickOpenAndFill( double inOpen[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("QSTICK openAndFill", "inOpen", inOpen);
      requireHistory("QSTICK openAndFill", inOpen.length);
      requireArgument("QSTICK openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("QSTICK openAndFill", inOpen.length, QSTICK_Lookback(optInTimePeriod));
      requireHistoryLength("QSTICK openAndFill", "inClose", inClose.length, inOpen.length);
      requireLength("QSTICK openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inOpen || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("QSTICK openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return qstickOpenAndFillInternal(inOpen, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
