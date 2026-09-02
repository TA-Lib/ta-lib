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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("MINMAXINDEX", startIdx, MINMAXINDEX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("MINMAXINDEX", startIdx, MINMAXINDEX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#minmaxindexOpen}; there is no close — the handle is
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
   public static final class MinmaxindexStream {
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
      int outRangeBegIdx;
      int outRangeCount;

      MinmaxindexStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MINMAXINDEX} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MinmaxindexStream( MinmaxindexStream other ) {
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
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(MinmaxindexOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inReal, MinmaxindexOut out ) {
         requireArgument("MINMAXINDEX update", "out", out);
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("MINMAXINDEX update: BadParam", RetCode.BadParam);
         }
         core.minmaxindexStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.minIdx = this.cur_outMinIdx;
         out.maxIdx = this.cur_outMaxIdx;
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
      public void updateAndFill( double inReal[], int outMinIdx[], int outMaxIdx[] ) {
         requireArgument("MINMAXINDEX updateAndFill", "inReal", inReal);
         requireArgument("MINMAXINDEX updateAndFill", "outMinIdx", outMinIdx);
         requireArgument("MINMAXINDEX updateAndFill", "outMaxIdx", outMaxIdx);
         final int barCount = inReal.length;
         if( outMinIdx.length < barCount || outMaxIdx.length < barCount || (Object)outMinIdx == (Object)inReal || (Object)outMaxIdx == (Object)inReal || (Object)outMinIdx == (Object)outMaxIdx )
            throw new TaLibArgumentException("MINMAXINDEX updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("MINMAXINDEX updateAndFill: BadParam", RetCode.BadParam);
            }
            core.minmaxindexStepImpl(this, inReal[i]);
            outMinIdx[i] = this.cur_outMinIdx;
            outMaxIdx[i] = this.cur_outMaxIdx;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public void peek( double inReal, MinmaxindexOut out ) {
         requireArgument("MINMAXINDEX peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MINMAXINDEX peek: BadParam", RetCode.BadParam);
         MinmaxindexStream sp = this;
         double tmpHigh = 0.0;
         double tmpLow = 0.0;
         int cur_outMaxIdx = sp.cur_outMaxIdx;
         int cur_outMinIdx = sp.cur_outMinIdx;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int today = sp.today;
         int trailingIdx = sp.trailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( today >= 1073741824 ) {
            int rebaseShift = trailingIdx & ~sp.xMask;
            today -= rebaseShift;
            trailingIdx -= rebaseShift;
            highestIdx -= rebaseShift;
            i -= rebaseShift;
            lowestIdx -= rebaseShift;
         }
         pkSlot0 = today & sp.xMask;
         pkVal0 = inReal;
         tmpHigh = ((today & sp.xMask) != pkSlot0) ? sp.x_inReal[today & sp.xMask] : pkVal0;
         tmpLow = tmpHigh;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = ((i & sp.xMask) != pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
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
            lowest = ((lowestIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[lowestIdx & sp.xMask] : pkVal0;
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = ((i & sp.xMask) != pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         cur_outMaxIdx = highestIdx;
         cur_outMinIdx = lowestIdx;
         out.minIdx = cur_outMinIdx;
         out.maxIdx = cur_outMaxIdx;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( MinmaxindexOut out ) {
         requireArgument("MINMAXINDEX value", "out", out);
         out.minIdx = this.cur_outMinIdx;
         out.maxIdx = this.cur_outMaxIdx;
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
      public MinmaxindexStream clone() {
         return new MinmaxindexStream(this);
      }
   }

   /**
    * The outputs of one MINMAXINDEX bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class MinmaxindexOut {
      /** Absolute index (into inReal) of the window minimum. */
      public int minIdx;
      /** Absolute index (into inReal) of the window maximum. */
      public int maxIdx;
   }
   void minmaxindexStepImpl( MinmaxindexStream sp, double inReal )
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
   private RetCode minmaxindexOpenImpl( MinmaxindexStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outMinIdx[], int outMaxIdx[], int outStride )
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
      return RetCode.Success;
   }
   /* minmaxindexOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MinmaxindexStream minmaxindexOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outMinIdx[], int outMaxIdx[] )
   {
      MinmaxindexStream sp = new MinmaxindexStream(this);
      RetCode retCode = minmaxindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx, 1);
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
   /* Internal startIdx-anchored open behind minmaxindexOpen (composition seam). */
   MinmaxindexStream minmaxindexOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MinmaxindexStream sp = new MinmaxindexStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outMinIdx = new int[1];
      int[] sink_outMaxIdx = new int[1];
      RetCode retCode = minmaxindexOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outMinIdx, sink_outMaxIdx, 0);
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
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MinmaxindexStream minmaxindexOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("MINMAXINDEX open", "inReal", inReal);
      requireHistory("MINMAXINDEX open", inReal.length);
      return minmaxindexOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#minmaxindexOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MINMAXINDEX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MinmaxindexStream#outRange()}.
    */
   public MinmaxindexStream minmaxindexOpenAndFill( double inReal[], int optInTimePeriod, int outMinIdx[], int outMaxIdx[] )
   {
      requireArgument("MINMAXINDEX openAndFill", "inReal", inReal);
      requireHistory("MINMAXINDEX openAndFill", inReal.length);
      int guardOutLen = openFillCount("MINMAXINDEX openAndFill", inReal.length, MINMAXINDEX_Lookback(optInTimePeriod));
      requireLength("MINMAXINDEX openAndFill", "outMinIdx", outMinIdx, guardOutLen);
      requireLength("MINMAXINDEX openAndFill", "outMaxIdx", outMaxIdx, guardOutLen);
      if( (Object)outMinIdx == (Object)inReal || (Object)outMaxIdx == (Object)inReal || (Object)outMinIdx == (Object)outMaxIdx ) {
         throw new TaLibArgumentException("MINMAXINDEX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return minmaxindexOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outMinIdx, outMaxIdx);
   }
