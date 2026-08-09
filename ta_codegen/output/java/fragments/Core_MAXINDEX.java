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
 *  120806 AC   Creation (equal to MAX but outputs index)
 */

   /**
    * Number of leading input bars {@link Core#MAXINDEX} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Window length over which the max is located
    *        (default 30; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MAXINDEX_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode MAXINDEX_Internal( int startIdx,
                              int endIdx,
                              double inReal[],
                              int optInTimePeriod,
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double highest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
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
         outInteger[outIdx++] = highestIdx;
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
   RetCode MAXINDEX_Internal( int startIdx,
                              int endIdx,
                              float inReal[],
                              int optInTimePeriod,
                              MInteger outBegIdx,
                              MInteger outNBElement,
                              int outInteger[] )
   {
      double highest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
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
      highestIdx = 0 - 1;
      highest = 0.0;
      while( today <= endIdx ) {
         tmp = (double)inReal[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inReal[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inReal[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         outInteger[outIdx++] = highestIdx;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Returns the index of the highest input value within a rolling window of
    * optInTimePeriod bars. Same as MAX but outputs the location instead of the
    * value.
    * <p><b>Formula</b>
    * <pre>{@code
    * outInteger[i] = argmax_{j in [i-optInTimePeriod+1, i]} inReal[j]
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When several bars in a window share the highest value, which bar's index is returned is not guaranteed to be a specific one of the tied bars.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MAXINDEX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series to scan.
    * @param optInTimePeriod Window length over which the max is located
    *        (default 30; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outInteger Absolute index (into inReal) of the highest value in
    *        each window. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#MAX
    * @see Core#MININDEX
    * @see Core#MIN
    * @see Core#MINMAXINDEX
    */
   public OutRange MAXINDEX( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAXINDEX_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("MAXINDEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Returns the index of the highest input value within a rolling window of
    * optInTimePeriod bars. Same as MAX but outputs the location instead of the
    * value.
    * <p><b>Formula</b>
    * <pre>{@code
    * outInteger[i] = argmax_{j in [i-optInTimePeriod+1, i]} inReal[j]
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When several bars in a window share the highest value, which bar's index is returned is not guaranteed to be a specific one of the tied bars.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MAXINDEX_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series to scan.
    * @param optInTimePeriod Window length over which the max is located
    *        (default 30; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outInteger Absolute index (into inReal) of the highest value in
    *        each window. Must hold at least {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#MAX
    * @see Core#MININDEX
    * @see Core#MIN
    * @see Core#MINMAXINDEX
    */
   public OutRange MAXINDEX( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAXINDEX_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("MAXINDEX", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MAXINDEX stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MAXINDEX} over the same series.
    * Open with {@link Core#MAXINDEX_Open}; there is no close — the handle is
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
   public static final class MAXINDEX_Stream {
      final Core core;
      int optInTimePeriod;
      double highest;
      int trailingIdx;
      int i;
      int highestIdx;
      int today;
      int xCap;
      double[] x_inReal;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      MAXINDEX_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MAXINDEX_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MAXINDEX_Stream( MAXINDEX_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.highestIdx = other.highestIdx;
         this.today = other.today;
         this.xCap = other.xCap;
         this.x_inReal = other.x_inReal.clone();
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inReal ) {
         core.MAXINDEX_StreamStep(this, inReal);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public int peek( double inReal ) {
         MAXINDEX_Stream scratch = new MAXINDEX_Stream(this);
         core.MAXINDEX_StreamStep(scratch, inReal);
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
      public MAXINDEX_Stream copy() {
         return new MAXINDEX_Stream(this);
      }
   }
   void MAXINDEX_StreamStep( MAXINDEX_Stream sp, double inReal )
   {
      double tmp = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = (sp.trailingIdx / sp.xCap) * sp.xCap;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
      }
      sp.x_inReal[sp.today % sp.xCap] = inReal;
      tmp = sp.x_inReal[sp.today % sp.xCap];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inReal[sp.highestIdx % sp.xCap];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inReal[sp.i % sp.xCap];
            if( tmp > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      sp.cur_outInteger = sp.highestIdx;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode MAXINDEX_OpenBody( MAXINDEX_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      double highest = 0;
      double tmp = 0;
      int outIdx = 0;
      int nbInitialElementNeeded = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int highestIdx = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
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
       * (The integer output can never share the real input's buffer —
       * different element type; issue #130.)
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
         lastValue_outInteger = highestIdx;
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
      double[] capX_inReal = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ % capX] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.highest = highest;
      sp.trailingIdx = trailingIdx;
      sp.i = i;
      sp.highestIdx = highestIdx;
      sp.today = today;
      sp.xCap = capX;
      sp.x_inReal = capX_inReal;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode MAXINDEX_OpenAndFillBody( MAXINDEX_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
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
      int startIdx = 0;
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
      if( (Object)outInteger == (Object)inReal ) {
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
       * (The integer output can never share the real input's buffer —
       * different element type; issue #130.)
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
         outInteger[outIdx++] = highestIdx;
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
      double[] capX_inReal = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inReal[fillJ % capX] = inReal[fillJ];
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.highest = highest;
      sp.trailingIdx = trailingIdx;
      sp.i = i;
      sp.highestIdx = highestIdx;
      sp.today = today;
      sp.xCap = capX;
      sp.x_inReal = capX_inReal;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind MAXINDEX_Open (composition seam). */
   MAXINDEX_Stream MAXINDEX_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      MAXINDEX_Stream sp = new MAXINDEX_Stream(this);
      RetCode retCode = MAXINDEX_OpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MAXINDEX open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MAXINDEX open: internal error");
      }
      throw new IllegalArgumentException("MAXINDEX open: " + retCode);
   }
   /**
    * Open a live MAXINDEX stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MAXINDEX} at that bar.
    * <p>The history must hold at least {@code MAXINDEX_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MAXINDEX_Stream MAXINDEX_Open( double inReal[], int optInTimePeriod )
   {
      return MAXINDEX_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#MAXINDEX_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MAXINDEX} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MAXINDEX_Stream#fillRange()}.
    */
   public MAXINDEX_Stream MAXINDEX_OpenAndFill( double inReal[], int optInTimePeriod, int outInteger[] )
   {
      MAXINDEX_Stream sp = new MAXINDEX_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MAXINDEX_OpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("MAXINDEX openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("MAXINDEX openAndFill: internal error");
      }
      throw new IllegalArgumentException("MAXINDEX openAndFill: " + retCode);
   }
