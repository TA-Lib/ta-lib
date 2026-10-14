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
 *  100726 KL,CC  Initial version (#484).
 *  101426 MF,CC  The EMA step is TA_EMA's; the trend and the range are selects.
 */

   /**
    * Number of leading input bars {@link Core#kvo} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the faster smoothing of the volume force
    *        (default 34; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the slower smoothing of the volume force
    *        (default 55; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the trigger line (default 13;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int kvoLookback( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 34;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 55;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 13;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      int longestPeriod;
      /* The three EMAs seed on one bar, so one EMA lookback of the longest
       * covers all three. The 1 is the reference bar before it.
       */
      longestPeriod = optInFastPeriod;
      if( optInSlowPeriod > longestPeriod ) {
         longestPeriod = optInSlowPeriod;
      }
      if( optInSignalPeriod > longestPeriod ) {
         longestPeriod = optInSignalPeriod;
      }
      return 1 + emaLookback(longestPeriod) + ((this.unstableCount(FuncUnstId.EMA.ordinal(), 1, 1) != this.unstableCount(FuncUnstId.EMA.ordinal(), 0, 0)) ? (this.unstableCount(FuncUnstId.EMA.ordinal(), (((Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) > 1) ? ((4 * 10 + 23) / 3 * Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) + 1) / 2 : 0) - ((Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) > 1) ? (10 * Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) + 1) / 2 : 0)), (((Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) > 1) ? ((4 * 19 + 23) / 3 * Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) + 1) / 2 : 0) - ((Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) > 1) ? (19 * Math.max(optInFastPeriod, Math.max(optInSlowPeriod, optInSignalPeriod)) + 1) / 2 : 0))) - this.unstableCount(FuncUnstId.EMA.ordinal(), 0, 0)) : 0) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#kvo}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInFastPeriod Period of the faster smoothing of the volume force
    *        (default 34; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the slower smoothing of the volume force
    *        (default 55; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the trigger line (default 13;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int kvoDisplayShift( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, int outputIdx )
   {
      if( kvoLookback( optInFastPeriod, optInSlowPeriod, optInSignalPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode kvoImpl( int startIdx,
                    int endIdx,
                    double inHigh[],
                    double inLow[],
                    double inClose[],
                    double inVolume[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    int optInSignalPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outKVO[],
                    double outKVOSignal[] )
   {
      double kFast = 0;
      double kSlow = 0;
      double kSignal = 0;
      double betaFast = 0;
      double betaSlow = 0;
      double betaSignal = 0;
      double hlc = 0;
      double prevHlc = 0;
      double dmToday = 0;
      double prevDm = 0;
      double cm = 0;
      double cmBase = 0;
      double vf = 0;
      double emaFast = 0;
      double emaSlow = 0;
      double kvoValue = 0;
      double signalValue = 0;
      double trend = 0;
      double prevTrend = 0;
      int lookbackTotal = 0;
      int today = 0;
      int outIdx = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 34;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 55;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 13;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outKVO == outKVOSignal ) {
         return RetCode.BAD_PARAM ;
      }
      /* Stephen J. Klinger, "Identifying Trends With Volume Analysis", Technical
       * Analysis of Stocks & Commodities V15:12 (December 1997), with the 1997
       * factor |2*(dm/cm) - 1| (#484, form A; LEAN's differs).
       *
       *    trend = +1 if H+L+C rose, -1 if it fell, unchanged on a tie
       *    cm    = previous + current range on a trend change, else cm + range
       *    vf    = volume * |2*(dm/cm) - 1| * 100 * trend
       *
       * The committed corpus holds a single tie in H+L+C, and reading it any other
       * way moves KVO by up to 1e7 from the next bar on: the tie rule is gated by
       * a built series in test_kvo.c, not by the corpus.
       *
       * All three EMAs seed raw on the first volume force, not on an SMA as
       * TA_EMA does. Each k + beta pair must sum to exactly 1.0, as in ema.c.
       */
      lookbackTotal = kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      betaFast = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      kFast = 1.0 - betaFast;
      if( betaFast < 0.5 ) {
         betaFast = 1.0 - kFast;
      }
      betaSlow = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      kSlow = 1.0 - betaSlow;
      if( betaSlow < 0.5 ) {
         betaSlow = 1.0 - kSlow;
      }
      betaSignal = (double)(optInSignalPeriod - 1) / (double)(optInSignalPeriod + 1);
      kSignal = 1.0 - betaSignal;
      if( betaSignal < 0.5 ) {
         betaSignal = 1.0 - kSignal;
      }
      /* The anchor: only its H+L+C and range are read, and the trend starts at +1. */
      today = startIdx - lookbackTotal;
      prevHlc = inHigh[today] + inLow[today] + inClose[today];
      prevDm = inHigh[today] - inLow[today];
      prevTrend = 1.0;
      today = today + 1;
      /* The seed bar: cm opens on both ranges whatever the trend, and both
       * averages start at this volume force, so KVO and the trigger are exactly 0.
       */
      hlc = inHigh[today] + inLow[today] + inClose[today];
      dmToday = inHigh[today] - inLow[today];
      trend = (hlc > prevHlc) ? 1.0 : prevTrend;
      trend = (hlc < prevHlc) ? -1.0 : trend;
      cm = prevDm + dmToday;
      if( cm == 0.0 ) {
         vf = 0.0;
      } else {
         vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
      }
      emaFast = vf;
      emaSlow = vf;
      kvoValue = emaFast - emaSlow;
      signalValue = kvoValue;
      prevHlc = hlc;
      prevDm = dmToday;
      prevTrend = trend;
      today = today + 1;
      while( today < startIdx ) {
         hlc = inHigh[today] + inLow[today] + inClose[today];
         dmToday = inHigh[today] - inLow[today];
         trend = (hlc > prevHlc) ? 1.0 : prevTrend;
         trend = (hlc < prevHlc) ? -1.0 : trend;
         cmBase = (trend != prevTrend) ? prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(betaFast, emaFast, kFast * vf);
         emaSlow = Math.fma(betaSlow, emaSlow, kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(betaSignal, signalValue, kSignal * kvoValue);
         prevHlc = hlc;
         prevDm = dmToday;
         prevTrend = trend;
         today = today + 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         hlc = inHigh[today] + inLow[today] + inClose[today];
         dmToday = inHigh[today] - inLow[today];
         trend = (hlc > prevHlc) ? 1.0 : prevTrend;
         trend = (hlc < prevHlc) ? -1.0 : trend;
         cmBase = (trend != prevTrend) ? prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(betaFast, emaFast, kFast * vf);
         emaSlow = Math.fma(betaSlow, emaSlow, kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(betaSignal, signalValue, kSignal * kvoValue);
         outKVO[outIdx] = kvoValue;
         outKVOSignal[outIdx] = signalValue;
         outIdx = outIdx + 1;
         prevHlc = hlc;
         prevDm = dmToday;
         prevTrend = trend;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode kvoImpl( int startIdx,
                    int endIdx,
                    float inHigh[],
                    float inLow[],
                    float inClose[],
                    float inVolume[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    int optInSignalPeriod,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outKVO[],
                    double outKVOSignal[] )
   {
      double kFast = 0;
      double kSlow = 0;
      double kSignal = 0;
      double betaFast = 0;
      double betaSlow = 0;
      double betaSignal = 0;
      double hlc = 0;
      double prevHlc = 0;
      double dmToday = 0;
      double prevDm = 0;
      double cm = 0;
      double cmBase = 0;
      double vf = 0;
      double emaFast = 0;
      double emaSlow = 0;
      double kvoValue = 0;
      double signalValue = 0;
      double trend = 0;
      double prevTrend = 0;
      int lookbackTotal = 0;
      int today = 0;
      int outIdx = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 34;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 55;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 13;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outKVO == outKVOSignal ) {
         return RetCode.BAD_PARAM ;
      }
      lookbackTotal = kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      betaFast = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      kFast = 1.0 - betaFast;
      if( betaFast < 0.5 ) {
         betaFast = 1.0 - kFast;
      }
      betaSlow = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      kSlow = 1.0 - betaSlow;
      if( betaSlow < 0.5 ) {
         betaSlow = 1.0 - kSlow;
      }
      betaSignal = (double)(optInSignalPeriod - 1) / (double)(optInSignalPeriod + 1);
      kSignal = 1.0 - betaSignal;
      if( betaSignal < 0.5 ) {
         betaSignal = 1.0 - kSignal;
      }
      today = startIdx - lookbackTotal;
      prevHlc = (double)inHigh[today] + (double)inLow[today] + (double)inClose[today];
      prevDm = (double)inHigh[today] - (double)inLow[today];
      prevTrend = 1.0;
      today = today + 1;
      hlc = (double)inHigh[today] + (double)inLow[today] + (double)inClose[today];
      dmToday = (double)inHigh[today] - (double)inLow[today];
      trend = (hlc > prevHlc) ? 1.0 : prevTrend;
      trend = (hlc < prevHlc) ? -1.0 : trend;
      cm = prevDm + dmToday;
      if( cm == 0.0 ) {
         vf = 0.0;
      } else {
         vf = (double)inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
      }
      emaFast = vf;
      emaSlow = vf;
      kvoValue = emaFast - emaSlow;
      signalValue = kvoValue;
      prevHlc = hlc;
      prevDm = dmToday;
      prevTrend = trend;
      today = today + 1;
      while( today < startIdx ) {
         hlc = (double)inHigh[today] + (double)inLow[today] + (double)inClose[today];
         dmToday = (double)inHigh[today] - (double)inLow[today];
         trend = (hlc > prevHlc) ? 1.0 : prevTrend;
         trend = (hlc < prevHlc) ? -1.0 : trend;
         cmBase = (trend != prevTrend) ? prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = (double)inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(betaFast, emaFast, kFast * vf);
         emaSlow = Math.fma(betaSlow, emaSlow, kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(betaSignal, signalValue, kSignal * kvoValue);
         prevHlc = hlc;
         prevDm = dmToday;
         prevTrend = trend;
         today = today + 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         hlc = (double)inHigh[today] + (double)inLow[today] + (double)inClose[today];
         dmToday = (double)inHigh[today] - (double)inLow[today];
         trend = (hlc > prevHlc) ? 1.0 : prevTrend;
         trend = (hlc < prevHlc) ? -1.0 : trend;
         cmBase = (trend != prevTrend) ? prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = (double)inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(betaFast, emaFast, kFast * vf);
         emaSlow = Math.fma(betaSlow, emaSlow, kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(betaSignal, signalValue, kSignal * kvoValue);
         outKVO[outIdx] = kvoValue;
         outKVOSignal[outIdx] = signalValue;
         outIdx = outIdx + 1;
         prevHlc = hlc;
         prevDm = dmToday;
         prevTrend = trend;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Klinger Volume Oscillator: each bar's volume is signed by the direction of
    * its high-low-close sum and weighted by where the bar's own range sits
    * inside the cumulative range of the current trend run, then the resulting
    * volume force is smoothed twice and differenced. Klinger's reading is that
    * volume alone says how much trading happened but not what it accomplished,
    * so he scales it by the share of the trend's accumulated range that this
    * bar actually covered. The oscillator crosses zero as the balance of that
    * force changes hands, and crosses its own trigger line earlier.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kvo">ta-lib.org/functions/kvo</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The trend is <b>held</b> when the high-low-close sum repeats, which is the article's own tenet: the existing trend is maintained on equality. A reading that breaks the tie in either direction changes every later bar, because the cumulative range is reset by a trend change.</li>
    * <li>The cumulative range is reset by every trend change, so two calls starting at different bars carry different range state only until the first trend change after both; what remains between them is the seed error of the three averages, which fades at the rate of the longest period. KVO converges, but as a small difference of two large averages it needs more bars than one of them would.</li>
    * <li>All three exponential averages are seeded from the volume force of the same anchor bar rather than from a simple average of their own first inputs, so one warm-up covers all three. KVO is exactly zero on that seed bar, which falls inside the warm-up and is never output. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up.</li>
    * <li>The weight is the 1997 form, {@code |2*(dm/cm) - 1|}, which is what the December 1997 Traders' Tips print. Some later implementations use {@code |2*(dm/cm - 1)|}, which is a different series.</li>
    * <li>A cumulative range of exactly zero leaves the volume force at zero rather than dividing.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#kvoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param inVolume Volume series.
    * @param optInFastPeriod Period of the faster smoothing of the volume force
    *        (default 34; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the slower smoothing of the volume force
    *        (default 55; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the trigger line (default 13;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outKVO Klinger Volume Oscillator. Must hold at least
    *        {@code endIdx - max(startIdx, kvoLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
    * @param outKVOSignal Trigger line, an exponential average of the
    *        oscillator. Must hold at least
    *        {@code endIdx - max(startIdx, kvoLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
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
    * @see Core#adosc
    * @see Core#obv
    * @see Core#mfi
    * @see Core#ad
    */
   public OutRange kvo( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        double inVolume[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInSignalPeriod,
                        double outKVO[],
                        double outKVOSignal[] )
   {
      requireIndexRange("KVO", startIdx, endIdx);
      int guardStart = clampedStart("KVO", startIdx, kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KVO", "inHigh", inHigh, guardInLen);
      requireLength("KVO", "inLow", inLow, guardInLen);
      requireLength("KVO", "inClose", inClose, guardInLen);
      requireLength("KVO", "inVolume", inVolume, guardInLen);
      requireLength("KVO", "outKVO", outKVO, guardOutLen);
      requireLength("KVO", "outKVOSignal", outKVOSignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kvoImpl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outKVO, outKVOSignal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KVO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Klinger Volume Oscillator: each bar's volume is signed by the direction of
    * its high-low-close sum and weighted by where the bar's own range sits
    * inside the cumulative range of the current trend run, then the resulting
    * volume force is smoothed twice and differenced. Klinger's reading is that
    * volume alone says how much trading happened but not what it accomplished,
    * so he scales it by the share of the trend's accumulated range that this
    * bar actually covered. The oscillator crosses zero as the balance of that
    * force changes hands, and crosses its own trigger line earlier.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/kvo">ta-lib.org/functions/kvo</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The trend is <b>held</b> when the high-low-close sum repeats, which is the article's own tenet: the existing trend is maintained on equality. A reading that breaks the tie in either direction changes every later bar, because the cumulative range is reset by a trend change.</li>
    * <li>The cumulative range is reset by every trend change, so two calls starting at different bars carry different range state only until the first trend change after both; what remains between them is the seed error of the three averages, which fades at the rate of the longest period. KVO converges, but as a small difference of two large averages it needs more bars than one of them would.</li>
    * <li>All three exponential averages are seeded from the volume force of the same anchor bar rather than from a simple average of their own first inputs, so one warm-up covers all three. KVO is exactly zero on that seed bar, which falls inside the warm-up and is never output. {@code TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)} discards more of that warm-up.</li>
    * <li>The weight is the 1997 form, {@code |2*(dm/cm) - 1|}, which is what the December 1997 Traders' Tips print. Some later implementations use {@code |2*(dm/cm - 1)|}, which is a different series.</li>
    * <li>A cumulative range of exactly zero leaves the volume force at zero rather than dividing.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#kvoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price series.
    * @param inLow Low price series.
    * @param inClose Close price series.
    * @param inVolume Volume series.
    * @param optInFastPeriod Period of the faster smoothing of the volume force
    *        (default 34; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowPeriod Period of the slower smoothing of the volume force
    *        (default 55; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the trigger line (default 13;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outKVO Klinger Volume Oscillator. Must hold at least
    *        {@code endIdx - max(startIdx, kvoLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
    * @param outKVOSignal Trigger line, an exponential average of the
    *        oscillator. Must hold at least
    *        {@code endIdx - max(startIdx, kvoLookback(...)) + 1} values, and never be
    *        empty: an empty array is an absent output.
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
    * @see Core#adosc
    * @see Core#obv
    * @see Core#mfi
    * @see Core#ad
    */
   public OutRange kvo( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        float inVolume[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        int optInSignalPeriod,
                        double outKVO[],
                        double outKVOSignal[] )
   {
      requireIndexRange("KVO", startIdx, endIdx);
      int guardStart = clampedStart("KVO", startIdx, kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("KVO", "inHigh", inHigh, guardInLen);
      requireLength("KVO", "inLow", inLow, guardInLen);
      requireLength("KVO", "inClose", inClose, guardInLen);
      requireLength("KVO", "inVolume", inVolume, guardInLen);
      requireLength("KVO", "outKVO", outKVO, guardOutLen);
      requireLength("KVO", "outKVOSignal", outKVOSignal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = kvoImpl(startIdx, endIdx, inHigh, inLow, inClose, inVolume, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outKVO, outKVOSignal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("KVO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live KVO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#kvo} over the same series.
    * Open with {@link Core#kvoOpen}; there is no close — the handle is
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
   public static final class KvoStream {
      private Core core;
      private int optInFastPeriod;
      private int optInSlowPeriod;
      private int optInSignalPeriod;
      private double kFast;
      private double kSlow;
      private double kSignal;
      private double betaFast;
      private double betaSlow;
      private double betaSignal;
      private double prevHlc;
      private double prevDm;
      private double cm;
      private double emaFast;
      private double emaSlow;
      private double signalValue;
      private double prevTrend;
      private double cur_outKVO;
      private double cur_outKVOSignal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private KvoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#kvo} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(KvoOut)} keeps answering the previous
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
            throw failure("KVO advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private KvoStream( KvoStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.kFast = other.kFast;
         this.kSlow = other.kSlow;
         this.kSignal = other.kSignal;
         this.betaFast = other.betaFast;
         this.betaSlow = other.betaSlow;
         this.betaSignal = other.betaSignal;
         this.prevHlc = other.prevHlc;
         this.prevDm = other.prevDm;
         this.cm = other.cm;
         this.emaFast = other.emaFast;
         this.emaSlow = other.emaSlow;
         this.signalValue = other.signalValue;
         this.prevTrend = other.prevTrend;
         this.cur_outKVO = other.cur_outKVO;
         this.cur_outKVOSignal = other.cur_outKVOSignal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(KvoOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, double inClose, double inVolume, KvoOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("KVO update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("KVO update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw nonFiniteBar("KVO update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : !Double.isFinite(inClose) ? "inClose" : "inVolume");
         core.kvoStepImpl(this, inHigh, inLow, inClose, inVolume);
         this.outRangeCount++;
         out.kvo = this.cur_outKVO;
         out.kvoSignal = this.cur_outKVOSignal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, double inClose, double inVolume, KvoOut out ) {
         requireArgument("KVO peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) || !Double.isFinite(inVolume) )
            throw nonFiniteBar("KVO peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : !Double.isFinite(inClose) ? "inClose" : "inVolume");
         KvoStream sp = this;
         double hlc = 0.0;
         double dmToday = 0.0;
         double cmBase = 0.0;
         double vf = 0.0;
         double kvoValue = 0.0;
         double trend = 0.0;
         double cm = sp.cm;
         double cur_outKVO = 0.0;
         double cur_outKVOSignal = 0.0;
         double emaFast = sp.emaFast;
         double emaSlow = sp.emaSlow;
         double signalValue = sp.signalValue;
         hlc = inHigh + inLow + inClose;
         dmToday = inHigh - inLow;
         trend = (hlc > sp.prevHlc) ? 1.0 : sp.prevTrend;
         trend = (hlc < sp.prevHlc) ? -1.0 : trend;
         cmBase = (trend != sp.prevTrend) ? sp.prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = inVolume * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(sp.betaFast, emaFast, sp.kFast * vf);
         emaSlow = Math.fma(sp.betaSlow, emaSlow, sp.kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(sp.betaSignal, signalValue, sp.kSignal * kvoValue);
         cur_outKVO = kvoValue;
         cur_outKVOSignal = signalValue;
         out.kvo = cur_outKVO;
         out.kvoSignal = cur_outKVOSignal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( KvoOut out ) {
         requireArgument("KVO value", "out", out);
         out.kvo = this.cur_outKVO;
         out.kvoSignal = this.cur_outKVOSignal;
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
      public KvoStream clone() {
         return new KvoStream(this);
      }
   }

   /**
    * The outputs of one KVO bar, written by the stream into an object the
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
   public static final class KvoOut {
      /** Klinger Volume Oscillator. */
      public double kvo;
      /** Trigger line, an exponential average of the oscillator. */
      public double kvoSignal;
   }
   private void kvoStepImpl( KvoStream sp, double inHigh, double inLow, double inClose, double inVolume )
   {
      double hlc = 0.0;
      double dmToday = 0.0;
      double cmBase = 0.0;
      double vf = 0.0;
      double kvoValue = 0.0;
      double trend = 0.0;
      hlc = inHigh + inLow + inClose;
      dmToday = inHigh - inLow;
      trend = (hlc > sp.prevHlc) ? 1.0 : sp.prevTrend;
      trend = (hlc < sp.prevHlc) ? -1.0 : trend;
      cmBase = (trend != sp.prevTrend) ? sp.prevDm : sp.cm;
      sp.cm = cmBase + dmToday;
      if( sp.cm == 0.0 ) {
         vf = 0.0;
      } else {
         vf = inVolume * Math.abs(2.0 * (dmToday / sp.cm) - 1.0) * 100.0 * trend;
      }
      sp.emaFast = Math.fma(sp.betaFast, sp.emaFast, sp.kFast * vf);
      sp.emaSlow = Math.fma(sp.betaSlow, sp.emaSlow, sp.kSlow * vf);
      kvoValue = sp.emaFast - sp.emaSlow;
      sp.signalValue = Math.fma(sp.betaSignal, sp.signalValue, sp.kSignal * kvoValue);
      sp.cur_outKVO = kvoValue;
      sp.cur_outKVOSignal = sp.signalValue;
      sp.prevHlc = hlc;
      sp.prevDm = dmToday;
      sp.prevTrend = trend;
   }
   private RetCode kvoOpenImpl( KvoStream sp, double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outKVO[], double outKVOSignal[], int outStride )
   {
      double kFast = 0;
      double kSlow = 0;
      double kSignal = 0;
      double betaFast = 0;
      double betaSlow = 0;
      double betaSignal = 0;
      double hlc = 0;
      double prevHlc = 0;
      double dmToday = 0;
      double prevDm = 0;
      double cm = 0;
      double cmBase = 0;
      double vf = 0;
      double emaFast = 0;
      double emaSlow = 0;
      double kvoValue = 0;
      double signalValue = 0;
      double trend = 0;
      double prevTrend = 0;
      int lookbackTotal = 0;
      int today = 0;
      int outIdx = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length || inVolume.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 34;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 55;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 13;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Stephen J. Klinger, "Identifying Trends With Volume Analysis", Technical
       * Analysis of Stocks & Commodities V15:12 (December 1997), with the 1997
       * factor |2*(dm/cm) - 1| (#484, form A; LEAN's differs).
       *
       *    trend = +1 if H+L+C rose, -1 if it fell, unchanged on a tie
       *    cm    = previous + current range on a trend change, else cm + range
       *    vf    = volume * |2*(dm/cm) - 1| * 100 * trend
       *
       * The committed corpus holds a single tie in H+L+C, and reading it any other
       * way moves KVO by up to 1e7 from the next bar on: the tie rule is gated by
       * a built series in test_kvo.c, not by the corpus.
       *
       * All three EMAs seed raw on the first volume force, not on an SMA as
       * TA_EMA does. Each k + beta pair must sum to exactly 1.0, as in ema.c.
       */
      lookbackTotal = kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      betaFast = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      kFast = 1.0 - betaFast;
      if( betaFast < 0.5 ) {
         betaFast = 1.0 - kFast;
      }
      betaSlow = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      kSlow = 1.0 - betaSlow;
      if( betaSlow < 0.5 ) {
         betaSlow = 1.0 - kSlow;
      }
      betaSignal = (double)(optInSignalPeriod - 1) / (double)(optInSignalPeriod + 1);
      kSignal = 1.0 - betaSignal;
      if( betaSignal < 0.5 ) {
         betaSignal = 1.0 - kSignal;
      }
      /* The anchor: only its H+L+C and range are read, and the trend starts at +1. */
      today = startIdx - lookbackTotal;
      prevHlc = inHigh[today] + inLow[today] + inClose[today];
      prevDm = inHigh[today] - inLow[today];
      prevTrend = 1.0;
      today = today + 1;
      /* The seed bar: cm opens on both ranges whatever the trend, and both
       * averages start at this volume force, so KVO and the trigger are exactly 0.
       */
      hlc = inHigh[today] + inLow[today] + inClose[today];
      dmToday = inHigh[today] - inLow[today];
      trend = (hlc > prevHlc) ? 1.0 : prevTrend;
      trend = (hlc < prevHlc) ? -1.0 : trend;
      cm = prevDm + dmToday;
      if( cm == 0.0 ) {
         vf = 0.0;
      } else {
         vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
      }
      emaFast = vf;
      emaSlow = vf;
      kvoValue = emaFast - emaSlow;
      signalValue = kvoValue;
      prevHlc = hlc;
      prevDm = dmToday;
      prevTrend = trend;
      today = today + 1;
      while( today < startIdx ) {
         hlc = inHigh[today] + inLow[today] + inClose[today];
         dmToday = inHigh[today] - inLow[today];
         trend = (hlc > prevHlc) ? 1.0 : prevTrend;
         trend = (hlc < prevHlc) ? -1.0 : trend;
         cmBase = (trend != prevTrend) ? prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(betaFast, emaFast, kFast * vf);
         emaSlow = Math.fma(betaSlow, emaSlow, kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(betaSignal, signalValue, kSignal * kvoValue);
         prevHlc = hlc;
         prevDm = dmToday;
         prevTrend = trend;
         today = today + 1;
      }
      outIdx = 0;
      while( today <= endIdx ) {
         hlc = inHigh[today] + inLow[today] + inClose[today];
         dmToday = inHigh[today] - inLow[today];
         trend = (hlc > prevHlc) ? 1.0 : prevTrend;
         trend = (hlc < prevHlc) ? -1.0 : trend;
         cmBase = (trend != prevTrend) ? prevDm : cm;
         cm = cmBase + dmToday;
         if( cm == 0.0 ) {
            vf = 0.0;
         } else {
            vf = inVolume[today] * Math.abs(2.0 * (dmToday / cm) - 1.0) * 100.0 * trend;
         }
         emaFast = Math.fma(betaFast, emaFast, kFast * vf);
         emaSlow = Math.fma(betaSlow, emaSlow, kSlow * vf);
         kvoValue = emaFast - emaSlow;
         signalValue = Math.fma(betaSignal, signalValue, kSignal * kvoValue);
         outKVO[outIdx * outStride] = kvoValue;
         outKVOSignal[outIdx * outStride] = signalValue;
         outIdx = outIdx + 1;
         prevHlc = hlc;
         prevDm = dmToday;
         prevTrend = trend;
         today = today + 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.kFast = kFast;
      sp.kSlow = kSlow;
      sp.kSignal = kSignal;
      sp.betaFast = betaFast;
      sp.betaSlow = betaSlow;
      sp.betaSignal = betaSignal;
      sp.prevHlc = prevHlc;
      sp.prevDm = prevDm;
      sp.cm = cm;
      sp.emaFast = emaFast;
      sp.emaSlow = emaSlow;
      sp.signalValue = signalValue;
      sp.prevTrend = prevTrend;
      sp.cur_outKVO = outKVO[(outNBElement.value - 1) * outStride];
      sp.cur_outKVOSignal = outKVOSignal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* kvoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   KvoStream kvoOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outKVO[], double outKVOSignal[] )
   {
      KvoStream sp = new KvoStream(this);
      RetCode retCode = kvoOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outKVO, outKVOSignal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("KVO openAndFill", inHigh.length, startIdx, kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      }
      throw streamFailure("KVO openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind kvoOpen (composition seam). */
   KvoStream kvoOpenInternal( double inHigh[], double inLow[], double inClose[], double inVolume[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      KvoStream sp = new KvoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outKVO = new double[1];
      double[] sink_outKVOSignal = new double[1];
      RetCode retCode = kvoOpenImpl(sp, inHigh, inLow, inClose, inVolume, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outKVO, sink_outKVOSignal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("KVO open", inHigh.length, startIdx, kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      }
      throw streamFailure("KVO open", retCode);
   }
   /**
    * Open a live KVO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#kvo} at that bar.
    * <p>The history must hold at least {@code kvoLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public KvoStream kvoOpen( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      requireArgument("KVO open", "inHigh", inHigh);
      requireHistory("KVO open", inHigh.length);
      requireArgument("KVO open", "inLow", inLow);
      requireArgument("KVO open", "inClose", inClose);
      requireArgument("KVO open", "inVolume", inVolume);
      requireHistoryLength("KVO open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("KVO open", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("KVO open", "inVolume", inVolume.length, inHigh.length);
      return kvoOpenInternal(inHigh, inLow, inClose, inVolume, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#kvoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#kvo} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link KvoStream#outRange()}.
    */
   public KvoStream kvoOpenAndFill( double inHigh[], double inLow[], double inClose[], double inVolume[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, double outKVO[], double outKVOSignal[] )
   {
      requireArgument("KVO openAndFill", "inHigh", inHigh);
      requireHistory("KVO openAndFill", inHigh.length);
      requireArgument("KVO openAndFill", "inLow", inLow);
      requireArgument("KVO openAndFill", "inClose", inClose);
      requireArgument("KVO openAndFill", "inVolume", inVolume);
      int guardOutLen = openFillCount("KVO openAndFill", inHigh.length, kvoLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      requireHistoryLength("KVO openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("KVO openAndFill", "inClose", inClose.length, inHigh.length);
      requireHistoryLength("KVO openAndFill", "inVolume", inVolume.length, inHigh.length);
      requireLength("KVO openAndFill", "outKVO", outKVO, guardOutLen);
      requireLength("KVO openAndFill", "outKVOSignal", outKVOSignal, guardOutLen);
      if( (Object)outKVO == (Object)inHigh || (Object)outKVO == (Object)inLow || (Object)outKVO == (Object)inClose || (Object)outKVO == (Object)inVolume || (Object)outKVOSignal == (Object)inHigh || (Object)outKVOSignal == (Object)inLow || (Object)outKVOSignal == (Object)inClose || (Object)outKVOSignal == (Object)inVolume || (Object)outKVO == (Object)outKVOSignal ) {
         throw streamFailure("KVO openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return kvoOpenAndFillInternal(inHigh, inLow, inClose, inVolume, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outKVO, outKVOSignal);
   }
