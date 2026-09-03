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
 *  090126 MF,CC  First version (issue #272).
 *  090326 MF,CC  #338 Two-coefficient Wilder step, in lockstep with TA_ATR.
 */

   /**
    * Number of leading input bars {@link Core#SUPERTREND} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Smoothing period of the Average True Range (default
    *        10; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to set
    *        the band width (default 3; minimum 0; {@code -4e37} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int SUPERTREND_Lookback( int optInTimePeriod, double optInMultiplier )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return -1;
      }
      /* Every output bar needs the Average True Range at the same bar, and nothing
       * else reaches further back, so the lookback is exactly the callee's. Never
       * restated here, which is what makes SUPERTREND inherit TA_FUNC_UNST_ATR.
       */
      return ATR_Lookback(optInTimePeriod) ;

   }
   RetCode SUPERTREND_Impl( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            double inClose[],
                            int optInTimePeriod,
                            double optInMultiplier,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[],
                            int outInteger[] )
   {
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int isUptrend = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      double medianPrice = 0;
      double band = 0;
      double basicUpper = 0;
      double basicLower = 0;
      double finalUpper = 0;
      double finalLower = 0;
      double closeToday = 0;
      double prevClose = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = SUPERTREND_Lookback(optInTimePeriod, optInMultiplier);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* The Average True Range is carried inline rather than taken from a call,
       * because the band and the ratchet advance together one bar at a time and a
       * whole-range buffer between them would not stream.
       *
       * The arithmetic order below is the bit-exactness contract with TA_ATR (do
       * not reorder): True Range from high-low, then the two previous-close
       * distances in that order; the seed summed from 0.0 over the first 'period'
       * True Ranges and divided once; the same two Wilder coefficients, wBeta
       * rounded and wAlpha derived from it, in one fused statement.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      /* Skip the Average True Range's unstable period. Taking the count from the
       * lookback rather than naming the setting keeps the two from disagreeing.
       */
      i = lookbackTotal - optInTimePeriod;
      while( i != 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
         i -= 1;
      }
      /* The first bar has no band to ratchet against and no trend to carry, so
       * both bands take their unclamped value and the trend is seeded long, as
       * ta4j's SuperTrendIndicator does. The formula does not settle this and the
       * published implementations are split on it, so the choice is a convention:
       * it stays visible for as long as the first trend lasts, and on a series
       * whose close never leaves the band it never washes out at all.
       */
      medianPrice = (inHigh[startIdx] + inLow[startIdx]) / 2.0;
      band = optInMultiplier * prevATR;
      finalUpper = medianPrice + band;
      finalLower = medianPrice - band;
      isUptrend = 1;
      prevClose = inClose[startIdx];
      outReal[0] = finalLower;
      outInteger[0] = 1;
      outIdx = 1;
      today = startIdx + 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         medianPrice = (tempHT + tempLT) / 2.0;
         band = optInMultiplier * prevATR;
         basicUpper = medianPrice + band;
         basicLower = medianPrice - band;
         /* Each band ratchets toward price and is released only by a close on its
          * far side. Nothing keeps the two ordered -- each is released by its own
          * condition, so the lower one can end up above the upper one -- and where
          * that happens a flip puts the emitted band on the far side of the close.
          * The line is therefore NOT monotone within a trend, and the flag does not
          * always say which side of the line price is on; both read as invariants
          * and neither is one.
          */
         if( basicUpper < finalUpper || prevClose > finalUpper ) {
            finalUpper = basicUpper;
         }
         if( basicLower > finalLower || prevClose < finalLower ) {
            finalLower = basicLower;
         }
         closeToday = inClose[today];
         /* The trend is carried in its own variable rather than recovered by
          * comparing the previous output against the previous bands. The two
          * carried bands do coincide on some bars -- always on a bar with no true
          * range, and on some of them at a multiplier of zero -- and there that
          * comparison cannot tell them apart, so it would silently lose the
          * hysteresis on exactly the flat input a corpus of real prices lacks.
          */
         if( (isUptrend) != 0 ) {
            if( closeToday < finalLower ) {
               isUptrend = 0;
            }
         } else if( closeToday > finalUpper ) {
            isUptrend = 1;
         }
         if( (isUptrend) != 0 ) {
            outReal[outIdx] = finalLower;
            outInteger[outIdx] = 1;
         } else {
            outReal[outIdx] = finalUpper;
            outInteger[outIdx] = 0 - 1;
         }
         prevClose = closeToday;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode SUPERTREND_Impl( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            float inClose[],
                            int optInTimePeriod,
                            double optInMultiplier,
                            MInteger outBegIdx,
                            MInteger outNBElement,
                            double outReal[],
                            int outInteger[] )
   {
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int isUptrend = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      double medianPrice = 0;
      double band = 0;
      double basicUpper = 0;
      double basicLower = 0;
      double finalUpper = 0;
      double finalLower = 0;
      double closeToday = 0;
      double prevClose = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = SUPERTREND_Lookback(optInTimePeriod, optInMultiplier);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      i = lookbackTotal - optInTimePeriod;
      while( i != 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
         i -= 1;
      }
      medianPrice = ((double)inHigh[startIdx] + (double)inLow[startIdx]) / 2.0;
      band = optInMultiplier * prevATR;
      finalUpper = medianPrice + band;
      finalLower = medianPrice - band;
      isUptrend = 1;
      prevClose = (double)inClose[startIdx];
      outReal[0] = finalLower;
      outInteger[0] = 1;
      outIdx = 1;
      today = startIdx + 1;
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         medianPrice = (tempHT + tempLT) / 2.0;
         band = optInMultiplier * prevATR;
         basicUpper = medianPrice + band;
         basicLower = medianPrice - band;
         if( basicUpper < finalUpper || prevClose > finalUpper ) {
            finalUpper = basicUpper;
         }
         if( basicLower > finalLower || prevClose < finalLower ) {
            finalLower = basicLower;
         }
         closeToday = (double)inClose[today];
         if( (isUptrend) != 0 ) {
            if( closeToday < finalLower ) {
               isUptrend = 0;
            }
         } else if( closeToday > finalUpper ) {
            isUptrend = 1;
         }
         if( (isUptrend) != 0 ) {
            outReal[outIdx] = finalLower;
            outInteger[outIdx] = 1;
         } else {
            outReal[outIdx] = finalUpper;
            outInteger[outIdx] = 0 - 1;
         }
         prevClose = closeToday;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * An ATR-scaled trailing band that follows price on one side at a time and
    * flips to the other side when the close breaks through it. The trend rides
    * the lower band while it is up and the upper band while it is down, so the
    * line is usually below price in an uptrend and above it in a downtrend, and
    * the flip is the signal. Attributed to Olivier Seban.
    * <p><b>Formula</b>
    * <pre>{@code
    * Median = (High + Low) / 2
    * BasicUpper = Median + Multiplier * ATR(TimePeriod)
    * BasicLower = Median - Multiplier * ATR(TimePeriod)
    * Upper = BasicUpper, when BasicUpper < previous Upper or previous Close > previous Upper; otherwise the previous Upper
    * Lower = BasicLower, when BasicLower > previous Lower or previous Close < previous Lower; otherwise the previous Lower
    * SuperTrend = Lower while the trend is up, until Close < Lower flips it down
    * SuperTrend = Upper while the trend is down, until Close > Upper flips it up
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Both bands are carried forward on every bar, and the trend is decided against the current bar's band. This is the form Investopedia, TradingView and ta4j all describe. A second published form, from the AmiBroker script attributed to Seban, carries only the band the trend is riding and lets the other float free; the two agree on almost every bar and part company at a flip, where this form hands back a band it has been carrying all along and that one hands back a fresh value.</li>
    * <li>The recurrence has no value before the first bar it can be computed on, so the trend is seeded up there and both bands take their unclamped value. Published implementations are split on that seed; this is ta4j's. The choice stays visible for as long as the first trend lasts, it never washes out on a series whose close never leaves the band, and it is why the same bar computed from a later start index can differ.</li>
    * <li>The direction is reported as +1 for an uptrend and -1 for a downtrend, the sign every other signed output in this library uses for bullish. TradingView's built-in {@code ta.supertrend} returns the opposite signs for the same two states, and seeds the other way; a strategy ported from Pine has to swap them.</li>
    * <li>The two carried bands are released by different conditions, so nothing keeps the lower one below the upper one. Where they cross, a flip leaves the line on the far side of the close, and within a single trend the line can widen away from price instead of tightening toward it. Both are ordinary consequences of the published formula, not a variation on it; they are simply not the guarantees the indicator is usually described as giving.</li>
    * <li>A multiplier of zero is degenerate but defined: the two basic bands collapse onto the median price exactly. The carried bands do not collapse with them — each is still released by its own condition — so they coincide on some bars and differ on others, and the trend flips on most bars rather than on every one.</li>
    * <li>The band inherits the Average True Range's warm-up, so a caller who wants it converged sets {@code TA_FUNC_UNST_ATR}, exactly as when calling that function directly.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SUPERTREND_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period of the Average True Range (default
    *        10; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to set
    *        the band width (default 3; minimum 0; {@code -4e37} selects the default).
    * @param outReal The SuperTrend line: the band the trend is currently
    *        riding. Must hold at least {@code endIdx - startIdx + 1} values.
    * @param outInteger Trend direction: +1 while the trend rides the lower
    *        band, -1 while it rides the upper one. Must hold at least
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
    * @see Core#ATR
    * @see Core#MEDPRICE
    * @see Core#SAR
    * @see Core#SAREXT
    * @see Core#KC
    */
   public OutRange SUPERTREND( int startIdx,
                               int endIdx,
                               double inHigh[],
                               double inLow[],
                               double inClose[],
                               int optInTimePeriod,
                               double optInMultiplier,
                               double outReal[],
                               int outInteger[] )
   {
      requireIndexRange("SUPERTREND", startIdx, endIdx);
      int guardStart = clampedStart("SUPERTREND", startIdx, SUPERTREND_Lookback(optInTimePeriod, optInMultiplier));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SUPERTREND", "inHigh", inHigh, guardInLen);
      requireLength("SUPERTREND", "inLow", inLow, guardInLen);
      requireLength("SUPERTREND", "inClose", inClose, guardInLen);
      requireLength("SUPERTREND", "outReal", outReal, guardOutLen);
      requireLength("SUPERTREND", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SUPERTREND_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outReal, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("SUPERTREND", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * An ATR-scaled trailing band that follows price on one side at a time and
    * flips to the other side when the close breaks through it. The trend rides
    * the lower band while it is up and the upper band while it is down, so the
    * line is usually below price in an uptrend and above it in a downtrend, and
    * the flip is the signal. Attributed to Olivier Seban.
    * <p><b>Formula</b>
    * <pre>{@code
    * Median = (High + Low) / 2
    * BasicUpper = Median + Multiplier * ATR(TimePeriod)
    * BasicLower = Median - Multiplier * ATR(TimePeriod)
    * Upper = BasicUpper, when BasicUpper < previous Upper or previous Close > previous Upper; otherwise the previous Upper
    * Lower = BasicLower, when BasicLower > previous Lower or previous Close < previous Lower; otherwise the previous Lower
    * SuperTrend = Lower while the trend is up, until Close < Lower flips it down
    * SuperTrend = Upper while the trend is down, until Close > Upper flips it up
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Both bands are carried forward on every bar, and the trend is decided against the current bar's band. This is the form Investopedia, TradingView and ta4j all describe. A second published form, from the AmiBroker script attributed to Seban, carries only the band the trend is riding and lets the other float free; the two agree on almost every bar and part company at a flip, where this form hands back a band it has been carrying all along and that one hands back a fresh value.</li>
    * <li>The recurrence has no value before the first bar it can be computed on, so the trend is seeded up there and both bands take their unclamped value. Published implementations are split on that seed; this is ta4j's. The choice stays visible for as long as the first trend lasts, it never washes out on a series whose close never leaves the band, and it is why the same bar computed from a later start index can differ.</li>
    * <li>The direction is reported as +1 for an uptrend and -1 for a downtrend, the sign every other signed output in this library uses for bullish. TradingView's built-in {@code ta.supertrend} returns the opposite signs for the same two states, and seeds the other way; a strategy ported from Pine has to swap them.</li>
    * <li>The two carried bands are released by different conditions, so nothing keeps the lower one below the upper one. Where they cross, a flip leaves the line on the far side of the close, and within a single trend the line can widen away from price instead of tightening toward it. Both are ordinary consequences of the published formula, not a variation on it; they are simply not the guarantees the indicator is usually described as giving.</li>
    * <li>A multiplier of zero is degenerate but defined: the two basic bands collapse onto the median price exactly. The carried bands do not collapse with them — each is still released by its own condition — so they coincide on some bars and differ on others, and the trend flips on most bars rather than on every one.</li>
    * <li>The band inherits the Average True Range's warm-up, so a caller who wants it converged sets {@code TA_FUNC_UNST_ATR}, exactly as when calling that function directly.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#SUPERTREND_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod Smoothing period of the Average True Range (default
    *        10; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInMultiplier Multiplier applied to the Average True Range to set
    *        the band width (default 3; minimum 0; {@code -4e37} selects the default).
    * @param outReal The SuperTrend line: the band the trend is currently
    *        riding. Must hold at least {@code endIdx - startIdx + 1} values.
    * @param outInteger Trend direction: +1 while the trend rides the lower
    *        band, -1 while it rides the upper one. Must hold at least
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
    * @see Core#ATR
    * @see Core#MEDPRICE
    * @see Core#SAR
    * @see Core#SAREXT
    * @see Core#KC
    */
   public OutRange SUPERTREND( int startIdx,
                               int endIdx,
                               float inHigh[],
                               float inLow[],
                               float inClose[],
                               int optInTimePeriod,
                               double optInMultiplier,
                               double outReal[],
                               int outInteger[] )
   {
      requireIndexRange("SUPERTREND", startIdx, endIdx);
      int guardStart = clampedStart("SUPERTREND", startIdx, SUPERTREND_Lookback(optInTimePeriod, optInMultiplier));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SUPERTREND", "inHigh", inHigh, guardInLen);
      requireLength("SUPERTREND", "inLow", inLow, guardInLen);
      requireLength("SUPERTREND", "inClose", inClose, guardInLen);
      requireLength("SUPERTREND", "outReal", outReal, guardOutLen);
      requireLength("SUPERTREND", "outInteger", outInteger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = SUPERTREND_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outReal, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("SUPERTREND", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SUPERTREND stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#SUPERTREND} over the same series.
    * Open with {@link Core#supertrendOpen}; there is no close — the handle is
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
   public static final class SupertrendStream {
      Core core;
      int optInTimePeriod;
      double optInMultiplier;
      int isUptrend;
      double prevATR;
      double wAlpha;
      double wBeta;
      double finalUpper;
      double finalLower;
      double prevClose;
      double lag1_inClose;
      double cur_outReal;
      int cur_outInteger;
      int outRangeBegIdx;
      int outRangeCount;

      SupertrendStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#SUPERTREND} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      SupertrendStream( SupertrendStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInMultiplier = other.optInMultiplier;
         this.isUptrend = other.isUptrend;
         this.prevATR = other.prevATR;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
         this.finalUpper = other.finalUpper;
         this.finalLower = other.finalLower;
         this.prevClose = other.prevClose;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outReal = other.cur_outReal;
         this.cur_outInteger = other.cur_outInteger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value(SupertrendOut)} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, SupertrendOut out ) {
         requireArgument("SUPERTREND update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("SUPERTREND update: BadParam", RetCode.BadParam);
         }
         core.supertrendStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.real = this.cur_outReal;
         out.integer = this.cur_outInteger;
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
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[], int outInteger[] ) {
         requireArgument("SUPERTREND updateAndFill", "inHigh", inHigh);
         requireArgument("SUPERTREND updateAndFill", "inLow", inLow);
         requireArgument("SUPERTREND updateAndFill", "inClose", inClose);
         requireArgument("SUPERTREND updateAndFill", "outReal", outReal);
         requireArgument("SUPERTREND updateAndFill", "outInteger", outInteger);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || outInteger.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose || (Object)outReal == (Object)outInteger )
            throw new TaLibArgumentException("SUPERTREND updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("SUPERTREND updateAndFill: BadParam", RetCode.BadParam);
            }
            core.supertrendStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outReal[i] = this.cur_outReal;
            outInteger[i] = this.cur_outInteger;
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
      public void peek( double inHigh, double inLow, double inClose, SupertrendOut out ) {
         requireArgument("SUPERTREND peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("SUPERTREND peek: BadParam", RetCode.BadParam);
         SupertrendStream sp = this;
         double val2 = 0.0;
         double val3 = 0.0;
         double greatest = 0.0;
         double tempCY = 0.0;
         double tempLT = 0.0;
         double tempHT = 0.0;
         double medianPrice = 0.0;
         double band = 0.0;
         double basicUpper = 0.0;
         double basicLower = 0.0;
         double closeToday = 0.0;
         int cur_outInteger = sp.cur_outInteger;
         double cur_outReal = sp.cur_outReal;
         double finalLower = sp.finalLower;
         double finalUpper = sp.finalUpper;
         int isUptrend = sp.isUptrend;
         double prevATR = sp.prevATR;
         tempLT = inLow;
         tempHT = inHigh;
         tempCY = sp.lag1_inClose;
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
         medianPrice = (tempHT + tempLT) / 2.0;
         band = sp.optInMultiplier * prevATR;
         basicUpper = medianPrice + band;
         basicLower = medianPrice - band;
         /* Each band ratchets toward price and is released only by a close on its
          * far side. Nothing keeps the two ordered -- each is released by its own
          * condition, so the lower one can end up above the upper one -- and where
          * that happens a flip puts the emitted band on the far side of the close.
          * The line is therefore NOT monotone within a trend, and the flag does not
          * always say which side of the line price is on; both read as invariants
          * and neither is one.
          */
         if( basicUpper < finalUpper || sp.prevClose > finalUpper ) {
            finalUpper = basicUpper;
         }
         if( basicLower > finalLower || sp.prevClose < finalLower ) {
            finalLower = basicLower;
         }
         closeToday = inClose;
         /* The trend is carried in its own variable rather than recovered by
          * comparing the previous output against the previous bands. The two
          * carried bands do coincide on some bars -- always on a bar with no true
          * range, and on some of them at a multiplier of zero -- and there that
          * comparison cannot tell them apart, so it would silently lose the
          * hysteresis on exactly the flat input a corpus of real prices lacks.
          */
         if( (isUptrend) != 0 ) {
            if( closeToday < finalLower ) {
               isUptrend = 0;
            }
         } else if( closeToday > finalUpper ) {
            isUptrend = 1;
         }
         if( (isUptrend) != 0 ) {
            cur_outReal = finalLower;
            cur_outInteger = 1;
         } else {
            cur_outReal = finalUpper;
            cur_outInteger = 0 - 1;
         }
         out.real = cur_outReal;
         out.integer = cur_outInteger;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( SupertrendOut out ) {
         requireArgument("SUPERTREND value", "out", out);
         out.real = this.cur_outReal;
         out.integer = this.cur_outInteger;
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
      public SupertrendStream clone() {
         return new SupertrendStream(this);
      }
   }

   /**
    * The outputs of one SUPERTREND bar, written by the stream into an object the
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
   public static final class SupertrendOut {
      /** The SuperTrend line: the band the trend is currently riding. */
      public double real;
      /** Trend direction: +1 while the trend rides the lower band, -1 while it rides the upper one. */
      public int integer;
   }
   void supertrendStepImpl( SupertrendStream sp, double inHigh, double inLow, double inClose )
   {
      double val2 = 0.0;
      double val3 = 0.0;
      double greatest = 0.0;
      double tempCY = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      double medianPrice = 0.0;
      double band = 0.0;
      double basicUpper = 0.0;
      double basicLower = 0.0;
      double closeToday = 0.0;
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      greatest = tempHT - tempLT;
      /* val1 */
      val2 = Math.abs(tempCY - tempHT);
      if( val2 > greatest ) {
         greatest = val2;
      }
      val3 = Math.abs(tempCY - tempLT);
      if( val3 > greatest ) {
         greatest = val3;
      }
      sp.prevATR = Math.fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
      medianPrice = (tempHT + tempLT) / 2.0;
      band = sp.optInMultiplier * sp.prevATR;
      basicUpper = medianPrice + band;
      basicLower = medianPrice - band;
      /* Each band ratchets toward price and is released only by a close on its
       * far side. Nothing keeps the two ordered -- each is released by its own
       * condition, so the lower one can end up above the upper one -- and where
       * that happens a flip puts the emitted band on the far side of the close.
       * The line is therefore NOT monotone within a trend, and the flag does not
       * always say which side of the line price is on; both read as invariants
       * and neither is one.
       */
      if( basicUpper < sp.finalUpper || sp.prevClose > sp.finalUpper ) {
         sp.finalUpper = basicUpper;
      }
      if( basicLower > sp.finalLower || sp.prevClose < sp.finalLower ) {
         sp.finalLower = basicLower;
      }
      closeToday = inClose;
      /* The trend is carried in its own variable rather than recovered by
       * comparing the previous output against the previous bands. The two
       * carried bands do coincide on some bars -- always on a bar with no true
       * range, and on some of them at a multiplier of zero -- and there that
       * comparison cannot tell them apart, so it would silently lose the
       * hysteresis on exactly the flat input a corpus of real prices lacks.
       */
      if( (sp.isUptrend) != 0 ) {
         if( closeToday < sp.finalLower ) {
            sp.isUptrend = 0;
         }
      } else if( closeToday > sp.finalUpper ) {
         sp.isUptrend = 1;
      }
      if( (sp.isUptrend) != 0 ) {
         sp.cur_outReal = sp.finalLower;
         sp.cur_outInteger = 1;
      } else {
         sp.cur_outReal = sp.finalUpper;
         sp.cur_outInteger = 0 - 1;
      }
      sp.prevClose = closeToday;
      sp.lag1_inClose = inClose;
   }
   private RetCode supertrendOpenImpl( SupertrendStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outInteger[], int outStride )
   {
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int isUptrend = 0;
      double prevATR = 0;
      double periodTotal = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      double medianPrice = 0;
      double band = 0;
      double basicUpper = 0;
      double basicLower = 0;
      double finalUpper = 0;
      double finalLower = 0;
      double closeToday = 0;
      double prevClose = 0;
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
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInMultiplier == REAL_DEFAULT ) {
         optInMultiplier = 3e0;
      } else if( !(optInMultiplier >= 0e0 && optInMultiplier <= REAL_MAX) ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = SUPERTREND_Lookback(optInTimePeriod, optInMultiplier);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      /* The Average True Range is carried inline rather than taken from a call,
       * because the band and the ratchet advance together one bar at a time and a
       * whole-range buffer between them would not stream.
       *
       * The arithmetic order below is the bit-exactness contract with TA_ATR (do
       * not reorder): True Range from high-low, then the two previous-close
       * distances in that order; the seed summed from 0.0 over the first 'period'
       * True Ranges and divided once; the same two Wilder coefficients, wBeta
       * rounded and wAlpha derived from it, in one fused statement.
       */
      wBeta = (double)(optInTimePeriod - 1) / (double)optInTimePeriod;
      wAlpha = 1.0 - wBeta;
      today = startIdx - lookbackTotal + 1;
      periodTotal = 0.0;
      i = optInTimePeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInTimePeriod;
      /* Skip the Average True Range's unstable period. Taking the count from the
       * lookback rather than naming the setting keeps the two from disagreeing.
       */
      i = lookbackTotal - optInTimePeriod;
      while( i != 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
         i -= 1;
      }
      /* The first bar has no band to ratchet against and no trend to carry, so
       * both bands take their unclamped value and the trend is seeded long, as
       * ta4j's SuperTrendIndicator does. The formula does not settle this and the
       * published implementations are split on it, so the choice is a convention:
       * it stays visible for as long as the first trend lasts, and on a series
       * whose close never leaves the band it never washes out at all.
       */
      medianPrice = (inHigh[startIdx] + inLow[startIdx]) / 2.0;
      band = optInMultiplier * prevATR;
      finalUpper = medianPrice + band;
      finalLower = medianPrice - band;
      isUptrend = 1;
      prevClose = inClose[startIdx];
      outReal[0 * outStride] = finalLower;
      outInteger[0 * outStride] = 1;
      outIdx = 1;
      today = startIdx + 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         medianPrice = (tempHT + tempLT) / 2.0;
         band = optInMultiplier * prevATR;
         basicUpper = medianPrice + band;
         basicLower = medianPrice - band;
         /* Each band ratchets toward price and is released only by a close on its
          * far side. Nothing keeps the two ordered -- each is released by its own
          * condition, so the lower one can end up above the upper one -- and where
          * that happens a flip puts the emitted band on the far side of the close.
          * The line is therefore NOT monotone within a trend, and the flag does not
          * always say which side of the line price is on; both read as invariants
          * and neither is one.
          */
         if( basicUpper < finalUpper || prevClose > finalUpper ) {
            finalUpper = basicUpper;
         }
         if( basicLower > finalLower || prevClose < finalLower ) {
            finalLower = basicLower;
         }
         closeToday = inClose[today];
         /* The trend is carried in its own variable rather than recovered by
          * comparing the previous output against the previous bands. The two
          * carried bands do coincide on some bars -- always on a bar with no true
          * range, and on some of them at a multiplier of zero -- and there that
          * comparison cannot tell them apart, so it would silently lose the
          * hysteresis on exactly the flat input a corpus of real prices lacks.
          */
         if( (isUptrend) != 0 ) {
            if( closeToday < finalLower ) {
               isUptrend = 0;
            }
         } else if( closeToday > finalUpper ) {
            isUptrend = 1;
         }
         if( (isUptrend) != 0 ) {
            outReal[outIdx * outStride] = finalLower;
            outInteger[outIdx * outStride] = 1;
         } else {
            outReal[outIdx * outStride] = finalUpper;
            outInteger[outIdx * outStride] = 0 - 1;
         }
         prevClose = closeToday;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInMultiplier = optInMultiplier;
      sp.isUptrend = isUptrend;
      sp.prevATR = prevATR;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.finalUpper = finalUpper;
      sp.finalLower = finalLower;
      sp.prevClose = prevClose;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      sp.cur_outInteger = outInteger[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* supertrendOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SupertrendStream supertrendOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outInteger[] )
   {
      SupertrendStream sp = new SupertrendStream(this);
      RetCode retCode = supertrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outReal, outInteger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SUPERTREND openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SUPERTREND openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("SUPERTREND openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind supertrendOpen (composition seam). */
   SupertrendStream supertrendOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod, double optInMultiplier )
   {
      SupertrendStream sp = new SupertrendStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      int[] sink_outInteger = new int[1];
      RetCode retCode = supertrendOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, sink_outReal, sink_outInteger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("SUPERTREND open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("SUPERTREND open: internal error", retCode);
      }
      throw new TaLibArgumentException("SUPERTREND open: " + retCode, retCode);
   }
   /**
    * Open a live SUPERTREND stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#SUPERTREND} at that bar.
    * <p>The history must hold at least {@code SUPERTREND_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SupertrendStream supertrendOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInMultiplier )
   {
      requireArgument("SUPERTREND open", "inHigh", inHigh);
      requireHistory("SUPERTREND open", inHigh.length);
      requireArgument("SUPERTREND open", "inLow", inLow);
      requireArgument("SUPERTREND open", "inClose", inClose);
      requireHistoryLength("SUPERTREND open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("SUPERTREND open", "inClose", inClose.length, inHigh.length);
      return supertrendOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInMultiplier);
   }
   /**
    * {@link Core#supertrendOpen} that also fills the output array(s) bit-identically
    * to {@link Core#SUPERTREND} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SupertrendStream#outRange()}.
    */
   public SupertrendStream supertrendOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod, double optInMultiplier, double outReal[], int outInteger[] )
   {
      requireArgument("SUPERTREND openAndFill", "inHigh", inHigh);
      requireHistory("SUPERTREND openAndFill", inHigh.length);
      requireArgument("SUPERTREND openAndFill", "inLow", inLow);
      requireArgument("SUPERTREND openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("SUPERTREND openAndFill", inHigh.length, SUPERTREND_Lookback(optInTimePeriod, optInMultiplier));
      requireHistoryLength("SUPERTREND openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("SUPERTREND openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("SUPERTREND openAndFill", "outReal", outReal, guardOutLen);
      requireLength("SUPERTREND openAndFill", "outInteger", outInteger, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose || (Object)outReal == (Object)outInteger ) {
         throw new TaLibArgumentException("SUPERTREND openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return supertrendOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod, optInMultiplier, outBegIdx, outNBElement, outReal, outInteger);
   }
