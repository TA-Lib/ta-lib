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
 *  071626 MF,CC  Initial version (#124).
 *  082326 MF,CC  Fix #253. Recognize a flat window by counting bars, so the
 *                0/0 is guarded exactly instead of against the fixed
 *                TA_IS_ZERO band -- which zeroed the oscillator for any
 *                instrument quoted small enough to fall under it.
 */

   /**
    * Number of leading input bars {@link Core#CMOU} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of trailing price changes summed (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CMOU_Lookback( int optInTimePeriod )
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
   RetCode CMOU_Impl( int startIdx,
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
      int nullRun = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
      lookbackTotal = CMOU_Lookback(optInTimePeriod);
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
      /* Consecutive changes of exactly zero, counted so that an empty window can
       * be recognized exactly (the shape #244 needed for MFI). The sums cannot
       * answer that question themselves once the window starts sliding: they are
       * maintained by add-then-subtract, so an emptied window leaves them holding
       * rounding residue of arbitrary sign rather than zero.
       */
      nullRun = 0;
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
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
       * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
       * exactly and emit 0.0. Not against a fixed band: a price change carries the
       * quote unit, so a constant put against the total zeroed the oscillator for
       * any instrument quoted below it (issue #253).
       *
       * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
       * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
       * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
       * which both scale before dividing. The two orders differ by <=1 ULP.
       */
      outIdx = 0;
      sum = upSum + downSum;
      if( sum > 0.0 ) {
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
         /* Once a whole period of flat bars has gone by, every change in the
          * window is exactly zero, so both sums are known to be exactly zero and
          * the residue can be dropped.
          */
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
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
   RetCode CMOU_Impl( int startIdx,
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
      int nullRun = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = CMOU_Lookback(optInTimePeriod);
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
      nullRun = 0;
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
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      outIdx = 0;
      sum = upSum + downSum;
      if( sum > 0.0 ) {
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
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
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
   /**
    * Chande Momentum Oscillator: Tushar Chande's original momentum oscillator,
    * computed from **plain moving-window sums** of the up-moves and down-moves
    * over the period. Bounded in [-100,+100]; positive = net upward momentum,
    * negative = net downward. CMOU is the version as defined by Chande in his
    * book *The New Technical Trader* (1994), and is the more common
    * implementation used by TradingView ({@code ta.cmo}), QuantConnect and
    * pandas-ta's default. See [{@code CMO}](/functions/cmo) for a smoothed
    * variant of CMOU.
    * <p><b>Formula</b>
    * <pre>{@code
    * d = P[t]-P[t-1]; over the trailing `optInTimePeriod` changes accumulate Su = sum of the positive d, Sd = sum of -d for negative d. CMOU = 100 * (Su-Sd)/(Su+Sd); 0 when Su+Sd == 0 (an exactly flat window). Unlike CMO, the sums are the plain period totals (a moving-window sum), not Wilder-smoothed averages, so there is no unstable period.
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CMOU_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Number of trailing price changes summed (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal CMOU oscillator value. Must hold at least
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
    * @see Core#CMO
    * @see Core#RSI
    */
   public OutRange CMOU( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("CMOU", startIdx, endIdx);
      int guardStart = clampedStart("CMOU", startIdx, CMOU_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CMOU", "inReal", inReal, guardInLen);
      requireLength("CMOU", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMOU_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMOU", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chande Momentum Oscillator: Tushar Chande's original momentum oscillator,
    * computed from **plain moving-window sums** of the up-moves and down-moves
    * over the period. Bounded in [-100,+100]; positive = net upward momentum,
    * negative = net downward. CMOU is the version as defined by Chande in his
    * book *The New Technical Trader* (1994), and is the more common
    * implementation used by TradingView ({@code ta.cmo}), QuantConnect and
    * pandas-ta's default. See [{@code CMO}](/functions/cmo) for a smoothed
    * variant of CMOU.
    * <p><b>Formula</b>
    * <pre>{@code
    * d = P[t]-P[t-1]; over the trailing `optInTimePeriod` changes accumulate Su = sum of the positive d, Sd = sum of -d for negative d. CMOU = 100 * (Su-Sd)/(Su+Sd); 0 when Su+Sd == 0 (an exactly flat window). Unlike CMO, the sums are the plain period totals (a moving-window sum), not Wilder-smoothed averages, so there is no unstable period.
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CMOU_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Number of trailing price changes summed (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal CMOU oscillator value. Must hold at least
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
    * @see Core#CMO
    * @see Core#RSI
    */
   public OutRange CMOU( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double outReal[] )
   {
      requireIndexRange("CMOU", startIdx, endIdx);
      int guardStart = clampedStart("CMOU", startIdx, CMOU_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CMOU", "inReal", inReal, guardInLen);
      requireLength("CMOU", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMOU_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMOU", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CMOU stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CMOU} over the same series.
    * Open with {@link Core#cmouOpen}; there is no close — the handle is
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
   public static final class CmouStream {
      Core core;
      int optInTimePeriod;
      int nullRun;
      double upSum;
      double downSum;
      double prevValue;
      double trailingValue;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      CmouStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#CMOU} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      CmouStream( CmouStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.nullRun = other.nullRun;
         this.upSum = other.upSum;
         this.downSum = other.downSum;
         this.prevValue = other.prevValue;
         this.trailingValue = other.trailingValue;
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("CMOU update: BadParam", RetCode.BadParam);
         }
         core.cmouStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("CMOU updateAndFill", "inReal", inReal);
         requireArgument("CMOU updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("CMOU updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("CMOU updateAndFill: BadParam", RetCode.BadParam);
            }
            core.cmouStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("CMOU peek: BadParam", RetCode.BadParam);
         CmouStream sp = this;
         double sum = 0.0;
         double diff = 0.0;
         double tempReal = 0.0;
         double cur_outReal = 0.0;
         double downSum = sp.downSum;
         int nullRun = sp.nullRun;
         double prevValue = sp.prevValue;
         double trailingValue = sp.trailingValue;
         double upSum = sp.upSum;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.ringCap_trailingIdx == 0 ) {
            pkSlot0 = 0;
            pkVal0 = inReal;
         }
         /* Drop the oldest change: inReal[trailingIdx] - inReal[trailingIdx-1].
          * inReal[trailingIdx-1] comes from the cache (already overwritten when
          * outReal == inReal); inReal[trailingIdx] is read here, before this
          * iteration writes outReal[outIdx], so it is still the original price.
          */
         tempReal = (sp.ringPos_trailingIdx != pkSlot0) ? sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] : pkVal0;
         diff = tempReal - trailingValue;
         trailingValue = tempReal;
         if( diff > 0.0 ) {
            upSum -= diff;
         } else if( diff < 0.0 ) {
            downSum += diff;
         }
         /* Add the newest change: inReal[today] - inReal[today-1]. */
         tempReal = inReal;
         diff = tempReal - prevValue;
         prevValue = tempReal;
         if( diff > 0.0 ) {
            upSum += diff;
         } else if( diff < 0.0 ) {
            downSum -= diff;
         }
         /* Once a whole period of flat bars has gone by, every change in the
          * window is exactly zero, so both sums are known to be exactly zero and
          * the residue can be dropped.
          */
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= sp.optInTimePeriod ) {
            nullRun = sp.optInTimePeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            cur_outReal = 100.0 * (upSum - downSum) / sum;
         } else {
            cur_outReal = 0.0;
         }
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
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
      public CmouStream clone() {
         return new CmouStream(this);
      }
   }
   void cmouStepImpl( CmouStream sp, double inReal )
   {
      double sum = 0.0;
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
      /* Once a whole period of flat bars has gone by, every change in the
       * window is exactly zero, so both sums are known to be exactly zero and
       * the residue can be dropped.
       */
      if( diff == 0.0 ) {
         sp.nullRun += 1;
      } else {
         sp.nullRun = 0;
      }
      if( sp.nullRun >= sp.optInTimePeriod ) {
         sp.nullRun = sp.optInTimePeriod;
         sp.upSum = 0.0;
         sp.downSum = 0.0;
      }
      sum = sp.upSum + sp.downSum;
      if( sum > 0.0 ) {
         sp.cur_outReal = 100.0 * (sp.upSum - sp.downSum) / sum;
      } else {
         sp.cur_outReal = 0.0;
      }
      sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
      sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
      if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
         sp.ringPos_trailingIdx = 0;
      }
   }
   private RetCode cmouOpenImpl( CmouStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int today = 0;
      int trailingIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      int nullRun = 0;
      double upSum = 0;
      double downSum = 0;
      double sum = 0;
      double diff = 0;
      double tempReal = 0;
      double prevValue = 0;
      double trailingValue = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
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
      lookbackTotal = CMOU_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
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
      /* Consecutive changes of exactly zero, counted so that an empty window can
       * be recognized exactly (the shape #244 needed for MFI). The sums cannot
       * answer that question themselves once the window starts sliding: they are
       * maintained by add-then-subtract, so an emptied window leaves them holding
       * rounding residue of arbitrary sign rather than zero.
       */
      nullRun = 0;
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
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
      }
      /* Emit the first output (bar startIdx). Su+Sd is a sum of non-negative
       * magnitudes, so it is zero only for an exactly flat window; guard the 0/0
       * exactly and emit 0.0. Not against a fixed band: a price change carries the
       * quote unit, so a constant put against the total zeroed the oscillator for
       * any instrument quoted below it (issue #253).
       *
       * Scale-then-divide -- (100*(Su-Sd))/(Su+Sd), NOT the 100*((Su-Sd)/(Su+Sd))
       * order TA_CMO/RSI use -- so CMOU is BIT-IDENTICAL to the reference unsmoothed
       * CMO of Tulip Indicators (ti_cmo) and pandas-ta-classic (cmo, talib=False),
       * which both scale before dividing. The two orders differ by <=1 ULP.
       */
      outIdx = 0;
      sum = upSum + downSum;
      if( sum > 0.0 ) {
         outReal[outIdx++ * outStride] = 100.0 * (upSum - downSum) / sum;
      } else {
         outReal[outIdx++ * outStride] = 0.0;
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
         /* Once a whole period of flat bars has gone by, every change in the
          * window is exactly zero, so both sums are known to be exactly zero and
          * the residue can be dropped.
          */
         if( diff == 0.0 ) {
            nullRun += 1;
         } else {
            nullRun = 0;
         }
         if( nullRun >= optInTimePeriod ) {
            nullRun = optInTimePeriod;
            upSum = 0.0;
            downSum = 0.0;
         }
         sum = upSum + downSum;
         if( sum > 0.0 ) {
            outReal[outIdx++ * outStride] = 100.0 * (upSum - downSum) / sum;
         } else {
            outReal[outIdx++ * outStride] = 0.0;
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
      sp.nullRun = nullRun;
      sp.upSum = upSum;
      sp.downSum = downSum;
      sp.prevValue = prevValue;
      sp.trailingValue = trailingValue;
      sp.ringPos_trailingIdx = 0;
      sp.ringCap_trailingIdx = cap_trailingIdx;
      sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   /* cmouOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CmouStream cmouOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CmouStream sp = new CmouStream(this);
      RetCode retCode = cmouOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CMOU openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CMOU openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("CMOU openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind cmouOpen (composition seam). */
   CmouStream cmouOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      CmouStream sp = new CmouStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = cmouOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("CMOU open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("CMOU open: internal error", retCode);
      }
      throw new TaLibArgumentException("CMOU open: " + retCode, retCode);
   }
   /**
    * Open a live CMOU stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CMOU} at that bar.
    * <p>The history must hold at least {@code CMOU_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CmouStream cmouOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("CMOU open", "inReal", inReal);
      requireHistory("CMOU open", inReal.length);
      return cmouOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cmouOpen} that also fills the output array(s) bit-identically
    * to {@link Core#CMOU} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CmouStream#outRange()}.
    */
   public CmouStream cmouOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CMOU openAndFill", "inReal", inReal);
      requireHistory("CMOU openAndFill", inReal.length);
      int guardOutLen = openFillCount("CMOU openAndFill", inReal.length, CMOU_Lookback(optInTimePeriod));
      requireLength("CMOU openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("CMOU openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cmouOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
