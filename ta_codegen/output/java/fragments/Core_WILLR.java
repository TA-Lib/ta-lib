/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  010802 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#WILLR} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Lookback bars for the high/low range (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int WILLR_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode WILLR_Impl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
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
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
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
         optInTimePeriod = 14;
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
      /* Initialize 'diff', just to avoid warning. */
      diff = 0.0;
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
       * into a per-bar automaton, so the streaming tier runs willr_ALT1
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
         highest = inHigh[i];
         lowest = inLow[i];
         sufHighest[optInTimePeriod - 1] = highest;
         sufLowest[optInTimePeriod - 1] = lowest;
         while( i > blockStart ) {
            i -= 1;
            tmp = inHigh[i];
            if( tmp > highest ) {
               highest = tmp;
            }
            tmp = inLow[i];
            if( tmp < lowest ) {
               lowest = tmp;
            }
            sufHighest[i - blockStart] = highest;
            sufLowest[i - blockStart] = lowest;
         }
         highest = sufHighest[0];
         lowest = sufLowest[0];
         diff = (highest - lowest) / (0 - 100.0);
         if( diff != 0.0 ) {
            outReal[outIdx++] = (highest - inClose[today]) / diff;
         } else {
            outReal[outIdx++] = 0.0;
         }
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
            highest = inHigh[blockNext];
            lowest = inLow[blockNext];
            preHighest[0] = highest;
            preLowest[0] = lowest;
            i = 1;
            while( i < nAvail ) {
               tmp = inHigh[blockNext + i];
               if( tmp > highest ) {
                  highest = tmp;
               }
               tmp = inLow[blockNext + i];
               if( tmp < lowest ) {
                  lowest = tmp;
               }
               preHighest[i] = highest;
               preLowest[i] = lowest;
               i += 1;
            }
            /* Combine and emit. The suffix half is the older one, so
             * preferring it on a tie keeps the earliest-wins rule. The
             * bar being emitted for offset m is today+m-1: 'today' was
             * advanced once above and is not touched inside this loop.
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
               diff = (highest - lowest) / (0 - 100.0);
               if( diff != 0.0 ) {
                  outReal[outIdx++] = (highest - inClose[today + m - 1]) / diff;
               } else {
                  outReal[outIdx++] = 0.0;
               }
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
   RetCode WILLR_Impl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
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
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
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
         optInTimePeriod = 14;
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
      diff = 0.0;
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
         highest = (double)inHigh[i];
         lowest = (double)inLow[i];
         sufHighest[optInTimePeriod - 1] = highest;
         sufLowest[optInTimePeriod - 1] = lowest;
         while( i > blockStart ) {
            i -= 1;
            tmp = (double)inHigh[i];
            if( tmp > highest ) {
               highest = tmp;
            }
            tmp = (double)inLow[i];
            if( tmp < lowest ) {
               lowest = tmp;
            }
            sufHighest[i - blockStart] = highest;
            sufLowest[i - blockStart] = lowest;
         }
         highest = sufHighest[0];
         lowest = sufLowest[0];
         diff = (highest - lowest) / (0 - 100.0);
         if( diff != 0.0 ) {
            outReal[outIdx++] = (highest - (double)inClose[today]) / diff;
         } else {
            outReal[outIdx++] = 0.0;
         }
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
            highest = (double)inHigh[blockNext];
            lowest = (double)inLow[blockNext];
            preHighest[0] = highest;
            preLowest[0] = lowest;
            i = 1;
            while( i < nAvail ) {
               tmp = (double)inHigh[blockNext + i];
               if( tmp > highest ) {
                  highest = tmp;
               }
               tmp = (double)inLow[blockNext + i];
               if( tmp < lowest ) {
                  lowest = tmp;
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
               diff = (highest - lowest) / (0 - 100.0);
               if( diff != 0.0 ) {
                  outReal[outIdx++] = (highest - (double)inClose[today + m - 1]) / diff;
               } else {
                  outReal[outIdx++] = 0.0;
               }
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
    * Williams' %R momentum oscillator over a rolling period, bounded in [-100,
    * 0]. Measures where the current close sits relative to the high-low range
    * of the last N bars. Near 0 = close at period high (overbought); near -100
    * = close at period low (oversold).
    * <p><b>Formula</b>
    * <pre>{@code
    * %R = -100 * (highestHigh - close) / (highestHigh - lowestLow) over the trailing optInTimePeriod bars; if highestHigh == lowestLow, output 0.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WILLR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Lookback bars for the high/low range (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Williams' %R value in [-100, 0]. Must hold at least
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
    * @see Core#STOCH
    * @see Core#STOCHF
    * @see Core#MINMAX
    */
   public OutRange WILLR( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("WILLR", startIdx, endIdx);
      int guardStart = clampedStart("WILLR", startIdx, WILLR_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WILLR", "inHigh", inHigh, guardInLen);
      requireLength("WILLR", "inLow", inLow, guardInLen);
      requireLength("WILLR", "inClose", inClose, guardInLen);
      requireLength("WILLR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WILLR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WILLR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Williams' %R momentum oscillator over a rolling period, bounded in [-100,
    * 0]. Measures where the current close sits relative to the high-low range
    * of the last N bars. Near 0 = close at period high (overbought); near -100
    * = close at period low (oversold).
    * <p><b>Formula</b>
    * <pre>{@code
    * %R = -100 * (highestHigh - close) / (highestHigh - lowestLow) over the trailing optInTimePeriod bars; if highestHigh == lowestLow, output 0.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WILLR_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Lookback bars for the high/low range (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Williams' %R value in [-100, 0]. Must hold at least
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
    * @see Core#STOCH
    * @see Core#STOCHF
    * @see Core#MINMAX
    */
   public OutRange WILLR( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("WILLR", startIdx, endIdx);
      int guardStart = clampedStart("WILLR", startIdx, WILLR_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WILLR", "inHigh", inHigh, guardInLen);
      requireLength("WILLR", "inLow", inLow, guardInLen);
      requireLength("WILLR", "inClose", inClose, guardInLen);
      requireLength("WILLR", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WILLR_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WILLR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using willr_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live WILLR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#WILLR} over the same series.
    * Open with {@link Core#willrOpen}; there is no close — the handle is
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
   public static final class WillrStream {
      Core core;
      int optInTimePeriod;
      double lowest;
      double highest;
      double diff;
      int trailingIdx;
      int lowestIdx;
      int highestIdx;
      int i;
      int today;
      int xMask;
      double[] x_inHigh;
      double[] x_inLow;
      double[] x_inClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      WillrStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#WILLR} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      WillrStream( WillrStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.diff = other.diff;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.x_inClose = other.x_inClose.clone();
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
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("WILLR update: BadParam", RetCode.BadParam);
         }
         core.willrStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("WILLR updateAndFill", "inHigh", inHigh);
         requireArgument("WILLR updateAndFill", "inLow", inLow);
         requireArgument("WILLR updateAndFill", "inClose", inClose);
         requireArgument("WILLR updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("WILLR updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("WILLR updateAndFill: BadParam", RetCode.BadParam);
            }
            core.willrStepImpl(this, inHigh[i], inLow[i], inClose[i]);
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
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("WILLR peek: BadParam", RetCode.BadParam);
         WillrStream sp = this;
         double tmp = 0.0;
         double cur_outReal = 0.0;
         double diff = sp.diff;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int today = sp.today;
         int trailingIdx = sp.trailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         if( today >= 1073741824 ) {
            int rebaseShift = trailingIdx & ~sp.xMask;
            today -= rebaseShift;
            trailingIdx -= rebaseShift;
            highestIdx -= rebaseShift;
            i -= rebaseShift;
            lowestIdx -= rebaseShift;
         }
         pkSlot0 = today & sp.xMask;
         pkVal0 = inHigh;
         pkSlot1 = today & sp.xMask;
         pkVal1 = inLow;
         pkSlot2 = today & sp.xMask;
         pkVal2 = inClose;
         /* Set the lowest low */
         tmp = ((today & sp.xMask) != pkSlot1) ? sp.x_inLow[today & sp.xMask] : pkVal1;
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = ((lowestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         /* Set the highest high */
         tmp = ((today & sp.xMask) != pkSlot0) ? sp.x_inHigh[today & sp.xMask] : pkVal0;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= today ) {
               tmp = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         if( diff != 0.0 ) {
            cur_outReal = (highest - (((today & sp.xMask) != pkSlot2) ? sp.x_inClose[today & sp.xMask] : pkVal2)) / diff;
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
      public WillrStream clone() {
         return new WillrStream(this);
      }
   }
   void willrStepImpl( WillrStream sp, double inHigh, double inLow, double inClose )
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
      sp.x_inClose[sp.today & sp.xMask] = inClose;
      /* Set the lowest low */
      tmp = sp.x_inLow[sp.today & sp.xMask];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i & sp.xMask];
            if( tmp < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
         sp.diff = (sp.highest - sp.lowest) / (0 - 100.0);
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
         sp.diff = (sp.highest - sp.lowest) / (0 - 100.0);
      }
      /* Set the highest high */
      tmp = sp.x_inHigh[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i & sp.xMask];
            if( tmp > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
         sp.diff = (sp.highest - sp.lowest) / (0 - 100.0);
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
         sp.diff = (sp.highest - sp.lowest) / (0 - 100.0);
      }
      if( sp.diff != 0.0 ) {
         sp.cur_outReal = (sp.highest - sp.x_inClose[sp.today & sp.xMask]) / sp.diff;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode willrOpenImpl( WillrStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
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
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
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
      /* Initialize 'diff', just to avoid warning. */
      diff = 0.0;
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * The highest high and lowest low of the window are cached with their
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
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      diff = highest;
      while( today <= endIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
            diff = (highest - lowest) / (0 - 100.0);
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / (0 - 100.0);
         }
         if( diff != 0.0 ) {
            outReal[outIdx++ * outStride] = (highest - inClose[today]) / diff;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
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
      double[] capX_inClose = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
         capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
         capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.lowest = lowest;
      sp.highest = highest;
      sp.diff = diff;
      sp.trailingIdx = trailingIdx;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* willrOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WillrStream willrOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WillrStream sp = new WillrStream(this);
      RetCode retCode = willrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WILLR openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WILLR openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("WILLR openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind willrOpen (composition seam). */
   WillrStream willrOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod )
   {
      WillrStream sp = new WillrStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = willrOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WILLR open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WILLR open: internal error", retCode);
      }
      throw new TaLibArgumentException("WILLR open: " + retCode, retCode);
   }
   /**
    * Open a live WILLR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#WILLR} at that bar.
    * <p>The history must hold at least {@code WILLR_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public WillrStream willrOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod )
   {
      requireArgument("WILLR open", "inHigh", inHigh);
      requireHistory("WILLR open", inHigh.length);
      requireArgument("WILLR open", "inLow", inLow);
      requireArgument("WILLR open", "inClose", inClose);
      requireHistoryLength("WILLR open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WILLR open", "inClose", inClose.length, inHigh.length);
      return willrOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod);
   }
   /**
    * {@link Core#willrOpen} that also fills the output array(s) bit-identically
    * to {@link Core#WILLR} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link WillrStream#outRange()}.
    */
   public WillrStream willrOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("WILLR openAndFill", "inHigh", inHigh);
      requireHistory("WILLR openAndFill", inHigh.length);
      requireArgument("WILLR openAndFill", "inLow", inLow);
      requireArgument("WILLR openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("WILLR openAndFill", inHigh.length, WILLR_Lookback(optInTimePeriod));
      requireHistoryLength("WILLR openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WILLR openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("WILLR openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("WILLR openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return willrOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
