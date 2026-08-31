/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CF       Christo Fogelberg
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  122104 MF,CF  Fix#1089506 for when optInTimePeriod is 1.
 */

   /**
    * Number of leading input bars {@link Core#PLUS_DM} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Wilder smoothing period (default 14; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int PLUS_DM_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInTimePeriod > 1 ) {
         return optInTimePeriod + this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()] - 1 ;
      } else {
         return 1 ;
      }

   }
   RetCode PLUS_DM_Impl( int startIdx,
                         int endIdx,
                         double inHigh[],
                         double inLow[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double tempReal = 0;
      double prevPlusDM = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /*
       * The DM1 (one period) is base on the largest part of
       * today's range that is outside of yesterdays range.
       *
       * The following 7 cases explain how the +DM and -DM are
       * calculated on one period:
       *
       * Case 1:                       Case 2:
       *    C|                        A|
       *     |                         | C|
       *     | +DM1 = (C-A)           B|  | +DM1 = 0
       *     | -DM1 = 0                   | -DM1 = (B-D)
       * A|  |                           D|
       *  | D|
       * B|
       *
       * Case 3:                       Case 4:
       *    C|                           C|
       *     |                        A|  |
       *     | +DM1 = (C-A)            |  | +DM1 = 0
       *     | -DM1 = 0               B|  | -DM1 = (B-D)
       * A|  |                            |
       *  |  |                           D|
       * B|  |
       *    D|
       *
       * Case 5:                      Case 6:
       * A|                           A| C|
       *  | C| +DM1 = 0                |  |  +DM1 = 0
       *  |  | -DM1 = 0                |  |  -DM1 = 0
       *  | D|                         |  |
       * B|                           B| D|
       *
       *
       * Case 7:
       *
       *    C|
       * A|  |
       *  |  | +DM=0
       * B|  | -DM=0
       *    D|
       *
       * In case 3 and 4, the rule is that the smallest delta between
       * (C-A) and (B-D) determine which of +DM or -DM is zero.
       *
       * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
       * zero.
       *
       * The rules remain the same when A=B and C=D (when the highs
       * equal the lows).
       *
       * When calculating the DM over a period > 1, the one-period DM
       * for the desired period are initialy sum. In other word,
       * for a +DM14, sum the +DM1 for the first 14 days (that's
       * 13 values because there is no DM for the first day!)
       * Subsequent DM are calculated using the Wilder's
       * smoothing approach:
       *
       *                                    Previous +DM14
       *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
       *                                         14
       *
       * Reference:
       *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
       */
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()] - 1;
      } else {
         lookbackTotal = 1;
      }
      /* Adjust startIdx to account for the lookback period. */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Indicate where the next output should be put
       * in the outReal.
       */
      outIdx = 0;
      /* Trap the case where no smoothing is needed. */
      if( optInTimePeriod <= 1 ) {
         /* No smoothing needed. Just do a simple DM1
          * for each price bar.
          */
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      /* Process the initial DM */
      outBegIdx.value = startIdx;
      prevPlusDM = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = inHigh[today];
      prevLow = inLow[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM += diffP;
         }
      }
      /* Process subsequent DM */
      /* Skip the unstable period. */
      i = this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()];
      while( i-- != 0 ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            /* Case 2,4,5 and 7 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
      }
      /* Now start to write the output in
       * the caller provided outReal.
       */
      outReal[0] = prevPlusDM;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = inHigh[today];
         diffP = tempReal - prevHigh;
         /* Plus Delta */
         prevHigh = tempReal;
         tempReal = inLow[today];
         diffM = prevLow - tempReal;
         /* Minus Delta */
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            /* Case 2,4,5 and 7 */
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
         outReal[outIdx++] = prevPlusDM;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode PLUS_DM_Impl( int startIdx,
                         int endIdx,
                         float inHigh[],
                         float inLow[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int today = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      double prevHigh = 0;
      double prevLow = 0;
      double tempReal = 0;
      double prevPlusDM = 0;
      double diffP = 0;
      double diffM = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod > 1 ) {
         lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()] - 1;
      } else {
         lookbackTotal = 1;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod <= 1 ) {
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = (double)inHigh[today];
         prevLow = (double)inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = (double)inHigh[today];
            diffP = tempReal - prevHigh;
            prevHigh = tempReal;
            tempReal = (double)inLow[today];
            diffM = prevLow - tempReal;
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               outReal[outIdx++] = diffP;
            } else {
               outReal[outIdx++] = 0;
            }
         }
         outNBElement.value = outIdx;
         return RetCode.Success ;
      }
      outBegIdx.value = startIdx;
      prevPlusDM = 0.0;
      today = startIdx - lookbackTotal;
      prevHigh = (double)inHigh[today];
      prevLow = (double)inLow[today];
      i = optInTimePeriod - 1;
      while( i-- > 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM += diffP;
         }
      }
      i = this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()];
      while( i-- != 0 ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
      }
      outReal[0] = prevPlusDM;
      outIdx = 1;
      while( today < endIdx ) {
         today += 1;
         tempReal = (double)inHigh[today];
         diffP = tempReal - prevHigh;
         prevHigh = tempReal;
         tempReal = (double)inLow[today];
         diffM = prevLow - tempReal;
         prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
         } else {
            prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
         }
         outReal[outIdx++] = prevPlusDM;
      }
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Plus Directional Movement: the Wilder-smoothed accumulation of upward
    * directional movement (+DM1). A component of the Directional Movement
    * System used to build +DI/DX/ADX.
    * <p><b>Formula</b>
    * <pre>{@code
    * +DM1 = (high - prevHigh) if (high-prevHigh) > 0 and > (prevLow-low), else 0.
    * period<=1: output = +DM1 per bar.
    * period>1: seed = sum of first (period-1) +DM1; then Wilder smoothing:
    * +DM = prevPlusDM - prevPlusDM/period + +DM1(today)
    * }</pre>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PLUS_DM_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Wilder smoothing period (default 14; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Smoothed plus directional movement. Must hold at least
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
    * @see Core#MINUS_DM
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    * @see Core#DX
    * @see Core#ADX
    * @see Core#ADXR
    */
   public OutRange PLUS_DM( int startIdx,
                            int endIdx,
                            double inHigh[],
                            double inLow[],
                            int optInTimePeriod,
                            double outReal[] )
   {
      requireIndexRange("PLUS_DM", startIdx, endIdx);
      int guardStart = clampedStart("PLUS_DM", startIdx, PLUS_DM_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PLUS_DM", "inHigh", inHigh, guardInLen);
      requireLength("PLUS_DM", "inLow", inLow, guardInLen);
      requireLength("PLUS_DM", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PLUS_DM_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PLUS_DM", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Plus Directional Movement: the Wilder-smoothed accumulation of upward
    * directional movement (+DM1). A component of the Directional Movement
    * System used to build +DI/DX/ADX.
    * <p><b>Formula</b>
    * <pre>{@code
    * +DM1 = (high - prevHigh) if (high-prevHigh) > 0 and > (prevLow-low), else 0.
    * period<=1: output = +DM1 per bar.
    * period>1: seed = sum of first (period-1) +DM1; then Wilder smoothing:
    * +DM = prevPlusDM - prevPlusDM/period + +DM1(today)
    * }</pre>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#PLUS_DM_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param optInTimePeriod Wilder smoothing period (default 14; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Smoothed plus directional movement. Must hold at least
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
    * @see Core#MINUS_DM
    * @see Core#PLUS_DI
    * @see Core#MINUS_DI
    * @see Core#DX
    * @see Core#ADX
    * @see Core#ADXR
    */
   public OutRange PLUS_DM( int startIdx,
                            int endIdx,
                            float inHigh[],
                            float inLow[],
                            int optInTimePeriod,
                            double outReal[] )
   {
      requireIndexRange("PLUS_DM", startIdx, endIdx);
      int guardStart = clampedStart("PLUS_DM", startIdx, PLUS_DM_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PLUS_DM", "inHigh", inHigh, guardInLen);
      requireLength("PLUS_DM", "inLow", inLow, guardInLen);
      requireLength("PLUS_DM", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = PLUS_DM_Impl(startIdx, endIdx, inHigh, inLow, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("PLUS_DM", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PLUS_DM stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#PLUS_DM} over the same series.
    * Open with {@link Core#plusDmOpen}; there is no close — the handle is
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
   public static final class PlusDmStream {
      Core core;
      int optInTimePeriod;
      double prevHigh;
      double prevLow;
      double prevPlusDM;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      PlusDmStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#PLUS_DM} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      PlusDmStream( PlusDmStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevHigh = other.prevHigh;
         this.prevLow = other.prevLow;
         this.prevPlusDM = other.prevPlusDM;
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
      public double update( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("PLUS_DM update: BadParam", RetCode.BadParam);
         }
         core.plusDmStepImpl(this, inHigh, inLow);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inHigh[], double inLow[], double outReal[] ) {
         requireArgument("PLUS_DM updateAndFill", "inHigh", inHigh);
         requireArgument("PLUS_DM updateAndFill", "inLow", inLow);
         requireArgument("PLUS_DM updateAndFill", "outReal", outReal);
         final int barCount = inHigh.length;
         if( inLow.length != barCount || outReal.length < barCount || (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow )
            throw new TaLibArgumentException("PLUS_DM updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("PLUS_DM updateAndFill: BadParam", RetCode.BadParam);
            }
            core.plusDmStepImpl(this, inHigh[i], inLow[i]);
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
      public double peek( double inHigh, double inLow ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) )
            throw new TaLibArgumentException("PLUS_DM peek: BadParam", RetCode.BadParam);
         PlusDmStream sp = this;
         double cur_outReal = 0.0;
         if( sp.optInTimePeriod <= 1 ) {
            double tempReal = 0.0;
            double diffP = 0.0;
            double diffM = 0.0;
            double prevHigh = sp.prevHigh;
            double prevLow = sp.prevLow;
            tempReal = inHigh;
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow;
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               cur_outReal = diffP;
            } else {
               cur_outReal = 0;
            }
         } else {
            double tempReal = 0.0;
            double diffP = 0.0;
            double diffM = 0.0;
            double prevHigh = sp.prevHigh;
            double prevLow = sp.prevLow;
            double prevPlusDM = sp.prevPlusDM;
            tempReal = inHigh;
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow;
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / sp.optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / sp.optInTimePeriod;
            }
            cur_outReal = prevPlusDM;
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
      public PlusDmStream clone() {
         return new PlusDmStream(this);
      }
   }
   void plusDmStepImpl( PlusDmStream sp, double inHigh, double inLow )
   {
      if( sp.optInTimePeriod <= 1 ) {
         double tempReal = 0.0;
         double diffP = 0.0;
         double diffM = 0.0;
         tempReal = inHigh;
         diffP = tempReal - sp.prevHigh;
         /* Plus Delta */
         sp.prevHigh = tempReal;
         tempReal = inLow;
         diffM = sp.prevLow - tempReal;
         /* Minus Delta */
         sp.prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            sp.cur_outReal = diffP;
         } else {
            sp.cur_outReal = 0;
         }
      } else {
         double tempReal = 0.0;
         double diffP = 0.0;
         double diffM = 0.0;
         tempReal = inHigh;
         diffP = tempReal - sp.prevHigh;
         /* Plus Delta */
         sp.prevHigh = tempReal;
         tempReal = inLow;
         diffM = sp.prevLow - tempReal;
         /* Minus Delta */
         sp.prevLow = tempReal;
         if( diffP > 0 && diffP > diffM ) {
            /* Case 1 and 3: +DM=diffP,-DM=0 */
            sp.prevPlusDM = sp.prevPlusDM - sp.prevPlusDM / sp.optInTimePeriod + diffP;
         } else {
            /* Case 2,4,5 and 7 */
            sp.prevPlusDM = sp.prevPlusDM - sp.prevPlusDM / sp.optInTimePeriod;
         }
         sp.cur_outReal = sp.prevPlusDM;
      }
   }
   private RetCode plusDmOpenImpl( PlusDmStream sp, double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod <= 1 ) {
         int today = 0;
         int lookbackTotal = 0;
         int outIdx = 0;
         double prevHigh = 0;
         double prevLow = 0;
         double tempReal = 0;
         double prevPlusDM = 0;
         double diffP = 0;
         double diffM = 0;
         int i = 0;
         /*
          * The DM1 (one period) is base on the largest part of
          * today's range that is outside of yesterdays range.
          *
          * The following 7 cases explain how the +DM and -DM are
          * calculated on one period:
          *
          * Case 1:                       Case 2:
          *    C|                        A|
          *     |                         | C|
          *     | +DM1 = (C-A)           B|  | +DM1 = 0
          *     | -DM1 = 0                   | -DM1 = (B-D)
          * A|  |                           D|
          *  | D|
          * B|
          *
          * Case 3:                       Case 4:
          *    C|                           C|
          *     |                        A|  |
          *     | +DM1 = (C-A)            |  | +DM1 = 0
          *     | -DM1 = 0               B|  | -DM1 = (B-D)
          * A|  |                            |
          *  |  |                           D|
          * B|  |
          *    D|
          *
          * Case 5:                      Case 6:
          * A|                           A| C|
          *  | C| +DM1 = 0                |  |  +DM1 = 0
          *  |  | -DM1 = 0                |  |  -DM1 = 0
          *  | D|                         |  |
          * B|                           B| D|
          *
          *
          * Case 7:
          *
          *    C|
          * A|  |
          *  |  | +DM=0
          * B|  | -DM=0
          *    D|
          *
          * In case 3 and 4, the rule is that the smallest delta between
          * (C-A) and (B-D) determine which of +DM or -DM is zero.
          *
          * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
          * zero.
          *
          * The rules remain the same when A=B and C=D (when the highs
          * equal the lows).
          *
          * When calculating the DM over a period > 1, the one-period DM
          * for the desired period are initialy sum. In other word,
          * for a +DM14, sum the +DM1 for the first 14 days (that's
          * 13 values because there is no DM for the first day!)
          * Subsequent DM are calculated using the Wilder's
          * smoothing approach:
          *
          *                                    Previous +DM14
          *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
          *                                         14
          *
          * Reference:
          *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
          */
         if( optInTimePeriod > 1 ) {
            lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()] - 1;
         } else {
            lookbackTotal = 1;
         }
         /* Adjust startIdx to account for the lookback period. */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.InsufficientHistory ;
         }
         /* Indicate where the next output should be put
          * in the outReal.
          */
         outIdx = 0;
         /* Trap the case where no smoothing is needed. */
         /* No smoothing needed. Just do a simple DM1
          * for each price bar.
          */
         outBegIdx.value = startIdx;
         today = startIdx - 1;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               outReal[outIdx++ * outStride] = diffP;
            } else {
               outReal[outIdx++ * outStride] = 0;
            }
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevHigh = prevHigh;
         sp.prevLow = prevLow;
         sp.prevPlusDM = prevPlusDM;
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      } else {
         int today = 0;
         int lookbackTotal = 0;
         int outIdx = 0;
         double prevHigh = 0;
         double prevLow = 0;
         double tempReal = 0;
         double prevPlusDM = 0;
         double diffP = 0;
         double diffM = 0;
         int i = 0;
         /*
          * The DM1 (one period) is base on the largest part of
          * today's range that is outside of yesterdays range.
          *
          * The following 7 cases explain how the +DM and -DM are
          * calculated on one period:
          *
          * Case 1:                       Case 2:
          *    C|                        A|
          *     |                         | C|
          *     | +DM1 = (C-A)           B|  | +DM1 = 0
          *     | -DM1 = 0                   | -DM1 = (B-D)
          * A|  |                           D|
          *  | D|
          * B|
          *
          * Case 3:                       Case 4:
          *    C|                           C|
          *     |                        A|  |
          *     | +DM1 = (C-A)            |  | +DM1 = 0
          *     | -DM1 = 0               B|  | -DM1 = (B-D)
          * A|  |                            |
          *  |  |                           D|
          * B|  |
          *    D|
          *
          * Case 5:                      Case 6:
          * A|                           A| C|
          *  | C| +DM1 = 0                |  |  +DM1 = 0
          *  |  | -DM1 = 0                |  |  -DM1 = 0
          *  | D|                         |  |
          * B|                           B| D|
          *
          *
          * Case 7:
          *
          *    C|
          * A|  |
          *  |  | +DM=0
          * B|  | -DM=0
          *    D|
          *
          * In case 3 and 4, the rule is that the smallest delta between
          * (C-A) and (B-D) determine which of +DM or -DM is zero.
          *
          * In case 7, (C-A) and (B-D) are equal, so both +DM and -DM are
          * zero.
          *
          * The rules remain the same when A=B and C=D (when the highs
          * equal the lows).
          *
          * When calculating the DM over a period > 1, the one-period DM
          * for the desired period are initialy sum. In other word,
          * for a +DM14, sum the +DM1 for the first 14 days (that's
          * 13 values because there is no DM for the first day!)
          * Subsequent DM are calculated using the Wilder's
          * smoothing approach:
          *
          *                                    Previous +DM14
          *  Today's +DM14 = Previous +DM14 -  -------------- + Today's +DM1
          *                                         14
          *
          * Reference:
          *    New Concepts In Technical Trading Systems, J. Welles Wilder Jr
          */
         if( optInTimePeriod > 1 ) {
            lookbackTotal = optInTimePeriod + this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()] - 1;
         } else {
            lookbackTotal = 1;
         }
         /* Adjust startIdx to account for the lookback period. */
         if( startIdx < lookbackTotal ) {
            startIdx = lookbackTotal;
         }
         /* Make sure there is still something to evaluate. */
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.InsufficientHistory ;
         }
         /* Indicate where the next output should be put
          * in the outReal.
          */
         outIdx = 0;
         /* Trap the case where no smoothing is needed. */
         /* Process the initial DM */
         outBegIdx.value = startIdx;
         prevPlusDM = 0.0;
         today = startIdx - lookbackTotal;
         prevHigh = inHigh[today];
         prevLow = inLow[today];
         i = optInTimePeriod - 1;
         while( i-- > 0 ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM += diffP;
            }
         }
         /* Process subsequent DM */
         /* Skip the unstable period. */
         i = this.unstablePeriod[FuncUnstId.PLUS_DM.ordinal()];
         while( i-- != 0 ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
            }
         }
         /* Now start to write the output in
          * the caller provided outReal.
          */
         outReal[0 * outStride] = prevPlusDM;
         outIdx = 1;
         while( today < endIdx ) {
            today += 1;
            tempReal = inHigh[today];
            diffP = tempReal - prevHigh;
            /* Plus Delta */
            prevHigh = tempReal;
            tempReal = inLow[today];
            diffM = prevLow - tempReal;
            /* Minus Delta */
            prevLow = tempReal;
            if( diffP > 0 && diffP > diffM ) {
               /* Case 1 and 3: +DM=diffP,-DM=0 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod + diffP;
            } else {
               /* Case 2,4,5 and 7 */
               prevPlusDM = prevPlusDM - prevPlusDM / optInTimePeriod;
            }
            outReal[outIdx++ * outStride] = prevPlusDM;
         }
         outNBElement.value = outIdx;
         /* Capture the live batch state into the handle. */
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevHigh = prevHigh;
         sp.prevLow = prevLow;
         sp.prevPlusDM = prevPlusDM;
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
   }
   /* plusDmOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PlusDmStream plusDmOpenAndFillInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PlusDmStream sp = new PlusDmStream(this);
      RetCode retCode = plusDmOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PLUS_DM openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PLUS_DM openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("PLUS_DM openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind plusDmOpen (composition seam). */
   PlusDmStream plusDmOpenInternal( double inHigh[], double inLow[], int startIdx, int optInTimePeriod )
   {
      PlusDmStream sp = new PlusDmStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = plusDmOpenImpl(sp, inHigh, inLow, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("PLUS_DM open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("PLUS_DM open: internal error", retCode);
      }
      throw new TaLibArgumentException("PLUS_DM open: " + retCode, retCode);
   }
   /**
    * Open a live PLUS_DM stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#PLUS_DM} at that bar.
    * <p>The history must hold at least {@code PLUS_DM_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PlusDmStream plusDmOpen( double inHigh[], double inLow[], int optInTimePeriod )
   {
      requireArgument("PLUS_DM open", "inHigh", inHigh);
      requireHistory("PLUS_DM open", inHigh.length);
      requireArgument("PLUS_DM open", "inLow", inLow);
      requireHistoryLength("PLUS_DM open", "inLow", inLow.length, inHigh.length);
      return plusDmOpenInternal(inHigh, inLow, 0, optInTimePeriod);
   }
   /**
    * {@link Core#plusDmOpen} that also fills the output array(s) bit-identically
    * to {@link Core#PLUS_DM} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PlusDmStream#outRange()}.
    */
   public PlusDmStream plusDmOpenAndFill( double inHigh[], double inLow[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("PLUS_DM openAndFill", "inHigh", inHigh);
      requireHistory("PLUS_DM openAndFill", inHigh.length);
      requireArgument("PLUS_DM openAndFill", "inLow", inLow);
      int guardOutLen = openFillCount("PLUS_DM openAndFill", inHigh.length, PLUS_DM_Lookback(optInTimePeriod));
      requireHistoryLength("PLUS_DM openAndFill", "inLow", inLow.length, inHigh.length);
      requireLength("PLUS_DM openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inHigh || (Object)outReal == (Object)inLow ) {
         throw new TaLibArgumentException("PLUS_DM openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return plusDmOpenAndFillInternal(inHigh, inLow, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
