/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  AC       Angelo Ciceri
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  120806 AC   Creation (equal to MIN but outputs index)
 */

   /**
    * Number of leading input bars {@link Core#MININDEX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length over which the minimum is located
    *        (default 30; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MININDEX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MININDEX_Impl( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          int outInteger[] )
   {
      double lowest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int today = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Proceed with the calculation for the requested range.
       * (The integer output can never share the real input's buffer —
       * different element type; issue #130.)
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmp = inReal[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inReal[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inReal[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         outInteger[outIdx++] = lowestIdx;
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
   RetCode MININDEX_Impl( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          int outInteger[] )
   {
      double lowest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int today = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      nbInitialElementNeeded = optInTimePeriod - 1;
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmp = (double)inReal[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inReal[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inReal[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         outInteger[outIdx++] = lowestIdx;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Returns the absolute index of the lowest value within a rolling window of
    * the given period. Same scan as MIN but outputs the position of the minimum
    * rather than its value.
    * <p><b>Formula</b>
    * <pre>{@code
    * outInteger[i] = index of min(inReal[i-optInTimePeriod+1 .. i])
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When several bars in a window share the lowest value, the index of one of them is returned — not necessarily the first or the last.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MININDEX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to scan for its minimum.
    * @param optInTimePeriod Window length over which the minimum is located
    *        (default 30; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outInteger Absolute index in inReal of the lowest value in each
    *        window. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MIN
    * @see Core#MAXINDEX
    * @see Core#MINMAXINDEX
    * @see Core#MINMAX
    */
   public OutRange MININDEX( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             int outInteger[] )
   {
      requireIndexRange("MININDEX", startIdx, endIdx);
      int guardStart = clampedStart("MININDEX", startIdx, MININDEX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MININDEX", "inReal", inReal, guardInLen);
      requireLength("MININDEX", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MININDEX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("MININDEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Returns the absolute index of the lowest value within a rolling window of
    * the given period. Same scan as MIN but outputs the position of the minimum
    * rather than its value.
    * <p><b>Formula</b>
    * <pre>{@code
    * outInteger[i] = index of min(inReal[i-optInTimePeriod+1 .. i])
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When several bars in a window share the lowest value, the index of one of them is returned — not necessarily the first or the last.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MININDEX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to scan for its minimum.
    * @param optInTimePeriod Window length over which the minimum is located
    *        (default 30; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outInteger Absolute index in inReal of the lowest value in each
    *        window. Must hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MIN
    * @see Core#MAXINDEX
    * @see Core#MINMAXINDEX
    * @see Core#MINMAX
    */
   public OutRange MININDEX( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             int outInteger[] )
   {
      requireIndexRange("MININDEX", startIdx, endIdx);
      int guardStart = clampedStart("MININDEX", startIdx, MININDEX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MININDEX", "inReal", inReal, guardInLen);
      requireLength("MININDEX", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MININDEX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("MININDEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MININDEX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MININDEX} over the same series.
    * Open with {@link Core#minindexOpen}; there is no close — the handle is
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
   public static final class MinindexStream {
      Core core;
      int optInTimePeriod;
      double lowest;
      int trailingIdx;
      int lowestIdx;
      int i;
      int today;
      int xMask;
      double[] x_inReal;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      MinindexStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MININDEX} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MinindexStream( MinindexStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( MinindexStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inReal != null && this.x_inReal.length == other.x_inReal.length ) {
            System.arraycopy( other.x_inReal, 0, this.x_inReal, 0, other.x_inReal.length );
         } else {
            this.x_inReal = other.x_inReal.clone();
         }
         this.cur_outInteger = other.cur_outInteger;
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
      public int update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MININDEX update: BadParam", RetCode.BadParam);
         core.minindexStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outInteger;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inReal[], int outInteger[] ) {
         requireArgument("MININDEX updateAndFill", "inReal", inReal);
         requireArgument("MININDEX updateAndFill", "outInteger", outInteger);
         final int barCount = inReal.length;
         if( outInteger.length < barCount || (Object)outInteger == (Object)inReal )
            throw new TaLibArgumentException("MININDEX updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) )
               throw new TaLibArgumentException("MININDEX updateAndFill: BadParam", RetCode.BadParam);
            core.minindexStepImpl(this, inReal[i]);
            outInteger[i] = this.cur_outInteger;
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
      public int peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MININDEX peek: BadParam", RetCode.BadParam);
         MinindexStream scratch = new MinindexStream(this);
         core.minindexStepImpl(scratch, inReal);
         return scratch.cur_outInteger;
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public int value() {
         return this.cur_outInteger;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MinindexStream copy() {
         return new MinindexStream(this);
      }
   }
   void minindexStepImpl( MinindexStream sp, double inReal )
   {
      double tmp = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      tmp = sp.x_inReal[sp.today & sp.xMask];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inReal[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inReal[sp.i & sp.xMask];
            if( tmp < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
      }
      sp.cur_outInteger = sp.lowestIdx;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode minindexOpenImpl( MinindexStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outInteger[], int outStride )
   {
      double lowest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int lowestIdx = 0;
      int today = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Proceed with the calculation for the requested range.
       * (The integer output can never share the real input's buffer —
       * different element type; issue #130.)
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmp = inReal[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inReal[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inReal[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         outInteger[outIdx++ * outStride] = lowestIdx;
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
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lowest = lowest;
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* minindexOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MinindexStream minindexOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
   {
      MinindexStream sp = new MinindexStream(this);
      RetCode retCode = minindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MININDEX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MININDEX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MININDEX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind minindexOpen (composition seam). */
   MinindexStream minindexOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MinindexStream sp = new MinindexStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outInteger = new int[1];
      RetCode retCode = minindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MININDEX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MININDEX open: internal error", retCode);
      }
      throw new TaLibArgumentException("MININDEX open: " + retCode, retCode);
   }
   /**
    * Open a live MININDEX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MININDEX} at that bar.
    * <p>The history must hold at least {@code MININDEX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MinindexStream minindexOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("MININDEX open", "inReal", inReal);
      requireHistory("MININDEX open", inReal.length);
      return minindexOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#minindexOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MININDEX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MinindexStream#outRange()}.
    */
   public MinindexStream minindexOpenAndFill( double inReal[], int optInTimePeriod, int outInteger[] )
   {
      requireArgument("MININDEX openAndFill", "inReal", inReal);
      requireHistory("MININDEX openAndFill", inReal.length);
      int guardOutLen = openFillCount("MININDEX openAndFill", inReal.length, MININDEX_Lookback(optInTimePeriod));
      requireLength("MININDEX openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outInteger == (Object)inReal ) {
         throw new TaLibArgumentException("MININDEX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return minindexOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outInteger);
   }
