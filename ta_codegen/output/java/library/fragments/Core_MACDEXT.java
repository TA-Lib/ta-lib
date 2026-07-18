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
/**** Streaming API *****/

   /**
    * A live MACDEXT stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#macdExt} over the same series.
    * Open with {@link Core#macdExtOpen}; there is no close — the handle is
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
   public static final class MacdExtStream {
      final Core core;
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
      MovingAverageStream sub0;
      MovingAverageStream sub1;
      MovingAverageStream sub2;

      MacdExtStream( Core core ) { this.core = core; }

      MacdExtStream( MacdExtStream other ) {
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
         this.sub0 = new MovingAverageStream(other.sub0);
         this.sub1 = new MovingAverageStream(other.sub1);
         this.sub2 = new MovingAverageStream(other.sub2);
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double macd;
         public final double macdSignal;
         public final double macdHist;
         Value( double macd, double macdSignal, double macdHist ) {
            this.macd = macd;
            this.macdSignal = macdSignal;
            this.macdHist = macdHist;
         }
         @Override public String toString() {
            return "Value[" + "macd=" + macd + ", " + "macdSignal=" + macdSignal + ", " + "macdHist=" + macdHist + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.macd) == Double.doubleToLongBits(v.macd) && Double.doubleToLongBits(this.macdSignal) == Double.doubleToLongBits(v.macdSignal) && Double.doubleToLongBits(this.macdHist) == Double.doubleToLongBits(v.macdHist);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(macd);
            h = 31 * h + Double.hashCode(macdSignal);
            h = 31 * h + Double.hashCode(macdHist);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.macdExtStreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outMACD, this.cur_outMACDSignal, this.cur_outMACDHist);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inReal ) {
         MacdExtStream scratch = new MacdExtStream(this);
         core.macdExtStreamStep(scratch, inReal);
         return new Value(scratch.cur_outMACD, scratch.cur_outMACDSignal, scratch.cur_outMACDHist);
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
      public MacdExtStream copy() {
         return new MacdExtStream(this);
      }
   }
   void macdExtStreamStep( MacdExtStream sp, double inReal )
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
   private RetCode macdExtOpenBody( MacdExtStream sp, double inReal[], int startIdx, int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
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
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      double[] sc_outMACD = new double[historyLen];
      double[] sc_outMACDSignal = new double[historyLen];
      double[] sc_outMACDHist = new double[historyLen];
      /* An all-EMA MACDEXT computes exactly what MACD computes. Delegate
       * to its single-pass implementation. Period 1 stays on the generic
       * path: ma() copies the input for it instead of running an EMA
       * recursion.
       */
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
         return RetCode.OutOfRangeEndIndex ;
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
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), tempInteger, optInSlowPeriod, optInSlowMAType);
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the fast MA. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub1 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), tempInteger, optInFastPeriod, optInFastMAType);
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
      System.arraycopy(fastMABuffer, lookbackSignal, sc_outMACD, 0, (endIdx - startIdx + 1) * 1);
      /* Calculate the signal/trigger line. */
      /* Sub-stream 2: ma over `fastMABuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub2 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(fastMABuffer, 0, (outNbElement1.value - 1) + 1), 0, optInSignalPeriod, optInSignalMAType);
      retCode = movingAverageUnguarded(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, sc_outMACDSignal);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the histogram. */
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         sc_outMACDHist[i] = sc_outMACD[i] - sc_outMACDSignal[i];
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
      sp.cachedValue = new MacdExtStream.Value(sp.cur_outMACD, sp.cur_outMACDSignal, sp.cur_outMACDHist);
      return RetCode.Success;
   }
   private RetCode macdExtOpenAndFillBody( MacdExtStream sp, double inReal[], int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
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
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( (Object)outMACD == (Object)inReal || (Object)outMACDSignal == (Object)inReal || (Object)outMACDHist == (Object)inReal || (Object)outMACD == (Object)outMACDSignal || (Object)outMACD == (Object)outMACDHist || (Object)outMACDSignal == (Object)outMACDHist ) {
         return RetCode.BadParam;
      }
      double[] sc_outMACD = new double[historyLen];
      double[] sc_outMACDSignal = new double[historyLen];
      double[] sc_outMACDHist = new double[historyLen];
      /* An all-EMA MACDEXT computes exactly what MACD computes. Delegate
       * to its single-pass implementation. Period 1 stays on the generic
       * path: ma() copies the input for it instead of running an EMA
       * recursion.
       */
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
         return RetCode.OutOfRangeEndIndex ;
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
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), tempInteger, optInSlowPeriod, optInSlowMAType);
      retCode = movingAverageUnguarded(tempInteger, endIdx, inReal, optInSlowPeriod, optInSlowMAType, outBegIdx1, outNbElement1, slowMABuffer);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the fast MA. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub1 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), tempInteger, optInFastPeriod, optInFastMAType);
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
      System.arraycopy(fastMABuffer, lookbackSignal, sc_outMACD, 0, (endIdx - startIdx + 1) * 1);
      /* Calculate the signal/trigger line. */
      /* Sub-stream 2: ma over `fastMABuffer`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub2 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(fastMABuffer, 0, (outNbElement1.value - 1) + 1), 0, optInSignalPeriod, optInSignalMAType);
      retCode = movingAverageUnguarded(0, outNbElement1.value - 1, fastMABuffer, optInSignalPeriod, optInSignalMAType, outBegIdx2, outNbElement2, sc_outMACDSignal);
      if( retCode != RetCode.Success ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return retCode ;
      }
      /* Calculate the histogram. */
      for( i = 0; i < outNbElement2.value; i += 1 ) {
         sc_outMACDHist[i] = sc_outMACD[i] - sc_outMACDSignal[i];
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outNbElement2.value;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
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
      sp.cachedValue = new MacdExtStream.Value(sp.cur_outMACD, sp.cur_outMACDSignal, sp.cur_outMACDHist);
      System.arraycopy(sc_outMACD, 0, outMACD, 0, outNBElement.value);
      System.arraycopy(sc_outMACDSignal, 0, outMACDSignal, 0, outNBElement.value);
      System.arraycopy(sc_outMACDHist, 0, outMACDHist, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind macdExtOpen (composition seam). */
   MacdExtStream macdExtOpenInternal( double inReal[], int startIdx, int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
   {
      MacdExtStream sp = new MacdExtStream(this);
      RetCode retCode = macdExtOpenBody(sp, inReal, startIdx, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MACDEXT open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MACDEXT open: internal error");
      }
      throw new IllegalArgumentException("TA_MACDEXT open: " + retCode);
   }
   /**
    * Open a live MACDEXT stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#macdExt} at that bar.
    * <p>The history must hold at least {@code macdExtLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public MacdExtStream macdExtOpen( double inReal[], int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType )
   {
      return macdExtOpenInternal(inReal, 0, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType);
   }
   /**
    * {@link Core#macdExtOpen} that also fills the output array(s) bit-identically
    * to {@link Core#macdExt} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public MacdExtStream macdExtOpenAndFill( double inReal[], int optInFastPeriod, MAType optInFastMAType, int optInSlowPeriod, MAType optInSlowMAType, int optInSignalPeriod, MAType optInSignalMAType, MInteger outBegIdx, MInteger outNBElement, double outMACD[], double outMACDSignal[], double outMACDHist[] )
   {
      MacdExtStream sp = new MacdExtStream(this);
      RetCode retCode = macdExtOpenAndFillBody(sp, inReal, optInFastPeriod, optInFastMAType, optInSlowPeriod, optInSlowMAType, optInSignalPeriod, optInSignalMAType, outBegIdx, outNBElement, outMACD, outMACDSignal, outMACDHist);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_MACDEXT openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_MACDEXT openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_MACDEXT openAndFill: " + retCode);
   }
