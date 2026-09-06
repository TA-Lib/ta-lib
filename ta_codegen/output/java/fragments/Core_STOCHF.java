/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  EKO      echo999@ifrance.com
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  010802 MF    Template creation.
 *  051103 EKO   Found bug and fix related to outFastD.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  071026 MF,CC Fix #107. Guard the Fast-K division with TA_IS_ZERO, not an
 *               exact `diff != 0.0`, so a machine-flat window yields 0 instead
 *               of dividing a sub-epsilon residue into [0,100] noise (STOCHRSI).
 *  072026 MF,CC Fix #130. Never elect outFastD as the K scratch buffer: %D's
 *               in-place ma() destroyed the raw K before the final copy.
 *  082326 MF,CC Fix #253. Scale that guard to the window's own extremes: the
 *               fixed band zeroed the whole output for any instrument quoted
 *               small enough to fall under it.
 *  082726 MF,CC Drop the dead retCode block after the copy: the rejection is
 *               already answered above it, and the shape reads like #269.
 *  090626 MF,CC Fix #390. Divide by the range, scale after: the hoisted
 *               `(highest-lowest)/100.0` underflowed to 0.0 on a denormal
 *               range that the guard still called "not flat".
 */

   /**
    * Number of leading input bars {@link Core#STOCHF} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastK_Period Lookback window for the highest-high/lowest-low
    *        of Fast-K (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInFastD_Period Smoothing period for the Fast-D line (default 3;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastD_MAType Moving-average type used to smooth Fast-D
    *        (default 0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA,
    *        6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int STOCHF_Lookback( int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return -1;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      int retValue;
      /* Account for the initial data needed for Fast-K. */
      retValue = optInFastK_Period - 1;
      /* Add the smoothing being done for Fast-D */
      retValue += MA_Lookback(optInFastD_Period, optInFastD_MAType);
      return retValue ;

   }
   RetCode STOCHF_Impl( int startIdx,
                        int endIdx,
                        double inHigh[],
                        double inLow[],
                        double inClose[],
                        int optInFastK_Period,
                        int optInFastD_Period,
                        MAType optInFastD_MAType,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outFastK[],
                        double outFastD[] )
   {
      RetCode retCode;
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double[] tempBuffer;
      int outIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int lookbackTotal = 0;
      int lookbackK = 0;
      int lookbackFastD = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int bufferIsAllocated = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      i = 0;
      /* With stochastic, there is a total of 4 different lines that
       * are defined: FASTK, FASTD, SLOWK and SLOWD.
       *
       * The D is the signal line usually drawn over its
       * corresponding K function.
       *
       *                    (Today's Close - LowestLow)
       *  FASTK(Kperiod) =  --------------------------- * 100
       *                     (HighestHigh - LowestLow)
       *
       *  FASTD(FastDperiod, MA type) = MA Smoothed FASTK over FastDperiod
       *
       *  SLOWK(SlowKperiod, MA type) = MA Smoothed FASTK over SlowKperiod
       *
       *  SLOWD(SlowDperiod, MA Type) = MA Smoothed SLOWK over SlowDperiod
       *
       * The HighestHigh and LowestLow are the extreme values among the
       * last 'Kperiod'.
       *
       * SLOWK and FASTD are equivalent when using the same period.
       *
       * The following shows how these four lines are made available in TA-LIB:
       *
       *  TA_STOCH  : Returns the SLOWK and SLOWD
       *  TA_STOCHF : Returns the FASTK and FASTD
       *
       * The TA_STOCH function correspond to the more widely implemented version
       * found in many software/charting package. The TA_STOCHF is more rarely
       * used because its higher volatility cause often whipsaws.
       */
      /* Identify the lookback needed. */
      lookbackK = optInFastK_Period - 1;
      lookbackFastD = MA_Lookback(optInFastD_Period, optInFastD_MAType);
      lookbackTotal = lookbackK + lookbackFastD;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         /* Succeed... but no data in the output. */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      /* Do the K calculation:
       *
       *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
       *
       * Kt is today stochastic
       * Ct is today closing price.
       * Lt is the lowest price of the last K Period (including today)
       * Ht is the highest price of the last K Period (including today)
       */
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      /* Calculate just enough K for ending up with the caller
       * requested range. (The range of k must consider all
       * the lookback involve with the smoothing).
       */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + lookbackK;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      /* Allocate a temporary buffer large enough to
       * store the K.
       *
       * When outFastK aliases a price input the caller buffer doubles as the
       * scratch, saving one allocation: the K writes trail the min/max window
       * reads, and the final memmove is overlap-safe. outFastD must NOT be
       * elected: the %D ma() below would then run in place over the raw K
       * that the memmove into outFastK still needs (issue #130).
       */
      bufferIsAllocated = 0;
      if( outFastK == inHigh || outFastK == inLow || outFastK == inClose ) {
         tempBuffer = outFastK;
      } else {
         bufferIsAllocated = 1;
         tempBuffer = new double[(int)((endIdx - today + 1) * 1)];
      }
      /* Do the K calculation */
      while( today <= endIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Divide by the range itself and scale after: the guard has to test the
          * very expression the division uses, or a scaling step can carry a
          * guarded-non-zero into a zero divisor.
          *
          * The band is the range against ITS OWN two extremes, not a fixed
          * constant: the range carries the quote unit, so a constant answers
          * "flat" for every window of an instrument quoted below it (issue #253).
          * It absorbs the machine-flat window an exact test would divide into
          * [0,100] noise (issue #107 / STOCHRSI).
          */
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            tempBuffer[outIdx++] = (inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            tempBuffer[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      /* Fast-K calculation completed. This K calculation is returned
       * to the caller. It is smoothed to become Fast-D.
       */
      OutRange _xr0 = MA(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outFastD);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         /* Something wrong happen? No further data? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Copy tempBuffer into the caller buffer.
       * (Calculation could not be done directly in the
       *  caller buffer because more input data then the
       *  requested range was needed for doing %D).
       */
      /* memmove, not memcpy: tempBuffer aliases outFastK when the caller buffer is
       * reused as scratch, so source and destination overlap (issue #94).
       */
      System.arraycopy(tempBuffer, lookbackFastD, outFastK, 0, (int)outNBElement.value * 1);
      /* Note: Keep the outBegIdx relative to the
       *       caller input before returning.
       */
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode STOCHF_Impl( int startIdx,
                        int endIdx,
                        float inHigh[],
                        float inLow[],
                        float inClose[],
                        int optInFastK_Period,
                        int optInFastD_Period,
                        MAType optInFastD_MAType,
                        MInteger outBegIdx,
                        MInteger outNBElement,
                        double outFastK[],
                        double outFastD[] )
   {
      RetCode retCode;
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double[] tempBuffer;
      int outIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int lookbackTotal = 0;
      int lookbackK = 0;
      int lookbackFastD = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int bufferIsAllocated = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      i = 0;
      lookbackK = optInFastK_Period - 1;
      lookbackFastD = MA_Lookback(optInFastD_Period, optInFastD_MAType);
      lookbackTotal = lookbackK + lookbackFastD;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + lookbackK;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      bufferIsAllocated = 0;
      if( false || false || false ) {
         tempBuffer = outFastK;
      } else {
         bufferIsAllocated = 1;
         tempBuffer = new double[(int)((endIdx - today + 1) * 1)];
      }
      while( today <= endIdx ) {
         tmp = (double)inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = (double)inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = (double)inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         tmp = (double)inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = (double)inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = (double)inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            tempBuffer[outIdx++] = ((double)inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            tempBuffer[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      OutRange _xr0 = MA(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outFastD);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      System.arraycopy(tempBuffer, lookbackFastD, outFastK, 0, (int)outNBElement.value * 1);
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Fast Stochastic Oscillator: the raw %K line and its
    * moving-average-smoothed %D line. Unlike STOCH (which slows both lines),
    * STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; &gt;80
    * overbought, &lt;20 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * FastK = 100 * (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
    * FastD = MA(FastK, FastD_Period, FastD_MAType)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the high-low range over the window is zero, %K is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STOCHF_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastK_Period Lookback window for the highest-high/lowest-low
    *        of Fast-K (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInFastD_Period Smoothing period for the Fast-D line (default 3;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastD_MAType Moving-average type used to smooth Fast-D
    *        (default 0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA,
    *        6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outFastK Raw %K stochastic line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outFastD MA-smoothed %K (signal line) Must hold at least
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
    * @see Core#STOCH
    * @see Core#STOCHRSI
    * @see Core#MA
    */
   public OutRange STOCHF( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           int optInFastK_Period,
                           int optInFastD_Period,
                           MAType optInFastD_MAType,
                           double outFastK[],
                           double outFastD[] )
   {
      requireIndexRange("STOCHF", startIdx, endIdx);
      requireArgument("STOCHF", "optInFastD_MAType", optInFastD_MAType);
      int guardStart = clampedStart("STOCHF", startIdx, STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STOCHF", "inHigh", inHigh, guardInLen);
      requireLength("STOCHF", "inLow", inLow, guardInLen);
      requireLength("STOCHF", "inClose", inClose, guardInLen);
      requireLength("STOCHF", "outFastK", outFastK, guardOutLen);
      requireLength("STOCHF", "outFastD", outFastD, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHF_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCHF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Fast Stochastic Oscillator: the raw %K line and its
    * moving-average-smoothed %D line. Unlike STOCH (which slows both lines),
    * STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; &gt;80
    * overbought, &lt;20 oversold.
    * <p><b>Formula</b>
    * <pre>{@code
    * FastK = 100 * (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
    * FastD = MA(FastK, FastD_Period, FastD_MAType)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the high-low range over the window is zero, %K is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STOCHF_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastK_Period Lookback window for the highest-high/lowest-low
    *        of Fast-K (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects
    *        the default).
    * @param optInFastD_Period Smoothing period for the Fast-D line (default 3;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInFastD_MAType Moving-average type used to smooth Fast-D
    *        (default 0 = SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA,
    *        6=KAMA, 7=MAMA, 8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outFastK Raw %K stochastic line. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outFastD MA-smoothed %K (signal line) Must hold at least
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
    * @see Core#STOCH
    * @see Core#STOCHRSI
    * @see Core#MA
    */
   public OutRange STOCHF( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           int optInFastK_Period,
                           int optInFastD_Period,
                           MAType optInFastD_MAType,
                           double outFastK[],
                           double outFastD[] )
   {
      requireIndexRange("STOCHF", startIdx, endIdx);
      requireArgument("STOCHF", "optInFastD_MAType", optInFastD_MAType);
      int guardStart = clampedStart("STOCHF", startIdx, STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STOCHF", "inHigh", inHigh, guardInLen);
      requireLength("STOCHF", "inLow", inLow, guardInLen);
      requireLength("STOCHF", "inClose", inClose, guardInLen);
      requireLength("STOCHF", "outFastK", outFastK, guardOutLen);
      requireLength("STOCHF", "outFastD", outFastD, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCHF_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCHF", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STOCHF stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#STOCHF} over the same series.
    * Open with {@link Core#stochfOpen}; there is no close — the handle is
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
   public static final class StochfStream {
      Core core;
      int optInFastK_Period;
      int optInFastD_Period;
      MAType optInFastD_MAType;
      double lowest;
      double highest;
      int lowestIdx;
      int highestIdx;
      int trailingIdx;
      int i;
      int today;
      int xMask;
      double[] x_inHigh;
      double[] x_inLow;
      double[] x_inClose;
      double cur_outFastK;
      double cur_outFastD;
      MaStream sub0;
      int outRangeBegIdx;
      int outRangeCount;

      StochfStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#STOCHF} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value(StochfOut)} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       */
      public void advance() { if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++; }

      StochfStream( StochfStream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInFastD_Period = other.optInFastD_Period;
         this.optInFastD_MAType = other.optInFastD_MAType;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.x_inClose = other.x_inClose.clone();
         this.cur_outFastK = other.cur_outFastK;
         this.cur_outFastD = other.cur_outFastD;
         this.sub0 = new MaStream(other.sub0);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(StochfOut)} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, StochfOut out ) {
         requireArgument("STOCHF update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("STOCHF update: BadParam", RetCode.BadParam);
         core.stochfStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.fastK = this.cur_outFastK;
         out.fastD = this.cur_outFastD;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public void peek( double inHigh, double inLow, double inClose, StochfOut out ) {
         requireArgument("STOCHF peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("STOCHF peek: BadParam", RetCode.BadParam);
         StochfStream sp = this;
         double cur_tempBuffer = 0.0;
         double cur_outFastD = 0.0;
         double cur_outFastK = 0.0;
         double tmp = 0.0;
         double highest = sp.highest;
         int highestIdx = sp.highestIdx;
         int i = sp.i;
         double lowest = sp.lowest;
         int lowestIdx = sp.lowestIdx;
         int today = sp.today;
         int trailingIdx = sp.trailingIdx;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         int pkSlot1 = -1;
         double pkVal1 = 0.0;
         int pkSlot2 = -1;
         double pkVal2 = 0.0;
         if( today >= 1073741824 ) {
            int rebaseShift = trailingIdx & ~sp.xMask;
            today -= rebaseShift;
            trailingIdx -= rebaseShift;
            highestIdx -= rebaseShift;
            i -= rebaseShift;
            lowestIdx -= rebaseShift;
         }
         pkSlot0 = today & sp.xMask;
         pkVal0 = inHigh;
         pkSlot1 = today & sp.xMask;
         pkVal1 = inLow;
         pkSlot2 = today & sp.xMask;
         pkVal2 = inClose;
         /* Set the lowest low */
         tmp = ((today & sp.xMask) != pkSlot1) ? sp.x_inLow[today & sp.xMask] : pkVal1;
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = ((lowestIdx & sp.xMask) != pkSlot1) ? sp.x_inLow[lowestIdx & sp.xMask] : pkVal1;
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = ((i & sp.xMask) != pkSlot1) ? sp.x_inLow[i & sp.xMask] : pkVal1;
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = ((today & sp.xMask) != pkSlot0) ? sp.x_inHigh[today & sp.xMask] : pkVal0;
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = ((highestIdx & sp.xMask) != pkSlot0) ? sp.x_inHigh[highestIdx & sp.xMask] : pkVal0;
            i = highestIdx;
            while( ++i <= today ) {
               tmp = ((i & sp.xMask) != pkSlot0) ? sp.x_inHigh[i & sp.xMask] : pkVal0;
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Divide by the range itself and scale after: the guard has to test the
          * very expression the division uses, or a scaling step can carry a
          * guarded-non-zero into a zero divisor.
          *
          * The band is the range against ITS OWN two extremes, not a fixed
          * constant: the range carries the quote unit, so a constant answers
          * "flat" for every window of an instrument quoted below it (issue #253).
          * It absorbs the machine-flat window an exact test would divide into
          * [0,100] noise (issue #107 / STOCHRSI).
          */
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            cur_tempBuffer = ((((today & sp.xMask) != pkSlot2) ? sp.x_inClose[today & sp.xMask] : pkVal2) - lowest) / (highest - lowest) * 100.0;
         } else {
            cur_tempBuffer = 0.0;
         }
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_outFastD = sp.sub0.peek(cur_tempBuffer);
         cur_outFastK = cur_tempBuffer;
         out.fastK = cur_outFastK;
         out.fastD = cur_outFastD;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( StochfOut out ) {
         requireArgument("STOCHF value", "out", out);
         out.fastK = this.cur_outFastK;
         out.fastD = this.cur_outFastD;
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
      public StochfStream clone() {
         return new StochfStream(this);
      }
   }

   /**
    * The outputs of one STOCHF bar, written by the stream into an object the
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
   public static final class StochfOut {
      /** Raw %K stochastic line. */
      public double fastK;
      /** MA-smoothed %K (signal line) */
      public double fastD;
   }
   void stochfStepImpl( StochfStream sp, double inHigh, double inLow, double inClose )
   {
      double tmp = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outFastD = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = sp.trailingIdx & ~sp.xMask;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inHigh[sp.today & sp.xMask] = inHigh;
      sp.x_inLow[sp.today & sp.xMask] = inLow;
      sp.x_inClose[sp.today & sp.xMask] = inClose;
      /* Set the lowest low */
      tmp = sp.x_inLow[sp.today & sp.xMask];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx & sp.xMask];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i & sp.xMask];
            if( tmp < sp.lowest ) {
               sp.lowestIdx = sp.i;
               sp.lowest = tmp;
            }
         }
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
      }
      /* Set the highest high */
      tmp = sp.x_inHigh[sp.today & sp.xMask];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx & sp.xMask];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i & sp.xMask];
            if( tmp > sp.highest ) {
               sp.highestIdx = sp.i;
               sp.highest = tmp;
            }
         }
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
      }
      /* Divide by the range itself and scale after: the guard has to test the
       * very expression the division uses, or a scaling step can carry a
       * guarded-non-zero into a zero divisor.
       *
       * The band is the range against ITS OWN two extremes, not a fixed
       * constant: the range carries the quote unit, so a constant answers
       * "flat" for every window of an instrument quoted below it (issue #253).
       * It absorbs the machine-flat window an exact test would divide into
       * [0,100] noise (issue #107 / STOCHRSI).
       */
      if( !(Math.abs(sp.highest - sp.lowest) <= 0.00000000000001 * (Math.abs(sp.highest) + Math.abs(sp.lowest))) ) {
         cur_tempBuffer = (sp.x_inClose[sp.today & sp.xMask] - sp.lowest) / (sp.highest - sp.lowest) * 100.0;
      } else {
         cur_tempBuffer = 0.0;
      }
      sp.trailingIdx += 1;
      sp.today += 1;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_outFastD = sp.sub0.update(cur_tempBuffer);
      sp.cur_outFastK = cur_tempBuffer;
      sp.cur_outFastD = cur_outFastD;
   }
   private RetCode stochfOpenImpl( StochfStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[], int outStride )
   {
      RetCode retCode;
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double[] tempBuffer;
      int outIdx = 0;
      int lowestIdx = 0;
      int highestIdx = 0;
      int lookbackTotal = 0;
      int lookbackK = 0;
      int lookbackFastD = 0;
      int trailingIdx = 0;
      int today = 0;
      int i = 0;
      int bufferIsAllocated = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInFastD_MAType == MAType.DEFAULT ) {
         optInFastD_MAType = MAType.SMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outFastK = outStride == 1 ? outFastK : new double[historyLen];
      double[] sc_outFastD = outStride == 1 ? outFastD : new double[historyLen];
      i = 0;
      /* With stochastic, there is a total of 4 different lines that
       * are defined: FASTK, FASTD, SLOWK and SLOWD.
       *
       * The D is the signal line usually drawn over its
       * corresponding K function.
       *
       *                    (Today's Close - LowestLow)
       *  FASTK(Kperiod) =  --------------------------- * 100
       *                     (HighestHigh - LowestLow)
       *
       *  FASTD(FastDperiod, MA type) = MA Smoothed FASTK over FastDperiod
       *
       *  SLOWK(SlowKperiod, MA type) = MA Smoothed FASTK over SlowKperiod
       *
       *  SLOWD(SlowDperiod, MA Type) = MA Smoothed SLOWK over SlowDperiod
       *
       * The HighestHigh and LowestLow are the extreme values among the
       * last 'Kperiod'.
       *
       * SLOWK and FASTD are equivalent when using the same period.
       *
       * The following shows how these four lines are made available in TA-LIB:
       *
       *  TA_STOCH  : Returns the SLOWK and SLOWD
       *  TA_STOCHF : Returns the FASTK and FASTD
       *
       * The TA_STOCH function correspond to the more widely implemented version
       * found in many software/charting package. The TA_STOCHF is more rarely
       * used because its higher volatility cause often whipsaws.
       */
      /* Identify the lookback needed. */
      lookbackK = optInFastK_Period - 1;
      lookbackFastD = MA_Lookback(optInFastD_Period, optInFastD_MAType);
      lookbackTotal = lookbackK + lookbackFastD;
      /* Move up the start index if there is not
       * enough initial data.
       */
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         /* Succeed... but no data in the output. */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Do the K calculation:
       *
       *    Kt = 100 x ((Ct-Lt)/(Ht-Lt))
       *
       * Kt is today stochastic
       * Ct is today closing price.
       * Lt is the lowest price of the last K Period (including today)
       * Ht is the highest price of the last K Period (including today)
       */
      /* Proceed with the calculation for the requested range.
       * Note that this algorithm allows the input and
       * output to be the same buffer.
       */
      outIdx = 0;
      /* Calculate just enough K for ending up with the caller
       * requested range. (The range of k must consider all
       * the lookback involve with the smoothing).
       */
      trailingIdx = startIdx - lookbackTotal;
      today = trailingIdx + lookbackK;
      highestIdx = 0 - 1;
      lowestIdx = highestIdx;
      lowest = 0.0;
      highest = lowest;
      /* Allocate a temporary buffer large enough to
       * store the K.
       *
       * When outFastK aliases a price input the caller buffer doubles as the
       * scratch, saving one allocation: the K writes trail the min/max window
       * reads, and the final memmove is overlap-safe. outFastD must NOT be
       * elected: the %D ma() below would then run in place over the raw K
       * that the memmove into outFastK still needs (issue #130).
       */
      bufferIsAllocated = 0;
      if( sc_outFastK == inHigh || sc_outFastK == inLow || sc_outFastK == inClose ) {
         tempBuffer = sc_outFastK;
      } else {
         bufferIsAllocated = 1;
         tempBuffer = new double[(int)((endIdx - today + 1) * 1)];
      }
      /* Do the K calculation */
      while( today <= endIdx ) {
         /* Set the lowest low */
         tmp = inLow[today];
         if( lowestIdx < trailingIdx ) {
            lowestIdx = trailingIdx;
            lowest = inLow[lowestIdx];
            i = lowestIdx;
            while( ++i <= today ) {
               tmp = inLow[i];
               if( tmp < lowest ) {
                  lowestIdx = i;
                  lowest = tmp;
               }
            }
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
         }
         /* Set the highest high */
         tmp = inHigh[today];
         if( highestIdx < trailingIdx ) {
            highestIdx = trailingIdx;
            highest = inHigh[highestIdx];
            i = highestIdx;
            while( ++i <= today ) {
               tmp = inHigh[i];
               if( tmp > highest ) {
                  highestIdx = i;
                  highest = tmp;
               }
            }
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
         }
         /* Divide by the range itself and scale after: the guard has to test the
          * very expression the division uses, or a scaling step can carry a
          * guarded-non-zero into a zero divisor.
          *
          * The band is the range against ITS OWN two extremes, not a fixed
          * constant: the range carries the quote unit, so a constant answers
          * "flat" for every window of an instrument quoted below it (issue #253).
          * It absorbs the machine-flat window an exact test would divide into
          * [0,100] noise (issue #107 / STOCHRSI).
          */
         if( !(Math.abs(highest - lowest) <= 0.00000000000001 * (Math.abs(highest) + Math.abs(lowest))) ) {
            tempBuffer[outIdx++] = (inClose[today] - lowest) / (highest - lowest) * 100.0;
         } else {
            tempBuffer[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      /* Fast-K calculation completed. This K calculation is returned
       * to the caller. It is smoothed to become Fast-D.
       */
      /* Sub-stream 0: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sc_outFastD);
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         /* Something wrong happen? No further data? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Copy tempBuffer into the caller buffer.
       * (Calculation could not be done directly in the
       *  caller buffer because more input data then the
       *  requested range was needed for doing %D).
       */
      /* memmove, not memcpy: tempBuffer aliases outFastK when the caller buffer is
       * reused as scratch, so source and destination overlap (issue #94).
       */
      System.arraycopy(tempBuffer, lookbackFastD, sc_outFastK, 0, (int)outNBElement.value * 1);
      /* Note: Keep the outBegIdx relative to the
       *       caller input before returning.
       */
      outBegIdx.value = startIdx;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.InsufficientHistory;
      }
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      int physX = 1;
      while( physX < capX ) {
         physX <<= 1;
      }
      double[] capX_inHigh = new double[physX];
      double[] capX_inLow = new double[physX];
      double[] capX_inClose = new double[physX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ & (physX - 1)] = inHigh[fillJ];
         capX_inLow[fillJ & (physX - 1)] = inLow[fillJ];
         capX_inClose[fillJ & (physX - 1)] = inClose[fillJ];
      }
      sp.optInFastK_Period = optInFastK_Period;
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
      sp.lowest = lowest;
      sp.highest = highest;
      sp.lowestIdx = lowestIdx;
      sp.highestIdx = highestIdx;
      sp.trailingIdx = trailingIdx;
      sp.i = i;
      sp.today = today;
      sp.xMask = physX - 1;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.sub0 = sub0;
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* stochfOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   StochfStream stochfOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
   {
      StochfStream sp = new StochfStream(this);
      RetCode retCode = stochfOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STOCHF openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STOCHF openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("STOCHF openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind stochfOpen (composition seam). */
   StochfStream stochfOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      StochfStream sp = new StochfStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outFastK = new double[1];
      double[] sink_outFastD = new double[1];
      RetCode retCode = stochfOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sink_outFastK, sink_outFastD, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STOCHF open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STOCHF open: internal error", retCode);
      }
      throw new TaLibArgumentException("STOCHF open: " + retCode, retCode);
   }
   /**
    * Open a live STOCHF stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#STOCHF} at that bar.
    * <p>The history must hold at least {@code STOCHF_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public StochfStream stochfOpen( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      requireArgument("STOCHF open", "inHigh", inHigh);
      requireHistory("STOCHF open", inHigh.length);
      requireArgument("STOCHF open", "optInFastD_MAType", optInFastD_MAType);
      requireArgument("STOCHF open", "inLow", inLow);
      requireArgument("STOCHF open", "inClose", inClose);
      requireHistoryLength("STOCHF open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("STOCHF open", "inClose", inClose.length, inHigh.length);
      return stochfOpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
   }
   /**
    * {@link Core#stochfOpen} that also fills the output array(s) bit-identically
    * to {@link Core#STOCHF} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link StochfStream#outRange()}.
    */
   public StochfStream stochfOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, double outFastK[], double outFastD[] )
   {
      requireArgument("STOCHF openAndFill", "inHigh", inHigh);
      requireHistory("STOCHF openAndFill", inHigh.length);
      requireArgument("STOCHF openAndFill", "optInFastD_MAType", optInFastD_MAType);
      requireArgument("STOCHF openAndFill", "inLow", inLow);
      requireArgument("STOCHF openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("STOCHF openAndFill", inHigh.length, STOCHF_Lookback(optInFastK_Period, optInFastD_Period, optInFastD_MAType));
      requireHistoryLength("STOCHF openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("STOCHF openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("STOCHF openAndFill", "outFastK", outFastK, guardOutLen);
      requireLength("STOCHF openAndFill", "outFastD", outFastD, guardOutLen);
      if( (Object)outFastK == (Object)inHigh || (Object)outFastK == (Object)inLow || (Object)outFastK == (Object)inClose || (Object)outFastD == (Object)inHigh || (Object)outFastD == (Object)inLow || (Object)outFastD == (Object)inClose || (Object)outFastK == (Object)outFastD ) {
         throw new TaLibArgumentException("STOCHF openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return stochfOpenAndFillInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
   }
