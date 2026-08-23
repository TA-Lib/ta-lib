/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AM       Adrian Michel <michel@pacbell.net>
 *
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
    * Number of leading input bars {@link Core#AROONOSC} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback window for locating the highest high and
    *        lowest low (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int AROONOSC_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod ;

   }
   RetCode AROONOSC_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      double aroon = 0;
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
      /* This code is almost identical to the TA_AROON function
       * except that instead of outputing ArroonUp and AroonDown
       * individually, an oscillator is build from both.
       *
       *  AroonOsc = AroonUp- AroonDown;
       */
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
         /* The oscillator is the following:
          *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
          *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
          *  AroonOsc  = AroonUp-AroonDown;
          *
          * An arithmetic simplification give us:
          *  Aroon = factor*(highestIdx-lowestIdx)
          */
         aroon = factor * (highestIdx - lowestIdx);
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outReal[outIdx] = aroon;
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
   RetCode AROONOSC_Impl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outReal[] )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      double aroon = 0;
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
         aroon = factor * (highestIdx - lowestIdx);
         outReal[outIdx] = aroon;
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Aroon Oscillator: AroonUp minus AroonDown over a lookback window. Measures
    * trend direction and strength on a -100..+100 scale. Positive when the high
    * is more recent than the low (up-trend); negative when the low is more
    * recent (down-trend).
    * <p><b>Formula</b>
    * <pre>{@code
    * factor = 100 / optInTimePeriod
    * AroonUp   = factor * (period - (today - highestIdx))
    * AroonDown = factor * (period - (today - lowestIdx))
    * AroonOsc  = AroonUp - AroonDown = factor * (highestIdx - lowestIdx)
    * highestIdx/lowestIdx = bar index of the highest high / lowest low in the last (period+1) bars.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AROONOSC_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Lookback window for locating the highest high and
    *        lowest low (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outReal Aroon oscillator value (AroonUp - AroonDown) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AROON
    * @see Core#MINMAX
    */
   public OutRange AROONOSC( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("AROONOSC", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, AROONOSC_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AROONOSC", "inHigh", inHigh, guardInLen);
      requireLength("AROONOSC", "inLow", inLow, guardInLen);
      requireLength("AROONOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AROONOSC_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AROONOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Aroon Oscillator: AroonUp minus AroonDown over a lookback window. Measures
    * trend direction and strength on a -100..+100 scale. Positive when the high
    * is more recent than the low (up-trend); negative when the low is more
    * recent (down-trend).
    * <p><b>Formula</b>
    * <pre>{@code
    * factor = 100 / optInTimePeriod
    * AroonUp   = factor * (period - (today - highestIdx))
    * AroonDown = factor * (period - (today - lowestIdx))
    * AroonOsc  = AroonUp - AroonDown = factor * (highestIdx - lowestIdx)
    * highestIdx/lowestIdx = bar index of the highest high / lowest low in the last (period+1) bars.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#AROONOSC_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Lookback window for locating the highest high and
    *        lowest low (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outReal Aroon oscillator value (AroonUp - AroonDown) Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#AROON
    * @see Core#MINMAX
    */
   public OutRange AROONOSC( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("AROONOSC", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, AROONOSC_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("AROONOSC", "inHigh", inHigh, guardInLen);
      requireLength("AROONOSC", "inLow", inLow, guardInLen);
      requireLength("AROONOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = AROONOSC_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("AROONOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live AROONOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#AROONOSC} over the same series.
    * Open with {@link Core#AROONOSC_Open}; there is no close — the handle is
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
   public static final class AROONOSC_Stream {
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
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      AROONOSC_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#AROONOSC} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      AROONOSC_Stream( AROONOSC_Stream other ) {
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
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( AROONOSC_Stream other ) {
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
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<AROONOSC_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AROONOSC update: BadParam", RetCode.BadParam);
         core.AROONOSC_StepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
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
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("AROONOSC updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
               throw new TaLibArgumentException("AROONOSC updateAndFill: BadParam", RetCode.BadParam);
            core.AROONOSC_StepImpl(this, inHigh[i], inLow[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
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
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("AROONOSC peek: BadParam", RetCode.BadParam);
         AROONOSC_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new AROONOSC_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.AROONOSC_StepImpl(scratch, inHigh, inLow);
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
      public AROONOSC_Stream copy() {
         return new AROONOSC_Stream(this);
      }
   }
   void AROONOSC_StepImpl( AROONOSC_Stream sp, double inHigh, double inLow )
   {
      double tmp = 0.0;
      double aroon = 0.0;
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
      /* The oscillator is the following:
       *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
       *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
       *  AroonOsc  = AroonUp-AroonDown;
       *
       * An arithmetic simplification give us:
       *  Aroon = factor*(highestIdx-lowestIdx)
       */
      aroon = sp.factor * (sp.highestIdx - sp.lowestIdx);
      /* Note: Do not forget that input and output buffer can be the same,
       *       so writing to the output is the last thing being done here.
       */
      sp.cur_outReal = aroon;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode AROONOSC_OpenImpl( AROONOSC_Stream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double factor = 0;
      double aroon = 0;
      int outIdx = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int today = 0;
      int i = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
      /* This code is almost identical to the TA_AROON function
       * except that instead of outputing ArroonUp and AroonDown
       * individually, an oscillator is build from both.
       *
       *  AroonOsc = AroonUp- AroonDown;
       */
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
         /* The oscillator is the following:
          *  AroonUp   = factor*(optInTimePeriod-(today-highestIdx));
          *  AroonDown = factor*(optInTimePeriod-(today-lowestIdx));
          *  AroonOsc  = AroonUp-AroonDown;
          *
          * An arithmetic simplification give us:
          *  Aroon = factor*(highestIdx-lowestIdx)
          */
         aroon = factor * (highestIdx - lowestIdx);
         /* Note: Do not forget that input and output buffer can be the same,
          *       so writing to the output is the last thing being done here.
          */
         outReal[outIdx * outStride] = aroon;
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
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* AROONOSC_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AROONOSC_Stream AROONOSC_OpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AROONOSC_Stream sp = new AROONOSC_Stream(this);
      RetCode retCode = AROONOSC_OpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AROONOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AROONOSC openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("AROONOSC openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind AROONOSC_Open (composition seam). */
   AROONOSC_Stream AROONOSC_OpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      AROONOSC_Stream sp = new AROONOSC_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = AROONOSC_OpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("AROONOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("AROONOSC open: internal error", retCode);
      }
      throw new TaLibArgumentException("AROONOSC open: " + retCode, retCode);
   }
   /**
    * Open a live AROONOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#AROONOSC} at that bar.
    * <p>The history must hold at least {@code AROONOSC_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public AROONOSC_Stream AROONOSC_Open( double inHigh[], double inLow[], int optInTimePeriod )
   {
      return AROONOSC_OpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#AROONOSC_Open} that also fills the output array(s) bit-identically
    * to {@link Core#AROONOSC} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link AROONOSC_Stream#outRange()}.
    */
   public AROONOSC_Stream AROONOSC_OpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("AROONOSC openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return AROONOSC_OpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
