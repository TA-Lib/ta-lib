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

   public int macdExtLookback( int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
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
      int lookbackLargest;
      /* Find the MA with the largest lookback */
      lookbackLargest = movingAverageLookback(optInFastPeriod, optInFastMAType);
      tempInteger = movingAverageLookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      /* Add to the largest MA lookback the signal line lookback */
      return lookbackLargest + movingAverageLookback(optInSignalPeriod, optInSignalMAType) ;

   }
   public RetCode macdExt( int startIdx,
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
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      /* An all-EMA MACDEXT computes exactly what MACD computes. Delegate
       * to its single-pass implementation. Period 1 stays on the generic
       * path: ma() copies the input for it instead of running an EMA
       * recursion.
       */
      if( optInFastMAType == MAType.Ema && optInSlowMAType == MAType.Ema && optInSignalMAType == MAType.Ema && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2 ) {
         return macdUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
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
      lookbackLargest = movingAverageLookback(optInFastPeriod, optInFastMAType);
      tempInteger = movingAverageLookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      /* Add the lookback needed for the signal line */
      lookbackSignal = movingAverageLookback(optInSignalPeriod, optInSignalMAType);
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
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the fast MA. */
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInFastPeriod, optInFastMAType, outBegIdx2, outNbElement2, fastMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
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
      retCode = movingAverageUnguarded(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, outMACDSignal);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the histogram. */
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         outMACDHist[i] = outMACD[i] - outMACDSignal[i];
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      return RetCode.Success ;
   }
   public RetCode macdExtUnguarded( int startIdx,
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
      if( optInFastMAType == MAType.Ema && optInSlowMAType == MAType.Ema && optInSignalMAType == MAType.Ema && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2 ) {
         return macdUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
         tempMAType = optInSlowMAType;
         optInSlowMAType = optInFastMAType;
         optInFastMAType = tempMAType;
      }
      lookbackLargest = movingAverageLookback(optInFastPeriod, optInFastMAType);
      tempInteger = movingAverageLookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      lookbackSignal = movingAverageLookback(optInSignalPeriod, optInSignalMAType);
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
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInFastPeriod, optInFastMAType, outBegIdx2, outNbElement2, fastMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      if( outBegIdx1.value != tempInteger || outBegIdx2.value != tempInteger || outNbElement1.value != outNbElement2.value || outNbElement1.value != endIdx - startIdx + 1 + lookbackSignal ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      for( i = 0; i < outNbElement1.value; i += 1 ) {
         fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
      }
      System.arraycopy(fastMABuffer, lookbackSignal, outMACD, 0, (endIdx - startIdx + 1) * 1);
      retCode = movingAverageUnguarded(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, outMACDSignal);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         outMACDHist[i] = outMACD[i] - outMACDSignal[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      return RetCode.Success ;
   }
   public RetCode macdExt( int startIdx,
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
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( outMACD == outMACDSignal || outMACD == outMACDHist || outMACDSignal == outMACDHist ) {
         return RetCode.BadParam ;
      }
      if( optInFastMAType == MAType.Ema && optInSlowMAType == MAType.Ema && optInSignalMAType == MAType.Ema && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2 ) {
         return macdUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
         tempMAType = optInSlowMAType;
         optInSlowMAType = optInFastMAType;
         optInFastMAType = tempMAType;
      }
      lookbackLargest = movingAverageLookback(optInFastPeriod, optInFastMAType);
      tempInteger = movingAverageLookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      lookbackSignal = movingAverageLookback(optInSignalPeriod, optInSignalMAType);
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
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInFastPeriod, optInFastMAType, outBegIdx2, outNbElement2, fastMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      if( outBegIdx1.value != tempInteger || outBegIdx2.value != tempInteger || outNbElement1.value != outNbElement2.value || outNbElement1.value != endIdx - startIdx + 1 + lookbackSignal ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      for( i = 0; i < outNbElement1.value; i += 1 ) {
         fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
      }
      System.arraycopy(fastMABuffer, lookbackSignal, outMACD, 0, (endIdx - startIdx + 1) * 1);
      retCode = movingAverageUnguarded(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, outMACDSignal);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         outMACDHist[i] = outMACD[i] - outMACDSignal[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      return RetCode.Success ;
   }
   public RetCode macdExtUnguarded( int startIdx,
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
      if( optInFastMAType == MAType.Ema && optInSlowMAType == MAType.Ema && optInSignalMAType == MAType.Ema && optInFastPeriod >= 2 && optInSlowPeriod >= 2 && optInSignalPeriod >= 2 ) {
         return macdUnguarded(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist) ;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
         tempMAType = optInSlowMAType;
         optInSlowMAType = optInFastMAType;
         optInFastMAType = tempMAType;
      }
      lookbackLargest = movingAverageLookback(optInFastPeriod, optInFastMAType);
      tempInteger = movingAverageLookback(optInSlowPeriod, optInSlowMAType);
      if( tempInteger > lookbackLargest ) {
         lookbackLargest = tempInteger;
      }
      lookbackSignal = movingAverageLookback(optInSignalPeriod, optInSignalMAType);
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
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInFastPeriod, optInFastMAType, outBegIdx2, outNbElement2, fastMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      if( outBegIdx1.value != tempInteger || outBegIdx2.value != tempInteger || outNbElement1.value != outNbElement2.value || outNbElement1.value != endIdx - startIdx + 1 + lookbackSignal ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.BadParam ;
      }
      for( i = 0; i < outNbElement1.value; i += 1 ) {
         fastMABuffer[i] = fastMABuffer[i] - slowMABuffer[i];
      }
      System.arraycopy(fastMABuffer, lookbackSignal, outMACD, 0, (endIdx - startIdx + 1) * 1);
      retCode = movingAverageUnguarded(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, outMACDSignal);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         outMACDHist[i] = outMACD[i] - outMACDSignal[i];
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      return RetCode.Success ;
   }
