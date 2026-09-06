/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY    Description
 *  -------------------------------------------------------------------
 *  112400 MF    Template creation.
 *  052603 MF    Adapt code to compile with .NET Managed C++
 *  071026 MF,CC Fix #107. Guard the Fast-K division with TA_IS_ZERO, not an
 *               exact `diff != 0.0`, so a machine-flat window yields 0 instead
 *               of dividing a sub-epsilon residue into [0,100] noise (STOCHRSI).
 *  072026 MF,CC Fix #130. Never elect outSlowD as the K scratch buffer: %D's
 *               in-place ma() destroyed the smoothed K before the final copy.
 *  082326 MF,CC Fix #253. Scale that guard to the window's own extremes: the
 *               fixed band zeroed the whole output for any instrument quoted
 *               small enough to fall under it.
 *  082726 MF,CC Fix #269. Answer a rejected %D ma() before the copy, not after:
 *               the stale *outNBElement overran outSlowK by lookbackDSlow.
 *  090626 MF,CC Fix #390. Divide by the range, scale after: the hoisted
 *               `(highest-lowest)/100.0` underflowed to 0.0 on a denormal
 *               range that the guard still called "not flat".
 */

   /**
    * Number of leading input bars {@link Core#STOCH} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastK_Period Lookback window for the raw %K high-low range
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_Period Smoothing period turning FastK into SlowK
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_MAType MA type used to smooth into SlowK (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSlowD_Period Smoothing period for the SlowD signal line
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowD_MAType MA type used for the SlowD line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int STOCH_Lookback( int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      if( optInFastK_Period == Integer.MIN_VALUE ) {
         optInFastK_Period = 5;
      } else if( optInFastK_Period < 1 || optInFastK_Period > 100000 ) {
         return -1;
      }
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return -1;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.SMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return -1;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.SMA;
      }
      int retValue;
      /* Account for the initial data needed for Fast-K. */
      retValue = optInFastK_Period - 1;
      /* Add the smoothing being done for %K slow */
      retValue += MA_Lookback(optInSlowK_Period, optInSlowK_MAType);
      /* Add the smoothing being done for %D slow. */
      retValue += MA_Lookback(optInSlowD_Period, optInSlowD_MAType);
      return retValue ;

   }
   RetCode STOCH_Impl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInFastK_Period,
                       int optInSlowK_Period,
                       MAType optInSlowK_MAType,
                       int optInSlowD_Period,
                       MAType optInSlowD_MAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outSlowK[],
                       double outSlowD[] )
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
      int lookbackKSlow = 0;
      int lookbackDSlow = 0;
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
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.SMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.SMA;
      }
      if( outSlowK == outSlowD ) {
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
      lookbackKSlow = MA_Lookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = MA_Lookback(optInSlowD_Period, optInSlowD_MAType);
      lookbackTotal = lookbackK + lookbackDSlow + lookbackKSlow;
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
       * When outSlowK aliases a price input the caller buffer doubles as the
       * scratch, saving one allocation: the K writes trail the min/max window
       * reads, and the final memmove is overlap-safe. outSlowD must NOT be
       * elected: the %D ma() below would then run in place over the smoothed K
       * that the memmove into outSlowK still needs (issue #130).
       */
      bufferIsAllocated = 0;
      if( outSlowK == inHigh || outSlowK == inLow || outSlowK == inClose ) {
         tempBuffer = outSlowK;
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
      /* Un-smoothed K calculation completed. This K calculation is not returned
       * to the caller. It is always smoothed and then return.
       * Some documentation will refer to the smoothed version as being
       * "K-Slow", but often this end up to be shorten to "K".
       */
      OutRange _xr0 = MA(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, tempBuffer);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         /* Something wrong happen? No further data? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the %D which is simply a moving average of
       * the already smoothed %K.
       */
      OutRange _xr1 = MA(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outSlowD);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      /* Copy tempBuffer into the caller buffer.
       * (Calculation could not be done directly in the
       *  caller buffer because more input data then the
       *  requested range was needed for doing %D).
       */
      /* memmove, not memcpy: tempBuffer aliases outSlowK when the caller buffer is
       * reused as scratch, so source and destination overlap (issue #94).
       */
      System.arraycopy(tempBuffer, lookbackDSlow, outSlowK, 0, (int)outNBElement.value * 1);
      /* Note: Keep the outBegIdx relative to the
       *       caller input before returning.
       */
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode STOCH_Impl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInFastK_Period,
                       int optInSlowK_Period,
                       MAType optInSlowK_MAType,
                       int optInSlowD_Period,
                       MAType optInSlowD_MAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outSlowK[],
                       double outSlowD[] )
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
      int lookbackKSlow = 0;
      int lookbackDSlow = 0;
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
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.SMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.SMA;
      }
      if( outSlowK == outSlowD ) {
         return RetCode.BadParam ;
      }
      i = 0;
      lookbackK = optInFastK_Period - 1;
      lookbackKSlow = MA_Lookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = MA_Lookback(optInSlowD_Period, optInSlowD_MAType);
      lookbackTotal = lookbackK + lookbackDSlow + lookbackKSlow;
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
         tempBuffer = outSlowK;
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
      OutRange _xr0 = MA(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, tempBuffer);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr1 = MA(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outSlowD);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      System.arraycopy(tempBuffer, lookbackDSlow, outSlowK, 0, (int)outNBElement.value * 1);
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Slow Stochastic oscillator: locates the close within the high-low range
    * over a lookback period, then double-smooths it. Returns the Slow-%K and
    * Slow-%D lines. SlowK/SlowD &gt; 80 overbought, &lt; 20 oversold; %K
    * crossing %D signals momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * FastK = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
    * SlowK = MA(FastK, SlowK_Period, SlowK_MAType)
    * SlowD = MA(SlowK, SlowD_Period, SlowD_MAType)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STOCH_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastK_Period Lookback window for the raw %K high-low range
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_Period Smoothing period turning FastK into SlowK
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_MAType MA type used to smooth into SlowK (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSlowD_Period Smoothing period for the SlowD signal line
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowD_MAType MA type used for the SlowD line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outSlowK Raw FastK smoothed by SlowK_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSlowD Signal line: SlowK smoothed by SlowD_Period MA. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#STOCHF
    * @see Core#STOCHRSI
    * @see Core#MA
    */
   public OutRange STOCH( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          int optInFastK_Period,
                          int optInSlowK_Period,
                          MAType optInSlowK_MAType,
                          int optInSlowD_Period,
                          MAType optInSlowD_MAType,
                          double outSlowK[],
                          double outSlowD[] )
   {
      requireIndexRange("STOCH", startIdx, endIdx);
      requireArgument("STOCH", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("STOCH", "optInSlowD_MAType", optInSlowD_MAType);
      int guardStart = clampedStart("STOCH", startIdx, STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STOCH", "inHigh", inHigh, guardInLen);
      requireLength("STOCH", "inLow", inLow, guardInLen);
      requireLength("STOCH", "inClose", inClose, guardInLen);
      requireLength("STOCH", "outSlowK", outSlowK, guardOutLen);
      requireLength("STOCH", "outSlowD", outSlowD, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCH_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Slow Stochastic oscillator: locates the close within the high-low range
    * over a lookback period, then double-smooths it. Returns the Slow-%K and
    * Slow-%D lines. SlowK/SlowD &gt; 80 overbought, &lt; 20 oversold; %K
    * crossing %D signals momentum shifts.
    * <p><b>Formula</b>
    * <pre>{@code
    * FastK = 100*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
    * SlowK = MA(FastK, SlowK_Period, SlowK_MAType)
    * SlowD = MA(SlowK, SlowD_Period, SlowD_MAType)
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#STOCH_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastK_Period Lookback window for the raw %K high-low range
    *        (default 5; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_Period Smoothing period turning FastK into SlowK
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowK_MAType MA type used to smooth into SlowK (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param optInSlowD_Period Smoothing period for the SlowD signal line
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowD_MAType MA type used for the SlowD line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outSlowK Raw FastK smoothed by SlowK_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSlowD Signal line: SlowK smoothed by SlowD_Period MA. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#STOCHF
    * @see Core#STOCHRSI
    * @see Core#MA
    */
   public OutRange STOCH( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          int optInFastK_Period,
                          int optInSlowK_Period,
                          MAType optInSlowK_MAType,
                          int optInSlowD_Period,
                          MAType optInSlowD_MAType,
                          double outSlowK[],
                          double outSlowD[] )
   {
      requireIndexRange("STOCH", startIdx, endIdx);
      requireArgument("STOCH", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("STOCH", "optInSlowD_MAType", optInSlowD_MAType);
      int guardStart = clampedStart("STOCH", startIdx, STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("STOCH", "inHigh", inHigh, guardInLen);
      requireLength("STOCH", "inLow", inLow, guardInLen);
      requireLength("STOCH", "inClose", inClose, guardInLen);
      requireLength("STOCH", "outSlowK", outSlowK, guardOutLen);
      requireLength("STOCH", "outSlowD", outSlowD, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = STOCH_Impl(startIdx, endIdx, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
      if( retCode != RetCode.Success ) {
         throw failure("STOCH", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live STOCH stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#STOCH} over the same series.
    * Open with {@link Core#stochOpen}; there is no close — the handle is
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
   public static final class StochStream {
      Core core;
      int optInFastK_Period;
      int optInSlowK_Period;
      MAType optInSlowK_MAType;
      int optInSlowD_Period;
      MAType optInSlowD_MAType;
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
      double cur_outSlowK;
      double cur_outSlowD;
      MaStream sub0;
      MaStream sub1;
      int outRangeBegIdx;
      int outRangeCount;

      StochStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#STOCH} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(StochOut)} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       */
      public void advance() { if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++; }

      StochStream( StochStream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInSlowK_Period = other.optInSlowK_Period;
         this.optInSlowK_MAType = other.optInSlowK_MAType;
         this.optInSlowD_Period = other.optInSlowD_Period;
         this.optInSlowD_MAType = other.optInSlowD_MAType;
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
         this.cur_outSlowK = other.cur_outSlowK;
         this.cur_outSlowD = other.cur_outSlowD;
         this.sub0 = new MaStream(other.sub0);
         this.sub1 = new MaStream(other.sub1);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(StochOut)} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public void update( double inHigh, double inLow, double inClose, StochOut out ) {
         requireArgument("STOCH update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("STOCH update: BadParam", RetCode.BadParam);
         core.stochStepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         out.slowK = this.cur_outSlowK;
         out.slowD = this.cur_outSlowD;
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
      public void peek( double inHigh, double inLow, double inClose, StochOut out ) {
         requireArgument("STOCH peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("STOCH peek: BadParam", RetCode.BadParam);
         StochStream sp = this;
         double cur_tempBuffer = 0.0;
         double cur_outSlowD = 0.0;
         double cur_outSlowK = 0.0;
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
         cur_tempBuffer = sp.sub0.peek(cur_tempBuffer);
         cur_outSlowD = sp.sub1.peek(cur_tempBuffer);
         cur_outSlowK = cur_tempBuffer;
         out.slowK = cur_outSlowK;
         out.slowD = cur_outSlowD;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}, allocating nothing.
       */
      public void value( StochOut out ) {
         requireArgument("STOCH value", "out", out);
         out.slowK = this.cur_outSlowK;
         out.slowD = this.cur_outSlowD;
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
      public StochStream clone() {
         return new StochStream(this);
      }
   }

   /**
    * The outputs of one STOCH bar, written by the stream into an object the
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
   public static final class StochOut {
      /** Raw FastK smoothed by SlowK_Period MA. */
      public double slowK;
      /** Signal line: SlowK smoothed by SlowD_Period MA. */
      public double slowD;
   }
   void stochStepImpl( StochStream sp, double inHigh, double inLow, double inClose )
   {
      double tmp = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outSlowD = 0.0;
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
      cur_tempBuffer = sp.sub0.update(cur_tempBuffer);
      cur_outSlowD = sp.sub1.update(cur_tempBuffer);
      sp.cur_outSlowK = cur_tempBuffer;
      sp.cur_outSlowD = cur_outSlowD;
   }
   private RetCode stochOpenImpl( StochStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outSlowK[], double outSlowD[], int outStride )
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
      int lookbackKSlow = 0;
      int lookbackDSlow = 0;
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
      if( optInSlowK_Period == Integer.MIN_VALUE ) {
         optInSlowK_Period = 3;
      } else if( optInSlowK_Period < 1 || optInSlowK_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowK_MAType == MAType.DEFAULT ) {
         optInSlowK_MAType = MAType.SMA;
      }
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInSlowD_MAType == MAType.DEFAULT ) {
         optInSlowD_MAType = MAType.SMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory;
      }
      if( historyLen < STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType) + 1 ) {
         return RetCode.InsufficientHistory;
      }
      double[] sc_outSlowK = outStride == 1 ? outSlowK : new double[historyLen];
      double[] sc_outSlowD = outStride == 1 ? outSlowD : new double[historyLen];
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
      lookbackKSlow = MA_Lookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = MA_Lookback(optInSlowD_Period, optInSlowD_MAType);
      lookbackTotal = lookbackK + lookbackDSlow + lookbackKSlow;
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
       * When outSlowK aliases a price input the caller buffer doubles as the
       * scratch, saving one allocation: the K writes trail the min/max window
       * reads, and the final memmove is overlap-safe. outSlowD must NOT be
       * elected: the %D ma() below would then run in place over the smoothed K
       * that the memmove into outSlowK still needs (issue #130).
       */
      bufferIsAllocated = 0;
      if( sc_outSlowK == inHigh || sc_outSlowK == inLow || sc_outSlowK == inClose ) {
         tempBuffer = sc_outSlowK;
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
      /* Un-smoothed K calculation completed. This K calculation is not returned
       * to the caller. It is always smoothed and then return.
       * Some documentation will refer to the smoothed version as being
       * "K-Slow", but often this end up to be shorten to "K".
       */
      /* Sub-stream 0: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInSlowK_Period, optInSlowK_MAType);
      OutRange _xr0 = MA(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, tempBuffer);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( (int)outNBElement.value == 0 ) {
         /* Something wrong happen? No further data? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.InsufficientHistory ;
      }
      /* Calculate the %D which is simply a moving average of
       * the already smoothed %K.
       */
      /* Sub-stream 1: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub1 = maOpenAndFillInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, ((int)outNBElement.value - 1) + 1), 0, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sc_outSlowD);
      retCode = RetCode.Success;
      /* Copy tempBuffer into the caller buffer.
       * (Calculation could not be done directly in the
       *  caller buffer because more input data then the
       *  requested range was needed for doing %D).
       */
      /* memmove, not memcpy: tempBuffer aliases outSlowK when the caller buffer is
       * reused as scratch, so source and destination overlap (issue #94).
       */
      System.arraycopy(tempBuffer, lookbackDSlow, sc_outSlowK, 0, (int)outNBElement.value * 1);
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
      sp.optInSlowK_Period = optInSlowK_Period;
      sp.optInSlowK_MAType = optInSlowK_MAType;
      sp.optInSlowD_Period = optInSlowD_Period;
      sp.optInSlowD_MAType = optInSlowD_MAType;
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
      sp.sub1 = sub1;
      sp.cur_outSlowK = sc_outSlowK[outNBElement.value - 1];
      sp.cur_outSlowD = sc_outSlowD[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* stochOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   StochStream stochOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outSlowK[], double outSlowD[] )
   {
      StochStream sp = new StochStream(this);
      RetCode retCode = stochOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STOCH openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STOCH openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("STOCH openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind stochOpen (composition seam). */
   StochStream stochOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      StochStream sp = new StochStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outSlowK = new double[1];
      double[] sink_outSlowD = new double[1];
      RetCode retCode = stochOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sink_outSlowK, sink_outSlowD, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("STOCH open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("STOCH open: internal error", retCode);
      }
      throw new TaLibArgumentException("STOCH open: " + retCode, retCode);
   }
   /**
    * Open a live STOCH stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#STOCH} at that bar.
    * <p>The history must hold at least {@code STOCH_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public StochStream stochOpen( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      requireArgument("STOCH open", "inHigh", inHigh);
      requireHistory("STOCH open", inHigh.length);
      requireArgument("STOCH open", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("STOCH open", "optInSlowD_MAType", optInSlowD_MAType);
      requireArgument("STOCH open", "inLow", inLow);
      requireArgument("STOCH open", "inClose", inClose);
      requireHistoryLength("STOCH open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("STOCH open", "inClose", inClose.length, inHigh.length);
      return stochOpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
   }
   /**
    * {@link Core#stochOpen} that also fills the output array(s) bit-identically
    * to {@link Core#STOCH} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link StochStream#outRange()}.
    */
   public StochStream stochOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, double outSlowK[], double outSlowD[] )
   {
      requireArgument("STOCH openAndFill", "inHigh", inHigh);
      requireHistory("STOCH openAndFill", inHigh.length);
      requireArgument("STOCH openAndFill", "optInSlowK_MAType", optInSlowK_MAType);
      requireArgument("STOCH openAndFill", "optInSlowD_MAType", optInSlowD_MAType);
      requireArgument("STOCH openAndFill", "inLow", inLow);
      requireArgument("STOCH openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("STOCH openAndFill", inHigh.length, STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      requireHistoryLength("STOCH openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("STOCH openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("STOCH openAndFill", "outSlowK", outSlowK, guardOutLen);
      requireLength("STOCH openAndFill", "outSlowD", outSlowD, guardOutLen);
      if( (Object)outSlowK == (Object)inHigh || (Object)outSlowK == (Object)inLow || (Object)outSlowK == (Object)inClose || (Object)outSlowD == (Object)inHigh || (Object)outSlowD == (Object)inLow || (Object)outSlowD == (Object)inClose || (Object)outSlowK == (Object)outSlowD ) {
         throw new TaLibArgumentException("STOCH openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return stochOpenAndFillInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
   }
