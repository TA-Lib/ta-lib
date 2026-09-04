/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  020605 AA     Fix #1117666 Lookback bug.
 *  071126 MF,CC  Rewrite the combine into flat error-guards and a single-cursor
 *                offset index (offset = fastNb - *outNBElement). Bit-identical,
 *                streamable, and index-safe; the TA_IS_ZERO guard is unchanged.
 */

   /**
    * Number of leading input bars {@link Core#PPO} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PPO_Lookback( int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      /* Lookback is driven by the slowest MA. */
      return MA_Lookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) ;

   }
   RetCode PPO_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInFastPeriod,
                     int optInSlowPeriod,
                     MAType optInMAType,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything.
       *
       * Without this the fast MA below runs first, and its lookback is SMALLER
       * than ppo's own — so it reads the whole range and computes a result the
       * empty slow MA then discards. Observably identical (the slow MA's own early
       * return already yields 0,0 here), but it is the difference between "a range
       * shorter than the lookback reads nothing" being true of this function and
       * being false: with a caller-supplied inReal that stops short of endIdx, that
       * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
       * probe over every guarded core.
       */
      if( MA_Lookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      OutRange _xr0 = MA(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, tempBuffer);
      fastBeg.value = _xr0.begIdx();
      fastNb.value = _xr0.count();
      retCode = RetCode.Success;
      /* Calculate the slow MA into the output. */
      OutRange _xr1 = MA(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            outReal[i] = 0.0;
         }
      }
      return RetCode.Success ;
   }
   RetCode PPO_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInFastPeriod,
                     int optInSlowPeriod,
                     MAType optInMAType,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      if( MA_Lookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      OutRange _xr0 = MA(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, tempBuffer);
      fastBeg.value = _xr0.begIdx();
      fastNb.value = _xr0.count();
      retCode = RetCode.Success;
      OutRange _xr1 = MA(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      offset = fastNb.value - outNBElement.value;
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            outReal[i] = 0.0;
         }
      }
      return RetCode.Success ;
   }
   /**
    * Percentage Price Oscillator: the difference between a fast and slow moving
    * average expressed as a percentage of the slow MA. A normalized
    * (scale-invariant) variant of APO. Positive when the fast MA is above the
    * slow MA (upward momentum), negative otherwise; magnitude is the %
    * deviation.
    * <p><b>Formula</b>
    * <pre>{@code
    * PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0
    * The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>{@code optInMAType} applies to both the fast and slow moving average. {@code TA_MAType_MAMA} ignores its period argument, so with {@code optInMAType = TA_MAType_MAMA} the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PPO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outReal PPO value in percent. Must hold at least
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
    * @see Core#APO
    * @see Core#MACD
    * @see Core#MA
    */
   public OutRange PPO( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        MAType optInMAType,
                        double outReal[] )
   {
      requireIndexRange("PPO", startIdx, endIdx);
      requireArgument("PPO", "optInMAType", optInMAType);
      int guardStart = clampedStart("PPO", startIdx, PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PPO", "inReal", inReal, guardInLen);
      requireLength("PPO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PPO_Impl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Percentage Price Oscillator: the difference between a fast and slow moving
    * average expressed as a percentage of the slow MA. A normalized
    * (scale-invariant) variant of APO. Positive when the fast MA is above the
    * slow MA (upward momentum), negative otherwise; magnitude is the %
    * deviation.
    * <p><b>Formula</b>
    * <pre>{@code
    * PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0
    * The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>{@code optInMAType} applies to both the fast and slow moving average. {@code TA_MAType_MAMA} ignores its period argument, so with {@code optInMAType = TA_MAType_MAMA} the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PPO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outReal PPO value in percent. Must hold at least
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
    * @see Core#APO
    * @see Core#MACD
    * @see Core#MA
    */
   public OutRange PPO( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        MAType optInMAType,
                        double outReal[] )
   {
      requireIndexRange("PPO", startIdx, endIdx);
      requireArgument("PPO", "optInMAType", optInMAType);
      int guardStart = clampedStart("PPO", startIdx, PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PPO", "inReal", inReal, guardInLen);
      requireLength("PPO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PPO_Impl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PPO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PPO} over the same series.
    * Open with {@link Core#ppoOpen}; there is no close — the handle is
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
   public static final class PpoStream {
      Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      MAType optInMAType;
      double cur_outReal;
      MaStream sub0;
      MaStream sub1;
      int outRangeBegIdx;
      int outRangeCount;

      PpoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#PPO} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      PpoStream( PpoStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new MaStream(other.sub0);
         this.sub1 = new MaStream(other.sub1);
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
            throw new TaLibArgumentException("PPO update: BadParam", RetCode.BadParam);
         }
         core.ppoStepImpl(this, inReal);
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
         requireArgument("PPO updateAndFill", "inReal", inReal);
         requireArgument("PPO updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("PPO updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("PPO updateAndFill: BadParam", RetCode.BadParam);
            }
            core.ppoStepImpl(this, inReal[i]);
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
            throw new TaLibArgumentException("PPO peek: BadParam", RetCode.BadParam);
         PpoStream sp = this;
         double tempReal = 0.0;
         double cur_tempBuffer = 0.0;
         double cur_outReal = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempBuffer = sp.sub0.peek(inReal);
         cur_outReal = sp.sub1.peek(inReal);
         /* Combine map (batch tail, per bar). */
         tempReal = cur_outReal;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
         } else {
            cur_outReal = 0.0;
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
      public PpoStream clone() {
         return new PpoStream(this);
      }
   }
   void ppoStepImpl( PpoStream sp, double inReal )
   {
      double tempReal = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempBuffer = sp.sub0.update(inReal);
      cur_outReal = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      tempReal = cur_outReal;
      if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
         cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
      } else {
         cur_outReal = 0.0;
      }
      sp.cur_outReal = cur_outReal;
   }
   private RetCode ppoOpenImpl( PpoStream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      /* Nothing to produce: the range is shorter than the lookback. Return before
       * touching anything.
       *
       * Without this the fast MA below runs first, and its lookback is SMALLER
       * than ppo's own — so it reads the whole range and computes a result the
       * empty slow MA then discards. Observably identical (the slow MA's own early
       * return already yields 0,0 here), but it is the difference between "a range
       * shorter than the lookback reads nothing" being true of this function and
       * being false: with a caller-supplied inReal that stops short of endIdx, that
       * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
       * probe over every guarded core.
       */
      if( MA_Lookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(inReal, startIdx, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      retCode = RetCode.Success;
      /* Calculate the slow MA into the output. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub1 = maOpenAndFillInternal(inReal, startIdx, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.Success;
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         tempReal = sc_outReal[i];
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
         } else {
            sc_outReal[i] = 0.0;
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* ppoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PpoStream ppoOpenAndFillInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PpoStream sp = new PpoStream(this);
      RetCode retCode = ppoOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PPO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PPO openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("PPO openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind ppoOpen (composition seam). */
   PpoStream ppoOpenInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      PpoStream sp = new PpoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ppoOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PPO open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PPO open: internal error", retCode);
      }
      throw new TaLibArgumentException("PPO open: " + retCode, retCode);
   }
   /**
    * Open a live PPO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PPO} at that bar.
    * <p>The history must hold at least {@code PPO_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PpoStream ppoOpen( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      requireArgument("PPO open", "inReal", inReal);
      requireHistory("PPO open", inReal.length);
      requireArgument("PPO open", "optInMAType", optInMAType);
      return ppoOpenInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInMAType);
   }
   /**
    * {@link Core#ppoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#PPO} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PpoStream#outRange()}.
    */
   public PpoStream ppoOpenAndFill( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, double outReal[] )
   {
      requireArgument("PPO openAndFill", "inReal", inReal);
      requireHistory("PPO openAndFill", inReal.length);
      requireArgument("PPO openAndFill", "optInMAType", optInMAType);
      int guardOutLen = openFillCount("PPO openAndFill", inReal.length, PPO_Lookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      requireLength("PPO openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("PPO openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ppoOpenAndFillInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
   }
