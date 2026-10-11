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
 *  100726 MF,CC  Initial version (#490).
 */

   /**
    * Number of leading input bars {@link Core#ichimoku} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTenkanPeriod Period of the conversion line (default 9; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInKijunPeriod Period of the base line, and the forward shift of
    *        the two spans (default 26; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSenkouBPeriod Period of the second leading span (default 52;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ichimokuLookback( int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod )
   {
      if( optInTenkanPeriod == Integer.MIN_VALUE ) {
         optInTenkanPeriod = 9;
      } else if( optInTenkanPeriod < 2 || optInTenkanPeriod > 100000 ) {
         return -1;
      }
      if( optInKijunPeriod == Integer.MIN_VALUE ) {
         optInKijunPeriod = 26;
      } else if( optInKijunPeriod < 2 || optInKijunPeriod > 100000 ) {
         return -1;
      }
      if( optInSenkouBPeriod == Integer.MIN_VALUE ) {
         optInSenkouBPeriod = 52;
      } else if( optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000 ) {
         return -1;
      }
      int longest;
      /* Each line is a midpoint over its own window, so each needs its own
       * window filled; the first bar that has all of them is the longest one's.
       * The MAX is not simplified to the Senkou B period: nothing orders the
       * three, and a kijun longer than senkouB dominates.
       *
       * Span A is the mean of the other two lines, so it adds nothing: it is
       * ready on the bar they both are.
       */
      longest = optInTenkanPeriod;
      if( optInKijunPeriod > longest ) {
         longest = optInKijunPeriod;
      }
      if( optInSenkouBPeriod > longest ) {
         longest = optInSenkouBPeriod;
      }
      return longest - 1 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#ichimoku}.
    * <p>The values are never shifted: this describes the drawing only.
    *
    * @param optInTenkanPeriod Period of the conversion line (default 9; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInKijunPeriod Period of the base line, and the forward shift of
    *        the two spans (default 26; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSenkouBPeriod Period of the second leading span (default 52;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int ichimokuDisplayShift( int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod, int outputIdx )
   {
      if( ichimokuLookback( optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( optInTenkanPeriod == Integer.MIN_VALUE ) {
         optInTenkanPeriod = 9;
      } else if( optInTenkanPeriod < 2 || optInTenkanPeriod > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInKijunPeriod == Integer.MIN_VALUE ) {
         optInKijunPeriod = 26;
      } else if( optInKijunPeriod < 2 || optInKijunPeriod > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInSenkouBPeriod == Integer.MIN_VALUE ) {
         optInSenkouBPeriod = 52;
      } else if( optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 4 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx == 0 || outputIdx == 1 ) {
         return 0;
      }
      /* rL9: a shift of s means a chart draws the value computed at bar i at
       * bar i + s. The two spans are the leading ones -- Goichi Hosoda draws
       * them kijun bars into the future, which is what makes the cloud sit
       * ahead of price -- so their shift is POSITIVE, where dpo.c's and
       * fractal.c's are negative. The conversion and base lines are drawn on
       * the bar that computed them.
       *
       * The shift reads the optional parameters and the output index only,
       * never a setting (rL9).
       */
      if( outputIdx == 2 || outputIdx == 3 ) {
         return optInKijunPeriod ;
      }
      return 0 ;

   }
   RetCode ichimokuImpl( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         int optInTenkanPeriod,
                         int optInKijunPeriod,
                         int optInSenkouBPeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outTenkanSen[],
                         double outKijunSen[],
                         double outSenkouSpanA[],
                         double outSenkouSpanB[] )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int n = 0;
      int i = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempT;
      double[] tempK;
      double[] tempB;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTenkanPeriod == Integer.MIN_VALUE ) {
         optInTenkanPeriod = 9;
      } else if( optInTenkanPeriod < 2 || optInTenkanPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInKijunPeriod == Integer.MIN_VALUE ) {
         optInKijunPeriod = 26;
      } else if( optInKijunPeriod < 2 || optInKijunPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSenkouBPeriod == Integer.MIN_VALUE ) {
         optInSenkouBPeriod = 52;
      } else if( optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outTenkanSen == outKijunSen || outTenkanSen == outSenkouSpanA || outTenkanSen == outSenkouSpanB || outKijunSen == outSenkouSpanA || outKijunSen == outSenkouSpanB || outSenkouSpanA == outSenkouSpanB ) {
         return RetCode.BAD_PARAM ;
      }
      /* PROTOTYPE (#490 Q7): each line IS a midpoint over its own window, which is
       * exactly what midprice computes, so the three scans are three midprice calls
       * and Span A is the mean of two of them. MEASURED bit-identical to the fused
       * loop over four parameter triples on the suite's corpus, every line, before
       * this was written.
       *
       * The point is the stream tier: three windows of different periods cannot be
       * one extrema automaton (the census refuses with "expected exactly one
       * window-start variable"), but a composed body is a different tier.
       *
       * The three results go to temporaries and are copied at the end: every read of
       * high and low has to happen before the first write to a caller buffer, or an
       * output aliased onto an input is read after it has been overwritten.
       */
      lookbackTotal = ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      n = endIdx - startIdx + 1;
      tempT = new double[(int)(n * 1)];
      tempK = new double[(int)(n * 1)];
      tempB = new double[(int)(n * 1)];
      OutRange _xr0 = midprice(startIdx, endIdx, inHigh, inLow, optInTenkanPeriod, tempT);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = midprice(startIdx, endIdx, inHigh, inLow, optInKijunPeriod, tempK);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = midprice(startIdx, endIdx, inHigh, inLow, optInSenkouBPeriod, tempB);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      /* Span A is the mean of the two lines, which medprice is over any two series. */
      OutRange _xr3 = medprice(0, n - 1, tempT, tempK, outSenkouSpanA);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < n; i += 1 ) {
         outTenkanSen[i] = tempT[i];
         outKijunSen[i] = tempK[i];
         outSenkouSpanB[i] = tempB[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = n;
      return RetCode.SUCCESS ;
   }
   RetCode ichimokuImpl( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         int optInTenkanPeriod,
                         int optInKijunPeriod,
                         int optInSenkouBPeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outTenkanSen[],
                         double outKijunSen[],
                         double outSenkouSpanA[],
                         double outSenkouSpanB[] )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int n = 0;
      int i = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempT;
      double[] tempK;
      double[] tempB;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTenkanPeriod == Integer.MIN_VALUE ) {
         optInTenkanPeriod = 9;
      } else if( optInTenkanPeriod < 2 || optInTenkanPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInKijunPeriod == Integer.MIN_VALUE ) {
         optInKijunPeriod = 26;
      } else if( optInKijunPeriod < 2 || optInKijunPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSenkouBPeriod == Integer.MIN_VALUE ) {
         optInSenkouBPeriod = 52;
      } else if( optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outTenkanSen == outKijunSen || outTenkanSen == outSenkouSpanA || outTenkanSen == outSenkouSpanB || outKijunSen == outSenkouSpanA || outKijunSen == outSenkouSpanB || outSenkouSpanA == outSenkouSpanB ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      n = endIdx - startIdx + 1;
      tempT = new double[(int)(n * 1)];
      tempK = new double[(int)(n * 1)];
      tempB = new double[(int)(n * 1)];
      OutRange _xr0 = midprice(startIdx, endIdx, inHigh, inLow, optInTenkanPeriod, tempT);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = midprice(startIdx, endIdx, inHigh, inLow, optInKijunPeriod, tempK);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = midprice(startIdx, endIdx, inHigh, inLow, optInSenkouBPeriod, tempB);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr3 = medprice(0, n - 1, tempT, tempK, outSenkouSpanA);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < n; i += 1 ) {
         outTenkanSen[i] = tempT[i];
         outKijunSen[i] = tempK[i];
         outSenkouSpanB[i] = tempB[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = n;
      return RetCode.SUCCESS ;
   }
   /**
    * Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from
    * highs and lows alone. Goichi Hosoda's reading is that a market's balance
    * is visible without any smoothing — each line is the midpoint of a window,
    * the mean of its highest high and its lowest low, so it marks the level at
    * which that stretch of trading was evenly divided. The conversion line
    * turns fastest, the base line is the reference, and the two leading spans
    * are drawn ahead of price, where the band between them is read as support
    * or resistance before it is reached.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ichimoku">ta-lib.org/functions/ichimoku</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Each line is {@code TA_MIDPRICE} over its own period, and Span A is {@code TA_MEDPRICE} of the other two lines. Span A halves the two midpoints after each has been rounded, rather than averaging the four extremes. The two spellings are the same number in real arithmetic and a different double in the last bit often enough to matter: on the 252-bar regression corpus the rate is 0% at the published 9/26 periods, 5.7% at 26/9, and 12.4% at 2/2, and on a four-decimal series at 3/5 it is 48%. The rate is not a function of the longer period alone: 9/26 and 26/9 share a 26-bar window and read 0% and 5.7%.</li>
    * <li>The two spans are drawn {@code kijunPeriod} bars ahead of the bar that computed them. That is a display shift: it is reported through the display-shift call and changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it.</li>
    * <li>The lookback is the longest of the three periods less one. It is not the Senkou B period: nothing orders the three, so a base line longer than the second span dominates.</li>
    * <li>The Chikou span, the close drawn backward, carries no computation and is not an output here: it is the input series with a display shift.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ichimokuLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param optInTenkanPeriod Period of the conversion line (default 9; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInKijunPeriod Period of the base line, and the forward shift of
    *        the two spans (default 26; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSenkouBPeriod Period of the second leading span (default 52;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outTenkanSen Conversion line. Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outKijunSen Base line. Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outSenkouSpanA First leading span, drawn ahead by the base period.
    *        Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outSenkouSpanB Second leading span, drawn ahead by the base period.
    *        Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
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
    * @see Core#midprice
    * @see Core#medprice
    * @see Core#sar
    * @see Core#bbands
    */
   public OutRange ichimoku( int startIdx,
                             int endIdx,
                             double inHigh[],
                             double inLow[],
                             int optInTenkanPeriod,
                             int optInKijunPeriod,
                             int optInSenkouBPeriod,
                             double outTenkanSen[],
                             double outKijunSen[],
                             double outSenkouSpanA[],
                             double outSenkouSpanB[] )
   {
      requireIndexRange("ICHIMOKU", startIdx, endIdx);
      int guardStart = clampedStart("ICHIMOKU", startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ICHIMOKU", "inHigh", inHigh, guardInLen);
      requireLength("ICHIMOKU", "inLow", inLow, guardInLen);
      requireLength("ICHIMOKU", "outTenkanSen", outTenkanSen, guardOutLen);
      requireLength("ICHIMOKU", "outKijunSen", outKijunSen, guardOutLen);
      requireLength("ICHIMOKU", "outSenkouSpanA", outSenkouSpanA, guardOutLen);
      requireLength("ICHIMOKU", "outSenkouSpanB", outSenkouSpanB, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ichimokuImpl(startIdx, endIdx, inHigh, inLow, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ICHIMOKU", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from
    * highs and lows alone. Goichi Hosoda's reading is that a market's balance
    * is visible without any smoothing — each line is the midpoint of a window,
    * the mean of its highest high and its lowest low, so it marks the level at
    * which that stretch of trading was evenly divided. The conversion line
    * turns fastest, the base line is the reference, and the two leading spans
    * are drawn ahead of price, where the band between them is read as support
    * or resistance before it is reached.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ichimoku">ta-lib.org/functions/ichimoku</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Each line is {@code TA_MIDPRICE} over its own period, and Span A is {@code TA_MEDPRICE} of the other two lines. Span A halves the two midpoints after each has been rounded, rather than averaging the four extremes. The two spellings are the same number in real arithmetic and a different double in the last bit often enough to matter: on the 252-bar regression corpus the rate is 0% at the published 9/26 periods, 5.7% at 26/9, and 12.4% at 2/2, and on a four-decimal series at 3/5 it is 48%. The rate is not a function of the longer period alone: 9/26 and 26/9 share a 26-bar window and read 0% and 5.7%.</li>
    * <li>The two spans are drawn {@code kijunPeriod} bars ahead of the bar that computed them. That is a display shift: it is reported through the display-shift call and changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it.</li>
    * <li>The lookback is the longest of the three periods less one. It is not the Senkou B period: nothing orders the three, so a base line longer than the second span dominates.</li>
    * <li>The Chikou span, the close drawn backward, carries no computation and is not an output here: it is the input series with a display shift.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ichimokuLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param optInTenkanPeriod Period of the conversion line (default 9; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInKijunPeriod Period of the base line, and the forward shift of
    *        the two spans (default 26; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInSenkouBPeriod Period of the second leading span (default 52;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outTenkanSen Conversion line. Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outKijunSen Base line. Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outSenkouSpanA First leading span, drawn ahead by the base period.
    *        Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outSenkouSpanB Second leading span, drawn ahead by the base period.
    *        Must hold at least
    *        {@code endIdx - max(startIdx, ichimokuLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
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
    * @see Core#midprice
    * @see Core#medprice
    * @see Core#sar
    * @see Core#bbands
    */
   public OutRange ichimoku( int startIdx,
                             int endIdx,
                             float inHigh[],
                             float inLow[],
                             int optInTenkanPeriod,
                             int optInKijunPeriod,
                             int optInSenkouBPeriod,
                             double outTenkanSen[],
                             double outKijunSen[],
                             double outSenkouSpanA[],
                             double outSenkouSpanB[] )
   {
      requireIndexRange("ICHIMOKU", startIdx, endIdx);
      int guardStart = clampedStart("ICHIMOKU", startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ICHIMOKU", "inHigh", inHigh, guardInLen);
      requireLength("ICHIMOKU", "inLow", inLow, guardInLen);
      requireLength("ICHIMOKU", "outTenkanSen", outTenkanSen, guardOutLen);
      requireLength("ICHIMOKU", "outKijunSen", outKijunSen, guardOutLen);
      requireLength("ICHIMOKU", "outSenkouSpanA", outSenkouSpanA, guardOutLen);
      requireLength("ICHIMOKU", "outSenkouSpanB", outSenkouSpanB, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ichimokuImpl(startIdx, endIdx, inHigh, inLow, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ICHIMOKU", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ICHIMOKU stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ichimoku} over the same series.
    * Open with {@link Core#ichimokuOpen}; there is no close — the handle is
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
   public static final class IchimokuStream {
      private Core core;
      private int optInTenkanPeriod;
      private int optInKijunPeriod;
      private int optInSenkouBPeriod;
      private double cur_outTenkanSen;
      private double cur_outKijunSen;
      private double cur_outSenkouSpanA;
      private double cur_outSenkouSpanB;
      private MidpriceStream sub0;
      private MidpriceStream sub1;
      private MidpriceStream sub2;
      private MedpriceStream sub3;
      private int outRangeBegIdx;
      private int outRangeCount;

      private IchimokuStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ichimoku} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(IchimokuOut)} keeps answering the previous
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
            throw failure("ICHIMOKU advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private IchimokuStream( IchimokuStream other ) {
         this.core = other.core;
         this.optInTenkanPeriod = other.optInTenkanPeriod;
         this.optInKijunPeriod = other.optInKijunPeriod;
         this.optInSenkouBPeriod = other.optInSenkouBPeriod;
         this.cur_outTenkanSen = other.cur_outTenkanSen;
         this.cur_outKijunSen = other.cur_outKijunSen;
         this.cur_outSenkouSpanA = other.cur_outSenkouSpanA;
         this.cur_outSenkouSpanB = other.cur_outSenkouSpanB;
         this.sub0 = new MidpriceStream(other.sub0);
         this.sub1 = new MidpriceStream(other.sub1);
         this.sub2 = new MidpriceStream(other.sub2);
         this.sub3 = new MedpriceStream(other.sub3);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(IchimokuOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, IchimokuOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ICHIMOKU update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("ICHIMOKU update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("ICHIMOKU update", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         core.ichimokuStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         out.tenkanSen = this.cur_outTenkanSen;
         out.kijunSen = this.cur_outKijunSen;
         out.senkouSpanA = this.cur_outSenkouSpanA;
         out.senkouSpanB = this.cur_outSenkouSpanB;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, IchimokuOut out ) {
         requireArgument("ICHIMOKU peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("ICHIMOKU peek", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         IchimokuStream sp = this;
         double cur_tempT = 0.0;
         double cur_tempK = 0.0;
         double cur_tempB = 0.0;
         double cur_outSenkouSpanA = 0.0;
         double cur_outTenkanSen = 0.0;
         double cur_outKijunSen = 0.0;
         double cur_outSenkouSpanB = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempT = sp.sub0.peek(inHigh, inLow);
         cur_tempK = sp.sub1.peek(inHigh, inLow);
         cur_tempB = sp.sub2.peek(inHigh, inLow);
         cur_outSenkouSpanA = sp.sub3.peek(cur_tempT, cur_tempK);
         /* Combine map (batch tail, per bar). */
         cur_outTenkanSen = cur_tempT;
         cur_outKijunSen = cur_tempK;
         cur_outSenkouSpanB = cur_tempB;
         out.tenkanSen = cur_outTenkanSen;
         out.kijunSen = cur_outKijunSen;
         out.senkouSpanA = cur_outSenkouSpanA;
         out.senkouSpanB = cur_outSenkouSpanB;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( IchimokuOut out ) {
         requireArgument("ICHIMOKU value", "out", out);
         out.tenkanSen = this.cur_outTenkanSen;
         out.kijunSen = this.cur_outKijunSen;
         out.senkouSpanA = this.cur_outSenkouSpanA;
         out.senkouSpanB = this.cur_outSenkouSpanB;
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
      public IchimokuStream clone() {
         return new IchimokuStream(this);
      }
   }

   /**
    * The outputs of one ICHIMOKU bar, written by the stream into an object the
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
   public static final class IchimokuOut {
      /** Conversion line. */
      public double tenkanSen;
      /** Base line. */
      public double kijunSen;
      /** First leading span, drawn ahead by the base period. */
      public double senkouSpanA;
      /** Second leading span, drawn ahead by the base period. */
      public double senkouSpanB;
   }
   private void ichimokuStepImpl( IchimokuStream sp, double inHigh, double inLow )
   {
      double cur_tempT = 0.0;
      double cur_tempK = 0.0;
      double cur_tempB = 0.0;
      double cur_outSenkouSpanA = 0.0;
      double cur_outTenkanSen = 0.0;
      double cur_outKijunSen = 0.0;
      double cur_outSenkouSpanB = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempT = sp.sub0.update(inHigh, inLow);
      cur_tempK = sp.sub1.update(inHigh, inLow);
      cur_tempB = sp.sub2.update(inHigh, inLow);
      cur_outSenkouSpanA = sp.sub3.update(cur_tempT, cur_tempK);
      /* Combine map (batch tail, per bar). */
      cur_outTenkanSen = cur_tempT;
      cur_outKijunSen = cur_tempK;
      cur_outSenkouSpanB = cur_tempB;
      sp.cur_outTenkanSen = cur_outTenkanSen;
      sp.cur_outKijunSen = cur_outKijunSen;
      sp.cur_outSenkouSpanA = cur_outSenkouSpanA;
      sp.cur_outSenkouSpanB = cur_outSenkouSpanB;
   }
   private RetCode ichimokuOpenImpl( IchimokuStream sp, double inHigh[], double inLow[], int startIdx, int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod, MInteger outBegIdx, MInteger outNBElement, double outTenkanSen[], double outKijunSen[], double outSenkouSpanA[], double outSenkouSpanB[], int outStride )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int n = 0;
      int i = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempT;
      double[] tempK;
      double[] tempB;
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
      if( optInTenkanPeriod == Integer.MIN_VALUE ) {
         optInTenkanPeriod = 9;
      } else if( optInTenkanPeriod < 2 || optInTenkanPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInKijunPeriod == Integer.MIN_VALUE ) {
         optInKijunPeriod = 26;
      } else if( optInKijunPeriod < 2 || optInKijunPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSenkouBPeriod == Integer.MIN_VALUE ) {
         optInSenkouBPeriod = 52;
      } else if( optInSenkouBPeriod < 2 || optInSenkouBPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outTenkanSen = outStride == 1 ? outTenkanSen : new double[historyLen];
      double[] sc_outKijunSen = outStride == 1 ? outKijunSen : new double[historyLen];
      double[] sc_outSenkouSpanA = outStride == 1 ? outSenkouSpanA : new double[historyLen];
      double[] sc_outSenkouSpanB = outStride == 1 ? outSenkouSpanB : new double[historyLen];
      /* PROTOTYPE (#490 Q7): each line IS a midpoint over its own window, which is
       * exactly what midprice computes, so the three scans are three midprice calls
       * and Span A is the mean of two of them. MEASURED bit-identical to the fused
       * loop over four parameter triples on the suite's corpus, every line, before
       * this was written.
       *
       * The point is the stream tier: three windows of different periods cannot be
       * one extrema automaton (the census refuses with "expected exactly one
       * window-start variable"), but a composed body is a different tier.
       *
       * The three results go to temporaries and are copied at the end: every read of
       * high and low has to happen before the first write to a caller buffer, or an
       * output aliased onto an input is read after it has been overwritten.
       */
      lookbackTotal = ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      n = endIdx - startIdx + 1;
      tempT = new double[(int)(n * 1)];
      tempK = new double[(int)(n * 1)];
      tempB = new double[(int)(n * 1)];
      /* Sub-stream 0: midprice over `inHigh, inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MidpriceStream sub0 = midpriceOpenAndFillInternal(inHigh, inLow, startIdx, optInTenkanPeriod, tempBegIdx, tempNbElement, tempT);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 1: midprice over `inHigh, inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MidpriceStream sub1 = midpriceOpenAndFillInternal(inHigh, inLow, startIdx, optInKijunPeriod, tempBegIdx, tempNbElement, tempK);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 2: midprice over `inHigh, inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MidpriceStream sub2 = midpriceOpenAndFillInternal(inHigh, inLow, startIdx, optInSenkouBPeriod, tempBegIdx, tempNbElement, tempB);
      retCode = RetCode.SUCCESS;
      /* Span A is the mean of the two lines, which medprice is over any two series. */
      /* Sub-stream 3: medprice over `tempT, tempK`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MedpriceStream sub3 = medpriceOpenAndFillInternal(java.util.Arrays.copyOfRange(tempT, 0, (n - 1) + 1), java.util.Arrays.copyOfRange(tempK, 0, (n - 1) + 1), 0, tempBegIdx, tempNbElement, sc_outSenkouSpanA);
      retCode = RetCode.SUCCESS;
      for( i = 0; i < n; i += 1 ) {
         sc_outTenkanSen[i] = tempT[i];
         sc_outKijunSen[i] = tempK[i];
         sc_outSenkouSpanB[i] = tempB[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = n;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      sp.optInTenkanPeriod = optInTenkanPeriod;
      sp.optInKijunPeriod = optInKijunPeriod;
      sp.optInSenkouBPeriod = optInSenkouBPeriod;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.sub3 = sub3;
      sp.cur_outTenkanSen = sc_outTenkanSen[outNBElement.value - 1];
      sp.cur_outKijunSen = sc_outKijunSen[outNBElement.value - 1];
      sp.cur_outSenkouSpanA = sc_outSenkouSpanA[outNBElement.value - 1];
      sp.cur_outSenkouSpanB = sc_outSenkouSpanB[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* ichimokuOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   IchimokuStream ichimokuOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod, MInteger outBegIdx, MInteger outNBElement, double outTenkanSen[], double outKijunSen[], double outSenkouSpanA[], double outSenkouSpanB[] )
   {
      IchimokuStream sp = new IchimokuStream(this);
      RetCode retCode = ichimokuOpenImpl(sp, inHigh, inLow, startIdx, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ICHIMOKU openAndFill", inHigh.length, startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod));
      }
      throw streamFailure("ICHIMOKU openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind ichimokuOpen (composition seam). */
   IchimokuStream ichimokuOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod )
   {
      IchimokuStream sp = new IchimokuStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outTenkanSen = new double[1];
      double[] sink_outKijunSen = new double[1];
      double[] sink_outSenkouSpanA = new double[1];
      double[] sink_outSenkouSpanB = new double[1];
      RetCode retCode = ichimokuOpenImpl(sp, inHigh, inLow, startIdx, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, sink_outTenkanSen, sink_outKijunSen, sink_outSenkouSpanA, sink_outSenkouSpanB, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ICHIMOKU open", inHigh.length, startIdx, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod));
      }
      throw streamFailure("ICHIMOKU open", retCode);
   }
   /**
    * Open a live ICHIMOKU stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ichimoku} at that bar.
    * <p>The history must hold at least {@code ichimokuLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public IchimokuStream ichimokuOpen( double inHigh[], double inLow[], int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod )
   {
      requireArgument("ICHIMOKU open", "inHigh", inHigh);
      requireHistory("ICHIMOKU open", inHigh.length);
      requireArgument("ICHIMOKU open", "inLow", inLow);
      requireHistoryLength("ICHIMOKU open", "inLow", inLow.length, inHigh.length);
      return ichimokuOpenInternal(inHigh, inLow, 0, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod);
   }
   /**
    * {@link Core#ichimokuOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ichimoku} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link IchimokuStream#outRange()}.
    */
   public IchimokuStream ichimokuOpenAndFill( double inHigh[], double inLow[], int optInTenkanPeriod, int optInKijunPeriod, int optInSenkouBPeriod, double outTenkanSen[], double outKijunSen[], double outSenkouSpanA[], double outSenkouSpanB[] )
   {
      requireArgument("ICHIMOKU openAndFill", "inHigh", inHigh);
      requireHistory("ICHIMOKU openAndFill", inHigh.length);
      requireArgument("ICHIMOKU openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("ICHIMOKU openAndFill", inHigh.length, ichimokuLookback(optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod));
      requireHistoryLength("ICHIMOKU openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("ICHIMOKU openAndFill", "outTenkanSen", outTenkanSen, guardOutLen);
      requireLength("ICHIMOKU openAndFill", "outKijunSen", outKijunSen, guardOutLen);
      requireLength("ICHIMOKU openAndFill", "outSenkouSpanA", outSenkouSpanA, guardOutLen);
      requireLength("ICHIMOKU openAndFill", "outSenkouSpanB", outSenkouSpanB, guardOutLen);
      if( (Object)outTenkanSen == (Object)inHigh || (Object)outTenkanSen == (Object)inLow || (Object)outKijunSen == (Object)inHigh || (Object)outKijunSen == (Object)inLow || (Object)outSenkouSpanA == (Object)inHigh || (Object)outSenkouSpanA == (Object)inLow || (Object)outSenkouSpanB == (Object)inHigh || (Object)outSenkouSpanB == (Object)inLow || (Object)outTenkanSen == (Object)outKijunSen || (Object)outTenkanSen == (Object)outSenkouSpanA || (Object)outTenkanSen == (Object)outSenkouSpanB || (Object)outKijunSen == (Object)outSenkouSpanA || (Object)outKijunSen == (Object)outSenkouSpanB || (Object)outSenkouSpanA == (Object)outSenkouSpanB ) {
         throw streamFailure("ICHIMOKU openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ichimokuOpenAndFillInternal(inHigh, inLow, 0, optInTenkanPeriod, optInKijunPeriod, optInSenkouBPeriod, outBegIdx, outNBElement, outTenkanSen, outKijunSen, outSenkouSpanA, outSenkouSpanB);
   }
