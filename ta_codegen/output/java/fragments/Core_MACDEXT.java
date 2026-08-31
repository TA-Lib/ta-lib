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
 *  010802 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070526 MF,CC  Speed optimization: delegate to the single-pass MACD
 *                when all three MA types are EMA (bit-exact).
 */

   /**
    * Number of leading input bars {@link Core#MACDEXT} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastMAType MA type for the fast MA (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowMAType MA type for the slow MA (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int MACDEXT_Lookback( int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
   {
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInFastMAType == MAType.DEFAULT ) {
         optInFastMAType = MAType.SMA;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowMAType == MAType.DEFAULT ) {
         optInSlowMAType = MAType.SMA;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      int tempInteger;
      int lookbackLargest;
      /* Find the MA with the largest lookback */
      lookbackLargest = MA_Lookback(optInFastPeriod, optInFastMAType);
      tempInteger = MA_Lookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      /* Add to the largest MA lookback the signal line lookback */
      return lookbackLargest + MA_Lookback(optInSignalPeriod, optInSignalMAType) ;

   }
   RetCode MACDEXT_Impl( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInFastPeriod,
                         MAType optInFastMAType,
                         int optInSlowPeriod,
                         MAType optInSlowMAType,
                         int optInSignalPeriod,
                         MAType optInSignalMAType,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outMACD[],
                         double outMACDSignal[],
                         double outMACDHist[] )
   {
      double[] slowMABuffer;
      double[] fastMABuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackLargest = 0;
      int i = 0;
      MAType tempMAType;
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
      if( optInFastMAType == MAType.DEFAULT ) {
         optInFastMAType = MAType.SMA;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowMAType == MAType.DEFAULT ) {
         optInSlowMAType = MAType.SMA;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      if( optInFastMAType == MAType.EMA && optInSlowMAType == MAType.EMA && optInSignalMAType == MAType.EMA && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2 ) {
         /* An all-EMA MACDEXT computes exactly what MACD computes. Delegate
          * to its single-pass implementation. Period 1 stays on the generic
          * path: ma() copies the input for it instead of running an EMA
          * recursion.
          *
          * This block is a batch-only specialization: the generator strips it
          * from the streaming tier, which composes the general three-MA path for
          * every parameter value. The two agreeing bit for bit is not assumed --
          * stream_verify's multi-enum diagonal selects all-EMA and holds this
          * block to the composed path (issue #181). Keep the comment INSIDE the
          * block: above it, the stream inherits it and reads as if it delegated.
          */
         OutRange _xr0 = MACD(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist);
         outBegIdx.value = _xr0.begIdx();
         outNBElement.value = _xr0.count();
         return RetCode.Success ;
      }
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap period */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
         /* swap type */
         tempMAType = optInSlowMAType;
         optInSlowMAType = optInFastMAType;
         optInFastMAType = tempMAType;
      }
      /* Find the MA with the largest lookback */
      lookbackLargest = MA_Lookback(optInFastPeriod, optInFastMAType);
      tempInteger = MA_Lookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      /* Add the lookback needed for the signal line */
      lookbackSignal = MA_Lookback(optInSignalPeriod, optInSignalMAType);
      lookbackTotal = lookbackSignal + lookbackLargest;
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
         return RetCode.Success ;
      }
      /* Allocate intermediate buffer for fast/slow MA. */
      tempInteger = endIdx - startIdx + 1 + lookbackSignal;
      fastMABuffer = new double[(int)(tempInteger * 1)];
      slowMABuffer = new double[(int)(tempInteger * 1)];
      /* Calculate the slow MA.
       *
       * Move back the startIdx to get enough data
       * for the signal period. That way, once the
       * signal calculation is done, all the output
       * will start at the requested 'startIdx'.
       */
      tempInteger = startIdx - lookbackSignal;
      OutRange _xr1 = MA(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, slowMABuffer);
      outBegIdx1.value = _xr1.begIdx();
      outNbElement1.value = _xr1.count();
      retCode = RetCode.Success;
      /* Calculate the fast MA. */
      OutRange _xr2 = MA(tempInteger, endIdx, inReal, optInFastPeriod, optInFastMAType, fastMABuffer);
      outBegIdx2.value = _xr2.begIdx();
      outNbElement2.value = _xr2.count();
      retCode = RetCode.Success;
      /* Parano tests. Will be removed eventually. */
      if( outBegIdx1.value != tempInteger || outBegIdx2.value != tempInteger || outNbElement1.value != outNbElement2.value || outNbElement1.value != endIdx - startIdx + 1 + lookbackSignal ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      /* Calculate (fast MA) - (slow MA). */
      for( i = 0; i < outNbElement1.value; i += 1 ) {
         fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
      }
      /* Copy the result into the output for the caller. */
      /* memmove, not memcpy: fastMABuffer aliases outMACD when the caller buffer is
       * reused as scratch, so source and destination overlap (issue #94).
       */
      System.arraycopy(fastMABuffer, lookbackSignal, outMACD, 0, (endIdx - startIdx + 1) * 1);
      /* Calculate the signal/trigger line. */
      OutRange _xr3 = MA(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outMACDSignal);
      outBegIdx2.value = _xr3.begIdx();
      outNbElement2.value = _xr3.count();
      retCode = RetCode.Success;
      /* Calculate the histogram. */
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         outMACDHist[i] = outMACD[i] - outMACDSignal[i];
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      return RetCode.Success ;
   }
   RetCode MACDEXT_Impl( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInFastPeriod,
                         MAType optInFastMAType,
                         int optInSlowPeriod,
                         MAType optInSlowMAType,
                         int optInSignalPeriod,
                         MAType optInSignalMAType,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outMACD[],
                         double outMACDSignal[],
                         double outMACDHist[] )
   {
      double[] slowMABuffer;
      double[] fastMABuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackLargest = 0;
      int i = 0;
      MAType tempMAType;
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
      if( optInFastMAType == MAType.DEFAULT ) {
         optInFastMAType = MAType.SMA;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowMAType == MAType.DEFAULT ) {
         optInSlowMAType = MAType.SMA;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      if( optInFastMAType == MAType.EMA && optInSlowMAType == MAType.EMA && optInSignalMAType == MAType.EMA && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2 ) {
         OutRange _xr0 = MACD(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outMACD, outMACDSignal, outMACDHist);
         outBegIdx.value = _xr0.begIdx();
         outNBElement.value = _xr0.count();
         return RetCode.Success ;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
         tempMAType = optInSlowMAType;
         optInSlowMAType = optInFastMAType;
         optInFastMAType = tempMAType;
      }
      lookbackLargest = MA_Lookback(optInFastPeriod, optInFastMAType);
      tempInteger = MA_Lookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      lookbackSignal = MA_Lookback(optInSignalPeriod, optInSignalMAType);
      lookbackTotal = lookbackSignal + lookbackLargest;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      tempInteger = endIdx - startIdx + 1 + lookbackSignal;
      fastMABuffer = new double[(int)(tempInteger * 1)];
      slowMABuffer = new double[(int)(tempInteger * 1)];
      tempInteger = startIdx - lookbackSignal;
      OutRange _xr1 = MA(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, slowMABuffer);
      outBegIdx1.value = _xr1.begIdx();
      outNbElement1.value = _xr1.count();
      retCode = RetCode.Success;
      OutRange _xr2 = MA(tempInteger, endIdx, inReal, optInFastPeriod, optInFastMAType, fastMABuffer);
      outBegIdx2.value = _xr2.begIdx();
      outNbElement2.value = _xr2.count();
      retCode = RetCode.Success;
      if( outBegIdx1.value != tempInteger || outBegIdx2.value != tempInteger || outNbElement1.value != outNbElement2.value || outNbElement1.value != endIdx - startIdx + 1 + lookbackSignal ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      for( i = 0; i < outNbElement1.value; i += 1 ) {
         fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
      }
      System.arraycopy(fastMABuffer, lookbackSignal, outMACD, 0, (endIdx - startIdx + 1) * 1);
      OutRange _xr3 = MA(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outMACDSignal);
      outBegIdx2.value = _xr3.begIdx();
      outNbElement2.value = _xr3.count();
      retCode = RetCode.Success;
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         outMACDHist[i] = outMACD[i] - outMACDSignal[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      return RetCode.Success ;
   }
   /**
    * MACD variant where the fast, slow, and signal moving averages each use a
    * user-selectable MA type. Outputs the MACD line, its signal line, and their
    * difference (histogram). Hist sign change (MACD crossing its signal line)
    * flags momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * MACD = MA_fast(inReal) - MA_slow(inReal)
    * Signal = MA_signal(MACD)
    * Hist = MACD - Signal
    * (each MA_* uses its own MA type and period)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>If the slow period is set smaller than the fast period, the fast and slow periods and their MA types are swapped so the slow moving average is always the longer one.</li>
    * <li>A signal period of 1 disables signal-line smoothing for every signal MAType: the signal equals the MACD line and the histogram is zero.</li>
    * <li>{@code TA_MAType_MAMA} ignores its period argument, so it always produces the same series regardless of the period requested. If both {@code optInFastMAType} and {@code optInSlowMAType} are set to MAMA, the fast and slow lines are therefore identical and MACD, Signal, and Hist are all zero at every bar. Select MAMA for only one side to get a meaningful spread.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MACDEXT_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastMAType MA type for the fast MA (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowMAType MA type for the slow MA (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outMACD MACD line: fast MA minus slow MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDSignal Signal line: MA of the MACD line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDHist Histogram: MACD minus signal. Must hold at least
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
    * @see Core#MACD
    * @see Core#MACDFIX
    * @see Core#MA
    * @see Core#EMA
    * @see Core#APO
    * @see Core#PPO
    */
   public OutRange MACDEXT( int startIdx,
                            int endIdx,
                            double inReal[],
                            int optInFastPeriod,
                            MAType optInFastMAType,
                            int optInSlowPeriod,
                            MAType optInSlowMAType,
                            int optInSignalPeriod,
                            MAType optInSignalMAType,
                            double outMACD[],
                            double outMACDSignal[],
                            double outMACDHist[] )
   {
      requireIndexRange("MACDEXT", startIdx, endIdx);
      requireArgument("MACDEXT", "optInFastMAType", optInFastMAType);
      requireArgument("MACDEXT", "optInSlowMAType", optInSlowMAType);
      requireArgument("MACDEXT", "optInSignalMAType", optInSignalMAType);
      int guardStart = clampedStart("MACDEXT", startIdx, MACDEXT_Lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACDEXT", "inReal", inReal, guardInLen);
      requireLength("MACDEXT", "outMACD", outMACD, guardOutLen);
      requireLength("MACDEXT", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACDEXT", "outMACDHist", outMACDHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACDEXT_Impl(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACDEXT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * MACD variant where the fast, slow, and signal moving averages each use a
    * user-selectable MA type. Outputs the MACD line, its signal line, and their
    * difference (histogram). Hist sign change (MACD crossing its signal line)
    * flags momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * MACD = MA_fast(inReal) - MA_slow(inReal)
    * Signal = MA_signal(MACD)
    * Hist = MACD - Signal
    * (each MA_* uses its own MA type and period)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>If the slow period is set smaller than the fast period, the fast and slow periods and their MA types are swapped so the slow moving average is always the longer one.</li>
    * <li>A signal period of 1 disables signal-line smoothing for every signal MAType: the signal equals the MACD line and the histogram is zero.</li>
    * <li>{@code TA_MAType_MAMA} ignores its period argument, so it always produces the same series regardless of the period requested. If both {@code optInFastMAType} and {@code optInSlowMAType} are set to MAMA, the fast and slow lines are therefore identical and MACD, Signal, and Hist are all zero at every bar. Select MAMA for only one side to get a meaningful spread.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#MACDEXT_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastMAType MA type for the fast MA (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowMAType MA type for the slow MA (default 0 = SMA; values:
    *        0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA, 8=T3, 9=HMA,
    *        10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the default).
    * @param optInSignalPeriod Period of the signal-line MA (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSignalMAType MA type for the signal line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outMACD MACD line: fast MA minus slow MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDSignal Signal line: MA of the MACD line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outMACDHist Histogram: MACD minus signal. Must hold at least
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
    * @see Core#MACD
    * @see Core#MACDFIX
    * @see Core#MA
    * @see Core#EMA
    * @see Core#APO
    * @see Core#PPO
    */
   public OutRange MACDEXT( int startIdx,
                            int endIdx,
                            float inReal[],
                            int optInFastPeriod,
                            MAType optInFastMAType,
                            int optInSlowPeriod,
                            MAType optInSlowMAType,
                            int optInSignalPeriod,
                            MAType optInSignalMAType,
                            double outMACD[],
                            double outMACDSignal[],
                            double outMACDHist[] )
   {
      requireIndexRange("MACDEXT", startIdx, endIdx);
      requireArgument("MACDEXT", "optInFastMAType", optInFastMAType);
      requireArgument("MACDEXT", "optInSlowMAType", optInSlowMAType);
      requireArgument("MACDEXT", "optInSignalMAType", optInSignalMAType);
      int guardStart = clampedStart("MACDEXT", startIdx, MACDEXT_Lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACDEXT", "inReal", inReal, guardInLen);
      requireLength("MACDEXT", "outMACD", outMACD, guardOutLen);
      requireLength("MACDEXT", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACDEXT", "outMACDHist", outMACDHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = MACDEXT_Impl(startIdx, endIdx, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode != RetCode.Success ) {
         throw failure("MACDEXT", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MACDEXT stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#MACDEXT} over the same series.
    * Open with {@link Core#macdextOpen}; there is no close — the handle is
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
   public static final class MacdextStream {
      Core core;
      int optInFastPeriod;
      MAType optInFastMAType;
      int optInSlowPeriod;
      MAType optInSlowMAType;
      int optInSignalPeriod;
      MAType optInSignalMAType;
      double cur_outMACD;
      double cur_outMACDSignal;
      double cur_outMACDHist;
      Value cachedValue;
      MaStream sub0;
      MaStream sub1;
      MaStream sub2;
      int outRangeBegIdx;
      int outRangeCount;

      MacdextStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#MACDEXT} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      MacdextStream( MacdextStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInFastMAType = other.optInFastMAType;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSlowMAType = other.optInSlowMAType;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.optInSignalMAType = other.optInSignalMAType;
         this.cur_outMACD = other.cur_outMACD;
         this.cur_outMACDSignal = other.cur_outMACDSignal;
         this.cur_outMACDHist = other.cur_outMACDHist;
         this.cachedValue = other.cachedValue;
         this.sub0 = new MaStream(other.sub0);
         this.sub1 = new MaStream(other.sub1);
         this.sub2 = new MaStream(other.sub2);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param macd MACD line: fast MA minus slow MA.
       * @param macdSignal Signal line: MA of the MACD line.
       * @param macdHist Histogram: MACD minus signal.
       */
      public record Value(double macd, double macdSignal, double macdHist) { }

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
      public Value update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("MACDEXT update: BadParam", RetCode.BadParam);
         }
         core.macdextStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         this.cachedValue = new Value(this.cur_outMACD, this.cur_outMACDSignal, this.cur_outMACDHist);
         return this.cachedValue;
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
      public void updateAndFill( double inReal[], double outMACD[], double outMACDSignal[], double outMACDHist[] ) {
         requireArgument("MACDEXT updateAndFill", "inReal", inReal);
         requireArgument("MACDEXT updateAndFill", "outMACD", outMACD);
         requireArgument("MACDEXT updateAndFill", "outMACDSignal", outMACDSignal);
         requireArgument("MACDEXT updateAndFill", "outMACDHist", outMACDHist);
         final int barCount = inReal.length;
         if( outMACD.length < barCount || outMACDSignal.length < barCount || outMACDHist.length < barCount || (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist )
            throw new TaLibArgumentException("MACDEXT updateAndFill: BadParam", RetCode.BadParam);
         int done = 0;
         try {
            for( int i = 0; i < barCount; i++ ) {
               if( !Double.isFinite(inReal[i]) ) {
                  if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
                  throw new TaLibArgumentException("MACDEXT updateAndFill: BadParam", RetCode.BadParam);
               }
               core.macdextStepImpl(this, inReal[i]);
               outMACD[i] = this.cur_outMACD;
               outMACDSignal[i] = this.cur_outMACDSignal;
               outMACDHist[i] = this.cur_outMACDHist;
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               done = i + 1;
            }
         } finally {
            if( done > 0 ) this.cachedValue = new Value(this.cur_outMACD, this.cur_outMACDSignal, this.cur_outMACDHist);
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies no buffer: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period. It does allocate a small bounded amount
       * per call — a size fixed by the indicator, never by the period.
       */
      public Value peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("MACDEXT peek: BadParam", RetCode.BadParam);
         MacdextStream sp = this;
         double cur_slowMABuffer = 0.0;
         double cur_fastMABuffer = 0.0;
         double cur_outMACDSignal = 0.0;
         double cur_outMACDHist = 0.0;
         double cur_outMACD = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_slowMABuffer = sp.sub0.peek(inReal);
         cur_fastMABuffer = sp.sub1.peek(inReal);
         /* Combine map (batch tail, per bar). */
         cur_fastMABuffer = cur_fastMABuffer - cur_slowMABuffer;
         cur_outMACDSignal = sp.sub2.peek(cur_fastMABuffer);
         /* Combine map (batch tail, per bar). */
         cur_outMACDHist = cur_fastMABuffer - cur_outMACDSignal;
         cur_outMACD = cur_fastMABuffer;
         return new Value(cur_outMACD, cur_outMACDSignal, cur_outMACDHist);
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
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
      public MacdextStream clone() {
         return new MacdextStream(this);
      }
   }
   void macdextStepImpl( MacdextStream sp, double inReal )
   {
      double cur_slowMABuffer = 0.0;
      double cur_fastMABuffer = 0.0;
      double cur_outMACDSignal = 0.0;
      double cur_outMACDHist = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_slowMABuffer = sp.sub0.update(inReal);
      cur_fastMABuffer = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      cur_fastMABuffer = cur_fastMABuffer - cur_slowMABuffer;
      cur_outMACDSignal = sp.sub2.update(cur_fastMABuffer);
      /* Combine map (batch tail, per bar). */
      cur_outMACDHist = cur_fastMABuffer - cur_outMACDSignal;
      sp.cur_outMACD = cur_fastMABuffer;
      sp.cur_outMACDSignal = cur_outMACDSignal;
      sp.cur_outMACDHist = cur_outMACDHist;
   }
   private RetCode macdextOpenImpl( MacdextStream sp, double inReal[], int startIdx, int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[], int outStride )
   {
      double[] slowMABuffer;
      double[] fastMABuffer;
      RetCode retCode;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackLargest = 0;
      int i = 0;
      MAType tempMAType;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastMAType == MAType.DEFAULT ) {
         optInFastMAType = MAType.SMA;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowMAType == MAType.DEFAULT ) {
         optInSlowMAType = MAType.SMA;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSignalMAType == MAType.DEFAULT ) {
         optInSignalMAType = MAType.SMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < MACDEXT_Lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outMACD = outStride == 1 ? outMACD : new double[historyLen];
      double[] sc_outMACDSignal = outStride == 1 ? outMACDSignal : new double[historyLen];
      double[] sc_outMACDHist = outStride == 1 ? outMACDHist : new double[historyLen];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap period */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
         /* swap type */
         tempMAType = optInSlowMAType;
         optInSlowMAType = optInFastMAType;
         optInFastMAType = tempMAType;
      }
      /* Find the MA with the largest lookback */
      lookbackLargest = MA_Lookback(optInFastPeriod, optInFastMAType);
      tempInteger = MA_Lookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      /* Add the lookback needed for the signal line */
      lookbackSignal = MA_Lookback(optInSignalPeriod, optInSignalMAType);
      lookbackTotal = lookbackSignal + lookbackLargest;
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
         return RetCode.InsufficientHistory ;
      }
      /* Allocate intermediate buffer for fast/slow MA. */
      tempInteger = endIdx - startIdx + 1 + lookbackSignal;
      fastMABuffer = new double[(int)(tempInteger * 1)];
      slowMABuffer = new double[(int)(tempInteger * 1)];
      /* Calculate the slow MA.
       *
       * Move back the startIdx to get enough data
       * for the signal period. That way, once the
       * signal calculation is done, all the output
       * will start at the requested 'startIdx'.
       */
      tempInteger = startIdx - lookbackSignal;
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(inReal, tempInteger, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      retCode = RetCode.Success;
      /* Calculate the fast MA. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub1 = maOpenAndFillInternal(inReal, tempInteger, optInFastPeriod, optInFastMAType, outBegIdx2, outNbElement2, fastMABuffer);
      retCode = RetCode.Success;
      /* Parano tests. Will be removed eventually. */
      if( outBegIdx1.value != tempInteger || outBegIdx2.value != tempInteger || outNbElement1.value != outNbElement2.value || outNbElement1.value != endIdx - startIdx + 1 + lookbackSignal ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      /* Calculate (fast MA) - (slow MA). */
      for( i = 0; i < outNbElement1.value; i += 1 ) {
         fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
      }
      /* Copy the result into the output for the caller. */
      /* memmove, not memcpy: fastMABuffer aliases outMACD when the caller buffer is
       * reused as scratch, so source and destination overlap (issue #94).
       */
      System.arraycopy(fastMABuffer, lookbackSignal, sc_outMACD, 0, (endIdx - startIdx + 1) * 1);
      /* Calculate the signal/trigger line. */
      /* Sub-stream 2: ma over `fastMABuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub2 = maOpenAndFillInternal(java.util.Arrays.copyOfRange(fastMABuffer, 0, (outNbElement1.value - 1) + 1), 0, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, sc_outMACDSignal);
      retCode = RetCode.Success;
      /* Calculate the histogram. */
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         sc_outMACDHist[i] = sc_outMACD[i] - sc_outMACDSignal[i];
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInFastMAType = optInFastMAType;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInSlowMAType = optInSlowMAType;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.optInSignalMAType = optInSignalMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.cur_outMACD = sc_outMACD[outNBElement.value - 1];
      sp.cur_outMACDSignal = sc_outMACDSignal[outNBElement.value - 1];
      sp.cur_outMACDHist = sc_outMACDHist[outNBElement.value - 1];
      sp.cachedValue = new MacdextStream.Value(sp.cur_outMACD, sp.cur_outMACDSignal, sp.cur_outMACDHist);
      return RetCode.Success;
   }
   /* macdextOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MacdextStream macdextOpenAndFillInternal( double inReal[], int startIdx, int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      MacdextStream sp = new MacdextStream(this);
      RetCode retCode = macdextOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACDEXT openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACDEXT openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("MACDEXT openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind macdextOpen (composition seam). */
   MacdextStream macdextOpenInternal( double inReal[], int startIdx, int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
   {
      MacdextStream sp = new MacdextStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMACD = new double[1];
      double[] sink_outMACDSignal = new double[1];
      double[] sink_outMACDHist = new double[1];
      RetCode retCode = macdextOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, sink_outMACD, sink_outMACDSignal, sink_outMACDHist, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("MACDEXT open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("MACDEXT open: internal error", retCode);
      }
      throw new TaLibArgumentException("MACDEXT open: " + retCode, retCode);
   }
   /**
    * Open a live MACDEXT stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#MACDEXT} at that bar.
    * <p>The history must hold at least {@code MACDEXT_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public MacdextStream macdextOpen( double inReal[], int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
   {
      requireArgument("MACDEXT open", "inReal", inReal);
      requireHistory("MACDEXT open", inReal.length);
      requireArgument("MACDEXT open", "optInFastMAType", optInFastMAType);
      requireArgument("MACDEXT open", "optInSlowMAType", optInSlowMAType);
      requireArgument("MACDEXT open", "optInSignalMAType", optInSignalMAType);
      return macdextOpenInternal(inReal, 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
   }
   /**
    * {@link Core#macdextOpen} that also fills the output array(s) bit-identically
    * to {@link Core#MACDEXT} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MacdextStream#outRange()}.
    */
   public MacdextStream macdextOpenAndFill( double inReal[], int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      requireArgument("MACDEXT openAndFill", "inReal", inReal);
      requireHistory("MACDEXT openAndFill", inReal.length);
      requireArgument("MACDEXT openAndFill", "optInFastMAType", optInFastMAType);
      requireArgument("MACDEXT openAndFill", "optInSlowMAType", optInSlowMAType);
      requireArgument("MACDEXT openAndFill", "optInSignalMAType", optInSignalMAType);
      int guardOutLen = openFillCount("MACDEXT openAndFill", inReal.length, MACDEXT_Lookback(optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType));
      requireLength("MACDEXT openAndFill", "outMACD", outMACD, guardOutLen);
      requireLength("MACDEXT openAndFill", "outMACDSignal", outMACDSignal, guardOutLen);
      requireLength("MACDEXT openAndFill", "outMACDHist", outMACDHist, guardOutLen);
      if( (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist ) {
         throw new TaLibArgumentException("MACDEXT openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return macdextOpenAndFillInternal(inReal, 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
   }
