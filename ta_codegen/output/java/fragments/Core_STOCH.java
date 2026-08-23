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
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param optInSlowD_Period Smoothing period for the SlowD signal line
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowD_MAType MA type used for the SlowD line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
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
      double diff = 0;
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
      diff = highest;
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
            diff = (highest - lowest) / 100.0;
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / 100.0;
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
            diff = (highest - lowest) / 100.0;
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / 100.0;
         }
         /* Calculate stochastic. Guard with TA_IS_ZERO, not an exact `diff != 0.0`:
          * a machine-flat window leaves a sub-epsilon residue that an exact check
          * would divide into [0,100] noise (issue #107 / STOCHRSI).
          */
         if( !((-0.00000000000001 < diff) && (diff < 0.00000000000001)) ) {
            tempBuffer[outIdx++] = (inClose[today] - lowest) / diff;
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
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
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
      /* Don't need K anymore, free it if it was allocated here. */
      if( (bufferIsAllocated) != 0 ) {
      }
      if( retCode != RetCode.Success ) {
         /* Something wrong happen while processing %D? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
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
      double diff = 0;
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
      diff = highest;
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
            diff = (highest - lowest) / 100.0;
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / 100.0;
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
            diff = (highest - lowest) / 100.0;
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / 100.0;
         }
         if( !((-0.00000000000001 < diff) && (diff < 0.00000000000001)) ) {
            tempBuffer[outIdx++] = ((double)inClose[today] - lowest) / diff;
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
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      OutRange _xr1 = MA(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outSlowD);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.Success;
      System.arraycopy(tempBuffer, lookbackDSlow, outSlowK, 0, (int)outNBElement.value * 1);
      if( (bufferIsAllocated) != 0 ) {
      }
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
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
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param optInSlowD_Period Smoothing period for the SlowD signal line
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowD_MAType MA type used for the SlowD line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outSlowK Raw FastK smoothed by SlowK_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSlowD Signal line: SlowK smoothed by SlowD_Period MA. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
      int guardStart = clampedStart(startIdx, endIdx, STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param optInSlowD_Period Smoothing period for the SlowD signal line
    *        (default 3; range 1..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSlowD_MAType MA type used for the SlowD line (default 0 = SMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT; {@code MAType.DEFAULT} selects the
    *        default).
    * @param outSlowK Raw FastK smoothed by SlowK_Period MA. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @param outSlowD Signal line: SlowK smoothed by SlowD_Period MA. Must hold
    *        at least {@code endIdx - startIdx + 1} values.
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
      int guardStart = clampedStart(startIdx, endIdx, STOCH_Lookback(optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType));
      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;
      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;
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
    * Open with {@link Core#STOCH_Open}; there is no close — the handle is
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
   public static final class STOCH_Stream {
      Core core;
      int optInFastK_Period;
      int optInSlowK_Period;
      MAType optInSlowK_MAType;
      int optInSlowD_Period;
      MAType optInSlowD_MAType;
      double lowest;
      double highest;
      double diff;
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
      Value cachedValue;
      MA_Stream sub0;
      MA_Stream sub1;
      int outRangeBegIdx;
      int outRangeCount;

      STOCH_Stream( Core core ) { this.core = core; }

      /**
       * The bars this stream has produced a value for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#STOCH} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count, {@code peek} leaves
       * it alone, and {@code copy()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      STOCH_Stream( STOCH_Stream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInSlowK_Period = other.optInSlowK_Period;
         this.optInSlowK_MAType = other.optInSlowK_MAType;
         this.optInSlowD_Period = other.optInSlowD_Period;
         this.optInSlowD_MAType = other.optInSlowD_MAType;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.diff = other.diff;
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
         this.cachedValue = other.cachedValue;
         this.sub0 = new MA_Stream(other.sub0);
         this.sub1 = new MA_Stream(other.sub1);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      void copyFrom( STOCH_Stream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInSlowK_Period = other.optInSlowK_Period;
         this.optInSlowK_MAType = other.optInSlowK_MAType;
         this.optInSlowD_Period = other.optInSlowD_Period;
         this.optInSlowD_MAType = other.optInSlowD_MAType;
         this.lowest = other.lowest;
         this.highest = other.highest;
         this.diff = other.diff;
         this.lowestIdx = other.lowestIdx;
         this.highestIdx = other.highestIdx;
         this.trailingIdx = other.trailingIdx;
         this.i = other.i;
         this.today = other.today;
         this.xMask = other.xMask;
         if( this.x_inHigh != null && this.x_inHigh.length == other.x_inHigh.length ) {
            System.arraycopy( other.x_inHigh, 0, this.x_inHigh, 0, other.x_inHigh.length );
         } else {
            this.x_inHigh = other.x_inHigh.clone();
         }
         if( this.x_inLow != null && this.x_inLow.length == other.x_inLow.length ) {
            System.arraycopy( other.x_inLow, 0, this.x_inLow, 0, other.x_inLow.length );
         } else {
            this.x_inLow = other.x_inLow.clone();
         }
         if( this.x_inClose != null && this.x_inClose.length == other.x_inClose.length ) {
            System.arraycopy( other.x_inClose, 0, this.x_inClose, 0, other.x_inClose.length );
         } else {
            this.x_inClose = other.x_inClose.clone();
         }
         this.cur_outSlowK = other.cur_outSlowK;
         this.cur_outSlowD = other.cur_outSlowD;
         this.cachedValue = other.cachedValue;
         if( this.sub0 == null ) {
            this.sub0 = new MA_Stream(other.sub0);
         } else {
            this.sub0.copyFrom(other.sub0);
         }
         if( this.sub1 == null ) {
            this.sub1 = new MA_Stream(other.sub1);
         } else {
            this.sub1.copyFrom(other.sub1);
         }
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /** {@code peek}'s reusable scratch — one per thread, see {@code copyFrom}. */
      private static final ThreadLocal<STOCH_Stream> PEEK_SCRATCH = new ThreadLocal<>();

      /**
       * One output set, in batch output order. Immutable.
       *
       * <p>{@code equals} compares every component bitwise, so {@code NaN}
       * equals {@code NaN} and {@code 0.0} does not equal {@code -0.0}.
       * {@code hashCode} is consistent with it but its exact value is
       * unspecified — do not persist it or compare it across JVM versions.
       *
       * @param slowK Raw FastK smoothed by SlowK_Period MA.
       * @param slowD Signal line: SlowK smoothed by SlowD_Period MA.
       */
      public record Value(double slowK, double slowD) { }

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
      public Value update( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("STOCH update: BadParam", RetCode.BadParam);
         core.STOCH_StepImpl(this, inHigh, inLow, inClose);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         this.cachedValue = new Value(this.cur_outSlowK, this.cur_outSlowD);
         return this.cachedValue;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inHigh.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what was committed, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * bars {@code 0..k} committed and written, bar {@code k} and everything
       * after it not, and the count advanced by {@code k}.
       */
      public void updateAndFill( double inHigh[], double inLow[], double inClose[], double outSlowK[], double outSlowD[] ) {
         final int barCount = inHigh.length;
         if( inLow.length != barCount || inClose.length != barCount || outSlowK.length < barCount || outSlowD.length < barCount || (Object)outSlowK == (Object)inHigh || (Object)outSlowK == (Object)inLow || (Object)outSlowK == (Object)inClose || (Object)outSlowD == (Object)inHigh || (Object)outSlowD == (Object)inLow || (Object)outSlowD == (Object)inClose || (Object)outSlowK == (Object)outSlowD )
            throw new TaLibArgumentException("STOCH updateAndFill: BadParam", RetCode.BadParam);
         int done = 0;
         try {
            for( int i = 0; i < barCount; i++ ) {
               if( !Double.isFinite(inHigh[i]) || !Double.isFinite(inLow[i]) || !Double.isFinite(inClose[i]) )
                  throw new TaLibArgumentException("STOCH updateAndFill: BadParam", RetCode.BadParam);
               core.STOCH_StepImpl(this, inHigh[i], inLow[i], inClose[i]);
               outSlowK[i] = this.cur_outSlowK;
               outSlowD[i] = this.cur_outSlowD;
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               done = i + 1;
            }
         } finally {
            if( done > 0 ) this.cachedValue = new Value(this.cur_outSlowK, this.cur_outSlowD);
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a copy). Never writes this handle, so peeks may
       * run concurrently with each other. It runs on a scratch handle held per thread and
       * reused, so the copy allocates nothing after the first peek of this
       * indicator on this thread. That scratch is retained for the life of
       * the thread.
       */
      public Value peek( double inHigh, double inLow, double inClose ) {
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw new TaLibArgumentException("STOCH peek: BadParam", RetCode.BadParam);
         STOCH_Stream scratch = PEEK_SCRATCH.get();
         if( scratch == null ) {
            scratch = new STOCH_Stream(this);
            PEEK_SCRATCH.set(scratch);
         } else {
            scratch.copyFrom(this);
         }
         core.STOCH_StepImpl(scratch, inHigh, inLow, inClose);
         return new Value(scratch.cur_outSlowK, scratch.cur_outSlowD);
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
      public STOCH_Stream copy() {
         return new STOCH_Stream(this);
      }
   }
   void STOCH_StepImpl( STOCH_Stream sp, double inHigh, double inLow, double inClose )
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
         sp.diff = (sp.highest - sp.lowest) / 100.0;
      } else if( tmp <= sp.lowest ) {
         sp.lowestIdx = sp.today;
         sp.lowest = tmp;
         sp.diff = (sp.highest - sp.lowest) / 100.0;
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
         sp.diff = (sp.highest - sp.lowest) / 100.0;
      } else if( tmp >= sp.highest ) {
         sp.highestIdx = sp.today;
         sp.highest = tmp;
         sp.diff = (sp.highest - sp.lowest) / 100.0;
      }
      /* Calculate stochastic. Guard with TA_IS_ZERO, not an exact `diff != 0.0`:
       * a machine-flat window leaves a sub-epsilon residue that an exact check
       * would divide into [0,100] noise (issue #107 / STOCHRSI).
       */
      if( !((-0.00000000000001 < sp.diff) && (sp.diff < 0.00000000000001)) ) {
         cur_tempBuffer = (sp.x_inClose[sp.today & sp.xMask] - sp.lowest) / sp.diff;
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
   private RetCode STOCH_OpenImpl( STOCH_Stream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outSlowK[], double outSlowD[], int outStride )
   {
      RetCode retCode;
      double lowest = 0;
      double highest = 0;
      double tmp = 0;
      double diff = 0;
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
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
      diff = highest;
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
            diff = (highest - lowest) / 100.0;
         } else if( tmp <= lowest ) {
            lowestIdx = today;
            lowest = tmp;
            diff = (highest - lowest) / 100.0;
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
            diff = (highest - lowest) / 100.0;
         } else if( tmp >= highest ) {
            highestIdx = today;
            highest = tmp;
            diff = (highest - lowest) / 100.0;
         }
         /* Calculate stochastic. Guard with TA_IS_ZERO, not an exact `diff != 0.0`:
          * a machine-flat window leaves a sub-epsilon residue that an exact check
          * would divide into [0,100] noise (issue #107 / STOCHRSI).
          */
         if( !((-0.00000000000001 < diff) && (diff < 0.00000000000001)) ) {
            tempBuffer[outIdx++] = (inClose[today] - lowest) / diff;
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
      MA_Stream sub0 = MA_OpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInSlowK_Period, optInSlowK_MAType);
      OutRange _xr0 = MA(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, tempBuffer);
      outBegIdx.value = _xr0.begIdx();
      outNBElement.value = _xr0.count();
      retCode = RetCode.Success;
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         /* Something wrong happen? No further data? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the %D which is simply a moving average of
       * the already smoothed %K.
       */
      /* Sub-stream 1: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MA_Stream sub1 = MA_OpenAndFillInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, ((int)outNBElement.value - 1) + 1), 0, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sc_outSlowD);
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
      /* Don't need K anymore, free it if it was allocated here. */
      if( (bufferIsAllocated) != 0 ) {
      }
      if( retCode != RetCode.Success ) {
         /* Something wrong happen while processing %D? */
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
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
      sp.diff = diff;
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
      sp.cachedValue = new STOCH_Stream.Value(sp.cur_outSlowK, sp.cur_outSlowD);
      return RetCode.Success;
   }
   /* STOCH_OpenAndFill anchored at startIdx — the composed-open fusion seam. */
   STOCH_Stream STOCH_OpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outSlowK[], double outSlowD[] )
   {
      STOCH_Stream sp = new STOCH_Stream(this);
      RetCode retCode = STOCH_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD, 1);
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
   /* Internal startIdx-anchored open behind STOCH_Open (composition seam). */
   STOCH_Stream STOCH_OpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      STOCH_Stream sp = new STOCH_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outSlowK = new double[1];
      double[] sink_outSlowD = new double[1];
      RetCode retCode = STOCH_OpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sink_outSlowK, sink_outSlowD, 0);
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
    * default, as in the batch API).
    */
   public STOCH_Stream STOCH_Open( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      return STOCH_OpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
   }
   /**
    * {@link Core#STOCH_Open} that also fills the output array(s) bit-identically
    * to {@link Core#STOCH} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link STOCH_Stream#outRange()}.
    */
   public STOCH_Stream STOCH_OpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, double outSlowK[], double outSlowD[] )
   {
      if( (Object)outSlowK == (Object)inHigh || (Object)outSlowK == (Object)inLow || (Object)outSlowK == (Object)inClose || (Object)outSlowD == (Object)inHigh || (Object)outSlowD == (Object)inLow || (Object)outSlowD == (Object)inClose || (Object)outSlowK == (Object)outSlowD ) {
         throw new TaLibArgumentException("STOCH openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return STOCH_OpenAndFillInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
   }
