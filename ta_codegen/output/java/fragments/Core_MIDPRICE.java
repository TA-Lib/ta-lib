/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070226 MF,CC  Speed optimization: for periods above 20, cache the
 *                highest/lowest index instead of rescanning the window
 *                on every bar (same approach as MIN/MAX/WILLR). Smaller
 *                periods keep the simple scan, which auto-vectorizes
 *                and is faster there. Both paths produce identical
 *                output.
 */

   /**
    * Number of leading input bars {@link Core#MIDPRICE} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length over which the high/low extremes are
    *        taken (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MIDPRICE_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MIDPRICE_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
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
      double tmpLow = 0;
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
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* MIDPRICE = (Highest High + Lowest Low)/2
       *
       * This function is equivalent to MEDPRICE when the
       * period is 1.
       */
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
       * into a per-bar automaton, so the streaming tier runs midprice_ALT1
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
            tmpHigh = inHigh[i];
            if( tmpHigh > highest ) {
               highest = tmpHigh;
            }
            tmpLow = inLow[i];
            if( tmpLow < lowest ) {
               lowest = tmpLow;
            }
            sufHighest[i - blockStart] = highest;
            sufLowest[i - blockStart] = lowest;
         }
         outReal[outIdx++] = (sufHighest[0] + sufLowest[0]) / 2.0;
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
               tmpHigh = inHigh[blockNext + i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
               tmpLow = inLow[blockNext + i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
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
               outReal[outIdx++] = (highest + lowest) / 2.0;
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
   RetCode MIDPRICE_Impl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
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
      double tmpLow = 0;
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
            tmpHigh = (double)inHigh[i];
            if( tmpHigh > highest ) {
               highest = tmpHigh;
            }
            tmpLow = (double)inLow[i];
            if( tmpLow < lowest ) {
               lowest = tmpLow;
            }
            sufHighest[i - blockStart] = highest;
            sufLowest[i - blockStart] = lowest;
         }
         outReal[outIdx++] = (sufHighest[0] + sufLowest[0]) / 2.0;
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
               tmpHigh = (double)inHigh[blockNext + i];
               if( tmpHigh > highest ) {
                  highest = tmpHigh;
               }
               tmpLow = (double)inLow[blockNext + i];
               if( tmpLow < lowest ) {
                  lowest = tmpLow;
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
               outReal[outIdx++] = (highest + lowest) / 2.0;
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
    * Midpoint of the price range over a rolling window: the average of the
    * highest high and lowest low across the last optInTimePeriod bars. An
    * overlap-study line plotted on price.
    * <p><b>Formula</b>
    * <pre>{@code
    * MIDPRICE = (Highest(High, N) + Lowest(Low, N)) / 2, over the N=optInTimePeriod bars ending at each index
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MIDPRICE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Window length over which the high/low extremes are
    *        taken (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Midpoint of the period's high/low extremes. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MIDPOINT
    * @see Core#MEDPRICE
    */
   public OutRange MIDPRICE( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("MIDPRICE", startIdx, endIdx);
      int guardStart = clampedStart("MIDPRICE", startIdx, MIDPRICE_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MIDPRICE", "inHigh", inHigh, guardInLen);
      requireLength("MIDPRICE", "inLow", inLow, guardInLen);
      requireLength("MIDPRICE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MIDPRICE_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MIDPRICE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Midpoint of the price range over a rolling window: the average of the
    * highest high and lowest low across the last optInTimePeriod bars. An
    * overlap-study line plotted on price.
    * <p><b>Formula</b>
    * <pre>{@code
    * MIDPRICE = (Highest(High, N) + Lowest(Low, N)) / 2, over the N=optInTimePeriod bars ending at each index
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MIDPRICE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Window length over which the high/low extremes are
    *        taken (default 14; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outReal Midpoint of the period's high/low extremes. Must hold at
    *        least {@code endIdx - startIdx + 1} values.
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
    * @see Core#MIDPOINT
    * @see Core#MEDPRICE
    */
   public OutRange MIDPRICE( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             int optInTimePeriod,
                             double outReal[] )
   {
      requireIndexRange("MIDPRICE", startIdx, endIdx);
      int guardStart = clampedStart("MIDPRICE", startIdx, MIDPRICE_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MIDPRICE", "inHigh", inHigh, guardInLen);
      requireLength("MIDPRICE", "inLow", inLow, guardInLen);
      requireLength("MIDPRICE", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MIDPRICE_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MIDPRICE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using midprice_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live MIDPRICE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MIDPRICE} over the same series.
    * Open with {@link Core#MIDPRICE_Open}; there is no close — the handle is
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
   public static final class MIDPRICE_Stream {
      Core core;
      int optInTimePeriod;
      double lowest;
      double highest;
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

      MIDPRICE_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MIDPRICE} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MIDPRICE_Stream( MIDPRICE_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
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

      void copyFrom( MIDPRICE_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.highest = other.highest;
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
      private static final ThreadLocal<MIDPRICE_Stream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("MIDPRICE update: BadParam", RetCode.BadParam);
         core.MIDPRICE_StepImpl(this, inHigh, inLow);
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
         requireArgument("MIDPRICE updateAndFill", "inHigh", inHigh);
         requireArgument("MIDPRICE updateAndFill", "inLow", inLow);
         requireArgument("MIDPRICE updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("MIDPRICE updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) )
               throw new TaLibArgumentException("MIDPRICE updateAndFill: BadParam", RetCode.BadParam);
            core.MIDPRICE_StepImpl(this, inHigh[i], inLow[i]);
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
            throw new TaLibArgumentException("MIDPRICE peek: BadParam", RetCode.BadParam);
         MIDPRICE_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new MIDPRICE_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.MIDPRICE_StepImpl(scratch, inHigh, inLow);
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
      public MIDPRICE_Stream copy() {
         return new MIDPRICE_Stream(this);
      }
   }
   void MIDPRICE_StepImpl( MIDPRICE_Stream sp, double inHigh, double inLow )
   {
      double tmpLow = 0.0;
      double tmpHigh = 0.0;
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
      tmpHigh = sp.x_inHigh[sp.today & sp.xMask];
      tmpLow = sp.x_inLow[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmpHigh = sp.x_inHigh[sp.i & sp.xMask];
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
         sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmpLow = sp.x_inLow[sp.i & sp.xMask];
            if( tmpLow < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmpLow;
            }
         }
      } else if( tmpLow <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmpLow;
      }
      sp.cur_outReal = (sp.highest + sp.lowest) / 2.0;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode MIDPRICE_OpenImpl( MIDPRICE_Stream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double lowest = 0;
      double highest = 0;
      double tmpLow = 0;
      double tmpHigh = 0;
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
      /* MIDPRICE = (Highest High + Lowest Low)/2
       *
       * This function is equivalent to MEDPRICE when the
       * period is 1.
       */
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
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = inHigh[today];
         tmpLow = inLow[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = inHigh[i];
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
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         outReal[outIdx++ * outStride] = (highest + lowest) / 2.0;
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
   /* MIDPRICE_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MIDPRICE_Stream MIDPRICE_OpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MIDPRICE_Stream sp = new MIDPRICE_Stream(this);
      RetCode retCode = MIDPRICE_OpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MIDPRICE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MIDPRICE openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MIDPRICE openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind MIDPRICE_Open (composition seam). */
   MIDPRICE_Stream MIDPRICE_OpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      MIDPRICE_Stream sp = new MIDPRICE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = MIDPRICE_OpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MIDPRICE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MIDPRICE open: internal error", retCode);
      }
      throw new TaLibArgumentException("MIDPRICE open: " + retCode, retCode);
   }
   /**
    * Open a live MIDPRICE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MIDPRICE} at that bar.
    * <p>The history must hold at least {@code MIDPRICE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MIDPRICE_Stream MIDPRICE_Open( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("MIDPRICE open", "inHigh", inHigh);
      requireHistory("MIDPRICE open", inHigh.length);
      requireArgument("MIDPRICE open", "inLow", inLow);
      requireHistoryLength("MIDPRICE open", "inLow", inLow.length, inHigh.length);
      return MIDPRICE_OpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#MIDPRICE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MIDPRICE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MIDPRICE_Stream#outRange()}.
    */
   public MIDPRICE_Stream MIDPRICE_OpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("MIDPRICE openAndFill", "inHigh", inHigh);
      requireHistory("MIDPRICE openAndFill", inHigh.length);
      requireArgument("MIDPRICE openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("MIDPRICE openAndFill", inHigh.length, MIDPRICE_Lookback(optInTimePeriod));
      requireHistoryLength("MIDPRICE openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("MIDPRICE openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("MIDPRICE openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return MIDPRICE_OpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
