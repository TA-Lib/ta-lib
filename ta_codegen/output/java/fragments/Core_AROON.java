/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel <michel@pacbell.net>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  050703 MF   Fix algorithm base on Adrian Michel bug report #748163
 */

   /**
    * Number of leading input bars {@link Core#AROON} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback window length (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AROON_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode AROON_Impl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outAroonDown[],
                       double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outAroonDown == outAroonUp ) {
         return RetCode.BadParam ;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode AROON_Impl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outAroonDown[],
                       double outAroonUp[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outAroonDown == outAroonUp ) {
         return RetCode.BadParam ;
      }
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outAroonUp[outIdx] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Aroon reports how recently the highest high and lowest low occurred within
    * a rolling window of length optInTimePeriod, as two 0-100 oscillators.
    * Indicates trend strength and direction. Up near 100 = a very recent new
    * high (strong uptrend); Down near 100 = a very recent new low. Up/Down
    * crossovers signal trend shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AROON_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Lookback window length (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outAroonDown Recency of the lowest low (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @param outAroonUp Recency of the highest high (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#AROONOSC
    * @see Core#MINMAXINDEX
    * @see Core#MIN
    * @see Core#MAX
    */
   public OutRange AROON( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          double outAroonDown[],
                          double outAroonUp[] )
   {
      requireIndexRange("AROON", startIdx, endIdx);
      int guardStart = clampedStart("AROON", startIdx, AROON_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AROON", "inHigh", inHigh, guardInLen);
      requireLength("AROON", "inLow", inLow, guardInLen);
      requireLength("AROON", "outAroonDown", outAroonDown, guardOutLen);
      requireLength("AROON", "outAroonUp", outAroonUp, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AROON_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
      if( retCode != RetCode.Success ) {
         throw failure("AROON", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Aroon reports how recently the highest high and lowest low occurred within
    * a rolling window of length optInTimePeriod, as two 0-100 oscillators.
    * Indicates trend strength and direction. Up near 100 = a very recent new
    * high (strong uptrend); Down near 100 = a very recent new low. Up/Down
    * crossovers signal trend shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AROON_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Lookback window length (default 14; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outAroonDown Recency of the lowest low (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @param outAroonUp Recency of the highest high (100 = it is the current
    *        bar, decaying as it ages) Must hold at least {@code endIdx - startIdx + 1}
    *        values.
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
    * @see Core#AROONOSC
    * @see Core#MINMAXINDEX
    * @see Core#MIN
    * @see Core#MAX
    */
   public OutRange AROON( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInTimePeriod,
                          double outAroonDown[],
                          double outAroonUp[] )
   {
      requireIndexRange("AROON", startIdx, endIdx);
      int guardStart = clampedStart("AROON", startIdx, AROON_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AROON", "inHigh", inHigh, guardInLen);
      requireLength("AROON", "inLow", inLow, guardInLen);
      requireLength("AROON", "outAroonDown", outAroonDown, guardOutLen);
      requireLength("AROON", "outAroonUp", outAroonUp, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AROON_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
      if( retCode != RetCode.Success ) {
         throw failure("AROON", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AROON stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AROON} over the same series.
    * Open with {@link Core#AROON_Open}; there is no close — the handle is
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
   public static final class AROON_Stream {
      Core core;
      int optInTimePeriod;
      double lowest;
      double highest;
      double factor;
      int trailingIdx;
      int lowestIdx;
      int highestIdx;
      int i;
      int today;
      int xMask;
      double[] x_inHigh;
      double[] x_inLow;
      double cur_outAroonDown;
      double cur_outAroonUp;
      Value cachedValue;
      int outRangeBegIdx;
      int outRangeCount;

      AROON_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AROON} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AROON_Stream( AROON_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.factor = other.factor;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.cur_outAroonDown = other.cur_outAroonDown;
         this.cur_outAroonUp = other.cur_outAroonUp;
         this.cachedValue = other.cachedValue;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AROON_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.factor = other.factor;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inHigh != null && this.x_inHigh.length == other.x_inHigh.length ) {
            System.arraycopy( other.x_inHigh, 0, this.x_inHigh, 0, other.x_inHigh.length );
         } else {
            this.x_inHigh = other.x_inHigh.clone();
         }
         if( this.x_inLow != null && this.x_inLow.length == other.x_inLow.length ) {
            System.arraycopy( other.x_inLow, 0, this.x_inLow, 0, other.x_inLow.length );
         } else {
            this.x_inLow = other.x_inLow.clone();
         }
         this.cur_outAroonDown = other.cur_outAroonDown;
         this.cur_outAroonUp = other.cur_outAroonUp;
         this.cachedValue = other.cachedValue;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<AROON_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param aroonDown Recency of the lowest low (100 = it is the current bar, decaying as it ages)
       * @param aroonUp Recency of the highest high (100 = it is the current bar, decaying as it ages)
       */
      public record Value(double aroonDown, double aroonUp) { }

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
      public Value update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AROON update: BadParam", RetCode.BadParam);
         core.AROON_StepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         this.cachedValue = new Value(this.cur_outAroonDown, this.cur_outAroonUp);
         return this.cachedValue;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double outAroonDown[], double outAroonUp[] ) {
         requireArgument("AROON updateAndFill", "inHigh", inHigh);
         requireArgument("AROON updateAndFill", "inLow", inLow);
         requireArgument("AROON updateAndFill", "outAroonDown", outAroonDown);
         requireArgument("AROON updateAndFill", "outAroonUp", outAroonUp);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outAroonDown.length < barCount || outAroonUp.length < barCount || (Object)outAroonDown == (Object)inHigh || (Object)outAroonDown == (Object)inLow || (Object)outAroonUp == (Object)inHigh || (Object)outAroonUp == (Object)inLow || (Object)outAroonDown == (Object)outAroonUp )
            throw new TaLibArgumentException("AROON updateAndFill: BadParam", RetCode.BadParam);
         int done = 0;
         try {
            for( int i = 0; i < barCount; i++ ) {
               if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
                  throw new TaLibArgumentException("AROON updateAndFill: BadParam", RetCode.BadParam);
               core.AROON_StepImpl(this, inHigh[i], inLow[i]);
               outAroonDown[i] = this.cur_outAroonDown;
               outAroonUp[i] = this.cur_outAroonUp;
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               done = i + 1;
            }
         } finally {
            if( done > 0 ) this.cachedValue = new Value(this.cur_outAroonDown, this.cur_outAroonUp);
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public Value peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AROON peek: BadParam", RetCode.BadParam);
         AROON_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new AROON_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.AROON_StepImpl(scratch, inHigh, inLow);
         return new Value(scratch.cur_outAroonDown, scratch.cur_outAroonUp);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public AROON_Stream copy() {
         return new AROON_Stream(this);
      }
   }
   void AROON_StepImpl( AROON_Stream sp, double inHigh, double inLow )
   {
      double tmp = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inHigh[sp.today & sp.xMask] = inHigh;
      sp.x_inLow[sp.today & sp.xMask] = inLow;
      /* Keep track of the lowestIdx */
      tmp = sp.x_inLow[sp.today & sp.xMask];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i & sp.xMask];
            if( tmp <= sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
      }
      /* Keep track of the highestIdx */
      tmp = sp.x_inHigh[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i & sp.xMask];
            if( tmp >= sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      /* Note: Do not forget that input and output buffer can be the same,
       *       so writing to the output is the last thing being done here.
       */
      sp.cur_outAroonUp = sp.factor * (sp.optInTimePeriod - (sp.today - sp.highestIdx));
      sp.cur_outAroonDown = sp.factor * (sp.optInTimePeriod - (sp.today - sp.lowestIdx));
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode AROON_OpenImpl( AROON_Stream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outAroonDown[], double outAroonUp[], int outStride )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
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
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* This function is using a speed optimized algorithm
       * for the min/max logic.
       *
       * You might want to first look at how TA_MIN/TA_MAX works
       * and this function will become easier to understand.
       */
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < optInTimePeriod ) {
         startIdx = optInTimePeriod;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - optInTimePeriod;
      lowestIdx = 0 - 1;
      highestIdx = 0 - 1;
      lowest = 0.0;
      highest = 0.0;
      factor = (double)100.0 / (double)optInTimePeriod;
      while( today <= endIdx ) {
         /* Keep track of the lowestIdx */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp <= lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Keep track of the highestIdx */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp >= highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outAroonUp[outIdx * outStride] = factor * (optInTimePeriod - (today - highestIdx));
         outAroonDown[outIdx * outStride] = factor * (optInTimePeriod - (today - lowestIdx));
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inHigh = new double[physX];
      double[] capX_inLow = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
         capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lowest = lowest;
      sp.highest = highest;
      sp.factor = factor;
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outAroonDown = outAroonDown[(outNBElement.value - 1) * outStride];
      sp.cur_outAroonUp = outAroonUp[(outNBElement.value - 1) * outStride];
      sp.cachedValue = new AROON_Stream.Value(sp.cur_outAroonDown, sp.cur_outAroonUp);
      return RetCode.Success;
   }
   /* AROON_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AROON_Stream AROON_OpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outAroonDown[], double outAroonUp[] )
   {
      AROON_Stream sp = new AROON_Stream(this);
      RetCode retCode = AROON_OpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AROON openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AROON openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AROON openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind AROON_Open (composition seam). */
   AROON_Stream AROON_OpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      AROON_Stream sp = new AROON_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outAroonDown = new double[1];
      double[] sink_outAroonUp = new double[1];
      RetCode retCode = AROON_OpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outAroonDown, sink_outAroonUp, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AROON open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AROON open: internal error", retCode);
      }
      throw new TaLibArgumentException("AROON open: " + retCode, retCode);
   }
   /**
    * Open a live AROON stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AROON} at that bar.
    * <p>The history must hold at least {@code AROON_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AROON_Stream AROON_Open( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("AROON open", "inHigh", inHigh);
      requireHistory("AROON open", inHigh.length);
      requireArgument("AROON open", "inLow", inLow);
      requireHistoryLength("AROON open", "inLow", inLow.length, inHigh.length);
      return AROON_OpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#AROON_Open} that also fills the output array(s) bit-identically
    * to {@link Core#AROON} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AROON_Stream#outRange()}.
    */
   public AROON_Stream AROON_OpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outAroonDown[], double outAroonUp[] )
   {
      requireArgument("AROON openAndFill", "inHigh", inHigh);
      requireHistory("AROON openAndFill", inHigh.length);
      requireArgument("AROON openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("AROON openAndFill", inHigh.length, AROON_Lookback(optInTimePeriod));
      requireHistoryLength("AROON openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("AROON openAndFill", "outAroonDown", outAroonDown, guardOutLen);
      requireLength("AROON openAndFill", "outAroonUp", outAroonUp, guardOutLen);
      if( (Object)outAroonDown == (Object)inHigh || (Object)outAroonDown == (Object)inLow || (Object)outAroonUp == (Object)inHigh || (Object)outAroonUp == (Object)inLow || (Object)outAroonDown == (Object)outAroonUp ) {
         throw new TaLibArgumentException("AROON openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return AROON_OpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outAroonDown, outAroonUp);
   }
