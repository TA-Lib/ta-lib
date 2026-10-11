/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#487).
 */

   /**
    * Number of leading input bars {@link Core#zigzag} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInSensitivity Minimum move away from the current extreme that
    *        reverses the leg, in percent (default 5; range 0..100;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMinTrendLength Minimum number of bars between two pivots
    *        (default 1; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int zigzagLookback( double optInSensitivity, int optInMinTrendLength )
   {
      if( optInSensitivity == REAL_DEFAULT ) {
         optInSensitivity = 5e0;
      } else if( !(optInSensitivity >= 0e0 && optInSensitivity <= 1e2) ) {
         return -1;
      }
      if( optInMinTrendLength == Integer.MIN_VALUE ) {
         optInMinTrendLength = 1;
      } else if( optInMinTrendLength < 1 || optInMinTrendLength > 100000 ) {
         return -1;
      }
      /* The first bar at which a reversal off the seed can fire. The gate counts
       * from the pivot's OWN bar and the seed is a pivot like any other, so the
       * earliest reversal is optInMinTrendLength bars after the seed, which sits
       * that many bars before the first output.
       *
       * Independent of the sensitivity: the threshold decides WHETHER a reversal
       * fires, never how early it may. sar_lookback is the precedent for a
       * constant lookback whose state is seeded from the bar before startIdx.
       */
      return optInMinTrendLength ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#zigzag}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInSensitivity Minimum move away from the current extreme that
    *        reverses the leg, in percent (default 5; range 0..100;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMinTrendLength Minimum number of bars between two pivots
    *        (default 1; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int zigzagDisplayShift( double optInSensitivity, int optInMinTrendLength, int outputIdx )
   {
      if( zigzagLookback( optInSensitivity, optInMinTrendLength ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 3 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode zigzagImpl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double optInSensitivity,
                       int optInMinTrendLength,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outZigZag[],
                       int outTrend[],
                       int outPivotIdx[] )
   {
      int lookbackTotal = 0;
      int today = 0;
      int outIdx = 0;
      int trend = 0;
      int pivotIdx = 0;
      int barsSincePivot = 0;
      int barIdx = 0;
      double s = 0;
      double up = 0;
      double dn = 0;
      double pivot = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInSensitivity == REAL_DEFAULT ) {
         optInSensitivity = 5e0;
      } else if( !(optInSensitivity >= 0e0 && optInSensitivity <= 1e2) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMinTrendLength == Integer.MIN_VALUE ) {
         optInMinTrendLength = 1;
      } else if( optInMinTrendLength < 1 || optInMinTrendLength > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outTrend == outPivotIdx ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = zigzagLookback(optInSensitivity, optInMinTrendLength);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      /* Percent in, fraction inside. Each factor is rounded once, here, rather
       * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
       * the expression order the oracle uses.
       */
      s = optInSensitivity / 100.0;
      up = 1.0 + s;
      dn = 1.0 - s;
      /* The seed: a down leg whose extreme is the low of the bar
       * optInMinTrendLength before the first output. Because the gate counts the
       * seed bar like every other pivot, the first reversal can fire exactly at
       * the first output bar, not one later.
       */
      today = startIdx - lookbackTotal;
      trend = -1;
      pivot = inLow[today];
      barIdx = today;
      pivotIdx = barIdx;
      barsSincePivot = 0;
      today = today + 1;
      outIdx = 0;
      while( today <= endIdx ) {
         /* Bars since the pivot's own bar, carried rather than recomputed as
          * `today - pivotIdx`: a difference of two absolute indices has no meaning
          * to a stream, which sees one bar at a time. talipp's gate is written the
          * same way (`len(input) - pivot.position`), and the counter resets with
          * the pivot so the two expressions are equal at every bar.
          *
          * barIdx is the absolute bar number the index output names, carried in
          * its own right rather than read off the loop cursor. A stream has a
          * cursor only into its own history, so the cursor cannot survive into the
          * per-bar transition; outPivotIdx still has to name the bar the pivot
          * sits on, and this is what lets it.
          */
         barsSincePivot = barsSincePivot + 1;
         barIdx = barIdx + 1;
         /* Reversal is tested BEFORE extension. On an outside bar that both makes
          * a new extreme and clears the threshold, the leg reverses and the old
          * pivot stays where it was; testing extension first would move the pivot
          * and lose the reversal, which is a whole leg of difference rather than a
          * rounding.
          */
         if( trend == -1 ) {
            if( inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength ) {
               trend = 1;
               pivot = inHigh[today];
               pivotIdx = barIdx;
               barsSincePivot = 0;
            } else if( inLow[today] <= pivot ) {
               /* Ties extend. At an equal price the pivot moves to the LATER bar,
                * so outZigZag does not change and outPivotIdx does: that movement
                * is the only thing the index output carries and the price output
                * cannot.
                */
               pivot = inLow[today];
               pivotIdx = barIdx;
               barsSincePivot = 0;
            }
         } else if( inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength ) {
            trend = -1;
            pivot = inLow[today];
            pivotIdx = barIdx;
            barsSincePivot = 0;
         } else if( inHigh[today] >= pivot ) {
            pivot = inHigh[today];
            pivotIdx = barIdx;
            barsSincePivot = 0;
         }
         if( today >= startIdx ) {
            outZigZag[outIdx] = pivot;
            outTrend[outIdx] = trend;
            outPivotIdx[outIdx] = pivotIdx;
            outIdx = outIdx + 1;
         }
         today = today + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode zigzagImpl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       double optInSensitivity,
                       int optInMinTrendLength,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outZigZag[],
                       int outTrend[],
                       int outPivotIdx[] )
   {
      int lookbackTotal = 0;
      int today = 0;
      int outIdx = 0;
      int trend = 0;
      int pivotIdx = 0;
      int barsSincePivot = 0;
      int barIdx = 0;
      double s = 0;
      double up = 0;
      double dn = 0;
      double pivot = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInSensitivity == REAL_DEFAULT ) {
         optInSensitivity = 5e0;
      } else if( !(optInSensitivity >= 0e0 && optInSensitivity <= 1e2) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMinTrendLength == Integer.MIN_VALUE ) {
         optInMinTrendLength = 1;
      } else if( optInMinTrendLength < 1 || optInMinTrendLength > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outTrend == outPivotIdx ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = zigzagLookback(optInSensitivity, optInMinTrendLength);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      s = optInSensitivity / 100.0;
      up = 1.0 + s;
      dn = 1.0 - s;
      today = startIdx - lookbackTotal;
      trend = -1;
      pivot = (double)inLow[today];
      barIdx = today;
      pivotIdx = barIdx;
      barsSincePivot = 0;
      today = today + 1;
      outIdx = 0;
      while( today <= endIdx ) {
         barsSincePivot = barsSincePivot + 1;
         barIdx = barIdx + 1;
         if( trend == -1 ) {
            if( (double)inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength ) {
               trend = 1;
               pivot = (double)inHigh[today];
               pivotIdx = barIdx;
               barsSincePivot = 0;
            } else if( (double)inLow[today] <= pivot ) {
               pivot = (double)inLow[today];
               pivotIdx = barIdx;
               barsSincePivot = 0;
            }
         } else if( (double)inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength ) {
            trend = -1;
            pivot = (double)inLow[today];
            pivotIdx = barIdx;
            barsSincePivot = 0;
         } else if( (double)inHigh[today] >= pivot ) {
            pivot = (double)inHigh[today];
            pivotIdx = barIdx;
            barsSincePivot = 0;
         }
         if( today >= startIdx ) {
            outZigZag[outIdx] = pivot;
            outTrend[outIdx] = trend;
            outPivotIdx[outIdx] = pivotIdx;
            outIdx = outIdx + 1;
         }
         today = today + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * The percent-filter swing line. A leg runs from the last confirmed pivot to
    * the most extreme price since it, and it reverses only when price moves at
    * least {@code optInSensitivity} percent away from that extreme and at least
    * {@code optInMinTrendLength} bars have passed since the pivot's own bar.
    * Achelis describes the chart form and warns that <i>"the last 'leg'
    * displayed in a Zig Zag chart can change"</i>. This function does not emit
    * the chart line, which is drawn with hindsight. It emits the causal state
    * the line is drawn from, one row per bar and final once emitted: the price
    * of the swing extreme currently being tracked, the absolute bar index of
    * that extreme, and whether the current leg is up or down. The chart is
    * rebuilt exactly from the outputs. A pivot is confirmed at bar
    * {@code i &gt; outBegIdx} whenever {@code outTrend[i]} differs from
    * {@code outTrend[i-1]}, and that pivot is
    * {@code (outPivotIdx[i-1], outZigZag[i-1])}. The last, still-open pivot is
    * {@code (outPivotIdx[last], outZigZag[last])} — the leg Achelis warns
    * about.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/zigzag">ta-lib.org/functions/zigzag</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>Reversal is tested before extension.</b> On an outside bar that both makes a new extreme and clears the threshold, the leg reverses and the old pivot stays where it was. The other order moves the pivot to the new extreme and defers the reversal to the next bar, against a threshold that has just moved further away: on the 252-bar regtest corpus at 5 percent that costs one bar (R TTR, which takes that order, places the swing low at 148 where this places it at 147), but a range narrower than the sensitivity after the outside bar keeps it from ever firing, and the leg is then lost rather than delayed.</li>
    * <li><b>Ties extend.</b> The comparisons are {@code &lt;=} and {@code &gt;=}, so at an equal price the pivot moves to the later bar: {@code outZigZag} does not change and {@code outPivotIdx} does. That movement is the only thing the index output carries and the price output cannot.</li>
    * <li><b>The gate counts from the pivot's own bar, the seed included.</b> No reversal can fire before the first output bar; the first one can fire exactly there.</li>
    * <li><b>The seed is a low.</b> A series that only rises therefore reports an up leg from its first reversal and never returns to a down leg.</li>
    * <li>The lookback is {@code optInMinTrendLength} and does not depend on the sensitivity: the threshold decides whether a reversal fires, never how early it may.</li>
    * <li>{@code optInSensitivity} is a percent, as Achelis, StockCharts, TTR, Skender and TradingView take it. 0 and 1 are degenerate but defined: at 0 a reversal fires on almost every bar the gate allows, and at 100 no downward reversal fires on positive prices.</li>
    * <li>The value at a bar depends on where the caller started, through the seed; the function is {@code path_dependent} for that reason, as SAR and SUPERTREND are.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#zigzagLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInSensitivity Minimum move away from the current extreme that
    *        reverses the leg, in percent (default 5; range 0..100;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMinTrendLength Minimum number of bars between two pivots
    *        (default 1; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outZigZag Price of the swing extreme currently being tracked. Must
    *        hold at least {@code endIdx - max(startIdx, zigzagLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @param outTrend +1 while the leg is up, -1 while it is down. Must hold at
    *        least {@code endIdx - max(startIdx, zigzagLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outPivotIdx Absolute index, into the input, of the bar that extreme
    *        sits on. Must hold at least
    *        {@code endIdx - max(startIdx, zigzagLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#sar
    * @see Core#supertrend
    * @see Core#minmaxindex
    * @see Core#midprice
    */
   public OutRange zigzag( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double optInSensitivity,
                           int optInMinTrendLength,
                           double outZigZag[],
                           int outTrend[],
                           int outPivotIdx[] )
   {
      requireIndexRange("ZIGZAG", startIdx, endIdx);
      int guardStart = clampedStart("ZIGZAG", startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ZIGZAG", "inHigh", inHigh, guardInLen);
      requireLength("ZIGZAG", "inLow", inLow, guardInLen);
      requireLength("ZIGZAG", "outZigZag", outZigZag, guardOutLen);
      requireLength("ZIGZAG", "outTrend", outTrend, guardOutLen);
      requireLength("ZIGZAG", "outPivotIdx", outPivotIdx, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = zigzagImpl(startIdx, endIdx, inHigh, inLow, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ZIGZAG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * The percent-filter swing line. A leg runs from the last confirmed pivot to
    * the most extreme price since it, and it reverses only when price moves at
    * least {@code optInSensitivity} percent away from that extreme and at least
    * {@code optInMinTrendLength} bars have passed since the pivot's own bar.
    * Achelis describes the chart form and warns that <i>"the last 'leg'
    * displayed in a Zig Zag chart can change"</i>. This function does not emit
    * the chart line, which is drawn with hindsight. It emits the causal state
    * the line is drawn from, one row per bar and final once emitted: the price
    * of the swing extreme currently being tracked, the absolute bar index of
    * that extreme, and whether the current leg is up or down. The chart is
    * rebuilt exactly from the outputs. A pivot is confirmed at bar
    * {@code i &gt; outBegIdx} whenever {@code outTrend[i]} differs from
    * {@code outTrend[i-1]}, and that pivot is
    * {@code (outPivotIdx[i-1], outZigZag[i-1])}. The last, still-open pivot is
    * {@code (outPivotIdx[last], outZigZag[last])} — the leg Achelis warns
    * about.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/zigzag">ta-lib.org/functions/zigzag</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li><b>Reversal is tested before extension.</b> On an outside bar that both makes a new extreme and clears the threshold, the leg reverses and the old pivot stays where it was. The other order moves the pivot to the new extreme and defers the reversal to the next bar, against a threshold that has just moved further away: on the 252-bar regtest corpus at 5 percent that costs one bar (R TTR, which takes that order, places the swing low at 148 where this places it at 147), but a range narrower than the sensitivity after the outside bar keeps it from ever firing, and the leg is then lost rather than delayed.</li>
    * <li><b>Ties extend.</b> The comparisons are {@code &lt;=} and {@code &gt;=}, so at an equal price the pivot moves to the later bar: {@code outZigZag} does not change and {@code outPivotIdx} does. That movement is the only thing the index output carries and the price output cannot.</li>
    * <li><b>The gate counts from the pivot's own bar, the seed included.</b> No reversal can fire before the first output bar; the first one can fire exactly there.</li>
    * <li><b>The seed is a low.</b> A series that only rises therefore reports an up leg from its first reversal and never returns to a down leg.</li>
    * <li>The lookback is {@code optInMinTrendLength} and does not depend on the sensitivity: the threshold decides whether a reversal fires, never how early it may.</li>
    * <li>{@code optInSensitivity} is a percent, as Achelis, StockCharts, TTR, Skender and TradingView take it. 0 and 1 are degenerate but defined: at 0 a reversal fires on almost every bar the gate allows, and at 100 no downward reversal fires on positive prices.</li>
    * <li>The value at a bar depends on where the caller started, through the seed; the function is {@code path_dependent} for that reason, as SAR and SUPERTREND are.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#zigzagLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInSensitivity Minimum move away from the current extreme that
    *        reverses the leg, in percent (default 5; range 0..100;
    *        {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMinTrendLength Minimum number of bars between two pivots
    *        (default 1; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param outZigZag Price of the swing extreme currently being tracked. Must
    *        hold at least {@code endIdx - max(startIdx, zigzagLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @param outTrend +1 while the leg is up, -1 while it is down. Must hold at
    *        least {@code endIdx - max(startIdx, zigzagLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outPivotIdx Absolute index, into the input, of the bar that extreme
    *        sits on. Must hold at least
    *        {@code endIdx - max(startIdx, zigzagLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#sar
    * @see Core#supertrend
    * @see Core#minmaxindex
    * @see Core#midprice
    */
   public OutRange zigzag( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           double optInSensitivity,
                           int optInMinTrendLength,
                           double outZigZag[],
                           int outTrend[],
                           int outPivotIdx[] )
   {
      requireIndexRange("ZIGZAG", startIdx, endIdx);
      int guardStart = clampedStart("ZIGZAG", startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ZIGZAG", "inHigh", inHigh, guardInLen);
      requireLength("ZIGZAG", "inLow", inLow, guardInLen);
      requireLength("ZIGZAG", "outZigZag", outZigZag, guardOutLen);
      requireLength("ZIGZAG", "outTrend", outTrend, guardOutLen);
      requireLength("ZIGZAG", "outPivotIdx", outPivotIdx, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = zigzagImpl(startIdx, endIdx, inHigh, inLow, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ZIGZAG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ZIGZAG stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#zigzag} over the same series.
    * Open with {@link Core#zigzagOpen}; there is no close — the handle is
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
   public static final class ZigzagStream {
      private Core core;
      private double optInSensitivity;
      private int optInMinTrendLength;
      private int trend;
      private int pivotIdx;
      private int barsSincePivot;
      private int barIdx;
      private double up;
      private double dn;
      private double pivot;
      private double cur_outZigZag;
      private int cur_outTrend;
      private int cur_outPivotIdx;
      private int outRangeBegIdx;
      private int outRangeCount;

      private ZigzagStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#zigzag} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value(ZigzagOut)} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ZIGZAG advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private ZigzagStream( ZigzagStream other ) {
         this.core = other.core;
         this.optInSensitivity = other.optInSensitivity;
         this.optInMinTrendLength = other.optInMinTrendLength;
         this.trend = other.trend;
         this.pivotIdx = other.pivotIdx;
         this.barsSincePivot = other.barsSincePivot;
         this.barIdx = other.barIdx;
         this.up = other.up;
         this.dn = other.dn;
         this.pivot = other.pivot;
         this.cur_outZigZag = other.cur_outZigZag;
         this.cur_outTrend = other.cur_outTrend;
         this.cur_outPivotIdx = other.cur_outPivotIdx;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(ZigzagOut)} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public void update( double inHigh, double inLow, ZigzagOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ZIGZAG update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("ZIGZAG update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("ZIGZAG update", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         core.zigzagStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         out.zigZag = this.cur_outZigZag;
         out.trend = this.cur_outTrend;
         out.pivotIdx = this.cur_outPivotIdx;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, ZigzagOut out ) {
         requireArgument("ZIGZAG peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("ZIGZAG peek", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         ZigzagStream sp = this;
         int barIdx = sp.barIdx;
         int barsSincePivot = sp.barsSincePivot;
         int cur_outPivotIdx = 0;
         int cur_outTrend = 0;
         double cur_outZigZag = 0.0;
         double pivot = sp.pivot;
         int pivotIdx = sp.pivotIdx;
         int trend = sp.trend;
         /* Bars since the pivot's own bar, carried rather than recomputed as
          * `today - pivotIdx`: a difference of two absolute indices has no meaning
          * to a stream, which sees one bar at a time. talipp's gate is written the
          * same way (`len(input) - pivot.position`), and the counter resets with
          * the pivot so the two expressions are equal at every bar.
          *
          * barIdx is the absolute bar number the index output names, carried in
          * its own right rather than read off the loop cursor. A stream has a
          * cursor only into its own history, so the cursor cannot survive into the
          * per-bar transition; outPivotIdx still has to name the bar the pivot
          * sits on, and this is what lets it.
          */
         barsSincePivot = barsSincePivot + 1;
         barIdx = barIdx + 1;
         /* Reversal is tested BEFORE extension. On an outside bar that both makes
          * a new extreme and clears the threshold, the leg reverses and the old
          * pivot stays where it was; testing extension first would move the pivot
          * and lose the reversal, which is a whole leg of difference rather than a
          * rounding.
          */
         if( trend == -1 ) {
            if( inHigh >= pivot * sp.up && barsSincePivot >= sp.optInMinTrendLength ) {
               trend = 1;
               pivot = inHigh;
               pivotIdx = barIdx;
               barsSincePivot = 0;
            } else if( inLow <= pivot ) {
               /* Ties extend. At an equal price the pivot moves to the LATER bar,
                * so outZigZag does not change and outPivotIdx does: that movement
                * is the only thing the index output carries and the price output
                * cannot.
                */
               pivot = inLow;
               pivotIdx = barIdx;
               barsSincePivot = 0;
            }
         } else if( inLow <= pivot * sp.dn && barsSincePivot >= sp.optInMinTrendLength ) {
            trend = -1;
            pivot = inLow;
            pivotIdx = barIdx;
            barsSincePivot = 0;
         } else if( inHigh >= pivot ) {
            pivot = inHigh;
            pivotIdx = barIdx;
            barsSincePivot = 0;
         }
         cur_outZigZag = pivot;
         cur_outTrend = trend;
         cur_outPivotIdx = pivotIdx;
         out.zigZag = cur_outZigZag;
         out.trend = cur_outTrend;
         out.pivotIdx = cur_outPivotIdx;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( ZigzagOut out ) {
         requireArgument("ZIGZAG value", "out", out);
         out.zigZag = this.cur_outZigZag;
         out.trend = this.cur_outTrend;
         out.pivotIdx = this.cur_outPivotIdx;
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
      public ZigzagStream clone() {
         return new ZigzagStream(this);
      }
   }

   /**
    * The outputs of one ZIGZAG bar, written by the stream into an object the
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
   public static final class ZigzagOut {
      /** Price of the swing extreme currently being tracked. */
      public double zigZag;
      /** +1 while the leg is up, -1 while it is down. */
      public int trend;
      /** Absolute index, into the input, of the bar that extreme sits on. */
      public int pivotIdx;
   }
   private void zigzagStepImpl( ZigzagStream sp, double inHigh, double inLow )
   {
      /* Bars since the pivot's own bar, carried rather than recomputed as
       * `today - pivotIdx`: a difference of two absolute indices has no meaning
       * to a stream, which sees one bar at a time. talipp's gate is written the
       * same way (`len(input) - pivot.position`), and the counter resets with
       * the pivot so the two expressions are equal at every bar.
       *
       * barIdx is the absolute bar number the index output names, carried in
       * its own right rather than read off the loop cursor. A stream has a
       * cursor only into its own history, so the cursor cannot survive into the
       * per-bar transition; outPivotIdx still has to name the bar the pivot
       * sits on, and this is what lets it.
       */
      sp.barsSincePivot = sp.barsSincePivot + 1;
      sp.barIdx = sp.barIdx + 1;
      /* Reversal is tested BEFORE extension. On an outside bar that both makes
       * a new extreme and clears the threshold, the leg reverses and the old
       * pivot stays where it was; testing extension first would move the pivot
       * and lose the reversal, which is a whole leg of difference rather than a
       * rounding.
       */
      if( sp.trend == -1 ) {
         if( inHigh >= sp.pivot * sp.up && sp.barsSincePivot >= sp.optInMinTrendLength ) {
            sp.trend = 1;
            sp.pivot = inHigh;
            sp.pivotIdx = sp.barIdx;
            sp.barsSincePivot = 0;
         } else if( inLow <= sp.pivot ) {
            /* Ties extend. At an equal price the pivot moves to the LATER bar,
             * so outZigZag does not change and outPivotIdx does: that movement
             * is the only thing the index output carries and the price output
             * cannot.
             */
            sp.pivot = inLow;
            sp.pivotIdx = sp.barIdx;
            sp.barsSincePivot = 0;
         }
      } else if( inLow <= sp.pivot * sp.dn && sp.barsSincePivot >= sp.optInMinTrendLength ) {
         sp.trend = -1;
         sp.pivot = inLow;
         sp.pivotIdx = sp.barIdx;
         sp.barsSincePivot = 0;
      } else if( inHigh >= sp.pivot ) {
         sp.pivot = inHigh;
         sp.pivotIdx = sp.barIdx;
         sp.barsSincePivot = 0;
      }
      sp.cur_outZigZag = sp.pivot;
      sp.cur_outTrend = sp.trend;
      sp.cur_outPivotIdx = sp.pivotIdx;
   }
   private RetCode zigzagOpenImpl( ZigzagStream sp, double inHigh[], double inLow[], int startIdx, double optInSensitivity, int optInMinTrendLength, MInteger outBegIdx, MInteger outNBElement, double outZigZag[], int outTrend[], int outPivotIdx[], int outStride )
   {
      int lookbackTotal = 0;
      int today = 0;
      int outIdx = 0;
      int trend = 0;
      int pivotIdx = 0;
      int barsSincePivot = 0;
      int barIdx = 0;
      double s = 0;
      double up = 0;
      double dn = 0;
      double pivot = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSensitivity == REAL_DEFAULT ) {
         optInSensitivity = 5e0;
      } else if( !(optInSensitivity >= 0e0 && optInSensitivity <= 1e2) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMinTrendLength == Integer.MIN_VALUE ) {
         optInMinTrendLength = 1;
      } else if( optInMinTrendLength < 1 || optInMinTrendLength > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      lookbackTotal = zigzagLookback(optInSensitivity, optInMinTrendLength);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Percent in, fraction inside. Each factor is rounded once, here, rather
       * than rebuilt per bar: the threshold is `pivot * up` in binary64, which is
       * the expression order the oracle uses.
       */
      s = optInSensitivity / 100.0;
      up = 1.0 + s;
      dn = 1.0 - s;
      /* The seed: a down leg whose extreme is the low of the bar
       * optInMinTrendLength before the first output. Because the gate counts the
       * seed bar like every other pivot, the first reversal can fire exactly at
       * the first output bar, not one later.
       */
      today = startIdx - lookbackTotal;
      trend = -1;
      pivot = inLow[today];
      barIdx = today;
      pivotIdx = barIdx;
      barsSincePivot = 0;
      today = today + 1;
      outIdx = 0;
      while( today <= endIdx ) {
         /* Bars since the pivot's own bar, carried rather than recomputed as
          * `today - pivotIdx`: a difference of two absolute indices has no meaning
          * to a stream, which sees one bar at a time. talipp's gate is written the
          * same way (`len(input) - pivot.position`), and the counter resets with
          * the pivot so the two expressions are equal at every bar.
          *
          * barIdx is the absolute bar number the index output names, carried in
          * its own right rather than read off the loop cursor. A stream has a
          * cursor only into its own history, so the cursor cannot survive into the
          * per-bar transition; outPivotIdx still has to name the bar the pivot
          * sits on, and this is what lets it.
          */
         barsSincePivot = barsSincePivot + 1;
         barIdx = barIdx + 1;
         /* Reversal is tested BEFORE extension. On an outside bar that both makes
          * a new extreme and clears the threshold, the leg reverses and the old
          * pivot stays where it was; testing extension first would move the pivot
          * and lose the reversal, which is a whole leg of difference rather than a
          * rounding.
          */
         if( trend == -1 ) {
            if( inHigh[today] >= pivot * up && barsSincePivot >= optInMinTrendLength ) {
               trend = 1;
               pivot = inHigh[today];
               pivotIdx = barIdx;
               barsSincePivot = 0;
            } else if( inLow[today] <= pivot ) {
               /* Ties extend. At an equal price the pivot moves to the LATER bar,
                * so outZigZag does not change and outPivotIdx does: that movement
                * is the only thing the index output carries and the price output
                * cannot.
                */
               pivot = inLow[today];
               pivotIdx = barIdx;
               barsSincePivot = 0;
            }
         } else if( inLow[today] <= pivot * dn && barsSincePivot >= optInMinTrendLength ) {
            trend = -1;
            pivot = inLow[today];
            pivotIdx = barIdx;
            barsSincePivot = 0;
         } else if( inHigh[today] >= pivot ) {
            pivot = inHigh[today];
            pivotIdx = barIdx;
            barsSincePivot = 0;
         }
         if( today >= startIdx ) {
            outZigZag[outIdx * outStride] = pivot;
            outTrend[outIdx * outStride] = trend;
            outPivotIdx[outIdx * outStride] = pivotIdx;
            outIdx = outIdx + 1;
         }
         today = today + 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInSensitivity = optInSensitivity;
      sp.optInMinTrendLength = optInMinTrendLength;
      sp.trend = trend;
      sp.pivotIdx = pivotIdx;
      sp.barsSincePivot = barsSincePivot;
      sp.barIdx = barIdx;
      sp.up = up;
      sp.dn = dn;
      sp.pivot = pivot;
      sp.cur_outZigZag = outZigZag[(outNBElement.value - 1) * outStride];
      sp.cur_outTrend = outTrend[(outNBElement.value - 1) * outStride];
      sp.cur_outPivotIdx = outPivotIdx[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* zigzagOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   ZigzagStream zigzagOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, double optInSensitivity, int optInMinTrendLength, MInteger outBegIdx, MInteger outNBElement, double outZigZag[], int outTrend[], int outPivotIdx[] )
   {
      ZigzagStream sp = new ZigzagStream(this);
      RetCode retCode = zigzagOpenImpl(sp, inHigh, inLow, startIdx, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ZIGZAG openAndFill", inHigh.length, startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength));
      }
      throw streamFailure("ZIGZAG openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind zigzagOpen (composition seam). */
   ZigzagStream zigzagOpenInternal( double inHigh[], double inLow[], int startIdx, double optInSensitivity, int optInMinTrendLength )
   {
      ZigzagStream sp = new ZigzagStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outZigZag = new double[1];
      int[] sink_outTrend = new int[1];
      int[] sink_outPivotIdx = new int[1];
      RetCode retCode = zigzagOpenImpl(sp, inHigh, inLow, startIdx, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, sink_outZigZag, sink_outTrend, sink_outPivotIdx, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ZIGZAG open", inHigh.length, startIdx, zigzagLookback(optInSensitivity, optInMinTrendLength));
      }
      throw streamFailure("ZIGZAG open", retCode);
   }
   /**
    * Open a live ZIGZAG stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#zigzag} at that bar.
    * <p>The history must hold at least {@code zigzagLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Core#REAL_DEFAULT} and {@link Integer#MIN_VALUE} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public ZigzagStream zigzagOpen( double inHigh[], double inLow[], double optInSensitivity, int optInMinTrendLength )
   {
      requireArgument("ZIGZAG open", "inHigh", inHigh);
      requireHistory("ZIGZAG open", inHigh.length);
      requireArgument("ZIGZAG open", "inLow", inLow);
      requireHistoryLength("ZIGZAG open", "inLow", inLow.length, inHigh.length);
      return zigzagOpenInternal(inHigh, inLow, 0, optInSensitivity, optInMinTrendLength);
   }
   /**
    * {@link Core#zigzagOpen} that also fills the output array(s) bit-identically
    * to {@link Core#zigzag} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link ZigzagStream#outRange()}.
    */
   public ZigzagStream zigzagOpenAndFill( double inHigh[], double inLow[], double optInSensitivity, int optInMinTrendLength, double outZigZag[], int outTrend[], int outPivotIdx[] )
   {
      requireArgument("ZIGZAG openAndFill", "inHigh", inHigh);
      requireHistory("ZIGZAG openAndFill", inHigh.length);
      requireArgument("ZIGZAG openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("ZIGZAG openAndFill", inHigh.length, zigzagLookback(optInSensitivity, optInMinTrendLength));
      requireHistoryLength("ZIGZAG openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("ZIGZAG openAndFill", "outZigZag", outZigZag, guardOutLen);
      requireLength("ZIGZAG openAndFill", "outTrend", outTrend, guardOutLen);
      requireLength("ZIGZAG openAndFill", "outPivotIdx", outPivotIdx, guardOutLen);
      if( (Object)outZigZag == (Object)inHigh || (Object)outZigZag == (Object)inLow || (Object)outTrend == (Object)inHigh || (Object)outTrend == (Object)inLow || (Object)outPivotIdx == (Object)inHigh || (Object)outPivotIdx == (Object)inLow || (Object)outZigZag == (Object)outTrend || (Object)outZigZag == (Object)outPivotIdx || (Object)outTrend == (Object)outPivotIdx ) {
         throw streamFailure("ZIGZAG openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return zigzagOpenAndFillInternal(inHigh, inLow, 0, optInSensitivity, optInMinTrendLength, outBegIdx, outNBElement, outZigZag, outTrend, outPivotIdx);
   }
