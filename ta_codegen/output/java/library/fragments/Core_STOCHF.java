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
 */

   public int stochFLookback( int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
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
      int retValue;
      /* Account for the initial data needed for Fast-K. */
      retValue = optInFastK_Period - 1;
      /* Add the smoothing being done for Fast-D */
      retValue += movingAverageLookback(optInFastD_Period, optInFastD_MAType);
      return retValue ;

   }
   public RetCode stochF( int startIdx,
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
      double diff = 0;
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
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outFastK == outFastD ) {
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
      lookbackFastD = movingAverageLookback(optInFastD_Period, optInFastD_MAType);
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
      diff = highest;
      /* Allocate a temporary buffer large enough to
       * store the K.
       *
       * If the output is the same as the input, great
       * we just save ourself one memory allocation.
       */
      bufferIsAllocated = 0;
      if( outFastK == inHigh || outFastK == inLow || outFastK == inClose ) {
         tempBuffer = outFastK;
      } else if( outFastD == inHigh || outFastD == inLow || outFastD == inClose ) {
         tempBuffer = outFastD;
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
      /* Fast-K calculation completed. This K calculation is returned
       * to the caller. It is smoothed to become Fast-D.
       */
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
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
   public RetCode stochFUnguarded( int startIdx,
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
      double diff = 0;
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
      lookbackK = optInFastK_Period - 1;
      lookbackFastD = movingAverageLookback(optInFastD_Period, optInFastD_MAType);
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
      diff = highest;
      bufferIsAllocated = 0;
      if( outFastK == inHigh || outFastK == inLow || outFastK == inClose ) {
         tempBuffer = outFastK;
      } else if( outFastD == inHigh || outFastD == inLow || outFastD == inClose ) {
         tempBuffer = outFastD;
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
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      System.arraycopy(tempBuffer, lookbackFastD, outFastK, 0, (int)outNBElement.value * 1);
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
   public RetCode stochF( int startIdx,
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
      double diff = 0;
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
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( outFastK == outFastD ) {
         return RetCode.BadParam ;
      }
      lookbackK = optInFastK_Period - 1;
      lookbackFastD = movingAverageLookback(optInFastD_Period, optInFastD_MAType);
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
      diff = highest;
      bufferIsAllocated = 0;
      if( false || false || false ) {
         tempBuffer = outFastK;
      } else if( false || false || false ) {
         tempBuffer = outFastD;
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
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      System.arraycopy(tempBuffer, lookbackFastD, outFastK, 0, (int)outNBElement.value * 1);
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
   public RetCode stochFUnguarded( int startIdx,
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
      double diff = 0;
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
      lookbackK = optInFastK_Period - 1;
      lookbackFastD = movingAverageLookback(optInFastD_Period, optInFastD_MAType);
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
      diff = highest;
      bufferIsAllocated = 0;
      if( false || false || false ) {
         tempBuffer = outFastK;
      } else if( false || false || false ) {
         tempBuffer = outFastD;
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
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      System.arraycopy(tempBuffer, lookbackFastD, outFastK, 0, (int)outNBElement.value * 1);
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
    * A live STOCHF stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#stochF} over the same series.
    * Open with {@link Core#stochFOpen}; there is no close — the handle is
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
   public static final class StochFStream {
      final Core core;
      int optInFastK_Period;
      int optInFastD_Period;
      MAType optInFastD_MAType;
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
      double cur_outFastK;
      double cur_outFastD;
      Value cachedValue;
      MovingAverageStream sub0;

      StochFStream( Core core ) { this.core = core; }

      StochFStream( StochFStream other ) {
         this.core = other.core;
         this.optInFastK_Period = other.optInFastK_Period;
         this.optInFastD_Period = other.optInFastD_Period;
         this.optInFastD_MAType = other.optInFastD_MAType;
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
         this.cur_outFastK = other.cur_outFastK;
         this.cur_outFastD = other.cur_outFastD;
         this.cachedValue = other.cachedValue;
         this.sub0 = new MovingAverageStream(other.sub0);
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double fastK;
         public final double fastD;
         Value( double fastK, double fastD ) {
            this.fastK = fastK;
            this.fastD = fastD;
         }
         @Override public String toString() {
            return "Value[" + "fastK=" + fastK + ", " + "fastD=" + fastD + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.fastK) == Double.doubleToLongBits(v.fastK) && Double.doubleToLongBits(this.fastD) == Double.doubleToLongBits(v.fastD);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(fastK);
            h = 31 * h + Double.hashCode(fastD);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inHigh, double inLow, double inClose ) {
         core.stochFStreamStep(this, inHigh, inLow, inClose);
         this.cachedValue = new Value(this.cur_outFastK, this.cur_outFastD);
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
         StochFStream scratch = new StochFStream(this);
         core.stochFStreamStep(scratch, inHigh, inLow, inClose);
         return new Value(scratch.cur_outFastK, scratch.cur_outFastD);
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
      public StochFStream copy() {
         return new StochFStream(this);
      }
   }
   void stochFStreamStep( StochFStream sp, double inHigh, double inLow, double inClose )
   {
      double tmp = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outFastD = 0.0;
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
      cur_outFastD = sp.sub0.update(cur_tempBuffer);
      sp.cur_outFastK = cur_tempBuffer;
      sp.cur_outFastD = cur_outFastD;
   }
   private RetCode stochFOpenBody( StochFStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
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
      int lookbackFastD = 0;
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
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      double[] sc_outFastK = new double[historyLen];
      double[] sc_outFastD = new double[historyLen];
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
      lookbackFastD = movingAverageLookback(optInFastD_Period, optInFastD_MAType);
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
      if( sc_outFastK == inHigh || sc_outFastK == inLow || sc_outFastK == inClose ) {
         tempBuffer = sc_outFastK;
      } else if( sc_outFastD == inHigh || sc_outFastD == inLow || sc_outFastD == inClose ) {
         tempBuffer = sc_outFastD;
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
      /* Fast-K calculation completed. This K calculation is returned
       * to the caller. It is smoothed to become Fast-D.
       */
      /* Sub-stream 0: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInFastD_Period, optInFastD_MAType);
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sc_outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
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
      System.arraycopy(tempBuffer, lookbackFastD, sc_outFastK, 0, (int)outNBElement.value * 1);
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
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
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
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      sp.cachedValue = new StochFStream.Value(sp.cur_outFastK, sp.cur_outFastD);
      return RetCode.Success;
   }
   private RetCode stochFOpenAndFillBody( StochFStream sp, double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
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
      int lookbackFastD = 0;
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
      if( optInFastD_Period == Integer.MIN_VALUE ) {
         optInFastD_Period = 3;
      } else if( optInFastD_Period < 1 || optInFastD_Period > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outFastK == (Object)inHigh || (Object)outFastK == (Object)inLow || (Object)outFastK == (Object)inClose || (Object)outFastD == (Object)inHigh || (Object)outFastD == (Object)inLow || (Object)outFastD == (Object)inClose || (Object)outFastK == (Object)outFastD ) {
         return RetCode.BadParam;
      }
      double[] sc_outFastK = new double[historyLen];
      double[] sc_outFastD = new double[historyLen];
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
      lookbackFastD = movingAverageLookback(optInFastD_Period, optInFastD_MAType);
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
      if( sc_outFastK == inHigh || sc_outFastK == inLow || sc_outFastK == inClose ) {
         tempBuffer = sc_outFastK;
      } else if( sc_outFastD == inHigh || sc_outFastD == inLow || sc_outFastD == inClose ) {
         tempBuffer = sc_outFastD;
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
      /* Fast-K calculation completed. This K calculation is returned
       * to the caller. It is smoothed to become Fast-D.
       */
      /* Sub-stream 0: ma over `tempBuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(tempBuffer, 0, (outIdx - 1) + 1), 0, optInFastD_Period, optInFastD_MAType);
      retCode = movingAverageUnguarded(0, outIdx - 1, tempBuffer, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, sc_outFastD);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         if( (bufferIsAllocated) != 0 ) {
         }
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
      System.arraycopy(tempBuffer, lookbackFastD, sc_outFastK, 0, (int)outNBElement.value * 1);
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
      sp.optInFastD_Period = optInFastD_Period;
      sp.optInFastD_MAType = optInFastD_MAType;
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
      sp.cur_outFastK = sc_outFastK[outNBElement.value - 1];
      sp.cur_outFastD = sc_outFastD[outNBElement.value - 1];
      sp.cachedValue = new StochFStream.Value(sp.cur_outFastK, sp.cur_outFastD);
      System.arraycopy(sc_outFastK, 0, outFastK, 0, outNBElement.value);
      System.arraycopy(sc_outFastD, 0, outFastD, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind stochFOpen (composition seam). */
   StochFStream stochFOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      StochFStream sp = new StochFStream(this);
      RetCode retCode = stochFOpenBody(sp, inHigh, inLow, inClose, startIdx, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STOCHF open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STOCHF open: internal error");
      }
      throw new IllegalArgumentException("TA_STOCHF open: " + retCode);
   }
   /**
    * Open a live STOCHF stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#stochF} at that bar.
    * <p>The history must hold at least {@code stochFLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public StochFStream stochFOpen( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType )
   {
      return stochFOpenInternal(inHigh, inLow, inClose, 0, optInFastK_Period, optInFastD_Period, optInFastD_MAType);
   }
   /**
    * {@link Core#stochFOpen} that also fills the output array(s) bit-identically
    * to {@link Core#stochF} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public StochFStream stochFOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastK_Period, int optInFastD_Period, MAType optInFastD_MAType, MInteger outBegIdx, MInteger outNBElement, double outFastK[], double outFastD[] )
   {
      StochFStream sp = new StochFStream(this);
      RetCode retCode = stochFOpenAndFillBody(sp, inHigh, inLow, inClose, optInFastK_Period, optInFastD_Period, optInFastD_MAType, outBegIdx, outNBElement, outFastK, outFastD);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_STOCHF openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_STOCHF openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_STOCHF openAndFill: " + retCode);
   }
