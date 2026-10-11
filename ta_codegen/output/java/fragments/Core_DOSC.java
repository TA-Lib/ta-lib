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
 *  100626 MF,CC  Initial version (#479).
 */

   /**
    * Number of leading input bars {@link Core#dosc} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Period of the RSI (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFirstPeriod Period of the first smoothing, applied to the RSI
    *        (default 5; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 3; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Period of the simple average subtracted from the
    *        smoothed line (default 9; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int doscLookback( int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 5;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return -1;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 3;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return -1;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return -1;
      }
      /* Wilder's RSI, the two exponential smoothings stacked on it and the
       * simple average taken over the result. Every term is exactly the lookback
       * of the function it comes from, so none of them is restated here -- which
       * is what makes DOSC inherit TA_FUNC_UNST_RSI and TA_FUNC_UNST_EMA from its
       * callees rather than take an id of its own, and what carries the Auto
       * warm-up levels of #492 through all three of them.
       *
       * The EMA term appears TWICE, once per smoothing stage, so a warm
       * TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, k) moves the lookback by 2k and
       * not by k.
       */
      return rsiLookback(optInTimePeriod) + emaLookback(optInFirstPeriod) + emaLookback(optInSecondPeriod) + smaLookback(optInSignalPeriod) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#dosc}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Period of the RSI (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFirstPeriod Period of the first smoothing, applied to the RSI
    *        (default 5; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 3; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Period of the simple average subtracted from the
    *        smoothed line (default 9; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int doscDisplayShift( int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod, int outputIdx )
   {
      if( doscLookback( optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode doscImpl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     int optInFirstPeriod,
                     int optInSecondPeriod,
                     int optInSignalPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double k1 = 0;
      double beta1 = 0;
      double k2 = 0;
      double beta2 = 0;
      double invPeriod = 0;
      double prevGain = 0;
      double prevLoss = 0;
      double prevValue = 0;
      double gainDelta = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double rsiValue = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      double sumSignal = 0;
      int lookbackTotal = 0;
      int lookbackRSI = 0;
      int lookbackEMA1 = 0;
      int lookbackEMA2 = 0;
      int skipRSI = 0;
      int today = 0;
      int i = 0;
      int outIdx = 0;
      int rsiBar = 0;
      int nRsi = 0;
      int n1 = 0;
      int n2 = 0;
      double[] dsBuffer;
      int dsBuffer_Idx = 0;
      int maxIdx_dsBuffer = (32)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 5;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 3;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      /* Constance Brown's "triple smoothed derivative of RSI plotted as a
       * histogram" (MTA Journal, 1994): MACD's histogram construction applied to
       * a double-smoothed RSI.
       *
       *    S1_t   = EMA(RSI(x, t), f)_t
       *    DS_t   = EMA(S1, s)_t
       *    DOSC_t = DS_t - SMA(DS, g)_t
       *
       * This walks the chain in one pass, with each stage's arithmetic spelled
       * exactly as its callee spells it: rsi.c's Wilder recursion, ema.c's seed
       * and step, and sma.c's add-new / snapshot / subtract-old running sum. The
       * intermediate series are never materialised -- the double-smoothed line
       * goes straight into a ring of the last `signal` values -- and the result
       * is bit-identical to TA_RSI -> TA_EMA -> TA_EMA -> TA_SMA -> TA_SUB
       * rather than merely close, which is what the composition gate holds.
       *
       * Every stage boundary below is the callee's LOOKBACK, not (period-1), so
       * each stage seeds on the values its predecessor would have published and a
       * warm unstable period folds in. The counters are compared BEFORE they are
       * subtracted, never after: written as `n = nRsi - skipRSI; if( n >= 0 )`
       * this is correct in C, where the counters are signed, and broken in the
       * Rust backend, which renders them usize (the lesson smi.c records).
       */
      /* This ptr will point on a circular buffer of at least
       * "optInSignalPeriod" element.
       */
      lookbackTotal = doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
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
         return RetCode.SUCCESS ;
      }
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      dsBuffer = new double[optInSignalPeriod];
      maxIdx_dsBuffer = (optInSignalPeriod)-1;
      dsBuffer_Idx = 0;
      lookbackRSI = rsiLookback(optInTimePeriod);
      lookbackEMA1 = emaLookback(optInFirstPeriod);
      lookbackEMA2 = emaLookback(optInSecondPeriod);
      /* The RSI values the composed chain never publishes: TA_RSI entered at the
       * first bar the first smoothing needs has already consumed its own
       * unstable period by then. Taking the difference of the two lookbacks
       * rather than reading TA_GetUnstablePeriod is what keeps this correct under
       * the Auto levels, where the count is a function of the period.
       */
      skipRSI = lookbackRSI - optInTimePeriod;
      /* ema.c's constants: k and beta must sum to exactly 1.0, or a flat input
       * drifts off its level.
       */
      beta1 = (double)(optInFirstPeriod - 1) / (double)(optInFirstPeriod + 1);
      k1 = 1.0 - beta1;
      beta1 = 1.0 - k1;
      beta2 = (double)(optInSecondPeriod - 1) / (double)(optInSecondPeriod + 1);
      k2 = 1.0 - beta2;
      beta2 = 1.0 - k2;
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      sumSignal = 0.0;
      nRsi = 0;
      /* Wilder's seed, exactly as rsi.c accumulates it: one simple sum of the
       * first optInTimePeriod changes, each side taken unconditionally, then
       * both scaled by 1/period.
       */
      invPeriod = 1.0 / (double)optInTimePeriod;
      today = startIdx - lookbackTotal;
      rsiBar = today + optInTimePeriod;
      prevValue = inReal[today];
      prevGain = 0.0;
      prevLoss = 0.0;
      today = today + 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
      }
      prevLoss *= invPeriod;
      prevGain *= invPeriod;
      /* rsi.c answers the neutral 50 when neither a gain nor a loss has been seen
       * since the seed, the 0/0 case of issue #480; the one-sided cases are 0 and
       * 100 and reach the division.
       */
      tempValue1 = prevGain + prevLoss;
      if( tempValue1 > 0.0 ) {
         rsiValue = 100.0 * (prevGain / tempValue1);
      } else {
         rsiValue = 50.0;
      }
      /* Warm-up. Feeds every RSI value before startIdx through the chain and
       * leaves rsiValue holding startIdx's own. Nothing is emitted here: the
       * first bar whose signal window is full is startIdx, by construction of
       * the lookback.
       */
      while( rsiBar < startIdx ) {
         if( nRsi >= skipRSI ) {
            n1 = nRsi - skipRSI;
            if( n1 < optInFirstPeriod ) {
               sum1 = sum1 + rsiValue;
               if( n1 == optInFirstPeriod - 1 ) {
                  ema1 = sum1 / optInFirstPeriod;
               }
            } else {
               ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
            }
            if( n1 >= lookbackEMA1 ) {
               n2 = n1 - lookbackEMA1;
               if( n2 < optInSecondPeriod ) {
                  sum2 = sum2 + ema1;
                  if( n2 == optInSecondPeriod - 1 ) {
                     ema2 = sum2 / optInSecondPeriod;
                  }
               } else {
                  ema2 = Math.fma(beta2, ema2, k2 * ema1);
               }
               if( n2 >= lookbackEMA2 ) {
                  dsBuffer[dsBuffer_Idx] = ema2;
                  sumSignal = sumSignal + ema2;
                  dsBuffer_Idx++;
                  if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
               }
            }
         }
         nRsi = nRsi + 1;
         tempValue1 = inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= invPeriod;
         prevGain *= invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         rsiBar = rsiBar + 1;
      }
      /* The first output. Every stage is past its seed here -- the shortest
       * reachable case, periods 2/2/2/2, still arrives with n1 = 3, n2 = 2 and a
       * signal window one short of full -- so from this bar on the chain is three
       * pure recursions and a running sum, with nothing left to branch on. That
       * is what keeps the managed peek frames from carrying a seeded output
       * local: the store below and the one in the stable loop always run.
       */
      ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
      ema2 = Math.fma(beta2, ema2, k2 * ema1);
      dsBuffer[dsBuffer_Idx] = ema2;
      sumSignal = sumSignal + ema2;
      outReal[0] = ema2 - sumSignal / (double)optInSignalPeriod;
      outIdx = 1;
      dsBuffer_Idx++;
      if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
      sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
      /* Stable zone. */
      while( today <= endIdx ) {
         tempValue1 = inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= invPeriod;
         prevGain *= invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
         ema2 = Math.fma(beta2, ema2, k2 * ema1);
         dsBuffer[dsBuffer_Idx] = ema2;
         sumSignal = sumSignal + ema2;
         outReal[outIdx] = ema2 - sumSignal / (double)optInSignalPeriod;
         outIdx = outIdx + 1;
         dsBuffer_Idx++;
         if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
         sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode doscImpl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     int optInFirstPeriod,
                     int optInSecondPeriod,
                     int optInSignalPeriod,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double k1 = 0;
      double beta1 = 0;
      double k2 = 0;
      double beta2 = 0;
      double invPeriod = 0;
      double prevGain = 0;
      double prevLoss = 0;
      double prevValue = 0;
      double gainDelta = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double rsiValue = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      double sumSignal = 0;
      int lookbackTotal = 0;
      int lookbackRSI = 0;
      int lookbackEMA1 = 0;
      int lookbackEMA2 = 0;
      int skipRSI = 0;
      int today = 0;
      int i = 0;
      int outIdx = 0;
      int rsiBar = 0;
      int nRsi = 0;
      int n1 = 0;
      int n2 = 0;
      double[] dsBuffer;
      int dsBuffer_Idx = 0;
      int maxIdx_dsBuffer = (32)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 5;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 3;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      dsBuffer = new double[optInSignalPeriod];
      maxIdx_dsBuffer = (optInSignalPeriod)-1;
      dsBuffer_Idx = 0;
      lookbackRSI = rsiLookback(optInTimePeriod);
      lookbackEMA1 = emaLookback(optInFirstPeriod);
      lookbackEMA2 = emaLookback(optInSecondPeriod);
      skipRSI = lookbackRSI - optInTimePeriod;
      beta1 = (double)(optInFirstPeriod - 1) / (double)(optInFirstPeriod + 1);
      k1 = 1.0 - beta1;
      beta1 = 1.0 - k1;
      beta2 = (double)(optInSecondPeriod - 1) / (double)(optInSecondPeriod + 1);
      k2 = 1.0 - beta2;
      beta2 = 1.0 - k2;
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      sumSignal = 0.0;
      nRsi = 0;
      invPeriod = 1.0 / (double)optInTimePeriod;
      today = startIdx - lookbackTotal;
      rsiBar = today + optInTimePeriod;
      prevValue = (double)inReal[today];
      prevGain = 0.0;
      prevLoss = 0.0;
      today = today + 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
      }
      prevLoss *= invPeriod;
      prevGain *= invPeriod;
      tempValue1 = prevGain + prevLoss;
      if( tempValue1 > 0.0 ) {
         rsiValue = 100.0 * (prevGain / tempValue1);
      } else {
         rsiValue = 50.0;
      }
      while( rsiBar < startIdx ) {
         if( nRsi >= skipRSI ) {
            n1 = nRsi - skipRSI;
            if( n1 < optInFirstPeriod ) {
               sum1 = sum1 + rsiValue;
               if( n1 == optInFirstPeriod - 1 ) {
                  ema1 = sum1 / optInFirstPeriod;
               }
            } else {
               ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
            }
            if( n1 >= lookbackEMA1 ) {
               n2 = n1 - lookbackEMA1;
               if( n2 < optInSecondPeriod ) {
                  sum2 = sum2 + ema1;
                  if( n2 == optInSecondPeriod - 1 ) {
                     ema2 = sum2 / optInSecondPeriod;
                  }
               } else {
                  ema2 = Math.fma(beta2, ema2, k2 * ema1);
               }
               if( n2 >= lookbackEMA2 ) {
                  dsBuffer[dsBuffer_Idx] = ema2;
                  sumSignal = sumSignal + ema2;
                  dsBuffer_Idx++;
                  if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
               }
            }
         }
         nRsi = nRsi + 1;
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= invPeriod;
         prevGain *= invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         rsiBar = rsiBar + 1;
      }
      ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
      ema2 = Math.fma(beta2, ema2, k2 * ema1);
      dsBuffer[dsBuffer_Idx] = ema2;
      sumSignal = sumSignal + ema2;
      outReal[0] = ema2 - sumSignal / (double)optInSignalPeriod;
      outIdx = 1;
      dsBuffer_Idx++;
      if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
      sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
      while( today <= endIdx ) {
         tempValue1 = (double)inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= invPeriod;
         prevGain *= invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
         ema2 = Math.fma(beta2, ema2, k2 * ema1);
         dsBuffer[dsBuffer_Idx] = ema2;
         sumSignal = sumSignal + ema2;
         outReal[outIdx] = ema2 - sumSignal / (double)optInSignalPeriod;
         outIdx = outIdx + 1;
         dsBuffer_Idx++;
         if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
         sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages
    * in series, with a simple average of that smoothed line subtracted from it.
    * Constance Brown's reading is that the RSI's own swings are too noisy to
    * time with, so she smooths it twice and then plots the distance from its
    * own average as a histogram — MACD's histogram construction, applied to a
    * smoothed RSI rather than to price. The result is in RSI points, centred on
    * zero: crossings of the zero line mark the turn, and the height measures
    * how far the smoothed RSI has run from its mean.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/dosc">ta-lib.org/functions/dosc</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Every stage is a call to a function TA-Lib already ships, so the output is identical, bit for bit, to {@code TA_RSI} followed by two {@code TA_EMA} calls, a {@code TA_SMA} and a {@code TA_SUB}. The shortest expression of that chain takes five calls and three intermediate buffers; this computes it in one pass without materialising them.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second seeds on what the first publishes. {@code TA_SetUnstablePeriod} on either {@code TA_FUNC_UNST_RSI} or {@code TA_FUNC_UNST_EMA} discards more of that warm-up, and the EMA setting counts twice because there are two exponential stages. Implementations seeding each stage from a single first sample differ over the transient and agree once it decays.</li>
    * <li>The degenerate reading is whatever {@code TA_RSI} answers when neither a gain nor a loss has been seen since the seed. Some other implementations answer 100 there and will disagree over that stretch.</li>
    * <li>The periods are independent: the stages commute, so no ordering between the two smoothing periods is required or checked. A signal period of 1 would make the output identically zero, so the minimum is 2.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#doscLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series, usually the close.
    * @param optInTimePeriod Period of the RSI (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFirstPeriod Period of the first smoothing, applied to the RSI
    *        (default 5; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 3; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Period of the simple average subtracted from the
    *        smoothed line (default 9; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Derivative Oscillator, in RSI points, centred on zero. Must
    *        hold at least {@code endIdx - max(startIdx, doscLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#rsi
    * @see Core#stochrsi
    * @see Core#macd
    * @see Core#ac
    */
   public OutRange dosc( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         int optInFirstPeriod,
                         int optInSecondPeriod,
                         int optInSignalPeriod,
                         double outReal[] )
   {
      requireIndexRange("DOSC", startIdx, endIdx);
      int guardStart = clampedStart("DOSC", startIdx, doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DOSC", "inReal", inReal, guardInLen);
      requireLength("DOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = doscImpl(startIdx, endIdx, inReal, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("DOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages
    * in series, with a simple average of that smoothed line subtracted from it.
    * Constance Brown's reading is that the RSI's own swings are too noisy to
    * time with, so she smooths it twice and then plots the distance from its
    * own average as a histogram — MACD's histogram construction, applied to a
    * smoothed RSI rather than to price. The result is in RSI points, centred on
    * zero: crossings of the zero line mark the turn, and the height measures
    * how far the smoothed RSI has run from its mean.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/dosc">ta-lib.org/functions/dosc</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Every stage is a call to a function TA-Lib already ships, so the output is identical, bit for bit, to {@code TA_RSI} followed by two {@code TA_EMA} calls, a {@code TA_SMA} and a {@code TA_SUB}. The shortest expression of that chain takes five calls and three intermediate buffers; this computes it in one pass without materialising them.</li>
    * <li>Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second seeds on what the first publishes. {@code TA_SetUnstablePeriod} on either {@code TA_FUNC_UNST_RSI} or {@code TA_FUNC_UNST_EMA} discards more of that warm-up, and the EMA setting counts twice because there are two exponential stages. Implementations seeding each stage from a single first sample differ over the transient and agree once it decays.</li>
    * <li>The degenerate reading is whatever {@code TA_RSI} answers when neither a gain nor a loss has been seen since the seed. Some other implementations answer 100 there and will disagree over that stretch.</li>
    * <li>The periods are independent: the stages commute, so no ordering between the two smoothing periods is required or checked. A signal period of 1 would make the output identically zero, so the minimum is 2.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#doscLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input series, usually the close.
    * @param optInTimePeriod Period of the RSI (default 14; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFirstPeriod Period of the first smoothing, applied to the RSI
    *        (default 5; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSecondPeriod Period of the second smoothing, applied to the
    *        first (default 3; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Period of the simple average subtracted from the
    *        smoothed line (default 9; range 2..100000; {@code Integer.MIN_VALUE}
    *        selects the default).
    * @param outReal Derivative Oscillator, in RSI points, centred on zero. Must
    *        hold at least {@code endIdx - max(startIdx, doscLookback(...)) + 1}
    *        values, and never be empty: an empty array is an absent output.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
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
    * @see Core#rsi
    * @see Core#stochrsi
    * @see Core#macd
    * @see Core#ac
    */
   public OutRange dosc( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         int optInFirstPeriod,
                         int optInSecondPeriod,
                         int optInSignalPeriod,
                         double outReal[] )
   {
      requireIndexRange("DOSC", startIdx, endIdx);
      int guardStart = clampedStart("DOSC", startIdx, doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("DOSC", "inReal", inReal, guardInLen);
      requireLength("DOSC", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = doscImpl(startIdx, endIdx, inReal, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("DOSC", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live DOSC stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#dosc} over the same series.
    * Open with {@link Core#doscOpen}; there is no close — the handle is
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
   public static final class DoscStream {
      private Core core;
      private int optInTimePeriod;
      private int optInFirstPeriod;
      private int optInSecondPeriod;
      private int optInSignalPeriod;
      private double k1;
      private double beta1;
      private double k2;
      private double beta2;
      private double invPeriod;
      private double prevGain;
      private double prevLoss;
      private double prevValue;
      private double ema1;
      private double ema2;
      private double sumSignal;
      private int dsBuffer_Idx;
      private int maxIdx_dsBuffer;
      private int cbSize_dsBuffer;
      private double[] cb_dsBuffer;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private DoscStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#dosc} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("DOSC advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private DoscStream( DoscStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInFirstPeriod = other.optInFirstPeriod;
         this.optInSecondPeriod = other.optInSecondPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.k1 = other.k1;
         this.beta1 = other.beta1;
         this.k2 = other.k2;
         this.beta2 = other.beta2;
         this.invPeriod = other.invPeriod;
         this.prevGain = other.prevGain;
         this.prevLoss = other.prevLoss;
         this.prevValue = other.prevValue;
         this.ema1 = other.ema1;
         this.ema2 = other.ema2;
         this.sumSignal = other.sumSignal;
         this.dsBuffer_Idx = other.dsBuffer_Idx;
         this.maxIdx_dsBuffer = other.maxIdx_dsBuffer;
         this.cbSize_dsBuffer = other.cbSize_dsBuffer;
         this.cb_dsBuffer = other.cb_dsBuffer.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("DOSC update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("DOSC update", "inReal");
         core.doscStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("DOSC peek", "inReal");
         DoscStream sp = this;
         double gainDelta = 0.0;
         double tempValue1 = 0.0;
         double tempValue2 = 0.0;
         double rsiValue = 0.0;
         double cur_outReal = 0.0;
         double ema1 = sp.ema1;
         double ema2 = sp.ema2;
         double prevGain = sp.prevGain;
         double prevLoss = sp.prevLoss;
         double prevValue = sp.prevValue;
         double sumSignal = sp.sumSignal;
         tempValue1 = inReal;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(sp.optInTimePeriod - 1);
         prevGain *= (double)(sp.optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= sp.invPeriod;
         prevGain *= sp.invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         ema1 = Math.fma(sp.beta1, ema1, sp.k1 * rsiValue);
         ema2 = Math.fma(sp.beta2, ema2, sp.k2 * ema1);
         sumSignal = sumSignal + ema2;
         cur_outReal = ema2 - sumSignal / (double)sp.optInSignalPeriod;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
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
      public DoscStream clone() {
         return new DoscStream(this);
      }
   }
   private void doscStepImpl( DoscStream sp, double inReal )
   {
      double gainDelta = 0.0;
      double tempValue1 = 0.0;
      double tempValue2 = 0.0;
      double rsiValue = 0.0;
      tempValue1 = inReal;
      tempValue2 = tempValue1 - sp.prevValue;
      sp.prevValue = tempValue1;
      sp.prevLoss *= (double)(sp.optInTimePeriod - 1);
      sp.prevGain *= (double)(sp.optInTimePeriod - 1);
      gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
      sp.prevGain += gainDelta;
      sp.prevLoss += gainDelta - tempValue2;
      sp.prevLoss *= sp.invPeriod;
      sp.prevGain *= sp.invPeriod;
      tempValue1 = sp.prevGain + sp.prevLoss;
      if( tempValue1 > 0.0 ) {
         rsiValue = 100.0 * (sp.prevGain / tempValue1);
      } else {
         rsiValue = 50.0;
      }
      sp.ema1 = Math.fma(sp.beta1, sp.ema1, sp.k1 * rsiValue);
      sp.ema2 = Math.fma(sp.beta2, sp.ema2, sp.k2 * sp.ema1);
      sp.cb_dsBuffer[sp.dsBuffer_Idx] = sp.ema2;
      sp.sumSignal = sp.sumSignal + sp.ema2;
      sp.cur_outReal = sp.ema2 - sp.sumSignal / (double)sp.optInSignalPeriod;
      sp.dsBuffer_Idx = sp.dsBuffer_Idx + 1;
      if( sp.dsBuffer_Idx > sp.maxIdx_dsBuffer ) {
         sp.dsBuffer_Idx = 0;
      }
      sp.sumSignal = sp.sumSignal - sp.cb_dsBuffer[sp.dsBuffer_Idx];
   }
   private RetCode doscOpenImpl( DoscStream sp, double inReal[], int startIdx, int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double k1 = 0;
      double beta1 = 0;
      double k2 = 0;
      double beta2 = 0;
      double invPeriod = 0;
      double prevGain = 0;
      double prevLoss = 0;
      double prevValue = 0;
      double gainDelta = 0;
      double tempValue1 = 0;
      double tempValue2 = 0;
      double rsiValue = 0;
      double ema1 = 0;
      double ema2 = 0;
      double sum1 = 0;
      double sum2 = 0;
      double sumSignal = 0;
      int lookbackTotal = 0;
      int lookbackRSI = 0;
      int lookbackEMA1 = 0;
      int lookbackEMA2 = 0;
      int skipRSI = 0;
      int today = 0;
      int i = 0;
      int outIdx = 0;
      int rsiBar = 0;
      int nRsi = 0;
      int n1 = 0;
      int n2 = 0;
      double[] dsBuffer;
      int dsBuffer_Idx = 0;
      int maxIdx_dsBuffer = (32)-1;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 14;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFirstPeriod == Integer.MIN_VALUE ) {
         optInFirstPeriod = 5;
      } else if( optInFirstPeriod < 2 || optInFirstPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSecondPeriod == Integer.MIN_VALUE ) {
         optInSecondPeriod = 3;
      } else if( optInSecondPeriod < 2 || optInSecondPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 2 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Constance Brown's "triple smoothed derivative of RSI plotted as a
       * histogram" (MTA Journal, 1994): MACD's histogram construction applied to
       * a double-smoothed RSI.
       *
       *    S1_t   = EMA(RSI(x, t), f)_t
       *    DS_t   = EMA(S1, s)_t
       *    DOSC_t = DS_t - SMA(DS, g)_t
       *
       * This walks the chain in one pass, with each stage's arithmetic spelled
       * exactly as its callee spells it: rsi.c's Wilder recursion, ema.c's seed
       * and step, and sma.c's add-new / snapshot / subtract-old running sum. The
       * intermediate series are never materialised -- the double-smoothed line
       * goes straight into a ring of the last `signal` values -- and the result
       * is bit-identical to TA_RSI -> TA_EMA -> TA_EMA -> TA_SMA -> TA_SUB
       * rather than merely close, which is what the composition gate holds.
       *
       * Every stage boundary below is the callee's LOOKBACK, not (period-1), so
       * each stage seeds on the values its predecessor would have published and a
       * warm unstable period folds in. The counters are compared BEFORE they are
       * subtracted, never after: written as `n = nRsi - skipRSI; if( n >= 0 )`
       * this is correct in C, where the counters are signed, and broken in the
       * Rust backend, which renders them usize (the lesson smi.c records).
       */
      /* This ptr will point on a circular buffer of at least
       * "optInSignalPeriod" element.
       */
      lookbackTotal = doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
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
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( optInSignalPeriod < 1 ) return RetCode.INTERNAL_ERROR;
      dsBuffer = new double[optInSignalPeriod];
      maxIdx_dsBuffer = (optInSignalPeriod)-1;
      dsBuffer_Idx = 0;
      lookbackRSI = rsiLookback(optInTimePeriod);
      lookbackEMA1 = emaLookback(optInFirstPeriod);
      lookbackEMA2 = emaLookback(optInSecondPeriod);
      /* The RSI values the composed chain never publishes: TA_RSI entered at the
       * first bar the first smoothing needs has already consumed its own
       * unstable period by then. Taking the difference of the two lookbacks
       * rather than reading TA_GetUnstablePeriod is what keeps this correct under
       * the Auto levels, where the count is a function of the period.
       */
      skipRSI = lookbackRSI - optInTimePeriod;
      /* ema.c's constants: k and beta must sum to exactly 1.0, or a flat input
       * drifts off its level.
       */
      beta1 = (double)(optInFirstPeriod - 1) / (double)(optInFirstPeriod + 1);
      k1 = 1.0 - beta1;
      beta1 = 1.0 - k1;
      beta2 = (double)(optInSecondPeriod - 1) / (double)(optInSecondPeriod + 1);
      k2 = 1.0 - beta2;
      beta2 = 1.0 - k2;
      ema1 = 0.0;
      ema2 = 0.0;
      sum1 = 0.0;
      sum2 = 0.0;
      sumSignal = 0.0;
      nRsi = 0;
      /* Wilder's seed, exactly as rsi.c accumulates it: one simple sum of the
       * first optInTimePeriod changes, each side taken unconditionally, then
       * both scaled by 1/period.
       */
      invPeriod = 1.0 / (double)optInTimePeriod;
      today = startIdx - lookbackTotal;
      rsiBar = today + optInTimePeriod;
      prevValue = inReal[today];
      prevGain = 0.0;
      prevLoss = 0.0;
      today = today + 1;
      for( i = optInTimePeriod; i > 0; i -= 1 ) {
         tempValue1 = inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
      }
      prevLoss *= invPeriod;
      prevGain *= invPeriod;
      /* rsi.c answers the neutral 50 when neither a gain nor a loss has been seen
       * since the seed, the 0/0 case of issue #480; the one-sided cases are 0 and
       * 100 and reach the division.
       */
      tempValue1 = prevGain + prevLoss;
      if( tempValue1 > 0.0 ) {
         rsiValue = 100.0 * (prevGain / tempValue1);
      } else {
         rsiValue = 50.0;
      }
      /* Warm-up. Feeds every RSI value before startIdx through the chain and
       * leaves rsiValue holding startIdx's own. Nothing is emitted here: the
       * first bar whose signal window is full is startIdx, by construction of
       * the lookback.
       */
      while( rsiBar < startIdx ) {
         if( nRsi >= skipRSI ) {
            n1 = nRsi - skipRSI;
            if( n1 < optInFirstPeriod ) {
               sum1 = sum1 + rsiValue;
               if( n1 == optInFirstPeriod - 1 ) {
                  ema1 = sum1 / optInFirstPeriod;
               }
            } else {
               ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
            }
            if( n1 >= lookbackEMA1 ) {
               n2 = n1 - lookbackEMA1;
               if( n2 < optInSecondPeriod ) {
                  sum2 = sum2 + ema1;
                  if( n2 == optInSecondPeriod - 1 ) {
                     ema2 = sum2 / optInSecondPeriod;
                  }
               } else {
                  ema2 = Math.fma(beta2, ema2, k2 * ema1);
               }
               if( n2 >= lookbackEMA2 ) {
                  dsBuffer[dsBuffer_Idx] = ema2;
                  sumSignal = sumSignal + ema2;
                  dsBuffer_Idx++;
                  if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
               }
            }
         }
         nRsi = nRsi + 1;
         tempValue1 = inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= invPeriod;
         prevGain *= invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         rsiBar = rsiBar + 1;
      }
      /* The first output. Every stage is past its seed here -- the shortest
       * reachable case, periods 2/2/2/2, still arrives with n1 = 3, n2 = 2 and a
       * signal window one short of full -- so from this bar on the chain is three
       * pure recursions and a running sum, with nothing left to branch on. That
       * is what keeps the managed peek frames from carrying a seeded output
       * local: the store below and the one in the stable loop always run.
       */
      ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
      ema2 = Math.fma(beta2, ema2, k2 * ema1);
      dsBuffer[dsBuffer_Idx] = ema2;
      sumSignal = sumSignal + ema2;
      outReal[0 * outStride] = ema2 - sumSignal / (double)optInSignalPeriod;
      outIdx = 1;
      dsBuffer_Idx++;
      if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
      sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
      /* Stable zone. */
      while( today <= endIdx ) {
         tempValue1 = inReal[today];
         today = today + 1;
         tempValue2 = tempValue1 - prevValue;
         prevValue = tempValue1;
         prevLoss *= (double)(optInTimePeriod - 1);
         prevGain *= (double)(optInTimePeriod - 1);
         gainDelta = (tempValue2 > 0.0) ? tempValue2 : 0.0;
         prevGain += gainDelta;
         prevLoss += gainDelta - tempValue2;
         prevLoss *= invPeriod;
         prevGain *= invPeriod;
         tempValue1 = prevGain + prevLoss;
         if( tempValue1 > 0.0 ) {
            rsiValue = 100.0 * (prevGain / tempValue1);
         } else {
            rsiValue = 50.0;
         }
         ema1 = Math.fma(beta1, ema1, k1 * rsiValue);
         ema2 = Math.fma(beta2, ema2, k2 * ema1);
         dsBuffer[dsBuffer_Idx] = ema2;
         sumSignal = sumSignal + ema2;
         outReal[outIdx * outStride] = ema2 - sumSignal / (double)optInSignalPeriod;
         outIdx = outIdx + 1;
         dsBuffer_Idx++;
         if( dsBuffer_Idx > maxIdx_dsBuffer ) { dsBuffer_Idx = 0; }
         sumSignal = sumSignal - dsBuffer[dsBuffer_Idx];
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int capCb_dsBuffer = maxIdx_dsBuffer + 1;
      if( capCb_dsBuffer > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInFirstPeriod = optInFirstPeriod;
      sp.optInSecondPeriod = optInSecondPeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.k1 = k1;
      sp.beta1 = beta1;
      sp.k2 = k2;
      sp.beta2 = beta2;
      sp.invPeriod = invPeriod;
      sp.prevGain = prevGain;
      sp.prevLoss = prevLoss;
      sp.prevValue = prevValue;
      sp.ema1 = ema1;
      sp.ema2 = ema2;
      sp.sumSignal = sumSignal;
      sp.dsBuffer_Idx = dsBuffer_Idx;
      sp.maxIdx_dsBuffer = maxIdx_dsBuffer;
      sp.cbSize_dsBuffer = capCb_dsBuffer;
      sp.cb_dsBuffer = dsBuffer;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* doscOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   DoscStream doscOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      DoscStream sp = new DoscStream(this);
      RetCode retCode = doscOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("DOSC openAndFill", inReal.length, startIdx, doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod));
      }
      throw streamFailure("DOSC openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind doscOpen (composition seam). */
   DoscStream doscOpenInternal( double inReal[], int startIdx, int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod )
   {
      DoscStream sp = new DoscStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = doscOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("DOSC open", inReal.length, startIdx, doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod));
      }
      throw streamFailure("DOSC open", retCode);
   }
   /**
    * Open a live DOSC stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#dosc} at that bar.
    * <p>The history must hold at least {@code doscLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public DoscStream doscOpen( double inReal[], int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod )
   {
      requireArgument("DOSC open", "inReal", inReal);
      requireHistory("DOSC open", inReal.length);
      return doscOpenInternal(inReal, 0, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#doscOpen} that also fills the output array(s) bit-identically
    * to {@link Core#dosc} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link DoscStream#outRange()}.
    */
   public DoscStream doscOpenAndFill( double inReal[], int optInTimePeriod, int optInFirstPeriod, int optInSecondPeriod, int optInSignalPeriod, double outReal[] )
   {
      requireArgument("DOSC openAndFill", "inReal", inReal);
      requireHistory("DOSC openAndFill", inReal.length);
      int guardOutLen = openFillCount("DOSC openAndFill", inReal.length, doscLookback(optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod));
      requireLength("DOSC openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("DOSC openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return doscOpenAndFillInternal(inReal, 0, optInTimePeriod, optInFirstPeriod, optInSecondPeriod, optInSignalPeriod, outBegIdx, outNBElement, outReal);
   }
