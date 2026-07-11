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
