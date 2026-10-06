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
 *  100626 KL,CC  Creation (#485).
 *  100626 MF,CC  Batch tier: block scan of the channel (#485).
 */

/* Using fisher_ALT1 for TA_ALT={BATCH,JAVA} */

   /**
    * Number of leading input bars {@link Core#fisher} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Number of bars in the channel the midpoint is
    *        located in (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int fisherLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* The seed reaches the output through the smoothing pole alone, 0.67 per
       * bar whatever the period: 2.5 bars per e-fold. The 9 e-folds added to K
       * are the gain between that pole and the output: the transform's
       * derivative, which the clamp caps at 500, times 5.9 for the convolution
       * with its own 0.5 pole.
       */
      return optInTimePeriod - 1 + this.unstableCount(FuncUnstId.FISHER.ordinal(), (5 * (10 + 9) + 1) / 2, (5 * (19 + 9) + 1) / 2) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#fisher}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of bars in the channel the midpoint is
    *        located in (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int fisherDisplayShift( int optInTimePeriod, int outputIdx )
   {
      if( fisherLookback( optInTimePeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode fisherImpl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outFisher[],
                       double outTrigger[] )
   {
      double price = 0;
      double highest = 0;
      double lowest = 0;
      double ratio = 0;
      double smoothed = 0;
      double fish = 0;
      double prevFish = 0;
      double tempReal = 0;
      double tempHigh = 0;
      double tempLow = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int unstablePeriod = 0;
      int nbInitialElementNeeded = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outFisher == outTrigger ) {
         return RetCode.BAD_PARAM ;
      }
      nbInitialElementNeeded = fisherLookback(optInTimePeriod);
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      lookbackTotal = optInTimePeriod - 1;
      unstablePeriod = nbInitialElementNeeded - lookbackTotal;
      /* John F. Ehlers, "Using The Fisher Transform", Stocks & Commodities
       * V.20:11 (November 2002), pp.40-42, the EasyLanguage listing in Figure 4.
       *
       * The bar's midpoint is located in its rolling n-bar channel, rescaled to
       * (-1, +1), smoothed, clamped, and passed through atanh. What the
       * transform buys is the tail: a channel position is close to uniformly
       * distributed, and the Fisher transform of a uniform variable is close to
       * normal, so an extreme reading is rare rather than routine and a turn is
       * a sharp corner rather than a drift.
       *
       * Both recursions start from the author's zero seed and decay at their own
       * coefficient -- 0.67 for the smoothing, 0.5 for the transform -- so the
       * first bars carry the seed rather than the series. That is what the
       * unstable period discards.
       */
      smoothed = 0.0;
      prevFish = 0.0;
      today = startIdx - unstablePeriod;
      trailingIdx = today - lookbackTotal;
      highestIdx = -1;
      highest = 0.0;
      lowestIdx = -1;
      lowest = 0.0;
      outIdx = 0;
      while( today <= endIdx ) {
         /* The channel, over the midpoints rather than over the highs and the
          * lows separately: this indicator reads one series, which happens to be
          * (H+L)/2, so both extremes come from that same series. STOCH's shape,
          * with the midpoint recomputed on the rare rescan rather than held in a
          * buffer the caller would have to own.
          */
         price = (inHigh[today] + inLow[today]) / 2.0;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            tempHigh = inHigh[highestIdx];
            tempLow = inLow[highestIdx];
            highest = (tempHigh + tempLow) / 2.0;
            i = highestIdx;
            while( ++i <= today ) {
               tempHigh = inHigh[i];
               tempLow = inLow[i];
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal > highest ) {
                  highestIdx = i;
                  highest = tempReal;
               }
            }
         } else if( price >= highest ) {
            highestIdx = today;
            highest = price;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            tempHigh = inHigh[lowestIdx];
            tempLow = inLow[lowestIdx];
            lowest = (tempHigh + tempLow) / 2.0;
            i = lowestIdx;
            while( ++i <= today ) {
               tempHigh = inHigh[i];
               tempLow = inLow[i];
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal < lowest ) {
                  lowestIdx = i;
                  lowest = tempReal;
               }
            }
         } else if( price <= lowest ) {
            lowestIdx = today;
            lowest = price;
         }
         /* A flat window answers the neutral position rather than dividing by
          * its own zero range. The band is the range against its own two
          * extremes, STOCH's test: a fixed constant answers "flat" for every
          * window of an instrument quoted below it (#253), and an exact test
          * divides a machine-flat window into noise (#107). At 0.5 the bar
          * contributes nothing and both recursions decay on their own
          * coefficients.
          */
         tempReal = highest - lowest;
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            ratio = (price - lowest) / tempReal;
         } else {
            ratio = 0.5;
         }
         /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
         smoothed = Math.fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
         /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
          * the next bar's smoothing reads -- the listing assigns it back to
          * Value1 rather than holding it for the transform alone.
          */
         if( smoothed > 0.99 ) {
            smoothed = 0.999;
         }
         if( smoothed < -0.99 ) {
            smoothed = -0.999;
         }
         fish = Math.fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * prevFish);
         if( today >= startIdx ) {
            outFisher[outIdx] = fish;
            /* The author's second plot is Fish[1]: the previous bar's smoothed,
             * which at the first output bar is the zero seed when no unstable
             * period was discarded, and the computed smoothed of the bar before it
             * when one was.
             */
            outTrigger[outIdx] = prevFish;
            outIdx = outIdx + 1;
         }
         prevFish = fish;
         trailingIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode fisherImpl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outFisher[],
                       double outTrigger[] )
   {
      double price = 0;
      double highest = 0;
      double lowest = 0;
      double ratio = 0;
      double smoothed = 0;
      double fish = 0;
      double prevFish = 0;
      double tempReal = 0;
      double tempHigh = 0;
      double tempLow = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int unstablePeriod = 0;
      int nbInitialElementNeeded = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outFisher == outTrigger ) {
         return RetCode.BAD_PARAM ;
      }
      nbInitialElementNeeded = fisherLookback(optInTimePeriod);
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      lookbackTotal = optInTimePeriod - 1;
      unstablePeriod = nbInitialElementNeeded - lookbackTotal;
      smoothed = 0.0;
      prevFish = 0.0;
      today = startIdx - unstablePeriod;
      trailingIdx = today - lookbackTotal;
      highestIdx = -1;
      highest = 0.0;
      lowestIdx = -1;
      lowest = 0.0;
      outIdx = 0;
      while( today <= endIdx ) {
         price = ((double)inHigh[today] + (double)inLow[today]) / 2.0;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            tempHigh = (double)inHigh[highestIdx];
            tempLow = (double)inLow[highestIdx];
            highest = (tempHigh + tempLow) / 2.0;
            i = highestIdx;
            while( ++i <= today ) {
               tempHigh = (double)inHigh[i];
               tempLow = (double)inLow[i];
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal > highest ) {
                  highestIdx = i;
                  highest = tempReal;
               }
            }
         } else if( price >= highest ) {
            highestIdx = today;
            highest = price;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            tempHigh = (double)inHigh[lowestIdx];
            tempLow = (double)inLow[lowestIdx];
            lowest = (tempHigh + tempLow) / 2.0;
            i = lowestIdx;
            while( ++i <= today ) {
               tempHigh = (double)inHigh[i];
               tempLow = (double)inLow[i];
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal < lowest ) {
                  lowestIdx = i;
                  lowest = tempReal;
               }
            }
         } else if( price <= lowest ) {
            lowestIdx = today;
            lowest = price;
         }
         tempReal = highest - lowest;
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            ratio = (price - lowest) / tempReal;
         } else {
            ratio = 0.5;
         }
         smoothed = Math.fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
         if( smoothed > 0.99 ) {
            smoothed = 0.999;
         }
         if( smoothed < -0.99 ) {
            smoothed = -0.999;
         }
         fish = Math.fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * prevFish);
         if( today >= startIdx ) {
            outFisher[outIdx] = fish;
            outTrigger[outIdx] = prevFish;
            outIdx = outIdx + 1;
         }
         prevFish = fish;
         trailingIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's
    * position in its rolling channel into a near-normal distribution. Extreme
    * values become rare and turning points sharp, where a plain channel
    * position spends much of its time pinned near the edges. The output is
    * unbounded and centred on zero. A peak or trough marks a likely turn, and
    * the Fisher line crossing its Trigger, the same line one bar later, is the
    * author's entry signal.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/fisher">ta-lib.org/functions/fisher</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>A window whose midpoints are all equal takes the neutral position, so a market that does not move decays toward 0. The original divides by the zero range there.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#fisherLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Number of bars in the channel the midpoint is
    *        located in (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outFisher Fisher Transform value. Must hold at least
    *        {@code endIdx - max(startIdx, fisherLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outTrigger Fisher value of the previous bar. Must hold at least
    *        {@code endIdx - max(startIdx, fisherLookback(...)) + 1} values, and never
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
    * @see Core#stochf
    * @see Core#willr
    * @see Core#midprice
    * @see Core#ibs
    */
   public OutRange fisher( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           int optInTimePeriod,
                           double outFisher[],
                           double outTrigger[] )
   {
      requireIndexRange("FISHER", startIdx, endIdx);
      int guardStart = clampedStart("FISHER", startIdx, fisherLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FISHER", "inHigh", inHigh, guardInLen);
      requireLength("FISHER", "inLow", inLow, guardInLen);
      requireLength("FISHER", "outFisher", outFisher, guardOutLen);
      requireLength("FISHER", "outTrigger", outTrigger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = fisherImpl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outFisher, outTrigger);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("FISHER", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's
    * position in its rolling channel into a near-normal distribution. Extreme
    * values become rare and turning points sharp, where a plain channel
    * position spends much of its time pinned near the edges. The output is
    * unbounded and centred on zero. A peak or trough marks a likely turn, and
    * the Fisher line crossing its Trigger, the same line one bar later, is the
    * author's entry signal.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/fisher">ta-lib.org/functions/fisher</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>A window whose midpoints are all equal takes the neutral position, so a market that does not move decays toward 0. The original divides by the zero range there.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#fisherLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Number of bars in the channel the midpoint is
    *        located in (default 10; range 2..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param outFisher Fisher Transform value. Must hold at least
    *        {@code endIdx - max(startIdx, fisherLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outTrigger Fisher value of the previous bar. Must hold at least
    *        {@code endIdx - max(startIdx, fisherLookback(...)) + 1} values, and never
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
    * @see Core#stochf
    * @see Core#willr
    * @see Core#midprice
    * @see Core#ibs
    */
   public OutRange fisher( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           int optInTimePeriod,
                           double outFisher[],
                           double outTrigger[] )
   {
      requireIndexRange("FISHER", startIdx, endIdx);
      int guardStart = clampedStart("FISHER", startIdx, fisherLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("FISHER", "inHigh", inHigh, guardInLen);
      requireLength("FISHER", "inLow", inLow, guardInLen);
      requireLength("FISHER", "outFisher", outFisher, guardOutLen);
      requireLength("FISHER", "outTrigger", outTrigger, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = fisherImpl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outFisher, outTrigger);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("FISHER", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using fisher_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live FISHER stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#fisher} over the same series.
    * Open with {@link Core#fisherOpen}; there is no close — the handle is
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
   public static final class FisherStream {
      private Core core;
      private int optInTimePeriod;
      private double highest;
      private double lowest;
      private double smoothed;
      private double prevFish;
      private int trailingIdx;
      private int highestIdx;
      private int lowestIdx;
      private int i;
      private int today;
      private int xMask;
      private double[] x_inHigh;
      private double[] x_inLow;
      private double cur_outFisher;
      private double cur_outTrigger;
      private int outRangeBegIdx;
      private int outRangeCount;

      private FisherStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#fisher} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(FisherOut)} keeps answering the previous
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
            throw failure("FISHER advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private FisherStream( FisherStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.highest = other.highest;
         this.lowest = other.lowest;
         this.smoothed = other.smoothed;
         this.prevFish = other.prevFish;
         this.trailingIdx = other.trailingIdx;
         this.highestIdx = other.highestIdx;
         this.lowestIdx = other.lowestIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.cur_outFisher = other.cur_outFisher;
         this.cur_outTrigger = other.cur_outTrigger;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(FisherOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, FisherOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("FISHER update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("FISHER update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("FISHER update", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         core.fisherStepImpl(this, inHigh, inLow);
         this.outRangeCount++;
         out.fisher = this.cur_outFisher;
         out.trigger = this.cur_outTrigger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, FisherOut out ) {
         requireArgument("FISHER peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw nonFiniteBar("FISHER peek", !Double.isFinite(inHigh) ? "inHigh" : "inLow");
         FisherStream sp = this;
         double price = 0.0;
         double ratio = 0.0;
         double fish = 0.0;
         double tempReal = 0.0;
         double tempHigh = 0.0;
         double tempLow = 0.0;
         double cur_outFisher = 0.0;
         double cur_outTrigger = 0.0;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         double smoothed = sp.smoothed;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         pkSlot0 = sp.today & sp.xMask;
         pkVal0 = inHigh;
         pkSlot1 = sp.today & sp.xMask;
         pkVal1 = inLow;
         /* The channel, over the midpoints rather than over the highs and the
          * lows separately: this indicator reads one series, which happens to be
          * (H+L)/2, so both extremes come from that same series. STOCH's shape,
          * with the midpoint recomputed on the rare rescan rather than held in a
          * buffer the caller would have to own.
          */
         price = ((((sp.today & sp.xMask) != pkSlot0) ? sp.x_inHigh[sp.today & sp.xMask] : pkVal0) + (((sp.today & sp.xMask) != pkSlot1) ? sp.x_inLow[sp.today & sp.xMask] : pkVal1)) / 2.0;
         if( highestIdx < sp.trailingIdx ) {
            highestIdx = sp.trailingIdx;
            tempHigh = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
            tempLow = ((highestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[highestIdx & sp.xMask] : pkVal1;
            highest = (tempHigh + tempLow) / 2.0;
            i = highestIdx;
            while( ++i <= sp.today ) {
               tempHigh = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
               tempLow = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal > highest ) {
                  highestIdx = i;
                  highest = tempReal;
               }
            }
         } else if( price >= highest ) {
            highestIdx = sp.today;
            highest = price;
         }
         if( lowestIdx < sp.trailingIdx ) {
            lowestIdx = sp.trailingIdx;
            tempHigh = ((lowestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[lowestIdx & sp.xMask] : pkVal0;
            tempLow = ((lowestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
            lowest = (tempHigh + tempLow) / 2.0;
            i = lowestIdx;
            while( ++i <= sp.today ) {
               tempHigh = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
               tempLow = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal < lowest ) {
                  lowestIdx = i;
                  lowest = tempReal;
               }
            }
         } else if( price <= lowest ) {
            lowestIdx = sp.today;
            lowest = price;
         }
         /* A flat window answers the neutral position rather than dividing by
          * its own zero range. The band is the range against its own two
          * extremes, STOCH's test: a fixed constant answers "flat" for every
          * window of an instrument quoted below it (#253), and an exact test
          * divides a machine-flat window into noise (#107). At 0.5 the bar
          * contributes nothing and both recursions decay on their own
          * coefficients.
          */
         tempReal = highest - lowest;
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            ratio = (price - lowest) / tempReal;
         } else {
            ratio = 0.5;
         }
         /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
         smoothed = Math.fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
         /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
          * the next bar's smoothing reads -- the listing assigns it back to
          * Value1 rather than holding it for the transform alone.
          */
         if( smoothed > 0.99 ) {
            smoothed = 0.999;
         }
         if( smoothed < -0.99 ) {
            smoothed = -0.999;
         }
         fish = Math.fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * sp.prevFish);
         cur_outFisher = fish;
         /* The author's second plot is Fish[1]: the previous bar's smoothed,
          * which at the first output bar is the zero seed when no unstable
          * period was discarded, and the computed smoothed of the bar before it
          * when one was.
          */
         cur_outTrigger = sp.prevFish;
         out.fisher = cur_outFisher;
         out.trigger = cur_outTrigger;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( FisherOut out ) {
         requireArgument("FISHER value", "out", out);
         out.fisher = this.cur_outFisher;
         out.trigger = this.cur_outTrigger;
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
      public FisherStream clone() {
         return new FisherStream(this);
      }
   }

   /**
    * The outputs of one FISHER bar, written by the stream into an object the
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
   public static final class FisherOut {
      /** Fisher Transform value. */
      public double fisher;
      /** Fisher value of the previous bar. */
      public double trigger;
   }
   private void fisherStepImpl( FisherStream sp, double inHigh, double inLow )
   {
      double price = 0.0;
      double ratio = 0.0;
      double fish = 0.0;
      double tempReal = 0.0;
      double tempHigh = 0.0;
      double tempLow = 0.0;
      sp.x_inHigh[sp.today & sp.xMask] = inHigh;
      sp.x_inLow[sp.today & sp.xMask] = inLow;
      /* The channel, over the midpoints rather than over the highs and the
       * lows separately: this indicator reads one series, which happens to be
       * (H+L)/2, so both extremes come from that same series. STOCH's shape,
       * with the midpoint recomputed on the rare rescan rather than held in a
       * buffer the caller would have to own.
       */
      price = (sp.x_inHigh[sp.today & sp.xMask] + sp.x_inLow[sp.today & sp.xMask]) / 2.0;
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         tempHigh = sp.x_inHigh[sp.highestIdx & sp.xMask];
         tempLow = sp.x_inLow[sp.highestIdx & sp.xMask];
         sp.highest = (tempHigh + tempLow) / 2.0;
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tempHigh = sp.x_inHigh[sp.i & sp.xMask];
            tempLow = sp.x_inLow[sp.i & sp.xMask];
            tempReal = (tempHigh + tempLow) / 2.0;
            if( tempReal > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tempReal;
            }
         }
      } else if( price >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = price;
      }
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         tempHigh = sp.x_inHigh[sp.lowestIdx & sp.xMask];
         tempLow = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.lowest = (tempHigh + tempLow) / 2.0;
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tempHigh = sp.x_inHigh[sp.i & sp.xMask];
            tempLow = sp.x_inLow[sp.i & sp.xMask];
            tempReal = (tempHigh + tempLow) / 2.0;
            if( tempReal < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tempReal;
            }
         }
      } else if( price <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = price;
      }
      /* A flat window answers the neutral position rather than dividing by
       * its own zero range. The band is the range against its own two
       * extremes, STOCH's test: a fixed constant answers "flat" for every
       * window of an instrument quoted below it (#253), and an exact test
       * divides a machine-flat window into noise (#107). At 0.5 the bar
       * contributes nothing and both recursions decay on their own
       * coefficients.
       */
      tempReal = sp.highest - sp.lowest;
      if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest))) ) {
         ratio = (price - sp.lowest) / tempReal;
      } else {
         ratio = 0.5;
      }
      /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
      sp.smoothed = Math.fma(0.67, sp.smoothed, 0.33 * 2.0 * (ratio - 0.5));
      /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
       * the next bar's smoothing reads -- the listing assigns it back to
       * Value1 rather than holding it for the transform alone.
       */
      if( sp.smoothed > 0.99 ) {
         sp.smoothed = 0.999;
      }
      if( sp.smoothed < -0.99 ) {
         sp.smoothed = -0.999;
      }
      fish = Math.fma(0.5, Math.log((1.0 + sp.smoothed) / (1.0 - sp.smoothed)), 0.5 * sp.prevFish);
      sp.cur_outFisher = fish;
      /* The author's second plot is Fish[1]: the previous bar's smoothed,
       * which at the first output bar is the zero seed when no unstable
       * period was discarded, and the computed smoothed of the bar before it
       * when one was.
       */
      sp.cur_outTrigger = sp.prevFish;
      sp.prevFish = fish;
      sp.trailingIdx += 1;
      sp.today += 1;
   }
   private RetCode fisherOpenImpl( FisherStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outFisher[], double outTrigger[], int outStride )
   {
      double price = 0;
      double highest = 0;
      double lowest = 0;
      double ratio = 0;
      double smoothed = 0;
      double fish = 0;
      double prevFish = 0;
      double tempReal = 0;
      double tempHigh = 0;
      double tempLow = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int unstablePeriod = 0;
      int nbInitialElementNeeded = 0;
      int today = 0;
      int trailingIdx = 0;
      int highestIdx = 0;
      int lowestIdx = 0;
      int i = 0;
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
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      nbInitialElementNeeded = fisherLookback(optInTimePeriod);
      if( startIdx < nbInitialElementNeeded ) {
         startIdx = nbInitialElementNeeded;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      lookbackTotal = optInTimePeriod - 1;
      unstablePeriod = nbInitialElementNeeded - lookbackTotal;
      /* John F. Ehlers, "Using The Fisher Transform", Stocks & Commodities
       * V.20:11 (November 2002), pp.40-42, the EasyLanguage listing in Figure 4.
       *
       * The bar's midpoint is located in its rolling n-bar channel, rescaled to
       * (-1, +1), smoothed, clamped, and passed through atanh. What the
       * transform buys is the tail: a channel position is close to uniformly
       * distributed, and the Fisher transform of a uniform variable is close to
       * normal, so an extreme reading is rare rather than routine and a turn is
       * a sharp corner rather than a drift.
       *
       * Both recursions start from the author's zero seed and decay at their own
       * coefficient -- 0.67 for the smoothing, 0.5 for the transform -- so the
       * first bars carry the seed rather than the series. That is what the
       * unstable period discards.
       */
      smoothed = 0.0;
      prevFish = 0.0;
      today = startIdx - unstablePeriod;
      trailingIdx = today - lookbackTotal;
      highestIdx = -1;
      highest = 0.0;
      lowestIdx = -1;
      lowest = 0.0;
      outIdx = 0;
      while( today <= endIdx ) {
         /* The channel, over the midpoints rather than over the highs and the
          * lows separately: this indicator reads one series, which happens to be
          * (H+L)/2, so both extremes come from that same series. STOCH's shape,
          * with the midpoint recomputed on the rare rescan rather than held in a
          * buffer the caller would have to own.
          */
         price = (inHigh[today] + inLow[today]) / 2.0;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            tempHigh = inHigh[highestIdx];
            tempLow = inLow[highestIdx];
            highest = (tempHigh + tempLow) / 2.0;
            i = highestIdx;
            while( ++i <= today ) {
               tempHigh = inHigh[i];
               tempLow = inLow[i];
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal > highest ) {
                  highestIdx = i;
                  highest = tempReal;
               }
            }
         } else if( price >= highest ) {
            highestIdx = today;
            highest = price;
         }
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            tempHigh = inHigh[lowestIdx];
            tempLow = inLow[lowestIdx];
            lowest = (tempHigh + tempLow) / 2.0;
            i = lowestIdx;
            while( ++i <= today ) {
               tempHigh = inHigh[i];
               tempLow = inLow[i];
               tempReal = (tempHigh + tempLow) / 2.0;
               if( tempReal < lowest ) {
                  lowestIdx = i;
                  lowest = tempReal;
               }
            }
         } else if( price <= lowest ) {
            lowestIdx = today;
            lowest = price;
         }
         /* A flat window answers the neutral position rather than dividing by
          * its own zero range. The band is the range against its own two
          * extremes, STOCH's test: a fixed constant answers "flat" for every
          * window of an instrument quoted below it (#253), and an exact test
          * divides a machine-flat window into noise (#107). At 0.5 the bar
          * contributes nothing and both recursions decay on their own
          * coefficients.
          */
         tempReal = highest - lowest;
         if( !(Math.abs(tempReal) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            ratio = (price - lowest) / tempReal;
         } else {
            ratio = 0.5;
         }
         /* The listing's .33*2*(r-.5) + .67*Value1[1]. */
         smoothed = Math.fma(0.67, smoothed, 0.33 * 2.0 * (ratio - 0.5));
         /* The clamp is what keeps atanh finite, and the CLAMPED smoothed is what
          * the next bar's smoothing reads -- the listing assigns it back to
          * Value1 rather than holding it for the transform alone.
          */
         if( smoothed > 0.99 ) {
            smoothed = 0.999;
         }
         if( smoothed < -0.99 ) {
            smoothed = -0.999;
         }
         fish = Math.fma(0.5, Math.log((1.0 + smoothed) / (1.0 - smoothed)), 0.5 * prevFish);
         if( today >= startIdx ) {
            outFisher[outIdx * outStride] = fish;
            /* The author's second plot is Fish[1]: the previous bar's smoothed,
             * which at the first output bar is the zero seed when no unstable
             * period was discarded, and the computed smoothed of the bar before it
             * when one was.
             */
            outTrigger[outIdx * outStride] = prevFish;
            outIdx = outIdx + 1;
         }
         prevFish = fish;
         trailingIdx += 1;
         today += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.INTERNAL_ERROR;
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
      sp.highest = highest;
      sp.lowest = lowest;
      sp.smoothed = smoothed;
      sp.prevFish = prevFish;
      sp.trailingIdx = trailingIdx;
      sp.highestIdx = highestIdx;
      sp.lowestIdx = lowestIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.cur_outFisher = outFisher[(outNBElement.value - 1) * outStride];
      sp.cur_outTrigger = outTrigger[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* fisherOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   FisherStream fisherOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outFisher[], double outTrigger[] )
   {
      FisherStream sp = new FisherStream(this);
      RetCode retCode = fisherOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outFisher, outTrigger, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("FISHER openAndFill", inHigh.length, startIdx, fisherLookback(optInTimePeriod));
      }
      throw streamFailure("FISHER openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind fisherOpen (composition seam). */
   FisherStream fisherOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      FisherStream sp = new FisherStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outFisher = new double[1];
      double[] sink_outTrigger = new double[1];
      RetCode retCode = fisherOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outFisher, sink_outTrigger, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("FISHER open", inHigh.length, startIdx, fisherLookback(optInTimePeriod));
      }
      throw streamFailure("FISHER open", retCode);
   }
   /**
    * Open a live FISHER stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#fisher} at that bar.
    * <p>The history must hold at least {@code fisherLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public FisherStream fisherOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("FISHER open", "inHigh", inHigh);
      requireHistory("FISHER open", inHigh.length);
      requireArgument("FISHER open", "inLow", inLow);
      requireHistoryLength("FISHER open", "inLow", inLow.length, inHigh.length);
      return fisherOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#fisherOpen} that also fills the output array(s) bit-identically
    * to {@link Core#fisher} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link FisherStream#outRange()}.
    */
   public FisherStream fisherOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outFisher[], double outTrigger[] )
   {
      requireArgument("FISHER openAndFill", "inHigh", inHigh);
      requireHistory("FISHER openAndFill", inHigh.length);
      requireArgument("FISHER openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("FISHER openAndFill", inHigh.length, fisherLookback(optInTimePeriod));
      requireHistoryLength("FISHER openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("FISHER openAndFill", "outFisher", outFisher, guardOutLen);
      requireLength("FISHER openAndFill", "outTrigger", outTrigger, guardOutLen);
      if( (Object)outFisher == (Object)inHigh || (Object)outFisher == (Object)inLow || (Object)outTrigger == (Object)inHigh || (Object)outTrigger == (Object)inLow || (Object)outFisher == (Object)outTrigger ) {
         throw streamFailure("FISHER openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return fisherOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outFisher, outTrigger);
   }
