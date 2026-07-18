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

   public int rsiLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      int retValue;
      retValue = optInTimePeriod + this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      if( this.compatibility == Compatibility.Metastock ) {
         retValue = retValue - 1;
      }
      return retValue ;

   }
   public RetCode rsi( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      lookbackTotal = (int)rsiLookback(optInTimePeriod);
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
         /* memmove, not memcpy: an in-place caller (outReal == inReal) with
          * startIdx > 0 overlaps source and destination (issue #94; matches WMA).
          */
         System.arraycopy(inReal, startIdx, outReal, 0, i * 1);
         return RetCode.Success ;
      }
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      /* If there is no unstable period,
       * calculate the 'additional' initial
       * price bar who is particuliar to
       * metastock.
       * If there is an unstable period,
       * no need to calculate since this
       * first value will be surely skip.
       */
      if( unstablePeriod == 0 && this.compatibility == Compatibility.Metastock ) {
         /* Preserve prevValue because it may get
          * overwritten by the output.
          * (because output ptr could be the same as input ptr).
          */
         savePrevValue = prevValue;
         /* No unstable period, so must calculate first output
          * particular to Metastock.
          * (Metastock re-use the first price bar, so there
          *  is no loss/gain at first. Beats me why they
          *  are doing all this).
          */
         prevGain = 0.0;
         prevLoss = 0.0;
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
         tempValue1 = prevLoss / (double)optInTimePeriod;
         tempValue2 = prevGain / (double)optInTimePeriod;
         /* Write the output. */
         tempValue1 = tempValue2 + tempValue1;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (tempValue2 / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
         /* Are we done? */
         if( today > endIdx ) {
            outBegIdx.value = startIdx;
            outNBElement.value = outIdx;
            return RetCode.Success ;
         }
         /* Start over for the next price bar. */
         today = today - (int)optInTimePeriod;
         prevValue = savePrevValue;
      }
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
   public RetCode rsiUnguarded( int startIdx,
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = (int)rsiLookback(optInTimePeriod);
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
         System.arraycopy(inReal, startIdx, outReal, 0, i * 1);
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      if( unstablePeriod == 0 && this.compatibility == Compatibility.Metastock ) {
         savePrevValue = prevValue;
         prevGain = 0.0;
         prevLoss = 0.0;
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
         tempValue1 = prevLoss / (double)optInTimePeriod;
         tempValue2 = prevGain / (double)optInTimePeriod;
         tempValue1 = tempValue2 + tempValue1;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (tempValue2 / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
         if( today > endIdx ) {
            outBegIdx.value = startIdx;
            outNBElement.value = outIdx;
            return RetCode.Success ;
         }
         today = today - (int)optInTimePeriod;
         prevValue = savePrevValue;
      }
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
   public RetCode rsi( int startIdx,
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
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = (int)rsiLookback(optInTimePeriod);
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
         System.arraycopy(inReal, startIdx, outReal, 0, i * 1);
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      if( unstablePeriod == 0 && this.compatibility == Compatibility.Metastock ) {
         savePrevValue = prevValue;
         prevGain = 0.0;
         prevLoss = 0.0;
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
         tempValue1 = prevLoss / (double)optInTimePeriod;
         tempValue2 = prevGain / (double)optInTimePeriod;
         tempValue1 = tempValue2 + tempValue1;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (tempValue2 / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
         if( today > endIdx ) {
            outBegIdx.value = startIdx;
            outNBElement.value = outIdx;
            return RetCode.Success ;
         }
         today = today - (int)optInTimePeriod;
         prevValue = savePrevValue;
      }
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
   public RetCode rsiUnguarded( int startIdx,
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
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = (int)rsiLookback(optInTimePeriod);
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
         System.arraycopy(inReal, startIdx, outReal, 0, i * 1);
         return RetCode.Success ;
      }
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      if( unstablePeriod == 0 && this.compatibility == Compatibility.Metastock ) {
         savePrevValue = prevValue;
         prevGain = 0.0;
         prevLoss = 0.0;
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
         tempValue1 = prevLoss / (double)optInTimePeriod;
         tempValue2 = prevGain / (double)optInTimePeriod;
         tempValue1 = tempValue2 + tempValue1;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (tempValue2 / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
         if( today > endIdx ) {
            outBegIdx.value = startIdx;
            outNBElement.value = outIdx;
            return RetCode.Success ;
         }
         today = today - (int)optInTimePeriod;
         prevValue = savePrevValue;
      }
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
/**** Streaming API *****/

   /**
    * A live RSI stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#rsi} over the same series.
    * Open with {@link Core#rsiOpen}; there is no close — the handle is
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
   public static final class RsiStream {
      final Core core;
      int optInTimePeriod;
      double prevGain;
      double prevLoss;
      double prevValue;
      double cur_outReal;

      RsiStream( Core core ) { this.core = core; }

      RsiStream( RsiStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.prevGain = other.prevGain;
         this.prevLoss = other.prevLoss;
         this.prevValue = other.prevValue;
         this.cur_outReal = other.cur_outReal;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public double update( double inReal ) {
         core.rsiStreamStep(this, inReal);
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public double peek( double inReal ) {
         RsiStream scratch = new RsiStream(this);
         core.rsiStreamStep(scratch, inReal);
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
      public RsiStream copy() {
         return new RsiStream(this);
      }
   }
   void rsiStreamStep( RsiStream sp, double inReal )
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
   private RetCode rsiOpenBody( RsiStream sp, double inReal[], int startIdx, int optInTimePeriod )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double lastValue_outReal = 0.0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < rsiLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevGain = 0.0;
         sp.prevLoss = 0.0;
         sp.prevValue = 0.0;
         sp.cur_outReal = inReal[historyLen - 1];
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
      lookbackTotal = (int)rsiLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Trap special case where the period is '1'.
       * In that case, just copy the input into the
       * output for the requested range (as-is !)
       */
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      /* If there is no unstable period,
       * calculate the 'additional' initial
       * price bar who is particuliar to
       * metastock.
       * If there is an unstable period,
       * no need to calculate since this
       * first value will be surely skip.
       */
      if( unstablePeriod == 0 && this.compatibility == Compatibility.Metastock ) {
         /* Preserve prevValue because it may get
          * overwritten by the output.
          * (because output ptr could be the same as input ptr).
          */
         savePrevValue = prevValue;
         /* No unstable period, so must calculate first output
          * particular to Metastock.
          * (Metastock re-use the first price bar, so there
          *  is no loss/gain at first. Beats me why they
          *  are doing all this).
          */
         prevGain = 0.0;
         prevLoss = 0.0;
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
         tempValue1 = prevLoss / (double)optInTimePeriod;
         tempValue2 = prevGain / (double)optInTimePeriod;
         /* Write the output. */
         tempValue1 = tempValue2 + tempValue1;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            lastValue_outReal = 100.0 * (tempValue2 / tempValue1);
            outIdx = outIdx + 1;
         } else {
            lastValue_outReal = 0.0;
            outIdx = outIdx + 1;
         }
         /* Are we done? */
         if( today > endIdx ) {
            outBegIdx.value = startIdx;
            outNBElement.value = outIdx;
            return RetCode.OutOfRangeEndIndex ;
         }
         /* Start over for the next price bar. */
         today = today - (int)optInTimePeriod;
         prevValue = savePrevValue;
      }
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
            lastValue_outReal = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            lastValue_outReal = 0.0;
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
            lastValue_outReal = 100.0 * (prevGain / tempValue1);
            outIdx = outIdx + 1;
         } else {
            lastValue_outReal = 0.0;
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
      sp.cur_outReal = lastValue_outReal;
      return RetCode.Success;
   }
   private RetCode rsiOpenAndFillBody( RsiStream sp, double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
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
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( (Object)outReal == (Object)inReal ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod == 1 ) {
         if( historyLen < rsiLookback(optInTimePeriod) + 1 ) {
            return RetCode.OutOfRangeEndIndex;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.prevGain = 0.0;
         sp.prevLoss = 0.0;
         sp.prevValue = 0.0;
         int fillLb = rsiLookback(optInTimePeriod);
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
            outReal[fillIdx] = inReal[fillLb + fillIdx];
         }
         sp.cur_outReal = outReal[outNBElement.value - 1];
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
      lookbackTotal = (int)rsiLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      /* Make sure there is still something to evaluate. */
      if( startIdx > endIdx ) {
         return RetCode.OutOfRangeEndIndex ;
      }
      outIdx = 0;
      /* Index into the output. */
      /* Trap special case where the period is '1'.
       * In that case, just copy the input into the
       * output for the requested range (as-is !)
       */
      /* Accumulate Wilder's "Average Gain" and "Average Loss"
       * among the initial period.
       */
      today = startIdx - lookbackTotal;
      prevValue = (double)inReal[today];
      unstablePeriod = this.unstablePeriod[FuncUnstId.Rsi.ordinal()];
      /* If there is no unstable period,
       * calculate the 'additional' initial
       * price bar who is particuliar to
       * metastock.
       * If there is an unstable period,
       * no need to calculate since this
       * first value will be surely skip.
       */
      if( unstablePeriod == 0 && this.compatibility == Compatibility.Metastock ) {
         /* Preserve prevValue because it may get
          * overwritten by the output.
          * (because output ptr could be the same as input ptr).
          */
         savePrevValue = prevValue;
         /* No unstable period, so must calculate first output
          * particular to Metastock.
          * (Metastock re-use the first price bar, so there
          *  is no loss/gain at first. Beats me why they
          *  are doing all this).
          */
         prevGain = 0.0;
         prevLoss = 0.0;
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
         tempValue1 = prevLoss / (double)optInTimePeriod;
         tempValue2 = prevGain / (double)optInTimePeriod;
         /* Write the output. */
         tempValue1 = tempValue2 + tempValue1;
         if( !((-0.00000000000001 < tempValue1) && (tempValue1 < 0.00000000000001)) ) {
            outReal[outIdx] = 100.0 * (tempValue2 / tempValue1);
            outIdx = outIdx + 1;
         } else {
            outReal[outIdx] = 0.0;
            outIdx = outIdx + 1;
         }
         /* Are we done? */
         if( today > endIdx ) {
            outBegIdx.value = startIdx;
            outNBElement.value = outIdx;
            return RetCode.OutOfRangeEndIndex ;
         }
         /* Start over for the next price bar. */
         today = today - (int)optInTimePeriod;
         prevValue = savePrevValue;
      }
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
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.prevGain = prevGain;
      sp.prevLoss = prevLoss;
      sp.prevValue = prevValue;
      sp.cur_outReal = outReal[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind rsiOpen (composition seam). */
   RsiStream rsiOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      RsiStream sp = new RsiStream(this);
      RetCode retCode = rsiOpenBody(sp, inReal, startIdx, optInTimePeriod);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_RSI open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_RSI open: internal error");
      }
      throw new IllegalArgumentException("TA_RSI open: " + retCode);
   }
   /**
    * Open a live RSI stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#rsi} at that bar.
    * <p>The history must hold at least {@code rsiLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public RsiStream rsiOpen( double inReal[], int optInTimePeriod )
   {
      return rsiOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#rsiOpen} that also fills the output array(s) bit-identically
    * to {@link Core#rsi} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public RsiStream rsiOpenAndFill( double inReal[], int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      RsiStream sp = new RsiStream(this);
      RetCode retCode = rsiOpenAndFillBody(sp, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_RSI openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_RSI openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_RSI openAndFill: " + retCode);
   }
