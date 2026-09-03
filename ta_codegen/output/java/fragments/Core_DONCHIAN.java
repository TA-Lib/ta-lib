/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  KL       Kevin Lin
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  090526 KL     First version (issue #342).
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
    * @param optInLag Bars the window is held back from the current bar (0
    *        includes the current bar) (default 1; range 0..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int DONCHIAN_Lookback( int optInTimePeriod, int optInLag )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInLag == Integer.MIN_VALUE ) {
         optInLag = 1;
      } else if( optInLag < 0 || optInLag > 100000 ) {
         return -1;
      }
      /* The window ends optInLag bars behind the output bar, so the first
       * bar with a full window behind it is (optInTimePeriod-1)+optInLag.
       */
      return optInTimePeriod - 1 + optInLag ;

   }
   RetCode DONCHIAN_Impl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInTimePeriod,
                          int optInLag,
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
      int winEnd = 0;
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
      if( optInLag == Integer.MIN_VALUE ) {
         optInLag = 1;
      } else if( optInLag < 0 || optInLag > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      /* Donchian Channels over the window [today-optInLag-optInTimePeriod+1 ..
       * today-optInLag]:
       *
       *    Upper  = Highest High of the window
       *    Lower  = Lowest  Low  of the window
       *    Middle = (Upper + Lower) / 2
       *
       * The default optInLag=1 is Donchian's original rule: the bar being
       * evaluated is measured against a window it is NOT part of, which is
       * what lets price cross the band. optInLag=0 includes the current bar
       * (the TradingView/NinjaTrader/pandas form), making Upper/Lower/Middle
       * exactly MAX(high,N)/MIN(low,N)/MIDPRICE(N).
       *
       * The middle line is the Donchian centerline, not a moving average:
       * at optInLag=0 it is bit-identical to MIDPRICE.
       */
      /* Identify the minimum number of price bar needed
       * to identify at least one output over the specified
       * period.
       */
      nbInitialElementNeeded = optInTimePeriod - 1 + optInLag;
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
       * out of it (same approach as MIN/MAX/WILLR and the MIDPRICE streaming
       * tier). The window is the one ending optInLag bars behind the output
       * bar, so the scan cursor is winEnd, not today.
       */
      outIdx = 0;
      today = startIdx;
      winEnd = today - optInLag;
      trailingIdx = winEnd - (optInTimePeriod - 1);
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = inHigh[winEnd];
         tmpLow = inLow[winEnd];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= winEnd ) {
               tmpHigh = inHigh[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = winEnd;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= winEnd ) {
               tmpLow = inLow[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = winEnd;
            lowest = tmpLow;
         }
         outRealUpperBand[outIdx] = highest;
         outRealLowerBand[outIdx] = lowest;
         outRealMiddleBand[outIdx] = (highest + lowest) / 2.0;
         outIdx += 1;
         trailingIdx += 1;
         winEnd += 1;
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
                          int optInLag,
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
      int winEnd = 0;
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
      if( optInLag == Integer.MIN_VALUE ) {
         optInLag = 1;
      } else if( optInLag < 0 || optInLag > 100000 ) {
         return RetCode.BadParam;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      nbInitialElementNeeded = optInTimePeriod - 1 + optInLag;
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
      winEnd = today - optInLag;
      trailingIdx = winEnd - (optInTimePeriod - 1);
      highestIdx = 0 - 1;
      highest = 0.0;
      lowestIdx = 0 - 1;
      lowest = 0.0;
      while( today <= endIdx ) {
         tmpHigh = (double)inHigh[winEnd];
         tmpLow = (double)inLow[winEnd];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= winEnd ) {
               tmpHigh = (double)inHigh[i];
               if( tmpHigh > highest ) {
                  highestIdx = i;
                  highest = tmpHigh;
               }
            }
         } else if( tmpHigh >= highest ) {
            highestIdx = winEnd;
            highest = tmpHigh;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= winEnd ) {
               tmpLow = (double)inLow[i];
               if( tmpLow < lowest ) {
                  lowestIdx = i;
                  lowest = tmpLow;
               }
            }
         } else if( tmpLow <= lowest ) {
            lowestIdx = winEnd;
            lowest = tmpLow;
         }
         outRealUpperBand[outIdx] = highest;
         outRealLowerBand[outIdx] = lowest;
         outRealMiddleBand[outIdx] = (highest + lowest) / 2.0;
         outIdx += 1;
         trailingIdx += 1;
         winEnd += 1;
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
    * Window = the optInTimePeriod bars ending optInLag bars before the current bar
    * Upper  = Highest High of Window
    * Lower  = Lowest  Low  of Window
    * Middle = (Upper + Lower) / 2
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The default {@code optInLag=1} is the original rule: a breakout is measured against a window the breaking bar is **not** part of. With the current bar inside the window ({@code optInLag=0}) the upper band can never be crossed upward — {@code High[t]} is already in the max — which is why StockCharts and IncredibleCharts both document the lagged form.</li>
    * <li>{@code optInLag=0} reproduces the inclusive convention used by TradingView ({@code ta.highest}/{@code ta.lowest}), NinjaTrader and pandas-ta. Users arriving from those platforms should pass {@code optInLag=0} to match their charts; the difference is exactly a one-bar shift.</li>
    * <li>At {@code optInLag=0} the three outputs equal {@code MAX(high, N)}, {@code MIN(low, N)} and {@code MIDPRICE(N)} bit for bit.</li>
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
    * @param optInLag Bars the window is held back from the current bar (0
    *        includes the current bar) (default 1; range 0..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
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
                             int optInLag,
                             double outRealUpperBand[],
                             double outRealMiddleBand[],
                             double outRealLowerBand[] )
   {
      requireIndexRange("DONCHIAN", startIdx, endIdx);
      int guardStart = clampedStart("DONCHIAN", startIdx, DONCHIAN_Lookback(optInTimePeriod, optInLag));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DONCHIAN", "inHigh", inHigh, guardInLen);
      requireLength("DONCHIAN", "inLow", inLow, guardInLen);
      requireLength("DONCHIAN", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("DONCHIAN", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("DONCHIAN", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DONCHIAN_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, optInLag, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
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
    * Window = the optInTimePeriod bars ending optInLag bars before the current bar
    * Upper  = Highest High of Window
    * Lower  = Lowest  Low  of Window
    * Middle = (Upper + Lower) / 2
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The default {@code optInLag=1} is the original rule: a breakout is measured against a window the breaking bar is **not** part of. With the current bar inside the window ({@code optInLag=0}) the upper band can never be crossed upward — {@code High[t]} is already in the max — which is why StockCharts and IncredibleCharts both document the lagged form.</li>
    * <li>{@code optInLag=0} reproduces the inclusive convention used by TradingView ({@code ta.highest}/{@code ta.lowest}), NinjaTrader and pandas-ta. Users arriving from those platforms should pass {@code optInLag=0} to match their charts; the difference is exactly a one-bar shift.</li>
    * <li>At {@code optInLag=0} the three outputs equal {@code MAX(high, N)}, {@code MIN(low, N)} and {@code MIDPRICE(N)} bit for bit.</li>
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
    * @param optInLag Bars the window is held back from the current bar (0
    *        includes the current bar) (default 1; range 0..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
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
                             int optInLag,
                             double outRealUpperBand[],
                             double outRealMiddleBand[],
                             double outRealLowerBand[] )
   {
      requireIndexRange("DONCHIAN", startIdx, endIdx);
      int guardStart = clampedStart("DONCHIAN", startIdx, DONCHIAN_Lookback(optInTimePeriod, optInLag));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DONCHIAN", "inHigh", inHigh, guardInLen);
      requireLength("DONCHIAN", "inLow", inLow, guardInLen);
      requireLength("DONCHIAN", "outRealUpperBand", outRealUpperBand, guardOutLen);
      requireLength("DONCHIAN", "outRealMiddleBand", outRealMiddleBand, guardOutLen);
      requireLength("DONCHIAN", "outRealLowerBand", outRealLowerBand, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = DONCHIAN_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, optInLag, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode != RetCode.Success ) {
         throw failure("DONCHIAN", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
