/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  100502 JV   Speed optimization of the algorithm
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  090404 MF   Fix #978056. Trap sqrt with negative zero values.
 *  082326 MF,CC #243 the sqrt trap moves to var's scale-relative floor.
 */

   /**
    * Number of leading input bars {@link Core#STDDEV} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length (default 5; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the standard deviation (default 1;
    *        {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int STDDEV_Lookback( int optInTimePeriod, double optInNbDev )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return -1;
      }
      /* Lookback is driven by the variance. */
      return VAR_Lookback(optInTimePeriod, optInNbDev) ;

   }
   RetCode STDDEV_Impl( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int i = 0;
      RetCode retCode;
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
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything.
       *
       * Same shape as the guard in apo and bbands: the variance below runs on the
       * same range and its lookback IS stddev's, so it declines and yields 0,0
       * without reading. Observably identical, but it makes "a range shorter than
       * the lookback reads nothing" true of stddev itself rather than only of var.
       * Pinned by the zero-length no-I/O probe over every guarded core.
       */
      if( STDDEV_Lookback(optInTimePeriod, optInNbDev) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Calculate the variance. */
      OutRange _xr0 = VAR(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      /* Calculate the square root of each variance, this
       * is the standard deviation.
       *
       * Multiply also by the ratio specified.
       *
       * Unconditional. var owns the dead-zone and owns the sign: it returns a
       * non-negative variance, already floored to exactly 0 on any window whose
       * re-anchored spread sat under its own rounding noise (var.c). What used to
       * stand here instead - zero the output wherever the variance fell under
       * TA_EPSILON - compared a SQUARED quantity to a fixed 1e-14, which is a cliff
       * at a price level rather than a noise floor: a $100.00 instrument quoted in
       * 1e-8 ticks has a variance around 1e-16 and came back as exactly 0 on every
       * bar, with TA_SUCCESS and nothing to say it had been suppressed (#243).
       * Dropping it also leaves a pure map, which the branch had kept sqrt out of.
       */
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = Math.sqrt(outReal[i]) * optInNbDev;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = Math.sqrt(outReal[i]);
         }
      }
      return RetCode.Success ;
   }
   RetCode STDDEV_Impl( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double optInNbDev,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int i = 0;
      RetCode retCode;
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
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( STDDEV_Lookback(optInTimePeriod, optInNbDev) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      OutRange _xr0 = VAR(startIdx, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = Math.sqrt(outReal[i]) * optInNbDev;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            outReal[i] = Math.sqrt(outReal[i]);
         }
      }
      return RetCode.Success ;
   }
   /**
    * Rolling standard deviation of a series over a window, scaled by a
    * deviations multiplier. Delegates to VAR, then takes the square root.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STDDEV_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to measure dispersion of.
    * @param optInTimePeriod Window length (default 5; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the standard deviation (default 1;
    *        {@code -4e37} selects the default).
    * @param outReal Standard deviation at each bar, scaled by optInNbDev. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#VAR
    * @see Core#BBANDS
    * @see Core#SMA
    */
   public OutRange STDDEV( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double optInNbDev,
                           double outReal[] )
   {
      requireIndexRange("STDDEV", startIdx, endIdx);
      int guardStart = clampedStart("STDDEV", startIdx, STDDEV_Lookback(optInTimePeriod, optInNbDev));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STDDEV", "inReal", inReal, guardInLen);
      requireLength("STDDEV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STDDEV_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("STDDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling standard deviation of a series over a window, scaled by a
    * deviations multiplier. Delegates to VAR, then takes the square root.
    * <p><b>Formula</b>
    * <pre>{@code
    * $\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STDDEV_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to measure dispersion of.
    * @param optInTimePeriod Window length (default 5; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Multiplier applied to the standard deviation (default 1;
    *        {@code -4e37} selects the default).
    * @param outReal Standard deviation at each bar, scaled by optInNbDev. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#VAR
    * @see Core#BBANDS
    * @see Core#SMA
    */
   public OutRange STDDEV( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double optInNbDev,
                           double outReal[] )
   {
      requireIndexRange("STDDEV", startIdx, endIdx);
      int guardStart = clampedStart("STDDEV", startIdx, STDDEV_Lookback(optInTimePeriod, optInNbDev));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STDDEV", "inReal", inReal, guardInLen);
      requireLength("STDDEV", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STDDEV_Impl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("STDDEV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STDDEV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#STDDEV} over the same series.
    * Open with {@link Core#stddevOpen}; there is no close — the handle is
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
   public static final class StddevStream {
      Core core;
      int optInTimePeriod;
      double optInNbDev;
      double cur_outReal;
      VarStream sub0;
      int outRangeBegIdx;
      int outRangeCount;

      StddevStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#STDDEV} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      StddevStream( StddevStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDev = other.optInNbDev;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new VarStream(other.sub0);
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
            throw new TaLibArgumentException("STDDEV update: BadParam", RetCode.BadParam);
         }
         core.stddevStepImpl(this, inReal);
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
         requireArgument("STDDEV updateAndFill", "inReal", inReal);
         requireArgument("STDDEV updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("STDDEV updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("STDDEV updateAndFill: BadParam", RetCode.BadParam);
            }
            core.stddevStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("STDDEV peek: BadParam", RetCode.BadParam);
         StddevStream sp = this;
         double cur_outReal = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_outReal = sp.sub0.peek(inReal);
         /* Combine map (batch tail, per bar). */
         if( sp.optInNbDev != 1.0 ) {
            cur_outReal = Math.sqrt(cur_outReal) * sp.optInNbDev;
         } else {
            cur_outReal = Math.sqrt(cur_outReal);
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
      public StddevStream clone() {
         return new StddevStream(this);
      }
   }
   void stddevStepImpl( StddevStream sp, double inReal )
   {
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_outReal = sp.sub0.update(inReal);
      /* Combine map (batch tail, per bar). */
      if( sp.optInNbDev != 1.0 ) {
         cur_outReal = Math.sqrt(cur_outReal) * sp.optInNbDev;
      } else {
         cur_outReal = Math.sqrt(cur_outReal);
      }
      sp.cur_outReal = cur_outReal;
   }
   private RetCode stddevOpenImpl( StddevStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int i = 0;
      RetCode retCode;
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
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 1e0;
      } else if( !(optInNbDev >= REAL_MIN && optInNbDev <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < STDDEV_Lookback(optInTimePeriod, optInNbDev) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything.
       *
       * Same shape as the guard in apo and bbands: the variance below runs on the
       * same range and its lookback IS stddev's, so it declines and yields 0,0
       * without reading. Observably identical, but it makes "a range shorter than
       * the lookback reads nothing" true of stddev itself rather than only of var.
       * Pinned by the zero-length no-I/O probe over every guarded core.
       */
      if( STDDEV_Lookback(optInTimePeriod, optInNbDev) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Calculate the variance. */
      /* Sub-stream 0: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarStream sub0 = varOpenAndFillInternal(inReal, startIdx, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.Success;
      /* Calculate the square root of each variance, this
       * is the standard deviation.
       *
       * Multiply also by the ratio specified.
       *
       * Unconditional. var owns the dead-zone and owns the sign: it returns a
       * non-negative variance, already floored to exactly 0 on any window whose
       * re-anchored spread sat under its own rounding noise (var.c). What used to
       * stand here instead - zero the output wherever the variance fell under
       * TA_EPSILON - compared a SQUARED quantity to a fixed 1e-14, which is a cliff
       * at a price level rather than a noise floor: a $100.00 instrument quoted in
       * 1e-8 ticks has a variance around 1e-16 and came back as exactly 0 on every
       * bar, with TA_SUCCESS and nothing to say it had been suppressed (#243).
       * Dropping it also leaves a pure map, which the branch had kept sqrt out of.
       */
      if( optInNbDev != 1.0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            sc_outReal[i] = Math.sqrt(sc_outReal[i]) * optInNbDev;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            sc_outReal[i] = Math.sqrt(sc_outReal[i]);
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDev = optInNbDev;
      sp.sub0 = sub0;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* stddevOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   StddevStream stddevOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      StddevStream sp = new StddevStream(this);
      RetCode retCode = stddevOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STDDEV openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STDDEV openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("STDDEV openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind stddevOpen (composition seam). */
   StddevStream stddevOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDev )
   {
      StddevStream sp = new StddevStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = stddevOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STDDEV open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STDDEV open: internal error", retCode);
      }
      throw new TaLibArgumentException("STDDEV open: " + retCode, retCode);
   }
   /**
    * Open a live STDDEV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#STDDEV} at that bar.
    * <p>The history must hold at least {@code STDDEV_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public StddevStream stddevOpen( double inReal[], int optInTimePeriod, double optInNbDev )
   {
      requireArgument("STDDEV open", "inReal", inReal);
      requireHistory("STDDEV open", inReal.length);
      return stddevOpenInternal(inReal, 0, optInTimePeriod, optInNbDev);
   }
   /**
    * {@link Core#stddevOpen} that also fills the output array(s) bit-identically
    * to {@link Core#STDDEV} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link StddevStream#outRange()}.
    */
   public StddevStream stddevOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDev, double outReal[] )
   {
      requireArgument("STDDEV openAndFill", "inReal", inReal);
      requireHistory("STDDEV openAndFill", inReal.length);
      int guardOutLen = openFillCount("STDDEV openAndFill", inReal.length, STDDEV_Lookback(optInTimePeriod, optInNbDev));
      requireLength("STDDEV openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("STDDEV openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return stddevOpenAndFillInternal(inReal, 0, optInTimePeriod, optInNbDev, outBegIdx, outNBElement, outReal);
   }
