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
       * NO VOLUME IS CONSUMED, despite the name. Larry Williams' original
       * multiplies the move by volume; Achelis' modification drops it, the
       * industry attached Williams' name to the modification anyway, and Tulip,
       * pandas-ta-classic, cTrader, TC2000, WealthCharts and MultiCharts all ship
       * the no-volume form. Shipping the volume form under this name would
       * surprise every user, so this is the one place the usual "the original
       * author wins" rule is set aside. The volume-weighted series is a different
       * indicator. What is left once the multiplier is gone is a signed
       * close-to-close move clipped by the true-range extreme and accumulated --
       * momentum measured on the true range -- which is why this is grouped with
       * the other no-volume directional lines (PLUS_DM, MINUS_DM, BOP, WILLR)
       * rather than with AD/OBV.
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
    * Williams' Accumulation/Distribution: a cumulative line meant to expose
    * whether a security is quietly under accumulation (informed buying) or
    * distribution (informed selling) beneath the surface of price. Larry
    * Williams built it to catch that shift before price confirms it — traders
    * watch for the line to diverge from price, since a line that keeps rising
    * while price stalls or falls points to accumulation, and one that stalls
    * while price pushes to a new high points to distribution. **It consumes no
    * volume.** Larry Williams' original multiplies each move by that bar's
    * volume; Steven Achelis published the modification that drops the
    * multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), and the
    * industry kept Williams' name on that no-volume form. That industry-wide
    * decision is enough for TA-Lib to ship the same form under the same name.
    * What remains once the multiplier is dropped is a signed close-to-close
    * move measured on the true range, so it is grouped as a momentum indicator,
    * not a volume one.
    * <p><b>Formula</b>
    * <pre>{@code
    * For each bar t:
    * TRH_t = max(close_{t-1}, high_t)
    * TRL_t = min(close_{t-1}, low_t)
    * if close_t > close_{t-1} then AD_t = close_t - TRL_t
    * if close_t < close_{t-1} then AD_t = close_t - TRH_t
    * otherwise                     AD_t = 0
    * WAD_t = WAD_{t-1} + AD_t
    * The first bar of the requested range has no previous close, so the first output is always AD_t = 0. A different `startIdx` shifts WAD's whole line by a constant.
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("WAD", startIdx, WAD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Williams' Accumulation/Distribution: a cumulative line meant to expose
    * whether a security is quietly under accumulation (informed buying) or
    * distribution (informed selling) beneath the surface of price. Larry
    * Williams built it to catch that shift before price confirms it — traders
    * watch for the line to diverge from price, since a line that keeps rising
    * while price stalls or falls points to accumulation, and one that stalls
    * while price pushes to a new high points to distribution. **It consumes no
    * volume.** Larry Williams' original multiplies each move by that bar's
    * volume; Steven Achelis published the modification that drops the
    * multiplier (*Technical Analysis from A to Z*, 2nd ed., p.368), and the
    * industry kept Williams' name on that no-volume form. That industry-wide
    * decision is enough for TA-Lib to ship the same form under the same name.
    * What remains once the multiplier is dropped is a signed close-to-close
    * move measured on the true range, so it is grouped as a momentum indicator,
    * not a volume one.
    * <p><b>Formula</b>
    * <pre>{@code
    * For each bar t:
    * TRH_t = max(close_{t-1}, high_t)
    * TRL_t = min(close_{t-1}, low_t)
    * if close_t > close_{t-1} then AD_t = close_t - TRL_t
    * if close_t < close_{t-1} then AD_t = close_t - TRH_t
    * otherwise                     AD_t = 0
    * WAD_t = WAD_{t-1} + AD_t
    * The first bar of the requested range has no previous close, so the first output is always AD_t = 0. A different `startIdx` shifts WAD's whole line by a constant.
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
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
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
      int guardStart = clampedStart("WAD", startIdx, WAD_Lookback());
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#wadOpen}; there is no close — the handle is
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
   public static final class WadStream {
      Core core;
      double sum;
      double prevClose;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      WadStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#WAD} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      WadStream( WadStream other ) {
         this.core = other.core;
         this.sum = other.sum;
         this.prevClose = other.prevClose;
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
            throw new TaLibArgumentException("WAD update: BadParam", RetCode.BadParam);
         }
         core.wadStepImpl(this, inHigh, inLow, inClose);
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
         requireArgument("WAD updateAndFill", "inHigh", inHigh);
         requireArgument("WAD updateAndFill", "inLow", inLow);
         requireArgument("WAD updateAndFill", "inClose", inClose);
         requireArgument("WAD updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("WAD updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("WAD updateAndFill: BadParam", RetCode.BadParam);
            }
            core.wadStepImpl(this, inHigh[i], inLow[i], inClose[i]);
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
            throw new TaLibArgumentException("WAD peek: BadParam", RetCode.BadParam);
         WadStream sp = this;
         double close = 0.0;
         double trueExtreme = 0.0;
         double cur_outReal = 0.0;
         double sum = sp.sum;
         close = inClose;
         if( close > sp.prevClose ) {
            trueExtreme = inLow;
            if( sp.prevClose < trueExtreme ) {
               trueExtreme = sp.prevClose;
            }
            sum += close - trueExtreme;
         } else if( close < sp.prevClose ) {
            trueExtreme = inHigh;
            if( sp.prevClose > trueExtreme ) {
               trueExtreme = sp.prevClose;
            }
            sum += close - trueExtreme;
         }
         cur_outReal = sum;
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
      public WadStream clone() {
         return new WadStream(this);
      }
   }
   void wadStepImpl( WadStream sp, double inHigh, double inLow, double inClose )
   {
      double close = 0.0;
      double trueExtreme = 0.0;
      close = inClose;
      if( close > sp.prevClose ) {
         trueExtreme = inLow;
         if( sp.prevClose < trueExtreme ) {
            trueExtreme = sp.prevClose;
         }
         sp.sum += close - trueExtreme;
      } else if( close < sp.prevClose ) {
         trueExtreme = inHigh;
         if( sp.prevClose > trueExtreme ) {
            trueExtreme = sp.prevClose;
         }
         sp.sum += close - trueExtreme;
      }
      sp.cur_outReal = sp.sum;
      sp.prevClose = close;
   }
   private RetCode wadOpenImpl( WadStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sum = 0;
      double prevClose = 0;
      double close = 0;
      double trueExtreme = 0;
      int i = 0;
      int outIdx = 0;
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
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
       * NO VOLUME IS CONSUMED, despite the name. Larry Williams' original
       * multiplies the move by volume; Achelis' modification drops it, the
       * industry attached Williams' name to the modification anyway, and Tulip,
       * pandas-ta-classic, cTrader, TC2000, WealthCharts and MultiCharts all ship
       * the no-volume form. Shipping the volume form under this name would
       * surprise every user, so this is the one place the usual "the original
       * author wins" rule is set aside. The volume-weighted series is a different
       * indicator. What is left once the multiplier is gone is a signed
       * close-to-close move clipped by the true-range extreme and accumulated --
       * momentum measured on the true range -- which is why this is grouped with
       * the other no-volume directional lines (PLUS_DM, MINUS_DM, BOP, WILLR)
       * rather than with AD/OBV.
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
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* wadOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   WadStream wadOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      WadStream sp = new WadStream(this);
      RetCode retCode = wadOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
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
   /* Internal startIdx-anchored open behind wadOpen (composition seam). */
   WadStream wadOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      WadStream sp = new WadStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = wadOpenImpl(sp, inHigh, inLow, inClose, startIdx, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
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
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public WadStream wadOpen( double inHigh[], double inLow[], double inClose[] )
   {
      requireArgument("WAD open", "inHigh", inHigh);
      requireHistory("WAD open", inHigh.length);
      requireArgument("WAD open", "inLow", inLow);
      requireArgument("WAD open", "inClose", inClose);
      requireHistoryLength("WAD open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WAD open", "inClose", inClose.length, inHigh.length);
      return wadOpenInternal(inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#wadOpen} that also fills the output array(s) bit-identically
    * to {@link Core#WAD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link WadStream#outRange()}.
    */
   public WadStream wadOpenAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] )
   {
      requireArgument("WAD openAndFill", "inHigh", inHigh);
      requireHistory("WAD openAndFill", inHigh.length);
      requireArgument("WAD openAndFill", "inLow", inLow);
      requireArgument("WAD openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("WAD openAndFill", inHigh.length, WAD_Lookback());
      requireHistoryLength("WAD openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("WAD openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("WAD openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("WAD openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return wadOpenAndFillInternal(inHigh, inLow, inClose, 0, outBegIdx, outNBElement, outReal);
   }
