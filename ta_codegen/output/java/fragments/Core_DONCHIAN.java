/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #342).
 *  090326 MF     Drop optInLag; the window ends at the current bar.
 */

   /**
    * Number of leading input bars {@link Core#DONCHIAN} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the extrema window (default 20;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int DONCHIAN_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode DONCHIAN_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      /* Donchian Channels over the optInTimePeriod bars ending at the current
       * bar:
       *
       *    Upper  = Highest High of the window
       *    Lower  = Lowest  Low  of the window
       *    Middle = (Upper + Lower) / 2
       *
       * The window includes the current bar, matching every other library and
       * charting platform. A breakout rule compares the current bar against the
       * PREVIOUS bar's band, which is where the one-bar offset belongs.
       *
       * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
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
       * output to be the same buffer: every position written (outIdx) sits
       * at or below trailingIdx, the oldest position any later bar reads.
       *
       * The highest high and lowest low of the window are cached with their
       * indices; the window is rescanned only when a cached extremum drops
       * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
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
         outRealUpperBand[outIdx] = highest;
         outRealLowerBand[outIdx] = lowest;
         outRealMiddleBand[outIdx] = (highest + lowest) / 2.0;
         outIdx += 1;
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
   RetCode DONCHIAN_Impl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInTimePeriod,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
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
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
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
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = (double)inHigh[today];
         tmpLow = (double)inLow[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = (double)inHigh[i];
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
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = (double)inLow[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         outRealUpperBand[outIdx] = highest;
         outRealLowerBand[outIdx] = lowest;
         outRealMiddleBand[outIdx] = (highest + lowest) / 2.0;
         outIdx += 1;
         trailingIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Donchian Channels: three overlap lines built from rolling price extrema.
    * The upper band is the highest high and the lower band the lowest low over
    * the period; the middle line is their midpoint. Richard Donchian's original
    * four-week rule — generally credited as the first published systematic
    * trend-following system — buys a break above the high of the preceding
    * weeks and sells a break below their low.
    * <p><b>Formula</b>
    * <pre>{@code
    * Window = the optInTimePeriod bars ending at the current bar
    * Upper  = Highest High of Window
    * Lower  = Lowest  Low  of Window
    * Middle = (Upper + Lower) / 2
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The window includes the current bar, matching TradingView ({@code ta.highest}/{@code ta.lowest}), NinjaTrader, ta4j, pandas-ta and every other library that ships Donchian Channels.</li>
    * <li>A breakout rule compares the current bar against the **previous** bar's band — {@code High[t] &gt; Upper[t-1]} — which is where the one-bar offset belongs. Reading {@code Upper[t]} against {@code High[t]} can never signal, because {@code High[t]} is inside the window that produced it.</li>
    * <li>Upper, Middle and Lower are bit-identical to {@code MAX(high, N)}, {@code MIDPRICE(N)} and {@code MIN(low, N)}. DONCHIAN computes all three in one pass under the name users look for.</li>
    * <li>The middle line is the channel midpoint, not a moving average of price.</li>
    * <li>No smoothing or recursion is involved, so there is no unstable period: outputs are exact from the first bar.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#DONCHIAN_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Number of bars in the extrema window (default 20;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outRealUpperBand Highest high of the window. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outRealMiddleBand Midpoint of the upper and lower bands. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
    * @param outRealLowerBand Lowest low of the window. Must hold at least
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
    */
   public OutRange DONCHIAN( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             int optInTimePeriod,
                             double outRealUpperBand[],
                             double outRealMiddleBand[],
                             double outRealLowerBand[] )
   {
      requireIndexRange("DONCHIAN", startIdx, endIdx);
      int guardStart = clampedStart("DONCHIAN", startIdx, DONCHIAN_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DONCHIAN", "inHigh", inHigh, guardInLen);
      requireLength("DONCHIAN", "inLow", inLow, guardInLen);
      requireLength("DONCHIAN", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("DONCHIAN", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("DONCHIAN", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DONCHIAN_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("DONCHIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Donchian Channels: three overlap lines built from rolling price extrema.
    * The upper band is the highest high and the lower band the lowest low over
    * the period; the middle line is their midpoint. Richard Donchian's original
    * four-week rule — generally credited as the first published systematic
    * trend-following system — buys a break above the high of the preceding
    * weeks and sells a break below their low.
    * <p><b>Formula</b>
    * <pre>{@code
    * Window = the optInTimePeriod bars ending at the current bar
    * Upper  = Highest High of Window
    * Lower  = Lowest  Low  of Window
    * Middle = (Upper + Lower) / 2
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The window includes the current bar, matching TradingView ({@code ta.highest}/{@code ta.lowest}), NinjaTrader, ta4j, pandas-ta and every other library that ships Donchian Channels.</li>
    * <li>A breakout rule compares the current bar against the **previous** bar's band — {@code High[t] &gt; Upper[t-1]} — which is where the one-bar offset belongs. Reading {@code Upper[t]} against {@code High[t]} can never signal, because {@code High[t]} is inside the window that produced it.</li>
    * <li>Upper, Middle and Lower are bit-identical to {@code MAX(high, N)}, {@code MIDPRICE(N)} and {@code MIN(low, N)}. DONCHIAN computes all three in one pass under the name users look for.</li>
    * <li>The middle line is the channel midpoint, not a moving average of price.</li>
    * <li>No smoothing or recursion is involved, so there is no unstable period: outputs are exact from the first bar.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#DONCHIAN_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Number of bars in the extrema window (default 20;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outRealUpperBand Highest high of the window. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outRealMiddleBand Midpoint of the upper and lower bands. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
    * @param outRealLowerBand Lowest low of the window. Must hold at least
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
    */
   public OutRange DONCHIAN( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             int optInTimePeriod,
                             double outRealUpperBand[],
                             double outRealMiddleBand[],
                             double outRealLowerBand[] )
   {
      requireIndexRange("DONCHIAN", startIdx, endIdx);
      int guardStart = clampedStart("DONCHIAN", startIdx, DONCHIAN_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DONCHIAN", "inHigh", inHigh, guardInLen);
      requireLength("DONCHIAN", "inLow", inLow, guardInLen);
      requireLength("DONCHIAN", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("DONCHIAN", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("DONCHIAN", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DONCHIAN_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("DONCHIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live DONCHIAN stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#DONCHIAN} over the same series.
    * Open with {@link Core#donchianOpen}; there is no close — the handle is
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
   public static final class DonchianStream {
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
      double cur_outRealUpperBand;
      double cur_outRealMiddleBand;
      double cur_outRealLowerBand;
      int outRangeBegIdx;
      int outRangeCount;

      DonchianStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#DONCHIAN} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      DonchianStream( DonchianStream other ) {
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
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(DonchianOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, DonchianOut out ) {
         requireArgument("DONCHIAN update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("DONCHIAN update: BadParam", RetCode.BadParam);
         }
         core.donchianStepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
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
      public void updateAndFill( double inHigh[], double inLow[], double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] ) {
         requireArgument("DONCHIAN updateAndFill", "inHigh", inHigh);
         requireArgument("DONCHIAN updateAndFill", "inLow", inLow);
         requireArgument("DONCHIAN updateAndFill", "outRealUpperBand", outRealUpperBand);
         requireArgument("DONCHIAN updateAndFill", "outRealMiddleBand", outRealMiddleBand);
         requireArgument("DONCHIAN updateAndFill", "outRealLowerBand", outRealLowerBand);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outRealUpperBand.length < barCount || outRealMiddleBand.length < barCount || outRealLowerBand.length < barCount || (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand )
            throw new TaLibArgumentException("DONCHIAN updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("DONCHIAN updateAndFill: BadParam", RetCode.BadParam);
            }
            core.donchianStepImpl(this, inHigh[i], inLow[i]);
            outRealUpperBand[i] = this.cur_outRealUpperBand;
            outRealMiddleBand[i] = this.cur_outRealMiddleBand;
            outRealLowerBand[i] = this.cur_outRealLowerBand;
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
      public void peek( double inHigh, double inLow, DonchianOut out ) {
         requireArgument("DONCHIAN peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("DONCHIAN peek: BadParam", RetCode.BadParam);
         DonchianStream sp = this;
         double tmpLow = 0.0;
         double tmpHigh = 0.0;
         double cur_outRealLowerBand = sp.cur_outRealLowerBand;
         double cur_outRealMiddleBand = sp.cur_outRealMiddleBand;
         double cur_outRealUpperBand = sp.cur_outRealUpperBand;
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
         tmpHigh = ((today & sp.xMask) != pkSlot0) ? sp.x_inHigh[today & sp.xMask] : pkVal0;
         tmpLow = ((today & sp.xMask) != pkSlot1) ? sp.x_inLow[today & sp.xMask] : pkVal1;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= today ) {
               tmpHigh = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
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
            lowest = ((lowestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
            i = lowestIdx;
            while( ++i <= today ) {
               tmpLow = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = today;
            lowest = tmpLow;
         }
         cur_outRealUpperBand = highest;
         cur_outRealLowerBand = lowest;
         cur_outRealMiddleBand = (highest + lowest) / 2.0;
         out.realUpperBand = cur_outRealUpperBand;
         out.realMiddleBand = cur_outRealMiddleBand;
         out.realLowerBand = cur_outRealLowerBand;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( DonchianOut out ) {
         requireArgument("DONCHIAN value", "out", out);
         out.realUpperBand = this.cur_outRealUpperBand;
         out.realMiddleBand = this.cur_outRealMiddleBand;
         out.realLowerBand = this.cur_outRealLowerBand;
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
      public DonchianStream clone() {
         return new DonchianStream(this);
      }
   }

   /**
    * The outputs of one DONCHIAN bar, written by the stream into an object the
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
   public static final class DonchianOut {
      /** Highest high of the window. */
      public double realUpperBand;
      /** Midpoint of the upper and lower bands. */
      public double realMiddleBand;
      /** Lowest low of the window. */
      public double realLowerBand;
   }
   void donchianStepImpl( DonchianStream sp, double inHigh, double inLow )
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
      sp.cur_outRealUpperBand = sp.highest;
      sp.cur_outRealLowerBand = sp.lowest;
      sp.cur_outRealMiddleBand = (sp.highest + sp.lowest) / 2.0;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode donchianOpenImpl( DonchianStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[], int outStride )
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
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      /* Donchian Channels over the optInTimePeriod bars ending at the current
       * bar:
       *
       *    Upper  = Highest High of the window
       *    Lower  = Lowest  Low  of the window
       *    Middle = (Upper + Lower) / 2
       *
       * The window includes the current bar, matching every other library and
       * charting platform. A breakout rule compares the current bar against the
       * PREVIOUS bar's band, which is where the one-bar offset belongs.
       *
       * Upper/Middle/Lower are bit-identical to MAX(high,N)/MIDPRICE(N)/MIN(low,N).
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
       * output to be the same buffer: every position written (outIdx) sits
       * at or below trailingIdx, the oldest position any later bar reads.
       *
       * The highest high and lowest low of the window are cached with their
       * indices; the window is rescanned only when a cached extremum drops
       * out of it (same approach as MIN/MAX/WILLR and MIDPRICE).
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
         outRealUpperBand[outIdx * outStride] = highest;
         outRealLowerBand[outIdx * outStride] = lowest;
         outRealMiddleBand[outIdx * outStride] = (highest + lowest) / 2.0;
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
      sp.cur_outRealUpperBand = outRealUpperBand[(outNBElement.value - 1) * outStride];
      sp.cur_outRealMiddleBand = outRealMiddleBand[(outNBElement.value - 1) * outStride];
      sp.cur_outRealLowerBand = outRealLowerBand[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* donchianOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   DonchianStream donchianOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      DonchianStream sp = new DonchianStream(this);
      RetCode retCode = donchianOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("DONCHIAN openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("DONCHIAN openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("DONCHIAN openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind donchianOpen (composition seam). */
   DonchianStream donchianOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      DonchianStream sp = new DonchianStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outRealUpperBand = new double[1];
      double[] sink_outRealMiddleBand = new double[1];
      double[] sink_outRealLowerBand = new double[1];
      RetCode retCode = donchianOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outRealUpperBand, sink_outRealMiddleBand, sink_outRealLowerBand, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("DONCHIAN open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("DONCHIAN open: internal error", retCode);
      }
      throw new TaLibArgumentException("DONCHIAN open: " + retCode, retCode);
   }
   /**
    * Open a live DONCHIAN stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#DONCHIAN} at that bar.
    * <p>The history must hold at least {@code DONCHIAN_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public DonchianStream donchianOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("DONCHIAN open", "inHigh", inHigh);
      requireHistory("DONCHIAN open", inHigh.length);
      requireArgument("DONCHIAN open", "inLow", inLow);
      requireHistoryLength("DONCHIAN open", "inLow", inLow.length, inHigh.length);
      return donchianOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#donchianOpen} that also fills the output array(s) bit-identically
    * to {@link Core#DONCHIAN} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link DonchianStream#outRange()}.
    */
   public DonchianStream donchianOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      requireArgument("DONCHIAN openAndFill", "inHigh", inHigh);
      requireHistory("DONCHIAN openAndFill", inHigh.length);
      requireArgument("DONCHIAN openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("DONCHIAN openAndFill", inHigh.length, DONCHIAN_Lookback(optInTimePeriod));
      requireHistoryLength("DONCHIAN openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("DONCHIAN openAndFill", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("DONCHIAN openAndFill", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("DONCHIAN openAndFill", "outRealLowerBand", outRealLowerBand, guardOutLen);
      if( (Object)outRealUpperBand == (Object)inHigh || (Object)outRealUpperBand == (Object)inLow || (Object)outRealMiddleBand == (Object)inHigh || (Object)outRealMiddleBand == (Object)inLow || (Object)outRealLowerBand == (Object)inHigh || (Object)outRealLowerBand == (Object)inLow || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand ) {
         throw new TaLibArgumentException("DONCHIAN openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return donchianOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
   }
