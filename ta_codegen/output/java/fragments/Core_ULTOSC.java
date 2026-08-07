/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  DM       Drew McCormack (http://www.trade-strategist.com)
 *  MF       Mario Fortier
 *  DX       Dex Hunter (https://github.com/dexhunter)
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  281206 DM   Initial Implementation
 *  010606 MF   Abstract local arrays. Detect divide by zero.
 *  073126 DX   Evaluate each bar's terms once via a CIRCBUF ring (PR #154).
 */

   /**
    * Number of leading input bars {@link Core#ultOsc} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod1 Bars for one averaging window (default 7; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInTimePeriod2 Bars for another averaging window (default 14;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInTimePeriod3 Bars for another averaging window (default 28;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ultOscLookback( int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return -1;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return -1;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return -1;
      }
      int maxPeriod;
      /* Lookback for the Ultimate Oscillator is the lookback of the SMA with the longest
       * time period, plus 1 for the True Range.
       */
      maxPeriod = Math.max(Math.max(optInTimePeriod1, optInTimePeriod2), optInTimePeriod3);
      return smaLookback(maxPeriod) + 1 ;

   }
   RetCode ultOscInternal( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           int optInTimePeriod1,
                           int optInTimePeriod2,
                           int optInTimePeriod3,
                           MInteger outBegIdx,
                           MInteger outNBElement,
                           double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingPos1 = 0;
      int trailingPos2 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return RetCode.BadParam;
      }
      /* The two per-bar terms the three moving sums are built from. Both are a
       * pure function of the bar, so each bar is evaluated once on entry and read
       * back when it leaves each of the three windows.
       */
      /* One entry per bar of the longest window. Stays on the stack for every
       * period up to 32, which covers the 7/14/28 default.
       */
      /* Id, Type, Static Size */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Ensure that the time periods are ordered from shortest to longest.
       * Sort.
       */
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      /* Adjust startIdx for lookback period. */
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod3 < 1 ) return RetCode.InternalError;
      term_closeMinusTrueLow = new double[optInTimePeriod3];
      term_trueRange = new double[optInTimePeriod3];
      maxIdx_term = (optInTimePeriod3)-1;
      term_Idx = 0;
      /* Prime running totals used in moving averages.
       *
       * One pass over the longest warm-up window replaces three overlapping
       * passes. A bar inside the shorter windows is added to those totals as it
       * is reached, so every total still accumulates exactly the same bars in
       * exactly the same ascending order as three separate loops did.
       */
      a1Total = 0;
      b1Total = 0;
      a2Total = 0;
      b2Total = 0;
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         if( i >= startIdx - optInTimePeriod1 + 1 ) {
            a1Total += closeMinusTrueLow;
            b1Total += trueRange;
         }
         if( i >= startIdx - optInTimePeriod2 + 1 ) {
            a2Total += closeMinusTrueLow;
            b2Total += trueRange;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      /* Calculate oscillator */
      today = startIdx;
      outIdx = 0;
      /* The warm-up wrote optInTimePeriod3-1 bars, so term_Idx is the slot for
       * `today` and, once advanced past it, the slot of the bar leaving the
       * longest window. The two shorter windows trail it by a fixed offset.
       */
      trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
      if( trailingPos1 >= optInTimePeriod3 ) {
         trailingPos1 -= optInTimePeriod3;
      }
      trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
      if( trailingPos2 >= optInTimePeriod3 ) {
         trailingPos2 -= optInTimePeriod3;
      }
      while( today <= endIdx ) {
         /* Add on today's terms */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         /* Calculate the oscillator value for today */
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         /* Remove the trailing terms to prepare for next day. Each was evaluated
          * once, when its bar entered the ring.
          */
         a1Total -= term_closeMinusTrueLow[trailingPos1];
         b1Total -= term_trueRange[trailingPos1];
         trailingPos1 += 1;
         if( trailingPos1 >= optInTimePeriod3 ) {
            trailingPos1 = 0;
         }
         a2Total -= term_closeMinusTrueLow[trailingPos2];
         b2Total -= term_trueRange[trailingPos2];
         trailingPos2 += 1;
         if( trailingPos2 >= optInTimePeriod3 ) {
            trailingPos2 = 0;
         }
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         a3Total -= term_closeMinusTrueLow[term_Idx];
         b3Total -= term_trueRange[term_Idx];
         /* Last operation is to write the output. Must
          * be done after the trailing index have all been
          * taken care of because the caller is allowed
          * to have the input array to be also the output
          * array.
          */
         outReal[outIdx] = 100.0 * (output / 7.0);
         /* Increment indexes */
         outIdx += 1;
         today += 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode ultOscInternal( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           int optInTimePeriod1,
                           int optInTimePeriod2,
                           int optInTimePeriod3,
                           MInteger outBegIdx,
                           MInteger outNBElement,
                           double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingPos1 = 0;
      int trailingPos2 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      if( optInTimePeriod3 < 1 ) return RetCode.InternalError;
      term_closeMinusTrueLow = new double[optInTimePeriod3];
      term_trueRange = new double[optInTimePeriod3];
      maxIdx_term = (optInTimePeriod3)-1;
      term_Idx = 0;
      a1Total = 0;
      b1Total = 0;
      a2Total = 0;
      b2Total = 0;
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = (double)inLow[i];
         tempHT = (double)inHigh[i];
         tempCY = (double)inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         if( i >= startIdx - optInTimePeriod1 + 1 ) {
            a1Total += closeMinusTrueLow;
            b1Total += trueRange;
         }
         if( i >= startIdx - optInTimePeriod2 + 1 ) {
            a2Total += closeMinusTrueLow;
            b2Total += trueRange;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      today = startIdx;
      outIdx = 0;
      trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
      if( trailingPos1 >= optInTimePeriod3 ) {
         trailingPos1 -= optInTimePeriod3;
      }
      trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
      if( trailingPos2 >= optInTimePeriod3 ) {
         trailingPos2 -= optInTimePeriod3;
      }
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = (double)inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         a1Total -= term_closeMinusTrueLow[trailingPos1];
         b1Total -= term_trueRange[trailingPos1];
         trailingPos1 += 1;
         if( trailingPos1 >= optInTimePeriod3 ) {
            trailingPos1 = 0;
         }
         a2Total -= term_closeMinusTrueLow[trailingPos2];
         b2Total -= term_trueRange[trailingPos2];
         trailingPos2 += 1;
         if( trailingPos2 >= optInTimePeriod3 ) {
            trailingPos2 = 0;
         }
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         a3Total -= term_closeMinusTrueLow[term_Idx];
         b3Total -= term_trueRange[term_Idx];
         outReal[outIdx] = 100.0 * (output / 7.0);
         outIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Ultimate Oscillator: momentum indicator combining
    * buying-pressure/true-range ratios over three time periods into one 0-100
    * weighted average. Blends short-, medium-, and long-term momentum to damp
    * single-period noise. Ranges 0-100; conventionally &gt;70 overbought,
    * &lt;30 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * trueLow = min(low, prevClose);  BP = close - trueLow
    * TR = max(high-low, |prevClose-high|, |prevClose-low|)
    * avg_n = (sum BP over n bars) / (sum TR over n bars)
    * ULTOSC = 100 * (4*avg_short + 2*avg_mid + avg_long) / 7
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The three periods are sorted internally, so the 4/2/1 weighting always applies to the shortest, middle, and longest period regardless of the order in which you pass them.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ultOscLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod1 Bars for one averaging window (default 7; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInTimePeriod2 Bars for another averaging window (default 14;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInTimePeriod3 Bars for another averaging window (default 28;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Ultimate Oscillator value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#atr
    * @see Core#trueRange
    * @see Core#rsi
    */
   public OutRange ultOsc( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           int optInTimePeriod1,
                           int optInTimePeriod2,
                           int optInTimePeriod3,
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ultOscInternal(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ULTOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Ultimate Oscillator: momentum indicator combining
    * buying-pressure/true-range ratios over three time periods into one 0-100
    * weighted average. Blends short-, medium-, and long-term momentum to damp
    * single-period noise. Ranges 0-100; conventionally &gt;70 overbought,
    * &lt;30 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * trueLow = min(low, prevClose);  BP = close - trueLow
    * TR = max(high-low, |prevClose-high|, |prevClose-low|)
    * avg_n = (sum BP over n bars) / (sum TR over n bars)
    * ULTOSC = 100 * (4*avg_short + 2*avg_mid + avg_long) / 7
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>The three periods are sorted internally, so the 4/2/1 weighting always applies to the shortest, middle, and longest period regardless of the order in which you pass them.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#ultOscLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInTimePeriod1 Bars for one averaging window (default 7; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInTimePeriod2 Bars for another averaging window (default 14;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInTimePeriod3 Bars for another averaging window (default 28;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Ultimate Oscillator value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#atr
    * @see Core#trueRange
    * @see Core#rsi
    */
   public OutRange ultOsc( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           int optInTimePeriod1,
                           int optInTimePeriod2,
                           int optInTimePeriod3,
                           double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ultOscInternal(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ULTOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ULTOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ultOsc} over the same series.
    * Open with {@link Core#ultOscOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class UltOscStream {
      final Core core;
      int optInTimePeriod1;
      int optInTimePeriod2;
      int optInTimePeriod3;
      double a1Total;
      double a2Total;
      double a3Total;
      double b1Total;
      double b2Total;
      double b3Total;
      double output;
      int trailingPos1;
      int trailingPos2;
      int term_Idx;
      int maxIdx_term;
      double lag1_inClose;
      int cbSize_term;
      double[] cb_term_closeMinusTrueLow;
      double[] cb_term_trueRange;
      double cur_outReal;
      OutRange fillRange;

      UltOscStream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#ultOscOpenAndFill}, or {@code null}
       * when this handle came from a plain {@code open} (which fills nothing).
       */
      public OutRange fillRange() { return fillRange; }

      UltOscStream( UltOscStream other ) {
         this.core = other.core;
         this.optInTimePeriod1 = other.optInTimePeriod1;
         this.optInTimePeriod2 = other.optInTimePeriod2;
         this.optInTimePeriod3 = other.optInTimePeriod3;
         this.a1Total = other.a1Total;
         this.a2Total = other.a2Total;
         this.a3Total = other.a3Total;
         this.b1Total = other.b1Total;
         this.b2Total = other.b2Total;
         this.b3Total = other.b3Total;
         this.output = other.output;
         this.trailingPos1 = other.trailingPos1;
         this.trailingPos2 = other.trailingPos2;
         this.term_Idx = other.term_Idx;
         this.maxIdx_term = other.maxIdx_term;
         this.lag1_inClose = other.lag1_inClose;
         this.cbSize_term = other.cbSize_term;
         this.cb_term_closeMinusTrueLow = other.cb_term_closeMinusTrueLow.clone();
         this.cb_term_trueRange = other.cb_term_trueRange.clone();
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inHigh, double inLow, double inClose ) {
         core.ultOscStreamStep(this, inHigh, inLow, inClose);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         UltOscStream scratch = new UltOscStream(this);
         core.ultOscStreamStep(scratch, inHigh, inLow, inClose);
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
      public UltOscStream copy() {
         return new UltOscStream(this);
      }
   }
   void ultOscStreamStep( UltOscStream sp, double inHigh, double inLow, double inClose )
   {
      double trueLow = 0.0;
      double trueRange = 0.0;
      double closeMinusTrueLow = 0.0;
      double tempDouble = 0.0;
      double tempHT = 0.0;
      double tempLT = 0.0;
      double tempCY = 0.0;
      /* Add on today's terms */
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      trueLow = Math.min(tempLT, tempCY);
      closeMinusTrueLow = inClose - trueLow;
      trueRange = tempHT - tempLT;
      tempDouble = Math.abs(tempCY - tempHT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      tempDouble = Math.abs(tempCY - tempLT);
      if( tempDouble > trueRange ) {
         trueRange = tempDouble;
      }
      sp.cb_term_closeMinusTrueLow[sp.term_Idx] = closeMinusTrueLow;
      sp.cb_term_trueRange[sp.term_Idx] = trueRange;
      sp.a1Total += closeMinusTrueLow;
      sp.a2Total += closeMinusTrueLow;
      sp.a3Total += closeMinusTrueLow;
      sp.b1Total += trueRange;
      sp.b2Total += trueRange;
      sp.b3Total += trueRange;
      /* Calculate the oscillator value for today */
      sp.output = 0.0;
      if( !((-0.00000000000001 < sp.b1Total) && (sp.b1Total < 0.00000000000001)) ) {
         sp.output += 4.0 * (sp.a1Total / sp.b1Total);
      }
      if( !((-0.00000000000001 < sp.b2Total) && (sp.b2Total < 0.00000000000001)) ) {
         sp.output += 2.0 * (sp.a2Total / sp.b2Total);
      }
      if( !((-0.00000000000001 < sp.b3Total) && (sp.b3Total < 0.00000000000001)) ) {
         sp.output += sp.a3Total / sp.b3Total;
      }
      /* Remove the trailing terms to prepare for next day. Each was evaluated
       * once, when its bar entered the ring.
       */
      sp.a1Total -= sp.cb_term_closeMinusTrueLow[sp.trailingPos1];
      sp.b1Total -= sp.cb_term_trueRange[sp.trailingPos1];
      sp.trailingPos1 += 1;
      if( sp.trailingPos1 >= sp.optInTimePeriod3 ) {
         sp.trailingPos1 = 0;
      }
      sp.a2Total -= sp.cb_term_closeMinusTrueLow[sp.trailingPos2];
      sp.b2Total -= sp.cb_term_trueRange[sp.trailingPos2];
      sp.trailingPos2 += 1;
      if( sp.trailingPos2 >= sp.optInTimePeriod3 ) {
         sp.trailingPos2 = 0;
      }
      sp.term_Idx = sp.term_Idx + 1;
      if( sp.term_Idx > sp.maxIdx_term ) {
         sp.term_Idx = 0;
      }
      sp.a3Total -= sp.cb_term_closeMinusTrueLow[sp.term_Idx];
      sp.b3Total -= sp.cb_term_trueRange[sp.term_Idx];
      /* Last operation is to write the output. Must
       * be done after the trailing index have all been
       * taken care of because the caller is allowed
       * to have the input array to be also the output
       * array.
       */
      sp.cur_outReal = 100.0 * (sp.output / 7.0);
      /* Increment indexes */
      sp.lag1_inClose = inClose;
   }
   private RetCode ultOscOpenBody( UltOscStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingPos1 = 0;
      int trailingPos2 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return RetCode.BadParam;
      }
      /* The two per-bar terms the three moving sums are built from. Both are a
       * pure function of the bar, so each bar is evaluated once on entry and read
       * back when it leaves each of the three windows.
       */
      /* One entry per bar of the longest window. Stays on the stack for every
       * period up to 32, which covers the 7/14/28 default.
       */
      /* Id, Type, Static Size */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Ensure that the time periods are ordered from shortest to longest.
       * Sort.
       */
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      /* Adjust startIdx for lookback period. */
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod3 < 1 ) return RetCode.InternalError;
      term_closeMinusTrueLow = new double[optInTimePeriod3];
      term_trueRange = new double[optInTimePeriod3];
      maxIdx_term = (optInTimePeriod3)-1;
      term_Idx = 0;
      /* Prime running totals used in moving averages.
       *
       * One pass over the longest warm-up window replaces three overlapping
       * passes. A bar inside the shorter windows is added to those totals as it
       * is reached, so every total still accumulates exactly the same bars in
       * exactly the same ascending order as three separate loops did.
       */
      a1Total = 0;
      b1Total = 0;
      a2Total = 0;
      b2Total = 0;
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         if( i >= startIdx - optInTimePeriod1 + 1 ) {
            a1Total += closeMinusTrueLow;
            b1Total += trueRange;
         }
         if( i >= startIdx - optInTimePeriod2 + 1 ) {
            a2Total += closeMinusTrueLow;
            b2Total += trueRange;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      /* Calculate oscillator */
      today = startIdx;
      outIdx = 0;
      /* The warm-up wrote optInTimePeriod3-1 bars, so term_Idx is the slot for
       * `today` and, once advanced past it, the slot of the bar leaving the
       * longest window. The two shorter windows trail it by a fixed offset.
       */
      trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
      if( trailingPos1 >= optInTimePeriod3 ) {
         trailingPos1 -= optInTimePeriod3;
      }
      trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
      if( trailingPos2 >= optInTimePeriod3 ) {
         trailingPos2 -= optInTimePeriod3;
      }
      while( today <= endIdx ) {
         /* Add on today's terms */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         /* Calculate the oscillator value for today */
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         /* Remove the trailing terms to prepare for next day. Each was evaluated
          * once, when its bar entered the ring.
          */
         a1Total -= term_closeMinusTrueLow[trailingPos1];
         b1Total -= term_trueRange[trailingPos1];
         trailingPos1 += 1;
         if( trailingPos1 >= optInTimePeriod3 ) {
            trailingPos1 = 0;
         }
         a2Total -= term_closeMinusTrueLow[trailingPos2];
         b2Total -= term_trueRange[trailingPos2];
         trailingPos2 += 1;
         if( trailingPos2 >= optInTimePeriod3 ) {
            trailingPos2 = 0;
         }
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         a3Total -= term_closeMinusTrueLow[term_Idx];
         b3Total -= term_trueRange[term_Idx];
         /* Last operation is to write the output. Must
          * be done after the trailing index have all been
          * taken care of because the caller is allowed
          * to have the input array to be also the output
          * array.
          */
         lastValue_outReal = 100.0 * (output / 7.0);
         /* Increment indexes */
         outIdx += 1;
         today += 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_term = maxIdx_term + 1;
      if( capCb_term > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod1 = optInTimePeriod1;
      sp.optInTimePeriod2 = optInTimePeriod2;
      sp.optInTimePeriod3 = optInTimePeriod3;
      sp.a1Total = a1Total;
      sp.a2Total = a2Total;
      sp.a3Total = a3Total;
      sp.b1Total = b1Total;
      sp.b2Total = b2Total;
      sp.b3Total = b3Total;
      sp.output = output;
      sp.trailingPos1 = trailingPos1;
      sp.trailingPos2 = trailingPos2;
      sp.term_Idx = term_Idx;
      sp.maxIdx_term = maxIdx_term;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cbSize_term = capCb_term;
      sp.cb_term_closeMinusTrueLow = term_closeMinusTrueLow;
      sp.cb_term_trueRange = term_trueRange;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode ultOscOpenAndFillBody( UltOscStream sp, double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      double a1Total = 0;
      double a2Total = 0;
      double a3Total = 0;
      double b1Total = 0;
      double b2Total = 0;
      double b3Total = 0;
      double trueLow = 0;
      double trueRange = 0;
      double closeMinusTrueLow = 0;
      double tempDouble = 0;
      double output = 0;
      double tempHT = 0;
      double tempLT = 0;
      double tempCY = 0;
      int lookbackTotal = 0;
      int longestPeriod = 0;
      int longestIndex = 0;
      int i = 0;
      int j = 0;
      int today = 0;
      int outIdx = 0;
      int trailingPos1 = 0;
      int trailingPos2 = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod1 == Integer.MIN_VALUE ) {
         optInTimePeriod1 = 7;
      } else if( optInTimePeriod1 < 1 || optInTimePeriod1 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod2 == Integer.MIN_VALUE ) {
         optInTimePeriod2 = 14;
      } else if( optInTimePeriod2 < 1 || optInTimePeriod2 > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod3 == Integer.MIN_VALUE ) {
         optInTimePeriod3 = 28;
      } else if( optInTimePeriod3 < 1 || optInTimePeriod3 > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         return RetCode.BadParam;
      }
      /* The two per-bar terms the three moving sums are built from. Both are a
       * pure function of the bar, so each bar is evaluated once on entry and read
       * back when it leaves each of the three windows.
       */
      /* One entry per bar of the longest window. Stays on the stack for every
       * period up to 32, which covers the 7/14/28 default.
       */
      /* Id, Type, Static Size */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Ensure that the time periods are ordered from shortest to longest.
       * Sort.
       */
      periods[0] = optInTimePeriod1;
      periods[1] = optInTimePeriod2;
      periods[2] = optInTimePeriod3;
      usedFlag[0] = 0;
      usedFlag[1] = 0;
      usedFlag[2] = 0;
      for( i = 0; i < 3; i += 1 ) {
         longestPeriod = 0;
         longestIndex = 0;
         for( j = 0; j < 3; j += 1 ) {
            if( usedFlag[j] == 0 && periods[j] > longestPeriod ) {
               longestPeriod = periods[j];
               longestIndex = j;
            }
         }
         usedFlag[longestIndex] = 1;
         sortedPeriods[i] = longestPeriod;
      }
      optInTimePeriod1 = sortedPeriods[2];
      optInTimePeriod2 = sortedPeriods[1];
      optInTimePeriod3 = sortedPeriods[0];
      /* Adjust startIdx for lookback period. */
      lookbackTotal = ultOscLookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod3 < 1 ) return RetCode.InternalError;
      term_closeMinusTrueLow = new double[optInTimePeriod3];
      term_trueRange = new double[optInTimePeriod3];
      maxIdx_term = (optInTimePeriod3)-1;
      term_Idx = 0;
      /* Prime running totals used in moving averages.
       *
       * One pass over the longest warm-up window replaces three overlapping
       * passes. A bar inside the shorter windows is added to those totals as it
       * is reached, so every total still accumulates exactly the same bars in
       * exactly the same ascending order as three separate loops did.
       */
      a1Total = 0;
      b1Total = 0;
      a2Total = 0;
      b2Total = 0;
      a3Total = 0;
      b3Total = 0;
      for( i = startIdx - optInTimePeriod3 + 1; i < startIdx; i += 1 ) {
         tempLT = inLow[i];
         tempHT = inHigh[i];
         tempCY = inClose[i - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[i] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         if( i >= startIdx - optInTimePeriod1 + 1 ) {
            a1Total += closeMinusTrueLow;
            b1Total += trueRange;
         }
         if( i >= startIdx - optInTimePeriod2 + 1 ) {
            a2Total += closeMinusTrueLow;
            b2Total += trueRange;
         }
         a3Total += closeMinusTrueLow;
         b3Total += trueRange;
      }
      /* Calculate oscillator */
      today = startIdx;
      outIdx = 0;
      /* The warm-up wrote optInTimePeriod3-1 bars, so term_Idx is the slot for
       * `today` and, once advanced past it, the slot of the bar leaving the
       * longest window. The two shorter windows trail it by a fixed offset.
       */
      trailingPos1 = term_Idx + optInTimePeriod3 - optInTimePeriod1 + 1;
      if( trailingPos1 >= optInTimePeriod3 ) {
         trailingPos1 -= optInTimePeriod3;
      }
      trailingPos2 = term_Idx + optInTimePeriod3 - optInTimePeriod2 + 1;
      if( trailingPos2 >= optInTimePeriod3 ) {
         trailingPos2 -= optInTimePeriod3;
      }
      while( today <= endIdx ) {
         /* Add on today's terms */
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         trueLow = Math.min(tempLT, tempCY);
         closeMinusTrueLow = inClose[today] - trueLow;
         trueRange = tempHT - tempLT;
         tempDouble = Math.abs(tempCY - tempHT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         tempDouble = Math.abs(tempCY - tempLT);
         if( tempDouble > trueRange ) {
            trueRange = tempDouble;
         }
         term_closeMinusTrueLow[term_Idx] = closeMinusTrueLow;
         term_trueRange[term_Idx] = trueRange;
         a1Total += closeMinusTrueLow;
         a2Total += closeMinusTrueLow;
         a3Total += closeMinusTrueLow;
         b1Total += trueRange;
         b2Total += trueRange;
         b3Total += trueRange;
         /* Calculate the oscillator value for today */
         output = 0.0;
         if( !((-0.00000000000001 < b1Total) && (b1Total < 0.00000000000001)) ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( !((-0.00000000000001 < b2Total) && (b2Total < 0.00000000000001)) ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( !((-0.00000000000001 < b3Total) && (b3Total < 0.00000000000001)) ) {
            output += a3Total / b3Total;
         }
         /* Remove the trailing terms to prepare for next day. Each was evaluated
          * once, when its bar entered the ring.
          */
         a1Total -= term_closeMinusTrueLow[trailingPos1];
         b1Total -= term_trueRange[trailingPos1];
         trailingPos1 += 1;
         if( trailingPos1 >= optInTimePeriod3 ) {
            trailingPos1 = 0;
         }
         a2Total -= term_closeMinusTrueLow[trailingPos2];
         b2Total -= term_trueRange[trailingPos2];
         trailingPos2 += 1;
         if( trailingPos2 >= optInTimePeriod3 ) {
            trailingPos2 = 0;
         }
         term_Idx++;
         if( term_Idx > maxIdx_term ) { term_Idx = 0; }
         a3Total -= term_closeMinusTrueLow[term_Idx];
         b3Total -= term_trueRange[term_Idx];
         /* Last operation is to write the output. Must
          * be done after the trailing index have all been
          * taken care of because the caller is allowed
          * to have the input array to be also the output
          * array.
          */
         outReal[outIdx] = 100.0 * (output / 7.0);
         /* Increment indexes */
         outIdx += 1;
         today += 1;
      }
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_term = maxIdx_term + 1;
      if( capCb_term > historyLen + 1 ) {
         return RetCode.InternalError;
      }
      sp.optInTimePeriod1 = optInTimePeriod1;
      sp.optInTimePeriod2 = optInTimePeriod2;
      sp.optInTimePeriod3 = optInTimePeriod3;
      sp.a1Total = a1Total;
      sp.a2Total = a2Total;
      sp.a3Total = a3Total;
      sp.b1Total = b1Total;
      sp.b2Total = b2Total;
      sp.b3Total = b3Total;
      sp.output = output;
      sp.trailingPos1 = trailingPos1;
      sp.trailingPos2 = trailingPos2;
      sp.term_Idx = term_Idx;
      sp.maxIdx_term = maxIdx_term;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cbSize_term = capCb_term;
      sp.cb_term_closeMinusTrueLow = term_closeMinusTrueLow;
      sp.cb_term_trueRange = term_trueRange;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind ultOscOpen (composition seam). */
   UltOscStream ultOscOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      UltOscStream sp = new UltOscStream(this);
      RetCode retCode = ultOscOpenBody(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ULTOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ULTOSC open: internal error");
      }
      throw new IllegalArgumentException("TA_ULTOSC open: " + retCode);
   }
   /**
    * Open a live ULTOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ultOsc} at that bar.
    * <p>The history must hold at least {@code ultOscLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public UltOscStream ultOscOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      return ultOscOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
   }
   /**
    * {@link Core#ultOscOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ultOsc} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link UltOscStream#fillRange()}.
    */
   public UltOscStream ultOscOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, double outReal[] )
   {
      UltOscStream sp = new UltOscStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ultOscOpenAndFillBody(sp, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_ULTOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_ULTOSC openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_ULTOSC openAndFill: " + retCode);
   }
