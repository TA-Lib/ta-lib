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
   RetCode MINMAX_Internal( int startIdx,
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
   RetCode MINMAX_Internal( int startIdx,
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAX_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
      if( retCode != RetCode.Success ) {
         throw failure("MINMAX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Returns both the lowest and highest values of the input over a rolling
    * window of the last optInTimePeriod bars. An overlap-study companion to MIN
    * and MAX that computes both extrema in one pass.
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
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAX_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
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
    * Open with {@link Core#MINMAX_Open}; there is no close — the handle is
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
   public static final class MINMAX_Stream {
      Core core;
      int optInTimePeriod;
      double highest;
      double lowest;
      double tmpHigh;
      double tmpLow;
      int trailingIdx;
      int i;
      int highestIdx;
      int lowestIdx;
      int today;
      int xMask;
      double[] x_inReal;
      double cur_outMin;
      double cur_outMax;
      Value cachedValue;
      OutRange fillRange = OutRange.EMPTY;

      MINMAX_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MINMAX_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MINMAX_Stream( MINMAX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.tmpHigh = other.tmpHigh;
         this.tmpLow = other.tmpLow;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outMin = other.cur_outMin;
         this.cur_outMax = other.cur_outMax;
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      void copyFrom( MINMAX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.tmpHigh = other.tmpHigh;
         this.tmpLow = other.tmpLow;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inReal != null && this.x_inReal.length == other.x_inReal.length ) {
            System.arraycopy( other.x_inReal, 0, this.x_inReal, 0, other.x_inReal.length );
         } else {
            this.x_inReal = other.x_inReal.clone();
         }
         this.cur_outMin = other.cur_outMin;
         this.cur_outMax = other.cur_outMax;
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param min Lowest value in each rolling window.
       * @param max Highest value in each rolling window.
       */
      public record Value(double min, double max) { }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.MINMAX_StreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outMin, this.cur_outMax);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public Value peek( double inReal ) {
         MINMAX_Stream scratch = new MINMAX_Stream(this);
         core.MINMAX_StreamStep(scratch, inReal);
         return new Value(scratch.cur_outMin, scratch.cur_outMax);
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
      public MINMAX_Stream copy() {
         return new MINMAX_Stream(this);
      }
   }
   void MINMAX_StreamStep( MINMAX_Stream sp, double inReal )
   {
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      sp.tmpHigh = sp.x_inReal[sp.today & sp.xMask];
      sp.tmpLow = sp.tmpHigh;
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inReal[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            sp.tmpHigh = sp.x_inReal[sp.i & sp.xMask];
            if( sp.tmpHigh > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = sp.tmpHigh;
            }
         }
      } else if( sp.tmpHigh >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = sp.tmpHigh;
      }
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inReal[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            sp.tmpLow = sp.x_inReal[sp.i & sp.xMask];
            if( sp.tmpLow < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = sp.tmpLow;
            }
         }
      } else if( sp.tmpLow <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = sp.tmpLow;
      }
      sp.cur_outMax = sp.highest;
      sp.cur_outMin = sp.lowest;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode MINMAX_OpenCore( MINMAX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outMin[], double outMax[], int outStride )
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
         return RetCode.OutOfRangeEndIndex ;
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
      sp.tmpHigh = tmpHigh;
      sp.tmpLow = tmpLow;
      sp.trailingIdx = trailingIdx;
      sp.i = i;
      sp.highestIdx = highestIdx;
      sp.lowestIdx = lowestIdx;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outMin = outMin[(outNBElement.value - 1) * outStride];
      sp.cur_outMax = outMax[(outNBElement.value - 1) * outStride];
      sp.cachedValue = new MINMAX_Stream.Value(sp.cur_outMin, sp.cur_outMax);
      return RetCode.Success;
   }
   private RetCode MINMAX_OpenBody( MINMAX_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMin = new double[1];
      double[] sink_outMax = new double[1];
      return MINMAX_OpenCore( sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outMin, sink_outMax, 0 );
   }
   private RetCode MINMAX_OpenAndFillBody( MINMAX_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outMin[], double outMax[] )
   {
      if( (Object)outMin == (Object)inReal || (Object)outMax == (Object)inReal || (Object)outMin == (Object)outMax ) {
         return RetCode.BadParam;
      }
      return MINMAX_OpenCore( sp, inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax, 1 );
   }
   private RetCode MINMAX_OpenAndFillInternalBody( MINMAX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outMin[], double outMax[] )
   {
      return MINMAX_OpenCore(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax, 1);
   }
   /* MINMAX_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MINMAX_Stream MINMAX_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outMin[], double outMax[] )
   {
      MINMAX_Stream sp = new MINMAX_Stream(this);
      RetCode retCode = MINMAX_OpenAndFillInternalBody(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MINMAX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MINMAX openAndFill: internal error");
      }
      throw new IllegalArgumentException("MINMAX openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind MINMAX_Open (composition seam). */
   MINMAX_Stream MINMAX_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MINMAX_Stream sp = new MINMAX_Stream(this);
      RetCode retCode = MINMAX_OpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MINMAX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MINMAX open: internal error");
      }
      throw new IllegalArgumentException("MINMAX open: " + retCode);
   }
   /**
    * Open a live MINMAX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MINMAX} at that bar.
    * <p>The history must hold at least {@code MINMAX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MINMAX_Stream MINMAX_Open( double inReal[], int optInTimePeriod )
   {
      return MINMAX_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#MINMAX_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MINMAX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MINMAX_Stream#fillRange()}.
    */
   public MINMAX_Stream MINMAX_OpenAndFill( double inReal[], int optInTimePeriod, double outMin[], double outMax[] )
   {
      MINMAX_Stream sp = new MINMAX_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MINMAX_OpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outMin, outMax);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MINMAX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MINMAX openAndFill: internal error");
      }
      throw new IllegalArgumentException("MINMAX openAndFill: " + retCode);
   }
