/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  BT       Barry Tsung
 *
 * Change history:
 *
 *  MMDDYY BY      Description
 *  -------------------------------------------------------------------
 *  112605 MF      Initial version.
 *  021806 MF,BT   Fix #1434450 reported by BT.
 */

   /**
    * Number of leading input bars {@link Core#CMO} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Bars over which gains/losses are smoothed (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CMO_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      retValue = optInTimePeriod + this.unstablePeriod[FuncUnstId.CMO.ordinal()];
      return retValue ;

   }
   RetCode CMO_Internal( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int unstablePeriod = 0;
      int i = 0;
      double prevGain = 0;
      double prevLoss = 0;
      double prevValue = 0;
      double savePrevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      double tempValue4 = 0;
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
      /* CMO calculation is mostly identical to RSI.
       *
       * The only difference is in the last step of calculation:
       *
       *   RSI = gain / (gain+loss)
       *   CMO = (gain-loss) / (gain+loss)
       *
       * See the RSI function for potentially some more info
       * on this algo.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = CMO_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Trap special case where the period is '1'.
       * In that case, just copy the input into the
       * output for the requested range (as-is !)
       */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         i = endIdx - startIdx + 1;
         outNBElement.value = i;
         /* Element loop, not a block copy: the C single-precision variant reads a
          * float array, so a double-sized byte copy would reinterpret and
          * over-read it (#137). Forward order keeps the in-place case correct (#94).
          */
         today = startIdx;
         for( outIdx = 0; outIdx < i; outIdx += 1 ) {
            outReal[outIdx] = inReal[today++];
         }
         return RetCode.Success ;
      }
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.CMO.ordinal()];
      /* If there is no unstable period,
       * calculate the 'additional' initial
       * price bar who is particuliar to
       * metastock.
       * If there is an unstable period,
       * no need to calculate since this
       * first value will be surely skip.
       */
      /* Remaining of the processing is identical
       * for both Classic calculation and Metastock.
       */
      prevGain = 0.0;
      prevLoss = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         if( tempValue2 < 0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
      }
      /* Subsequent prevLoss and prevGain are smoothed
       * using the previous values (Wilder's approach).
       *  1) Multiply the previous by 'period-1'.
       *  2) Add today value.
       *  3) Divide by 'period'.
       */
      prevLoss /= optInTimePeriod;
      prevGain /= optInTimePeriod;
      /* Often documentation present the RSI calculation as follow:
       *    RSI = 100 - (100 / 1 + (prevGain/prevLoss))
       *
       * The following is equivalent:
       *    RSI = 100 * (prevGain/(prevGain+prevLoss))
       *
       * The second equation is used here for speed optimization.
       */
      if( today > startIdx ) {
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * ((prevGain - prevLoss) / tempValue1);
         } else {
            outReal[outIdx++] = 0.0;
         }
      } else {
         /* Skip the unstable period. Do the processing
          * but do not write it in the output.
          */
         while( today < startIdx ) {
            tempValue1 = inReal[today];
            tempValue2 = tempValue1 - prevValue;
            prevValue = tempValue1;
            prevLoss *= optInTimePeriod - 1;
            prevGain *= optInTimePeriod - 1;
            if( tempValue2 < 0 ) {
               prevLoss -= tempValue2;
            } else {
               prevGain += tempValue2;
            }
            prevLoss /= optInTimePeriod;
            prevGain /= optInTimePeriod;
            today += 1;
         }
      }
      /* Unstable period skipped... now continue
       * processing if needed.
       */
      while( today <= endIdx ) {
         tempValue1 = inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= optInTimePeriod - 1;
         prevGain *= optInTimePeriod - 1;
         if( tempValue2 < 0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
         prevLoss /= optInTimePeriod;
         prevGain /= optInTimePeriod;
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * ((prevGain - prevLoss) / tempValue1);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode CMO_Internal( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int unstablePeriod = 0;
      int i = 0;
      double prevGain = 0;
      double prevLoss = 0;
      double prevValue = 0;
      double savePrevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      double tempValue4 = 0;
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
      lookbackTotal = CMO_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         i = endIdx - startIdx + 1;
         outNBElement.value = i;
         today = startIdx;
         for( outIdx = 0; outIdx < i; outIdx += 1 ) {
            outReal[outIdx] = (double)inReal[today++];
         }
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.CMO.ordinal()];
      prevGain = 0.0;
      prevLoss = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (double)inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         if( tempValue2 < 0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
      }
      prevLoss /= optInTimePeriod;
      prevGain /= optInTimePeriod;
      if( today > startIdx ) {
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * ((prevGain - prevLoss) / tempValue1);
         } else {
            outReal[outIdx++] = 0.0;
         }
      } else {
         while( today < startIdx ) {
            tempValue1 = (double)inReal[today];
            tempValue2 = tempValue1 - prevValue;
            prevValue = tempValue1;
            prevLoss *= optInTimePeriod - 1;
            prevGain *= optInTimePeriod - 1;
            if( tempValue2 < 0 ) {
               prevLoss -= tempValue2;
            } else {
               prevGain += tempValue2;
            }
            prevLoss /= optInTimePeriod;
            prevGain /= optInTimePeriod;
            today += 1;
         }
      }
      while( today <= endIdx ) {
         tempValue1 = (double)inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= optInTimePeriod - 1;
         prevGain *= optInTimePeriod - 1;
         if( tempValue2 < 0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
         prevLoss /= optInTimePeriod;
         prevGain /= optInTimePeriod;
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx++] = 100.0 * ((prevGain - prevLoss) / tempValue1);
         } else {
            outReal[outIdx++] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed
    * average up-moves and down-moves. Identical to RSI except the numerator
    * uses (gain-loss) instead of gain. Bounded in [-100,+100]; positive = net
    * upward momentum, negative = net downward.
    * <p><b>Formula</b>
    * <pre>{@code
    * d = P[t]-P[t-1]; over the initial period accumulate gain = sum of positive d, loss = sum of -d for negative d. Wilder-smooth each: prevGain = (prevGain*(period-1) + gain_today)/period (same for loss). CMO = 100 * (prevGain-prevLoss)/(prevGain+prevLoss); 0 when prevGain+prevLoss == 0.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Gains and losses are smoothed with Wilder's method (as in RSI) rather than the simple period sums of Chande's original definition.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CMO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Bars over which gains/losses are smoothed (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal CMO oscillator value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#RSI
    */
   public OutRange CMO( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMO_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed
    * average up-moves and down-moves. Identical to RSI except the numerator
    * uses (gain-loss) instead of gain. Bounded in [-100,+100]; positive = net
    * upward momentum, negative = net downward.
    * <p><b>Formula</b>
    * <pre>{@code
    * d = P[t]-P[t-1]; over the initial period accumulate gain = sum of positive d, loss = sum of -d for negative d. Wilder-smooth each: prevGain = (prevGain*(period-1) + gain_today)/period (same for loss). CMO = 100 * (prevGain-prevLoss)/(prevGain+prevLoss); 0 when prevGain+prevLoss == 0.
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Gains and losses are smoothed with Wilder's method (as in RSI) rather than the simple period sums of Chande's original definition.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CMO_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price/value series.
    * @param optInTimePeriod Bars over which gains/losses are smoothed (default
    *        14; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal CMO oscillator value. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#RSI
    */
   public OutRange CMO( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMO_Internal(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("CMO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CMO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CMO} over the same series.
    * Open with {@link Core#CMO_Open}; there is no close — the handle is
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
   public static final class CMO_Stream {
      Core core;
      int optInTimePeriod;
      double prevGain;
      double prevLoss;
      double prevValue;
      double cur_outReal;
      OutRange fillRange = OutRange.EMPTY;

      CMO_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CMO_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CMO_Stream( CMO_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevGain = other.prevGain;
         this.prevLoss = other.prevLoss;
         this.prevValue = other.prevValue;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      void copyFrom( CMO_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevGain = other.prevGain;
         this.prevLoss = other.prevLoss;
         this.prevValue = other.prevValue;
         this.cur_outReal = other.cur_outReal;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.CMO_StreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         CMO_Stream scratch = new CMO_Stream(this);
         core.CMO_StreamStep(scratch, inReal);
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
      public CMO_Stream copy() {
         return new CMO_Stream(this);
      }
   }
   void CMO_StreamStep( CMO_Stream sp, double inReal )
   {
      double tempValue1 = 0.0;
      double tempValue2 = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      tempValue1 = inReal;
      tempValue2 = tempValue1 - sp.prevValue;
      sp.prevValue = tempValue1;
      sp.prevLoss *= sp.optInTimePeriod - 1;
      sp.prevGain *= sp.optInTimePeriod - 1;
      if( tempValue2 < 0 ) {
         sp.prevLoss -= tempValue2;
      } else {
         sp.prevGain += tempValue2;
      }
      sp.prevLoss /= sp.optInTimePeriod;
      sp.prevGain /= sp.optInTimePeriod;
      tempValue1 = sp.prevGain + sp.prevLoss;
      if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
         sp.cur_outReal = 100.0 * ((sp.prevGain - sp.prevLoss) / tempValue1);
      } else {
         sp.cur_outReal = 0.0;
      }
   }
   private RetCode CMO_OpenCore( CMO_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int outIdx = 0;
      int today = 0;
      int lookbackTotal = 0;
      int unstablePeriod = 0;
      int i = 0;
      double prevGain = 0;
      double prevLoss = 0;
      double prevValue = 0;
      double savePrevValue = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double tempValue3 = 0;
      double tempValue4 = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < CMO_Lookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevGain = 0.0;
         sp.prevLoss = 0.0;
         sp.prevValue = 0.0;
         int fillLb = CMO_Lookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
      /* CMO calculation is mostly identical to RSI.
       *
       * The only difference is in the last step of calculation:
       *
       *   RSI = gain / (gain+loss)
       *   CMO = (gain-loss) / (gain+loss)
       *
       * See the RSI function for potentially some more info
       * on this algo.
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = CMO_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.CMO.ordinal()];
      /* If there is no unstable period,
       * calculate the 'additional' initial
       * price bar who is particuliar to
       * metastock.
       * If there is an unstable period,
       * no need to calculate since this
       * first value will be surely skip.
       */
      /* Remaining of the processing is identical
       * for both Classic calculation and Metastock.
       */
      prevGain = 0.0;
      prevLoss = 0.0;
      today += 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         if( tempValue2 < 0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
      }
      /* Subsequent prevLoss and prevGain are smoothed
       * using the previous values (Wilder's approach).
       *  1) Multiply the previous by 'period-1'.
       *  2) Add today value.
       *  3) Divide by 'period'.
       */
      prevLoss /= optInTimePeriod;
      prevGain /= optInTimePeriod;
      /* Often documentation present the RSI calculation as follow:
       *    RSI = 100 - (100 / 1 + (prevGain/prevLoss))
       *
       * The following is equivalent:
       *    RSI = 100 * (prevGain/(prevGain+prevLoss))
       *
       * The second equation is used here for speed optimization.
       */
      if( today > startIdx ) {
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx++ * outStride] = 100.0 * ((prevGain - prevLoss) / tempValue1);
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
      } else {
         /* Skip the unstable period. Do the processing
          * but do not write it in the output.
          */
         while( today < startIdx ) {
            tempValue1 = inReal[today];
            tempValue2 = tempValue1 - prevValue;
            prevValue = tempValue1;
            prevLoss *= optInTimePeriod - 1;
            prevGain *= optInTimePeriod - 1;
            if( tempValue2 < 0 ) {
               prevLoss -= tempValue2;
            } else {
               prevGain += tempValue2;
            }
            prevLoss /= optInTimePeriod;
            prevGain /= optInTimePeriod;
            today += 1;
         }
      }
      /* Unstable period skipped... now continue
       * processing if needed.
       */
      while( today <= endIdx ) {
         tempValue1 = inReal[today++];
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= optInTimePeriod - 1;
         prevGain *= optInTimePeriod - 1;
         if( tempValue2 < 0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
         prevLoss /= optInTimePeriod;
         prevGain /= optInTimePeriod;
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx++ * outStride] = 100.0 * ((prevGain - prevLoss) / tempValue1);
         } else {
            outReal[outIdx++ * outStride] = 0.0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevGain = prevGain;
      sp.prevLoss = prevLoss;
      sp.prevValue = prevValue;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.Success;
   }
   private RetCode CMO_OpenBody( CMO_Stream sp, double inReal[], int startIdx, int optInTimePeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      return CMO_OpenCore( sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0 );
   }
   private RetCode CMO_OpenAndFillBody( CMO_Stream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      return CMO_OpenCore( sp, inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal, 1 );
   }
   private RetCode CMO_OpenAndFillInternalBody( CMO_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      return CMO_OpenCore(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
   }
   /* CMO_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CMO_Stream CMO_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CMO_Stream sp = new CMO_Stream(this);
      RetCode retCode = CMO_OpenAndFillInternalBody(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CMO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CMO openAndFill: internal error");
      }
      throw new IllegalArgumentException("CMO openAndFill: " + retCode);
   }
   /* Internal startIdx-anchored open behind CMO_Open (composition seam). */
   CMO_Stream CMO_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      CMO_Stream sp = new CMO_Stream(this);
      RetCode retCode = CMO_OpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CMO open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CMO open: internal error");
      }
      throw new IllegalArgumentException("CMO open: " + retCode);
   }
   /**
    * Open a live CMO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CMO} at that bar.
    * <p>The history must hold at least {@code CMO_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CMO_Stream CMO_Open( double inReal[], int optInTimePeriod )
   {
      return CMO_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#CMO_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CMO} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CMO_Stream#fillRange()}.
    */
   public CMO_Stream CMO_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      CMO_Stream sp = new CMO_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CMO_OpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CMO openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CMO openAndFill: internal error");
      }
      throw new IllegalArgumentException("CMO openAndFill: " + retCode);
   }
