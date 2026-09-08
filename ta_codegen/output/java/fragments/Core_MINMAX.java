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
 *  120906 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#MINMAX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Rolling window length (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MINMAX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MINMAX_Impl( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outMin[],
                        double outMax[] )
   {
      double[] sufHighest;
      int sufHighest_Idx = 0;
      int maxIdx_sufHighest = (30)-1;
      double[] preHighest;
      int preHighest_Idx = 0;
      int maxIdx_preHighest = (30)-1;
      double[] sufLowest;
      int sufLowest_Idx = 0;
      int maxIdx_sufLowest = (30)-1;
      double[] preLowest;
      int preLowest_Idx = 0;
      int maxIdx_preLowest = (30)-1;
      double highest = 0;
      double lowest = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int blockStart = 0;
      int nAvail = 0;
      int m = 0;
      int blockNext = 0;
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
      if( outMin == outMax ) {
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
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * Van Herk / Gil-Werman block scan, block-batched form. The p outputs
       * belonging to one block boundary are produced together: one backward
       * pass builds the older block's suffix extrema, one forward pass builds
       * the newer block's prefix extrema, and a third pass combines them.
       * Both extrema travel in the same passes.
       * All the loops are straight-line with no data-dependent branching,
       * which is what lets a compiler vectorize them, and the work per bar is
       * a fixed number of comparisons regardless of period. Every scratch
       * array holds COPIES, so input and output may alias.
       *
       * Producing a whole block at a time is also why this cannot be turned
       * into a per-bar automaton, so the streaming tier runs minmax_ALT1
       * below. See issue #147.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sufHighest = new double[optInTimePeriod];
      maxIdx_sufHighest = (optInTimePeriod)-1;
      sufHighest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      preHighest = new double[optInTimePeriod];
      maxIdx_preHighest = (optInTimePeriod)-1;
      preHighest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sufLowest = new double[optInTimePeriod];
      maxIdx_sufLowest = (optInTimePeriod)-1;
      sufLowest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      preLowest = new double[optInTimePeriod];
      maxIdx_preLowest = (optInTimePeriod)-1;
      preLowest_Idx = 0;
      blockStart = trailingIdx;
      while( today <= endIdx ) {
         /* Suffix extrema of the block [blockStart, blockStart+p-1], which
          * is fully available here: today == blockStart+p-1 <= endIdx.
          * Scanning backward while keeping the incumbent on a tie
          * leaves the later element holding a tie, which is what lets this
          * compile to a single min/max instruction.
          */
         i = blockStart + optInTimePeriod - 1;
         highest = inReal[i];
         lowest = highest;
         sufHighest[optInTimePeriod - 1] = highest;
         sufLowest[optInTimePeriod - 1] = lowest;
         while( i > blockStart ) {
            i -= 1;
            tmpHigh = inReal[i];
            if( tmpHigh > highest ) {
               highest = tmpHigh;
            }
            if( tmpHigh < lowest ) {
               lowest = tmpHigh;
            }
            sufHighest[i - blockStart] = highest;
            sufLowest[i - blockStart] = lowest;
         }
         outMax[outIdx] = sufHighest[0];
         outMin[outIdx] = sufLowest[0];
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
         if( today > endIdx ) {
            blockStart = blockStart + optInTimePeriod;
         } else {
            /* Prefix extrema of the next block, clamped to what remains.
             * Forward, keeping the incumbent on a tie: earliest wins again.
             */
            blockNext = blockStart + optInTimePeriod;
            nAvail = endIdx - blockNext + 1;
            if( nAvail > optInTimePeriod - 1 ) {
               nAvail = optInTimePeriod - 1;
            }
            highest = inReal[blockNext];
            lowest = highest;
            preHighest[0] = highest;
            preLowest[0] = lowest;
            i = 1;
            while( i < nAvail ) {
               tmpHigh = inReal[blockNext + i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
               if( tmpHigh < lowest ) {
                  lowest = tmpHigh;
               }
               preHighest[i] = highest;
               preLowest[i] = lowest;
               i += 1;
            }
            /* Combine. The suffix half is the older one, so preferring it
             * on a tie keeps the earliest-wins rule.
             */
            m = 1;
            while( m <= nAvail ) {
               highest = sufHighest[m];
               if( preHighest[m - 1] > highest ) {
                  highest = preHighest[m - 1];
               }
               lowest = sufLowest[m];
               if( preLowest[m - 1] < lowest ) {
                  lowest = preLowest[m - 1];
               }
               outMax[outIdx] = highest;
               outMin[outIdx] = lowest;
               outIdx += 1;
               m += 1;
            }
            trailingIdx = trailingIdx + nAvail;
            today = today + nAvail;
            blockStart = blockStart + optInTimePeriod;
         }
      }
      /* Keep the outBegIdx relative to the
       * caller input before returning.
       */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode MINMAX_Impl( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outMin[],
                        double outMax[] )
   {
      double[] sufHighest;
      int sufHighest_Idx = 0;
      int maxIdx_sufHighest = (30)-1;
      double[] preHighest;
      int preHighest_Idx = 0;
      int maxIdx_preHighest = (30)-1;
      double[] sufLowest;
      int sufLowest_Idx = 0;
      int maxIdx_sufLowest = (30)-1;
      double[] preLowest;
      int preLowest_Idx = 0;
      int maxIdx_preLowest = (30)-1;
      double highest = 0;
      double lowest = 0;
      double tmpHigh = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int blockStart = 0;
      int nAvail = 0;
      int m = 0;
      int blockNext = 0;
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
      if( outMin == outMax ) {
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
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sufHighest = new double[optInTimePeriod];
      maxIdx_sufHighest = (optInTimePeriod)-1;
      sufHighest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      preHighest = new double[optInTimePeriod];
      maxIdx_preHighest = (optInTimePeriod)-1;
      preHighest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sufLowest = new double[optInTimePeriod];
      maxIdx_sufLowest = (optInTimePeriod)-1;
      sufLowest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      preLowest = new double[optInTimePeriod];
      maxIdx_preLowest = (optInTimePeriod)-1;
      preLowest_Idx = 0;
      blockStart = trailingIdx;
      while( today <= endIdx ) {
         i = blockStart + optInTimePeriod - 1;
         highest = (double)inReal[i];
         lowest = highest;
         sufHighest[optInTimePeriod - 1] = highest;
         sufLowest[optInTimePeriod - 1] = lowest;
         while( i > blockStart ) {
            i -= 1;
            tmpHigh = (double)inReal[i];
            if( tmpHigh > highest ) {
               highest = tmpHigh;
            }
            if( tmpHigh < lowest ) {
               lowest = tmpHigh;
            }
            sufHighest[i - blockStart] = highest;
            sufLowest[i - blockStart] = lowest;
         }
         outMax[outIdx] = sufHighest[0];
         outMin[outIdx] = sufLowest[0];
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
         if( today > endIdx ) {
            blockStart = blockStart + optInTimePeriod;
         } else {
            blockNext = blockStart + optInTimePeriod;
            nAvail = endIdx - blockNext + 1;
            if( nAvail > optInTimePeriod - 1 ) {
               nAvail = optInTimePeriod - 1;
            }
            highest = (double)inReal[blockNext];
            lowest = highest;
            preHighest[0] = highest;
            preLowest[0] = lowest;
            i = 1;
            while( i < nAvail ) {
               tmpHigh = (double)inReal[blockNext + i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
               if( tmpHigh < lowest ) {
                  lowest = tmpHigh;
               }
               preHighest[i] = highest;
               preLowest[i] = lowest;
               i += 1;
            }
            m = 1;
            while( m <= nAvail ) {
               highest = sufHighest[m];
               if( preHighest[m - 1] > highest ) {
                  highest = preHighest[m - 1];
               }
               lowest = sufLowest[m];
               if( preLowest[m - 1] < lowest ) {
                  lowest = preLowest[m - 1];
               }
               outMax[outIdx] = highest;
               outMin[outIdx] = lowest;
               outIdx += 1;
               m += 1;
            }
            trailingIdx = trailingIdx + nAvail;
            today = today + nAvail;
            blockStart = blockStart + optInTimePeriod;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Returns both the lowest and highest values of the input over a rolling
    * window of the last optInTimePeriod bars. An overlap-study companion to MIN
    * and MAX that computes both extrema in one pass.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/minmax">ta-lib.org/functions/minmax</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MINMAX_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Values scanned for the window min and max.
    * @param optInTimePeriod Rolling window length (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outMin Lowest value in each rolling window. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMax Highest value in each rolling window. Must hold at least
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
    * @see Core#MIN
    * @see Core#MAX
    * @see Core#MINMAXINDEX
    * @see Core#MININDEX
    * @see Core#MAXINDEX
    */
   public OutRange MINMAX( int startIdx,
                           int endIdx,
                           double inReal[],
                           int optInTimePeriod,
                           double outMin[],
                           double outMax[] )
   {
      requireIndexRange("MINMAX", startIdx, endIdx);
      int guardStart = clampedStart("MINMAX", startIdx, MINMAX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MINMAX", "inReal", inReal, guardInLen);
      requireLength("MINMAX", "outMin", outMin, guardOutLen);
      requireLength("MINMAX", "outMax", outMax, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
      if( retCode != RetCode.Success ) {
         throw failure("MINMAX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Returns both the lowest and highest values of the input over a rolling
    * window of the last optInTimePeriod bars. An overlap-study companion to MIN
    * and MAX that computes both extrema in one pass.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/minmax">ta-lib.org/functions/minmax</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MINMAX_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Values scanned for the window min and max.
    * @param optInTimePeriod Rolling window length (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outMin Lowest value in each rolling window. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMax Highest value in each rolling window. Must hold at least
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
    * @see Core#MIN
    * @see Core#MAX
    * @see Core#MINMAXINDEX
    * @see Core#MININDEX
    * @see Core#MAXINDEX
    */
   public OutRange MINMAX( int startIdx,
                           int endIdx,
                           float inReal[],
                           int optInTimePeriod,
                           double outMin[],
                           double outMax[] )
   {
      requireIndexRange("MINMAX", startIdx, endIdx);
      int guardStart = clampedStart("MINMAX", startIdx, MINMAX_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MINMAX", "inReal", inReal, guardInLen);
      requireLength("MINMAX", "outMin", outMin, guardOutLen);
      requireLength("MINMAX", "outMax", outMax, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
      if( retCode != RetCode.Success ) {
         throw failure("MINMAX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using minmax_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live MINMAX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MINMAX} over the same series.
    * Open with {@link Core#minmaxOpen}; there is no close — the handle is
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
   public static final class MinmaxStream {
      private Core core;
      private int optInTimePeriod;
      private double highest;
      private double lowest;
      private int trailingIdx;
      private int highestIdx;
      private int lowestIdx;
      private int i;
      private int today;
      private int xMask;
      private double[] x_inReal;
      private double cur_outMin;
      private double cur_outMax;
      private int outRangeBegIdx;
      private int outRangeCount;

      private MinmaxStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MINMAX} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(MinmaxOut)} keeps answering the previous
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
            throw failure("MINMAX advance", RetCode.OutOfRangeEndIndex);
         this.outRangeCount++;
      }

      private MinmaxStream( MinmaxStream other ) {
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
         this.cur_outMin = other.cur_outMin;
         this.cur_outMax = other.cur_outMax;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(MinmaxOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inReal, MinmaxOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("MINMAX update", RetCode.OutOfRangeEndIndex);
         requireArgument("MINMAX update", "out", out);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MINMAX update: BadParam", RetCode.BadParam);
         core.minmaxStepImpl(this, inReal);
         this.outRangeCount++;
         out.min = this.cur_outMin;
         out.max = this.cur_outMax;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public void peek( double inReal, MinmaxOut out ) {
         requireArgument("MINMAX peek", "out", out);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MINMAX peek: BadParam", RetCode.BadParam);
         MinmaxStream sp = this;
         double tmpHigh = 0.0;
         double tmpLow = 0.0;
         double cur_outMax = 0.0;
         double cur_outMin = 0.0;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.today & sp.xMask;
         pkVal0 = inReal;
         tmpHigh = ((sp.today & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
         tmpLow = tmpHigh;
         if( highestIdx < sp.trailingIdx ) {
            highestIdx = sp.trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= sp.today ) {
               tmpHigh = ((i & sp.xMask) != pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = sp.today;
            highest = tmpHigh;
         }
         if( lowestIdx < sp.trailingIdx ) {
            lowestIdx = sp.trailingIdx;
            lowest = ((lowestIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[lowestIdx & sp.xMask] : pkVal0;
            i = lowestIdx;
            while( ++i <= sp.today ) {
               tmpLow = ((i & sp.xMask) != pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = sp.today;
            lowest = tmpLow;
         }
         cur_outMax = highest;
         cur_outMin = lowest;
         out.min = cur_outMin;
         out.max = cur_outMax;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( MinmaxOut out ) {
         requireArgument("MINMAX value", "out", out);
         out.min = this.cur_outMin;
         out.max = this.cur_outMax;
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
      public MinmaxStream clone() {
         return new MinmaxStream(this);
      }
   }

   /**
    * The outputs of one MINMAX bar, written by the stream into an object the
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
   public static final class MinmaxOut {
      /** Lowest value in each rolling window. */
      public double min;
      /** Highest value in each rolling window. */
      public double max;
   }
   private void minmaxStepImpl( MinmaxStream sp, double inReal )
   {
      double tmpHigh = 0.0;
      double tmpLow = 0.0;
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
      sp.cur_outMax = sp.highest;
      sp.cur_outMin = sp.lowest;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode minmaxOpenImpl( MinmaxStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outMin[], double outMax[], int outStride )
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
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * The highest and lowest values of the window are cached with their
       * indices; the window is rescanned only when a cached extremum drops out
       * of it. That is O(1)
       * per bar while the extremum sits away from the trailing edge, but it is
       * not amortized O(1): an extremum on the oldest in-window bar drops out
       * on the very next bar, so the rescan repeats and the cost stays
       * O(period) per bar for as long as that persists.
       *
       * Tracking both extrema keeps that state going through a trend: while
       * the high is refreshed by each new bar, the low stays pinned at the
       * oldest bar for the whole leg (and the reverse on the way down). A flat
       * stretch pins both. Random-walk input is the favourable case, where
       * rescans are rare.
       *
       * Slower than the block scan the batch tier runs; it is here because one
       * bar at a time is exactly what the streaming tier needs. See issue #147.
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
         outMax[outIdx * outStride] = highest;
         outMin[outIdx * outStride] = lowest;
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
      sp.cur_outMin = outMin[(outNBElement.value - 1) * outStride];
      sp.cur_outMax = outMax[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* minmaxOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MinmaxStream minmaxOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outMin[], double outMax[] )
   {
      MinmaxStream sp = new MinmaxStream(this);
      RetCode retCode = minmaxOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MINMAX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MINMAX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MINMAX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind minmaxOpen (composition seam). */
   MinmaxStream minmaxOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MinmaxStream sp = new MinmaxStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMin = new double[1];
      double[] sink_outMax = new double[1];
      RetCode retCode = minmaxOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outMin, sink_outMax, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MINMAX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MINMAX open: internal error", retCode);
      }
      throw new TaLibArgumentException("MINMAX open: " + retCode, retCode);
   }
   /**
    * Open a live MINMAX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MINMAX} at that bar.
    * <p>The history must hold at least {@code MINMAX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MinmaxStream minmaxOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("MINMAX open", "inReal", inReal);
      requireHistory("MINMAX open", inReal.length);
      return minmaxOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#minmaxOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MINMAX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MinmaxStream#outRange()}.
    */
   public MinmaxStream minmaxOpenAndFill( double inReal[], int optInTimePeriod, double outMin[], double outMax[] )
   {
      requireArgument("MINMAX openAndFill", "inReal", inReal);
      requireHistory("MINMAX openAndFill", inReal.length);
      int guardOutLen = openFillCount("MINMAX openAndFill", inReal.length, MINMAX_Lookback(optInTimePeriod));
      requireLength("MINMAX openAndFill", "outMin", outMin, guardOutLen);
      requireLength("MINMAX openAndFill", "outMax", outMax, guardOutLen);
      if( (Object)outMin == (Object)inReal || (Object)outMax == (Object)inReal || (Object)outMin == (Object)outMax ) {
         throw new TaLibArgumentException("MINMAX openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return minmaxOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
   }
