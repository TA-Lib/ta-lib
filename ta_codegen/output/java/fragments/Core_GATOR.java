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
 *  110726 MF,CC  First version (issue #583).
 */

   /**
    * Number of leading input bars {@link Core#gator} consumes before it can
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
   public int gatorLookback( int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift )
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
      int backJaw;
      int backTeethUpper;
      int backTeethLower;
      int backLips;
      int lookback;
      int legLookback;
      /* Each histogram pairs its two lines as a chart draws them: the line drawn
       * further ahead is read back by the difference of the two shifts. Each term
       * is that leg's rma_lookback plus its read-back, so every leg inherits
       * TA_FUNC_UNST_RMA.
       */
      backJaw = 0;
      backTeethUpper = 0;
      if( optInJawShift > optInTeethShift ) {
         backJaw = optInJawShift - optInTeethShift;
      } else {
         backTeethUpper = optInTeethShift - optInJawShift;
      }
      backTeethLower = 0;
      backLips = 0;
      if( optInTeethShift > optInLipsShift ) {
         backTeethLower = optInTeethShift - optInLipsShift;
      } else {
         backLips = optInLipsShift - optInTeethShift;
      }
      lookback = rmaLookback(optInJawPeriod) + backJaw;
      legLookback = rmaLookback(optInTeethPeriod) + backTeethUpper;
      if( legLookback > lookback ) {
         lookback = legLookback;
      }
      legLookback = rmaLookback(optInTeethPeriod) + backTeethLower;
      if( legLookback > lookback ) {
         lookback = legLookback;
      }
      legLookback = rmaLookback(optInLipsPeriod) + backLips;
      if( legLookback > lookback ) {
         lookback = legLookback;
      }
      return lookback ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#gator}.
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
   public int gatorDisplayShift( int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, int outputIdx )
   {
      if( gatorLookback( optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift ) < 0 ) {
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
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx == 0 ) {
         return (optInJawShift < optInTeethShift) ? optInJawShift : optInTeethShift ;
      }
      return (optInTeethShift < optInLipsShift) ? optInTeethShift : optInLipsShift ;

   }
   RetCode gatorImpl( int startIdx,
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
                      double outUpper[],
                      double outLower[] )
   {
      RetCode retCode;
      int i = 0;
      int nbElement = 0;
      int maxBack = 0;
      int lookbackTotal = 0;
      int mpStartIdx = 0;
      int localStart = 0;
      int localEnd = 0;
      int backJaw = 0;
      int backTeethUpper = 0;
      int backTeethLower = 0;
      int backLips = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempMP;
      double[] tempA;
      double[] tempB;
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
      if( outUpper == outLower ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
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
      backJaw = 0;
      backTeethUpper = 0;
      if( optInJawShift > optInTeethShift ) {
         backJaw = optInJawShift - optInTeethShift;
      } else {
         backTeethUpper = optInTeethShift - optInJawShift;
      }
      backTeethLower = 0;
      backLips = 0;
      if( optInTeethShift > optInLipsShift ) {
         backTeethLower = optInTeethShift - optInLipsShift;
      } else {
         backLips = optInLipsShift - optInTeethShift;
      }
      maxBack = backJaw + backTeethUpper;
      if( backTeethLower + backLips > maxBack ) {
         maxBack = backTeethLower + backLips;
      }
      mpStartIdx = startIdx - lookbackTotal;
      localStart = lookbackTotal;
      localEnd = endIdx - mpStartIdx;
      nbElement = endIdx - startIdx + 1;
      tempMP = new double[(int)((endIdx - mpStartIdx + 1) * 1)];
      tempA = new double[(int)((nbElement + maxBack) * 1)];
      tempB = new double[(int)((nbElement + maxBack) * 1)];
      /* Every price is consumed here, before any output is written: an output
       * may be aliased onto inHigh or inLow.
       */
      OutRange _xr0 = medprice(mpStartIdx, endIdx, inHigh, inLow, tempMP);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      /* Each leg is its own rma call starting at its read-back bar and seeded at
       * its own lookback before that, so neither histogram moves when the line it
       * does not read changes; the teeth is therefore computed once per
       * histogram. A leg read back by `back` bars starts that many bars early, so
       * element i of its buffer is the bar the output's element i pairs it with.
       */
      OutRange _xr1 = rma(localStart - backJaw, localEnd, tempMP, optInJawPeriod, tempA);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = rma(localStart - backTeethUpper, localEnd, tempMP, optInTeethPeriod, tempB);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < nbElement; i += 1 ) {
         outUpper[i] = Math.abs(tempA[i] - tempB[i]);
      }
      OutRange _xr3 = rma(localStart - backTeethLower, localEnd, tempMP, optInTeethPeriod, tempA);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr4 = rma(localStart - backLips, localEnd, tempMP, optInLipsPeriod, tempB);
      tempBegIdx.value = _xr4.begIdx();
      tempNbElement.value = _xr4.count();
      retCode = RetCode.SUCCESS;
      /* A zero spread must come out +0.0. Keep the select: gcc folds the literal
       * 0.0 - fabs(x) into -fabs(x), which is -0.0 there.
       */
      for( i = 0; i < nbElement; i += 1 ) {
         outLower[i] = (tempA[i] < tempB[i]) ? tempA[i] - tempB[i] : tempB[i] - tempA[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.SUCCESS ;
   }
   RetCode gatorImpl( int startIdx,
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
                      double outUpper[],
                      double outLower[] )
   {
      RetCode retCode;
      int i = 0;
      int nbElement = 0;
      int maxBack = 0;
      int lookbackTotal = 0;
      int mpStartIdx = 0;
      int localStart = 0;
      int localEnd = 0;
      int backJaw = 0;
      int backTeethUpper = 0;
      int backTeethLower = 0;
      int backLips = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempMP;
      double[] tempA;
      double[] tempB;
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
      if( outUpper == outLower ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      backJaw = 0;
      backTeethUpper = 0;
      if( optInJawShift > optInTeethShift ) {
         backJaw = optInJawShift - optInTeethShift;
      } else {
         backTeethUpper = optInTeethShift - optInJawShift;
      }
      backTeethLower = 0;
      backLips = 0;
      if( optInTeethShift > optInLipsShift ) {
         backTeethLower = optInTeethShift - optInLipsShift;
      } else {
         backLips = optInLipsShift - optInTeethShift;
      }
      maxBack = backJaw + backTeethUpper;
      if( backTeethLower + backLips > maxBack ) {
         maxBack = backTeethLower + backLips;
      }
      mpStartIdx = startIdx - lookbackTotal;
      localStart = lookbackTotal;
      localEnd = endIdx - mpStartIdx;
      nbElement = endIdx - startIdx + 1;
      tempMP = new double[(int)((endIdx - mpStartIdx + 1) * 1)];
      tempA = new double[(int)((nbElement + maxBack) * 1)];
      tempB = new double[(int)((nbElement + maxBack) * 1)];
      OutRange _xr0 = medprice(mpStartIdx, endIdx, inHigh, inLow, tempMP);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = rma(localStart - backJaw, localEnd, tempMP, optInJawPeriod, tempA);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = rma(localStart - backTeethUpper, localEnd, tempMP, optInTeethPeriod, tempB);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < nbElement; i += 1 ) {
         outUpper[i] = Math.abs(tempA[i] - tempB[i]);
      }
      OutRange _xr3 = rma(localStart - backTeethLower, localEnd, tempMP, optInTeethPeriod, tempA);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr4 = rma(localStart - backLips, localEnd, tempMP, optInLipsPeriod, tempB);
      tempBegIdx.value = _xr4.begIdx();
      tempNbElement.value = _xr4.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < nbElement; i += 1 ) {
         outLower[i] = (tempA[i] < tempB[i]) ? tempA[i] - tempB[i] : tempB[i] - tempA[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      return RetCode.SUCCESS ;
   }
   /**
    * Bill Williams' Gator Oscillator: how far apart the three Alligator lines
    * are, as two histograms. The upper bars are the gap between Jaw and Teeth,
    * the lower bars the gap between Teeth and Lips drawn below zero. Both
    * growing is the alligator eating, a trend under way; both shrinking toward
    * zero is it falling asleep, the lines converging in a range. One side
    * growing while the other shrinks is it waking or sated, the turn from one
    * state into the other. It is read under the Alligator (<a
    * href="https://ta-lib.org/functions/alligator">{@code ALLIGATOR}</a>), the
    * Profitunity leg it measures.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/gator">ta-lib.org/functions/gator</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Each line is drawn ahead by its own shift, so the line drawn further ahead is read back by the difference of the two shifts: at the default shifts Upper_t = | Jaw_t−3 − Teeth_t | and Lower_t = −| Teeth_t−2 − Lips_t |. {@code TA_GATOR_DisplayShift} reports where a chart draws each histogram, the smaller of its two lines' shifts; it changes nothing about the values or the returned range.</li>
    * <li>Each histogram computes its own lines, each seeded over the bars its own lookback reaches back, so the Teeth an output reads can differ from the {@code TA_ALLIGATOR} Teeth by the seed until it has decayed; setting the {@code RMA} unstable period closes the gap.</li>
    * <li>A zero Lower is {@code +0.0}.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#gatorLookback} is a <b>success with no values</b>
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
    * @param outUpper Gap between the Jaw and the Teeth, zero or above. Must
    *        hold at least {@code endIdx - max(startIdx, gatorLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @param outLower Gap between the Teeth and the Lips, zero or below. Must
    *        hold at least {@code endIdx - max(startIdx, gatorLookback(...)) + 1}
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
    * @see Core#alligator
    * @see Core#ao
    * @see Core#ac
    * @see Core#rma
    */
   public OutRange gator( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          int optInJawPeriod,
                          int optInJawShift,
                          int optInTeethPeriod,
                          int optInTeethShift,
                          int optInLipsPeriod,
                          int optInLipsShift,
                          double outUpper[],
                          double outLower[] )
   {
      requireIndexRange("GATOR", startIdx, endIdx);
      int guardStart = clampedStart("GATOR", startIdx, gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("GATOR", "inHigh", inHigh, guardInLen);
      requireLength("GATOR", "inLow", inLow, guardInLen);
      requireLength("GATOR", "outUpper", outUpper, guardOutLen);
      requireLength("GATOR", "outLower", outLower, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = gatorImpl(startIdx, endIdx, inHigh, inLow, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outUpper, outLower);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("GATOR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bill Williams' Gator Oscillator: how far apart the three Alligator lines
    * are, as two histograms. The upper bars are the gap between Jaw and Teeth,
    * the lower bars the gap between Teeth and Lips drawn below zero. Both
    * growing is the alligator eating, a trend under way; both shrinking toward
    * zero is it falling asleep, the lines converging in a range. One side
    * growing while the other shrinks is it waking or sated, the turn from one
    * state into the other. It is read under the Alligator (<a
    * href="https://ta-lib.org/functions/alligator">{@code ALLIGATOR}</a>), the
    * Profitunity leg it measures.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/gator">ta-lib.org/functions/gator</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Each line is drawn ahead by its own shift, so the line drawn further ahead is read back by the difference of the two shifts: at the default shifts Upper_t = | Jaw_t−3 − Teeth_t | and Lower_t = −| Teeth_t−2 − Lips_t |. {@code TA_GATOR_DisplayShift} reports where a chart draws each histogram, the smaller of its two lines' shifts; it changes nothing about the values or the returned range.</li>
    * <li>Each histogram computes its own lines, each seeded over the bars its own lookback reaches back, so the Teeth an output reads can differ from the {@code TA_ALLIGATOR} Teeth by the seed until it has decayed; setting the {@code RMA} unstable period closes the gap.</li>
    * <li>A zero Lower is {@code +0.0}.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#gatorLookback} is a <b>success with no values</b>
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
    * @param outUpper Gap between the Jaw and the Teeth, zero or above. Must
    *        hold at least {@code endIdx - max(startIdx, gatorLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @param outLower Gap between the Teeth and the Lips, zero or below. Must
    *        hold at least {@code endIdx - max(startIdx, gatorLookback(...)) + 1}
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
    * @see Core#alligator
    * @see Core#ao
    * @see Core#ac
    * @see Core#rma
    */
   public OutRange gator( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          int optInJawPeriod,
                          int optInJawShift,
                          int optInTeethPeriod,
                          int optInTeethShift,
                          int optInLipsPeriod,
                          int optInLipsShift,
                          double outUpper[],
                          double outLower[] )
   {
      requireIndexRange("GATOR", startIdx, endIdx);
      int guardStart = clampedStart("GATOR", startIdx, gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("GATOR", "inHigh", inHigh, guardInLen);
      requireLength("GATOR", "inLow", inLow, guardInLen);
      requireLength("GATOR", "outUpper", outUpper, guardOutLen);
      requireLength("GATOR", "outLower", outLower, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = gatorImpl(startIdx, endIdx, inHigh, inLow, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outUpper, outLower);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("GATOR", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live GATOR stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#gator} over the same series.
    * Open with {@link Core#gatorOpen}; there is no close — the handle is
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
   public static final class GatorStream {
      private Core core;
      private int optInJawPeriod;
      private int optInJawShift;
      private int optInTeethPeriod;
      private int optInTeethShift;
      private int optInLipsPeriod;
      private int optInLipsShift;
      private double cur_outUpper;
      private double cur_outLower;
      private int lagPos0;
      private int lagCap0;
      private double[] lagBuf0;
      private int lagPos1;
      private int lagCap1;
      private double[] lagBuf1;
      private int lagPos2;
      private int lagCap2;
      private double[] lagBuf2;
      private int lagPos3;
      private int lagCap3;
      private double[] lagBuf3;
      private MedpriceStream sub0;
      private RmaStream sub1;
      private RmaStream sub2;
      private RmaStream sub3;
      private RmaStream sub4;
      private int outRangeBegIdx;
      private int outRangeCount;

      private GatorStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#gator} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(GatorOut)} keeps answering the previous
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
            throw failure("GATOR advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private GatorStream( GatorStream other ) {
         this.core = other.core;
         this.optInJawPeriod = other.optInJawPeriod;
         this.optInJawShift = other.optInJawShift;
         this.optInTeethPeriod = other.optInTeethPeriod;
         this.optInTeethShift = other.optInTeethShift;
         this.optInLipsPeriod = other.optInLipsPeriod;
         this.optInLipsShift = other.optInLipsShift;
         this.cur_outUpper = other.cur_outUpper;
         this.cur_outLower = other.cur_outLower;
         this.lagPos0 = other.lagPos0;
         this.lagCap0 = other.lagCap0;
         this.lagBuf0 = other.lagBuf0.clone();
         this.lagPos1 = other.lagPos1;
         this.lagCap1 = other.lagCap1;
         this.lagBuf1 = other.lagBuf1.clone();
         this.lagPos2 = other.lagPos2;
         this.lagCap2 = other.lagCap2;
         this.lagBuf2 = other.lagBuf2.clone();
         this.lagPos3 = other.lagPos3;
         this.lagCap3 = other.lagCap3;
         this.lagBuf3 = other.lagBuf3.clone();
         this.sub0 = new MedpriceStream(other.sub0);
         this.sub1 = new RmaStream(other.sub1);
         this.sub2 = new RmaStream(other.sub2);
         this.sub3 = new RmaStream(other.sub3);
         this.sub4 = new RmaStream(other.sub4);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(GatorOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, GatorOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("GATOR update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("GATOR update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("GATOR update", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         core.gatorStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         out.upper = this.cur_outUpper;
         out.lower = this.cur_outLower;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, GatorOut out ) {
         requireArgument("GATOR peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("GATOR peek", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         GatorStream sp = this;
         double cur_tempMP = 0.0;
         double cur_tempA = 0.0;
         double cur_tempB = 0.0;
         double cur_outUpper = 0.0;
         double cur_outLower = 0.0;
         double lagIn0 = 0.0;
         double lagTap0_0 = 0.0;
         double lagIn1 = 0.0;
         double lagTap1_0 = 0.0;
         double lagIn2 = 0.0;
         double lagTap2_0 = 0.0;
         double lagIn3 = 0.0;
         double lagTap3_0 = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempMP = sp.sub0.peek(inHigh, inLow);
         cur_tempA = sp.sub1.peek(cur_tempMP);
         lagIn0 = cur_tempA;
         cur_tempB = sp.sub2.peek(cur_tempMP);
         lagIn1 = cur_tempB;
         /* Combine map (batch tail, per bar). */
         lagTap0_0 = sp.lagBuf0[sp.lagPos0];
         lagTap0_0 = (sp.lagCap0 == 0) ? lagIn0 : lagTap0_0;
         lagTap1_0 = sp.lagBuf1[sp.lagPos1];
         lagTap1_0 = (sp.lagCap1 == 0) ? lagIn1 : lagTap1_0;
         cur_outUpper = Math.abs(lagTap0_0 - lagTap1_0);
         cur_tempA = sp.sub3.peek(cur_tempMP);
         lagIn2 = cur_tempA;
         cur_tempB = sp.sub4.peek(cur_tempMP);
         lagIn3 = cur_tempB;
         /* Combine map (batch tail, per bar). */
         lagTap2_0 = sp.lagBuf2[sp.lagPos2];
         lagTap2_0 = (sp.lagCap2 == 0) ? lagIn2 : lagTap2_0;
         lagTap3_0 = sp.lagBuf3[sp.lagPos3];
         lagTap3_0 = (sp.lagCap3 == 0) ? lagIn3 : lagTap3_0;
         cur_outLower = (lagTap2_0 < lagTap3_0) ? lagTap2_0 - lagTap3_0 : lagTap3_0 - lagTap2_0;
         out.upper = cur_outUpper;
         out.lower = cur_outLower;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( GatorOut out ) {
         requireArgument("GATOR value", "out", out);
         out.upper = this.cur_outUpper;
         out.lower = this.cur_outLower;
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
      public GatorStream clone() {
         return new GatorStream(this);
      }
   }

   /**
    * The outputs of one GATOR bar, written by the stream into an object the
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
   public static final class GatorOut {
      /** Gap between the Jaw and the Teeth, zero or above. */
      public double upper;
      /** Gap between the Teeth and the Lips, zero or below. */
      public double lower;
   }
   private void gatorStepImpl( GatorStream sp, double inHigh, double inLow )
   {
      double cur_tempMP = 0.0;
      double cur_tempA = 0.0;
      double cur_tempB = 0.0;
      double cur_outUpper = 0.0;
      double cur_outLower = 0.0;
      double lagIn0 = 0.0;
      double lagTap0_0 = 0.0;
      double lagIn1 = 0.0;
      double lagTap1_0 = 0.0;
      double lagIn2 = 0.0;
      double lagTap2_0 = 0.0;
      double lagIn3 = 0.0;
      double lagTap3_0 = 0.0;
      int lagCapL0 = 0;
      int lagPosL0 = 0;
      int lagCapL1 = 0;
      int lagPosL1 = 0;
      int lagCapL2 = 0;
      int lagPosL2 = 0;
      int lagCapL3 = 0;
      int lagPosL3 = 0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempMP = sp.sub0.update(inHigh, inLow);
      cur_tempA = sp.sub1.update(cur_tempMP);
      lagIn0 = cur_tempA;
      cur_tempB = sp.sub2.update(cur_tempMP);
      lagIn1 = cur_tempB;
      /* Combine map (batch tail, per bar). */
      lagTap0_0 = sp.lagBuf0[sp.lagPos0];
      lagTap0_0 = (sp.lagCap0 == 0) ? lagIn0 : lagTap0_0;
      lagTap1_0 = sp.lagBuf1[sp.lagPos1];
      lagTap1_0 = (sp.lagCap1 == 0) ? lagIn1 : lagTap1_0;
      cur_outUpper = Math.abs(lagTap0_0 - lagTap1_0);
      cur_tempA = sp.sub3.update(cur_tempMP);
      lagIn2 = cur_tempA;
      cur_tempB = sp.sub4.update(cur_tempMP);
      lagIn3 = cur_tempB;
      /* Combine map (batch tail, per bar). */
      lagTap2_0 = sp.lagBuf2[sp.lagPos2];
      lagTap2_0 = (sp.lagCap2 == 0) ? lagIn2 : lagTap2_0;
      lagTap3_0 = sp.lagBuf3[sp.lagPos3];
      lagTap3_0 = (sp.lagCap3 == 0) ? lagIn3 : lagTap3_0;
      cur_outLower = (lagTap2_0 < lagTap3_0) ? lagTap2_0 - lagTap3_0 : lagTap3_0 - lagTap2_0;
      lagCapL0 = sp.lagCap0;
      lagPosL0 = sp.lagPos0;
      sp.lagBuf0[lagPosL0] = lagIn0;
      lagPosL0 = lagPosL0 + 1;
      if( lagPosL0 >= lagCapL0 ) {
         lagPosL0 = 0;
      }
      sp.lagPos0 = lagPosL0;
      lagCapL1 = sp.lagCap1;
      lagPosL1 = sp.lagPos1;
      sp.lagBuf1[lagPosL1] = lagIn1;
      lagPosL1 = lagPosL1 + 1;
      if( lagPosL1 >= lagCapL1 ) {
         lagPosL1 = 0;
      }
      sp.lagPos1 = lagPosL1;
      lagCapL2 = sp.lagCap2;
      lagPosL2 = sp.lagPos2;
      sp.lagBuf2[lagPosL2] = lagIn2;
      lagPosL2 = lagPosL2 + 1;
      if( lagPosL2 >= lagCapL2 ) {
         lagPosL2 = 0;
      }
      sp.lagPos2 = lagPosL2;
      lagCapL3 = sp.lagCap3;
      lagPosL3 = sp.lagPos3;
      sp.lagBuf3[lagPosL3] = lagIn3;
      lagPosL3 = lagPosL3 + 1;
      if( lagPosL3 >= lagCapL3 ) {
         lagPosL3 = 0;
      }
      sp.lagPos3 = lagPosL3;
      sp.cur_outUpper = cur_outUpper;
      sp.cur_outLower = cur_outLower;
   }
   private RetCode gatorOpenImpl( GatorStream sp, double inHigh[], double inLow[], int startIdx, int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, MInteger outBegIdx, MInteger outNBElement, double outUpper[], double outLower[], int outStride )
   {
      RetCode retCode;
      int i = 0;
      int nbElement = 0;
      int maxBack = 0;
      int lookbackTotal = 0;
      int mpStartIdx = 0;
      int localStart = 0;
      int localEnd = 0;
      int backJaw = 0;
      int backTeethUpper = 0;
      int backTeethLower = 0;
      int backLips = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempMP;
      double[] tempA;
      double[] tempB;
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
      if( historyLen < gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outUpper = outStride == 1 ? outUpper : new double[historyLen];
      double[] sc_outLower = outStride == 1 ? outLower : new double[historyLen];
      int lagPos0 = 0;
      int lagCap0 = 0;
      double[] lagBuf0 = null;
      int lagPos1 = 0;
      int lagCap1 = 0;
      double[] lagBuf1 = null;
      int lagPos2 = 0;
      int lagCap2 = 0;
      double[] lagBuf2 = null;
      int lagPos3 = 0;
      int lagCap3 = 0;
      double[] lagBuf3 = null;
      int lagD0_0 = 0;
      int lagD1_0 = 0;
      int lagD2_0 = 0;
      int lagD3_0 = 0;
      int lagNb0 = 0;
      int lagNb1 = 0;
      int lagNb2 = 0;
      int lagNb3 = 0;
      lookbackTotal = gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
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
      backJaw = 0;
      backTeethUpper = 0;
      if( optInJawShift > optInTeethShift ) {
         backJaw = optInJawShift - optInTeethShift;
      } else {
         backTeethUpper = optInTeethShift - optInJawShift;
      }
      backTeethLower = 0;
      backLips = 0;
      if( optInTeethShift > optInLipsShift ) {
         backTeethLower = optInTeethShift - optInLipsShift;
      } else {
         backLips = optInLipsShift - optInTeethShift;
      }
      maxBack = backJaw + backTeethUpper;
      if( backTeethLower + backLips > maxBack ) {
         maxBack = backTeethLower + backLips;
      }
      mpStartIdx = startIdx - lookbackTotal;
      localStart = lookbackTotal;
      localEnd = endIdx - mpStartIdx;
      nbElement = endIdx - startIdx + 1;
      tempMP = new double[(int)((endIdx - mpStartIdx + 1) * 1)];
      tempA = new double[(int)((nbElement + maxBack) * 1)];
      tempB = new double[(int)((nbElement + maxBack) * 1)];
      /* Every price is consumed here, before any output is written: an output
       * may be aliased onto inHigh or inLow.
       */
      /* Sub-stream 0: medprice over `inHigh, inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MedpriceStream sub0 = medpriceOpenAndFillInternal(inHigh, inLow, mpStartIdx, tempBegIdx, tempNbElement, tempMP);
      retCode = RetCode.SUCCESS;
      /* Each leg is its own rma call starting at its read-back bar and seeded at
       * its own lookback before that, so neither histogram moves when the line it
       * does not read changes; the teeth is therefore computed once per
       * histogram. A leg read back by `back` bars starts that many bars early, so
       * element i of its buffer is the bar the output's element i pairs it with.
       */
      /* Sub-stream 1: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub1 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (localEnd) + 1), localStart - backJaw, optInJawPeriod, tempBegIdx, tempNbElement, tempA);
      retCode = RetCode.SUCCESS;
      lagNb0 = tempNbElement.value;
      /* Sub-stream 2: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub2 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (localEnd) + 1), localStart - backTeethUpper, optInTeethPeriod, tempBegIdx, tempNbElement, tempB);
      retCode = RetCode.SUCCESS;
      lagNb1 = tempNbElement.value;
      if( nbElement < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      lagD0_0 = lagNb0 - nbElement;
      if( lagD0_0 < 0 || lagD0_0 > lagNb0 - 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      lagD1_0 = lagNb1 - nbElement;
      if( lagD1_0 < 0 || lagD1_0 > lagNb1 - 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      lagCap0 = lagD0_0;
      lagBuf0 = new double[Math.max(lagCap0, 1)];
      System.arraycopy(tempA, lagNb0 - lagCap0, lagBuf0, 0, lagCap0);
      lagPos0 = 0;
      lagCap1 = lagD1_0;
      lagBuf1 = new double[Math.max(lagCap1, 1)];
      System.arraycopy(tempB, lagNb1 - lagCap1, lagBuf1, 0, lagCap1);
      lagPos1 = 0;
      for( i = 0; i < nbElement; i += 1 ) {
         sc_outUpper[i] = Math.abs(tempA[i] - tempB[i]);
      }
      /* Sub-stream 3: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub3 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (localEnd) + 1), localStart - backTeethLower, optInTeethPeriod, tempBegIdx, tempNbElement, tempA);
      retCode = RetCode.SUCCESS;
      lagNb2 = tempNbElement.value;
      /* Sub-stream 4: rma over `tempMP`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      RmaStream sub4 = rmaOpenAndFillInternal(java.util.Arrays.copyOfRange(tempMP, 0, (localEnd) + 1), localStart - backLips, optInLipsPeriod, tempBegIdx, tempNbElement, tempB);
      retCode = RetCode.SUCCESS;
      lagNb3 = tempNbElement.value;
      /* A zero spread must come out +0.0. Keep the select: gcc folds the literal
       * 0.0 - fabs(x) into -fabs(x), which is -0.0 there.
       */
      if( nbElement < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      lagD2_0 = lagNb2 - nbElement;
      if( lagD2_0 < 0 || lagD2_0 > lagNb2 - 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      lagD3_0 = lagNb3 - nbElement;
      if( lagD3_0 < 0 || lagD3_0 > lagNb3 - 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      lagCap2 = lagD2_0;
      lagBuf2 = new double[Math.max(lagCap2, 1)];
      System.arraycopy(tempA, lagNb2 - lagCap2, lagBuf2, 0, lagCap2);
      lagPos2 = 0;
      lagCap3 = lagD3_0;
      lagBuf3 = new double[Math.max(lagCap3, 1)];
      System.arraycopy(tempB, lagNb3 - lagCap3, lagBuf3, 0, lagCap3);
      lagPos3 = 0;
      for( i = 0; i < nbElement; i += 1 ) {
         sc_outLower[i] = (tempA[i] < tempB[i]) ? tempA[i] - tempB[i] : tempB[i] - tempA[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = nbElement;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
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
      sp.sub4 = sub4;
      sp.lagPos0 = lagPos0;
      sp.lagCap0 = lagCap0;
      sp.lagBuf0 = lagBuf0;
      sp.lagPos1 = lagPos1;
      sp.lagCap1 = lagCap1;
      sp.lagBuf1 = lagBuf1;
      sp.lagPos2 = lagPos2;
      sp.lagCap2 = lagCap2;
      sp.lagBuf2 = lagBuf2;
      sp.lagPos3 = lagPos3;
      sp.lagCap3 = lagCap3;
      sp.lagBuf3 = lagBuf3;
      sp.cur_outUpper = sc_outUpper[outNBElement.value - 1];
      sp.cur_outLower = sc_outLower[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* gatorOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   GatorStream gatorOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, MInteger outBegIdx, MInteger outNBElement, double outUpper[], double outLower[] )
   {
      GatorStream sp = new GatorStream(this);
      RetCode retCode = gatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outUpper, outLower, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("GATOR openAndFill", inHigh.length, startIdx, gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      }
      throw streamFailure("GATOR openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind gatorOpen (composition seam). */
   GatorStream gatorOpenInternal( double inHigh[], double inLow[], int startIdx, int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift )
   {
      GatorStream sp = new GatorStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outUpper = new double[1];
      double[] sink_outLower = new double[1];
      RetCode retCode = gatorOpenImpl(sp, inHigh, inLow, startIdx, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, sink_outUpper, sink_outLower, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("GATOR open", inHigh.length, startIdx, gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      }
      throw streamFailure("GATOR open", retCode);
   }
   /**
    * Open a live GATOR stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#gator} at that bar.
    * <p>The history must hold at least {@code gatorLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown, an empty history included. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). A null argument throws
    * {@link IllegalArgumentException}, ahead of everything above.
    */
   public GatorStream gatorOpen( double inHigh[], double inLow[], int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift )
   {
      requireArgument("GATOR open", "inHigh", inHigh);
      requireHistory("GATOR open", inHigh.length);
      requireArgument("GATOR open", "inLow", inLow);
      requireHistoryLength("GATOR open", "inLow", inLow.length, inHigh.length);
      return gatorOpenInternal(inHigh, inLow, 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift);
   }
   /**
    * {@link Core#gatorOpen} that also fills the output array(s) bit-identically
    * to {@link Core#gator} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link GatorStream#outRange()}.
    */
   public GatorStream gatorOpenAndFill( double inHigh[], double inLow[], int optInJawPeriod, int optInJawShift, int optInTeethPeriod, int optInTeethShift, int optInLipsPeriod, int optInLipsShift, double outUpper[], double outLower[] )
   {
      requireArgument("GATOR openAndFill", "inHigh", inHigh);
      requireHistory("GATOR openAndFill", inHigh.length);
      requireArgument("GATOR openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("GATOR openAndFill", inHigh.length, gatorLookback(optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift));
      requireHistoryLength("GATOR openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("GATOR openAndFill", "outUpper", outUpper, guardOutLen);
      requireLength("GATOR openAndFill", "outLower", outLower, guardOutLen);
      if( (Object)outUpper == (Object)inHigh || (Object)outUpper == (Object)inLow || (Object)outLower == (Object)inHigh || (Object)outLower == (Object)inLow || (Object)outUpper == (Object)outLower ) {
         throw streamFailure("GATOR openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return gatorOpenAndFillInternal(inHigh, inLow, 0, optInJawPeriod, optInJawShift, optInTeethPeriod, optInTeethShift, optInLipsPeriod, optInLipsShift, outBegIdx, outNBElement, outUpper, outLower);
   }
