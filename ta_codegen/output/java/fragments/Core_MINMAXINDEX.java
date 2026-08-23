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
 *  120906 AC   Creation (equal to MINMAX but outputs index)
 */

   /**
    * Number of leading input bars {@link Core#MINMAXINDEX} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length in bars (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MINMAXINDEX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MINMAXINDEX_Impl( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outMinIdx[],
                             int outMaxIdx[] )
   {
      double highest = 0;
      double lowest = 0;
      double tmpHigh = 0;
      double tmpLow = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
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
      if( outMinIdx == outMaxIdx ) {
         return RetCode.BadParam ;
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
       * (The integer outputs can never share the real input's buffer —
       * different element type; issue #130.)
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = inReal[today];
         tmpLow = tmpHigh;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inReal[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = inReal[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = today;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inReal[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = inReal[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         outMaxIdx[outIdx] = highestIdx;
         outMinIdx[outIdx] = lowestIdx;
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
   RetCode MINMAXINDEX_Impl( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             MInteger outBegIdx,
                             MInteger outNBElement,
                             int outMinIdx[],
                             int outMaxIdx[] )
   {
      double highest = 0;
      double lowest = 0;
      double tmpHigh = 0;
      double tmpLow = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
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
      if( outMinIdx == outMaxIdx ) {
         return RetCode.BadParam ;
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
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = (double)inReal[today];
         tmpLow = tmpHigh;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inReal[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = (double)inReal[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = today;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inReal[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = (double)inReal[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         outMaxIdx[outIdx] = highestIdx;
         outMinIdx[outIdx] = lowestIdx;
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Returns the absolute input indices of the lowest and highest values within
    * each rolling window of optInTimePeriod bars. Index variant of MINMAX.
    * <p><b>Formula</b>
    * <pre>{@code
    * outMinIdx[i] = index of min(inReal[i-optInTimePeriod+1 .. i])
    * outMaxIdx[i] = index of max(inReal[i-optInTimePeriod+1 .. i])
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When several bars in a window share the extreme value, the index of one of them is returned — not necessarily the first or the last.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MINMAXINDEX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series scanned for extremes.
    * @param optInTimePeriod Window length in bars (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outMinIdx Absolute index (into inReal) of the window minimum. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
    * @param outMaxIdx Absolute index (into inReal) of the window maximum. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MINMAX
    * @see Core#MIN
    * @see Core#MAX
    * @see Core#MININDEX
    * @see Core#MAXINDEX
    */
   public OutRange MINMAXINDEX( int startIdx,
                                int endIdx,
                                double inReal[],
                                int optInTimePeriod,
                                int outMinIdx[],
                                int outMaxIdx[] )
   {
      requireIndexRange("MINMAXINDEX", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, MINMAXINDEX_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MINMAXINDEX", "inReal", inReal, guardInLen);
      requireLength("MINMAXINDEX", "outMinIdx", outMinIdx, guardOutLen);
      requireLength("MINMAXINDEX", "outMaxIdx", outMaxIdx, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAXINDEX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx);
      if( retCode != RetCode.Success ) {
         throw failure("MINMAXINDEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Returns the absolute input indices of the lowest and highest values within
    * each rolling window of optInTimePeriod bars. Index variant of MINMAX.
    * <p><b>Formula</b>
    * <pre>{@code
    * outMinIdx[i] = index of min(inReal[i-optInTimePeriod+1 .. i])
    * outMaxIdx[i] = index of max(inReal[i-optInTimePeriod+1 .. i])
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When several bars in a window share the extreme value, the index of one of them is returned — not necessarily the first or the last.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MINMAXINDEX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series scanned for extremes.
    * @param optInTimePeriod Window length in bars (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outMinIdx Absolute index (into inReal) of the window minimum. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
    * @param outMaxIdx Absolute index (into inReal) of the window maximum. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MINMAX
    * @see Core#MIN
    * @see Core#MAX
    * @see Core#MININDEX
    * @see Core#MAXINDEX
    */
   public OutRange MINMAXINDEX( int startIdx,
                                int endIdx,
                                float inReal[],
                                int optInTimePeriod,
                                int outMinIdx[],
                                int outMaxIdx[] )
   {
      requireIndexRange("MINMAXINDEX", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, MINMAXINDEX_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MINMAXINDEX", "inReal", inReal, guardInLen);
      requireLength("MINMAXINDEX", "outMinIdx", outMinIdx, guardOutLen);
      requireLength("MINMAXINDEX", "outMaxIdx", outMaxIdx, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAXINDEX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx);
      if( retCode != RetCode.Success ) {
         throw failure("MINMAXINDEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MINMAXINDEX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MINMAXINDEX} over the same series.
    * Open with {@link Core#MINMAXINDEX_Open}; there is no close — the handle is
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
   public static final class MINMAXINDEX_Stream {
      Core core;
      int optInTimePeriod;
      double highest;
      double lowest;
      int trailingIdx;
      int highestIdx;
      int lowestIdx;
      int i;
      int today;
      int xMask;
      double[] x_inReal;
      int cur_outMinIdx;
      int cur_outMaxIdx;
      Value cachedValue;
      int outRangeBegIdx;
      int outRangeCount;

      MINMAXINDEX_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MINMAXINDEX} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MINMAXINDEX_Stream( MINMAXINDEX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.trailingIdx = other.trailingIdx;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outMinIdx = other.cur_outMinIdx;
         this.cur_outMaxIdx = other.cur_outMaxIdx;
         this.cachedValue = other.cachedValue;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( MINMAXINDEX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.trailingIdx = other.trailingIdx;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inReal != null && this.x_inReal.length == other.x_inReal.length ) {
            System.arraycopy( other.x_inReal, 0, this.x_inReal, 0, other.x_inReal.length );
         } else {
            this.x_inReal = other.x_inReal.clone();
         }
         this.cur_outMinIdx = other.cur_outMinIdx;
         this.cur_outMaxIdx = other.cur_outMaxIdx;
         this.cachedValue = other.cachedValue;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param minIdx Absolute index (into inReal) of the window minimum.
       * @param maxIdx Absolute index (into inReal) of the window maximum.
       */
      public record Value(int minIdx, int maxIdx) { }

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
      public Value update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MINMAXINDEX update: BadParam", RetCode.BadParam);
         core.MINMAXINDEX_StepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         this.cachedValue = new Value(this.cur_outMinIdx, this.cur_outMaxIdx);
         return this.cachedValue;
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
      public void updateAndFill( double inReal[], int outMinIdx[], int outMaxIdx[] ) {
         final int barCount = inReal.length;
         if( outMinIdx.length < barCount || outMaxIdx.length < barCount || (Object)outMinIdx == (Object)inReal || (Object)outMaxIdx == (Object)inReal || (Object)outMinIdx == (Object)outMaxIdx )
            throw new TaLibArgumentException("MINMAXINDEX updateAndFill: BadParam", RetCode.BadParam);
         int done = 0;
         try {
            for( int i = 0; i < barCount; i++ ) {
               if( !Double.isFinite(inReal[i]) )
                  throw new TaLibArgumentException("MINMAXINDEX updateAndFill: BadParam", RetCode.BadParam);
               core.MINMAXINDEX_StepImpl(this, inReal[i]);
               outMinIdx[i] = this.cur_outMinIdx;
               outMaxIdx[i] = this.cur_outMaxIdx;
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               done = i + 1;
            }
         } finally {
            if( done > 0 ) this.cachedValue = new Value(this.cur_outMinIdx, this.cur_outMaxIdx);
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public Value peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MINMAXINDEX peek: BadParam", RetCode.BadParam);
         MINMAXINDEX_Stream scratch = new MINMAXINDEX_Stream(this);
         core.MINMAXINDEX_StepImpl(scratch, inReal);
         return new Value(scratch.cur_outMinIdx, scratch.cur_outMaxIdx);
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
      public MINMAXINDEX_Stream copy() {
         return new MINMAXINDEX_Stream(this);
      }
   }
   void MINMAXINDEX_StepImpl( MINMAXINDEX_Stream sp, double inReal )
   {
      double tmpHigh = 0.0;
      double tmpLow = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      tmpHigh = sp.x_inReal[sp.today & sp.xMask];
      tmpLow = tmpHigh;
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inReal[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmpHigh = sp.x_inReal[sp.i & sp.xMask];
            if( tmpHigh > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmpHigh;
            }
         }
      } else if( tmpHigh >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmpHigh;
      }
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inReal[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmpLow = sp.x_inReal[sp.i & sp.xMask];
            if( tmpLow < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmpLow;
            }
         }
      } else if( tmpLow <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmpLow;
      }
      sp.cur_outMaxIdx = sp.highestIdx;
      sp.cur_outMinIdx = sp.lowestIdx;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode MINMAXINDEX_OpenImpl( MINMAXINDEX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outMinIdx[], int outMaxIdx[], int outStride )
   {
      double highest = 0;
      double lowest = 0;
      double tmpHigh = 0;
      double tmpLow = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
       * (The integer outputs can never share the real input's buffer —
       * different element type; issue #130.)
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = inReal[today];
         tmpLow = tmpHigh;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inReal[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = inReal[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = today;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inReal[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = inReal[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         outMaxIdx[outIdx * outStride] = highestIdx;
         outMinIdx[outIdx * outStride] = lowestIdx;
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
      double[] capX_inReal = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ & (physX - 1)] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.highest = highest;
      sp.lowest = lowest;
      sp.trailingIdx = trailingIdx;
      sp.highestIdx = highestIdx;
      sp.lowestIdx = lowestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outMinIdx = outMinIdx[(outNBElement.value - 1) * outStride];
      sp.cur_outMaxIdx = outMaxIdx[(outNBElement.value - 1) * outStride];
      sp.cachedValue = new MINMAXINDEX_Stream.Value(sp.cur_outMinIdx, sp.cur_outMaxIdx);
      return RetCode.Success;
   }
   /* MINMAXINDEX_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MINMAXINDEX_Stream MINMAXINDEX_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outMinIdx[], int outMaxIdx[] )
   {
      MINMAXINDEX_Stream sp = new MINMAXINDEX_Stream(this);
      RetCode retCode = MINMAXINDEX_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MINMAXINDEX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MINMAXINDEX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MINMAXINDEX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind MINMAXINDEX_Open (composition seam). */
   MINMAXINDEX_Stream MINMAXINDEX_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MINMAXINDEX_Stream sp = new MINMAXINDEX_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outMinIdx = new int[1];
      int[] sink_outMaxIdx = new int[1];
      RetCode retCode = MINMAXINDEX_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outMinIdx, sink_outMaxIdx, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MINMAXINDEX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MINMAXINDEX open: internal error", retCode);
      }
      throw new TaLibArgumentException("MINMAXINDEX open: " + retCode, retCode);
   }
   /**
    * Open a live MINMAXINDEX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MINMAXINDEX} at that bar.
    * <p>The history must hold at least {@code MINMAXINDEX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MINMAXINDEX_Stream MINMAXINDEX_Open( double inReal[], int optInTimePeriod )
   {
      return MINMAXINDEX_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#MINMAXINDEX_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MINMAXINDEX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MINMAXINDEX_Stream#outRange()}.
    */
   public MINMAXINDEX_Stream MINMAXINDEX_OpenAndFill( double inReal[], int optInTimePeriod, int outMinIdx[], int outMaxIdx[] )
   {
      if( (Object)outMinIdx == (Object)inReal || (Object)outMaxIdx == (Object)inReal || (Object)outMinIdx == (Object)outMaxIdx ) {
         throw new TaLibArgumentException("MINMAXINDEX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return MINMAXINDEX_OpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx);
   }
