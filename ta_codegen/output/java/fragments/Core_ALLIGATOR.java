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
 *  110726 MF,CC  First version (issue #584).
 */

   /**
    * Number of leading input bars {@link Core#alligator} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInJawPeriod Number of bars in the jaw's smoothed moving average
    *        (default 13; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInJawShift Bars ahead of the computing bar at which a chart
    *        draws the jaw (default 8; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInTeethPeriod Number of bars in the teeth's smoothed moving
    *        average (default 8; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInTeethShift Bars ahead of the computing bar at which a chart
    *        draws the teeth (default 5; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInLipsPeriod Number of bars in the lips' smoothed moving average
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInLipsShift Bars ahead of the computing bar at which a chart
    *        draws the lips (default 3; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int alligatorLookback( int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift )
   {
      if( optInJawPeriod == Integer.MIN_VALUE ) {
         optInJawPeriod = 13;
      } else if( optInJawPeriod < 1 || optInJawPeriod > 100000 ) {
         return -1;
      }
      if( optInJawShift == Integer.MIN_VALUE ) {
         optInJawShift = 8;
      } else if( optInJawShift < 0 || optInJawShift > 100000 ) {
         return -1;
      }
      if( optInTeethPeriod == Integer.MIN_VALUE ) {
         optInTeethPeriod = 8;
      } else if( optInTeethPeriod < 1 || optInTeethPeriod > 100000 ) {
         return -1;
      }
      if( optInTeethShift == Integer.MIN_VALUE ) {
         optInTeethShift = 5;
      } else if( optInTeethShift < 0 || optInTeethShift > 100000 ) {
         return -1;
      }
      if( optInLipsPeriod == Integer.MIN_VALUE ) {
         optInLipsPeriod = 5;
      } else if( optInLipsPeriod < 1 || optInLipsPeriod > 100000 ) {
         return -1;
      }
      if( optInLipsShift == Integer.MIN_VALUE ) {
         optInLipsShift = 3;
      } else if( optInLipsShift < 0 || optInLipsShift > 100000 ) {
         return -1;
      }
      int jawLookback;
      int teethLookback;
      int lipsLookback;
      int lookback;
      /* Each term is exactly its leg's rma_lookback, never restated, so all three
       * legs inherit TA_FUNC_UNST_RMA. Nothing orders the periods: any leg can be
       * the longest.
       */
      jawLookback = rmaLookback(optInJawPeriod);
      teethLookback = rmaLookback(optInTeethPeriod);
      lipsLookback = rmaLookback(optInLipsPeriod);
      lookback = (jawLookback > teethLookback) ? jawLookback : teethLookback;
      return (lookback > lipsLookback) ? lookback : lipsLookback ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#alligator}.
    * <p>The values are never shifted: this describes the drawing only.
    *
    * @param optInJawPeriod Number of bars in the jaw's smoothed moving average
    *        (default 13; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInJawShift Bars ahead of the computing bar at which a chart
    *        draws the jaw (default 8; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInTeethPeriod Number of bars in the teeth's smoothed moving
    *        average (default 8; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInTeethShift Bars ahead of the computing bar at which a chart
    *        draws the teeth (default 5; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInLipsPeriod Number of bars in the lips' smoothed moving average
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInLipsShift Bars ahead of the computing bar at which a chart
    *        draws the lips (default 3; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int alligatorDisplayShift( int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, int outputIdx )
   {
      if( alligatorLookback( optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( optInJawPeriod == Integer.MIN_VALUE ) {
         optInJawPeriod = 13;
      } else if( optInJawPeriod < 1 || optInJawPeriod > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInJawShift == Integer.MIN_VALUE ) {
         optInJawShift = 8;
      } else if( optInJawShift < 0 || optInJawShift > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInTeethPeriod == Integer.MIN_VALUE ) {
         optInTeethPeriod = 8;
      } else if( optInTeethPeriod < 1 || optInTeethPeriod > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInTeethShift == Integer.MIN_VALUE ) {
         optInTeethShift = 5;
      } else if( optInTeethShift < 0 || optInTeethShift > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInLipsPeriod == Integer.MIN_VALUE ) {
         optInLipsPeriod = 5;
      } else if( optInLipsPeriod < 1 || optInLipsPeriod > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( optInLipsShift == Integer.MIN_VALUE ) {
         optInLipsShift = 3;
      } else if( optInLipsShift < 0 || optInLipsShift > 100000 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 3 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx == 0 ) {
         return optInJawShift ;
      }
      if( outputIdx == 1 ) {
         return optInTeethShift ;
      }
      return optInLipsShift ;

   }
   RetCode alligatorImpl( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInJawPeriod,
                          int optInJawShift,
                          int optInTeethPeriod,
                          int optInTeethShift,
                          int optInLipsPeriod,
                          int optInLipsShift,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outJaw[],
                          double outTeeth[],
                          double outLips[] )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int mpStartIdx = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempMP;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInJawPeriod == Integer.MIN_VALUE ) {
         optInJawPeriod = 13;
      } else if( optInJawPeriod < 1 || optInJawPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInJawShift == Integer.MIN_VALUE ) {
         optInJawShift = 8;
      } else if( optInJawShift < 0 || optInJawShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTeethPeriod == Integer.MIN_VALUE ) {
         optInTeethPeriod = 8;
      } else if( optInTeethPeriod < 1 || optInTeethPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTeethShift == Integer.MIN_VALUE ) {
         optInTeethShift = 5;
      } else if( optInTeethShift < 0 || optInTeethShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLipsPeriod == Integer.MIN_VALUE ) {
         optInLipsPeriod = 5;
      } else if( optInLipsPeriod < 1 || optInLipsPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLipsShift == Integer.MIN_VALUE ) {
         optInLipsShift = 3;
      } else if( optInLipsShift < 0 || optInLipsShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outJaw == outTeeth || outJaw == outLips || outTeeth == outLips ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
      /* Return before touching anything, so that an input which stops short of
       * endIdx is never read past its end.
       */
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      mpStartIdx = startIdx - lookbackTotal;
      tempMP = new double[(int)((endIdx - mpStartIdx + 1) * 1)];
      /* Every price is consumed here, before any output is written: an output
       * may be aliased onto inHigh or inLow.
       */
      OutRange _xr0 = medprice(mpStartIdx, endIdx, inHigh, inLow, tempMP);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      /* Each leg is entered at startIdx (local index lookbackTotal) and seeds at
       * its OWN lookback before it, so each is exactly TA_RMA over the bars it
       * reads and no leg moves when another leg's period changes. A caller who
       * wants the short legs converged sets TA_FUNC_UNST_RMA.
       */
      OutRange _xr1 = rma(lookbackTotal, endIdx - mpStartIdx, tempMP, optInJawPeriod, outJaw);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr2 = rma(lookbackTotal, endIdx - mpStartIdx, tempMP, optInTeethPeriod, outTeeth);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr3 = rma(lookbackTotal, endIdx - mpStartIdx, tempMP, optInLipsPeriod, outLips);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = tempNbElement.value;
      return RetCode.SUCCESS ;
   }
   RetCode alligatorImpl( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInJawPeriod,
                          int optInJawShift,
                          int optInTeethPeriod,
                          int optInTeethShift,
                          int optInLipsPeriod,
                          int optInLipsShift,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outJaw[],
                          double outTeeth[],
                          double outLips[] )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int mpStartIdx = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempMP;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInJawPeriod == Integer.MIN_VALUE ) {
         optInJawPeriod = 13;
      } else if( optInJawPeriod < 1 || optInJawPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInJawShift == Integer.MIN_VALUE ) {
         optInJawShift = 8;
      } else if( optInJawShift < 0 || optInJawShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTeethPeriod == Integer.MIN_VALUE ) {
         optInTeethPeriod = 8;
      } else if( optInTeethPeriod < 1 || optInTeethPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTeethShift == Integer.MIN_VALUE ) {
         optInTeethShift = 5;
      } else if( optInTeethShift < 0 || optInTeethShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLipsPeriod == Integer.MIN_VALUE ) {
         optInLipsPeriod = 5;
      } else if( optInLipsPeriod < 1 || optInLipsPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLipsShift == Integer.MIN_VALUE ) {
         optInLipsShift = 3;
      } else if( optInLipsShift < 0 || optInLipsShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outJaw == outTeeth || outJaw == outLips || outTeeth == outLips ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      mpStartIdx = startIdx - lookbackTotal;
      tempMP = new double[(int)((endIdx - mpStartIdx + 1) * 1)];
      OutRange _xr0 = medprice(mpStartIdx, endIdx, inHigh, inLow, tempMP);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = rma(lookbackTotal, endIdx - mpStartIdx, tempMP, optInJawPeriod, outJaw);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr2 = rma(lookbackTotal, endIdx - mpStartIdx, tempMP, optInTeethPeriod, outTeeth);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr3 = rma(lookbackTotal, endIdx - mpStartIdx, tempMP, optInLipsPeriod, outLips);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = tempNbElement.value;
      return RetCode.SUCCESS ;
   }
   /**
    * Bill Williams' Alligator (<i>Trading Chaos</i>, 1995): three smoothed
    * moving averages of the median price. The Jaw, Teeth and Lips are each
    * drawn a few bars ahead of the bar that computed it. When the three lines
    * are intertwined the alligator sleeps and the market has no trend; when
    * they fan out in order, Lips above Teeth above Jaw or the reverse, it is
    * feeding and a trend is under way. Price crossing back into the lines is
    * read as the trend tiring. The Alligator is a leg of Williams' Profitunity
    * system, alongside the Awesome Oscillator (<a
    * href="https://ta-lib.org/functions/ao">{@code AO}</a>) and the
    * Accelerator/Decelerator (<a
    * href="https://ta-lib.org/functions/ac">{@code AC}</a>).
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/alligator">ta-lib.org/functions/alligator</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Each line is {@code TA_RMA} of {@code TA_MEDPRICE} over its own period. Williams draws the lines ahead of price; that is a display shift: {@code TA_ALLIGATOR_DisplayShift} reports each line's shift, and it changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it, so the last {@code shift} values of a line are the part a chart draws past the last bar.</li>
    * <li>All three lines start at the bar of the longest period, and each shorter line seeds over the bars its own lookback reaches back from there, not at the first bar of the series. A chart that seeds every line at the first bar draws different short lines until the seed has decayed; setting the {@code RMA} unstable period closes the gap.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#alligatorLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInJawPeriod Number of bars in the jaw's smoothed moving average
    *        (default 13; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInJawShift Bars ahead of the computing bar at which a chart
    *        draws the jaw (default 8; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInTeethPeriod Number of bars in the teeth's smoothed moving
    *        average (default 8; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInTeethShift Bars ahead of the computing bar at which a chart
    *        draws the teeth (default 5; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInLipsPeriod Number of bars in the lips' smoothed moving average
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInLipsShift Bars ahead of the computing bar at which a chart
    *        draws the lips (default 3; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outJaw Jaw: the slowest line, drawn ahead by the jaw shift. Must
    *        hold at least {@code endIdx - max(startIdx, alligatorLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @param outTeeth Teeth: the middle line, drawn ahead by the teeth shift.
    *        Must hold at least
    *        {@code endIdx - max(startIdx, alligatorLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outLips Lips: the fastest line, drawn ahead by the lips shift. Must
    *        hold at least {@code endIdx - max(startIdx, alligatorLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#rma
    * @see Core#medprice
    * @see Core#ao
    * @see Core#ac
    * @see Core#ichimoku
    */
   public OutRange alligator( int startIdx,
                              int endIdx,
                              double inHigh[],
                              double inLow[],
                              int optInJawPeriod,
                              int optInJawShift,
                              int optInTeethPeriod,
                              int optInTeethShift,
                              int optInLipsPeriod,
                              int optInLipsShift,
                              double outJaw[],
                              double outTeeth[],
                              double outLips[] )
   {
      requireIndexRange("ALLIGATOR", startIdx, endIdx);
      int guardStart = clampedStart("ALLIGATOR", startIdx, alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ALLIGATOR", "inHigh", inHigh, guardInLen);
      requireLength("ALLIGATOR", "inLow", inLow, guardInLen);
      requireLength("ALLIGATOR", "outJaw", outJaw, guardOutLen);
      requireLength("ALLIGATOR", "outTeeth", outTeeth, guardOutLen);
      requireLength("ALLIGATOR", "outLips", outLips, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = alligatorImpl(startIdx, endIdx, inHigh, inLow, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outJaw, outTeeth, outLips);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ALLIGATOR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bill Williams' Alligator (<i>Trading Chaos</i>, 1995): three smoothed
    * moving averages of the median price. The Jaw, Teeth and Lips are each
    * drawn a few bars ahead of the bar that computed it. When the three lines
    * are intertwined the alligator sleeps and the market has no trend; when
    * they fan out in order, Lips above Teeth above Jaw or the reverse, it is
    * feeding and a trend is under way. Price crossing back into the lines is
    * read as the trend tiring. The Alligator is a leg of Williams' Profitunity
    * system, alongside the Awesome Oscillator (<a
    * href="https://ta-lib.org/functions/ao">{@code AO}</a>) and the
    * Accelerator/Decelerator (<a
    * href="https://ta-lib.org/functions/ac">{@code AC}</a>).
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/alligator">ta-lib.org/functions/alligator</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Each line is {@code TA_RMA} of {@code TA_MEDPRICE} over its own period. Williams draws the lines ahead of price; that is a display shift: {@code TA_ALLIGATOR_DisplayShift} reports each line's shift, and it changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it, so the last {@code shift} values of a line are the part a chart draws past the last bar.</li>
    * <li>All three lines start at the bar of the longest period, and each shorter line seeds over the bars its own lookback reaches back from there, not at the first bar of the series. A chart that seeds every line at the first bar draws different short lines until the seed has decayed; setting the {@code RMA} unstable period closes the gap.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#alligatorLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInJawPeriod Number of bars in the jaw's smoothed moving average
    *        (default 13; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInJawShift Bars ahead of the computing bar at which a chart
    *        draws the jaw (default 8; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInTeethPeriod Number of bars in the teeth's smoothed moving
    *        average (default 8; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInTeethShift Bars ahead of the computing bar at which a chart
    *        draws the teeth (default 5; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param optInLipsPeriod Number of bars in the lips' smoothed moving average
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInLipsShift Bars ahead of the computing bar at which a chart
    *        draws the lips (default 3; range 0..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outJaw Jaw: the slowest line, drawn ahead by the jaw shift. Must
    *        hold at least {@code endIdx - max(startIdx, alligatorLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @param outTeeth Teeth: the middle line, drawn ahead by the teeth shift.
    *        Must hold at least
    *        {@code endIdx - max(startIdx, alligatorLookback(...)) + 1} values, and
    *        never be empty: an empty array is an absent output.
    * @param outLips Lips: the fastest line, drawn ahead by the lips shift. Must
    *        hold at least {@code endIdx - max(startIdx, alligatorLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#rma
    * @see Core#medprice
    * @see Core#ao
    * @see Core#ac
    * @see Core#ichimoku
    */
   public OutRange alligator( int startIdx,
                              int endIdx,
                              float inHigh[],
                              float inLow[],
                              int optInJawPeriod,
                              int optInJawShift,
                              int optInTeethPeriod,
                              int optInTeethShift,
                              int optInLipsPeriod,
                              int optInLipsShift,
                              double outJaw[],
                              double outTeeth[],
                              double outLips[] )
   {
      requireIndexRange("ALLIGATOR", startIdx, endIdx);
      int guardStart = clampedStart("ALLIGATOR", startIdx, alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ALLIGATOR", "inHigh", inHigh, guardInLen);
      requireLength("ALLIGATOR", "inLow", inLow, guardInLen);
      requireLength("ALLIGATOR", "outJaw", outJaw, guardOutLen);
      requireLength("ALLIGATOR", "outTeeth", outTeeth, guardOutLen);
      requireLength("ALLIGATOR", "outLips", outLips, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = alligatorImpl(startIdx, endIdx, inHigh, inLow, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outJaw, outTeeth, outLips);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ALLIGATOR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ALLIGATOR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#alligator} over the same series.
    * Open with {@link Core#alligatorOpen}; there is no close — the handle is
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
   public static final class AlligatorStream {
      private Core core;
      private int optInJawPeriod;
      private int optInJawShift;
      private int optInTeethPeriod;
      private int optInTeethShift;
      private int optInLipsPeriod;
      private int optInLipsShift;
      private double cur_outJaw;
      private double cur_outTeeth;
      private double cur_outLips;
      private MedpriceStream sub0;
      private RmaStream sub1;
      private RmaStream sub2;
      private RmaStream sub3;
      private int outRangeBegIdx;
      private int outRangeCount;

      private AlligatorStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#alligator} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(AlligatorOut)} keeps answering the previous
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
            throw failure("ALLIGATOR advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private AlligatorStream( AlligatorStream other ) {
         this.core = other.core;
         this.optInJawPeriod = other.optInJawPeriod;
         this.optInJawShift = other.optInJawShift;
         this.optInTeethPeriod = other.optInTeethPeriod;
         this.optInTeethShift = other.optInTeethShift;
         this.optInLipsPeriod = other.optInLipsPeriod;
         this.optInLipsShift = other.optInLipsShift;
         this.cur_outJaw = other.cur_outJaw;
         this.cur_outTeeth = other.cur_outTeeth;
         this.cur_outLips = other.cur_outLips;
         this.sub0 = new MedpriceStream(other.sub0);
         this.sub1 = new RmaStream(other.sub1);
         this.sub2 = new RmaStream(other.sub2);
         this.sub3 = new RmaStream(other.sub3);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(AlligatorOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, AlligatorOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ALLIGATOR update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("ALLIGATOR update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("ALLIGATOR update", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         core.alligatorStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         out.jaw = this.cur_outJaw;
         out.teeth = this.cur_outTeeth;
         out.lips = this.cur_outLips;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, AlligatorOut out ) {
         requireArgument("ALLIGATOR peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("ALLIGATOR peek", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         AlligatorStream sp = this;
         double cur_tempMP = 0.0;
         double cur_outJaw = 0.0;
         double cur_outTeeth = 0.0;
         double cur_outLips = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempMP = sp.sub0.peek(inHigh, inLow);
         cur_outJaw = sp.sub1.peek(cur_tempMP);
         cur_outTeeth = sp.sub2.peek(cur_tempMP);
         cur_outLips = sp.sub3.peek(cur_tempMP);
         out.jaw = cur_outJaw;
         out.teeth = cur_outTeeth;
         out.lips = cur_outLips;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( AlligatorOut out ) {
         requireArgument("ALLIGATOR value", "out", out);
         out.jaw = this.cur_outJaw;
         out.teeth = this.cur_outTeeth;
         out.lips = this.cur_outLips;
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
      public AlligatorStream clone() {
         return new AlligatorStream(this);
      }
   }

   /**
    * The outputs of one ALLIGATOR bar, written by the stream into an object the
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
   public static final class AlligatorOut {
      /** Jaw: the slowest line, drawn ahead by the jaw shift. */
      public double jaw;
      /** Teeth: the middle line, drawn ahead by the teeth shift. */
      public double teeth;
      /** Lips: the fastest line, drawn ahead by the lips shift. */
      public double lips;
   }
   private void alligatorStepImpl( AlligatorStream sp, double inHigh, double inLow )
   {
      double cur_tempMP = 0.0;
      double cur_outJaw = 0.0;
      double cur_outTeeth = 0.0;
      double cur_outLips = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempMP = sp.sub0.update(inHigh, inLow);
      cur_outJaw = sp.sub1.update(cur_tempMP);
      cur_outTeeth = sp.sub2.update(cur_tempMP);
      cur_outLips = sp.sub3.update(cur_tempMP);
      sp.cur_outJaw = cur_outJaw;
      sp.cur_outTeeth = cur_outTeeth;
      sp.cur_outLips = cur_outLips;
   }
   private RetCode alligatorOpenImpl( AlligatorStream sp, double inHigh[], double inLow[], int startIdx, int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, MInteger outBegIdx, MInteger outNBElement, double outJaw[], double outTeeth[], double outLips[], int outStride )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int mpStartIdx = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempMP;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInJawPeriod == Integer.MIN_VALUE ) {
         optInJawPeriod = 13;
      } else if( optInJawPeriod < 1 || optInJawPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInJawShift == Integer.MIN_VALUE ) {
         optInJawShift = 8;
      } else if( optInJawShift < 0 || optInJawShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTeethPeriod == Integer.MIN_VALUE ) {
         optInTeethPeriod = 8;
      } else if( optInTeethPeriod < 1 || optInTeethPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInTeethShift == Integer.MIN_VALUE ) {
         optInTeethShift = 5;
      } else if( optInTeethShift < 0 || optInTeethShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLipsPeriod == Integer.MIN_VALUE ) {
         optInLipsPeriod = 5;
      } else if( optInLipsPeriod < 1 || optInLipsPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInLipsShift == Integer.MIN_VALUE ) {
         optInLipsShift = 3;
      } else if( optInLipsShift < 0 || optInLipsShift > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx >= historyLen ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outJaw = outStride == 1 ? outJaw : new double[historyLen];
      double[] sc_outTeeth = outStride == 1 ? outTeeth : new double[historyLen];
      double[] sc_outLips = outStride == 1 ? outLips : new double[historyLen];
      int lagNb0 = 0;
      int lagNb1 = 0;
      lookbackTotal = alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
      /* Return before touching anything, so that an input which stops short of
       * endIdx is never read past its end.
       */
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      mpStartIdx = startIdx - lookbackTotal;
      tempMP = new double[(int)((endIdx - mpStartIdx + 1) * 1)];
      /* Every price is consumed here, before any output is written: an output
       * may be aliased onto inHigh or inLow.
       */
      /* Sub-stream 0: medprice over `inHigh, inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MedpriceStream sub0 = medpriceOpenAndFillInternal(inHigh, inLow, mpStartIdx, tempBegIdx, tempNbElement, tempMP);
      retCode = RetCode.SUCCESS;
      /* Each leg is entered at startIdx (local index lookbackTotal) and seeds at
       * its OWN lookback before it, so each is exactly TA_RMA over the bars it
       * reads and no leg moves when another leg's period changes. A caller who
       * wants the short legs converged sets TA_FUNC_UNST_RMA.
       */
      /* Sub-stream 1: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub1 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (endIdx - mpStartIdx) + 1), lookbackTotal, optInJawPeriod, tempBegIdx, tempNbElement, sc_outJaw);
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      lagNb0 = tempNbElement.value;
      /* Sub-stream 2: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub2 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (endIdx - mpStartIdx) + 1), lookbackTotal, optInTeethPeriod, tempBegIdx, tempNbElement, sc_outTeeth);
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      lagNb1 = tempNbElement.value;
      /* Sub-stream 3: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub3 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (endIdx - mpStartIdx) + 1), lookbackTotal, optInLipsPeriod, tempBegIdx, tempNbElement, sc_outLips);
      retCode = RetCode.SUCCESS;
      if( tempNbElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = tempNbElement.value;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( lagNb0 - outNBElement.value != 0 ) {
         return RetCode.INTERNAL_ERROR;
      }
      if( lagNb1 - outNBElement.value != 0 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInJawPeriod = optInJawPeriod;
      sp.optInJawShift = optInJawShift;
      sp.optInTeethPeriod = optInTeethPeriod;
      sp.optInTeethShift = optInTeethShift;
      sp.optInLipsPeriod = optInLipsPeriod;
      sp.optInLipsShift = optInLipsShift;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.sub3 = sub3;
      sp.cur_outJaw = sc_outJaw[outNBElement.value - 1];
      sp.cur_outTeeth = sc_outTeeth[outNBElement.value - 1];
      sp.cur_outLips = sc_outLips[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* alligatorOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AlligatorStream alligatorOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, MInteger outBegIdx, MInteger outNBElement, double outJaw[], double outTeeth[], double outLips[] )
   {
      AlligatorStream sp = new AlligatorStream(this);
      RetCode retCode = alligatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outJaw, outTeeth, outLips, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ALLIGATOR openAndFill", inHigh.length, startIdx, alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      }
      throw streamFailure("ALLIGATOR openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind alligatorOpen (composition seam). */
   AlligatorStream alligatorOpenInternal( double inHigh[], double inLow[], int startIdx, int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift )
   {
      AlligatorStream sp = new AlligatorStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outJaw = new double[1];
      double[] sink_outTeeth = new double[1];
      double[] sink_outLips = new double[1];
      RetCode retCode = alligatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, sink_outJaw, sink_outTeeth, sink_outLips, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ALLIGATOR open", inHigh.length, startIdx, alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      }
      throw streamFailure("ALLIGATOR open", retCode);
   }
   /**
    * Open a live ALLIGATOR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#alligator} at that bar.
    * <p>The history must hold at least {@code alligatorLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public AlligatorStream alligatorOpen( double inHigh[], double inLow[], int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift )
   {
      requireArgument("ALLIGATOR open", "inHigh", inHigh);
      requireHistory("ALLIGATOR open", inHigh.length);
      requireArgument("ALLIGATOR open", "inLow", inLow);
      requireHistoryLength("ALLIGATOR open", "inLow", inLow.length, inHigh.length);
      return alligatorOpenInternal(inHigh, inLow, 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
   }
   /**
    * {@link Core#alligatorOpen} that also fills the output array(s) bit-identically
    * to {@link Core#alligator} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AlligatorStream#outRange()}.
    */
   public AlligatorStream alligatorOpenAndFill( double inHigh[], double inLow[], int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, double outJaw[], double outTeeth[], double outLips[] )
   {
      requireArgument("ALLIGATOR openAndFill", "inHigh", inHigh);
      requireHistory("ALLIGATOR openAndFill", inHigh.length);
      requireArgument("ALLIGATOR openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("ALLIGATOR openAndFill", inHigh.length, alligatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      requireHistoryLength("ALLIGATOR openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("ALLIGATOR openAndFill", "outJaw", outJaw, guardOutLen);
      requireLength("ALLIGATOR openAndFill", "outTeeth", outTeeth, guardOutLen);
      requireLength("ALLIGATOR openAndFill", "outLips", outLips, guardOutLen);
      if( (Object)outJaw == (Object)inHigh || (Object)outJaw == (Object)inLow || (Object)outTeeth == (Object)inHigh || (Object)outTeeth == (Object)inLow || (Object)outLips == (Object)inHigh || (Object)outLips == (Object)inLow || (Object)outJaw == (Object)outTeeth || (Object)outJaw == (Object)outLips || (Object)outTeeth == (Object)outLips ) {
         throw streamFailure("ALLIGATOR openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return alligatorOpenAndFillInternal(inHigh, inLow, 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outJaw, outTeeth, outLips);
   }
