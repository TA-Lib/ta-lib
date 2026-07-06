/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112400 MF   Template creation.
 *  052603 MF   Adapt code to compile with .NET Managed C++
 *  020605 AA   Fix #1117666 Lookback bug.
 */

   public int ppoLookback( int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
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
      /* Lookback is driven by the slowest MA. */
      return movingAverageLookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) ;

   }
   public RetCode ppo( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MAType optInMAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int i = 0;
      int j = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, outBegIdx2, outNbElement2, tempBuffer);
      if( retCode == RetCode.Success ) {
         /* Calculate the slow MA into the output. */
         retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx1, outNbElement1, outReal);
         if( retCode == RetCode.Success ) {
            /* The slow MA begins at or after the fast MA, so the offset is
             * valid whenever the slow MA produced output. Guard it so the empty
             * case leaves the difference loop untouched.
             */
            if( outNbElement1.value > 0 ) {
               tempInteger = outBegIdx1.value - outBegIdx2.value;
               /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
               for( i = 0, j = tempInteger; i < outNbElement1.value; i += 1, j += 1 ) {
                  tempReal = outReal[i];
                  if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                     outReal[i] = (tempBuffer[j] - tempReal) / tempReal * 100.0;
                  } else {
                     outReal[i] = 0.0;
                  }
               }
            }
            outBegIdx.value = outBegIdx1.value;
            outNBElement.value = outNbElement1.value;
         }
      }
      return retCode ;
   }
   public RetCode ppoUnguarded( int startIdx,
                                int endIdx,
                                double inReal[],
                                int optInFastPeriod,
                                int optInSlowPeriod,
                                MAType optInMAType,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int i = 0;
      int j = 0;
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, outBegIdx2, outNbElement2, tempBuffer);
      if( retCode == RetCode.Success ) {
         retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx1, outNbElement1, outReal);
         if( retCode == RetCode.Success ) {
            if( outNbElement1.value > 0 ) {
               tempInteger = outBegIdx1.value - outBegIdx2.value;
               for( i = 0, j = tempInteger; i < outNbElement1.value; i += 1, j += 1 ) {
                  tempReal = outReal[i];
                  if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                     outReal[i] = (tempBuffer[j] - tempReal) / tempReal * 100.0;
                  } else {
                     outReal[i] = 0.0;
                  }
               }
            }
            outBegIdx.value = outBegIdx1.value;
            outNBElement.value = outNbElement1.value;
         }
      }
      return retCode ;
   }
   public RetCode ppo( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInFastPeriod,
                       int optInSlowPeriod,
                       MAType optInMAType,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int i = 0;
      int j = 0;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
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
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, outBegIdx2, outNbElement2, tempBuffer);
      if( retCode == RetCode.Success ) {
         retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx1, outNbElement1, outReal);
         if( retCode == RetCode.Success ) {
            if( outNbElement1.value > 0 ) {
               tempInteger = outBegIdx1.value - outBegIdx2.value;
               for( i = 0, j = tempInteger; i < outNbElement1.value; i += 1, j += 1 ) {
                  tempReal = outReal[i];
                  if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                     outReal[i] = (tempBuffer[j] - tempReal) / tempReal * 100.0;
                  } else {
                     outReal[i] = 0.0;
                  }
               }
            }
            outBegIdx.value = outBegIdx1.value;
            outNBElement.value = outNbElement1.value;
         }
      }
      return retCode ;
   }
   public RetCode ppoUnguarded( int startIdx,
                                int endIdx,
                                float inReal[],
                                int optInFastPeriod,
                                int optInSlowPeriod,
                                MAType optInMAType,
                                MInteger outBegIdx,
                                MInteger outNBElement,
                                double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger outBegIdx1 = new MInteger();
      MInteger outNbElement1 = new MInteger();
      MInteger outBegIdx2 = new MInteger();
      MInteger outNbElement2 = new MInteger();
      int i = 0;
      int j = 0;
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, outBegIdx2, outNbElement2, tempBuffer);
      if( retCode == RetCode.Success ) {
         retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outBegIdx1, outNbElement1, outReal);
         if( retCode == RetCode.Success ) {
            if( outNbElement1.value > 0 ) {
               tempInteger = outBegIdx1.value - outBegIdx2.value;
               for( i = 0, j = tempInteger; i < outNbElement1.value; i += 1, j += 1 ) {
                  tempReal = outReal[i];
                  if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
                     outReal[i] = (tempBuffer[j] - tempReal) / tempReal * 100.0;
                  } else {
                     outReal[i] = 0.0;
                  }
               }
            }
            outBegIdx.value = outBegIdx1.value;
            outNBElement.value = outNbElement1.value;
         }
      }
      return retCode ;
   }
