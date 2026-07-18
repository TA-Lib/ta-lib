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
 *  071626 MF,CC  Initial version.
 */

   public int cmouLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      /* CMOU needs optInTimePeriod price changes -> optInTimePeriod+1 prices ->
       * the first output is at index optInTimePeriod.
       *
       * Unlike the shipped CMO, there is NO unstable period and NO Metastock
       * "extra initial bar" adjustment: CMOU is a plain moving-window sum, so its
       * lookback is exactly the period.
       */
      return optInTimePeriod ;

   }
   public RetCode cmou( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
       * QuantConnect, pandas-ta default). Over the trailing optInTimePeriod changes
       * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
       * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
       * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
       * smoothing -- hence no unstable period.
       *
       * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
       * precedes this iteration's write (trailingIdx >= outIdx), and the oldest
       * change's older endpoint comes from the `trailingValue` cache, not a re-read.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      /* Accumulate the up/down sums over the first window: the optInTimePeriod
       * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
       * startIdx]). `trailingValue` caches the oldest price so the window's oldest
       * change can later be dropped by reading only the newer of its two prices.
       * `trailingIdx` points AT that newer price (one past the cached one).
       */
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
       * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
       * with TA_IS_ZERO (as TA_CMO does for its own gain+loss) and emit 0.0.
       *
       * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
       * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
       * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
       * which both scale before dividing. The two orders differ by <=1 ULP.
       */
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      /* Slide the window forward one bar at a time. */
      today += 1;
      while( today <= endIdx ) {
         /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
          * inReal[trailingIdx-1] comes from the cache (already overwritten when
          * outReal == inReal); inReal[trailingIdx] is read here, before this
          * iteration writes outReal[outIdx], so it is still the original price.
          */
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         /* Add the newest change: inReal[today] - inReal[today-1]. */
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode cmouUnguarded( int startIdx,
                                 int endIdx,
                                 double inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode cmou( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = (double)inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   public RetCode cmouUnguarded( int startIdx,
                                 int endIdx,
                                 float inReal[],
                                 int optInTimePeriod,
                                 MInteger outBegIdx,
                                 MInteger outNBElement,
                                 double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = (double)inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      today += 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         tempReal = (double)inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
/**** Streaming API *****/

   /**
    * A live CMOU stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cmou} over the same series.
    * Open with {@link Core#cmouOpen}; there is no close — the handle is
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
   public static final class CmouStream {
      final Core core;
      int optInTimePeriod;
      double upSum;
      double downSum;
      double sum;
      double prevValue;
      double trailingValue;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;

      CmouStream( Core core ) { this.core = core; }

      CmouStream( CmouStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.upSum = other.upSum;
         this.downSum = other.downSum;
         this.sum = other.sum;
         this.prevValue = other.prevValue;
         this.trailingValue = other.trailingValue;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.cmouStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         CmouStream scratch = new CmouStream(this);
         core.cmouStreamStep(scratch, inReal);
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
      public CmouStream copy() {
         return new CmouStream(this);
      }
   }
   void cmouStreamStep( CmouStream sp, double inReal )
   {
      double diff = 0.0;
      double tempReal = 0.0;
      if( sp.ringCap_trailingIdx == 0 ) {
         sp.ring_trailingIdx_inReal[0] = inReal;
      }
      /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
       * inReal[trailingIdx-1] comes from the cache (already overwritten when
       * outReal == inReal); inReal[trailingIdx] is read here, before this
       * iteration writes outReal[outIdx], so it is still the original price.
       */
      tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
      diff = tempReal - sp.trailingValue;
      sp.trailingValue = tempReal;
      if( diff > 0.0 ) {
         sp.upSum -= diff;
      } else if( diff < 0.0 ) {
         sp.downSum += diff;
      }
      /* Add the newest change: inReal[today] - inReal[today-1]. */
      tempReal = inReal;
      diff = tempReal - sp.prevValue;
      sp.prevValue = tempReal;
      if( diff > 0.0 ) {
         sp.upSum += diff;
      } else if( diff < 0.0 ) {
         sp.downSum -= diff;
      }
      sp.sum = sp.upSum + sp.downSum;
      if( !((-0.00000000000001 < sp.sum) && (sp.sum < 0.00000000000001)) ) {
         sp.cur_outReal = 100.0 * (sp.upSum - sp.downSum) / sp.sum;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode cmouOpenBody( CmouStream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
       * QuantConnect, pandas-ta default). Over the trailing optInTimePeriod changes
       * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
       * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
       * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
       * smoothing -- hence no unstable period.
       *
       * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
       * precedes this iteration's write (trailingIdx >= outIdx), and the oldest
       * change's older endpoint comes from the `trailingValue` cache, not a re-read.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Accumulate the up/down sums over the first window: the optInTimePeriod
       * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
       * startIdx]). `trailingValue` caches the oldest price so the window's oldest
       * change can later be dropped by reading only the newer of its two prices.
       * `trailingIdx` points AT that newer price (one past the cached one).
       */
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
       * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
       * with TA_IS_ZERO (as TA_CMO does for its own gain+loss) and emit 0.0.
       *
       * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
       * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
       * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
       * which both scale before dividing. The two orders differ by <=1 ULP.
       */
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         lastValue_outReal = 100.0 * (upSum - downSum) / sum;
      } else {
         lastValue_outReal = 0.0;
      }
      /* Slide the window forward one bar at a time. */
      today += 1;
      while( today <= endIdx ) {
         /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
          * inReal[trailingIdx-1] comes from the cache (already overwritten when
          * outReal == inReal); inReal[trailingIdx] is read here, before this
          * iteration writes outReal[outIdx], so it is still the original price.
          */
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         /* Add the newest change: inReal[today] - inReal[today-1]. */
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            lastValue_outReal = 100.0 * (upSum - downSum) / sum;
         } else {
            lastValue_outReal = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.upSum = upSum;
      sp.downSum = downSum;
      sp.sum = sum;
      sp.prevValue = prevValue;
      sp.trailingValue = trailingValue;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode cmouOpenAndFillBody( CmouStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      /* CMOU -- unsmoothed Chande Momentum Oscillator (as in TradingView ta.cmo,
       * QuantConnect, pandas-ta default). Over the trailing optInTimePeriod changes
       * d = inReal[i]-inReal[i-1]: Su = sum of up-moves (d>0), Sd = sum of
       * |down-moves| (d<0); CMOU = 100*(Su-Sd)/(Su+Sd), 0 for a flat window. A plain
       * moving-window sum (drop oldest change, add newest), NOT TA_CMO's Wilder
       * smoothing -- hence no unstable period.
       *
       * In-place safe (outReal == inReal): the trailing read inReal[trailingIdx]
       * precedes this iteration's write (trailingIdx >= outIdx), and the oldest
       * change's older endpoint comes from the `trailingValue` cache, not a re-read.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cmouLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Accumulate the up/down sums over the first window: the optInTimePeriod
       * changes ending at startIdx (prices inReal[startIdx-optInTimePeriod ..
       * startIdx]). `trailingValue` caches the oldest price so the window's oldest
       * change can later be dropped by reading only the newer of its two prices.
       * `trailingIdx` points AT that newer price (one past the cached one).
       */
      today = startIdx - lookbackTotal;
      trailingIdx = today + 1;
      prevValue = inReal[today];
      trailingValue = prevValue;
      upSum = 0.0;
      downSum = 0.0;
      for( i = 0; i < optInTimePeriod; i += 1 ) {
         today += 1;
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
      }
      /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
       * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
       * with TA_IS_ZERO (as TA_CMO does for its own gain+loss) and emit 0.0.
       *
       * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
       * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
       * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
       * which both scale before dividing. The two orders differ by <=1 ULP.
       */
      outIdx = 0;
      sum = upSum + downSum;
      if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
         outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++] = 0.0;
      }
      /* Slide the window forward one bar at a time. */
      today += 1;
      while( today <= endIdx ) {
         /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
          * inReal[trailingIdx-1] comes from the cache (already overwritten when
          * outReal == inReal); inReal[trailingIdx] is read here, before this
          * iteration writes outReal[outIdx], so it is still the original price.
          */
         tempReal = inReal[trailingIdx];
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         trailingIdx += 1;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         /* Add the newest change: inReal[today] - inReal[today-1]. */
         tempReal = inReal[today];
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         sum = upSum + downSum;
         if( !((-0.00000000000001 < sum) && (sum < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++] = 0.0;
         }
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_trailingIdx = today - trailingIdx;
      if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
         return RetCode.InternalError;
      }
      int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
      double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
      System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
      sp.optInTimePeriod = optInTimePeriod;
      sp.upSum = upSum;
      sp.downSum = downSum;
      sp.sum = sum;
      sp.prevValue = prevValue;
      sp.trailingValue = trailingValue;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind cmouOpen (composition seam). */
   CmouStream cmouOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      CmouStream sp = new CmouStream(this);
      RetCode retCode = cmouOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CMOU open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CMOU open: internal error");
      }
      throw new IllegalArgumentException("TA_CMOU open: " + retCode);
   }
   /**
    * Open a live CMOU stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cmou} at that bar.
    * <p>The history must hold at least {@code cmouLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CmouStream cmouOpen( double inReal[], int optInTimePeriod )
   {
      return cmouOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cmouOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cmou} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public CmouStream cmouOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CmouStream sp = new CmouStream(this);
      RetCode retCode = cmouOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_CMOU openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_CMOU openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_CMOU openAndFill: " + retCode);
   }
