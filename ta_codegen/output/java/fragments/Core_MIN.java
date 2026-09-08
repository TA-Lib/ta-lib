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
    * Number of leading input bars {@link Core#MIN} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the trailing window (default 30;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MIN_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MIN_Impl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double[] sufLowest;
      int sufLowest_Idx = 0;
      int maxIdx_sufLowest = (30)-1;
      double[] preLowest;
      int preLowest_Idx = 0;
      int maxIdx_preLowest = (30)-1;
      double lowest = 0;
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
       * the newer block's prefix extrema, and a third pass combines them. All
       * three are straight-line loops with no data-dependent branching, which
       * is what lets a compiler vectorize them, and the work is 3 comparisons
       * per bar regardless of period. Both scratch arrays hold COPIES, so
       * input and output may alias.
       *
       * Producing a whole block at a time is also why this cannot be turned
       * into a per-bar automaton, so the streaming tier runs min_ALT1 below.
       */
      outIdx = 0;
      today = startIdx;
      trailingIdx = startIdx - nbInitialElementNeeded;
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
          * compile to a single min instruction.
          */
         i = blockStart + optInTimePeriod - 1;
         lowest = inReal[i];
         sufLowest[optInTimePeriod - 1] = lowest;
         while( i > blockStart ) {
            i -= 1;
            tmp = inReal[i];
            if( tmp < lowest ) {
               lowest = tmp;
            }
            sufLowest[i - blockStart] = lowest;
         }
         lowest = sufLowest[0];
         outReal[outIdx++] = lowest;
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
            lowest = inReal[blockStart + optInTimePeriod];
            preLowest[0] = lowest;
            i = 1;
            while( i < nAvail ) {
               tmp = inReal[blockStart + optInTimePeriod + i];
               if( tmp < lowest ) {
                  lowest = tmp;
               }
               preLowest[i] = lowest;
               i += 1;
            }
            /* Combine. The suffix half is the older one, so preferring it
             * on a tie keeps the earliest-wins rule.
             */
            m = 1;
            while( m <= nAvail ) {
               lowest = sufLowest[m];
               if( preLowest[m - 1] < lowest ) {
                  lowest = preLowest[m - 1];
               }
               outReal[outIdx++] = lowest;
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
   RetCode MIN_Impl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double[] sufLowest;
      int sufLowest_Idx = 0;
      int maxIdx_sufLowest = (30)-1;
      double[] preLowest;
      int preLowest_Idx = 0;
      int maxIdx_preLowest = (30)-1;
      double lowest = 0;
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
         lowest = (double)inReal[i];
         sufLowest[optInTimePeriod - 1] = lowest;
         while( i > blockStart ) {
            i -= 1;
            tmp = (double)inReal[i];
            if( tmp < lowest ) {
               lowest = tmp;
            }
            sufLowest[i - blockStart] = lowest;
         }
         lowest = sufLowest[0];
         outReal[outIdx++] = lowest;
         trailingIdx += 1;
         today += 1;
         if( today > endIdx ) {
            blockStart = blockStart + optInTimePeriod;
         } else {
            nAvail = endIdx - (blockStart + optInTimePeriod) + 1;
            if( nAvail > optInTimePeriod - 1 ) {
               nAvail = optInTimePeriod - 1;
            }
            lowest = (double)inReal[blockStart + optInTimePeriod];
            preLowest[0] = lowest;
            i = 1;
            while( i < nAvail ) {
               tmp = (double)inReal[blockStart + optInTimePeriod + i];
               if( tmp < lowest ) {
                  lowest = tmp;
               }
               preLowest[i] = lowest;
               i += 1;
            }
            m = 1;
            while( m <= nAvail ) {
               lowest = sufLowest[m];
               if( preLowest[m - 1] < lowest ) {
                  lowest = preLowest[m - 1];
               }
               outReal[outIdx++] = lowest;
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
    * Rolling minimum: the lowest input value over the trailing period.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/min">ta-lib.org/functions/min</a>.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MIN_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to take the minimum of.
    * @param optInTimePeriod Number of bars in the trailing window (default 30;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Lowest input value over the trailing window. Must hold at
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
    * @see Core#MAX
    * @see Core#MININDEX
    * @see Core#MINMAX
    */
   public OutRange MIN( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("MIN", startIdx, endIdx);
      int guardStart = clampedStart("MIN", startIdx, MIN_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MIN", "inReal", inReal, guardInLen);
      requireLength("MIN", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MIN_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MIN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Rolling minimum: the lowest input value over the trailing period.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/min">ta-lib.org/functions/min</a>.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MIN_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series to take the minimum of.
    * @param optInTimePeriod Number of bars in the trailing window (default 30;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Lowest input value over the trailing window. Must hold at
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
    * @see Core#MAX
    * @see Core#MININDEX
    * @see Core#MINMAX
    */
   public OutRange MIN( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("MIN", startIdx, endIdx);
      int guardStart = clampedStart("MIN", startIdx, MIN_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MIN", "inReal", inReal, guardInLen);
      requireLength("MIN", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MIN_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("MIN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using min_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live MIN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MIN} over the same series.
    * Open with {@link Core#minOpen}; there is no close — the handle is
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
   public static final class MinStream {
      private Core core;
      private int optInTimePeriod;
      private double lowest;
      private int trailingIdx;
      private int lowestIdx;
      private int i;
      private int today;
      private int xMask;
      private double[] x_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private MinStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MIN} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value()} keeps answering the previous
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
            throw failure("MIN advance", RetCode.OutOfRangeEndIndex);
         this.outRangeCount++;
      }

      private MinStream( MinStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.lowest = other.lowest;
         this.trailingIdx = other.trailingIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
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
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("MIN update", RetCode.OutOfRangeEndIndex);
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MIN update: BadParam", RetCode.BadParam);
         core.minStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MIN peek: BadParam", RetCode.BadParam);
         MinStream sp = this;
         double tmp = 0.0;
         double cur_outReal = 0.0;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.today & sp.xMask;
         pkVal0 = inReal;
         tmp = ((sp.today & sp.xMask) != pkSlot0) ? sp.x_inReal[sp.today & sp.xMask] : pkVal0;
         if( lowestIdx < sp.trailingIdx ) {
            lowestIdx = sp.trailingIdx;
            lowest = ((lowestIdx & sp.xMask) != pkSlot0) ? sp.x_inReal[lowestIdx & sp.xMask] : pkVal0;
            i = lowestIdx;
            while( ++i <= sp.today ) {
               tmp = ((i & sp.xMask) != pkSlot0) ? sp.x_inReal[i & sp.xMask] : pkVal0;
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = sp.today;
            lowest = tmp;
         }
         cur_outReal = lowest;
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
      public MinStream clone() {
         return new MinStream(this);
      }
   }
   private void minStepImpl( MinStream sp, double inReal )
   {
      double tmp = 0.0;
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
      sp.cur_outReal = sp.lowest;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode minOpenImpl( MinStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       *
       * Cache the lowest value with its index; rescan only when the cached
       * extremum leaves the window. O(1) per bar while the extremum sits away
       * from the trailing edge, but not amortized O(1): an extremum on the
       * oldest in-window bar drops out on the very next bar, so the rescan
       * repeats and the cost stays O(period) per bar for as long as that
       * persists. Slower than the block scan the batch tier runs; it is here
       * because one bar at a time is exactly what the streaming tier needs.
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
         outReal[outIdx++ * outStride] = lowest;
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
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* minOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MinStream minOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      MinStream sp = new MinStream(this);
      RetCode retCode = minOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MIN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MIN openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MIN openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind minOpen (composition seam). */
   MinStream minOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MinStream sp = new MinStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = minOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MIN open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MIN open: internal error", retCode);
      }
      throw new TaLibArgumentException("MIN open: " + retCode, retCode);
   }
   /**
    * Open a live MIN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MIN} at that bar.
    * <p>The history must hold at least {@code MIN_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MinStream minOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("MIN open", "inReal", inReal);
      requireHistory("MIN open", inReal.length);
      return minOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#minOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MIN} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MinStream#outRange()}.
    */
   public MinStream minOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("MIN openAndFill", "inReal", inReal);
      requireHistory("MIN openAndFill", inReal.length);
      int guardOutLen = openFillCount("MIN openAndFill", inReal.length, MIN_Lookback(optInTimePeriod));
      requireLength("MIN openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("MIN openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return minOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
