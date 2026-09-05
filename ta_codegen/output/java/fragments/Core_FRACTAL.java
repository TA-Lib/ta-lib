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
 *  090526 MF,CC  Initial version (#371).
 */

   /**
    * Number of leading input bars {@link Core#FRACTAL} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInLeftBars Bars before the pivot that it must strictly dominate
    *        (default 2; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRightBars Bars after the pivot that it must strictly dominate,
    *        and the delay before the verdict is reported (default 2; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int FRACTAL_Lookback( int optInLeftBars, int optInRightBars )
   {
      if( optInLeftBars == Integer.MIN_VALUE ) {
         optInLeftBars = 2;
      } else if( optInLeftBars < 1 || optInLeftBars > 100000 ) {
         return -1;
      }
      if( optInRightBars == Integer.MIN_VALUE ) {
         optInRightBars = 2;
      } else if( optInRightBars < 1 || optInRightBars > 100000 ) {
         return -1;
      }
      return optInLeftBars + optInRightBars ;

   }
   RetCode FRACTAL_Impl( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         int optInLeftBars,
                         int optInRightBars,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         int outSwingHigh[],
                         int outSwingLow[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double pivotHigh = 0;
      double pivotLow = 0;
      double otherHigh = 0;
      double otherLow = 0;
      double tempHigh = 0;
      double tempLow = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInLeftBars == Integer.MIN_VALUE ) {
         optInLeftBars = 2;
      } else if( optInLeftBars < 1 || optInLeftBars > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInRightBars == Integer.MIN_VALUE ) {
         optInRightBars = 2;
      } else if( optInRightBars < 1 || optInRightBars > 100000 ) {
         return RetCode.BadParam;
      }
      if( outSwingHigh == outSwingLow ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = FRACTAL_Lookback(optInLeftBars, optInRightBars);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         pivotHigh = 0.0;
         pivotLow = 0.0;
         otherHigh = 0.0;
         otherLow = 0.0;
         for( i = optInLeftBars + optInRightBars; i >= 0; i -= 1 ) {
            tempHigh = inHigh[today - i];
            tempLow = inLow[today - i];
            if( i == optInRightBars ) {
               pivotHigh = tempHigh;
               pivotLow = tempLow;
            } else if( i == optInLeftBars + optInRightBars ) {
               /* optInLeftBars is at least 1, so the oldest bar of the window is
                * never the pivot and always seeds the rest-of-window extrema.
                */
               otherHigh = tempHigh;
               otherLow = tempLow;
            } else {
               if( tempHigh > otherHigh ) {
                  otherHigh = tempHigh;
               }
               if( tempLow < otherLow ) {
                  otherLow = tempLow;
               }
            }
         }
         if( pivotHigh > otherHigh ) {
            outSwingHigh[outIdx] = 100;
         } else {
            outSwingHigh[outIdx] = 0;
         }
         if( pivotLow < otherLow ) {
            outSwingLow[outIdx] = 100;
         } else {
            outSwingLow[outIdx] = 0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode FRACTAL_Impl( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         int optInLeftBars,
                         int optInRightBars,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         int outSwingHigh[],
                         int outSwingLow[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double pivotHigh = 0;
      double pivotLow = 0;
      double otherHigh = 0;
      double otherLow = 0;
      double tempHigh = 0;
      double tempLow = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInLeftBars == Integer.MIN_VALUE ) {
         optInLeftBars = 2;
      } else if( optInLeftBars < 1 || optInLeftBars > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInRightBars == Integer.MIN_VALUE ) {
         optInRightBars = 2;
      } else if( optInRightBars < 1 || optInRightBars > 100000 ) {
         return RetCode.BadParam;
      }
      if( outSwingHigh == outSwingLow ) {
         return RetCode.BadParam ;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = FRACTAL_Lookback(optInLeftBars, optInRightBars);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         pivotHigh = 0.0;
         pivotLow = 0.0;
         otherHigh = 0.0;
         otherLow = 0.0;
         for( i = optInLeftBars + optInRightBars; i >= 0; i -= 1 ) {
            tempHigh = (double)inHigh[today - i];
            tempLow = (double)inLow[today - i];
            if( i == optInRightBars ) {
               pivotHigh = tempHigh;
               pivotLow = tempLow;
            } else if( i == optInLeftBars + optInRightBars ) {
               otherHigh = tempHigh;
               otherLow = tempLow;
            } else {
               if( tempHigh > otherHigh ) {
                  otherHigh = tempHigh;
               }
               if( tempLow < otherLow ) {
                  otherLow = tempLow;
               }
            }
         }
         if( pivotHigh > otherHigh ) {
            outSwingHigh[outIdx] = 100;
         } else {
            outSwingHigh[outIdx] = 0;
         }
         if( pivotLow < otherLow ) {
            outSwingLow[outIdx] = 100;
         } else {
            outSwingLow[outIdx] = 0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Williams Fractal: a causal swing-pivot detector. A bar is a swing high
    * when its high strictly exceeds the highs of the {@code optInLeftBars} bars
    * before it and the {@code optInRightBars} bars after it; a swing low is the
    * mirror on the lows. Bill Williams' original is the symmetric five-candle
    * case; independent left and right arms generalise it. The right arm cannot
    * be known until it has closed, so the verdict is reported on the
    * confirmation bar, {@code optInRightBars} bars after the pivot itself. Each
    * output value therefore describes the bar {@code optInRightBars} back, not
    * the bar it is written at: a flag at output index {@code k} names input bar
    * {@code outBegIdx + k - optInRightBars}, whose price is {@code inHigh[...]}
    * / {@code inLow[...]} at that index. The two outputs are independent flags
    * rather than one signed value, because an outside bar can be a swing high
    * and a swing low at once.
    * <p><b>Formula</b>
    * <pre>{@code
    * With `L = optInLeftBars`, `R = optInRightBars` and pivot `c = i - R`:
    * swingHigh(i) = 100 if High[c] > High[j] for every j in [c-L, c+R] other than c, else 0.
    * swingLow(i) = 100 if Low[c] < Low[j] for every j in [c-L, c+R] other than c, else 0.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Strict on both sides: a bar tied with any other bar of its window is not a pivot. TradingView's Pine runtime differs — its {@code ta.pivothigh} / {@code ta.pivotlow} let a tie with an older bar stand and let a tie with a newer bar cancel, i.e. non-strict left and strict right — so a plateau Pine reports as a pivot is not one here.</li>
    * <li>Each output is decided on its own side: a high tied with any other high in the window forces {@code outSwingHigh} to 0 while leaving {@code outSwingLow} free to fire 100, and the mirror holds. Only a window flat in both series emits 0 on both.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#FRACTAL_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param optInLeftBars Bars before the pivot that it must strictly dominate
    *        (default 2; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRightBars Bars after the pivot that it must strictly dominate,
    *        and the delay before the verdict is reported (default 2; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outSwingHigh 100 when the bar {@code optInRightBars} back is a
    *        strict swing high, 0 otherwise. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSwingLow 100 when the bar {@code optInRightBars} back is a
    *        strict swing low, 0 otherwise. Must hold at least
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
    * @see Core#AROON
    * @see Core#MAXINDEX
    * @see Core#MININDEX
    * @see Core#MINMAXINDEX
    */
   public OutRange FRACTAL( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            int optInLeftBars,
                            int optInRightBars,
                            int outSwingHigh[],
                            int outSwingLow[] )
   {
      requireIndexRange("FRACTAL", startIdx, endIdx);
      int guardStart = clampedStart("FRACTAL", startIdx, FRACTAL_Lookback(optInLeftBars, optInRightBars));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FRACTAL", "inHigh", inHigh, guardInLen);
      requireLength("FRACTAL", "inLow", inLow, guardInLen);
      requireLength("FRACTAL", "outSwingHigh", outSwingHigh, guardOutLen);
      requireLength("FRACTAL", "outSwingLow", outSwingLow, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = FRACTAL_Impl(startIdx, endIdx, inHigh, inLow, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow);
      if( retCode != RetCode.Success ) {
         throw failure("FRACTAL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Williams Fractal: a causal swing-pivot detector. A bar is a swing high
    * when its high strictly exceeds the highs of the {@code optInLeftBars} bars
    * before it and the {@code optInRightBars} bars after it; a swing low is the
    * mirror on the lows. Bill Williams' original is the symmetric five-candle
    * case; independent left and right arms generalise it. The right arm cannot
    * be known until it has closed, so the verdict is reported on the
    * confirmation bar, {@code optInRightBars} bars after the pivot itself. Each
    * output value therefore describes the bar {@code optInRightBars} back, not
    * the bar it is written at: a flag at output index {@code k} names input bar
    * {@code outBegIdx + k - optInRightBars}, whose price is {@code inHigh[...]}
    * / {@code inLow[...]} at that index. The two outputs are independent flags
    * rather than one signed value, because an outside bar can be a swing high
    * and a swing low at once.
    * <p><b>Formula</b>
    * <pre>{@code
    * With `L = optInLeftBars`, `R = optInRightBars` and pivot `c = i - R`:
    * swingHigh(i) = 100 if High[c] > High[j] for every j in [c-L, c+R] other than c, else 0.
    * swingLow(i) = 100 if Low[c] < Low[j] for every j in [c-L, c+R] other than c, else 0.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Strict on both sides: a bar tied with any other bar of its window is not a pivot. TradingView's Pine runtime differs — its {@code ta.pivothigh} / {@code ta.pivotlow} let a tie with an older bar stand and let a tie with a newer bar cancel, i.e. non-strict left and strict right — so a plateau Pine reports as a pivot is not one here.</li>
    * <li>Each output is decided on its own side: a high tied with any other high in the window forces {@code outSwingHigh} to 0 while leaving {@code outSwingLow} free to fire 100, and the mirror holds. Only a window flat in both series emits 0 on both.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#FRACTAL_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param optInLeftBars Bars before the pivot that it must strictly dominate
    *        (default 2; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInRightBars Bars after the pivot that it must strictly dominate,
    *        and the delay before the verdict is reported (default 2; range 1..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param outSwingHigh 100 when the bar {@code optInRightBars} back is a
    *        strict swing high, 0 otherwise. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSwingLow 100 when the bar {@code optInRightBars} back is a
    *        strict swing low, 0 otherwise. Must hold at least
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
    * @see Core#AROON
    * @see Core#MAXINDEX
    * @see Core#MININDEX
    * @see Core#MINMAXINDEX
    */
   public OutRange FRACTAL( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            int optInLeftBars,
                            int optInRightBars,
                            int outSwingHigh[],
                            int outSwingLow[] )
   {
      requireIndexRange("FRACTAL", startIdx, endIdx);
      int guardStart = clampedStart("FRACTAL", startIdx, FRACTAL_Lookback(optInLeftBars, optInRightBars));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FRACTAL", "inHigh", inHigh, guardInLen);
      requireLength("FRACTAL", "inLow", inLow, guardInLen);
      requireLength("FRACTAL", "outSwingHigh", outSwingHigh, guardOutLen);
      requireLength("FRACTAL", "outSwingLow", outSwingLow, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = FRACTAL_Impl(startIdx, endIdx, inHigh, inLow, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow);
      if( retCode != RetCode.Success ) {
         throw failure("FRACTAL", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live FRACTAL stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#FRACTAL} over the same series.
    * Open with {@link Core#fractalOpen}; there is no close — the handle is
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
   public static final class FractalStream {
      Core core;
      int optInLeftBars;
      int optInRightBars;
      int winPos_i;
      int winCap_i;
      double[] win_i_inHigh;
      double[] win_i_inLow;
      int cur_outSwingHigh;
      int cur_outSwingLow;
      int outRangeBegIdx;
      int outRangeCount;

      FractalStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#FRACTAL} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      FractalStream( FractalStream other ) {
         this.core = other.core;
         this.optInLeftBars = other.optInLeftBars;
         this.optInRightBars = other.optInRightBars;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inHigh = other.win_i_inHigh.clone();
         this.win_i_inLow = other.win_i_inLow.clone();
         this.cur_outSwingHigh = other.cur_outSwingHigh;
         this.cur_outSwingLow = other.cur_outSwingLow;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(FractalOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, FractalOut out ) {
         requireArgument("FRACTAL update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("FRACTAL update: BadParam", RetCode.BadParam);
         }
         core.fractalStepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.swingHigh = this.cur_outSwingHigh;
         out.swingLow = this.cur_outSwingLow;
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
      public void updateAndFill( double inHigh[], double inLow[], int outSwingHigh[], int outSwingLow[] ) {
         requireArgument("FRACTAL updateAndFill", "inHigh", inHigh);
         requireArgument("FRACTAL updateAndFill", "inLow", inLow);
         requireArgument("FRACTAL updateAndFill", "outSwingHigh", outSwingHigh);
         requireArgument("FRACTAL updateAndFill", "outSwingLow", outSwingLow);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outSwingHigh.length < barCount || outSwingLow.length < barCount || (Object)outSwingHigh == (Object)inHigh || (Object)outSwingHigh == (Object)inLow || (Object)outSwingLow == (Object)inHigh || (Object)outSwingLow == (Object)inLow || (Object)outSwingHigh == (Object)outSwingLow )
            throw new TaLibArgumentException("FRACTAL updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("FRACTAL updateAndFill: BadParam", RetCode.BadParam);
            }
            core.fractalStepImpl(this, inHigh[i], inLow[i]);
            outSwingHigh[i] = this.cur_outSwingHigh;
            outSwingLow[i] = this.cur_outSwingLow;
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
      public void peek( double inHigh, double inLow, FractalOut out ) {
         requireArgument("FRACTAL peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("FRACTAL peek: BadParam", RetCode.BadParam);
         FractalStream sp = this;
         int i = 0;
         double pivotHigh = 0.0;
         double pivotLow = 0.0;
         double otherHigh = 0.0;
         double otherLow = 0.0;
         double tempHigh = 0.0;
         double tempLow = 0.0;
         int cur_outSwingHigh = 0;
         int cur_outSwingLow = 0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         pkSlot0 = sp.winPos_i;
         pkVal0 = inHigh;
         pkSlot1 = sp.winPos_i;
         pkVal1 = inLow;
         pivotHigh = 0.0;
         pivotLow = 0.0;
         otherHigh = 0.0;
         otherLow = 0.0;
         for( i = sp.optInLeftBars + sp.optInRightBars; i >= 0; i -= 1 ) {
            tempHigh = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
            tempLow = (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot1) ? sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal1;
            if( i == sp.optInRightBars ) {
               pivotHigh = tempHigh;
               pivotLow = tempLow;
            } else if( i == sp.optInLeftBars + sp.optInRightBars ) {
               /* optInLeftBars is at least 1, so the oldest bar of the window is
                * never the pivot and always seeds the rest-of-window extrema.
                */
               otherHigh = tempHigh;
               otherLow = tempLow;
            } else {
               if( tempHigh > otherHigh ) {
                  otherHigh = tempHigh;
               }
               if( tempLow < otherLow ) {
                  otherLow = tempLow;
               }
            }
         }
         if( pivotHigh > otherHigh ) {
            cur_outSwingHigh = 100;
         } else {
            cur_outSwingHigh = 0;
         }
         if( pivotLow < otherLow ) {
            cur_outSwingLow = 100;
         } else {
            cur_outSwingLow = 0;
         }
         out.swingHigh = cur_outSwingHigh;
         out.swingLow = cur_outSwingLow;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( FractalOut out ) {
         requireArgument("FRACTAL value", "out", out);
         out.swingHigh = this.cur_outSwingHigh;
         out.swingLow = this.cur_outSwingLow;
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
      public FractalStream clone() {
         return new FractalStream(this);
      }
   }

   /**
    * The outputs of one FRACTAL bar, written by the stream into an object the
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
   public static final class FractalOut {
      /** 100 when the bar {@code optInRightBars} back is a strict swing high, 0 otherwise. */
      public int swingHigh;
      /** 100 when the bar {@code optInRightBars} back is a strict swing low, 0 otherwise. */
      public int swingLow;
   }
   void fractalStepImpl( FractalStream sp, double inHigh, double inLow )
   {
      int i = 0;
      double pivotHigh = 0.0;
      double pivotLow = 0.0;
      double otherHigh = 0.0;
      double otherLow = 0.0;
      double tempHigh = 0.0;
      double tempLow = 0.0;
      sp.win_i_inHigh[sp.winPos_i] = inHigh;
      sp.win_i_inLow[sp.winPos_i] = inLow;
      pivotHigh = 0.0;
      pivotLow = 0.0;
      otherHigh = 0.0;
      otherLow = 0.0;
      for( i = sp.optInLeftBars + sp.optInRightBars; i >= 0; i -= 1 ) {
         tempHigh = sp.win_i_inHigh[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         tempLow = sp.win_i_inLow[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         if( i == sp.optInRightBars ) {
            pivotHigh = tempHigh;
            pivotLow = tempLow;
         } else if( i == sp.optInLeftBars + sp.optInRightBars ) {
            /* optInLeftBars is at least 1, so the oldest bar of the window is
             * never the pivot and always seeds the rest-of-window extrema.
             */
            otherHigh = tempHigh;
            otherLow = tempLow;
         } else {
            if( tempHigh > otherHigh ) {
               otherHigh = tempHigh;
            }
            if( tempLow < otherLow ) {
               otherLow = tempLow;
            }
         }
      }
      if( pivotHigh > otherHigh ) {
         sp.cur_outSwingHigh = 100;
      } else {
         sp.cur_outSwingHigh = 0;
      }
      if( pivotLow < otherLow ) {
         sp.cur_outSwingLow = 100;
      } else {
         sp.cur_outSwingLow = 0;
      }
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode fractalOpenImpl( FractalStream sp, double inHigh[], double inLow[], int startIdx, int optInLeftBars, int optInRightBars, MInteger outBegIdx, MInteger outNBElement, int outSwingHigh[], int outSwingLow[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double pivotHigh = 0;
      double pivotLow = 0;
      double otherHigh = 0;
      double otherLow = 0;
      double tempHigh = 0;
      double tempLow = 0;
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
      if( optInLeftBars == Integer.MIN_VALUE ) {
         optInLeftBars = 2;
      } else if( optInLeftBars < 1 || optInLeftBars > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInRightBars == Integer.MIN_VALUE ) {
         optInRightBars = 2;
      } else if( optInRightBars < 1 || optInRightBars > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = FRACTAL_Lookback(optInLeftBars, optInRightBars);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         pivotHigh = 0.0;
         pivotLow = 0.0;
         otherHigh = 0.0;
         otherLow = 0.0;
         for( i = optInLeftBars + optInRightBars; i >= 0; i -= 1 ) {
            tempHigh = inHigh[today - i];
            tempLow = inLow[today - i];
            if( i == optInRightBars ) {
               pivotHigh = tempHigh;
               pivotLow = tempLow;
            } else if( i == optInLeftBars + optInRightBars ) {
               /* optInLeftBars is at least 1, so the oldest bar of the window is
                * never the pivot and always seeds the rest-of-window extrema.
                */
               otherHigh = tempHigh;
               otherLow = tempLow;
            } else {
               if( tempHigh > otherHigh ) {
                  otherHigh = tempHigh;
               }
               if( tempLow < otherLow ) {
                  otherLow = tempLow;
               }
            }
         }
         if( pivotHigh > otherHigh ) {
            outSwingHigh[outIdx * outStride] = 100;
         } else {
            outSwingHigh[outIdx * outStride] = 0;
         }
         if( pivotLow < otherLow ) {
            outSwingLow[outIdx * outStride] = 100;
         } else {
            outSwingLow[outIdx * outStride] = 0;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInLeftBars + optInRightBars + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capWin_i_inHigh = new double[cap_i];
      System.arraycopy(inHigh, historyLen - cap_i, capWin_i_inHigh, 0, cap_i);
      double[] capWin_i_inLow = new double[cap_i];
      System.arraycopy(inLow, historyLen - cap_i, capWin_i_inLow, 0, cap_i);
      sp.optInLeftBars = optInLeftBars;
      sp.optInRightBars = optInRightBars;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inHigh = capWin_i_inHigh;
      sp.win_i_inLow = capWin_i_inLow;
      sp.cur_outSwingHigh = outSwingHigh[(outNBElement.value - 1) * outStride];
      sp.cur_outSwingLow = outSwingLow[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* fractalOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   FractalStream fractalOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInLeftBars, int optInRightBars, MInteger outBegIdx, MInteger outNBElement, int outSwingHigh[], int outSwingLow[] )
   {
      FractalStream sp = new FractalStream(this);
      RetCode retCode = fractalOpenImpl(sp, inHigh, inLow, startIdx, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("FRACTAL openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("FRACTAL openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("FRACTAL openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind fractalOpen (composition seam). */
   FractalStream fractalOpenInternal( double inHigh[], double inLow[], int startIdx, int optInLeftBars, int optInRightBars )
   {
      FractalStream sp = new FractalStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int[] sink_outSwingHigh = new int[1];
      int[] sink_outSwingLow = new int[1];
      RetCode retCode = fractalOpenImpl(sp, inHigh, inLow, startIdx, optInLeftBars, optInRightBars, outBegIdx, outNBElement, sink_outSwingHigh, sink_outSwingLow, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("FRACTAL open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("FRACTAL open: internal error", retCode);
      }
      throw new TaLibArgumentException("FRACTAL open: " + retCode, retCode);
   }
   /**
    * Open a live FRACTAL stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#FRACTAL} at that bar.
    * <p>The history must hold at least {@code FRACTAL_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public FractalStream fractalOpen( double inHigh[], double inLow[], int optInLeftBars, int optInRightBars )
   {
      requireArgument("FRACTAL open", "inHigh", inHigh);
      requireHistory("FRACTAL open", inHigh.length);
      requireArgument("FRACTAL open", "inLow", inLow);
      requireHistoryLength("FRACTAL open", "inLow", inLow.length, inHigh.length);
      return fractalOpenInternal(inHigh, inLow, 0, optInLeftBars, optInRightBars);
   }
   /**
    * {@link Core#fractalOpen} that also fills the output array(s) bit-identically
    * to {@link Core#FRACTAL} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link FractalStream#outRange()}.
    */
   public FractalStream fractalOpenAndFill( double inHigh[], double inLow[], int optInLeftBars, int optInRightBars, int outSwingHigh[], int outSwingLow[] )
   {
      requireArgument("FRACTAL openAndFill", "inHigh", inHigh);
      requireHistory("FRACTAL openAndFill", inHigh.length);
      requireArgument("FRACTAL openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("FRACTAL openAndFill", inHigh.length, FRACTAL_Lookback(optInLeftBars, optInRightBars));
      requireHistoryLength("FRACTAL openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("FRACTAL openAndFill", "outSwingHigh", outSwingHigh, guardOutLen);
      requireLength("FRACTAL openAndFill", "outSwingLow", outSwingLow, guardOutLen);
      if( (Object)outSwingHigh == (Object)inHigh || (Object)outSwingHigh == (Object)inLow || (Object)outSwingLow == (Object)inHigh || (Object)outSwingLow == (Object)inLow || (Object)outSwingHigh == (Object)outSwingLow ) {
         throw new TaLibArgumentException("FRACTAL openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return fractalOpenAndFillInternal(inHigh, inLow, 0, optInLeftBars, optInRightBars, outBegIdx, outNBElement, outSwingHigh, outSwingLow);
   }
