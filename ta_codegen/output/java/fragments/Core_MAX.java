/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  101902 JV   Speed optimization of the algorithm
 *  102202 MF   Speed optimize a bit further
 *  052603 MF   Adapt code to compile with .NET Managed C++
 */

   /**
    * Number of leading input bars {@link Core#MAX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length in bars (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MAX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MAX_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
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
      double highest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int blockStart = 0;
      int nAvail = 0;
      int m = 0;
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
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * Van Herk / Gil-Werman block scan, block-batched form. The p outputs
       * belonging to one block boundary are produced together: one backward
       * pass builds the older block's suffix extrema, one forward pass builds
       * the newer block's prefix extrema, and a third pass combines them.
       * All the loops are straight-line with no data-dependent branching,
       * which is what lets a compiler vectorize them, and the work per bar is
       * a fixed number of comparisons regardless of period. Every scratch
       * array holds COPIES, so input and output may alias.
       *
       * Producing a whole block at a time is also why this cannot be turned
       * into a per-bar automaton, so the streaming tier runs max_ALT1
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
      blockStart = trailingIdx;
      while( today <= endIdx ) {
         /* Suffix extrema of the block [blockStart, blockStart+p-1], which
          * is fully available here: today == blockStart+p-1 <= endIdx.
          * Scanning backward while keeping the incumbent on a tie
          * leaves the later element holding a tie, which is what lets this
          * compile to a single max instruction.
          */
         i = blockStart + optInTimePeriod - 1;
         highest = inReal[i];
         sufHighest[optInTimePeriod - 1] = highest;
         while( i > blockStart ) {
            i -= 1;
            tmp = inReal[i];
            if( tmp > highest ) {
               highest = tmp;
            }
            sufHighest[i - blockStart] = highest;
         }
         highest = sufHighest[0];
         outReal[outIdx++] = highest;
         trailingIdx += 1;
         today += 1;
         if( today > endIdx ) {
            blockStart = blockStart + optInTimePeriod;
         } else {
            /* Prefix extrema of the next block, clamped to what remains.
             * Forward, keeping the incumbent on a tie: earliest wins again.
             */
            nAvail = endIdx - (blockStart + optInTimePeriod) + 1;
            if( nAvail > optInTimePeriod - 1 ) {
               nAvail = optInTimePeriod - 1;
            }
            highest = inReal[blockStart + optInTimePeriod];
            preHighest[0] = highest;
            i = 1;
            while( i < nAvail ) {
               tmp = inReal[blockStart + optInTimePeriod + i];
               if( tmp > highest ) {
                  highest = tmp;
               }
               preHighest[i] = highest;
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
               outReal[outIdx++] = highest;
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
   RetCode MAX_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
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
      double highest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int blockStart = 0;
      int nAvail = 0;
      int m = 0;
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
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      sufHighest = new double[optInTimePeriod];
      maxIdx_sufHighest = (optInTimePeriod)-1;
      sufHighest_Idx = 0;
      if( optInTimePeriod < 1 ) return RetCode.InternalError;
      preHighest = new double[optInTimePeriod];
      maxIdx_preHighest = (optInTimePeriod)-1;
      preHighest_Idx = 0;
      blockStart = trailingIdx;
      while( today <= endIdx ) {
         i = blockStart + optInTimePeriod - 1;
         highest = (double)inReal[i];
         sufHighest[optInTimePeriod - 1] = highest;
         while( i > blockStart ) {
            i -= 1;
            tmp = (double)inReal[i];
            if( tmp > highest ) {
               highest = tmp;
            }
            sufHighest[i - blockStart] = highest;
         }
         highest = sufHighest[0];
         outReal[outIdx++] = highest;
         trailingIdx += 1;
         today += 1;
         if( today > endIdx ) {
            blockStart = blockStart + optInTimePeriod;
         } else {
            nAvail = endIdx - (blockStart + optInTimePeriod) + 1;
            if( nAvail > optInTimePeriod - 1 ) {
               nAvail = optInTimePeriod - 1;
            }
            highest = (double)inReal[blockStart + optInTimePeriod];
            preHighest[0] = highest;
            i = 1;
            while( i < nAvail ) {
               tmp = (double)inReal[blockStart + optInTimePeriod + i];
               if( tmp > highest ) {
                  highest = tmp;
               }
               preHighest[i] = highest;
               i += 1;
            }
            m = 1;
            while( m <= nAvail ) {
               highest = sufHighest[m];
               if( preHighest[m - 1] > highest ) {
                  highest = preHighest[m - 1];
               }
               outReal[outIdx++] = highest;
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
    * Highest input value over a rolling window of the last optInTimePeriod
    * bars. A moving-window maximum.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = max(inReal[i-optInTimePeriod+1 .. i])
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MAX_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to take the rolling maximum of.
    * @param optInTimePeriod Window length in bars (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Highest value within each trailing window. Must hold at
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
    * @see Core#MIN
    * @see Core#MAXINDEX
    * @see Core#MINMAX
    */
   public OutRange MAX( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("MAX", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, MAX_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MAX", "inReal", inReal, guardInLen);
      requireLength("MAX", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MAX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Highest input value over a rolling window of the last optInTimePeriod
    * bars. A moving-window maximum.
    * <p><b>Formula</b>
    * <pre>{@code
    * outReal[i] = max(inReal[i-optInTimePeriod+1 .. i])
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MAX_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Series to take the rolling maximum of.
    * @param optInTimePeriod Window length in bars (default 30; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Highest value within each trailing window. Must hold at
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
    * @see Core#MIN
    * @see Core#MAXINDEX
    * @see Core#MINMAX
    */
   public OutRange MAX( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("MAX", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, MAX_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MAX", "inReal", inReal, guardInLen);
      requireLength("MAX", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAX_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MAX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using max_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live MAX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MAX} over the same series.
    * Open with {@link Core#MAX_Open}; there is no close — the handle is
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
   public static final class MAX_Stream {
      Core core;
      int optInTimePeriod;
      double highest;
      int trailingIdx;
      int i;
      int highestIdx;
      int today;
      int xMask;
      double[] x_inReal;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      MAX_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MAX_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MAX_Stream( MAX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.highestIdx = other.highestIdx;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( MAX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.highestIdx = other.highestIdx;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inReal != null && this.x_inReal.length == other.x_inReal.length ) {
            System.arraycopy( other.x_inReal, 0, this.x_inReal, 0, other.x_inReal.length );
         } else {
            this.x_inReal = other.x_inReal.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MAX update: BadParam", RetCode.BadParam);
         core.MAX_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MAX peek: BadParam", RetCode.BadParam);
         MAX_Stream scratch = new MAX_Stream(this);
         core.MAX_StreamStep(scratch, inReal);
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
      public MAX_Stream copy() {
         return new MAX_Stream(this);
      }
   }
   void MAX_StreamStep( MAX_Stream sp, double inReal )
   {
      double tmp = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
      }
      sp.x_inReal[sp.today & sp.xMask] = inReal;
      tmp = sp.x_inReal[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inReal[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inReal[sp.i & sp.xMask];
            if( tmp > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      sp.cur_outReal = sp.highest;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode MAX_OpenPass( MAX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double highest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
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
         return RetCode.InsufficientHistory ;
      }
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * The highest value of the window is cached with its index; the window is
       * rescanned only when the cached extremum drops out of it. That is O(1)
       * per bar while the extremum sits away from the trailing edge, but it is
       * not amortized O(1): an extremum on the oldest in-window bar drops out
       * on the very next bar, so the rescan repeats and the cost stays
       * O(period) per bar for as long as that persists.
       *
       * Slower than the block scan the batch tier runs; it is here because one
       * bar at a time is exactly what the streaming tier needs. See issue #147.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
      highestIdx = 0 - 1;
      highest = 0.0;
      while( today <= endIdx ) {
         tmp = inReal[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inReal[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inReal[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outReal[outIdx++ * outStride] = highest;
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
      sp.trailingIdx = trailingIdx;
      sp.i = i;
      sp.highestIdx = highestIdx;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inReal = capX_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode MAX_OpenImpl( MAX_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return MAX_OpenPass( sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode MAX_OpenAndFillImpl( MAX_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return MAX_OpenPass( sp, inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode MAX_OpenAndFillInternalImpl( MAX_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return MAX_OpenPass(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* MAX_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MAX_Stream MAX_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MAX_Stream sp = new MAX_Stream(this);
      RetCode retCode = MAX_OpenAndFillInternalImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MAX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MAX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MAX openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind MAX_Open (composition seam). */
   MAX_Stream MAX_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MAX_Stream sp = new MAX_Stream(this);
      RetCode retCode = MAX_OpenImpl(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MAX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MAX open: internal error", retCode);
      }
      throw new TaLibArgumentException("MAX open: " + retCode, retCode);
   }
   /**
    * Open a live MAX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MAX} at that bar.
    * <p>The history must hold at least {@code MAX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MAX_Stream MAX_Open( double inReal[], int optInTimePeriod )
   {
      return MAX_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#MAX_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MAX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MAX_Stream#fillRange()}.
    */
   public MAX_Stream MAX_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      MAX_Stream sp = new MAX_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAX_OpenAndFillImpl(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MAX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MAX openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MAX openAndFill: " + retCode, retCode);
   }
