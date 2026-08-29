/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  DM       Drew McCormack (http://www.trade-strategist.com)
 *  MF       Mario Fortier
 *  DX       Dex Hunter (https://github.com/dexhunter)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  281206 DM    Initial Implementation
 *  010606 MF    Abstract local arrays. Detect divide by zero.
 *  073126 DX    Evaluate each bar's terms once via a CIRCBUF ring (PR #154).
 *  082326 MF,CC Fix #253. Recognize an empty window by counting bars, so the
 *               divides are guarded exactly instead of against the fixed
 *               TA_IS_ZERO band -- which zeroed the oscillator for any
 *               instrument quoted small enough to fall under it.
 */

   /**
    * Number of leading input bars {@link Core#ULTOSC} consumes before it can
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
   public int ULTOSC_Lookback( int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
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
      return SMA_Lookback(maxPeriod) + 1 ;

   }
   RetCode ULTOSC_Impl( int startIdx,
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
      int nullRun = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      lookbackTotal = ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
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
      /* Consecutive bars that put nothing into the windows, counted so that an
       * empty window can be recognized exactly (the shape #244 needed for MFI).
       * The running totals cannot answer that question themselves: they are
       * maintained by add-then-subtract, so once a window empties they hold
       * rounding residue of arbitrary sign rather than zero, and v0.6.4 divides
       * one residue by another there -- it returns -92.9 for an oscillator
       * documented to run 0..100. Both of a bar's terms have to be zero for it to
       * count, which for valid bars is one condition (a zero true range means
       * H == L == the previous close, which leaves the close on the true low).
       * Reseeding on the count is what lets the divides below be guarded exactly
       * rather than against a fixed band -- a true range carries the quote unit,
       * so the band they used to carry zeroed the oscillator for any instrument
       * quoted below it (issue #253).
       */
      nullRun = 0;
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
         if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
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
         /* Once a whole window of no-contribution bars has gone by, every slot it
          * spans is 0.0, so its totals are known to be exactly zero and the
          * residue can be dropped. The periods are sorted shortest-first, so a
          * run long enough for a longer window is long enough for every shorter
          * one.
          */
         if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod1 ) {
            a1Total = 0.0;
            b1Total = 0.0;
            if( nullRun >= optInTimePeriod2 ) {
               a2Total = 0.0;
               b2Total = 0.0;
               if( nullRun >= optInTimePeriod3 ) {
                  nullRun = optInTimePeriod3;
                  a3Total = 0.0;
                  b3Total = 0.0;
               }
            }
         }
         /* Calculate the oscillator value for today. Each window contributes only
          * when it holds a true range; the totals are sums of non-negative terms
          * and the reseed above removes their residue, so the test is exact.
          */
         output = 0.0;
         if( b1Total > 0.0 ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( b2Total > 0.0 ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( b3Total > 0.0 ) {
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
   RetCode ULTOSC_Impl( int startIdx,
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
      int nullRun = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      lookbackTotal = ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
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
      nullRun = 0;
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
         if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
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
         if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod1 ) {
            a1Total = 0.0;
            b1Total = 0.0;
            if( nullRun >= optInTimePeriod2 ) {
               a2Total = 0.0;
               b2Total = 0.0;
               if( nullRun >= optInTimePeriod3 ) {
                  nullRun = optInTimePeriod3;
                  a3Total = 0.0;
                  b3Total = 0.0;
               }
            }
         }
         output = 0.0;
         if( b1Total > 0.0 ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( b2Total > 0.0 ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( b3Total > 0.0 ) {
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
    * valid range shorter than {@link Core#ULTOSC_Lookback} is a <b>success with
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
    * @see Core#TRANGE
    * @see Core#RSI
    */
   public OutRange ULTOSC( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           int optInTimePeriod1,
                           int optInTimePeriod2,
                           int optInTimePeriod3,
                           double outReal[] )
   {
      requireIndexRange("ULTOSC", startIdx, endIdx);
      int guardStart = clampedStart("ULTOSC", startIdx, ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ULTOSC", "inHigh", inHigh, guardInLen);
      requireLength("ULTOSC", "inLow", inLow, guardInLen);
      requireLength("ULTOSC", "inClose", inClose, guardInLen);
      requireLength("ULTOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ULTOSC_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
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
    * valid range shorter than {@link Core#ULTOSC_Lookback} is a <b>success with
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
    * @see Core#TRANGE
    * @see Core#RSI
    */
   public OutRange ULTOSC( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           int optInTimePeriod1,
                           int optInTimePeriod2,
                           int optInTimePeriod3,
                           double outReal[] )
   {
      requireIndexRange("ULTOSC", startIdx, endIdx);
      int guardStart = clampedStart("ULTOSC", startIdx, ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ULTOSC", "inHigh", inHigh, guardInLen);
      requireLength("ULTOSC", "inLow", inLow, guardInLen);
      requireLength("ULTOSC", "inClose", inClose, guardInLen);
      requireLength("ULTOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ULTOSC_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("ULTOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ULTOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ULTOSC} over the same series.
    * Open with {@link Core#ultoscOpen}; there is no close — the handle is
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
   public static final class UltoscStream {
      Core core;
      int optInTimePeriod1;
      int optInTimePeriod2;
      int optInTimePeriod3;
      double a1Total;
      double a2Total;
      double a3Total;
      double b1Total;
      double b2Total;
      double b3Total;
      int trailingPos1;
      int trailingPos2;
      int nullRun;
      int term_Idx;
      int maxIdx_term;
      double lag1_inClose;
      int cbSize_term;
      double[] cb_term_closeMinusTrueLow;
      double[] cb_term_trueRange;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      UltoscStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ULTOSC} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      UltoscStream( UltoscStream other ) {
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
         this.trailingPos1 = other.trailingPos1;
         this.trailingPos2 = other.trailingPos2;
         this.nullRun = other.nullRun;
         this.term_Idx = other.term_Idx;
         this.maxIdx_term = other.maxIdx_term;
         this.lag1_inClose = other.lag1_inClose;
         this.cbSize_term = other.cbSize_term;
         this.cb_term_closeMinusTrueLow = other.cb_term_closeMinusTrueLow.clone();
         this.cb_term_trueRange = other.cb_term_trueRange.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( UltoscStream other ) {
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
         this.trailingPos1 = other.trailingPos1;
         this.trailingPos2 = other.trailingPos2;
         this.nullRun = other.nullRun;
         this.term_Idx = other.term_Idx;
         this.maxIdx_term = other.maxIdx_term;
         this.lag1_inClose = other.lag1_inClose;
         this.cbSize_term = other.cbSize_term;
         if( this.cb_term_closeMinusTrueLow != null && this.cb_term_closeMinusTrueLow.length == other.cb_term_closeMinusTrueLow.length ) {
            System.arraycopy( other.cb_term_closeMinusTrueLow, 0, this.cb_term_closeMinusTrueLow, 0, other.cb_term_closeMinusTrueLow.length );
         } else {
            this.cb_term_closeMinusTrueLow = other.cb_term_closeMinusTrueLow.clone();
         }
         if( this.cb_term_trueRange != null && this.cb_term_trueRange.length == other.cb_term_trueRange.length ) {
            System.arraycopy( other.cb_term_trueRange, 0, this.cb_term_trueRange, 0, other.cb_term_trueRange.length );
         } else {
            this.cb_term_trueRange = other.cb_term_trueRange.clone();
         }
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<UltoscStream> PEEK_SCRATCH = new ThreadLocal<>();

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
            throw new TaLibArgumentException("ULTOSC update: BadParam", RetCode.BadParam);
         core.ultoscStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outReal[] ) {
         requireArgument("ULTOSC updateAndFill", "inHigh", inHigh);
         requireArgument("ULTOSC updateAndFill", "inLow", inLow);
         requireArgument("ULTOSC updateAndFill", "inClose", inClose);
         requireArgument("ULTOSC updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose )
            throw new TaLibArgumentException("ULTOSC updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
               throw new TaLibArgumentException("ULTOSC updateAndFill: BadParam", RetCode.BadParam);
            core.ultoscStepImpl(this, inHigh[i], inLow[i], inClose[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public double peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("ULTOSC peek: BadParam", RetCode.BadParam);
         UltoscStream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new UltoscStream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.ultoscStepImpl(scratch, inHigh, inLow, inClose);
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
      public UltoscStream copy() {
         return new UltoscStream(this);
      }
   }
   void ultoscStepImpl( UltoscStream sp, double inHigh, double inLow, double inClose )
   {
      double trueLow = 0.0;
      double trueRange = 0.0;
      double closeMinusTrueLow = 0.0;
      double tempDouble = 0.0;
      double output = 0.0;
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
      /* Once a whole window of no-contribution bars has gone by, every slot it
       * spans is 0.0, so its totals are known to be exactly zero and the
       * residue can be dropped. The periods are sorted shortest-first, so a
       * run long enough for a longer window is long enough for every shorter
       * one.
       */
      if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
         sp.nullRun += 1;
      } else {
         sp.nullRun = 0;
      }
      if( sp.nullRun >= sp.optInTimePeriod1 ) {
         sp.a1Total = 0.0;
         sp.b1Total = 0.0;
         if( sp.nullRun >= sp.optInTimePeriod2 ) {
            sp.a2Total = 0.0;
            sp.b2Total = 0.0;
            if( sp.nullRun >= sp.optInTimePeriod3 ) {
               sp.nullRun = sp.optInTimePeriod3;
               sp.a3Total = 0.0;
               sp.b3Total = 0.0;
            }
         }
      }
      /* Calculate the oscillator value for today. Each window contributes only
       * when it holds a true range; the totals are sums of non-negative terms
       * and the reseed above removes their residue, so the test is exact.
       */
      output = 0.0;
      if( sp.b1Total > 0.0 ) {
         output += 4.0 * (sp.a1Total / sp.b1Total);
      }
      if( sp.b2Total > 0.0 ) {
         output += 2.0 * (sp.a2Total / sp.b2Total);
      }
      if( sp.b3Total > 0.0 ) {
         output += sp.a3Total / sp.b3Total;
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
      sp.cur_outReal = 100.0 * (output / 7.0);
      /* Increment indexes */
      sp.lag1_inClose = inClose;
   }
   private RetCode ultoscOpenImpl( UltoscStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      int nullRun = 0;
      int[] usedFlag = new int[3];
      int[] periods = new int[3];
      int[] sortedPeriods = new int[3];
      double[] term_closeMinusTrueLow;
      double[] term_trueRange;
      int term_Idx = 0;
      int maxIdx_term = (32)-1;
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
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
      lookbackTotal = ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
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
      /* Consecutive bars that put nothing into the windows, counted so that an
       * empty window can be recognized exactly (the shape #244 needed for MFI).
       * The running totals cannot answer that question themselves: they are
       * maintained by add-then-subtract, so once a window empties they hold
       * rounding residue of arbitrary sign rather than zero, and v0.6.4 divides
       * one residue by another there -- it returns -92.9 for an oscillator
       * documented to run 0..100. Both of a bar's terms have to be zero for it to
       * count, which for valid bars is one condition (a zero true range means
       * H == L == the previous close, which leaves the close on the true low).
       * Reseeding on the count is what lets the divides below be guarded exactly
       * rather than against a fixed band -- a true range carries the quote unit,
       * so the band they used to carry zeroed the oscillator for any instrument
       * quoted below it (issue #253).
       */
      nullRun = 0;
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
         if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
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
         /* Once a whole window of no-contribution bars has gone by, every slot it
          * spans is 0.0, so its totals are known to be exactly zero and the
          * residue can be dropped. The periods are sorted shortest-first, so a
          * run long enough for a longer window is long enough for every shorter
          * one.
          */
         if( trueRange == 0.0 && closeMinusTrueLow == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod1 ) {
            a1Total = 0.0;
            b1Total = 0.0;
            if( nullRun >= optInTimePeriod2 ) {
               a2Total = 0.0;
               b2Total = 0.0;
               if( nullRun >= optInTimePeriod3 ) {
                  nullRun = optInTimePeriod3;
                  a3Total = 0.0;
                  b3Total = 0.0;
               }
            }
         }
         /* Calculate the oscillator value for today. Each window contributes only
          * when it holds a true range; the totals are sums of non-negative terms
          * and the reseed above removes their residue, so the test is exact.
          */
         output = 0.0;
         if( b1Total > 0.0 ) {
            output += 4.0 * (a1Total / b1Total);
         }
         if( b2Total > 0.0 ) {
            output += 2.0 * (a2Total / b2Total);
         }
         if( b3Total > 0.0 ) {
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
         outReal[outIdx * outStride] = 100.0 * (output / 7.0);
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
      sp.trailingPos1 = trailingPos1;
      sp.trailingPos2 = trailingPos2;
      sp.nullRun = nullRun;
      sp.term_Idx = term_Idx;
      sp.maxIdx_term = maxIdx_term;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cbSize_term = capCb_term;
      sp.cb_term_closeMinusTrueLow = term_closeMinusTrueLow;
      sp.cb_term_trueRange = term_trueRange;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* ultoscOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   UltoscStream ultoscOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      UltoscStream sp = new UltoscStream(this);
      RetCode retCode = ultoscOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ULTOSC openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ULTOSC openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("ULTOSC openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind ultoscOpen (composition seam). */
   UltoscStream ultoscOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      UltoscStream sp = new UltoscStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ultoscOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("ULTOSC open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("ULTOSC open: internal error", retCode);
      }
      throw new TaLibArgumentException("ULTOSC open: " + retCode, retCode);
   }
   /**
    * Open a live ULTOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ULTOSC} at that bar.
    * <p>The history must hold at least {@code ULTOSC_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public UltoscStream ultoscOpen( double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3 )
   {
      requireArgument("ULTOSC open", "inHigh", inHigh);
      requireHistory("ULTOSC open", inHigh.length);
      requireArgument("ULTOSC open", "inLow", inLow);
      requireArgument("ULTOSC open", "inClose", inClose);
      requireHistoryLength("ULTOSC open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ULTOSC open", "inClose", inClose.length, inHigh.length);
      return ultoscOpenInternal(inHigh, inLow, inClose, 0, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3);
   }
   /**
    * {@link Core#ultoscOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ULTOSC} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link UltoscStream#outRange()}.
    */
   public UltoscStream ultoscOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInTimePeriod1, int optInTimePeriod2, int optInTimePeriod3, double outReal[] )
   {
      requireArgument("ULTOSC openAndFill", "inHigh", inHigh);
      requireHistory("ULTOSC openAndFill", inHigh.length);
      requireArgument("ULTOSC openAndFill", "inLow", inLow);
      requireArgument("ULTOSC openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("ULTOSC openAndFill", inHigh.length, ULTOSC_Lookback(optInTimePeriod1, optInTimePeriod2, optInTimePeriod3));
      requireHistoryLength("ULTOSC openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("ULTOSC openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("ULTOSC openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow || (Object)outReal == (Object)inClose ) {
         throw new TaLibArgumentException("ULTOSC openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ultoscOpenAndFillInternal(inHigh, inLow, inClose, 0, optInTimePeriod1, optInTimePeriod2, optInTimePeriod3, outBegIdx, outNBElement, outReal);
   }
