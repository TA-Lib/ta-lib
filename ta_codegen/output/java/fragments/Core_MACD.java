/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JPP      JP Pienaar (j.pienaar@mci.co.za)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  080403 JPP    Fix #767653 for logic when swapping periods.
 *  070526 MF,CC  Speed optimization: compute the two price EMA, the
 *                signal line and the histogram in a single lockstep
 *                pass (bit-exact, no temporary buffers).
 *  080926 MF,CC  Explicit no-smoothing signal at a signal period of 1.
 */

   /**
    * Number of leading input bars {@link Core#MACD} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 26; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MACD_Lookback( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      int tempInteger;
      /* The lookback is driven by the signal line output.
       *
       * (must also account for the initial data consume
       *  by the slow period).
       */
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      return EMA_Lookback(optInSlowPeriod) + EMA_Lookback(optInSignalPeriod) ;

   }
   RetCode MACD_Impl( int startIdx,
                      int endIdx,
                      double inReal[],
                      int optInFastPeriod,
                      int optInSlowPeriod,
                      int optInSignalPeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outMACD[],
                      double outMACDSignal[],
                      double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Catch special case for fix 26/12 MACD.
       * Use hardcoded k values matching the original algorithm.
       */
      if( optInSlowPeriod == 0 ) {
         /* Fix 26 */
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         /* Fix 12 */
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      /* A signal period of 1 disables signal-line smoothing: the signal IS the
       * MACD line and the histogram is exactly zero. signalK is then exactly
       * 1.0, so the recursion below reduces to (x-prev)+prev -- which returns x
       * only while consecutive MACD-line values stay within a factor of two of
       * each other. The MACD line oscillates through zero, so it leaves that
       * window on ordinary data; hence the explicit arm at each step.
       */
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = EMA_Lookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += EMA_Lookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Everything is computed in a single lockstep pass: each bar
       * advances the fast and slow EMA (two independent recursions),
       * their difference is the MACD line, and each MACD-line value
       * is immediately fed into the signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum of
       *    its first 'period' inputs, accumulated from 0.0 in input
       *    order, divided by the period. The fast and slow seed
       *    windows end on the same bar. The signal EMA is seeded the
       *    same way from the first 'signal period' MACD-line values.
       *  - Metastock compatibility: the fast and slow EMA are seeded
       *    from inReal[0], the signal EMA from the first MACD-line
       *    value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (an output == inReal) is supported: outputs at
       * [outIdx] are written only after inReal[startIdx+outIdx] was
       * read.
       */
      /* Seed each price EMA with a simple average of its first
       * 'period' price bars. The fast window is the tail of the
       * slow window: consume the leading slow-only bars first,
       * then accumulate both over the shared bars.
       */
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += inReal[today];
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;
      /* Advance both EMA through their unstable period, up to the
       * first MACD-line bar.
       */
      while( today <= startIdx - lookbackSignal ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
      /* Seed the signal EMA with a simple average of the first
       * 'signal period' MACD-line values, accumulated as they are
       * produced.
       */
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      /* Advance everything in lockstep through the unstable period
       * of the signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
      }
      /* Stable zone: keep advancing in lockstep and write the three
       * outputs.
       */
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   RetCode MACD_Impl( int startIdx,
                      int endIdx,
                      float inReal[],
                      int optInFastPeriod,
                      int optInSlowPeriod,
                      int optInSignalPeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outMACD[],
                      double outMACDSignal[],
                      double outMACDHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      if( optInSlowPeriod == 0 ) {
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = EMA_Lookback(optInSignalPeriod);
      lookbackTotal = lookbackSignal;
      lookbackTotal += EMA_Lookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += (double)inReal[today++];
      }
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += (double)inReal[today];
         tempReal += (double)inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;
      while( today <= startIdx - lookbackSignal ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      while( today <= startIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
      }
      outMACD[0] = macdValue;
      outMACDSignal[0] = prevSignal;
      outMACDHist[0] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = (double)inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
         outMACD[outIdx] = macdValue;
         outMACDSignal[outIdx] = prevSignal;
         outMACDHist[outIdx] = macdValue - prevSignal;
         outIdx += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.Success ;
   }
   /**
    * Moving Average Convergence/Divergence: the difference between a fast and a
    * slow EMA of the input, plus an EMA-smoothed signal line and their
    * histogram. MACD crossing its signal line and histogram sign changes flag
    * momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * MACD = EMA_fast - EMA_slow;  Signal = EMA(MACD, signalPeriod);  Hist = MACD - Signal
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one.</li>
    * <li>A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MACD_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series (typically close)
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 26; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outMACD Fast EMA minus slow EMA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDSignal EMA of the MACD line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDHist MACD minus signal line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#MACDEXT
    * @see Core#MACDFIX
    * @see Core#EMA
    * @see Core#APO
    */
   public OutRange MACD( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInFastPeriod,
                         int optInSlowPeriod,
                         int optInSignalPeriod,
                         double outMACD[],
                         double outMACDSignal[],
                         double outMACDHist[] )
   {
      requireIndexRange("MACD", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, MACD_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACD", "inReal", inReal, guardInLen);
      requireLength("MACD", "outMACD", outMACD, guardOutLen);
      requireLength("MACD", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACD", "outMACDHist", outMACDHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACD_Impl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Moving Average Convergence/Divergence: the difference between a fast and a
    * slow EMA of the input, plus an EMA-smoothed signal line and their
    * histogram. MACD crossing its signal line and histogram sign changes flag
    * momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * MACD = EMA_fast - EMA_slow;  Signal = EMA(MACD, signalPeriod);  Hist = MACD - Signal
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one.</li>
    * <li>A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MACD_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series (typically close)
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA (default 26; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outMACD Fast EMA minus slow EMA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDSignal EMA of the MACD line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDHist MACD minus signal line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
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
    * @see Core#MACDEXT
    * @see Core#MACDFIX
    * @see Core#EMA
    * @see Core#APO
    */
   public OutRange MACD( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInFastPeriod,
                         int optInSlowPeriod,
                         int optInSignalPeriod,
                         double outMACD[],
                         double outMACDSignal[],
                         double outMACDHist[] )
   {
      requireIndexRange("MACD", startIdx, endIdx);
      int guardStart = clampedStart(startIdx, endIdx, MACD_Lookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACD", "inReal", inReal, guardInLen);
      requireLength("MACD", "outMACD", outMACD, guardOutLen);
      requireLength("MACD", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACD", "outMACDHist", outMACDHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACD_Impl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACD", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MACD stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MACD} over the same series.
    * Open with {@link Core#MACD_Open}; there is no close — the handle is
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
   public static final class MACD_Stream {
      Core core;
      int optInFastPeriod;
      int optInSlowPeriod;
      int optInSignalPeriod;
      double prevFast;
      double prevSlow;
      double prevSignal;
      double slowK;
      double fastK;
      double signalK;
      double cur_outMACD;
      double cur_outMACDSignal;
      double cur_outMACDHist;
      Value cachedValue;
      OutRange fillRange = OutRange.EMPTY;

      MACD_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#MACD_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      MACD_Stream( MACD_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.prevFast = other.prevFast;
         this.prevSlow = other.prevSlow;
         this.prevSignal = other.prevSignal;
         this.slowK = other.slowK;
         this.fastK = other.fastK;
         this.signalK = other.signalK;
         this.cur_outMACD = other.cur_outMACD;
         this.cur_outMACDSignal = other.cur_outMACDSignal;
         this.cur_outMACDHist = other.cur_outMACDHist;
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      void copyFrom( MACD_Stream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.prevFast = other.prevFast;
         this.prevSlow = other.prevSlow;
         this.prevSignal = other.prevSignal;
         this.slowK = other.slowK;
         this.fastK = other.fastK;
         this.signalK = other.signalK;
         this.cur_outMACD = other.cur_outMACD;
         this.cur_outMACDSignal = other.cur_outMACDSignal;
         this.cur_outMACDHist = other.cur_outMACDHist;
         this.cachedValue = other.cachedValue;
         this.fillRange = other.fillRange;
      }

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param macd Fast EMA minus slow EMA.
       * @param macdSignal EMA of the MACD line.
       * @param macdHist MACD minus signal line.
       */
      public record Value(double macd, double macdSignal, double macdHist) { }

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
      public Value update( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MACD update: BadParam", RetCode.BadParam);
         core.MACD_StreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outMACD, this.cur_outMACDSignal, this.cur_outMACDHist);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a throwaway copy, which for this
       * handle's shape is cheaper than reusing one.
       */
      public Value peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MACD peek: BadParam", RetCode.BadParam);
         MACD_Stream scratch = new MACD_Stream(this);
         core.MACD_StreamStep(scratch, inReal);
         return new Value(scratch.cur_outMACD, scratch.cur_outMACDSignal, scratch.cur_outMACDHist);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public MACD_Stream copy() {
         return new MACD_Stream(this);
      }
   }
   void MACD_StreamStep( MACD_Stream sp, double inReal )
   {
      double macdValue = 0.0;
      double tempReal = 0.0;
      tempReal = inReal;
      sp.prevFast = Math.fma(tempReal - sp.prevFast, sp.fastK, sp.prevFast);
      sp.prevSlow = Math.fma(tempReal - sp.prevSlow, sp.slowK, sp.prevSlow);
      macdValue = sp.prevFast - sp.prevSlow;
      if( sp.optInSignalPeriod == 1 ) {
         sp.prevSignal = macdValue;
      } else {
         sp.prevSignal = Math.fma(macdValue - sp.prevSignal, sp.signalK, sp.prevSignal);
      }
      sp.cur_outMACD = macdValue;
      sp.cur_outMACDSignal = sp.prevSignal;
      sp.cur_outMACDHist = macdValue - sp.prevSignal;
   }
   private RetCode MACD_OpenPass( MACD_Stream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[], int outStride )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevSignal = 0;
      double macdValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Catch special case for fix 26/12 MACD.
       * Use hardcoded k values matching the original algorithm.
       */
      if( optInSlowPeriod == 0 ) {
         /* Fix 26 */
         optInSlowPeriod = 26;
         slowK = 0.075;
      } else {
         slowK = 2.0 / (double)(optInSlowPeriod + 1);
      }
      if( optInFastPeriod == 0 ) {
         /* Fix 12 */
         optInFastPeriod = 12;
         fastK = 0.15;
      } else {
         fastK = 2.0 / (double)(optInFastPeriod + 1);
      }
      /* A signal period of 1 disables signal-line smoothing: the signal IS the
       * MACD line and the histogram is exactly zero. signalK is then exactly
       * 1.0, so the recursion below reduces to (x-prev)+prev -- which returns x
       * only while consecutive MACD-line values stay within a factor of two of
       * each other. The MACD line oscillates through zero, so it leaves that
       * window on ordinary data; hence the explicit arm at each step.
       */
      signalK = 2.0 / (double)(optInSignalPeriod + 1);
      lookbackSignal = EMA_Lookback(optInSignalPeriod);
      /* Move up the start index if there is not
       * enough initial data.
       */
      lookbackTotal = lookbackSignal;
      lookbackTotal += EMA_Lookback(optInSlowPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Everything is computed in a single lockstep pass: each bar
       * advances the fast and slow EMA (two independent recursions),
       * their difference is the MACD line, and each MACD-line value
       * is immediately fed into the signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract
       * (do not reorder or fuse operations):
       *  - EMA recursion: ((x-prev)*k)+prev.
       *  - Default compatibility: each EMA is seeded with the sum of
       *    its first 'period' inputs, accumulated from 0.0 in input
       *    order, divided by the period. The fast and slow seed
       *    windows end on the same bar. The signal EMA is seeded the
       *    same way from the first 'signal period' MACD-line values.
       *  - Metastock compatibility: the fast and slow EMA are seeded
       *    from inReal[0], the signal EMA from the first MACD-line
       *    value.
       * Output alignment is identical for all compatibility modes;
       * only the seed values differ.
       *
       * In-place (an output == inReal) is supported: outputs at
       * [outIdx] are written only after inReal[startIdx+outIdx] was
       * read.
       */
      /* Seed each price EMA with a simple average of its first
       * 'period' price bars. The fast window is the tail of the
       * slow window: consume the leading slow-only bars first,
       * then accumulate both over the shared bars.
       */
      today = startIdx - lookbackTotal;
      tempReal = 0.0;
      i = optInSlowPeriod - optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += inReal[today++];
      }
      prevFast = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         prevFast += inReal[today];
         tempReal += inReal[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      prevFast = prevFast / optInFastPeriod;
      /* Advance both EMA through their unstable period, up to the
       * first MACD-line bar.
       */
      while( today <= startIdx - lookbackSignal ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
      }
      macdValue = prevFast - prevSlow;
      /* Seed the signal EMA with a simple average of the first
       * 'signal period' MACD-line values, accumulated as they are
       * produced.
       */
      prevSignal = 0.0;
      prevSignal += macdValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         prevSignal += macdValue;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      /* Advance everything in lockstep through the unstable period
       * of the signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
      }
      /* Stable zone: keep advancing in lockstep and write the three
       * outputs.
       */
      outMACD[0 * outStride] = macdValue;
      outMACDSignal[0 * outStride] = prevSignal;
      outMACDHist[0 * outStride] = macdValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempReal = inReal[today++];
         prevFast = Math.fma(tempReal - prevFast, fastK, prevFast);
         prevSlow = Math.fma(tempReal - prevSlow, slowK, prevSlow);
         macdValue = prevFast - prevSlow;
         if( optInSignalPeriod == 1 ) {
            prevSignal = macdValue;
         } else {
            prevSignal = Math.fma(macdValue - prevSignal, signalK, prevSignal);
         }
         outMACD[outIdx * outStride] = macdValue;
         outMACDSignal[outIdx * outStride] = prevSignal;
         outMACDHist[outIdx * outStride] = macdValue - prevSignal;
         outIdx += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.prevFast = prevFast;
      sp.prevSlow = prevSlow;
      sp.prevSignal = prevSignal;
      sp.slowK = slowK;
      sp.fastK = fastK;
      sp.signalK = signalK;
      sp.cur_outMACD = outMACD[(outNBElement.value - 1) * outStride];
      sp.cur_outMACDSignal = outMACDSignal[(outNBElement.value - 1) * outStride];
      sp.cur_outMACDHist = outMACDHist[(outNBElement.value - 1) * outStride];
      sp.cachedValue = new MACD_Stream.Value(sp.cur_outMACD, sp.cur_outMACDSignal, sp.cur_outMACDHist);
      return RetCode.Success;
   }
   private RetCode MACD_OpenImpl( MACD_Stream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMACD = new double[1];
      double[] sink_outMACDSignal = new double[1];
      double[] sink_outMACDHist = new double[1];
      return MACD_OpenPass( sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outMACD, sink_outMACDSignal, sink_outMACDHist, 0 );
   }
   private RetCode MACD_OpenAndFillImpl( MACD_Stream sp, double inReal[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      if( (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist ) {
         return RetCode.BadParam;
      }
      return MACD_OpenPass( sp, inReal, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1 );
   }
   private RetCode MACD_OpenAndFillInternalImpl( MACD_Stream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      return MACD_OpenPass(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1);
   }
   /* MACD_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MACD_Stream MACD_OpenAndFillInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      MACD_Stream sp = new MACD_Stream(this);
      RetCode retCode = MACD_OpenAndFillInternalImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MACD openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind MACD_Open (composition seam). */
   MACD_Stream MACD_OpenInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      MACD_Stream sp = new MACD_Stream(this);
      RetCode retCode = MACD_OpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACD open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACD open: internal error", retCode);
      }
      throw new TaLibArgumentException("MACD open: " + retCode, retCode);
   }
   /**
    * Open a live MACD stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MACD} at that bar.
    * <p>The history must hold at least {@code MACD_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MACD_Stream MACD_Open( double inReal[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      return MACD_OpenInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#MACD_Open} that also fills the output array(s) bit-identically
    * to {@link Core#MACD} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link MACD_Stream#fillRange()}.
    */
   public MACD_Stream MACD_OpenAndFill( double inReal[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      MACD_Stream sp = new MACD_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACD_OpenAndFillImpl(sp, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACD openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACD openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MACD openAndFill: " + retCode, retCode);
   }
