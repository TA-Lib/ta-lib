/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin (@kevinlincg)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  081226 KL   Initial version (#200).
 */

   /**
    * Number of leading input bars {@link Core#WAD} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int WAD_Lookback( )
   {
      /* The first bar has no previous close, so it accumulates nothing and the
       * line starts at 0.0 -- the same convention as the other four cumulative
       * lines in the tree: OBV, AD, NVI and PVI all return 0 here and emit a
       * seed value at startIdx. Tulip's ti_wad_start() returns 1 instead, so its
       * series is this one without the leading zero.
       */
      return 0 ;

   }
   RetCode WAD_Impl( int startIdx,
                     int endIdx,
                     double inHigh[],
                     double inLow[],
                     double inClose[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double sum = 0;
      double prevClose = 0;
      double close = 0;
      double trueExtreme = 0;
      int i = 0;
      int outIdx = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Williams' Accumulation/Distribution, in the form Steven Achelis
       * published (Technical Analysis from A to Z, 2nd ed., p.368) and the form
       * every modern vendor ships: each bar's close is measured against the TRUE
       * range extreme -- the previous close when it lies outside today's bar --
       * and the results accumulate.
       *
       *    TRH = max( prevClose, high )      TRL = min( prevClose, low )
       *    AD  = close - TRL   if close > prevClose
       *        = close - TRH   if close < prevClose
       *        = 0             if close == prevClose
       *    WAD = running sum of AD
       *
       * NO VOLUME IS CONSUMED, despite the name and despite the group this is
       * filed under. Larry Williams' original multiplies the move by volume;
       * Achelis' modification drops it, the industry attached Williams' name to
       * the modification anyway, and Tulip, pandas-ta-classic, cTrader, TC2000,
       * WealthCharts and MultiCharts all ship the no-volume form. Shipping the
       * volume form under this name would surprise every user, so this is the
       * one place the usual "the original author wins" rule is set aside. The
       * volume-weighted series is a different indicator.
       *
       * The three-way branch is written with plain > and < rather than any
       * epsilon: the flat arm must fire on exactly-equal consecutive closes and
       * on nothing else, which also keeps -0.0 and NaN behaviour identical
       * across the C, Rust, Java and .NET backends.
       *
       * prevClose is carried in a scalar, so outReal may alias any input: every
       * read of bar i happens before the store at outIdx <= i, and no earlier
       * bar is ever re-read.
       */
      sum = 0.0;
      outIdx = 0;
      /* The first bar of the requested range is measured against itself, i.e. it
       * contributes exactly 0.0. The accumulator therefore restarts wherever the
       * caller starts, which is why this function is flagged path_dependent.
       */
      prevClose = inClose[startIdx];
      for( i = startIdx; i <= endIdx; i += 1 ) {
         close = inClose[i];
         if( close > prevClose ) {
            trueExtreme = inLow[i];
            if( prevClose < trueExtreme ) {
               trueExtreme = prevClose;
            }
            sum += close - trueExtreme;
         } else if( close < prevClose ) {
            trueExtreme = inHigh[i];
            if( prevClose > trueExtreme ) {
               trueExtreme = prevClose;
            }
            sum += close - trueExtreme;
         }
         outReal[outIdx] = sum;
         outIdx = outIdx + 1;
         prevClose = close;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode WAD_Impl( int startIdx,
                     int endIdx,
                     float inHigh[],
                     float inLow[],
                     float inClose[],
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double sum = 0;
      double prevClose = 0;
      double close = 0;
      double trueExtreme = 0;
      int i = 0;
      int outIdx = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      sum = 0.0;
      outIdx = 0;
      prevClose = (double)inClose[startIdx];
      for( i = startIdx; i <= endIdx; i += 1 ) {
         close = (double)inClose[i];
         if( close > prevClose ) {
            trueExtreme = (double)inLow[i];
            if( prevClose < trueExtreme ) {
               trueExtreme = prevClose;
            }
            sum += close - trueExtreme;
         } else if( close < prevClose ) {
            trueExtreme = (double)inHigh[i];
            if( prevClose > trueExtreme ) {
               trueExtreme = prevClose;
            }
            sum += close - trueExtreme;
         }
         outReal[outIdx] = sum;
         outIdx = outIdx + 1;
         prevClose = close;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Williams' Accumulation/Distribution: a cumulative line that measures each
    * bar's close against the *true range* extreme — the previous close,
    * whenever it lies outside the current bar — rather than against the bar's
    * own high and low. A close above the previous one accumulates the distance
    * up from the true low; a close below it distributes the distance down from
    * the true high; an unchanged close contributes nothing. **It consumes no
    * volume.** Larry Williams' original multiplies each move by that bar's
    * volume; Steven Achelis published the modification that drops the
    * multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), and the
    * industry kept Williams' name on that no-volume form. That industry-wide
    * decision is enough for TA-Lib to ship the same form under the same name.
    * <p><b>Formula</b>
    * <pre>{@code
    * TRH_t = max(close_{t-1}, high_t); TRL_t = min(close_{t-1}, low_t); AD_t = close_t - TRL_t if close_t > close_{t-1}, close_t - TRH_t if close_t < close_{t-1}, otherwise 0; WAD_t = WAD_{t-1} + AD_t
    * The first bar of the requested range has no previous close, so it contributes 0 and the line starts there — the same convention as AD, OBV, NVI and PVI. The accumulator restarts wherever the caller starts, so a different `startIdx` shifts the whole line by a constant.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WAD_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Cumulative accumulation/distribution. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#ADOSC
    * @see Core#NVI
    * @see Core#OBV
    * @see Core#PVI
    */
   public OutRange WAD( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double outReal[] )
   {
      requireIndexRange("WAD", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, WAD_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WAD", "inHigh", inHigh, guardInLen);
      requireLength("WAD", "inLow", inLow, guardInLen);
      requireLength("WAD", "inClose", inClose, guardInLen);
      requireLength("WAD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WAD_Impl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WAD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Williams' Accumulation/Distribution: a cumulative line that measures each
    * bar's close against the *true range* extreme — the previous close,
    * whenever it lies outside the current bar — rather than against the bar's
    * own high and low. A close above the previous one accumulates the distance
    * up from the true low; a close below it distributes the distance down from
    * the true high; an unchanged close contributes nothing. **It consumes no
    * volume.** Larry Williams' original multiplies each move by that bar's
    * volume; Steven Achelis published the modification that drops the
    * multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), and the
    * industry kept Williams' name on that no-volume form. That industry-wide
    * decision is enough for TA-Lib to ship the same form under the same name.
    * <p><b>Formula</b>
    * <pre>{@code
    * TRH_t = max(close_{t-1}, high_t); TRL_t = min(close_{t-1}, low_t); AD_t = close_t - TRL_t if close_t > close_{t-1}, close_t - TRH_t if close_t < close_{t-1}, otherwise 0; WAD_t = WAD_{t-1} + AD_t
    * The first bar of the requested range has no previous close, so it contributes 0 and the line starts there — the same convention as AD, OBV, NVI and PVI. The accumulator restarts wherever the caller starts, so a different `startIdx` shifts the whole line by a constant.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#WAD_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outReal Cumulative accumulation/distribution. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#AD
    * @see Core#ADOSC
    * @see Core#NVI
    * @see Core#OBV
    * @see Core#PVI
    */
   public OutRange WAD( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        double outReal[] )
   {
      requireIndexRange("WAD", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, WAD_Lookback());
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("WAD", "inHigh", inHigh, guardInLen);
      requireLength("WAD", "inLow", inLow, guardInLen);
      requireLength("WAD", "inClose", inClose, guardInLen);
      requireLength("WAD", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WAD_Impl(startIdx, endIdx, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("WAD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live WAD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#WAD} over the same series.
    * Open with {@link Core#WAD_Open}; there is no close — the handle is
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
   public static final class WAD_Stream {
      Core core;
      double sum;
      double prevClose;
      double trueExtreme;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      WAD_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#WAD_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      WAD_Stream( WAD_Stream other ) {
         this.core = other.core;
         this.sum = other.sum;
         this.prevClose = other.prevClose;
         this.trueExtreme = other.trueExtreme;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( WAD_Stream other ) {
         this.core = other.core;
         this.sum = other.sum;
         this.prevClose = other.prevClose;
         this.trueExtreme = other.trueExtreme;
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
      public double update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("WAD update: BadParam", RetCode.BadParam);
         core.WAD_StreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("WAD peek: BadParam", RetCode.BadParam);
         WAD_Stream scratch = new WAD_Stream(this);
         core.WAD_StreamStep(scratch, inHigh, inLow, inClose);
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
      public WAD_Stream copy() {
         return new WAD_Stream(this);
      }
   }
   void WAD_StreamStep( WAD_Stream sp, double inHigh, double inLow, double inClose )
   {
      double close = 0.0;
      close = inClose;
      if( close > sp.prevClose ) {
         sp.trueExtreme = inLow;
         if( sp.prevClose < sp.trueExtreme ) {
            sp.trueExtreme = sp.prevClose;
         }
         sp.sum += close - sp.trueExtreme;
      } else if( close < sp.prevClose ) {
         sp.trueExtreme = inHigh;
         if( sp.prevClose > sp.trueExtreme ) {
            sp.trueExtreme = sp.prevClose;
         }
         sp.sum += close - sp.trueExtreme;
      }
      sp.cur_outReal = sp.sum;
      sp.prevClose = close;
   }
   private RetCode WAD_OpenPass( WAD_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sum = 0;
      double prevClose = 0;
      double close = 0;
      double trueExtreme = 0;
      int i = 0;
      int outIdx = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      /* Williams' Accumulation/Distribution, in the form Steven Achelis
       * published (Technical Analysis from A to Z, 2nd ed., p.368) and the form
       * every modern vendor ships: each bar's close is measured against the TRUE
       * range extreme -- the previous close when it lies outside today's bar --
       * and the results accumulate.
       *
       *    TRH = max( prevClose, high )      TRL = min( prevClose, low )
       *    AD  = close - TRL   if close > prevClose
       *        = close - TRH   if close < prevClose
       *        = 0             if close == prevClose
       *    WAD = running sum of AD
       *
       * NO VOLUME IS CONSUMED, despite the name and despite the group this is
       * filed under. Larry Williams' original multiplies the move by volume;
       * Achelis' modification drops it, the industry attached Williams' name to
       * the modification anyway, and Tulip, pandas-ta-classic, cTrader, TC2000,
       * WealthCharts and MultiCharts all ship the no-volume form. Shipping the
       * volume form under this name would surprise every user, so this is the
       * one place the usual "the original author wins" rule is set aside. The
       * volume-weighted series is a different indicator.
       *
       * The three-way branch is written with plain > and < rather than any
       * epsilon: the flat arm must fire on exactly-equal consecutive closes and
       * on nothing else, which also keeps -0.0 and NaN behaviour identical
       * across the C, Rust, Java and .NET backends.
       *
       * prevClose is carried in a scalar, so outReal may alias any input: every
       * read of bar i happens before the store at outIdx <= i, and no earlier
       * bar is ever re-read.
       */
      sum = 0.0;
      outIdx = 0;
      /* The first bar of the requested range is measured against itself, i.e. it
       * contributes exactly 0.0. The accumulator therefore restarts wherever the
       * caller starts, which is why this function is flagged path_dependent.
       */
      prevClose = inClose[startIdx];
      for( i = startIdx; i <= endIdx; i += 1 ) {
         close = inClose[i];
         if( close > prevClose ) {
            trueExtreme = inLow[i];
            if( prevClose < trueExtreme ) {
               trueExtreme = prevClose;
            }
            sum += close - trueExtreme;
         } else if( close < prevClose ) {
            trueExtreme = inHigh[i];
            if( prevClose > trueExtreme ) {
               trueExtreme = prevClose;
            }
            sum += close - trueExtreme;
         }
         outReal[outIdx * outStride] = sum;
         outIdx = outIdx + 1;
         prevClose = close;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.sum = sum;
      sp.prevClose = prevClose;
      sp.trueExtreme = trueExtreme;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode WAD_OpenImpl( WAD_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return WAD_OpenPass( sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode WAD_OpenAndFillImpl( WAD_Stream sp, double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      return WAD_OpenPass( sp, inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode WAD_OpenAndFillInternalImpl( WAD_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return WAD_OpenPass(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
   }
   /* WAD_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WAD_Stream WAD_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WAD_Stream sp = new WAD_Stream(this);
      RetCode retCode = WAD_OpenAndFillInternalImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WAD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WAD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("WAD openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind WAD_Open (composition seam). */
   WAD_Stream WAD_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      WAD_Stream sp = new WAD_Stream(this);
      RetCode retCode = WAD_OpenImpl(sp, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WAD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WAD open: internal error", retCode);
      }
      throw new TaLibArgumentException("WAD open: " + retCode, retCode);
   }
   /**
    * Open a live WAD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#WAD} at that bar.
    * <p>The history must hold at least {@code WAD_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public WAD_Stream WAD_Open( double inHigh[], double inLow[], double inClose[] )
   {
      return WAD_OpenInternal(inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#WAD_Open} that also fills the output array(s) bit-identically
    * to {@link Core#WAD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link WAD_Stream#fillRange()}.
    */
   public WAD_Stream WAD_OpenAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      WAD_Stream sp = new WAD_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = WAD_OpenAndFillImpl(sp, inHigh, inLow, inClose, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("WAD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("WAD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("WAD openAndFill: " + retCode, retCode);
   }
