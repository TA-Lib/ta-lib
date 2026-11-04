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
 *  103126 MF,CC  Creation (#570).
 */

   /**
    * Number of leading input bars {@link Core#macdv} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA and of the Average True
    *        Range (default 26; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int macdvLookback( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
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
      int lookbackLine;
      int lookbackATR;
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* The line needs the slow EMA and the Average True Range on the same bar,
       * so its first bar is the later of their two lookbacks. Each term is
       * exactly the lookback of the function it comes from and is never restated
       * here, which is what makes MACDV inherit TA_FUNC_UNST_EMA and
       * TA_FUNC_UNST_ATR from its callees.
       */
      lookbackLine = emaLookback(optInSlowPeriod);
      lookbackATR = atrLookback(optInSlowPeriod);
      if( lookbackATR > lookbackLine ) {
         lookbackLine = lookbackATR;
      }
      return lookbackLine + emaLookback(optInSignalPeriod) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#macdv}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA and of the Average True
    *        Range (default 26; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int macdvDisplayShift( int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, int outputIdx )
   {
      if( macdvLookback( optInFastPeriod, optInSlowPeriod, optInSignalPeriod ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 3 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode macdvImpl( int startIdx,
                      int endIdx,
                      double inHigh[],
                      double inLow[],
                      double inClose[],
                      int optInFastPeriod,
                      int optInSlowPeriod,
                      int optInSignalPeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outMACDV[],
                      double outMACDVSignal[],
                      double outMACDVHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevATR = 0;
      double prevSignal = 0;
      double lineValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      double slowBeta = 0;
      double fastBeta = 0;
      double signalBeta = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double periodTotal = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double spread = 0;
      double bandScale = 0;
      double periodScale = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackSlow = 0;
      int lookbackFast = 0;
      int lookbackATR = 0;
      int lineIdx = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outMACDV == outMACDVSignal || outMACDV == outMACDVHist || outMACDVSignal == outMACDVHist ) {
         return RetCode.BAD_PARAM ;
      }
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* ema.c's k and beta for the three EMA, macd.c's spelling. */
      slowBeta = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      slowK = 1.0 - slowBeta;
      if( slowBeta < 0.5 ) {
         slowBeta = 1.0 - slowK;
      }
      fastBeta = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      fastK = 1.0 - fastBeta;
      if( fastBeta < 0.5 ) {
         fastBeta = 1.0 - fastK;
      }
      /* A signal period of 1 disables signal-line smoothing: the signal IS the
       * MACD-V line and the histogram is exactly zero. The recursion below, at a
       * k of 1.0 and a beta of 0.0, does not keep the sign of a -0.0 line value;
       * hence the explicit arm at each step.
       */
      signalBeta = (double)(optInSignalPeriod - 1) / (double)(optInSignalPeriod + 1);
      signalK = 1.0 - signalBeta;
      if( signalBeta < 0.5 ) {
         signalBeta = 1.0 - signalK;
      }
      /* atr.c's Wilder coefficients: wAlpha derived FROM wBeta, never the
       * reverse, so that the pair is the one TA_ATR runs.
       */
      wBeta = (double)(optInSlowPeriod - 1) / (double)optInSlowPeriod;
      wAlpha = 1.0 - wBeta;
      /* An EMA of a constant can settle short of it by a number of ulps that
       * grows in proportion to the period.
       */
      periodScale = (double)(optInSlowPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackSlow = emaLookback(optInSlowPeriod);
      lookbackFast = emaLookback(optInFastPeriod);
      lookbackATR = atrLookback(optInSlowPeriod);
      lookbackTotal = macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
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
      /* Everything is computed in a single lockstep pass: each bar advances the
       * fast EMA, the slow EMA and the Average True Range, the scaled quotient
       * is the MACD-V line, and each line value is immediately fed into the
       * signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract with TA_EMA,
       * TA_MACD and TA_ATR (do not reorder or fuse operations):
       *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
       *  - True Range: start from high-low, then compare/replace with the two
       *    previous-close distances, in that order; Wilder smoothing in ONE
       *    statement.
       *  - Each recursion is seeded with the sum of its first 'period' inputs,
       *    accumulated from 0.0 in input order, divided by the period. The
       *    signal EMA is seeded the same way from the first 'signal period'
       *    line values.
       *  - The line is the spread divided by the Average True Range, then
       *    scaled: what a caller dividing TA_MACD by TA_ATR computes.
       *
       * In-place (an output == an input) is supported: outputs at [outIdx] are
       * written only after every input at or before bar startIdx+outIdx was
       * read, and the previous close is re-read from the input one bar back,
       * which no output store has reached.
       */
      /* Each leg is seeded at its OWN lookback and warmed up to lineIdx, the
       * first MACD-V line bar, so each is the shipped function entered at that
       * bar and nothing here over-warms the shorter one: a caller who wants a
       * leg converged sets that function's unstable period, exactly as when
       * calling it directly.
       */
      lineIdx = startIdx - lookbackSignal;
      /* Slow EMA. */
      today = lineIdx - lookbackSlow;
      tempReal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempReal += inClose[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      while( today <= lineIdx ) {
         tempReal = inClose[today++];
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      /* Fast EMA. */
      today = lineIdx - lookbackFast;
      tempReal = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += inClose[today++];
      }
      prevFast = tempReal / optInFastPeriod;
      while( today <= lineIdx ) {
         tempReal = inClose[today++];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
      }
      /* Average True Range: the first True Range consumes a previous close. */
      today = lineIdx - lookbackATR + 1;
      periodTotal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInSlowPeriod;
      while( today <= lineIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
      }
      /* Without price movement the line answers its neutral point, 0.0 (#570).
       * In a flat run the Wilder average only decays, to a subnormal floor,
       * while the spread stalls at the EMAs' rounding residue, so an exact test
       * lets their quotient grow until it overflows. Both are therefore tested
       * against the EMAs' own magnitude times the residue's period factor: that
       * also bounds |line| for any finite input, which a scale taken from the
       * close cannot, since a close may lie outside its bar and be zero.
       */
      spread = prevFast - prevSlow;
      bandScale = Math.abs(prevFast);
      if( Math.abs(prevSlow) > bandScale ) {
         bandScale = Math.abs(prevSlow);
      }
      bandScale = bandScale * periodScale;
      lineValue = 0.0;
      if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
         lineValue = spread / prevATR * 100.0;
      }
      /* Seed the signal EMA with a simple average of the first 'signal period'
       * line values, accumulated as they are produced.
       */
      prevSignal = 0.0;
      prevSignal += lineValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         tempReal = inClose[today];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         prevSignal += lineValue;
         today += 1;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      /* Advance everything in lockstep through the unstable period of the
       * signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         tempReal = inClose[today];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(signalBeta, prevSignal, signalK * lineValue);
         }
         today += 1;
      }
      /* Stable zone: keep advancing in lockstep and write the three outputs. */
      outMACDV[0] = lineValue;
      outMACDVSignal[0] = prevSignal;
      outMACDVHist[0] = lineValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         tempReal = inClose[today];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(signalBeta, prevSignal, signalK * lineValue);
         }
         outMACDV[outIdx] = lineValue;
         outMACDVSignal[outIdx] = prevSignal;
         outMACDVHist[outIdx] = lineValue - prevSignal;
         outIdx += 1;
         today += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode macdvImpl( int startIdx,
                      int endIdx,
                      float inHigh[],
                      float inLow[],
                      float inClose[],
                      int optInFastPeriod,
                      int optInSlowPeriod,
                      int optInSignalPeriod,
                      MInteger outBegIdx,
                      MInteger outNBElement,
                      double outMACDV[],
                      double outMACDVSignal[],
                      double outMACDVHist[] )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevATR = 0;
      double prevSignal = 0;
      double lineValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      double slowBeta = 0;
      double fastBeta = 0;
      double signalBeta = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double periodTotal = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double spread = 0;
      double bandScale = 0;
      double periodScale = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackSlow = 0;
      int lookbackFast = 0;
      int lookbackATR = 0;
      int lineIdx = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( outMACDV == outMACDVSignal || outMACDV == outMACDVHist || outMACDVSignal == outMACDVHist ) {
         return RetCode.BAD_PARAM ;
      }
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      slowBeta = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      slowK = 1.0 - slowBeta;
      if( slowBeta < 0.5 ) {
         slowBeta = 1.0 - slowK;
      }
      fastBeta = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      fastK = 1.0 - fastBeta;
      if( fastBeta < 0.5 ) {
         fastBeta = 1.0 - fastK;
      }
      signalBeta = (double)(optInSignalPeriod - 1) / (double)(optInSignalPeriod + 1);
      signalK = 1.0 - signalBeta;
      if( signalBeta < 0.5 ) {
         signalBeta = 1.0 - signalK;
      }
      wBeta = (double)(optInSlowPeriod - 1) / (double)optInSlowPeriod;
      wAlpha = 1.0 - wBeta;
      periodScale = (double)(optInSlowPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackSlow = emaLookback(optInSlowPeriod);
      lookbackFast = emaLookback(optInFastPeriod);
      lookbackATR = atrLookback(optInSlowPeriod);
      lookbackTotal = macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      lineIdx = startIdx - lookbackSignal;
      today = lineIdx - lookbackSlow;
      tempReal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempReal += (double)inClose[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      while( today <= lineIdx ) {
         tempReal = (double)inClose[today++];
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      today = lineIdx - lookbackFast;
      tempReal = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += (double)inClose[today++];
      }
      prevFast = tempReal / optInFastPeriod;
      while( today <= lineIdx ) {
         tempReal = (double)inClose[today++];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
      }
      today = lineIdx - lookbackATR + 1;
      periodTotal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInSlowPeriod;
      while( today <= lineIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
      }
      spread = prevFast - prevSlow;
      bandScale = Math.abs(prevFast);
      if( Math.abs(prevSlow) > bandScale ) {
         bandScale = Math.abs(prevSlow);
      }
      bandScale = bandScale * periodScale;
      lineValue = 0.0;
      if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
         lineValue = spread / prevATR * 100.0;
      }
      prevSignal = 0.0;
      prevSignal += lineValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         tempReal = (double)inClose[today];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         prevSignal += lineValue;
         today += 1;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      while( today <= startIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         tempReal = (double)inClose[today];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(signalBeta, prevSignal, signalK * lineValue);
         }
         today += 1;
      }
      outMACDV[0] = lineValue;
      outMACDVSignal[0] = prevSignal;
      outMACDVHist[0] = lineValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempLT = (double)inLow[today];
         tempHT = (double)inHigh[today];
         tempCY = (double)inClose[today - 1];
         tempReal = (double)inClose[today];
         greatest = tempHT - tempLT;
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(signalBeta, prevSignal, signalK * lineValue);
         }
         outMACDV[outIdx] = lineValue;
         outMACDVSignal[outIdx] = prevSignal;
         outMACDVHist[outIdx] = lineValue - prevSignal;
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast
    * and a slow EMA of the close over the Average True Range, times 100. Like
    * MACD it comes with an EMA-smoothed signal line and their histogram.
    * Dividing by the ATR expresses momentum in units of the instrument's own
    * volatility, so one reading means the same thing across instruments,
    * timeframes and volatility regimes, where the raw MACD spread does not.
    * Spiroglou reads the line in ranges: beyond 150 or -150 is an overextended
    * move, between -50 and 50 a ranging market, and the bands in between a
    * trend.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/macdv">ta-lib.org/functions/macdv</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The Average True Range is Wilder's, over the slow period: the publication's {@code ATR(26)} beside the 26-bar slow EMA, so one parameter sets both. Without price movement the line falls to 0, its neutral point.</li>
    * <li>The two price EMAs and the Average True Range each warm up as when calling {@code EMA} and {@code ATR} directly, so a caller who wants the line converged sets {@code TA_FUNC_UNST_EMA} and {@code TA_FUNC_UNST_ATR}, as for those functions.</li>
    * <li>If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one, as {@code MACD} does; the ATR period follows the slow period after the swap.</li>
    * <li>A signal period of 1 disables signal-line smoothing: the signal equals the MACD-V line and the histogram is zero.</li>
    * <li>The publication's MACD section writes the histogram as the signal line minus the MACD line; its reading of the histogram ("above 40" is stretched to the upside) holds only for MACD-V minus the signal, which is what is emitted here, as for {@code MACD}.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#macdvLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA and of the Average True
    *        Range (default 26; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outMACDV Fast EMA minus slow EMA, divided by the Average True
    *        Range, times 100. Must hold at least
    *        {@code endIdx - max(startIdx, macdvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outMACDVSignal EMA of the MACD-V line. Must hold at least
    *        {@code endIdx - max(startIdx, macdvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outMACDVHist MACD-V minus signal line. Must hold at least
    *        {@code endIdx - max(startIdx, macdvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#macd
    * @see Core#atr
    * @see Core#ema
    * @see Core#apo
    * @see Core#ppo
    */
   public OutRange macdv( int startIdx,
                          int endIdx,
                          double inHigh[],
                          double inLow[],
                          double inClose[],
                          int optInFastPeriod,
                          int optInSlowPeriod,
                          int optInSignalPeriod,
                          double outMACDV[],
                          double outMACDVSignal[],
                          double outMACDVHist[] )
   {
      requireIndexRange("MACDV", startIdx, endIdx);
      int guardStart = clampedStart("MACDV", startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACDV", "inHigh", inHigh, guardInLen);
      requireLength("MACDV", "inLow", inLow, guardInLen);
      requireLength("MACDV", "inClose", inClose, guardInLen);
      requireLength("MACDV", "outMACDV", outMACDV, guardOutLen);
      requireLength("MACDV", "outMACDVSignal", outMACDVSignal, guardOutLen);
      requireLength("MACDV", "outMACDVHist", outMACDVHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = macdvImpl(startIdx, endIdx, inHigh, inLow, inClose, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MACDV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast
    * and a slow EMA of the close over the Average True Range, times 100. Like
    * MACD it comes with an EMA-smoothed signal line and their histogram.
    * Dividing by the ATR expresses momentum in units of the instrument's own
    * volatility, so one reading means the same thing across instruments,
    * timeframes and volatility regimes, where the raw MACD spread does not.
    * Spiroglou reads the line in ranges: beyond 150 or -150 is an overextended
    * move, between -50 and 50 a ranging market, and the bands in between a
    * trend.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/macdv">ta-lib.org/functions/macdv</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The Average True Range is Wilder's, over the slow period: the publication's {@code ATR(26)} beside the 26-bar slow EMA, so one parameter sets both. Without price movement the line falls to 0, its neutral point.</li>
    * <li>The two price EMAs and the Average True Range each warm up as when calling {@code EMA} and {@code ATR} directly, so a caller who wants the line converged sets {@code TA_FUNC_UNST_EMA} and {@code TA_FUNC_UNST_ATR}, as for those functions.</li>
    * <li>If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one, as {@code MACD} does; the ATR period follows the slow period after the swap.</li>
    * <li>A signal period of 1 disables signal-line smoothing: the signal equals the MACD-V line and the histogram is zero.</li>
    * <li>The publication's MACD section writes the histogram as the signal line minus the MACD line; its reading of the histogram ("above 40" is stretched to the upside) holds only for MACD-V minus the signal, which is what is emitted here, as for {@code MACD}.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#macdvLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param optInFastPeriod Period of the fast EMA (default 12; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow EMA and of the Average True
    *        Range (default 26; range 2..100000; {@code Integer.MIN_VALUE} selects the
    *        default).
    * @param optInSignalPeriod Smoothing period of the signal line (default 9;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outMACDV Fast EMA minus slow EMA, divided by the Average True
    *        Range, times 100. Must hold at least
    *        {@code endIdx - max(startIdx, macdvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outMACDVSignal EMA of the MACD-V line. Must hold at least
    *        {@code endIdx - max(startIdx, macdvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outMACDVHist MACD-V minus signal line. Must hold at least
    *        {@code endIdx - max(startIdx, macdvLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    *        same. Checked before anything is written, so a rejected call leaves every
    *        buffer untouched.
    *
    * @see Core#macd
    * @see Core#atr
    * @see Core#ema
    * @see Core#apo
    * @see Core#ppo
    */
   public OutRange macdv( int startIdx,
                          int endIdx,
                          float inHigh[],
                          float inLow[],
                          float inClose[],
                          int optInFastPeriod,
                          int optInSlowPeriod,
                          int optInSignalPeriod,
                          double outMACDV[],
                          double outMACDVSignal[],
                          double outMACDVHist[] )
   {
      requireIndexRange("MACDV", startIdx, endIdx);
      int guardStart = clampedStart("MACDV", startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("MACDV", "inHigh", inHigh, guardInLen);
      requireLength("MACDV", "inLow", inLow, guardInLen);
      requireLength("MACDV", "inClose", inClose, guardInLen);
      requireLength("MACDV", "outMACDV", outMACDV, guardOutLen);
      requireLength("MACDV", "outMACDVSignal", outMACDVSignal, guardOutLen);
      requireLength("MACDV", "outMACDVHist", outMACDVHist, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = macdvImpl(startIdx, endIdx, inHigh, inLow, inClose, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("MACDV", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live MACDV stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#macdv} over the same series.
    * Open with {@link Core#macdvOpen}; there is no close — the handle is
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
   public static final class MacdvStream {
      private Core core;
      private int optInFastPeriod;
      private int optInSlowPeriod;
      private int optInSignalPeriod;
      private double prevFast;
      private double prevSlow;
      private double prevATR;
      private double prevSignal;
      private double slowK;
      private double fastK;
      private double signalK;
      private double slowBeta;
      private double fastBeta;
      private double signalBeta;
      private double wAlpha;
      private double wBeta;
      private double periodScale;
      private double lag1_inClose;
      private double cur_outMACDV;
      private double cur_outMACDVSignal;
      private double cur_outMACDVHist;
      private int outRangeBegIdx;
      private int outRangeCount;

      private MacdvStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#macdv} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(MacdvOut)} keeps answering the previous
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
            throw failure("MACDV advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private MacdvStream( MacdvStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInSignalPeriod = other.optInSignalPeriod;
         this.prevFast = other.prevFast;
         this.prevSlow = other.prevSlow;
         this.prevATR = other.prevATR;
         this.prevSignal = other.prevSignal;
         this.slowK = other.slowK;
         this.fastK = other.fastK;
         this.signalK = other.signalK;
         this.slowBeta = other.slowBeta;
         this.fastBeta = other.fastBeta;
         this.signalBeta = other.signalBeta;
         this.wAlpha = other.wAlpha;
         this.wBeta = other.wBeta;
         this.periodScale = other.periodScale;
         this.lag1_inClose = other.lag1_inClose;
         this.cur_outMACDV = other.cur_outMACDV;
         this.cur_outMACDVSignal = other.cur_outMACDVSignal;
         this.cur_outMACDVHist = other.cur_outMACDVHist;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(MacdvOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, double inClose, MacdvOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("MACDV update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("MACDV update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("MACDV update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.macdvStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         out.macdv = this.cur_outMACDV;
         out.macdvSignal = this.cur_outMACDVSignal;
         out.macdvHist = this.cur_outMACDVHist;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, double inClose, MacdvOut out ) {
         requireArgument("MACDV peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("MACDV peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         MacdvStream sp = this;
         double lineValue = 0.0;
         double tempReal = 0.0;
         double val2 = 0.0;
         double val3 = 0.0;
         double greatest = 0.0;
         double spread = 0.0;
         double bandScale = 0.0;
         double tempCY = 0.0;
         double tempLT = 0.0;
         double tempHT = 0.0;
         double cur_outMACDV = 0.0;
         double cur_outMACDVHist = 0.0;
         double cur_outMACDVSignal = 0.0;
         double prevATR = sp.prevATR;
         double prevFast = sp.prevFast;
         double prevSignal = sp.prevSignal;
         double prevSlow = sp.prevSlow;
         tempLT = inLow;
         tempHT = inHigh;
         tempCY = sp.lag1_inClose;
         tempReal = inClose;
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(sp.wBeta, prevATR, sp.wAlpha * greatest);
         prevFast = Math.fma(sp.fastBeta, prevFast, sp.fastK * tempReal);
         prevSlow = Math.fma(sp.slowBeta, prevSlow, sp.slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * sp.periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( sp.optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(sp.signalBeta, prevSignal, sp.signalK * lineValue);
         }
         cur_outMACDV = lineValue;
         cur_outMACDVSignal = prevSignal;
         cur_outMACDVHist = lineValue - prevSignal;
         out.macdv = cur_outMACDV;
         out.macdvSignal = cur_outMACDVSignal;
         out.macdvHist = cur_outMACDVHist;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( MacdvOut out ) {
         requireArgument("MACDV value", "out", out);
         out.macdv = this.cur_outMACDV;
         out.macdvSignal = this.cur_outMACDVSignal;
         out.macdvHist = this.cur_outMACDVHist;
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
      public MacdvStream clone() {
         return new MacdvStream(this);
      }
   }

   /**
    * The outputs of one MACDV bar, written by the stream into an object the
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
   public static final class MacdvOut {
      /** Fast EMA minus slow EMA, divided by the Average True Range, times 100. */
      public double macdv;
      /** EMA of the MACD-V line. */
      public double macdvSignal;
      /** MACD-V minus signal line. */
      public double macdvHist;
   }
   private void macdvStepImpl( MacdvStream sp, double inHigh, double inLow, double inClose )
   {
      double lineValue = 0.0;
      double tempReal = 0.0;
      double val2 = 0.0;
      double val3 = 0.0;
      double greatest = 0.0;
      double spread = 0.0;
      double bandScale = 0.0;
      double tempCY = 0.0;
      double tempLT = 0.0;
      double tempHT = 0.0;
      tempLT = inLow;
      tempHT = inHigh;
      tempCY = sp.lag1_inClose;
      tempReal = inClose;
      greatest = tempHT - tempLT;
      /* val1 */
      val2 = Math.abs(tempCY - tempHT);
      if( val2 > greatest ) {
         greatest = val2;
      }
      val3 = Math.abs(tempCY - tempLT);
      if( val3 > greatest ) {
         greatest = val3;
      }
      sp.prevATR = Math.fma(sp.wBeta, sp.prevATR, sp.wAlpha * greatest);
      sp.prevFast = Math.fma(sp.fastBeta, sp.prevFast, sp.fastK * tempReal);
      sp.prevSlow = Math.fma(sp.slowBeta, sp.prevSlow, sp.slowK * tempReal);
      spread = sp.prevFast - sp.prevSlow;
      bandScale = Math.abs(sp.prevFast);
      if( Math.abs(sp.prevSlow) > bandScale ) {
         bandScale = Math.abs(sp.prevSlow);
      }
      bandScale = bandScale * sp.periodScale;
      lineValue = 0.0;
      if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(sp.prevATR) <= 0.00000000000001 * (bandScale)) && sp.prevATR >= 2.2250738585072014e-308 ) {
         lineValue = spread / sp.prevATR * 100.0;
      }
      if( sp.optInSignalPeriod == 1 ) {
         sp.prevSignal = lineValue;
      } else {
         sp.prevSignal = Math.fma(sp.signalBeta, sp.prevSignal, sp.signalK * lineValue);
      }
      sp.cur_outMACDV = lineValue;
      sp.cur_outMACDVSignal = sp.prevSignal;
      sp.cur_outMACDVHist = lineValue - sp.prevSignal;
      sp.lag1_inClose = inClose;
   }
   private RetCode macdvOpenImpl( MacdvStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACDV[], double outMACDVSignal[], double outMACDVHist[], int outStride )
   {
      double prevFast = 0;
      double prevSlow = 0;
      double prevATR = 0;
      double prevSignal = 0;
      double lineValue = 0;
      double tempReal = 0;
      double slowK = 0;
      double fastK = 0;
      double signalK = 0;
      double slowBeta = 0;
      double fastBeta = 0;
      double signalBeta = 0;
      double wAlpha = 0;
      double wBeta = 0;
      double periodTotal = 0;
      double val2 = 0;
      double val3 = 0;
      double greatest = 0;
      double spread = 0;
      double bandScale = 0;
      double periodScale = 0;
      double tempCY = 0;
      double tempLT = 0;
      double tempHT = 0;
      int i = 0;
      int today = 0;
      int outIdx = 0;
      int tempInteger = 0;
      int lookbackTotal = 0;
      int lookbackSignal = 0;
      int lookbackSlow = 0;
      int lookbackFast = 0;
      int lookbackATR = 0;
      int lineIdx = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSignalPeriod == Integer.MIN_VALUE ) {
         optInSignalPeriod = 9;
      } else if( optInSignalPeriod < 1 || optInSignalPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* ema.c's k and beta for the three EMA, macd.c's spelling. */
      slowBeta = (double)(optInSlowPeriod - 1) / (double)(optInSlowPeriod + 1);
      slowK = 1.0 - slowBeta;
      if( slowBeta < 0.5 ) {
         slowBeta = 1.0 - slowK;
      }
      fastBeta = (double)(optInFastPeriod - 1) / (double)(optInFastPeriod + 1);
      fastK = 1.0 - fastBeta;
      if( fastBeta < 0.5 ) {
         fastBeta = 1.0 - fastK;
      }
      /* A signal period of 1 disables signal-line smoothing: the signal IS the
       * MACD-V line and the histogram is exactly zero. The recursion below, at a
       * k of 1.0 and a beta of 0.0, does not keep the sign of a -0.0 line value;
       * hence the explicit arm at each step.
       */
      signalBeta = (double)(optInSignalPeriod - 1) / (double)(optInSignalPeriod + 1);
      signalK = 1.0 - signalBeta;
      if( signalBeta < 0.5 ) {
         signalBeta = 1.0 - signalK;
      }
      /* atr.c's Wilder coefficients: wAlpha derived FROM wBeta, never the
       * reverse, so that the pair is the one TA_ATR runs.
       */
      wBeta = (double)(optInSlowPeriod - 1) / (double)optInSlowPeriod;
      wAlpha = 1.0 - wBeta;
      /* An EMA of a constant can settle short of it by a number of ulps that
       * grows in proportion to the period.
       */
      periodScale = (double)(optInSlowPeriod + 1);
      lookbackSignal = emaLookback(optInSignalPeriod);
      lookbackSlow = emaLookback(optInSlowPeriod);
      lookbackFast = emaLookback(optInFastPeriod);
      lookbackATR = atrLookback(optInSlowPeriod);
      lookbackTotal = macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
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
      /* Everything is computed in a single lockstep pass: each bar advances the
       * fast EMA, the slow EMA and the Average True Range, the scaled quotient
       * is the MACD-V line, and each line value is immediately fed into the
       * signal EMA. No temporary buffers.
       *
       * The arithmetic order below is the bit-exactness contract with TA_EMA,
       * TA_MACD and TA_ATR (do not reorder or fuse operations):
       *  - EMA recursion: k*x + beta*prev, with ema.c's k and beta.
       *  - True Range: start from high-low, then compare/replace with the two
       *    previous-close distances, in that order; Wilder smoothing in ONE
       *    statement.
       *  - Each recursion is seeded with the sum of its first 'period' inputs,
       *    accumulated from 0.0 in input order, divided by the period. The
       *    signal EMA is seeded the same way from the first 'signal period'
       *    line values.
       *  - The line is the spread divided by the Average True Range, then
       *    scaled: what a caller dividing TA_MACD by TA_ATR computes.
       *
       * In-place (an output == an input) is supported: outputs at [outIdx] are
       * written only after every input at or before bar startIdx+outIdx was
       * read, and the previous close is re-read from the input one bar back,
       * which no output store has reached.
       */
      /* Each leg is seeded at its OWN lookback and warmed up to lineIdx, the
       * first MACD-V line bar, so each is the shipped function entered at that
       * bar and nothing here over-warms the shorter one: a caller who wants a
       * leg converged sets that function's unstable period, exactly as when
       * calling it directly.
       */
      lineIdx = startIdx - lookbackSignal;
      /* Slow EMA. */
      today = lineIdx - lookbackSlow;
      tempReal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempReal += inClose[today++];
      }
      prevSlow = tempReal / optInSlowPeriod;
      while( today <= lineIdx ) {
         tempReal = inClose[today++];
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
      }
      /* Fast EMA. */
      today = lineIdx - lookbackFast;
      tempReal = 0.0;
      i = optInFastPeriod;
      while( i-- > 0 ) {
         tempReal += inClose[today++];
      }
      prevFast = tempReal / optInFastPeriod;
      while( today <= lineIdx ) {
         tempReal = inClose[today++];
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
      }
      /* Average True Range: the first True Range consumes a previous close. */
      today = lineIdx - lookbackATR + 1;
      periodTotal = 0.0;
      i = optInSlowPeriod;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         periodTotal += greatest;
         today += 1;
      }
      prevATR = periodTotal / optInSlowPeriod;
      while( today <= lineIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         today += 1;
      }
      /* Without price movement the line answers its neutral point, 0.0 (#570).
       * In a flat run the Wilder average only decays, to a subnormal floor,
       * while the spread stalls at the EMAs' rounding residue, so an exact test
       * lets their quotient grow until it overflows. Both are therefore tested
       * against the EMAs' own magnitude times the residue's period factor: that
       * also bounds |line| for any finite input, which a scale taken from the
       * close cannot, since a close may lie outside its bar and be zero.
       */
      spread = prevFast - prevSlow;
      bandScale = Math.abs(prevFast);
      if( Math.abs(prevSlow) > bandScale ) {
         bandScale = Math.abs(prevSlow);
      }
      bandScale = bandScale * periodScale;
      lineValue = 0.0;
      if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
         lineValue = spread / prevATR * 100.0;
      }
      /* Seed the signal EMA with a simple average of the first 'signal period'
       * line values, accumulated as they are produced.
       */
      prevSignal = 0.0;
      prevSignal += lineValue;
      i = optInSignalPeriod - 1;
      while( i-- > 0 ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         tempReal = inClose[today];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         prevSignal += lineValue;
         today += 1;
      }
      prevSignal = prevSignal / optInSignalPeriod;
      /* Advance everything in lockstep through the unstable period of the
       * signal EMA, up to the first output bar.
       */
      while( today <= startIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         tempReal = inClose[today];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(signalBeta, prevSignal, signalK * lineValue);
         }
         today += 1;
      }
      /* Stable zone: keep advancing in lockstep and write the three outputs. */
      outMACDV[0 * outStride] = lineValue;
      outMACDVSignal[0 * outStride] = prevSignal;
      outMACDVHist[0 * outStride] = lineValue - prevSignal;
      outIdx = 1;
      while( today <= endIdx ) {
         tempLT = inLow[today];
         tempHT = inHigh[today];
         tempCY = inClose[today - 1];
         tempReal = inClose[today];
         greatest = tempHT - tempLT;
         /* val1 */
         val2 = Math.abs(tempCY - tempHT);
         if( val2 > greatest ) {
            greatest = val2;
         }
         val3 = Math.abs(tempCY - tempLT);
         if( val3 > greatest ) {
            greatest = val3;
         }
         prevATR = Math.fma(wBeta, prevATR, wAlpha * greatest);
         prevFast = Math.fma(fastBeta, prevFast, fastK * tempReal);
         prevSlow = Math.fma(slowBeta, prevSlow, slowK * tempReal);
         spread = prevFast - prevSlow;
         bandScale = Math.abs(prevFast);
         if( Math.abs(prevSlow) > bandScale ) {
            bandScale = Math.abs(prevSlow);
         }
         bandScale = bandScale * periodScale;
         lineValue = 0.0;
         if( !(Math.abs(spread) <= 0.00000000000001 * (bandScale)) && !(Math.abs(prevATR) <= 0.00000000000001 * (bandScale)) && prevATR >= 2.2250738585072014e-308 ) {
            lineValue = spread / prevATR * 100.0;
         }
         if( optInSignalPeriod == 1 ) {
            prevSignal = lineValue;
         } else {
            prevSignal = Math.fma(signalBeta, prevSignal, signalK * lineValue);
         }
         outMACDV[outIdx * outStride] = lineValue;
         outMACDVSignal[outIdx * outStride] = prevSignal;
         outMACDVHist[outIdx * outStride] = lineValue - prevSignal;
         outIdx += 1;
         today += 1;
      }
      /* All done! Indicate the output limits and return success. */
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInSignalPeriod = optInSignalPeriod;
      sp.prevFast = prevFast;
      sp.prevSlow = prevSlow;
      sp.prevATR = prevATR;
      sp.prevSignal = prevSignal;
      sp.slowK = slowK;
      sp.fastK = fastK;
      sp.signalK = signalK;
      sp.slowBeta = slowBeta;
      sp.fastBeta = fastBeta;
      sp.signalBeta = signalBeta;
      sp.wAlpha = wAlpha;
      sp.wBeta = wBeta;
      sp.periodScale = periodScale;
      sp.lag1_inClose = inClose[historyLen - 1];
      sp.cur_outMACDV = outMACDV[(outNBElement.value - 1) * outStride];
      sp.cur_outMACDVSignal = outMACDVSignal[(outNBElement.value - 1) * outStride];
      sp.cur_outMACDVHist = outMACDVHist[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* macdvOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   MacdvStream macdvOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, MInteger outBegIdx, MInteger outNBElement, double outMACDV[], double outMACDVSignal[], double outMACDVHist[] )
   {
      MacdvStream sp = new MacdvStream(this);
      RetCode retCode = macdvOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MACDV openAndFill", inHigh.length, startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      }
      throw streamFailure("MACDV openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind macdvOpen (composition seam). */
   MacdvStream macdvOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      MacdvStream sp = new MacdvStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMACDV = new double[1];
      double[] sink_outMACDVSignal = new double[1];
      double[] sink_outMACDVHist = new double[1];
      RetCode retCode = macdvOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, sink_outMACDV, sink_outMACDVSignal, sink_outMACDVHist, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("MACDV open", inHigh.length, startIdx, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      }
      throw streamFailure("MACDV open", retCode);
   }
   /**
    * Open a live MACDV stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#macdv} at that bar.
    * <p>The history must hold at least {@code macdvLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link InsufficientHistoryException} too, and a null argument
    * {@link IllegalArgumentException}, both ahead of everything above.
    */
   public MacdvStream macdvOpen( double inHigh[], double inLow[], double inClose[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod )
   {
      requireArgument("MACDV open", "inHigh", inHigh);
      requireHistory("MACDV open", inHigh.length);
      requireArgument("MACDV open", "inLow", inLow);
      requireArgument("MACDV open", "inClose", inClose);
      requireHistoryLength("MACDV open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("MACDV open", "inClose", inClose.length, inHigh.length);
      return macdvOpenInternal(inHigh, inLow, inClose, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod);
   }
   /**
    * {@link Core#macdvOpen} that also fills the output array(s) bit-identically
    * to {@link Core#macdv} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link MacdvStream#outRange()}.
    */
   public MacdvStream macdvOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInFastPeriod, int optInSlowPeriod, int optInSignalPeriod, double outMACDV[], double outMACDVSignal[], double outMACDVHist[] )
   {
      requireArgument("MACDV openAndFill", "inHigh", inHigh);
      requireHistory("MACDV openAndFill", inHigh.length);
      requireArgument("MACDV openAndFill", "inLow", inLow);
      requireArgument("MACDV openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("MACDV openAndFill", inHigh.length, macdvLookback(optInFastPeriod, optInSlowPeriod, optInSignalPeriod));
      requireHistoryLength("MACDV openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("MACDV openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("MACDV openAndFill", "outMACDV", outMACDV, guardOutLen);
      requireLength("MACDV openAndFill", "outMACDVSignal", outMACDVSignal, guardOutLen);
      requireLength("MACDV openAndFill", "outMACDVHist", outMACDVHist, guardOutLen);
      if( (Object)outMACDV == (Object)inHigh || (Object)outMACDV == (Object)inLow || (Object)outMACDV == (Object)inClose || (Object)outMACDVSignal == (Object)inHigh || (Object)outMACDVSignal == (Object)inLow || (Object)outMACDVSignal == (Object)inClose || (Object)outMACDVHist == (Object)inHigh || (Object)outMACDVHist == (Object)inLow || (Object)outMACDVHist == (Object)inClose || (Object)outMACDV == (Object)outMACDVSignal || (Object)outMACDV == (Object)outMACDVHist || (Object)outMACDVSignal == (Object)outMACDVHist ) {
         throw streamFailure("MACDV openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return macdvOpenAndFillInternal(inHigh, inLow, inClose, 0, optInFastPeriod, optInSlowPeriod, optInSignalPeriod, outBegIdx, outNBElement, outMACDV, outMACDVSignal, outMACDVHist);
   }
