/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  062804 MF   Resolve div by zero bug on limit case.
 */

   /**
    * Number of leading input bars {@link Core#RSI} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    * <p>This function is recursive, so the result also includes this
    * {@code Core}'s unstable-period setting — which is why it is an instance
    * method.
    *
    * @param optInTimePeriod Lookback for the gain/loss averaging (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int RSI_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      retValue = optInTimePeriod + this.unstablePeriod[FuncUnstId.RSI.ordinal()];
      return retValue ;

   }
   RetCode RSI_Impl( int startIdx,
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
      /* The following algorithm is base on the original
       * work from Wilder's and shall represent the
       * original idea behind the classic RSI.
       *
       * Metastock is starting the calculation one price
       * bar earlier. To make this possible, they assume
       * that the very first bar will be identical to the
       * previous one (no gain or loss).
       */
      /* If changing this function, please check also CMO
       * which is mostly identical (just different in one step
       * of calculation).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = (int)RSI_Lookback(optInTimePeriod);
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
         i = (int)(endIdx - startIdx + 1);
         outNBElement.value = (int)i;
         /* Element loop, not a block copy: the C single-precision variant reads a
          * float array, so a double-sized byte copy would reinterpret and
          * over-read it (#137). Forward order keeps the in-place case correct (#94).
          */
         today = (int)startIdx;
         for( outIdx = 0; outIdx < (int)i; outIdx += 1 ) {
            outReal[outIdx] = inReal[today++];
         }
         return RetCode.Success ;
      }
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.RSI.ordinal()];
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
      today = today + 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         if( tempValue2 < 0.0 ) {
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
      prevLoss /= (double)optInTimePeriod;
      prevGain /= (double)optInTimePeriod;
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
            outReal[outIdx] = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
      } else {
         /* Skip the unstable period. Do the processing
          * but do not write it in the output.
          */
         while( today < startIdx ) {
            tempValue1 = (double)inReal[today];
            tempValue2 = tempValue1 - prevValue;
            prevValue = tempValue1;
            prevLoss *= (double)(optInTimePeriod - 1);
            prevGain *= (double)(optInTimePeriod - 1);
            if( tempValue2 < 0.0 ) {
               prevLoss -= tempValue2;
            } else {
               prevGain += tempValue2;
            }
            prevLoss /= (double)optInTimePeriod;
            prevGain /= (double)optInTimePeriod;
            today = today + 1;
         }
      }
      /* Unstable period skipped... now continue
       * processing if needed.
       */
      while( today <= endIdx ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         if( tempValue2 < 0.0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
         prevLoss /= (double)optInTimePeriod;
         prevGain /= (double)optInTimePeriod;
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode RSI_Impl( int startIdx,
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
      lookbackTotal = (int)RSI_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         i = (int)(endIdx - startIdx + 1);
         outNBElement.value = (int)i;
         today = (int)startIdx;
         for( outIdx = 0; outIdx < (int)i; outIdx += 1 ) {
            outReal[outIdx] = (double)inReal[today++];
         }
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.RSI.ordinal()];
      prevGain = 0.0;
      prevLoss = 0.0;
      today = today + 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         if( tempValue2 < 0.0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
      }
      prevLoss /= (double)optInTimePeriod;
      prevGain /= (double)optInTimePeriod;
      if( today > startIdx ) {
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
      } else {
         while( today < startIdx ) {
            tempValue1 = (double)inReal[today];
            tempValue2 = tempValue1 - prevValue;
            prevValue = tempValue1;
            prevLoss *= (double)(optInTimePeriod - 1);
            prevGain *= (double)(optInTimePeriod - 1);
            if( tempValue2 < 0.0 ) {
               prevLoss -= tempValue2;
            } else {
               prevGain += tempValue2;
            }
            prevLoss /= (double)optInTimePeriod;
            prevGain /= (double)optInTimePeriod;
            today = today + 1;
         }
      }
      while( today <= endIdx ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         if( tempValue2 < 0.0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
         prevLoss /= (double)optInTimePeriod;
         prevGain /= (double)optInTimePeriod;
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from
    * the ratio of average gains to average losses over the period. Used to
    * gauge overbought/oversold conditions. &gt;70 overbought, &lt;30 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * $$
    * \begin{aligned}
    * U_t &= \max(X_t - X_{t-1},\ 0)
    * &  D_t &= \max(X_{t-1} - X_t,\ 0) \\[4pt]
    * \overline{U}_t &= \begin{cases}
    * \operatorname{SMA}(U, n)_t                 & \text{if } t = n \\[4pt]
    * \dfrac{(n-1)\,\overline{U}_{t-1} + U_t}{n} & \text{if } t > n
    * \end{cases}
    * &  \overline{D}_t &= \begin{cases}
    * \operatorname{SMA}(D, n)_t                 & \text{if } t = n \\[4pt]
    * \dfrac{(n-1)\,\overline{D}_{t-1} + D_t}{n} & \text{if } t > n
    * \end{cases} \\[4pt]
    * \mathrm{RS}_t &= \frac{\overline{U}_t}{\overline{D}_t}
    * &  \mathrm{RSI}_t &= 100 - \frac{100}{1 + \mathrm{RS}_t}
    * \end{aligned}
    * $$
    * }</pre>
    * <p>where $X$ is the input series and $n$ the period.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RSI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Price series (typically close)
    * @param optInTimePeriod Lookback for the gain/loss averaging (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal RSI value. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CMO
    * @see Core#STOCHRSI
    */
   public OutRange RSI( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("RSI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, RSI_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RSI", "inReal", inReal, guardInLen);
      requireLength("RSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RSI_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from
    * the ratio of average gains to average losses over the period. Used to
    * gauge overbought/oversold conditions. &gt;70 overbought, &lt;30 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * $$
    * \begin{aligned}
    * U_t &= \max(X_t - X_{t-1},\ 0)
    * &  D_t &= \max(X_{t-1} - X_t,\ 0) \\[4pt]
    * \overline{U}_t &= \begin{cases}
    * \operatorname{SMA}(U, n)_t                 & \text{if } t = n \\[4pt]
    * \dfrac{(n-1)\,\overline{U}_{t-1} + U_t}{n} & \text{if } t > n
    * \end{cases}
    * &  \overline{D}_t &= \begin{cases}
    * \operatorname{SMA}(D, n)_t                 & \text{if } t = n \\[4pt]
    * \dfrac{(n-1)\,\overline{D}_{t-1} + D_t}{n} & \text{if } t > n
    * \end{cases} \\[4pt]
    * \mathrm{RS}_t &= \frac{\overline{U}_t}{\overline{D}_t}
    * &  \mathrm{RSI}_t &= 100 - \frac{100}{1 + \mathrm{RS}_t}
    * \end{aligned}
    * $$
    * }</pre>
    * <p>where $X$ is the input series and $n$ the period.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#RSI_Lookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Price series (typically close)
    * @param optInTimePeriod Lookback for the gain/loss averaging (default 14;
    *        range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal RSI value. Must hold at least {@code endIdx - startIdx + 1}
    *        values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is too short
    *        for the range requested — an input this function <i>reads</i> that does
    *        not reach {@code endIdx}, or an output that cannot hold the values
    *        produced. Checked before anything is written, so a rejected call leaves
    *        every buffer untouched.
    * @throws NullPointerException if an input this function reads, or any
    *        output, is null. A few candlestick patterns declare an OHLC series they
    *        never index; those are neither length-checked nor null-checked, because
    *        rejecting them would refuse a call the algorithm can answer.
    *
    * @see Core#CMO
    * @see Core#STOCHRSI
    */
   public OutRange RSI( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double outReal[] )
   {
      requireIndexRange("RSI", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, RSI_Lookback(optInTimePeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("RSI", "inReal", inReal, guardInLen);
      requireLength("RSI", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = RSI_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("RSI", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live RSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#RSI} over the same series.
    * Open with {@link Core#RSI_Open}; there is no close — the handle is
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
   public static final class RSI_Stream {
      Core core;
      int optInTimePeriod;
      double prevGain;
      double prevLoss;
      double prevValue;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      RSI_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#RSI} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      RSI_Stream( RSI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevGain = other.prevGain;
         this.prevLoss = other.prevLoss;
         this.prevValue = other.prevValue;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( RSI_Stream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevGain = other.prevGain;
         this.prevLoss = other.prevLoss;
         this.prevValue = other.prevValue;
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

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
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("RSI update: BadParam", RetCode.BadParam);
         core.RSI_StepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("RSI updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) )
               throw new TaLibArgumentException("RSI updateAndFill: BadParam", RetCode.BadParam);
            core.RSI_StepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("RSI peek: BadParam", RetCode.BadParam);
         RSI_Stream scratch = new RSI_Stream(this);
         core.RSI_StepImpl(scratch, inReal);
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
      public RSI_Stream copy() {
         return new RSI_Stream(this);
      }
   }
   void RSI_StepImpl( RSI_Stream sp, double inReal )
   {
      double tempValue1 = 0.0;
      double tempValue2 = 0.0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      tempValue1 = (double)inReal;
      tempValue2 = tempValue1 - sp.prevValue;
      sp.prevValue = tempValue1;
      sp.prevLoss *= (double)(sp.optInTimePeriod - 1);
      sp.prevGain *= (double)(sp.optInTimePeriod - 1);
      if( tempValue2 < 0.0 ) {
         sp.prevLoss -= tempValue2;
      } else {
         sp.prevGain += tempValue2;
      }
      sp.prevLoss /= (double)sp.optInTimePeriod;
      sp.prevGain /= (double)sp.optInTimePeriod;
      tempValue1 = sp.prevGain + sp.prevLoss;
      if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
         sp.cur_outReal = 100.0 * (sp.prevGain / tempValue1);
      } else {
         sp.cur_outReal = 0.0;
      }
   }
   private RetCode RSI_OpenImpl( RSI_Stream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
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
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = RSI_Lookback(optInTimePeriod);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.InsufficientHistory;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevGain = 0.0;
         sp.prevLoss = 0.0;
         sp.prevValue = 0.0;
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
      /* The following algorithm is base on the original
       * work from Wilder's and shall represent the
       * original idea behind the classic RSI.
       *
       * Metastock is starting the calculation one price
       * bar earlier. To make this possible, they assume
       * that the very first bar will be identical to the
       * previous one (no gain or loss).
       */
      /* If changing this function, please check also CMO
       * which is mostly identical (just different in one step
       * of calculation).
       */
      outBegIdx.value = 0;
      outNBElement.value = 0;
      /* Adjust startIdx to account for the lookback period. */
      lookbackTotal = (int)RSI_Lookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.InsufficientHistory ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.RSI.ordinal()];
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
      today = today + 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         if( tempValue2 < 0.0 ) {
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
      prevLoss /= (double)optInTimePeriod;
      prevGain /= (double)optInTimePeriod;
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
            outReal[outIdx * outStride] = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx * outStride] = 0.0;
            outIdx = outIdx + 1;
         }
      } else {
         /* Skip the unstable period. Do the processing
          * but do not write it in the output.
          */
         while( today < startIdx ) {
            tempValue1 = (double)inReal[today];
            tempValue2 = tempValue1 - prevValue;
            prevValue = tempValue1;
            prevLoss *= (double)(optInTimePeriod - 1);
            prevGain *= (double)(optInTimePeriod - 1);
            if( tempValue2 < 0.0 ) {
               prevLoss -= tempValue2;
            } else {
               prevGain += tempValue2;
            }
            prevLoss /= (double)optInTimePeriod;
            prevGain /= (double)optInTimePeriod;
            today = today + 1;
         }
      }
      /* Unstable period skipped... now continue
       * processing if needed.
       */
      while( today <= endIdx ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         if( tempValue2 < 0.0 ) {
            prevLoss -= tempValue2;
         } else {
            prevGain += tempValue2;
         }
         prevLoss /= (double)optInTimePeriod;
         prevGain /= (double)optInTimePeriod;
         tempValue1 = prevGain + prevLoss;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx * outStride] = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx * outStride] = 0.0;
            outIdx = outIdx + 1;
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
   /* RSI_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   RSI_Stream RSI_OpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RSI_Stream sp = new RSI_Stream(this);
      RetCode retCode = RSI_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RSI openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("RSI openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind RSI_Open (composition seam). */
   RSI_Stream RSI_OpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      RSI_Stream sp = new RSI_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = RSI_OpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("RSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("RSI open: internal error", retCode);
      }
      throw new TaLibArgumentException("RSI open: " + retCode, retCode);
   }
   /**
    * Open a live RSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#RSI} at that bar.
    * <p>The history must hold at least {@code RSI_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public RSI_Stream RSI_Open( double inReal[], int optInTimePeriod )
   {
      return RSI_OpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#RSI_Open} that also fills the output array(s) bit-identically
    * to {@link Core#RSI} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link RSI_Stream#outRange()}.
    */
   public RSI_Stream RSI_OpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("RSI openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return RSI_OpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
