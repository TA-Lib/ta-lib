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
 */

   public int stochLookback( int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
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
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return -1;
      }
      int retValue;
      /* Account for the initial data needed for Fast-K. */
      retValue = optInFastK_Period - 1;
      /* Add the smoothing being done for %K slow */
      retValue += movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      /* Add the smoothing being done for %D slow. */
      retValue += movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
      return retValue ;

   }
   public RetCode stoch( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
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
      lookbackKSlow = movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
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
       * If the output is the same as the input, great
       * we just save ourself one memory allocation.
       */
      bufferIsAllocated = 0;
      if( outSlowK == inHigh || outSlowK == inLow || outSlowK == inClose ) {
         tempBuffer = outSlowK;
      } else if( outSlowD == inHigh || outSlowD == inLow || outSlowD == inClose ) {
         tempBuffer = outSlowD;
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
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, outBegIdx, outNBElement, tempBuffer);
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
      retCode = movingAverageUnguarded(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowD);
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
   public RetCode stochUnguarded( int startIdx,
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
      lookbackK = optInFastK_Period - 1;
      lookbackKSlow = movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
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
      if( outSlowK == inHigh || outSlowK == inLow || outSlowK == inClose ) {
         tempBuffer = outSlowK;
      } else if( outSlowD == inHigh || outSlowD == inLow || outSlowD == inClose ) {
         tempBuffer = outSlowD;
      } else {
         bufferIsAllocated = 1;
         tempBuffer = new double[(int)((endIdx - today + 1) * 1)];
      }
      while( today <= endIdx ) {
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
         if( !((-0.00000000000001 < diff) && (diff < 0.00000000000001)) ) {
            tempBuffer[outIdx++] = (inClose[today] - lowest) / diff;
         } else {
            tempBuffer[outIdx++] = 0.0;
         }
         trailingIdx += 1;
         today += 1;
      }
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, outBegIdx, outNBElement, tempBuffer);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = movingAverageUnguarded(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowD);
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
   public RetCode stoch( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outSlowK == outSlowD ) {
         return RetCode.BadParam ;
      }
      lookbackK = optInFastK_Period - 1;
      lookbackKSlow = movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
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
      } else if( false || false || false ) {
         tempBuffer = outSlowD;
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
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, outBegIdx, outNBElement, tempBuffer);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = movingAverageUnguarded(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowD);
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
   public RetCode stochUnguarded( int startIdx,
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
      lookbackK = optInFastK_Period - 1;
      lookbackKSlow = movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
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
      } else if( false || false || false ) {
         tempBuffer = outSlowD;
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
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, outBegIdx, outNBElement, tempBuffer);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = movingAverageUnguarded(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowD);
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
/**** Streaming API *****/

   /**
    * A live STOCH stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#stoch} over the same series.
    * Open with {@link Core#stochOpen}; there is no close — the handle is
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
   public static final class StochStream {
      final Core core;
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
      int xCap;
      double[] x_inHigh;
      double[] x_inLow;
      double[] x_inClose;
      double cur_outSlowK;
      double cur_outSlowD;
      Value cachedValue;
      MovingAverageStream sub0;
      MovingAverageStream sub1;

      StochStream( Core core ) { this.core = core; }

      StochStream( StochStream other ) {
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
         this.xCap = other.xCap;
         this.x_inHigh = other.x_inHigh.clone();
         this.x_inLow = other.x_inLow.clone();
         this.x_inClose = other.x_inClose.clone();
         this.cur_outSlowK = other.cur_outSlowK;
         this.cur_outSlowD = other.cur_outSlowD;
         this.cachedValue = other.cachedValue;
         this.sub0 = new MovingAverageStream(other.sub0);
         this.sub1 = new MovingAverageStream(other.sub1);
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double slowK;
         public final double slowD;
         Value( double slowK, double slowD ) {
            this.slowK = slowK;
            this.slowD = slowD;
         }
         @Override public String toString() {
            return "Value[" + "slowK=" + slowK + ", " + "slowD=" + slowD + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.slowK) == Double.doubleToLongBits(v.slowK) && Double.doubleToLongBits(this.slowD) == Double.doubleToLongBits(v.slowD);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(slowK);
            h = 31 * h + Double.hashCode(slowD);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inHigh, double inLow, double inClose ) {
         core.stochStreamStep(this, inHigh, inLow, inClose);
         this.cachedValue = new Value(this.cur_outSlowK, this.cur_outSlowD);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inHigh, double inLow, double inClose ) {
         StochStream scratch = new StochStream(this);
         core.stochStreamStep(scratch, inHigh, inLow, inClose);
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
      public StochStream copy() {
         return new StochStream(this);
      }
   }
   void stochStreamStep( StochStream sp, double inHigh, double inLow, double inClose )
   {
      double tmp = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outSlowD = 0.0;
      if( sp.today >= 1073741824 ) {
         int rebaseShift = (sp.trailingIdx / sp.xCap) * sp.xCap;
         sp.today -= rebaseShift;
         sp.trailingIdx -= rebaseShift;
         sp.highestIdx -= rebaseShift;
         sp.i -= rebaseShift;
         sp.lowestIdx -= rebaseShift;
      }
      sp.x_inHigh[sp.today % sp.xCap] = inHigh;
      sp.x_inLow[sp.today % sp.xCap] = inLow;
      sp.x_inClose[sp.today % sp.xCap] = inClose;
      /* Set the lowest low */
      tmp = sp.x_inLow[sp.today % sp.xCap];
      if( sp.lowestIdx < sp.trailingIdx ) {
         sp.lowestIdx = sp.trailingIdx;
         sp.lowest = sp.x_inLow[sp.lowestIdx % sp.xCap];
         sp.i = sp.lowestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inLow[sp.i % sp.xCap];
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
      tmp = sp.x_inHigh[sp.today % sp.xCap];
      if( sp.highestIdx < sp.trailingIdx ) {
         sp.highestIdx = sp.trailingIdx;
         sp.highest = sp.x_inHigh[sp.highestIdx % sp.xCap];
         sp.i = sp.highestIdx;
         while( ++sp.i <= sp.today ) {
            tmp = sp.x_inHigh[sp.i % sp.xCap];
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
         cur_tempBuffer = (sp.x_inClose[sp.today % sp.xCap] - sp.lowest) / sp.diff;
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
   private RetCode stochOpenBody( StochStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
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
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      double[] sc_outSlowK = new double[historyLen];
      double[] sc_outSlowD = new double[historyLen];
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
      lookbackKSlow = movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
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
         return RetCode.OutOfRangeEndIndex ;
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
       * If the output is the same as the input, great
       * we just save ourself one memory allocation.
       */
      bufferIsAllocated = 0;
      if( sc_outSlowK == inHigh || sc_outSlowK == inLow || sc_outSlowK == inClose ) {
         tempBuffer = sc_outSlowK;
      } else if( sc_outSlowD == inHigh || sc_outSlowD == inLow || sc_outSlowD == inClose ) {
         tempBuffer = sc_outSlowD;
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
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInSlowK_Period, optInSlowK_MAType);
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, outBegIdx, outNBElement, tempBuffer);
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
      MovingAverageStream sub1 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, ((int)outNBElement.value - 1) + 1), 0, optInSlowD_Period, optInSlowD_MAType);
      retCode = movingAverageUnguarded(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sc_outSlowD);
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
         return RetCode.OutOfRangeEndIndex;
      }
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capX_inHigh = new double[capX];
      double[] capX_inLow = new double[capX];
      double[] capX_inClose = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ % capX] = inHigh[fillJ];
         capX_inLow[fillJ % capX] = inLow[fillJ];
         capX_inClose[fillJ % capX] = inClose[fillJ];
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
      sp.xCap = capX;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outSlowK = sc_outSlowK[outNBElement.value - 1];
      sp.cur_outSlowD = sc_outSlowD[outNBElement.value - 1];
      sp.cachedValue = new StochStream.Value(sp.cur_outSlowK, sp.cur_outSlowD);
      return RetCode.Success;
   }
   private RetCode stochOpenAndFillBody( StochStream sp, double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outSlowK[], double outSlowD[] )
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
      int startIdx = 0;
      if( historyLen < 1 || inLow.length != inHigh.length || inClose.length != inHigh.length ) {
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
      if( optInSlowD_Period == Integer.MIN_VALUE ) {
         optInSlowD_Period = 3;
      } else if( optInSlowD_Period < 1 || optInSlowD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outSlowK == (Object)inHigh || (Object)outSlowK == (Object)inLow || (Object)outSlowK == (Object)inClose || (Object)outSlowD == (Object)inHigh || (Object)outSlowD == (Object)inLow || (Object)outSlowD == (Object)inClose || (Object)outSlowK == (Object)outSlowD ) {
         return RetCode.BadParam;
      }
      double[] sc_outSlowK = new double[historyLen];
      double[] sc_outSlowD = new double[historyLen];
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
      lookbackKSlow = movingAverageLookback(optInSlowK_Period, optInSlowK_MAType);
      lookbackDSlow = movingAverageLookback(optInSlowD_Period, optInSlowD_MAType);
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
         return RetCode.OutOfRangeEndIndex ;
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
       * If the output is the same as the input, great
       * we just save ourself one memory allocation.
       */
      bufferIsAllocated = 0;
      if( sc_outSlowK == inHigh || sc_outSlowK == inLow || sc_outSlowK == inClose ) {
         tempBuffer = sc_outSlowK;
      } else if( sc_outSlowD == inHigh || sc_outSlowD == inLow || sc_outSlowD == inClose ) {
         tempBuffer = sc_outSlowD;
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
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInSlowK_Period, optInSlowK_MAType);
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInSlowK_Period, optInSlowK_MAType, outBegIdx, outNBElement, tempBuffer);
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
      MovingAverageStream sub1 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, ((int)outNBElement.value - 1) + 1), 0, optInSlowD_Period, optInSlowD_MAType);
      retCode = movingAverageUnguarded(0, (int)outNBElement.value - 1, tempBuffer, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, sc_outSlowD);
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
         return RetCode.OutOfRangeEndIndex;
      }
      /* Capture the live batch state into the handle. */
      int capX = today - trailingIdx + 1;
      if( capX < 1 || capX > historyLen ) {
         return RetCode.InternalError;
      }
      double[] capX_inHigh = new double[capX];
      double[] capX_inLow = new double[capX];
      double[] capX_inClose = new double[capX];
      for( int fillJ = historyLen - capX; fillJ < historyLen; fillJ++ ) {
         capX_inHigh[fillJ % capX] = inHigh[fillJ];
         capX_inLow[fillJ % capX] = inLow[fillJ];
         capX_inClose[fillJ % capX] = inClose[fillJ];
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
      sp.xCap = capX;
      sp.x_inHigh = capX_inHigh;
      sp.x_inLow = capX_inLow;
      sp.x_inClose = capX_inClose;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outSlowK = sc_outSlowK[outNBElement.value - 1];
      sp.cur_outSlowD = sc_outSlowD[outNBElement.value - 1];
      sp.cachedValue = new StochStream.Value(sp.cur_outSlowK, sp.cur_outSlowD);
      System.arraycopy(sc_outSlowK, 0, outSlowK, 0, outNBElement.value);
      System.arraycopy(sc_outSlowD, 0, outSlowD, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind stochOpen (composition seam). */
   StochStream stochOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      StochStream sp = new StochStream(this);
      RetCode retCode = stochOpenBody(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STOCH open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STOCH open: internal error");
      }
      throw new IllegalArgumentException("TA_STOCH open: " + retCode);
   }
   /**
    * Open a live STOCH stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#stoch} at that bar.
    * <p>The history must hold at least {@code stochLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public StochStream stochOpen( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType )
   {
      return stochOpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType);
   }
   /**
    * {@link Core#stochOpen} that also fills the output array(s) bit-identically
    * to {@link Core#stoch} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public StochStream stochOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInSlowK_Period, MAType optInSlowK_MAType, int optInSlowD_Period, MAType optInSlowD_MAType, MInteger outBegIdx, MInteger outNBElement, double outSlowK[], double outSlowD[] )
   {
      StochStream sp = new StochStream(this);
      RetCode retCode = stochOpenAndFillBody(sp, inHigh, inLow, inClose, optInFastK_Period, optInSlowK_Period, optInSlowK_MAType, optInSlowD_Period, optInSlowD_MAType, outBegIdx, outNBElement, outSlowK, outSlowD);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STOCH openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STOCH openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_STOCH openAndFill: " + retCode);
   }
